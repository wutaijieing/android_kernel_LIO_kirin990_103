/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Xml.c
*
* 版    本 :
*
* 功    能 : Kunit  写Xml  文件函数实现
*
* 作    者 :
*
* 创建日期 : 2011-03-07
*
* 修改记录 :
*
*    1: <时  间> : 2011-3-29
*
*       <作  者> :
*
*       <版  本> :
*
*       <内  容> : 增加输出文件名设置函数
*
*    2:
**********************************************************************************/

#include <linux/mm.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/security.h>
#include <linux/vfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include "Xml.h"
#include "Util.h"

/**** 全局变量定义 ****/

#define FILENAME_MAX    255
static char      f_szDefaultFileRoot[] = "KUnit";

/** 测试列表文件名. */
static char      f_szTestListFileName[FILENAME_MAX] = "";

/** 测试结果文件名. */
static char      f_szTestResultFileName[FILENAME_MAX] = "";

/** 字符串缓存数组. */
static char      g_readbuftmp[BUFFER_SIZE];

/** 测试结果文件句柄. */
static unsigned int iFd_Result;

/** 测试包信息写xml  标签. */
static KU_BOOL f_bWriting_KUNIT_RUN_SUITE = KU_FALSE;

/** 当前测试包. */
static KU_pSuite f_pRunningSuite = NULL;

/**** uSDC项目打印接口****/
#ifdef _ENABLE_DEBUG_OUTPUT_
extern USER_DEFIND_PRINTF(char *fmt, ...);
#endif

#ifndef _ENABLE_DEBUG_OUTPUT_
#define SPRINTF_DBG(fmt, args...)
#else
#define SPRINTF_DBG(fmt, args...) USER_DEFIND_PRINTF(char *fmt, ...)
#endif


