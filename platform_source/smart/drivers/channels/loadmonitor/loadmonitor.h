/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: Contexthub loadmonitor driver.
 * Create: 2017-06-13
 */
#ifndef __CONTEXTHUB_LOADMONITOR_H__
#define __CONTEXTHUB_LOADMONITOR_H__
#include <linux/types.h>

#define MAX_SIG_CNT_PER_IP                    32
enum {
	DATA_INDEX_IDLE = 0,
	DATA_INDEX_BUSY_NORMAL,
	DATA_INDEX_BUSY_LOW,
	DATA_INDEX_MAX,
};

struct loadmonitor_sig {
	uint64_t                 count[DATA_INDEX_MAX];
	uint32_t                 samples;
	uint32_t                 reserved;
};

struct loadmonitor_sigs {
	struct loadmonitor_sig   sig[MAX_SIG_CNT_PER_IP];
};

struct ao_loadmonitor_sig {
	struct loadmonitor_sigs  sigs;
	uint32_t                 enable;
	uint32_t                 interval;
};

/* EXTERNAL FUNCTIONS */
#if defined(CONFIG_CONTEXTHUB_LOADMONITOR)
/*
 * 发送IPC通知contexthub使能loadmonitor
 */
int ao_loadmonitor_enable(unsigned int delay_value, unsigned int freq);
/*
 * 发送IPC通知contexthub关闭loadmonitor
 */
int ao_loadmonitor_disable(void);
/*
 * 发送IPC通知contexthub读取loadmonitor数据
 */
int32_t _ao_loadmonitor_read(void *data, uint32_t len);
int get_iom3_state(void);

#else
inline int ao_loadmonitor_enable(unsigned int delay_value, unsigned int freq)
{
	(void)delay_value;
	(void)freq;

	pr_err("ao_loadmonitor_enable invalid;\n");
	return -EINVAL;
}

inline int ao_loadmonitor_disable(void)
{
	pr_err("ao_loadmonitor_disable invalid;\n");
	return -EINVAL;
}

inline int32_t _ao_loadmonitor_read(void *data, uint32_t len)
{
	(void)data;
	(void)len;

	pr_err("_ao_loadmonitor_read invalid;\n");
	return -EINVAL;
}
#endif
#endif

