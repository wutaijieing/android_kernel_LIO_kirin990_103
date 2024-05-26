/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Regist.h
*
* 版    本 :
*
* 功    能 : 实现Kunit 注册和注销相关结构体定义&  函数声明
*
* 作    者 :
*
* 创建日期 : 2011-03-07
*
* 修改记录 :
*
*    1: <时  间> :
*
*       <作  者> :
*
*       <版  本> :
*
*       <内  容> :
*
*    2:
**********************************************************************************/

#ifndef KUNIT_H_REGIST
#define KUNIT_H_REGIST

#include "Kerror.h"
#include <linux/slab.h>
#include <linux/gfp.h>

#define KU_BOOL int
#define KU_TRUE  1
#define KU_FALSE 0

/* 内存管理 */
#define KU_MALLOC(x)            kmalloc((x), GFP_KERNEL)
#define KU_FREE(x)              kfree((x))

/**** 结构体定义 ****/

/** 测试包初始化函数. */
typedef int  (*KU_InitializeFunc)(void);
/** 测试包清理函数. */
typedef int  (*KU_CleanupFunc)(void);
/** 测试用例函数. */
typedef void (*KU_TestFunc)(void);

/** 测试用例数据类型. */
typedef struct KU_Test
{
	char*           pName;                 /** 测试用例名. */
	KU_TestFunc     pTestFunc;             /** 测试用例函数. */
	struct KU_Test* pNext;                 /** 下一个测试用例. */
	struct KU_Test* pPrev;                 /** 前一个测试用例. */
} KU_Test;
typedef KU_Test* KU_pTest;           /**KU_Test指针类型. */

/** 测试包数据类型. */
typedef struct KU_Suite
{
	char*             pName;                 /** 测试包名称. */
	KU_pTest          pTest;                 /** 第一个测试用例. */
	KU_InitializeFunc pInitializeFunc;       /** 初始化函数. */
	KU_CleanupFunc    pCleanupFunc;          /** 清理函数. */
	unsigned int      uiNumberOfTests;       /** 测试用例数目. */
	struct KU_Suite*  pNext;                 /** 下一个测试包. */
	struct KU_Suite*  pPrev;                 /** 前一个测试包. */
} KU_Suite;
typedef KU_Suite* KU_pSuite;           /** KU_Suite指针类型. */

/** 注册器数据类型. */
typedef struct KU_TestRegistry
{
	unsigned int uiNumberOfSuites;           /** 测试包数目. */
	unsigned int uiNumberOfTests;            /** 测试用例数目. */
	KU_pSuite    pSuite;                     /** 第一个测试包. */
} KU_TestRegistry;
typedef KU_TestRegistry* KU_pTestRegistry;  /** KU_TestRegistry指针类型. */

/** 测试用例信息类型. */
/** 供用户使用. */
typedef struct KU_TestInfo
{
	char       *pName;                          /** 测试用例名. */
	KU_TestFunc pTestFunc;                      /** 测试用例函数. */
} KU_TestInfo;
typedef KU_TestInfo* KU_pTestInfo;   /** KU_TestInfo指针类型. */

/** 测试包信息类型. */
/** 供用户使用. */
typedef struct KU_SuiteInfo
{
	char             *pName;                   /** 测试包名称. */
	KU_InitializeFunc pInitFunc;               /** 初始化函数. */
	KU_CleanupFunc    pCleanupFunc;            /** 清理函数 */
	KU_TestInfo      *pTests;                  /** 测试用例数组. */
	KU_BOOL    bRegist;                        /** 测试包注册标签. */
} KU_SuiteInfo;
typedef KU_SuiteInfo* KU_pSuiteInfo;  /** KU_SuiteInfo指针类型. */

/** NULL CU_test_info_t to terminate arrays of tests. */
#define KU_TEST_INFO_NULL { NULL, NULL }

/** NULL CU_suite_info_t to terminate arrays of suites. */
#define KU_SUITE_INFO_NULL { NULL, NULL, NULL, NULL }

/**** 函数声明 ****/

/** 初始化注册器. */
KU_ErrorCode KU_initialize_registry(void);
/** 清理注册器. */
KU_ErrorCode KU_cleanup_registry(void);
/** 获取注册器. */
KU_pTestRegistry KU_get_registry(void);
/** 注册测试包. */
KU_ErrorCode KU_register_suites(KU_SuiteInfo suite_info[]);


#endif

