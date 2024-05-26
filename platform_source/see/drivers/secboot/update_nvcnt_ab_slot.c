/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: secboot trigger update nv counter in cdc(enable AB slot)
 * Create: 2022/07/07
 */

#include <platform_include/see/secboot.h>
#include <platform_include/see/efuse_driver.h>
#include <linux/kernel.h>


void seb_trigger_update_version(void)
{
	s32 ret;
	u8 buf[EFUSE_NVCNT_LENGTH_BYTES] = {0};

	ret = efuse_update_nvcnt(&buf[0], sizeof(buf));
	if (ret != OK) {
		pr_err("%s: update nvcnt failed,ret=%d\n", __func__, ret);
		return;
	}
}

