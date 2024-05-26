/** @file
 * Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
 *
 * 文件名称 : Kerror.h
 *
 * 版    本 :
 *
 * 功    能 : 错误码定义
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
 */

#ifndef KUNIT_H_ERROR
#define KUNIT_H_ERROR

/** 错误码结构体定义. */
typedef enum
{
     /* 基本错误*/
    KUE_SUCCESS           = 0,                   /** 没有错误 */
    KUE_FAILED            = 1,                   /** 失败 */
    KUE_NOMEMORY          = 2,                   /** 内存申请失败 */
    KUE_WRONGPARA         = 3,                   /** 参数错 */
    /* 注册错误 */
    KUE_NOREGISTRY        = 10,                  /** 没有注册 */
    KUE_REGISTRY_EXISTS   = 11,                  /** 注册器已存在 */
    KUE_TEST_RUNNING       =12,                  /** 测试用例正在运行 */

    /* 测试包错误 */
    KUE_NOSUITE           = 20,                  /** 没有测试包 */
    KUE_NO_SUITENAME      = 21,                  /** 没有测试包名称 */
    KUE_SINIT_FAILED      = 22,                  /** 测试包初始化失败 */
    KUE_SCLEAN_FAILED     = 23,                  /** 测试包清理失败 */
    KUE_DUP_SUITE         = 24,                  /** 测试包重名 */
    /* 测试用例错误 */
    KUE_NOTEST            = 30,                   /** 没有测试用例 */
    KUE_NO_TESTNAME       = 31,                   /** 没有测试用例名 */
    KUE_DUP_TEST          = 32,                   /** 测试用例重名 */
    KUE_TEST_NOT_IN_SUITE = 33,                   /** 测试包内无此测试用例 */
    /* 文件操作错误 */
    KUE_FOPEN_FAILED      = 40,              /** 文件打开失败 */
    KUE_FCLOSE_FAILED     = 41,              /** 文件关闭失败 */
    KUE_BAD_FILENAME      = 42,              /** 文件名错误 */
    KUE_WRITE_ERROR       = 43               /** 写文件错误 */
} KU_ErrorCode;


/** 设置错误码. */
void KU_set_error(KU_ErrorCode error);
/** 获取错误码. */
KU_ErrorCode KU_get_error(void);

#endif