/********************************************************
  * 函数名称: KFILE_fget_light
  *
  * 功能描述: 在当前进程描述表中获取文件
  *                         描述符fd  所指的file  结构
  *
  * 调用函数: 无
  *
  * 被调函数: KFILE_write
  *
  * 输入参数:
  *                       Param1: unsigned int fd
  *
  * 输出参数:
  *                       Param1: int *fput_needed
  *
  * 返 回 值:
  *                       struct file                       执行成功
  *                       NULL                             执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
struct file *KFILE_fget_light(unsigned int fd, int *fput_needed)
{
	struct file *file;
	struct files_struct *files = current->files;
	*fput_needed = 0;

	if (atomic_read(&files->count) == 1) {
		file = fcheck_files(files, fd);
	} else {
		spin_lock(&files->file_lock);
		file = fcheck_files(files, fd);
		if (file) {
			get_file(file);
			*fput_needed = 1;
		}
		spin_unlock(&files->file_lock);
	}
	return file;
}

/********************************************************
  * 函数名称: KFILE_open
  *
  * 功能描述: 打开文件
  *
  * 调用函数: 无
  *
  * 被调函数: automated_list_all_tests    initialize_result_file
  *
  * 输入参数:
  *                       Param1: const char *filename
  *                       Param2: int flags
  *                       Param3: int mode
  * 输出参数:
  *                       无
  *
  * 返 回 值:
  *                       unsigned int fd                     文件描述符
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
unsigned int KFILE_open(const char *filename, int flags, int mode)
{
	int fd;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	fd = get_unused_fd_flags(0);
	if (fd >= 0) {
		struct file *f = filp_open(filename, flags, mode);
		if (!IS_ERR(f)) {
			fd_install(fd, f);
		} else {
			put_unused_fd(fd);
			fd = PTR_ERR(f);
		}
	}

	set_fs(oldfs);
	return (unsigned int)fd;
}

/********************************************************
  * 函数名称: KFILE_close
  *
  * 功能描述: 关闭文件
  *
  * 调用函数: 无
  *
  * 被调函数: automated_list_all_tests    initialize_result_file
  *
  * 输入参数:
  *                       Param1: unsigned int fd
  *
  * 输出参数:
  *                       无
  *
  * 返 回 值:
  *                       0                               执行成功
  *                       其他                      执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
unsigned int KFILE_close(unsigned int fd)
{
	return sys_close(fd);;
}

/********************************************************
  * 函数名称: KFILE_write
  *
  * 功能描述: 写字符串到文件
  *
  * 调用函数: KFILE_fget_light
  *
  * 被调函数: automated_list_all_tests
  *
  * 输入参数:
  *                         Param1: unsigned int fd
  *                         Param2: const char * buf
  *
  * 输出参数:
  *                       无
  *
  * 返 回 值:
  *                       0                               执行成功
  *                       其他                      执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
ssize_t KFILE_write(unsigned int fd, const char * buf)
{
	struct file *file;
	ssize_t ret = -EBADF;
	int fput_needed;
	mm_segment_t oldfs;
	size_t count;

	count = strlen(buf);
	file = KFILE_fget_light(fd, &fput_needed);
	if (file) {
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		ret = vfs_write(file, buf, count, &file->f_pos);
		set_fs(oldfs);
		fput_light(file, fput_needed);
	}

	return ret;
}


/********************************************************
  * 函数名称: KU_set_output_filename
  *
  * 功能描述:设置输出文件名
  *
  * 调用函数:
  *
  * 被调函数: RUN_ALL_TESTS
  *
  * 输入参数:
  *                         Param1: const char* szFilenameRoot
  *
  * 输出参数:
  *                       无
  *
  * 返 回 值:
  *                       无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
void KU_set_output_filename(const char* szFilenameRoot)
{
	const char* szListEnding = "-Listing.xml";
	const char* szResultEnding = "-Results.xml";

	/* 构造测试列表文件名 */
	if (szFilenameRoot)
		strncpy(f_szTestListFileName, szFilenameRoot, FILENAME_MAX - strlen(szListEnding) - 1);
	else
		strncpy(f_szTestListFileName, f_szDefaultFileRoot, FILENAME_MAX - strlen(szListEnding) - 1);

	f_szTestListFileName[FILENAME_MAX - strlen(szListEnding) - 1] = '\0';
	strcat(f_szTestListFileName, szListEnding);

	/* 构造测试结果文件名 */
	if (szFilenameRoot)
		strncpy(f_szTestResultFileName, szFilenameRoot, FILENAME_MAX - strlen(szResultEnding) - 1);
	else
		strncpy(f_szTestResultFileName, f_szDefaultFileRoot, FILENAME_MAX - strlen(szResultEnding) - 1);

	f_szTestResultFileName[FILENAME_MAX - strlen(szResultEnding) - 1] = '\0';
	strcat(f_szTestResultFileName, szResultEnding);
}

