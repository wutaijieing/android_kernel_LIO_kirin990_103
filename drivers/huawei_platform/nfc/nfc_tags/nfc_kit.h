/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: the head file for nfc_kit.c
 * Author: lizhengyang lizhengyang
 * Create: 2019-08-05
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __NFC_KIT__
#define __NFC_KIT__

#include <linux/module.h>
#include <linux/version.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/pinctrl/consumer.h>
#include <asm/uaccess.h>
#include <linux/kobject.h>
#include <linux/rtc.h>

#define NFC_LOG_NAME "NFC"
#define NFC_KIT_NAME "nfc_kit"
#define NFC_FAIL (-1)
#define NFC_OK 0

enum nfc_log_level {
	NFC_LL_ERR = 1,
	NFC_LL_MAIN,
	NFC_LL_INFO,
	NFC_LL_DEBUG
};

enum nfc_id_enum {
	NFC_NXP = 0,
	NFC_FM = 1,
	NFC_BUTT
};

struct nfc_kit_data { /* 平台层数据结构 */
	bool is_registered;
	u32 irq_gpio;
	int irq_id;
	u32 nfc_id;
	u32 init_flag;
	struct i2c_client *client;
	struct nfc_device_data *chip_data;
	struct device_node *node;
	struct input_dev *input_dev;
	struct mutex mutex;
	struct work_struct irq0_sch_work;
	struct work_struct irq1_sch_work;
	bool   is_first_irq;
	u32    irq_cnt;
};

struct vendor_info {
	u32 nfc_id;
	int (*chip_probe)(struct nfc_kit_data *pdata);
};

struct nfc_device_ops { /* 芯片层提供给平台层的函数指针,芯片层往平台层注册的时候填充 */
	int (*irq_handler)(void);
	int (*irq0_sch_work)(void);
	int (*irq1_sch_work)(void);
	int (*chip_init)(void);
	int (*ndef_store)(const char *buf, size_t count);
	int (*ndef_show)(char *buf, size_t *count);
};

struct nfc_device_dts {
	u32 vcc_vld;       /* 0: 不支持控制vcc引脚 1：可以控制vcc引脚 */
	u32 vcc_gpio;      /* 控制vcc引脚的gpio编号 */
	u32 hpd_vld;       /* 0: 不支持控制hpd引脚 1：可以控制hpd引脚 */
	u32 hpd_gpio;      /* 控制hpd引脚的gpio编号 */
	u32 sram_enable;   /* 0: sram is not accessable; 1: sram is available */
	u32 event_detect;  /* 0: 不使用ed pin触发中断; 1: 使用ed pin触发中断 */
};

struct nfc_device_data { /* 芯片层数据结构, 后续会继续填充 */
	struct nfc_device_ops *ops;
	struct nfc_device_dts dts;
	/* reserved */
};

int nfc_of_read_u32(const struct device_node *node, const char *prop_name, u32 *value);
int nfc_of_read_string(const struct device_node *node, const char *prop_name, const char **buf);
int nfc_of_read_u8_array(const struct device_node *node, const char *prop_name, u8 *value, size_t sz);
int nfc_chip_register(struct nfc_device_data *chip_data);
int nxp_probe(struct nfc_kit_data *pdata);
int fm11n_probe(struct nfc_kit_data *pdata);
u32 get_nfc_kit_log_level(void);
void nfc_log_level_change(u32 log_level);
struct nfc_kit_data *get_nfc_kit_data(void);

#define NFC_ERR(x...) do { \
	if (get_nfc_kit_log_level() >= NFC_LL_ERR) { \
	    printk(KERN_ERR NFC_LOG_NAME "[E]" x); \
	} \
} while (0)

#define NFC_MAIN(x...) do { \
	if (get_nfc_kit_log_level() >= NFC_LL_MAIN) { \
	    printk(KERN_ERR NFC_LOG_NAME "[M]" x); \
	} \
} while (0)

#define NFC_INFO(x...) do { \
	if (get_nfc_kit_log_level() >= NFC_LL_INFO) { \
	    printk(KERN_ERR NFC_LOG_NAME "[I]" x); \
	} \
} while (0)

#define NFC_DBG(x...) do { \
	if (get_nfc_kit_log_level() >= NFC_LL_DEBUG) { \
	    printk(KERN_ERR NFC_LOG_NAME "[D]" x); \
	} \
} while (0)

#endif /* end of __NFC_KIT__ */
