
#pragma once

#include "ObjectService/ObjectUnit.hpp"

#include "Perform/Source.hpp"
#include "ObjectService/Control.hpp"
#include "ObjectService/Reader.hpp"
#include "ObjectService/Writer.hpp"
#include "Advance/Container.hpp"
#include "Perform/MockBase.hpp"
#include "UnitTest/Mock.hpp"

namespace object_mock {
    /** generate source range */
	const int c_key_range[] = {10, 30};
	const int c_data_range[] = {c_length_4K, c_length_1K * 10};
}
using namespace object_mock;

/**
 * context generator
 **/
class ObjectSource {

public:
	ObjectSource() {
		common::tool::unexpect_char({'\\', '(', ')', '[', ']', '\''});
		mSource.set_key(true, true);
		mSource.set_data(true, false);
		mSource.key_range(c_key_range[0], c_key_range[1]);
		mSource.data_range(c_data_range[0], c_data_range[1]);
	}
public:
	/**
	 * initial
	 **/
	void		Init() { generator(100); }

	/**
	 * get next object
	 **/
	Object* 	Next(int len = 0) {
		char* key;
		char* data;
		int klen, dlen;

		mSource.key(key, klen);
		mSource.get(data, dlen, len);

		Object* object = Object::Malloc();
		object->SetKey(key, klen);
		object->SetData(data, dlen);
		return object;
	}
protected:
	DataSource 	  mSource;
};
extern ObjectSource g_source;

/**
 * test environment
 **/
class ObjectEnvironment : public MockEnvironment
{
    virtual void SetUp() {

    	/** global init work */
        GetControl()->Init();
    }
    virtual void TearDown() {}
};

/**
 * base object test
 **/
class ObjectTest : public MockBase
{
protected:
    ObjectConfig mConfig;
};
