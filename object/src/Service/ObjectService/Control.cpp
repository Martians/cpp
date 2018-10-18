

#include "ObjectService/Config.hpp"
#include "ObjectService/Logger.hpp"
#include "ObjectService/Reader.hpp"
#include "ObjectService/Writer.hpp"
#include "ObjectService/Control.hpp"
#include "Advance/DynBuffer.hpp"

Control::Control()
{
	mConfig = new ObjectConfig;
}

Control::~Control()
{
	Stop();

	reset(mReader);
	reset(mWriter);
	reset(mConfig);
}

void
Control::SetLogging()
{
	LogLevel level = common::LOG_LEVEL_TRACE;
	Logging::set_name("object", Logging::c_log_path, LT_object);
	Logging::set_name("reader", Logging::c_log_path, LT_reader);
	Logging::set_name("writer", Logging::c_log_path, LT_writer);

	Logging::set_level(level, LOG_START);
	Logging::set_level(level, LT_object);
	Logging::set_level(level, LT_reader);
	Logging::set_level(level, LT_writer);
}

void
Control::SetConfig(ObjectConfig* config)
{
	if (config) {
		*mConfig = *config;
	}
}

void
Control::Init()
{
	common::dyn_chunk_pool(DynBuffer::c_length,
		DynBuffer::c_length);
}

void
Control::Start(ObjectConfig* config)
{
	Init();

	if (!change_status(true)) {
		return;
	}

	SetConfig(config);

	//SetLogging();

	mReader = new Reader(mConfig);
	mWriter = new Writer(mConfig);

	mReader->Start();
	mWriter->Start();
}

void
Control::Stop(int wait)
{
	if (!change_status(false)) {
		return;
	}

	if (mReader) {
		mReader->Stop(wait);
	}

	if (mWriter) {
		mWriter->Stop();
	}
}

