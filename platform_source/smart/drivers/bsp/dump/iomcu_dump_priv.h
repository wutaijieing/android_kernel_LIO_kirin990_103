/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub dump private head file
 * Create: 2021-11-26
 */
#ifndef __IOMCU_DUMP_PRIV_H__
#define __IOMCU_DUMP_PRIV_H__
#include "common/common.h"
#include <linux/types.h>
#include <linux/of_address.h>

#define SENSORHUB_DUMP_BUFF_ADDR DDR_LOG_BUFF_ADDR_AP
#define SENSORHUB_DUMP_BUFF_SIZE DDR_LOG_BUFF_SIZE
#define IS_ENABLE_DUMP 1

#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
union sctrl_sh_boot {
	unsigned int value;
	struct {
		unsigned int boot_step : 8;  /* bit[0-7]: 0~255 boot step pos */
		unsigned int boot_stat : 24; /* bit[8-31] : boot stat */
	} reg;
};

union sctrl_sh_boot get_boot_stat(void);
void reset_boot_stat(void);
#endif

int get_sysctrl_base(void);
int get_nmi_offset(void);
void send_nmi(void);
void clear_nmi(void);

int save_sh_dump_file(uint8_t * sensorhub_dump_buff, union dump_info dump_info);
#endif /* __IOMCU_DUMP_PRIV_H__ */