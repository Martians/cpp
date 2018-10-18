
#include "UnitTest/ReaderUnitTest.hpp"

TEST_F(DataBaseTest, Connect_AddressNotExist_Failed)
{
	mConfig.reader.conn.host = mock::c_not_exist_host;
	g_mock.Regist(CONNECTION_BAD);

	DBConnect();

    ASSERT_EQ(0, Wait(DE_connect_refuse));
}

TEST_F(DataBaseTest, Connect_HostExistServiceNotExist_Failed)
{
	mConfig.reader.conn.port = mock::c_not_listen_port;
	g_mock.Regist(CONNECTION_BAD);

	DBConnect();

	ASSERT_EQ(0, Wait({DE_connect_refuse, DE_connect_noroute}));
}

TEST_F(DataBaseTest, Connect_DatabaseNotExist_Failed)
{
	mConfig.reader.conn.dbname = mock::c_not_exist_value;
	g_mock.Regist(CONNECTION_BAD);

	DBConnect();

    ASSERT_EQ(0, Wait(DE_not_exist));
}

TEST_F(DataBaseTest, Connect_UserNotExist_Failed)
{
	mConfig.reader.conn.user = mock::c_not_exist_value;
	g_mock.Regist(CONNECTION_BAD);

	DBConnect();

	ASSERT_EQ(0, Wait({DE_auth_denay, DE_not_exist}));
}

TEST_F(DataBaseTest, Connect_PasswdNotMatch_Failed)
{
	mConfig.reader.conn.passwd = mock::c_not_exist_value;
	g_mock.Regist(CONNECTION_BAD);

	DBConnect();

	ASSERT_EQ(0, Wait({DE_auth_denay, DE_not_exist}));
}

TEST_F(ReaderServer, CreateTables_Success)
{
	g_mock.Regist(WT_prepare_table);

	Start();

    ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderServer, Prepare_CreateTablesExist_Success)
{
	CreateTable();
	g_mock.Regist(WT_prepare_table);

	Start();

    ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderServer, Prepare_FetchUnit_Success)
{
	g_mock.Regist(WT_fetch_free);
	g_mock.Count(WT_fetch_free, mConfig.reader.free.max);

	Start();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderServer, Recover_TimeoutUnit_Success)
{
	Establish();
	g_mock.Regist(WT_fetch_recover);
	RegistEvent(US_timeout, UnitIndex::US_alloc);

	Start();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderServer, Recover_TimeoutAndSelfUnit_Success)
{
}

TEST_F(ReaderServer, PingTest_ServerDropReconnect_DoNothing)
{
	PingTest(false);

	g_mock.Regist(WT_ping);
	g_mock.RegistUnexpect({WT_drop, WT_restart, WT_fetch_recover});

	ConnDrop();
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderServer, PingTest_ServerDropTimeout_DropRequest)
{
	PingTest();
	g_mock.Regist({WT_drop, WT_restart, WT_fetch_recover});
	//RegistEvent(US_timeout, UnitIndex::US_alloc);

	ConnDrop(true);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, DISABLED_CommitObject_OneDuplicate_Success)
{
	int count = 1;
	DuplicateTest(count);

	DoCommit(count, c_duplicate_value);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, DISABLED_CommitObject_ALLDuplicate_Success)
{
	int count = 2;
	DuplicateTest(count);

	DoCommit(count, c_duplicate_value);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, DISABLED_CommitObject_ContainDuplicate_Success)
{
	int count = 3;
	DuplicateTest(count);

	DoCommit();
	DoCommit(1, c_duplicate_value);
	DoCommit();
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitObject_SingleWaitTimeout_Success)
{
	CommitObjectTest();

	int count = 1;
	g_mock.Regist({WT_commit_object, WT_commit_object_wait});
	g_mock.RegistUnexpect(WT_commit_object_batch);
	g_mock.Count(WT_commit_object, count);

	DoCommit(count);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitObject_MultiWaitTimeout_Success)
{
	CommitObjectTest();

	int count = c_commit_object_batch / 2;
	g_mock.Regist({WT_commit_object, WT_commit_object_wait});
	g_mock.RegistUnexpect(WT_commit_object_batch);
	g_mock.Count(WT_commit_object, count);

	DoCommit(count);
	g_mock.SyncDone();


	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitObject_BatchFullFlush_Success)
{
	CommitObjectTest();

	int count = c_commit_object_batch;
	g_mock.Regist({WT_commit_object, WT_commit_object_batch});
	g_mock.RegistUnexpect(WT_commit_object_wait);
	g_mock.Count(WT_commit_object, count);

	DoCommit(count);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}


