
#define LOG_CODE 0

#include "Common/Display.hpp"
#include "Advance/Pointer.hpp"
#include "UnitTest/Mock.hpp"

#include "ObjectService/LogWriter.hpp"
#include "ObjectService/Config.hpp"
#include "ObjectService/Writer.hpp"
#include "ObjectService/Statistic.hpp"
#include "ObjectService/ObjectUnit.hpp"

using namespace common;

const int ObjectUnit::Head::s_magic[] = {
	0x1111111, 0x2222222, 0x3333333, 0x4444444
};

ObjectUnit::ObjectUnit(Writer* writer)
	: mWriter(writer)
{
}

const string&
ObjectUnit::String()
{
	int64_t size = 0;
	FileBase* file = NULL;
	if (mMode == OM_recover) {
		file = &mRecover;
		size = mLength;

	} else {
		file = &mFile;
		size = mIndex.curr;
	}
	return format(mString, "unit path %s size %s",
		file->path().c_str(), string_size(size).c_str());
}

int
ObjectUnit::Errno(int error, int syserr)
{
	mCurr.Errno(error, syserr);

	if (mObject) {
		if (error == OS_done) {
			MockWakeup(WT_object_done);
			mObject->mError = 0;

		} else {
			MockWakeup(error);
			mObject->mError = error;
		}
		mObject = NULL;
	}
    return Errno();
}

void
ObjectUnit::Clear()
{
	mIndex.Reset();
	mIndex.curr = 0;
	mFile.close();

	mRecover.close();
	mLength = 0;

	mData.clear();
	mPost = 0;
}

ObjectConfig&
ObjectUnit::GlobalConfig()
{
	return mWriter->GlobalConfig();
}

ObjectConfig::Writer&
ObjectUnit::Config()
{
	return mWriter->Config();
}

string
ObjectUnit::Path(const UnitIndex& index)
{
	return GlobalConfig().global.root + UnitPath(index);
}

int
ObjectUnit::Write(Object* object)
{
	Errno(OS_null);

	#if OBJECT_PERFORM
		mTimer.reset(true);
		mTimer.next("wait last");
	#endif

	if (Prepare(object) != 0) {
		return Errno();
	}
	
	#if OBJECT_PERFORM
		mTimer.next("prepare");
	#endif
	if (WriteObject() != 0) {
		return Errno();
	}

	#if OBJECT_PERFORM
		mTimer.next("write data", WRITE_TIME);
		mTimer.bomb("object unit write object");
	#endif
	return Errno(OS_done);
}

int
ObjectUnit::FetchUnit()
{
	UnitIndex old = mIndex;

	Clear();

	if (!old.Empty()) {
		success(mWriter->CommitUnit(old) == 0);
	}

	if (mWriter->FetchUnit(mIndex, Config().task.wait) != 0) {
		trace("fetch unit failed");
		return Errno(OS_alloc_unit);
	}

	#if OBJECT_PERFORM
		mTimer.next("fetch unit");
	#endif

	if (old.Empty()) {
		log_debug("first start, alloc new " << StringIndex(mIndex));

	} else {
		log_debug(StringIndex(old) << " full, alloc new " << StringIndex(mIndex));
	}
	mIndex.size = Config().unit;

	/** test for recover */
	if (MockData(WT_test_recover)) {
		Recover(mIndex);
	}
    return 0;
}

int
ObjectUnit::Prepare(Object* object)
{
	mObject = object;
	object->Ajustment();

	#if OBJECT_PERFORM
		mTimer.next("adjust");
	#endif

	if (NewUnit(object)) {
		/** get new unit */
		if (FlushData() == 0 &&
			FetchUnit() == 0 &&
			OpenFile(mIndex) == 0)
		{
			FileSwitch();
		}
	}
    return Errno();
}

void
ObjectUnit::FileSwitch()
{
	if (mLength == 0) {
		assert(RecoverHead() == 0);
		#if OBJECT_PERFORM
			mTimer.next("recover head");
		#endif
	}

	mIndex.curr = mLength;
	mFile = mRecover;
	mLength = 0;
}

