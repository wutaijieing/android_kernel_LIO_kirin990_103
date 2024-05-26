/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description:common_func.h
 * Create:2019.11.25
 */

#ifndef _COMMON_FUNC_H_
#define _COMMON_FUNC_H_
#include <platform_include/smart/linux/base/ap/protocol.h>

/*
 * Function    : really_do_enable_disable
 * Description : according to ref cnt and enable, deciding real operate
 * Input       : [ref_cnt] reference cnt
 *             : [enable] enable or not
 *             : [bit] bit in *ref_cnt to place 0 or 1
 * Output      : none
 * Return      : true is do, false is not to do
 */
bool really_do_enable_disable(unsigned int *ref_cnt, bool enable, int bit);

#endif /* _COMMON_FUNC_H_ */