TEST_F(ReaderCommit, CommitObject_BatchFullAndWakeupRightly_Success)
{
	CommitObjectTest(false);

	int count = c_commit_object_batch * 2;
	g_mock.Regist({WT_commit_object, WT_commit_object_batch, WT_commit_object_wakeup, WT_commit_object_wait});
	g_mock.Count(WT_commit_object, count);

	DoCommit(count);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitObject_ServerDropReconnect_Success)
{
	mReader.PingTest(false);
	CommitObjectTest();

	/** drop until ping timeout */
	g_mock.Regist(WT_commit_object);
	g_mock.RegistUnexpect({WT_drop, WT_restart, WT_fetch_recover});

	DoCommit();
	ConnDrop();
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitObject_ServerDropTimeout_ClearRequest)
{
	mReader.PingTest();
	CommitObjectTest();

	g_mock.Regist({WT_drop, WT_restart, WT_fetch_recover});
	g_mock.RegistUnexpect(WT_commit_object);

	DoCommit();
	ConnDrop(true);
	g_mock.SyncDone();
	EXPECT_EQ(0, g_mock.Wait());

	ASSERT_EQ(0, mReader.CommittingObject());
}

TEST_F(ReaderCommit, CommitUnit_Success)
{
	CommitUnitTest();
	int count = 2;
	g_mock.Regist(WT_commit_unit);
	g_mock.Count(WT_commit_unit, count);

	DoCommitUnit(c_commit_unit_start, count);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitUnit_UnitNotRegisted_Success)
{
	CommitUnitTest();
	g_mock.Regist(WT_commit_unit);

	DoCommitUnit(c_invalid_int);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}


TEST_F(ReaderCommit, DISABLED_CommitUnit_RecoverFullUnit_Success)
{
}

TEST_F(ReaderCommit, CommitUnit_SingleCommitWaitTimeout_Success)
{
	CommitUnitTest();

	g_mock.Regist({WT_commit_unit, WT_commit_unit_wait});
	g_mock.RegistUnexpect(WT_commit_unit_batch);
	g_mock.Count(WT_commit_unit, 1);

	DoCommitUnit(c_commit_unit_start);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitUnit_BatchFullFlush_Success)
{
	CommitUnitTest();

	int count = c_commit_unit_batch;
	g_mock.Regist({WT_commit_unit, WT_commit_unit_batch});
	g_mock.RegistUnexpect(WT_commit_unit_wait);
	g_mock.Count(WT_commit_unit, count);

	DoCommitUnit(c_commit_unit_start, count);
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitUnit_ServerDrop_ResendSuccess)
{
	mReader.PingTest(false);
	CommitUnitTest();

	/** drop until ping timeout */
	g_mock.Regist(WT_commit_unit);
	g_mock.RegistUnexpect({WT_drop, WT_restart, WT_fetch_recover});

	DoCommitUnit(c_commit_unit_start);
	ConnDrop();
	g_mock.SyncDone();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(ReaderCommit, CommitUnit_ServerDropTimeout_ClearRequest)
{
	mReader.PingTest();
	CommitUnitTest();

	g_mock.Regist({WT_drop, WT_restart, WT_fetch_recover});
	g_mock.RegistUnexpect(WT_commit_unit);

	DoCommitUnit(c_commit_unit_start);
	ConnDrop(true);
	g_mock.SyncDone();
	EXPECT_EQ(0, g_mock.Wait());

	ASSERT_EQ(0, mReader.CommittingUnit());
}


TEST_F(ReaderCommit, WriterFetchUnit_Success)
{
	FetchUnitTest();
	g_mock.SyncDone();

	UnitIndex index;
	ASSERT_EQ(0, mReader.FetchUnit(index, c_fetch_unit_wait));
}

TEST_F(ReaderCommit, WriterFetchUnitTimeout_Failed)
{
	g_mock.SetData(WT_fetch_unit_tout);
	FetchUnitTest();

	g_mock.SetData(WT_fetch_unit_tout);
	g_mock.SyncDone();

	UnitIndex index;
	ASSERT_EQ(-1, mReader.FetchUnit(index, c_fetch_unit_wait));
}

TEST_F(ReaderCommit, WriterFetchUnit_UseUpWaitFetchNew_Success)
{
	FetchUnitTest();
	g_mock.SyncDone();

	g_mock.Regist(WT_fetch_unit_wait);

	UnitIndex index;
	int count = c_fetch_unit_count;
	while (count -- > 0) {
		ASSERT_EQ(0, mReader.FetchUnit(index, c_fetch_unit_wait));
	}
	EXPECT_EQ(0, g_mock.Wait(1));
}

#if 0
TEST_F(ReaderCommit, WriterFetchUnit_WriterTimeout_StopAndDropRequest)
{
	//FetchUnit();
}

TEST_F(ReaderCommit, WriterFetchUnit_DropCurrentTaskAndDoRecover_Success)
{
}

/*
TEST_F(ReaderServer, ObjectQuery_Success)
{
}

TEST_F(ReaderServer, ObjectQuery_ServerDropTimeout_Failed)
{
}

TEST_F(ReaderServer, Writer_FailedWriter_StopReader)
{
}
*/

#endif
