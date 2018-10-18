
#pragma once

#include <cstring>

#include "Common/Type.hpp"
#include "Common/Define.hpp"
#include "Common/Atomic.hpp"
#include "Common/CodeHelper.hpp"
#include "Advance/DynBuffer.hpp"

namespace object {
	const int c_user_name_size 	= 512;
	const int c_container_name_size = 512;
	const int c_object_name_size = 512;

	const int c_object_head_size = c_length_4K;
	const int c_object_magic_size = 16;

	/** unit head length */
	const int c_unit_head_size 	= c_length_4K;
	const int c_unit_magic_size = 16;

	const int c_object_dir_width = 256;
	const int c_object_dir_files = 256;

	typedef common::DynBuffer Buffer;
//	typedef int64_t	UnitIndex;
};
using namespace object;

struct UnitIndex
{
public:
	UnitIndex(int64_t idx = 0, int64_t s = 0)
		: index(idx), size(s) {}

	UnitIndex(const UnitIndex& v) {
		operator = (v);
	}

	const UnitIndex& operator = (const UnitIndex& v) {
		index = v.index;
		curr = v.curr;
		size = v.size;
		status = v.status;
        return *this;
	}

	/**
	 * unit status in db
	 **/
	enum UnitStatus {
		US_alloc = 0,
		US_recov,
		US_using,
		US_cmplt,
	};

public:
	/**
	 * reset
	 **/
	void	Reset() {
		index = 0;
		curr = 0;
		size = 0;
		status = US_alloc;
	}

	/**
	 * check if index is empty
	 **/
	bool	Empty() {
		return index == 0;
	}

	bool	Status(int st) const {
		return st == status;
	}

public:
	int64_t	index = {0};
	int64_t	curr  = {0};
	int64_t size  = {0};
	int		status = {US_alloc};
};

inline bool
operator < (const struct UnitIndex& a, const struct UnitIndex& b) {
	return a.index < b.index;
}

/**
 * location for object in unit
 **/
struct Location
{
	UnitIndex index;
	int32_t	offset  = {0};
};

struct ObjectUsr
{
	/** object key */
	char 	name[c_user_name_size] = { 0 };
};

struct Container
{
public:
	char 	name[c_container_name_size] = {0};
};

struct ObjectKey
{
	/** object key */
	char 	name[c_object_name_size] = { 0 };
};

/**
 * object container
 **/
struct Object
{
public:
    Object() {}
    virtual ~Object() {}

public:

    /**
     * object head
     **/
    struct Head {

    public:
    	Head(Object* ob = NULL) { object = ob; }

    public:
    	/**
    	 * read data from head
    	 **/
    	int 	Read(char* data, uint32_t total);

    	/**
    	 * write head to data buffer
    	 **/
    	void	Write(char* data, uint32_t total, int32_t off);

    	/**
    	 * check object last block hash
    	 **/
    	int		Last(char* data, uint32_t total);

    	/**
    	 * get head start point
    	 **/
    	byte_t*	Start() { return (byte_t*)this + c_head_pos; }

    	/**
		 * object end position, including suffix
		 **/
		int32_t	EndPos();

		/**
		 * get check position
		 **/
		int32_t	CheckPos();

		/**
		 * get check size
		 **/
		int32_t CheckSize();

    protected:
		/**
		 * get head hash
		 **/
		uint32_t HeadHash();

		/**
		 * get last block hash
		 **/
		uint32_t LastHash(char* data = NULL);

    public:
		/** head magic */
    	int 		magic[4] = { s_magic[0], s_magic[1], s_magic[2], s_magic[3] };
    	/** current offset */
    	int32_t	    offset = {0};
    	/** actual data length */
    	int32_t	    length = {0};
    	/** including suffix */
    	int32_t 	actual = {0};
    	/** object ke length */
    	int32_t	    keylen = {0};
    	/** head and block hash for check */
    	uint32_t 	hash[2] = {0};

    	const char*	keyptr  = {NULL};
    	Object*		object 	= {NULL};

		static const int s_magic[object::c_object_magic_size / sizeof(int)];
    };
	static const int c_head_pos = OFFSET(Head, magic[0]);
	static const int c_head_fixed_size = OFFSET(Head, keyptr) - c_head_pos;

public:
	/**
	 * format with head
	 **/
	void	Set(const UnitIndex& index, Head& head) {
		SetKey(head.keyptr, head.keylen);
		mLength = head.length;
		mActual = head.actual;
		mLocation.index = index;
		mLocation.offset = head.offset;
	}

	/**
	 * set user part
	 **/
	void	SetUser(const byte_t* data, uint32_t len) {
		memcpy(mUser.name, data, len);
	}

	/**
	 * set container part
	 **/
	void	SetContainer(const byte_t* data, uint32_t len) {
		memcpy(mKey.name, data, len);
	}

	/**
	 * set key part
	 **/
	void	SetKey(const byte_t* data, uint32_t len) {
		memset(mKey.name, 0, sizeof(mKey.name));
		memcpy(mKey.name, data, len);
	}

	/**
	 * set data value
	 **/
	void	SetData(const byte_t* data, uint32_t len) {
		mData.write(data, len);
		mLength = len;
	}

	/**
	 * get object occupy size after write
	 **/
	int		OccupySize();

	/**
	 * fill last part
	 **/
	void	Ajustment();

	/**
	 * release data length
	 **/
	void	ClearData() {
		mData.clear();
	}

	/**
	 * inc refernce
	 **/
	void	Inc() { mRefer.inc(); }

	/**
	 * dec reference
	 **/
	void	Dec() {
		if (mRefer.dec()) {
			Cycle();
		}
	}

public:
	/**
	 * create new object
	 **/
	static Object* Malloc();

	/**
	 * cycle object
	 **/
	virtual void Cycle();

	/**
	 * work complete
	 **/
	virtual void Done() {}

public:
	/** object error */
	int		mError = {0};
	/** data length */
	int		mLength = {0};
	/** occupy length */
	int		mActual = {0};
	/** reference count */
	common::Refer mRefer;
	/** object user */
	ObjectUsr mUser;
	/** object container */
	Container mContainer;
	/** object key */
	ObjectKey mKey;
	/** object data */
	Buffer	 mData;
	/** object location */
	Location mLocation;
};

#define	StringObject(head) \
	"object key " << head.keyptr << " size " << string_size(head.length) \
	<< " suffix " << head.actual - head.length

#define	StringIndex(unit_index) \
	"unit " << unit_index.index << "-" << string_size(unit_index.curr)

/** get unit path */
std::string UnitPath(const UnitIndex& index);