/********************************************************
  * 函数名称: automated_list_all_tests
  *
  * 功能描述: 写测试列表文件
  *
  * 调用函数: KFILE_open      KFILE_write     KFILE_close
  *
  * 被调函数: KU_list_tests_to_file
  *
  * 输入参数:
  *                         Param1: KU_pTestRegistry pRegistry
  *                         Param2: const char* szFilename
  *
  * 输出参数:
  *                       无
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_ErrorCode automated_list_all_tests(KU_pTestRegistry pRegistry, const char* szFilename)
{
	KU_pSuite pSuite = NULL;
	KU_pTest  pTest = NULL;
	unsigned int iFd=-1;

	KU_set_error(KUE_SUCCESS);
	if (!pRegistry) {
		KU_set_error(KUE_NOREGISTRY);
	} else if ((!szFilename) || (strlen(szFilename) == 0)) {
		KU_set_error(KUE_BAD_FILENAME);
	} else if ((iFd = KFILE_open(f_szTestListFileName, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) < 0) {
		KU_set_error(KUE_FOPEN_FAILED);
	} else {
		KFILE_write(iFd,
			"<?xml version=\"1.0\" encoding=\"gb2312\"?> \n"
			"<?xml-stylesheet type=\"text/xsl\" href=\"KUnit-List.xsl\" ?> \n"
			"<!DOCTYPE KUNIT_TEST_LIST_REPORT SYSTEM \"KUnit-List.dtd\"> \n"
			"<KUNIT_TEST_LIST_REPORT> \n"
			"  <KUNIT_HEADER/> \n"
			"  <KUNIT_LIST_TOTAL_SUMMARY> \n");

		sprintf(g_readbuftmp, "%s%u%s",
			"    <KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
			"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> Total Number of Suites </KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> \n"
			"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> ", pRegistry->uiNumberOfSuites,
			" </KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> \n"
			"    </KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n");
		KFILE_write(iFd, g_readbuftmp);

		sprintf(g_readbuftmp,"%s%u%s",
		"    <KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
		"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> Total Number of Test Cases </KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> \n"
		"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> ",pRegistry->uiNumberOfTests,
		" </KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> \n"
		"    </KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
		"  </KUNIT_LIST_TOTAL_SUMMARY> \n");
		KFILE_write(iFd, g_readbuftmp);
		KFILE_write(iFd, "  <KUNIT_ALL_TEST_LISTING> \n");

		pSuite = pRegistry->pSuite;
		while (pSuite) {
			pTest = pSuite->pTest;
			sprintf(g_readbuftmp,"%s%s%s%s%s%s%s%u%s",
				"    <KUNIT_ALL_TEST_LISTING_SUITE> \n"
				"      <KUNIT_ALL_TEST_LISTING_SUITE_DEFINITION> \n"
				"        <SUITE_NAME> ", (NULL != pSuite->pName) ? pSuite->pName : "",
				" </SUITE_NAME> \n"
				"        <INITIALIZE_VALUE> ", (NULL != pSuite->pInitializeFunc) ? "Yes" : "No",
				" </INITIALIZE_VALUE> \n"
				"        <CLEANUP_VALUE> ", (NULL != pSuite->pCleanupFunc) ? "Yes" : "No",
				" </CLEANUP_VALUE> \n"
				"        <TEST_COUNT_VALUE> ", pSuite->uiNumberOfTests,
				" </TEST_COUNT_VALUE> \n"
				"      </KUNIT_ALL_TEST_LISTING_SUITE_DEFINITION> \n");
			KFILE_write(iFd, g_readbuftmp);
			KFILE_write(iFd, "      <KUNIT_ALL_TEST_LISTING_SUITE_TESTS> \n");

			while (NULL != pTest) {
				sprintf(g_readbuftmp,"%s%s%s",
					"        <TEST_CASE_NAME> ", (NULL != pTest->pName) ? pTest->pName : "",
					" </TEST_CASE_NAME> \n");
				KFILE_write(iFd, g_readbuftmp);
				pTest = pTest->pNext;
			}

			KFILE_write(iFd,
				"      </KUNIT_ALL_TEST_LISTING_SUITE_TESTS> \n"
				"    </KUNIT_ALL_TEST_LISTING_SUITE> \n");

			pSuite = pSuite->pNext;
		}

		KFILE_write(iFd, "  </KUNIT_ALL_TEST_LISTING> \n");
		KFILE_write(iFd,
			"  <KUNIT_FOOTER> File Generated By KUnit v" KU_VERSION " \n </KUNIT_FOOTER> \n"
			"</KUNIT_TEST_LIST_REPORT> \n");
	}

	KFILE_close(iFd);
	return KU_get_error();
}


/********************************************************
  * 函数名称: KU_list_tests_to_file
  *
  * 功能描述: 生成测试列表文件
  *
  * 调用函数: automated_list_all_tests
  *
  * 被调函数:
  *
  * 输入参数: 无
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_ErrorCode KU_list_tests_to_file(void)
{
	return automated_list_all_tests(KU_get_registry(), f_szTestListFileName);
}

/********************************************************
  * 函数名称: initialize_result_file
  *
  * 功能描述: 初始化测试结果文件
  *
  * 调用函数: KFILE_open   KFILE_write
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         Param1: const char* szFilename
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_ErrorCode initialize_result_file(const char* szFilename)
{
	KU_set_error(KUE_SUCCESS);
	if ((!szFilename) || (strlen(szFilename) == 0)) {
		KU_set_error(KUE_BAD_FILENAME);
	} else if ((iFd_Result = KFILE_open(szFilename, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) < 0) {
		KU_set_error(KUE_FOPEN_FAILED);
	} else {
		KFILE_write(iFd_Result,
			"<?xml version=\"1.0\" encoding=\"gb2312\"?> \n"
			"<?xml-stylesheet type=\"text/xsl\" href=\"KUnit-Run.xsl\" ?> \n"
			"<!DOCTYPE KUNIT_TEST_RUN_REPORT SYSTEM \"KUnit-Run.dtd\"> \n"
			"<KUNIT_TEST_RUN_REPORT> \n"
			"  <KUNIT_HEADER/> \n");
	}

	return KU_get_error();
}


/********************************************************
  * 函数名称: automated_test_start_message_handler
  *
  * 功能描述: 执行测试包前向测试结果文件
  *                         中写XML文件信息
  *
  * 调用函数: KFILE_write
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         Param1: const KU_pTest pTest
  *                         Param2: const KU_pSuite pSuite
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void automated_test_start_message_handler(const KU_pTest pTest, const KU_pSuite pSuite)
{
	/* write suite close/open tags if this is the 1st test for this szSuite */
	if ((!f_pRunningSuite) || (f_pRunningSuite != pSuite)) {
		if (f_bWriting_KUNIT_RUN_SUITE == KU_TRUE) {
			KFILE_write(iFd_Result,
				"      </KUNIT_RUN_SUITE_SUCCESS> \n"
				"    </KUNIT_RUN_SUITE> \n");
		}
		sprintf(g_readbuftmp,"%s%s%s",
			"    <KUNIT_RUN_SUITE> \n"
			"      <KUNIT_RUN_SUITE_SUCCESS> \n"
			"        <SUITE_NAME> ", (NULL != pSuite->pName) ? pSuite->pName : "",
			" </SUITE_NAME> \n");

		KFILE_write(iFd_Result,g_readbuftmp);
		f_bWriting_KUNIT_RUN_SUITE = KU_TRUE;
		f_pRunningSuite = pSuite;
	}
}

