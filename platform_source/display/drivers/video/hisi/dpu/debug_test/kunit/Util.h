/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Util.h
*
* 版    本 :
*
* 功    能 : 写XML  文件相关的宏定义、函数说明
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

#ifndef KUNIT_H_UTIL
#define KUNIT_H_UTIL
#include <linux/syscalls.h>

/**** 宏定义 ****/

/** 字符串最大长度. */
#define KUNIT_MAX_STRING_LENGTH    1024

/** XML 实体解析过程中最大字符数. */
#define KUNIT_MAX_ENTITY_LEN 5

/** 特殊字符解析. */
int KU_translate_special_characters(const char* szSrc, char* szDest, size_t maxlen);

#endif

