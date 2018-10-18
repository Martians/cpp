
#pragma once

struct ObjectConfig;
class Reader;
class Writer;

#include "CodeHelper/RunStat.hpp"

class Control
{
public:
	Control();

	~Control();

public:
	/**
	 * start control
	 **/
	void	Start(ObjectConfig* config = NULL);

	/**
	 * stop control
	 * */
	void	Stop(int wait = 0);

	/**
	 * get config file
	 **/
	ObjectConfig* GetConfig() { return mConfig; }

	/**
	 * get reader
	 **/
	Reader*	GetReader() { return mReader; }

	/**
	 * get writer
	 **/
	Writer* GetWriter() { return mWriter; }

public:
	/**
	 * do init work, should manually load for unit test
	 **/
	void	Init();

	/**
	 * set global config
	 **/
	void	SetConfig(ObjectConfig* config);

	/**
	 * set reader for unit test
	 **/
	void	SetReader(Reader* reader) { mReader = reader; }

	/**
	 * set writer for unit test
	 **/
	void	SetWriter(Writer* writer) { mWriter = writer; }

	/**
	 * set logging status
	 **/
	void	SetLogging();

	RUNSTATE_DEFINE;

protected:
	/** global config */
	ObjectConfig* mConfig = {NULL};
	/** global reader */
	Reader*	mReader = {NULL};
	/** global writer */
	Writer*	mWriter = {NULL};
};

inline Control* GetControl() {
	static Control s_control;
	return &s_control;
}

inline ObjectConfig* GetConfig() {
	return GetControl()->GetConfig();
}

inline Reader* GetReader() {
	return GetControl()->GetReader();
}

inline Writer* GetWriter() {
	return GetControl()->GetWriter();
}
