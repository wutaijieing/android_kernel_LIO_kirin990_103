/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/mm.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/security.h>
#include <linux/vfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include "xml.h"
#include "util.h"

#define FILENAME_MAX    255
static char      f_sz_sefault_file_root[] = "KUnit";

/** Test list file name. */
static char      f_sz_test_list_file_name[FILENAME_MAX] = "";

/** The test results file name. */
static char      f_sz_test_result_file_name[FILENAME_MAX] = "";

/** String cache array. */
static char      g_readbuftmp[BUFFER_SIZE];

/** Test results file handle. */
static unsigned int ifd_result;

/** The test package information write XML tags. */
static KU_BOOL f_bwriting_kunit_run_suite = KU_FALSE;

/** The current test package. */
static ku_psuite f_prunning_suite = NULL;

#define SPRINTF_DBG(fmt, args...)

/**
 * @brief In the current process description file descriptor fd in the table refers to the file structure
 *
 * @param fd
 * @param fput_needed
 * @return struct file*
 */
struct file *kfile_fget_light(unsigned int fd, int *fput_needed)
{
	struct file *file = NULL;
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

/**
 * @brief Open the file
 *
 * @param filename
 * @param flags
 * @param mode
 * @return unsigned int
 */
unsigned int kfile_open(const char *filename, int flags, int mode)
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

/**
 * @brief Close the file
 *
 * @param fd
 * @return unsigned int
 */
unsigned int kfile_close(unsigned int fd)
{
	return ksys_close(fd);;
}

/**
 * @brief Write a string to the file
 *
 * @param fd
 * @param buf
 * @return ssize_t
 */
ssize_t kfile_write(unsigned int fd, const char * buf)
{
	struct file *file;
	ssize_t ret = -EBADF;
	int fput_needed;
	mm_segment_t oldfs;
	size_t count;

	count = strlen(buf);
	file = kfile_fget_light(fd, &fput_needed);
	if (file) {
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		ret = vfs_write(file, buf, count, &file->f_pos);
		set_fs(oldfs);
		fput_light(file, fput_needed);
	}

	return ret;
}

/**
 * @brief Set the output file name
 *
 * @param sz_file_name_root
 */
void ku_set_output_filename(const char* sz_file_name_root)
{
	const char* sz_list_ending = "-Listing.xml";
	const char* sz_result_ending = "-Results.xml";

	/* Structural testing file name list */
	if (sz_file_name_root)
		strncpy(f_sz_test_list_file_name, sz_file_name_root, FILENAME_MAX - strlen(sz_list_ending) - 1);
	else
		strncpy(f_sz_test_list_file_name, f_sz_sefault_file_root, FILENAME_MAX - strlen(sz_list_ending) - 1);

	f_sz_test_list_file_name[FILENAME_MAX - strlen(sz_list_ending) - 1] = '\0';
	strcat(f_sz_test_list_file_name, sz_list_ending);

	/* File name structure test result */
	if (sz_file_name_root)
		strncpy(f_sz_test_result_file_name, sz_file_name_root, FILENAME_MAX - strlen(sz_result_ending) - 1);
	else
		strncpy(f_sz_test_result_file_name, f_sz_sefault_file_root, FILENAME_MAX - strlen(sz_result_ending) - 1);

	f_sz_test_result_file_name[FILENAME_MAX - strlen(sz_result_ending) - 1] = '\0';
	strcat(f_sz_test_result_file_name, sz_result_ending);
}

/**
 * @brief Write the test list file
 *
 * @param pregistry
 * @param sz_filename
 * @return ku_error_code
 */
static ku_error_code automated_list_all_tests(ku_ptest_registry pregistry, const char* sz_filename)
{
	ku_psuite psuite = NULL;
	ku_ptest  ptest = NULL;
	unsigned int ifd=-1;

	ku_set_error(KUE_SUCCESS);
	if (!pregistry) {
		ku_set_error(KUE_NOREGISTRY);
	} else if ((!sz_filename) || (strlen(sz_filename) == 0)) {
		ku_set_error(KUE_BAD_FILENAME);
	} else if ((ifd = kfile_open(f_sz_test_list_file_name, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) < 0) {
		ku_set_error(KUE_FOPEN_FAILED);
	} else {
		kfile_write(ifd,
			"<?xml version=\"1.0\" encoding=\"gb2312\"?> \n"
			"<?xml-stylesheet type=\"text/xsl\" href=\"KUnit-List.xsl\" ?> \n"
			"<!DOCTYPE KUNIT_TEST_LIST_REPORT SYSTEM \"KUnit-List.dtd\"> \n"
			"<KUNIT_TEST_LIST_REPORT> \n"
			"  <KUNIT_HEADER/> \n"
			"  <KUNIT_LIST_TOTAL_SUMMARY> \n");

		sprintf(g_readbuftmp, "%s%u%s",
			"    <KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
			"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> Total Number of Suites </KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> \n"
			"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> ", pregistry->ui_number_of_suites,
			" </KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> \n"
			"    </KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n");
		kfile_write(ifd, g_readbuftmp);

		sprintf(g_readbuftmp,"%s%u%s",
		"    <KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
		"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> Total Number of test Cases </KUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> \n"
		"      <KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> ",pregistry->ui_number_of_tests,
		" </KUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> \n"
		"    </KUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
		"  </KUNIT_LIST_TOTAL_SUMMARY> \n");
		kfile_write(ifd, g_readbuftmp);
		kfile_write(ifd, "  <KUNIT_ALL_TEST_LISTING> \n");

		psuite = pregistry->psuite;
		while (psuite) {
			ptest = psuite->ptest;
			sprintf(g_readbuftmp,"%s%s%s%s%s%s%s%u%s",
				"    <KUNIT_ALL_TEST_LISTING_SUITE> \n"
				"      <KUNIT_ALL_TEST_LISTING_SUITE_DEFINITION> \n"
				"        <SUITE_NAME> ", (NULL != psuite->pname) ? psuite->pname : "",
				" </SUITE_NAME> \n"
				"        <INITIALIZE_VALUE> ", (NULL != psuite->pinitialize_func) ? "Yes" : "No",
				" </INITIALIZE_VALUE> \n"
				"        <CLEANUP_VALUE> ", (NULL != psuite->pcleanup_func) ? "Yes" : "No",
				" </CLEANUP_VALUE> \n"
				"        <TEST_COUNT_VALUE> ", psuite->ui_number_of_tests,
				" </TEST_COUNT_VALUE> \n"
				"      </KUNIT_ALL_TEST_LISTING_SUITE_DEFINITION> \n");
			kfile_write(ifd, g_readbuftmp);
			kfile_write(ifd, "      <KUNIT_ALL_TEST_LISTING_SUITE_TESTS> \n");

			while (NULL != ptest) {
				sprintf(g_readbuftmp,"%s%s%s",
					"        <TEST_CASE_NAME> ", (NULL != ptest->pname) ? ptest->pname : "",
					" </TEST_CASE_NAME> \n");
				kfile_write(ifd, g_readbuftmp);
				ptest = ptest->pnext;
			}

			kfile_write(ifd,
				"      </KUNIT_ALL_TEST_LISTING_SUITE_TESTS> \n"
				"    </KUNIT_ALL_TEST_LISTING_SUITE> \n");

			psuite = psuite->pnext;
		}

		kfile_write(ifd, "  </KUNIT_ALL_TEST_LISTING> \n");
		kfile_write(ifd,
			"  <KUNIT_FOOTER> File Generated By KUnit v" KU_VERSION " \n </KUNIT_FOOTER> \n"
			"</KUNIT_TEST_LIST_REPORT> \n");
	}

	kfile_close(ifd);
	return ku_get_error();
}

/**
 * @brief Generate test list file
 *
 * @return ku_error_code
 */
ku_error_code ku_list_tests_to_file(void)
{
	return automated_list_all_tests(ku_get_registry(), f_sz_test_list_file_name);
}

/**
 * @brief Initialize test result file
 *
 * @param sz_filename
 * @return ku_error_code
 */
static ku_error_code initialize_result_file(const char* sz_filename)
{
	ku_set_error(KUE_SUCCESS);
	if ((!sz_filename) || (strlen(sz_filename) == 0)) {
		ku_set_error(KUE_BAD_FILENAME);
	} else if ((ifd_result = kfile_open(sz_filename, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) < 0) {
		ku_set_error(KUE_FOPEN_FAILED);
	} else {
		kfile_write(ifd_result,
			"<?xml version=\"1.0\" encoding=\"gb2312\"?> \n"
			"<?xml-stylesheet type=\"text/xsl\" href=\"KUnit-Run.xsl\" ?> \n"
			"<!DOCTYPE KUNIT_TEST_RUN_REPORT SYSTEM \"KUnit-Run.dtd\"> \n"
			"<KUNIT_TEST_RUN_REPORT> \n"
			"  <KUNIT_HEADER/> \n");
	}

	return ku_get_error();
}

/**
 * @brief Execute the test before the package to the test results file write XML file information
 *
 * @param ptest
 * @param psuite
 */
static void automated_test_start_message_handler(const ku_ptest ptest, const ku_psuite psuite)
{
	/* write suite close/open tags if this is the 1st test for this szSuite */
	if ((!f_prunning_suite) || (f_prunning_suite != psuite)) {
		if (f_bwriting_kunit_run_suite == KU_TRUE)
			kfile_write(ifd_result,
				"      </KUNIT_RUN_SUITE_SUCCESS> \n"
				"    </KUNIT_RUN_SUITE> \n");

		sprintf(g_readbuftmp,"%s%s%s",
			"    <KUNIT_RUN_SUITE> \n"
			"      <KUNIT_RUN_SUITE_SUCCESS> \n"
			"        <SUITE_NAME> ", (NULL != psuite->pname) ? psuite->pname : "",
			" </SUITE_NAME> \n");

		kfile_write(ifd_result,g_readbuftmp);
		f_bwriting_kunit_run_suite = KU_TRUE;
		f_prunning_suite = psuite;
	}
}

/**
 * @brief After running a test case file writing XML file information to the test result
 *
 * @param ptest
 * @param psuite
 * @param pfailure
 */
static void automated_test_complete_message_handler(const ku_ptest ptest,
	const ku_psuite psuite, const ku_pfailure_record pfailure)
{
	ku_pfailure_record ptemp_failure = pfailure;
	int i = 1;
	char *sz_temp = NULL;

	if (ptemp_failure) {
		/* Temporary failure information storage arrays */
		sz_temp = KU_MALLOC(KUNIT_MAX_ENTITY_LEN * KUNIT_MAX_STRING_LENGTH);
		if (!sz_temp)
			return;

		SPRINTF_DBG("%s%s%s%s%s", "\nSuite ", (NULL != psuite->pname) ? psuite->pname : "", ", test ",
			(NULL != ptest->pname) ? ptest->pname : "", " had failures:");

		while (ptemp_failure) {
			if (ptemp_failure->str_condition)
				ku_translate_special_characters(ptemp_failure->str_condition, sz_temp, KUNIT_MAX_ENTITY_LEN * KUNIT_MAX_STRING_LENGTH);
			else
				sz_temp[0] = '\0';

			sprintf(g_readbuftmp,"%s%s%s%s%s%u%s%s%s",
				"        <KUNIT_RUN_TEST_RECORD> \n"
				"          <KUNIT_RUN_TEST_FAILURE> \n"
				"            <TEST_NAME> ", (NULL != ptest->pname) ? ptest->pname : "",
				" </TEST_NAME> \n"
				"            <FILE_NAME> ", (NULL != ptemp_failure->str_file_name) ? ptemp_failure->str_file_name : "",
				" </FILE_NAME> \n"
				"            <LINE_NUMBER> ", ptemp_failure->ui_line_number,
				" </LINE_NUMBER> \n"
				"            <CONDITION> ", sz_temp,
				" </CONDITION> \n"
				"          </KUNIT_RUN_TEST_FAILURE> \n"
				"        </KUNIT_RUN_TEST_RECORD> \n");
			kfile_write(ifd_result, g_readbuftmp);

			SPRINTF_DBG("%s%d%s%s%s%u%s%s", "\n    ", i, ". ",
				(NULL != pfailure->str_file_name) ? pfailure->str_file_name : "", ":", pfailure->ui_line_number, "  - ",
				(NULL != pfailure->str_condition) ? pfailure->str_condition : "");
			i++;
			ptemp_failure = ptemp_failure->pnext;
		}
	} else {
		sprintf(g_readbuftmp,"%s%s%s",
			"        <KUNIT_RUN_TEST_RECORD> \n"
			"          <KUNIT_RUN_TEST_SUCCESS> \n"
			"            <TEST_NAME> ", (NULL != ptest->pname) ? ptest->pname : "",
			" </TEST_NAME> \n"
			"          </KUNIT_RUN_TEST_SUCCESS> \n"
			"        </KUNIT_RUN_TEST_RECORD> \n");

		kfile_write(ifd_result, g_readbuftmp);
	}
}

/**
 * @brief Run after a test package all the test cases to the test results file write XML file information
 *
 * @param pfailure
 */
static void automated_all_tests_complete_message_handler(const ku_pfailure_record pfailure)
{
	ku_ptest_registry pregistry = ku_get_registry();
	ku_prun_summary prun_summary = ku_get_run_summary();

	if ((f_prunning_suite) && (f_bwriting_kunit_run_suite == KU_TRUE))
		kfile_write(ifd_result,
			"      </KUNIT_RUN_SUITE_SUCCESS> \n"
			"    </KUNIT_RUN_SUITE> \n");

	kfile_write(ifd_result,
		"  </KUNIT_RESULT_LISTING>\n"
		"  <KUNIT_RUN_SUMMARY> \n");

	sprintf(g_readbuftmp,"%s%u%s%u%s%u%s",
		"    <KUNIT_RUN_SUMMARY_RECORD> \n"
		"      <TYPE> Suites </TYPE> \n"
		"      <TOTAL> ", pregistry->ui_number_of_suites,
		" </TOTAL> \n"
		"      <RUN> ", prun_summary->n_suites_run,
		" </RUN> \n"
		"      <SUCCEEDED> - NA - </SUCCEEDED> \n"
		"      <FAILED> ", prun_summary->n_suites_failed,
		" </FAILED> \n"
		"    </KUNIT_RUN_SUMMARY_RECORD> \n");

	kfile_write(ifd_result, g_readbuftmp);
	sprintf(g_readbuftmp,"%s%u%s%u%s%u%s%u%s",
		"    <KUNIT_RUN_SUMMARY_RECORD> \n"
		"      <TYPE> test Cases </TYPE> \n"
		"      <TOTAL> ", pregistry->ui_number_of_tests,
		" </TOTAL> \n"
		"      <RUN> ", prun_summary->n_tests_run,
		" </RUN> \n"
		"      <SUCCEEDED> ", prun_summary->n_tests_run - prun_summary->n_tests_failed,
		" </SUCCEEDED> \n"
		"      <FAILED> ", prun_summary->n_tests_failed,
		" </FAILED> \n"
		"    </KUNIT_RUN_SUMMARY_RECORD> \n");

	kfile_write(ifd_result, g_readbuftmp);
	sprintf(g_readbuftmp,"%s%u%s%u%s%u%s%u%s",
		"    <KUNIT_RUN_SUMMARY_RECORD> \n"
		"      <TYPE> Assertions </TYPE> \n"
		"      <TOTAL> ", prun_summary->n_asserts,
		" </TOTAL> \n"
		"      <RUN> ", prun_summary->n_asserts,
		" </RUN> \n"
		"      <SUCCEEDED> ", prun_summary->n_asserts - prun_summary->n_asserts_failed,
		" </SUCCEEDED> \n"
		"      <FAILED> ", prun_summary->n_asserts_failed,
		" </FAILED> \n"
		"    </KUNIT_RUN_SUMMARY_RECORD> \n"
		"  </KUNIT_RUN_SUMMARY> \n");
	kfile_write(ifd_result, g_readbuftmp);

	SPRINTF_DBG("%s%8u%8u%s%8u%s%8u%8u%8u%8u%s%8u%8u%8u%8u%s",
		"\n\n--Run Summary: Type      Total     Ran  Passed  Failed"
		"\n               suites ", pregistry->ui_number_of_suites, prun_summary->n_suites_run,
		"     n/a", prun_summary->n_suites_failed, "\n               tests  ",
		pregistry->ui_number_of_tests, prun_summary->n_tests_run, prun_summary->n_tests_run - prun_summary->n_tests_failed,
		prun_summary->n_tests_failed, "\n               asserts", prun_summary->n_asserts,
		prun_summary->n_asserts, prun_summary->n_asserts - prun_summary->n_asserts_failed,
		prun_summary->n_asserts_failed,"\n");
}

/**
 * @brief failed to initiate the test package to the test results after file write XML file information
 *
 * @param psuite
 */
static void automated_suite_init_failure_message_handler(const ku_psuite psuite)
{
	if (f_bwriting_kunit_run_suite == KU_TRUE ) {
		kfile_write(ifd_result,
			"      </KUNIT_RUN_SUITE_SUCCESS> \n"
			"    </KUNIT_RUN_SUITE> \n");
		f_bwriting_kunit_run_suite = KU_FALSE;
	}
	sprintf(g_readbuftmp,"%s%s%s%s%s",
		"    <KUNIT_RUN_SUITE> \n"
		"      <KUNIT_RUN_SUITE_FAILURE> \n"
		"        <SUITE_NAME> ", (NULL != psuite->pname) ? psuite->pname : "",
		" </SUITE_NAME> \n"
		"        <FAILURE_REASON> ",
		"Suite Initialization Failed",
		" </FAILURE_REASON> \n"
		"      </KUNIT_RUN_SUITE_FAILURE> \n"
		"    </KUNIT_RUN_SUITE>  \n");

	kfile_write(ifd_result, g_readbuftmp);
}

/**
 * @brief Test package cleaning after failing to the test results file write XML file information
 *
 * @param psuite
 */
static void automated_suite_cleanup_failure_message_handler(const ku_psuite psuite)
{
	if (f_bwriting_kunit_run_suite == KU_TRUE) {
		kfile_write(ifd_result,
			"      </KUNIT_RUN_SUITE_SUCCESS> \n"
			"    </KUNIT_RUN_SUITE> \n");
		f_bwriting_kunit_run_suite = KU_FALSE;
	}
	sprintf(g_readbuftmp,"%s%s%s%s%s",
		"    <KUNIT_RUN_SUITE> \n"
		"      <KUNIT_RUN_SUITE_FAILURE> \n"
		"        <SUITE_NAME> ", (NULL != psuite->pname) ? psuite->pname : "",
		" </SUITE_NAME> \n"
		"        <FAILURE_REASON> ",
		"Suite Cleanup Failed",
		" </FAILURE_REASON> \n"
		"      </KUNIT_RUN_SUITE_FAILURE> \n"
		"    </KUNIT_RUN_SUITE>  \n");

	kfile_write(ifd_result, g_readbuftmp);
}

/**
 * @brief Close test result file
 *
 * @return ku_error_code
 */
static ku_error_code uninitialize_result_file(void)
{
	ku_set_error(KUE_SUCCESS);
	kfile_write(ifd_result,
		"  <KUNIT_FOOTER> File Generated By KUnit v" KU_VERSION
		"  </KUNIT_FOOTER> \n"
		"</KUNIT_TEST_RUN_REPORT>");
	if (0 != kfile_close(ifd_result))
		ku_set_error(KUE_FCLOSE_FAILED);

	return ku_get_error();
}

/**
 * @brief Run all the registered test cases
 *
 */
static void automated_run_all_tests(void)
{
	f_prunning_suite = NULL;
	kfile_write(ifd_result, "  <KUNIT_RESULT_LISTING> \n");
	ku_run_all_tests();
}

/**
 * @brief All registered test cases to run automatically
 *
 */
void ku_automated_run_tests(void)
{
	if (initialize_result_file(f_sz_test_result_file_name) != KUE_SUCCESS) {
		printk("KUNIT ERROR : Failed to create/initialize the result file.\n");
	} else {
		/* Set the write function test result file handle */
		ku_set_test_start_handler(automated_test_start_message_handler);
		ku_set_test_complete_handler(automated_test_complete_message_handler);
		ku_set_all_test_complete_handler(automated_all_tests_complete_message_handler);
		ku_set_suite_init_failure_handler(automated_suite_init_failure_message_handler);
		ku_set_suite_cleanup_failure_handler(automated_suite_cleanup_failure_message_handler);
		f_bwriting_kunit_run_suite = KU_FALSE;
		automated_run_all_tests();
		if (uninitialize_result_file() != KUE_SUCCESS )
			printk("KUNIT ERROR : Failed to close/uninitialize the result files.\n");
	}
}
