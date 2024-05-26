/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Util.c
*
* 版    本 :
*
* 功    能 : 写XML  文件相关函数
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

#include "Util.h"
#include <linux/string.h>


/****结构体定义****/

/**xml实体码和特殊字符的映射结构体. */
static const struct {
	char special_char;
	char* replacement;
} bindings [] = {
	{'&', "&amp;"},
	{'>', "&gt;"},
	{'<', "&lt;"}
};

/********************************************************
  * 函数名称: get_index
  *
  * 功能描述: 判断字符是否为xml  特殊字符
  *
  * 调用函数: 无
  *
  * 被调函数:KU_translate_special_characters
  *
  * 输入参数:
  *                       Param1: char ch
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       大于等于零            执行成功
  *                       小于零(-1)                 执行失败
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
static int get_index(char ch)
{
	int length = sizeof(bindings) / sizeof(bindings[0]);
	int counter;

	for (counter = 0; counter < length && bindings[counter].special_char != ch; ++counter)
		;

	return (counter < length ? counter : -1);
}


/********************************************************
  * 函数名称: KU_translate_special_characters
  *
  * 功能描述: 将xml  特殊字符转换为xml  实体码
  *
  * 调用函数: get_index
  *
  * 被调函数:automated_test_complete_message_handler
  *
  * 输入参数:
  *                       Param1: const char* szSrc
  *                       Param2: char* szDest
  *                       Param3: size_t maxlen
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
int KU_translate_special_characters(const char* szSrc, char* szDest, size_t maxlen)
{
	int count = 0;
	size_t src = 0;
	size_t dest = 0;
	size_t length = strlen(szSrc);
	int conv_index;

	if (!szDest)
		return 0;

	memset(szDest, 0, maxlen);
	while ((dest < maxlen) && (src < length)) {
		if ((-1 != (conv_index = get_index(szSrc[src]))) &&
			((dest + (int)strlen(bindings[conv_index].replacement)) <= maxlen)) {
			strcat(szDest, bindings[conv_index].replacement);
			dest += (int)strlen(bindings[conv_index].replacement);
			++count;
		} else {
			szDest[dest++] = szSrc[src];
		}
		++src;
	}

	return count;
}
