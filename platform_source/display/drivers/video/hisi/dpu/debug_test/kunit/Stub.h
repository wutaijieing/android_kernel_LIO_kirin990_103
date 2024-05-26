/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Stub.h
*
* 版    本 :
*
* 功    能 : 打桩函数宏定义&  函数声明
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

#ifndef KUNIT_H_STUB
#define KUNIT_H_STUB

/**** 函数指针定义****/
typedef void (*CommonFunc)(void);


/** 清除所有桩函数. */
void ClearAllFuncStubs(void);
/** 对指定函数打桩. */
int SetFuncStub(CommonFunc fnOldFunc, CommonFunc fnNewFunc);
/** 清除指定函数的桩函数. */
int SetFuncStubNone(CommonFunc fnOldFunc);

/**** 宏定义****/

/** 对指定函数打桩. */
#define SET_FUNC_STUB(oldFunc,newFunc) SetFuncStub((CommonFunc)oldFunc, (CommonFunc)newFunc)

/** 清除所有桩函数. */
#define CLEAR_ALL_FUNC_STUBS ClearAllFuncStubs()

/** 清除指定函数的桩函数. */
#define CLEAR_FUNC_STUB(oldFunc)  SetFuncStubNone((CommonFunc)oldFunc)

#endif

