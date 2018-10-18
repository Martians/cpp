
#pragma once

#include "ObjectService/Config.hpp"
#include "ObjectService/Object.hpp"
#include "Common/File.hpp"
#include "Advance/AppHelper.hpp"

using namespace common;
class Writer;

/**
 * object unit file
 **/
class ObjectUnit
{
public:
	ObjectUnit(Writer* writer = NULL);

	/**
	 * set writer
	 **/
	void	Set(Writer* writer) { mWriter = writer; }

	/**
	 * current work mode
	 **/
	enum Mode {
		OM_null = 0,
		OM_recover,
	};

	/**
	 * unit file head
	 **/
	struct Head
	{
	public:
		struct Magic {
			int 	data[4] = { s_magic[0], s_magic[1], s_magic[2], s_magic[3] };
		};
	public:
		/**
		 * read head from data buffer, check if valid
		 **/
		int		Read(byte_t* data, uint32_t len);

		/**
		 * write head to data buffer
		 **/
		int		Write(byte_t* data, uint32_t len);

        /**
         * get buffer start
         **/
        byte_t* Start() { return (byte_t*)this + c_head_pos; }

	public:
		Magic	magic;
		Magic	tail;

		static const int s_magic[object::c_unit_magic_size / sizeof(int)];
	};
    static const int c_head_pos = OFFSET(Head, magic);

public:
	/**
	 * write object to unit
	 **/
	int		Write(Object* object);

	/**
	 * recover current unit
	 **/
	int 	Recover(UnitIndex& index);

	/**
	 * get errno
	 **/
	int		Errno() { return mCurr.Errno(); }

	/**
	 * write buffer data to file
	 **/
	int		SingleWrite(const byte_t* data, length_t len);

    /**
	 * drop current work
	 **/
	void	Clear();

	/**
	 * get current display
	 **/
	const string& String();

protected:
	/**
	 * get writer config
	 **/
	ObjectConfig& GlobalConfig();

	/**
	 * get writer config
	 **/
	ObjectConfig::Writer& Config();

	/**
	 * set stadge and errno
	 **/
	int		Errno(int error, int syserr = errno);

	/**
	 * get file paht
	 **/
	string	Path(const UnitIndex& index);

protected:
	/**
	 * should try another unit
	 **/
	bool	NewUnit(Object* object) {
		if (mFile.is_open()) {
			return !object ? false : mIndex.curr + object->OccupySize() > mIndex.size;

		} else {
			return true;
		}
	}

	/**
	 * set current work mode
	 **/
	void	RecoverMode(bool set);

	/**
	 * switch recover to work file
	 **/
	void	FileSwitch();

	/**
	 * open file
	 **/
	int		OpenFile(const UnitIndex& index);

	/**
	 * recover unit head
	 **/
	int		RecoverHead();

	/**
	 * recover data
	 **/
	int		RecoverData(UnitIndex& index);

	/**
	 * recover complete
	 **/
	int		RecoverDone(UnitIndex& index, bool success);

	/**
	 * move next object
	 **/
	int64_t	RecoverNext(int64_t pos, Object::Head& head, byte_t* data);

	/**
	 * check state for current object write
	 **/
	int		Prepare(Object* object);

	/**
	 * refresh local info, alloc new unit
	 **/
	int		FetchUnit();

	/**
	 * write object data
	 **/
	int		WriteObject();

	/**
	 * write head
	 **/
	int		WriteHead();

	/**
	 * write data
	 **/
	int		WriteData();

	/**
	 * commit object
	 **/
	int		CommitObject();

	/**
	 * flush data
	 **/
	int		FlushData();

protected:
	/**
	 * commit object when recover
	 **/
	int		CommitObject(const UnitIndex& index, Object::Head& head);

	/**
	 * comimt object to reader
	 **/
	int		CommitObject(Object* object);

protected:
	/** writer pointer */
	Writer* 	mWriter = {NULL};
	/** current status */
	CurrentStatus mCurr;
	/** current unit */
	UnitIndex	mIndex;
	/** current file operator */
	FileBase	mFile;
	/** recover file operator */
	FileBase	mRecover;
	/** recover file size */
	int64_t		mLength = {0};
	/** current work mode */
	int			mMode = {OM_null};
	/** current object head */
	Object::Head mHead;
	/** current object */
	Object*		mObject = {NULL};
	/** display string */
	string		mString;
	/** current buffer data */
	DynBuffer	mData;
	/** current buffer pos */
	int64_t		mPost = {0};
	#if OBJECT_PERFORM
		StadgeTimer mTimer;
	#endif
};