void
ObjectUnit::RecoverMode(bool set)
{
	if (set) {
		mMode = OM_recover;
	} else {
		mMode = OM_null;
	}
}

int
ObjectUnit::Recover(UnitIndex& index)
{
	RecoverMode(true);

	if (OpenFile(index) == 0) {
		if (RecoverHead() == 0 &&
			RecoverData(index) == 0)
		{
			RecoverDone(index, true);
		}
		mRecover.close();
	}

	RecoverMode(false);
	return Errno();
}

int
ObjectUnit::OpenFile(const UnitIndex& index)
{
	int ret = 0;
	std::string path = Path(index);

	int flag = make_bit(FileBase::op_creat, FileBase::op_write,
			Config().io.sync ? FileBase::op_sync : FileBase::op_null,
			Config().io.direct ? FileBase::op_direct : FileBase::op_null);

    if ((ret = mRecover.open(path, flag)) != 0 ||
    	MockEvent(OS_unit_open))
    {
		log_warn("open unit " << path << ", but failed, " << syserr());
		return Errno(OS_unit_open);

	} else if ((ret = mRecover.seek()) < 0) {
		log_warn("open and seek unit " << path << ", but failed, " << syserr());
		return Errno(OS_unit_seek);
	}

    mLength = mRecover.file_len();
    MockWakeup(WT_unit_roll);

	#if OBJECT_PERFORM
		mTimer.next("open unit");
	#endif
    return 0;
}

int
ObjectUnit::Head::Read(byte_t* data, uint32_t len)
{
	return memcmp(data, this, sizeof(Head));
}

int
ObjectUnit::Head::Write(byte_t* data, uint32_t len)
{
	memcpy(data, Start(), sizeof(Head));
	return 0;
}

int
ObjectUnit::RecoverHead()
{
	struct Head head;
	stack_align(data, object::c_unit_head_size, c_page_size);

	if (mRecover.seek(0) < 0) {
		log_info("unit recover, seek to 0 but failed, " << syserr());
		return Errno(OS_unit_seek);

	} else if (mLength < 0) {
		log_info("unit recover, try to get file length but failed, " << syserr());
		return Errno(OS_unit_stat);
	}

	/** length not invalid */
	if (mLength < object::c_unit_head_size) {
		if (mLength > 0) {
			MockWakeup(WT_unit_head_partial);
			writer_inc(WS_recovr_head_partial);
			log_info("unit recover, " << String() << " [" << mLength
				<< "], but smaller than head length " << object::c_unit_head_size << ", not valid");
		    mLength = 0;
		}
	
    /** check if head is valid */
	} else {
		if (mRecover.read(data, object::c_unit_head_size) != object::c_unit_head_size) {
			writer_inc(WS_recovr_read_failed);
			log_warn("unit recover, " << String() << ", read head failed, " << syserr());
			return Errno(OS_unit_read);
		}
		writer_inc(WS_recovr_read, object::c_unit_head_size);

		/** head crashed, truncate length */
		if (head.Read(data, object::c_unit_head_size) != 0) {
			MockWakeup(WT_unit_head_crash);
			writer_inc(WS_recovr_head_crash);
			log_info("unit recover, " << String() << ", head crashed, truncate all data");

			if (mRecover.trunc(0) != 0) {
				MockWakeup(WT_unit_trunc);
				log_info("unit recover, " << String() << ", truncate to 0 failed, " << syserr());
				return Errno(OS_unit_trunc);
			}
			mLength = 0;

		} else {
			MockWakeup(WT_unit_parse_head);
		}
	}

	/** write new head */
	if (mLength == 0) {
		head.Write(data, sizeof(Head));

		if (mRecover.seek(0) < 0) {
			log_info("unit recover, seek to 0 but failed, " << syserr());
			return Errno(OS_unit_seek);

		} else if (mRecover.write(data, object::c_unit_head_size) != object::c_unit_head_size) {
			log_warn("unit recover, " << String() << ", write head but failed, " << syserr());
			return Errno(OS_unit_write);
		}
		mLength = object::c_unit_head_size;

		if (mRecover.flush() != 0) {
			log_info("unit recover, " << String() << ", write head flush failed, " << syserr());
			return Errno(OS_unit_flush);
		}

		MockWakeup(WT_unit_write_head);
		//log_trace("unit recover, " << String() << " write new head");

	} else {
		writer_inc(WS_recovr_span, object::c_unit_head_size);
	}
	return 0;
}

