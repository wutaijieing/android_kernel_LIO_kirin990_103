/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt5727.h
 *
 * mt5727 macro, addr etc.
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _MT5727_H_
#define _MT5727_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/hardware_ic/boost_5v.h>
#include <chipset_common/hwpower/wireless_charge/wireless_firmware.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_time.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/protocol/wireless_protocol_qi.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>

#include "mt5727_chip.h"

#define MT5727_PINCTRL_LEN                   2

struct mt5727_chip_info {
	u16 chip_id;
	u8 cust_id;
	u8 hw_id;
	u16 minor_ver;
	u16 major_ver;
};

struct mt5727_global_val {
	bool mtp_chk_complete;
	bool mtp_latest;
	struct hwqi_handle *qi_hdl;
};

struct mt5727_tx_init_para {
	u16 ping_interval;
	u16 ping_freq;
};

struct mt5727_tx_fod_para {
	u16 ploss_th0;
	u16 ploss_cnt;
};

struct mt5727_tx_fop_para {
	u16 tx_max_fop;
	u16 tx_min_fop;
};

struct mt5727_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	struct delayed_work mtp_check_work;
	struct mutex mutex_irq;
	struct wakeup_source *wakelock;
	struct mt5727_global_val g_val;
	struct mt5727_tx_init_para tx_init_para;
	struct mt5727_tx_fod_para tx_fod;
	struct mt5727_tx_fop_para tx_fop;
	unsigned int ic_type;
	bool irq_active;
	int gpio_en;
	int gpio_en_valid_val;
	int gpio_int;
	int irq_int;
	u32 irq_val;
	int irq_cnt;
	u32 ept_type;
	int tx_pocp_th;
	u16 chip_id;
};

/* mt5727 common */
int mt5727_read_byte(struct mt5727_dev_info *di, u16 reg, u8 *data);
int mt5727_write_byte(struct mt5727_dev_info *di, u16 reg, u8 data);
int mt5727_read_word(struct mt5727_dev_info *di, u16 reg, u16 *data);
int mt5727_write_word(struct mt5727_dev_info *di, u16 reg, u16 data);
int mt5727_read_dword(struct mt5727_dev_info *di, u16 reg, u32 *data);
int mt5727_write_dword(struct mt5727_dev_info *di, u16 reg, u32 data);
int mt5727_write_dword_mask(struct mt5727_dev_info *di, u16 reg, u32 mask, u32 shift, u32 data);
int mt5727_read_block(struct mt5727_dev_info *di, u16 reg, u8 *data, u8 len);
int mt5727_write_block(struct mt5727_dev_info *di, u16 reg, u8 *data, u8 data_len);
void mt5727_chip_reset(void *dev_data);
void mt5727_chip_enable(bool enable, void *dev_data);
bool mt5727_is_chip_enable(void *dev_data);
void mt5727_enable_irq(struct mt5727_dev_info *di);
void mt5727_disable_irq_nosync(struct mt5727_dev_info *di);
struct device_node *mt5727_dts_dev_node(void *dev_data);
int mt5727_get_mode(struct mt5727_dev_info *di, u16 *mode);

/* mt5727 chip_info */
int mt5727_get_chip_id(struct mt5727_dev_info *di, u16 *chip_id);
int mt5727_get_chip_fw_version(u8 *data, int len, void *dev_data);

/* mt5727 tx */
int mt5727_tx_ops_register(struct wltrx_ic_ops *ops, struct mt5727_dev_info *di);
void mt5727_tx_mode_irq_handler(struct mt5727_dev_info *di);

/* mt5727 fw */
int mt5727_fw_ops_register(struct wltrx_ic_ops *ops, struct mt5727_dev_info *di);
void mt5727_fw_mtp_check_work(struct work_struct *work);
int mt5727_fw_sram_update(void *dev_data);

/* mt5727 qi */
int mt5727_qi_ops_register(struct wltrx_ic_ops *ops, struct mt5727_dev_info *di);

/* mt5727 dts */
int mt5727_parse_dts(struct device_node *np, struct mt5727_dev_info *di);

#endif /* _MT5727_H_ */
