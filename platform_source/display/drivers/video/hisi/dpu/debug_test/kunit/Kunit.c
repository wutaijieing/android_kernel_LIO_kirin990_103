/* @file
 * Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
 *
 * 文件名称 : Kunit.c
 *
 * 版    本 :
 *
 * 功    能 : 用户调用接口的实现&  导出
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
 *       <内  容> : 修改函数RUN_ALL_TESTS，增加
 *                            输出文件名设置功能
 *
 *    2:
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
#include "Test.h"
#include "Xml.h"
#include "Regist.h"


/********************************************************
  * 函数名称: RUN_ALL_TESTS
  *
  * 功能描述:运行所有测试用例
  *
  * 调用函数: KU_initialize_registry
  *                         KU_register_suites
  *                         KU_list_tests_to_file
  *                         KU_automated_run_tests
  *                         KU_cleanup_registry
  *
  * 被调函数:
  *
  * 输入参数: Param1: KU_SuiteInfo suite_info[]
  *
  * 输出参数:
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
void RUN_ALL_TESTS(KU_SuiteInfo suite_info[], char* szFilenameRoot)
{
	KU_ErrorCode result;

	result = KU_initialize_registry();
	if (result != KUE_SUCCESS) {
		printk("KUNIT ERROR : initialize error code is :%d\n", result);
		return;
	}

	result = KU_register_suites(suite_info);
	if (result != KUE_SUCCESS) {
		printk("KUNIT ERROR : register error code is :%d\n", result);
	}

	KU_set_output_filename(szFilenameRoot);

	result = KU_list_tests_to_file();
	if (result != KUE_SUCCESS) {
		printk("KUNIT ERROR : list file error code is :%d\n",result);
	}

	KU_automated_run_tests();

	result = KU_cleanup_registry();
	printk("KUNIT ERROR : error code is :%d\n",result);
}


static int kunit_init(void)
{
	printk("hello,kunit!\n");
	return 0;
}

static void kunit_exit(void)
{
	printk("bye,kunit!\n");
}

/** 运行所有测试用例. */
EXPORT_SYMBOL(RUN_ALL_TESTS);

/** 断言执行函数. */
EXPORT_SYMBOL(KU_assertImplementation);

module_init(kunit_init);
module_exit(kunit_exit);

MODULE_LICENSE("GPL");

