/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Xml.h
*
* 版    本 :
*
* 功    能 : 写xml函数相关宏定义&  函数声明
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

#ifndef KUNIT_H_XML
#define KUNIT_H_XML

#include "Kerror.h"
#include "Regist.h"
#include "Test.h"

/** KUnit 版本号. */
#define KU_VERSION "1.0"

/** 写xml  字符串最大长度. */
#define BUFFER_SIZE 1024

/****  函数声明****/

/** 写测试列表文件函数. */
KU_ErrorCode KU_list_tests_to_file(void);

/** 自动运行测试用例. */
void KU_automated_run_tests(void);

/** 设置输出文件名. */
void KU_set_output_filename(const char* szFilenameRoot);

#endif
