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

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "test.h"
#include "xml.h"
#include "regist.h"

/**
 * @brief Run all test cases
 *        Call functions: ku_initialize_registry
 *                 ku_register_suites
 *                 ku_list_tests_to_file
 *                 ku_automated_run_tests
 *                 ku_cleanup_registry
 * @param suite_info
 * @param sz_file_name_root
 */
void run_all_tests(ku_suite_info suite_info[], char* sz_file_name_root)
{
	ku_error_code result;

	result = ku_initialize_registry();
	if (result != KUE_SUCCESS) {
		printk("KUNIT ERROR : initialize error code is :%d\n", result);
		return;
	}

	result = ku_register_suites(suite_info);
	if (result != KUE_SUCCESS)
		printk("KUNIT ERROR : register error code is :%d\n", result);

	ku_set_output_filename(sz_file_name_root);

	result = ku_list_tests_to_file();
	if (result != KUE_SUCCESS)
		printk("KUNIT ERROR : list file error code is :%d\n",result);

	ku_automated_run_tests();

	result = ku_cleanup_registry();
	printk("KUNIT ERROR : error code is :%d\n",result);
}
EXPORT_SYMBOL(run_all_tests);

static int kunit_init(void)
{
	printk("hello,kunit!\n");
	return 0;
}

static void kunit_exit(void)
{
	printk("bye,kunit!\n");
}

module_init(kunit_init);
module_exit(kunit_exit);

MODULE_LICENSE("GPL");
