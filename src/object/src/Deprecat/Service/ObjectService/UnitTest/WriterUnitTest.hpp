
#pragma once

#include "Advance/Pointer.hpp"
#include "ObjectUnitTest.hpp"

namespace object_mock {
	/** write test loop */
	const int c_small_unit_size = c_length_1K * 8;
	const int c_small_write_loop = 2;
	const int c_multi_write_loop = 20;

	/** recover prepare */
	const int c_fixed_unit = 1000;
	const int c_prepare_count = 5;
	const int c_crash_data_head_index = 1;
	const int c_crash_data_last_index = 3;
	const int c_crash_data_length_index = c_prepare_count;
}
using namespace object_mock;


/**
 * mock reader
 **/
class MockReader : public Reader
{
public:
	MockReader(ObjectConfig* config)
		: Reader(config), mMutex(""), mAlloc(0) {}
public:
	/**
	 * mock for none alloc
	 **/
	void	MockNone() { mAlloc = -1; }

	/**
	 * fix next unit
	 **/
	void	FixUnit(int64_t index) { mAlloc = index; }

public:
	/**
	 * alloc unit
	 **/
	virtual int	FetchUnit(UnitIndex& index, int wait) {
		Mutex::Locker lock(mMutex);
		if (mAlloc < 0) {
			return -1;
		}
		index.index = ++mAlloc;
		return 0;
	}

	/**
	 * finish unit
	 */
	virtual int CommitObject(Object* object) {
		return 0;
	}

	virtual int	RecoverDone(const UnitIndex& index, bool success) {
		return 0;
	}

protected:
	Mutex	mMutex;
	int64_t	mAlloc;
};

class WriterTest : public ObjectTest
{
public:
	WriterTest() : mWriter(&mConfig) {

		ResetEnvironment();
		mConfig.writer.thread = 3;
	}

	virtual ~WriterTest() {
		/** stop and wait */
		mWriter.Stop(true);

		GetControl()->SetReader(NULL);
		reset(mReader);
	}

public:
	/**
	 * reset env, remove file dir
	 **/
	void	ResetEnvironment() {
		g_source.Init();
		::traverse_rmdir(mConfig.global.root);

		mReader = new MockReader(&mConfig);
		GetControl()->SetReader(mReader);
	}

	/**
	 * start for test
	 **/
	void	Start() { mWriter.Start(); }

public:
	/**
	 * do test work
	 **/
	void	DoWrite(int count = 0) {
		do {
			Object* object = g_source.Next();
			mWriter.Put(object);
		} while (--count > 0);
	}

	void	MockUnitSmall() {
		mConfig.writer.unit = c_small_unit_size;
	}

	void	MockNoneUnit() {
		mReader->FixUnit(-1);
		mConfig.writer.thread = 1;
	}
protected:
	MockReader* mReader	= {NULL};
	Writer		mWriter;
};

/**
 * writer recover test
 **/
class WriterRecover : public WriterTest
{
public:
	WriterRecover() : mUnit(&mWriter) {
		MockOneUnit();

		g_mock.SetData(WT_test_recover);
	}

public:
	/**
	 * mocker and write use same unit file
	 **/
	void	MockOneUnit() {
		mReader->FixUnit(c_fixed_unit);
		mConfig.writer.thread = 1;
	}

	/**
	 * start after mock unit work done
	 **/
	void	Start() {
		/** cancelled by MockUnit, reset value */
		mConfig.writer.io.direct = true;

		/** reset to fixed unit */
		mReader->FixUnit(c_fixed_unit);
		mUnit.Clear();

		WriterTest::Start();
	}

public:
	/**
	 * mock unit for change unit file
	 **/
	class MockUnit : public ObjectUnit
	{
	public:
		MockUnit(Writer* writer)
			: ObjectUnit(writer)
		{
			/** mocker not use direct */
			writer->Config().io.direct = false;
		}

	public:
		/**
		 * open file and write head
		 **/
		void	Prepare(int count = 0) {
			if (count == 0) {
				count = c_prepare_count;
			}
			while (count-- > 0) {
				Object* object = g_source.Next();
				assert(Write(object) == 0);
				object->Dec();
			}
		}

		void	SetLength(int64_t length) {
			assert(mFile.trunc(length) == 0);
		}

	public:
		void	SetEmpty() {
			Prepare();
			SetLength(0);
		}

        void    Trunc(int64_t length) {
            Prepare();
            SetLength(length);
        }

		void	OnlyHead() {
			Prepare();
			SetLength(object::c_unit_head_size);
		}

		void	SetEmptyData() {
			Prepare();
			SetLength(object::c_unit_head_size);
			assert(mFile.seek(object::c_unit_head_size) > 0);
			stack_align(data, c_length_4K, c_page_size);
			assert(mFile.write(data, c_length_4K) > 0);
		}

		void	CrashHead() {
			Prepare();
			stack_align(data, c_length_4K, c_page_size);
			common::tool::fill_data(data, c_length_4K);
			assert(mFile.seek(0) == 0);
			assert(mFile.write(data, c_length_4K) > 0);
		}

		void	CrashObjectHeadMagic(int count) {
			Prepare();

			int64_t pos = MoveCount(count);
			assert(mFile.seek(pos + OFFSET(Object::Head, magic[2])) > 0);
			int value = 0x98AC2309;
			assert(mFile.write(&value, sizeof(value)) == sizeof(value));
		}

		void	CrashObjectLastBlock(int count) {
			Prepare();

			int64_t pos = MoveCount(count);
			assert(mFile.seek(pos + head.CheckPos() + 56) > 0);
			int value = 0x98AC2309;
			assert(mFile.write(&value, sizeof(value)) == sizeof(value));
		}

		void	CrashObjectLength(int count) {
			Prepare();
			int64_t pos = MoveCount(count);
			SetLength(pos + head.CheckPos() + 56);
		}

		int		WriteUnitFull() {
			/** reduce size and test time */
			mWriter->Config()->unit = c_length_1M - 897;
			Prepare();

			int count = c_prepare_count;
			while (mIndex.curr + c_data_range[1] < mIndex.size) {
				Prepare(1);
				count++;
			}

			int len = mIndex.size - mIndex.curr - object::c_object_head_size - 1;
			if (len > 0) {
				Object* object = g_source.Next(len);
				assert(Write(object) == 0);
				object->Dec();
				count++;
			}
			return count;
		}
	protected:
		/**
		 * move to object
		 **/
		int64_t	MoveCount(int count) {
			assert(OpenFile(mIndex) == 0);
			mLength = mRecover.file_len();

			stack_align(data, object::c_object_head_size, c_page_size);
			int64_t pos = object::c_unit_head_size;
			int64_t last = 0;

			while (count-- > 0) {
				last = pos;
				pos = RecoverNext(pos, head, data);
				assert(pos > 0);
			}
			FileSwitch();
			return last;
		}

	protected:
		Object::Head head;
	};
protected:
	MockUnit	mUnit;
};
