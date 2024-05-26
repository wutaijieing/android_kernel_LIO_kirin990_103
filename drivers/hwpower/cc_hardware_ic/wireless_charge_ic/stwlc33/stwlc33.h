/* SPDX-License-Identifier: GPL-2.0 */
/*
 * stwlc33.h
 *
 * stwlc33 macro, addr etc.
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

#ifndef _STWLC33_H_
#define _STWLC33_H_

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
#include <chipset_common/hwpower/wireless_charge/wireless_firmware.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <huawei_platform/power/wireless/wireless_charger.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include "stwlc33_chip.h"

struct stwlc33_global_val {
	struct hwqi_handle *qi_hdl;
};

struct stwlc33_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	struct mutex mutex_irq;
	struct wakeup_source *wakelock;
	struct stwlc33_global_val g_val;
	unsigned int ic_type;
	int tx_fod_th_5v;
	int gpio_en;
	int gpio_en_valid_val;
	int gpio_int;
	int irq_int;
	bool irq_active;
	u16 irq_val;
	u16 ept_type;
	bool support_tx_adjust_vin;
	u16 ping_freq_init_dym;
	u8 tx_ploss_th0;
	u16 chip_id;
};

/* stwlc33 i2c */
int stwlc33_read_block(struct stwlc33_dev_info *di, u16 reg, u8 *data, u8 len);
int stwlc33_write_block(struct stwlc33_dev_info *di, u16 reg, u8 *data, u8 len);
int stwlc33_read_byte(struct stwlc33_dev_info *di, u16 reg, u8 *data);
int stwlc33_write_byte(struct stwlc33_dev_info *di, u16 reg, u8 data);
int stwlc33_read_word(struct stwlc33_dev_info *di, u16 reg, u16 *data);
int stwlc33_write_word(struct stwlc33_dev_info *di, u16 reg, u16 data);
int stwlc33_write_word_mask(struct stwlc33_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data);

/* stwlc33 common */
void stwlc33_enable_irq(struct stwlc33_dev_info *di);
void stwlc33_disable_irq_nosync(struct stwlc33_dev_info *di);
void stwlc33_chip_reset(void *dev_data);
void stwlc33_chip_enable(bool enable, void *dev_data);
int stwlc33_check_nvm_for_power(struct stwlc33_dev_info *di);
int stwlc33_get_mode(struct stwlc33_dev_info *di, u8 *mode);
struct device_node *stwlc33_dts_dev_node(void *dev_data);

/* stwlc33 fw */
int stwlc33_fw_check_fwupdate(struct stwlc33_dev_info *di, u32 fw_sram_mode);
int stwlc33_fw_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di);

/* stwlc33 qi */
int stwlc33_qi_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di);

/* stwlc33 dts */
int stwlc33_parse_dts(struct device_node *np, struct stwlc33_dev_info *di);

/* stwlc33 tx */
int stwlc33_tx_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di);
void stwlc33_tx_mode_irq_handler(struct stwlc33_dev_info *di);
int stwlc33_tx_enable_tx_mode(bool enable, void *dev_data);
bool stwlc33_tx_in_tx_mode(void *dev_data);

#endif /* _STWLC33_H_ */
