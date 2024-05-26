/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: socid
 * Create: 2021/08/17
 */
#ifndef __SOCID_H__
#define __SOCID_H__

#include <linux/types.h>
#include <linux/kernel.h>
#ifdef CONFIG_SECURITY_INFO
#include <teek_client_api.h>
#include <teek_client_id.h>

s32 get_socid(u8 *buf, u32 size);
int teek_init(TEEC_Session *session, TEEC_Context *context);

#else
static s32 get_socid(u8 *buf, u32 size)
{
	(void)buf;
	(void)size;
	pr_err("%s not implement\n", __func__);
	return -1;
}
#endif

#endif
