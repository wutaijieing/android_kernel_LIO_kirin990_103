/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *  Description: hkip adapt when kernel boot from uefi.
 *  Create : 2020/4/26
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <platform_include/see/hkip.h>

int hkip_register_rox_text(const void *base, size_t size)
{
	uintptr_t addr = (uintptr_t)base;

	BUG_ON((addr | size) & ~PAGE_MASK);

	if (hkip_hvc3(HHEE_HVC_ROX_TEXT_REGISTER, addr, size))
		return -ENOTSUPP;
	return 0;
}
