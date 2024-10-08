/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: syscounter head file
 * Create: 2021-04-01
 */

#ifndef __BSP_SYSCOUNTER_H__
#define __BSP_SYSCOUNTER_H__

#include <soc_syscounter_interface.h>
#include <soc_acpu_baseaddr_interface.h>

union syscnt_val {
	u64 val;
	struct {
		u32 l32;
		u32 h32;
	}val_lh32;
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
struct syscnt_to_timespec_record {
	u64 syscnt;
	struct timespec64 ts;
	struct timespec64 utc;
};
#else
struct syscnt_to_timespec_record {
	u64 syscnt;
	struct timespec64 ts;
	struct timespec utc;
};
#endif

struct syscnt_device {
	struct resource *res;
	void __iomem *base;
	u64 clock_rate;         /* syscounter clock rate */
	u64 debug_syscnt;
	spinlock_t r_lock;      /* for read 64bit-syscounter */
	spinlock_t sync_lock;   /* for record syscounter & timespec */
	u32 sync_interval;      /* period of sync work */
	struct delayed_work sync_record_work;
	struct syscnt_to_timespec_record record;
};

#define SYSCOUNTER_L32 SOC_SYSCOUNTER_CNTCV_L32_NS_ADDR(0)
#define SYSCOUNTER_H32 SOC_SYSCOUNTER_CNTCV_H32_NS_ADDR(0)
#define IOMCU_TIMER_BASEADDR  (u64)(SOC_ACPU_IOMCU_TIMER_BASE_ADDR)

extern int syscounter_to_timespec64(u64 syscnt, struct timespec64 *ts);
extern u64 bsp_get_syscount(void);

#endif