int
ObjectUnit::RecoverData(UnitIndex& index)
{
	Object::Head head;
	stack_align(data, object::c_object_head_size, c_page_size);
	int64_t pos = object::c_unit_head_size;

	/** 
	 * TODO: do more check here
	 **/
	/** recover from record */
	if (index.curr != 0) {
		trace("recover data, " << StringIndex(index) << ", change point to curr " << index.curr);

		assert(index.curr >= object::c_object_head_size);
		index.status = UnitIndex::US_using;
		pos = index.curr;
	}

	int64_t start = pos;
	int count = 0;
	while (1) {
		int64_t end = RecoverNext(pos, head, data);
		if (end > 0) {
			CommitObject(index, head);
			pos = end;
			count++;

		} else {
			break;
		}
	}
	writer_inc(WS_recovr_object, count);

	if (pos != mLength) {
		if (mRecover.trunc(pos) == 0) {
			MockWakeup(WT_object_trunc);
			writer_inc(WS_recovr_trunc);
			log_info("unit recover, " << String() << ", truncate to " << string_size(pos));

		} else {
			log_info("unit recover, " << String() << ", truncate to " << string_size(pos) << " but failed, " << syserr());
			return Errno(OS_unit_trunc);
		}
		mLength = pos;
	}
	writer_inc(WS_recovr_span, pos - start);

	if (mRecover.seek(mLength) < 0) {
		log_info("unit recover, " << String() << ", seek to " << mLength << " but failed, " << syserr());
		return Errno(OS_unit_seek);
	}

	if (mLength > object::c_unit_head_size) {
		/** have recovered more data then recorded */
		if (mLength > start) {
			log_info("recover unit, " << String()  << ", recover " << start << " to " << mLength
				<< ", length: " << string_size(mLength - start) << ", object count " << count);

		} else {
			trace("recover unit, " << String() << ", do nothings, keep curr " << string_size(start));
		}
	}

	index.size = Config().unit;
	if (mLength >= Config().unit - (c_page_size + c_page_size)) {
		index.curr = mLength;
		index.status = UnitIndex::US_cmplt;

	} else if (mLength > index.curr) {
		index.curr = mLength;

		if (mLength > object::c_unit_head_size) {
			index.status = UnitIndex::US_recov;
		}
	} else if (mLength == index.curr) {
		index.status = UnitIndex::US_using;

	} else {
		assert(0);
	}
	return 0;
}

int
ObjectUnit::RecoverDone(UnitIndex& index, bool success)
{
	return mWriter->RecoverDone(index, success);
}

