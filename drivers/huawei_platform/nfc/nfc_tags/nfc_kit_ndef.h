/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: the head file for nfc_kit_ndef.c
 * Author: zhangxiaohui
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
#ifndef __NFC_KIT_NDEF__
#define __NFC_KIT_NDEF__

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

#define HIDOT20_DEV_INFO_LEN 5
#define HIDOT20_TAG_FMT_LEN 2
#define HIDOT20_DEV_TYPE_ID_LEN 3
#define HIDOT20_SAFE_TK_LEN 32
#define HIDOT20_BT_MAC_LEN 6
#define HIDOT20_PASSWD_LEN 32
#define HIDOT20_UDID_LEN 32
#define HIDOT20_SN_CODE_LEN 32   /* 该长度协议没有规定，可根据项目需要修改 */

struct ndef_hidot10_param {
	u8 chip_type;
};

struct ndef_hidot20_param {
	u8 func_and_flags;
	u8 chip_type;
	u8 device_info[HIDOT20_DEV_INFO_LEN];
	u8 tag_format[HIDOT20_TAG_FMT_LEN];
	u8 dev_type_id_v[HIDOT20_DEV_TYPE_ID_LEN];
	u8 safe_token_v[HIDOT20_SAFE_TK_LEN];
	u8 mac_v[HIDOT20_BT_MAC_LEN];
	u8 passwd_v[HIDOT20_PASSWD_LEN];
	u8 udid_v[HIDOT20_UDID_LEN];
	u8 sn_code_l;
	u8 sn_code_v[HIDOT20_SN_CODE_LEN];
};

struct nfc_ndef_param {
	const char *ndef_type;
	struct ndef_hidot10_param hd1;
	struct ndef_hidot20_param hd2;
};

struct nfc_ndef {
	u8 *buf;
	u32 buf_len;
	u32 used_len;
};

struct nfc_ndef_ops { /* 协议层提供给平台层的函数指针,协议层往平台层注册的时候填充 */
	int (*nfc_ndef_create)(struct nfc_ndef_param *param, struct nfc_ndef *msg);
};

struct nfc_ndef_data { /* 协议层数据结构, 后续会继续填充 */
	struct nfc_ndef_param param;
	struct nfc_ndef msg;
	struct nfc_ndef_ops *ops;
	struct mutex mutex;
	/* reserved */
};

struct ndef_info {
	const char* ndef_type;
	int (*nfc_ndef_probe)(struct nfc_ndef_data *pdata);
};

int nfc_huashan_ndef_probe(struct nfc_ndef_data *pdata);
int nfc_ndef_ops_register(struct nfc_ndef_ops *ndef_ops);
void nfc_kit_ndef_free(void);
int nfc_kit_ndef_init(const struct device *dev);

ssize_t nfc_ndef_test_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_safe_token_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_safe_token_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_mac_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_mac_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_passwd_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_passwd_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_udid_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_udid_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_sn_code_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_sn_code_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_ldn_mmi_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_ldn_mmi_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_direct_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count);
ssize_t nfc_ndef_direct_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t nfc_ndef_ldn_at_show(struct device *dev, struct device_attribute *attr, char *buf);

#endif /* __NFC_KIT_NDEF__ */
