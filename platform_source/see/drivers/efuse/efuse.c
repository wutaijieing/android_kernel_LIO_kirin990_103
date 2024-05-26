/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: efuse source file to support interfaces
 * Create: 2022/01/18
 */

#include <platform_include/see/efuse_driver.h>
#include "efuse_internal.h"
#include <linux/kernel.h>        /* pr_err */
#include <linux/mutex.h>         /* mutex_lock */

static DEFINE_MUTEX(g_efuse_cnt_lock);

uint32_t efuse_get_item_bit_cnt(uint32_t item_vid, uint32_t *bit_cnt)
{
	uint32_t ret;
	struct efuse_trans_t trans = { 0 };

	if (bit_cnt == NULL) {
		pr_err ("[%s]: error, bit_cnt is null\n", __func__);
		return EFUSE_INVALID_PARAM_ERR;
	}

	/* get the real bit_cnt of efuse item */
	trans.item_vid = item_vid;
	ret = efuse_get_item_info(&trans);
	if (ret != EFUSE_OK) {
		pr_err ("[%s]: error, get efuse item info, ret=0x%08X\n", __func__, ret);
		return ret;
	}

	*bit_cnt = trans.bit_cnt;
	return EFUSE_OK;
}

uint32_t efuse_read_value_t(struct efuse_desc_t *desc)
{
	uint32_t ret;

	mutex_lock(&g_efuse_cnt_lock);
	ret = efuse_inner_read_value(desc);
	mutex_unlock(&g_efuse_cnt_lock);
	return ret;
}