int64_t
ObjectUnit::RecoverNext(int64_t pos, Object::Head& head, byte_t* data)
{
	/** try page io here */
	do {
		if (pos + object::c_object_head_size < mLength) {
			if (mRecover.pread(data, object::c_object_head_size, pos) != object::c_object_head_size) {
				writer_inc(WS_recovr_read_failed);
				log_info("recover object, " << String() << " pos " << pos
					<< ", read object head but failed, " << syserr());
				break;

			} else if (head.Read(data, object::c_object_head_size) != 0) {
				MockWakeup(WT_object_head_crash);
				writer_inc(WS_recovr_object_head_crash);
				log_info("recover object, " << String() << " pos " << pos
					<< ", object head not valid");
				break;
			}
			writer_inc(WS_recovr_read, object::c_object_head_size);

			stack_align(chunk, object::c_object_head_size, c_page_size);
			int64_t end = head.EndPos() + pos;
			if (end > mLength) {
				MockWakeup(WT_object_length_exceed);
				writer_inc(WS_recovr_object_trunc);
				log_info("recover object, " << String() << " pos " << pos << ", " << StringObject(head)
					<< " end " << end << ", exceed file length " << mLength);
				break;

			} else if (mRecover.pread(chunk, head.CheckSize(), pos + head.CheckPos()) != head.CheckSize()) {
				writer_inc(WS_recovr_read_failed);
				log_info("recover object, " << String() << " pos " << pos << ", " << StringObject(head)
					<< ", read " << pos + head.CheckPos() << " size " << head.CheckSize()
					<< " but read block failed, " << syserr());
				break;

			} else if (head.Last(chunk, head.CheckSize()) != 0) {
				MockWakeup(WT_object_block_crash);
				log_info("recover object, " << String() << " pos " << pos << ", " << StringObject(head)
					<< ", object block not valid");
				break;
			}
			writer_inc(WS_recovr_read, head.CheckSize());

			MockWakeup(WT_object_parse);
			trace("recover object, " << StringObject(head));
			return end;
		}
	} while (0);

	return -1;
}

int
WriteData(void* ptr, const byte_t* data, length_t len)
{
	ObjectUnit* unit = (ObjectUnit*)ptr;
	return unit->SingleWrite(data, len);
}

int
ObjectUnit::SingleWrite(const byte_t* data, length_t len)
{
	if (mFile.write(data, len) != (int)len) {
		return Errno(OS_object_data);
	}
	return 0;
}

int
ObjectUnit::WriteObject()
{
	if (WriteHead() != 0 ||
		WriteData() != 0)
	{
		return Errno();
	}

	return CommitObject();
}

int
ObjectUnit::WriteHead()
{
	#if OBJECT_PERFORM
		return 0;
	#endif
	mHead.object = mObject;
	stack_align(data, object::c_object_head_size, c_page_size);
	mHead.Write(data, object::c_object_head_size, mIndex.curr);

	if (mFile.write(data, object::c_object_head_size) != object::c_object_head_size ||
		MockWakeupEvent(WT_object_write_head))
	{
        log_warn("write object, " << String() << " pos " << mLength << ", " << StringObject(mHead)
        	<< ", write head failed, " << syserr());
		return Errno(OS_object_write);
	}
	return 0;
}

int
ObjectUnit::WriteData()
{
	Buffer* buffer = &mObject->mData;
	if (buffer->dispatch(::WriteData, this) != (int)buffer->length() ||
		MockWakeupEvent(WT_object_write_data))
	{
		log_warn("write object, " << String() << " pos " << mLength + object::c_unit_head_size << ", " << StringObject(mHead)
		     << ", write data failed, " << syserr());
		return Errno(OS_object_write);
	}
	return 0;
}

int
ObjectUnit::FlushData()
{
	return 0;
	if (!mIndex.Empty()) {
		if (mData.dispatch(::WriteData, this) != (int)mData.length() ||
			MockWakeupEvent(WT_object_write_data))
		{
			log_warn("flush data, " << String() << " pos " << mLength + object::c_unit_head_size << ", " << StringObject(mHead)
				 << ", write data failed, " << syserr());
			return Errno(OS_object_write);
		}
		mData.clear();
	}
	return 0;
}

int
ObjectUnit::CommitObject()
{
	mObject->mLocation.index  = mIndex;
    mObject->mLocation.offset = mIndex.curr;
	mIndex.curr += mObject->OccupySize();

	writer_inc(WS_object_size_done, mObject->mLength + c_object_head_size);
	/** release data for reuse */
	mObject->ClearData();

	return CommitObject(mObject);
}

int
ObjectUnit::CommitObject(const UnitIndex& index, Object::Head& head)
{
	Object* object = Object::Malloc();

	object->Set(index, head);
	object->SetKey(head.keyptr, head.keylen);
	int ret = CommitObject(object);
	object->Dec();
	return ret;
}

int
ObjectUnit::CommitObject(Object* object)
{
	return mWriter->CommitObject(object);
}