/********************************************************
  * 函数名称: automated_test_complete_message_handler
  *
  * 功能描述: 运行完一个测试用例后向
  *                         测试结果文件中写XML文件信息
  *
  * 调用函数: KU_translate_special_characters      KFILE_write
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         Param1: const KU_pTest pTest
  *                         Param2: const KU_pSuite pSuite
  *                         Param3: const KU_pFailureRecord pFailure
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void automated_test_complete_message_handler(const KU_pTest pTest, const KU_pSuite pSuite, const KU_pFailureRecord pFailure)
{
	KU_pFailureRecord pTempFailure = pFailure;
	int i = 1;
	char *szTemp = NULL;

	if (pTempFailure) {
		/* 失败信息临时存放数组 */
		szTemp = KU_MALLOC(KUNIT_MAX_ENTITY_LEN * KUNIT_MAX_STRING_LENGTH);
		if (!szTemp)
			return;

		// uSDC项目组打印信息
		SPRINTF_DBG("%s%s%s%s%s", "\nSuite ", (NULL != pSuite->pName) ? pSuite->pName : "", ", Test ",
			(NULL != pTest->pName) ? pTest->pName : "", " had failures:");

		while (pTempFailure) {
			if (pTempFailure->strCondition)
				KU_translate_special_characters(pTempFailure->strCondition, szTemp, KUNIT_MAX_ENTITY_LEN * KUNIT_MAX_STRING_LENGTH);
			else
				szTemp[0] = '\0';

			sprintf(g_readbuftmp,"%s%s%s%s%s%u%s%s%s",
				"        <KUNIT_RUN_TEST_RECORD> \n"
				"          <KUNIT_RUN_TEST_FAILURE> \n"
				"            <TEST_NAME> ", (NULL != pTest->pName) ? pTest->pName : "",
				" </TEST_NAME> \n"
				"            <FILE_NAME> ", (NULL != pTempFailure->strFileName) ? pTempFailure->strFileName : "",
				" </FILE_NAME> \n"
				"            <LINE_NUMBER> ", pTempFailure->uiLineNumber,
				" </LINE_NUMBER> \n"
				"            <CONDITION> ", szTemp,
				" </CONDITION> \n"
				"          </KUNIT_RUN_TEST_FAILURE> \n"
				"        </KUNIT_RUN_TEST_RECORD> \n");
			KFILE_write(iFd_Result, g_readbuftmp);

			// uSDC项目组打印信息
			SPRINTF_DBG("%s%d%s%s%s%u%s%s", "\n    ", i, ". ",
				(NULL != pFailure->strFileName) ? pFailure->strFileName : "", ":", pFailure->uiLineNumber, "  - ",
				(NULL != pFailure->strCondition) ? pFailure->strCondition : "");
			i++;
			pTempFailure = pTempFailure->pNext;
		}
	} else {
		sprintf(g_readbuftmp,"%s%s%s",
			"        <KUNIT_RUN_TEST_RECORD> \n"
			"          <KUNIT_RUN_TEST_SUCCESS> \n"
			"            <TEST_NAME> ", (NULL != pTest->pName) ? pTest->pName : "",
			" </TEST_NAME> \n"
			"          </KUNIT_RUN_TEST_SUCCESS> \n"
			"        </KUNIT_RUN_TEST_RECORD> \n");

		KFILE_write(iFd_Result, g_readbuftmp);
	}
}


