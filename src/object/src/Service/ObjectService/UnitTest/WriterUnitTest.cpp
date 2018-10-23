
#include "WriterUnitTest.hpp"

ObjectSource g_source;

int
main(int argc, char *argv[]) {
    return RunTest(argc, argv, new ObjectEnvironment);
}

TEST_F(WriterTest, Write_FetchNoneUnit_Failed)
{
	MockNoneUnit();

	g_mock.Regist({OS_alloc_unit});
	Start();

	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterTest, Write_CanntOpenFile_Failed)
{
	/** regist event, and wait error happen */
	g_mock.Regist({OS_unit_open});
	Start();

	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterTest, Write_WriteHeadFailed_Failed)
{
	g_mock.Regist({WT_object_write_head, OS_object_write});
	Start();

	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterTest, Write_WriteDataFailed_Failed)
{
	g_mock.Regist({WT_object_write_data, OS_object_write});
	Start();

	DoWrite();
	
    ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterTest, Write_Success)
{
	g_mock.Regist(WT_object_done);
	Start();

	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterTest, Write_FileFullMoveNext_Success)
{
	MockUnitSmall();
	g_mock.Regist(WT_unit_roll);
	g_mock.Count(WT_unit_roll, 3);
	Start();

	DoWrite(c_small_write_loop * 5);

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterTest, Write_MultThread_Success)
{
	g_mock.Regist(WT_object_done);
	g_mock.Count(WT_object_done, c_multi_write_loop);
	Start();

	DoWrite(c_multi_write_loop);

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverUnit_FileEmpty_Success)
{
	mUnit.SetEmpty();
	g_mock.Regist({WT_unit_write_head, WT_object_done});
	g_mock.RegistUnexpect(WT_unit_parse_head);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverUnit_LengthSmallThanHeadLength_WriteHead)
{
	mUnit.Trunc(object::c_unit_head_size - 1);
	g_mock.Regist({WT_unit_head_partial, WT_unit_write_head, WT_object_done});
	g_mock.RegistUnexpect(WT_unit_parse_head);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverUnit_OnlyHead_Success)
{
	mUnit.OnlyHead();
	g_mock.Regist({WT_unit_parse_head, WT_object_done});
	g_mock.RegistUnexpect(WT_object_parse);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverUnit_HeadCrashed_WriteHead)
{
	mUnit.CrashHead();
	g_mock.Regist({WT_unit_head_crash, WT_unit_write_head, WT_object_done});

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverData_HaveEmptyData_Truncate)
{
	mUnit.SetEmptyData();
	g_mock.Regist({WT_object_trunc, WT_object_done});
	g_mock.RegistUnexpect(WT_object_parse);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverData_ExistData_Success)
{
	mUnit.Prepare();
	g_mock.Regist({WT_object_parse, WT_object_done});
	g_mock.Count(WT_object_parse, c_prepare_count);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverData_HeadFailed_Truncate)
{
	mUnit.CrashObjectHeadMagic(c_crash_data_head_index);
	g_mock.Regist({WT_object_head_crash, WT_object_parse, WT_object_trunc, WT_object_done});
	g_mock.Count(WT_object_parse, c_crash_data_head_index - 1);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverData_BlockHashNotMatch_Truncate)
{
	mUnit.CrashObjectLastBlock(c_crash_data_last_index);
	g_mock.Regist({WT_object_block_crash, WT_object_parse, WT_object_trunc, WT_object_done});
	g_mock.Count(WT_object_parse, c_crash_data_last_index - 1);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverData_LengthExceedFile_Truncate)
{
	mUnit.CrashObjectLength(c_crash_data_length_index);
	g_mock.Regist({WT_object_length_exceed, WT_object_parse, WT_object_trunc, WT_object_done});
	g_mock.Count(WT_object_parse, c_crash_data_length_index - 1);

	Start();
	DoWrite();

	ASSERT_EQ(0, g_mock.Wait());
}

TEST_F(WriterRecover, RecoverData_FullAllocNewUnit_Roll)
{
	int count = mUnit.WriteUnitFull();
	g_mock.Regist({WT_unit_roll, WT_object_parse, WT_object_done});
	g_mock.Count(WT_object_parse, count);
	g_mock.Count(WT_unit_roll, 2);

	Start();
	DoWrite(3);

	ASSERT_EQ(0, g_mock.Wait());
}

