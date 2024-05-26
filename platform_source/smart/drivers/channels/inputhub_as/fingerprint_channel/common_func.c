/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description:common_func.c
 * Create:2019.11.25
 */
#include <securec.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ipc.h"
#include "iomcu_log.h"

#define MAX_BIT_NUM (sizeof(int32_t) << 3)
static DEFINE_SPINLOCK(g_ref_cnt_lock);

bool really_do_enable_disable(unsigned int *ref_cnt, bool enable, int bit)
{
	bool ret = false;
	unsigned long flags;

	if ((bit < 0) || (bit >= (int)MAX_BIT_NUM)) {
		ctxhub_err("bit %d out of range in %s.\n", bit, __func__);
		return false;
	}

	spin_lock_irqsave(&g_ref_cnt_lock, flags);
	if (enable) {
		ret = (*ref_cnt == 0);
		*ref_cnt |= 1 << bit;
	} else {
		*ref_cnt &= ~(unsigned int)(1 << bit);
		ret = (*ref_cnt == 0);
	}
	spin_unlock_irqrestore(&g_ref_cnt_lock, flags);
	return ret;
}