/********************************************************
  * 函数名称: automated_all_tests_complete_message_handler
  *
  * 功能描述:  运行完一个测试包所有
  *                          测试用例后向测试结果文件
  *                          中写XML文件信息
  *
  * 调用函数: KU_get_registry      KU_get_run_summary
  *                         KFILE_write
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         Param1: const KU_pFailureRecord pFailure
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void automated_all_tests_complete_message_handler(const KU_pFailureRecord pFailure)
{
	KU_pTestRegistry pRegistry = KU_get_registry();
	KU_pRunSummary pRunSummary = KU_get_run_summary();

	if ((f_pRunningSuite) && (f_bWriting_KUNIT_RUN_SUITE == KU_TRUE)) {
		KFILE_write(iFd_Result,
			"      </KUNIT_RUN_SUITE_SUCCESS> \n"
			"    </KUNIT_RUN_SUITE> \n");
	}

	KFILE_write(iFd_Result,
		"  </KUNIT_RESULT_LISTING>\n"
		"  <KUNIT_RUN_SUMMARY> \n");

	sprintf(g_readbuftmp,"%s%u%s%u%s%u%s",
		"    <KUNIT_RUN_SUMMARY_RECORD> \n"
		"      <TYPE> Suites </TYPE> \n"
		"      <TOTAL> ", pRegistry->uiNumberOfSuites,
		" </TOTAL> \n"
		"      <RUN> ", pRunSummary->nSuitesRun,
		" </RUN> \n"
		"      <SUCCEEDED> - NA - </SUCCEEDED> \n"
		"      <FAILED> ", pRunSummary->nSuitesFailed,
		" </FAILED> \n"
		"    </KUNIT_RUN_SUMMARY_RECORD> \n");

	KFILE_write(iFd_Result, g_readbuftmp);
	sprintf(g_readbuftmp,"%s%u%s%u%s%u%s%u%s",
		"    <KUNIT_RUN_SUMMARY_RECORD> \n"
		"      <TYPE> Test Cases </TYPE> \n"
		"      <TOTAL> ", pRegistry->uiNumberOfTests,
		" </TOTAL> \n"
		"      <RUN> ", pRunSummary->nTestsRun,
		" </RUN> \n"
		"      <SUCCEEDED> ", pRunSummary->nTestsRun - pRunSummary->nTestsFailed,
		" </SUCCEEDED> \n"
		"      <FAILED> ", pRunSummary->nTestsFailed,
		" </FAILED> \n"
		"    </KUNIT_RUN_SUMMARY_RECORD> \n");

	KFILE_write(iFd_Result, g_readbuftmp);
	sprintf(g_readbuftmp,"%s%u%s%u%s%u%s%u%s",
		"    <KUNIT_RUN_SUMMARY_RECORD> \n"
		"      <TYPE> Assertions </TYPE> \n"
		"      <TOTAL> ", pRunSummary->nAsserts,
		" </TOTAL> \n"
		"      <RUN> ", pRunSummary->nAsserts,
		" </RUN> \n"
		"      <SUCCEEDED> ", pRunSummary->nAsserts - pRunSummary->nAssertsFailed,
		" </SUCCEEDED> \n"
		"      <FAILED> ", pRunSummary->nAssertsFailed,
		" </FAILED> \n"
		"    </KUNIT_RUN_SUMMARY_RECORD> \n"
		"  </KUNIT_RUN_SUMMARY> \n");
	KFILE_write(iFd_Result, g_readbuftmp);

	// uSDC项目组打印信息
	SPRINTF_DBG("%s%8u%8u%s%8u%s%8u%8u%8u%8u%s%8u%8u%8u%8u%s",
		"\n\n--Run Summary: Type      Total     Ran  Passed  Failed"
		"\n               suites ", pRegistry->uiNumberOfSuites, pRunSummary->nSuitesRun,
		"     n/a", pRunSummary->nSuitesFailed, "\n               tests  ",
		pRegistry->uiNumberOfTests, pRunSummary->nTestsRun, pRunSummary->nTestsRun - pRunSummary->nTestsFailed,
		pRunSummary->nTestsFailed, "\n               asserts", pRunSummary->nAsserts,
		pRunSummary->nAsserts, pRunSummary->nAsserts - pRunSummary->nAssertsFailed,
		pRunSummary->nAssertsFailed,"\n");
}


/********************************************************
  * 函数名称: automated_all_tests_complete_message_handler
  *
  * 功能描述:  测试包初始化失败后向
  *                          测试结果文件中写XML文件信息
  *
  * 调用函数: KFILE_write
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         Param1: const KU_pSuite pSuite
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void automated_suite_init_failure_message_handler(const KU_pSuite pSuite)
{
	if (f_bWriting_KUNIT_RUN_SUITE == KU_TRUE ) {
		KFILE_write(iFd_Result,
			"      </KUNIT_RUN_SUITE_SUCCESS> \n"
			"    </KUNIT_RUN_SUITE> \n");
		f_bWriting_KUNIT_RUN_SUITE = KU_FALSE;
	}
	sprintf(g_readbuftmp,"%s%s%s%s%s",
		"    <KUNIT_RUN_SUITE> \n"
		"      <KUNIT_RUN_SUITE_FAILURE> \n"
		"        <SUITE_NAME> ", (NULL != pSuite->pName) ? pSuite->pName : "",
		" </SUITE_NAME> \n"
		"        <FAILURE_REASON> ",
		"Suite Initialization Failed",
		" </FAILURE_REASON> \n"
		"      </KUNIT_RUN_SUITE_FAILURE> \n"
		"    </KUNIT_RUN_SUITE>  \n");

	KFILE_write(iFd_Result, g_readbuftmp);
}


/********************************************************
  * 函数名称: automated_suite_cleanup_failure_message_handler
  *
  * 功能描述:  测试包清理失败后向
  *                          测试结果文件中写XML文件信息
  *
  * 调用函数: KFILE_write
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         Param1: const KU_pSuite pSuite
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void automated_suite_cleanup_failure_message_handler(const KU_pSuite pSuite)
{
	if (f_bWriting_KUNIT_RUN_SUITE == KU_TRUE) {
		KFILE_write(iFd_Result,
			"      </KUNIT_RUN_SUITE_SUCCESS> \n"
			"    </KUNIT_RUN_SUITE> \n");
		f_bWriting_KUNIT_RUN_SUITE = KU_FALSE;
	}
	sprintf(g_readbuftmp,"%s%s%s%s%s",
		"    <KUNIT_RUN_SUITE> \n"
		"      <KUNIT_RUN_SUITE_FAILURE> \n"
		"        <SUITE_NAME> ", (NULL != pSuite->pName) ? pSuite->pName : "",
		" </SUITE_NAME> \n"
		"        <FAILURE_REASON> ",
		"Suite Cleanup Failed",
		" </FAILURE_REASON> \n"
		"      </KUNIT_RUN_SUITE_FAILURE> \n"
		"    </KUNIT_RUN_SUITE>  \n");

	KFILE_write(iFd_Result, g_readbuftmp);
}


/********************************************************
  * 函数名称: uninitialize_result_file
  *
  * 功能描述: 关闭测试结果文件
  *
  * 调用函数: KFILE_write    KFILE_close
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         无
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_ErrorCode uninitialize_result_file(void)
{
	KU_set_error(KUE_SUCCESS);
	KFILE_write(iFd_Result,
		"  <KUNIT_FOOTER> File Generated By KUnit v" KU_VERSION
		"  </KUNIT_FOOTER> \n"
		"</KUNIT_TEST_RUN_REPORT>");
	if (0 != KFILE_close(iFd_Result))
		KU_set_error(KUE_FCLOSE_FAILED);

	return KU_get_error();
}

/********************************************************
  * 函数名称: automated_run_all_tests
  *
  * 功能描述:  运行所有已注册测试用例
  *
  * 调用函数: KFILE_write  KU_run_all_tests
  *
  * 被调函数: KU_automated_run_tests
  *
  * 输入参数:
  *                         无
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void automated_run_all_tests(void)
{
	f_pRunningSuite = NULL;
	KFILE_write(iFd_Result, "  <KUNIT_RESULT_LISTING> \n");
	KU_run_all_tests();
}


/********************************************************
  * 函数名称: KU_automated_run_tests
  *
  * 功能描述:  自动运行所有已注册测试用例
  *
  * 调用函数: initialize_result_file
  *                         automated_run_all_tests
  *                         uninitialize_result_file
  * 被调函数:
  *
  * 输入参数:
  *                         无
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
void KU_automated_run_tests(void)
{
	if (initialize_result_file(f_szTestResultFileName) != KUE_SUCCESS) {
		printk("KUNIT ERROR : Failed to create/initialize the result file.\n");
	} else {
		/* 设置写 测试结果文件 函数句柄*/
		KU_set_test_start_handler(automated_test_start_message_handler);
		KU_set_test_complete_handler(automated_test_complete_message_handler);
		KU_set_all_test_complete_handler(automated_all_tests_complete_message_handler);
		KU_set_suite_init_failure_handler(automated_suite_init_failure_message_handler);
		KU_set_suite_cleanup_failure_handler(automated_suite_cleanup_failure_message_handler);
		f_bWriting_KUNIT_RUN_SUITE = KU_FALSE;
		automated_run_all_tests();
		if (uninitialize_result_file() != KUE_SUCCESS )
			printk("KUNIT ERROR : Failed to close/uninitialize the result files.\n");
	}
}

