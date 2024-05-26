/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt5785.h
 *
 * mt5785 macro, addr etc.
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#ifndef _MT5785_H_
#define _MT5785_H_

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
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <chipset_common/hwpower/hardware_ic/wireless_ic_fod.h>
#include <chipset_common/hwpower/hardware_ic/wireless_ic_iout.h>
#include <chipset_common/hwpower/wireless_charge/wireless_firmware.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <huawei_platform/power/wireless/wireless_charger.h>
#include <huawei_platform/power/wireless/wireless_direct_charger.h>
#include <huawei_platform/power/wireless/wireless_transmitter.h>
#include <chipset_common/hwpower/wireless_charge/wireless_test_hw.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_time.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include "mt5785_chip.h"

struct mt5785_chip_info {
	u16 chip_id;
	u16 mtp_ver;
};

struct mt5785_fw_info {
	u16 mtp_ver;
	u16 mtp_crc;
	u16 mtp_len;
};

struct mt5785_mtp_check_delay {
	u32 user;
	u32 fac;
};

struct mt5785_global_val {
	bool mtp_latest;
	bool mtp_chk_complete;
	bool irq_abnormal;
	bool rx_stop_chrg;
	bool tx_stop_chrg;
	bool tx_open_flag;
	struct hwqi_handle *qi_hdl;
};

struct mt5785_rx_cm_cfg {
	u8 l_volt;
	u8 h_volt;
	u8 fac_h_volt;
};

struct mt5785_rx_polar_cfg {
	u8 l_volt;
	u8 h_volt;
	u8 fac_h_volt;
};

struct mt5785_rx_mod_cfg {
	struct mt5785_rx_cm_cfg cm;
	struct mt5785_rx_polar_cfg polar;
};

struct mt5785_rx_ldo_cfg {
	u8 l_volt[MT5785_RX_LDO_CFG_LEN]; /* 5V buck */
	u8 m_volt[MT5785_RX_LDO_CFG_LEN]; /* 9V buck */
	u8 sc[MT5785_RX_LDO_CFG_LEN];
};

struct mt5785_tx_init_para {
	u16 ping_interval;
	u16 ping_freq;
};

struct mt5785_tx_fod_para {
	u32 ploss_th0;
	u32 ploss_th1;
	u32 ploss_th2;
	u32 ploss_th3;
	u32 hp_ploss_th0;
	u32 hp_ploss_th1;
	u32 hp_ploss_th2;
	u32 hp_cur_th0;
	u32 hp_cur_th1;
	u32 ploss_cnt;
	u32 q_en;
	u32 q_coil_th;
	u32 q_th;
};

struct mt5785_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct mutex mutex_irq;
	struct wakeup_source *wakelock;
	struct work_struct irq_work;
	struct delayed_work mtp_check_work;
	struct mt5785_fw_info fw_mtp;
	struct mt5785_global_val g_val;
	struct mt5785_rx_mod_cfg rx_mod_cfg;
	struct mt5785_rx_ldo_cfg rx_ldo_cfg;
	struct mt5785_tx_init_para tx_init_para;
	struct mt5785_tx_fod_para tx_fod;
	struct mt5785_mtp_check_delay mtp_check_delay;
	unsigned int ic_type;
	int rx_ss_good_lth;
	int gpio_en;
	int gpio_en_valid_val;
	int gpio_sleep_en;
	int gpio_int;
	int gpio_pwr_good;
	int irq_int;
	int irq_cnt;
	u32 irq_val;
	u32 tx_ept_type;
	bool need_ignore_irq;
	bool irq_active;
	u32 tx_ping_freq;
	int cali_a;
	int cali_b;
	u16 chip_id;
};

/* mt5785 i2c */
int mt5785_read_byte(struct mt5785_dev_info *di, u16 reg, u8 *data);
int mt5785_write_byte(struct mt5785_dev_info *di, u16 reg, u8 data);
int mt5785_read_byte_mask(struct mt5785_dev_info *di, u16 reg, u8 mask, u8 shift, u8 *data);
int mt5785_write_byte_mask(struct mt5785_dev_info *di, u16 reg, u8 mask, u8 shift, u8 data);
int mt5785_read_word(struct mt5785_dev_info *di, u16 reg, u16 *data);
int mt5785_write_word(struct mt5785_dev_info *di, u16 reg, u16 data);
int mt5785_write_word_mask(struct mt5785_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data);
int mt5785_read_dword(struct mt5785_dev_info *di, u16 reg, u32 *data);
int mt5785_write_dword(struct mt5785_dev_info *di, u16 reg, u32 data);
int mt5785_read_dword_mask(struct mt5785_dev_info *di, u16 reg, u32 mask, u32 shift, u32 *data);
int mt5785_write_dword_mask(struct mt5785_dev_info *di, u16 reg, u32 mask, u32 shift, u32 data);
int mt5785_read_block(struct mt5785_dev_info *di, u16 reg, u8 *data, u8 len);
int mt5785_write_block(struct mt5785_dev_info *di, u16 reg, u8 *data, u8 data_len);

/* mt5785 common */
int mt5785_get_chip_info(struct mt5785_dev_info *di, struct mt5785_chip_info *info);
int mt5785_get_chip_info_str(char *info_str, int len, void *dev_data);
void mt5785_enable_irq(struct mt5785_dev_info *di);
void mt5785_disable_irq_nosync(struct mt5785_dev_info *di);
void mt5785_chip_enable(bool enable, void *dev_data);
bool mt5785_is_chip_enable(void *dev_data);
bool mt5785_is_pwr_good(struct mt5785_dev_info *di);
int mt5785_get_mode(struct mt5785_dev_info *di, u32 *mode);

/* mt5785 dts */
int mt5785_parse_dts(struct device_node *np, struct mt5785_dev_info *di);

/* mt5785 rx */
bool mt5785_rx_is_tx_exist(void *dev_data);
void mt5785_rx_probe_check_tx_exist(struct mt5785_dev_info *di);
void mt5785_rx_shutdown_handler(struct mt5785_dev_info *di);
void mt5785_rx_mode_irq_handler(struct mt5785_dev_info *di);
void mt5785_rx_abnormal_irq_handler(struct mt5785_dev_info *di);
int mt5785_rx_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di);

/* mt5785 qi */
int mt5785_qi_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di);

/* mt5785 fw */
void mt5785_fw_mtp_check_work(struct work_struct *work);
int mt5785_fw_sram_update(void *dev_data);
int mt5785_fw_get_mtp_ver(struct mt5785_dev_info *di, u16 *fw);
int mt5785_fw_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di);

/* mt5785 tx */
void mt5785_tx_mode_irq_handler(struct mt5785_dev_info *di);
int mt5785_tx_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di);

/* mt5785 hw_test */
int mt5785_hw_test_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di);

/* mt5785 calibration */
int mt5785_cali_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di);
int mt5785_fw_load_bootloader_cali(struct mt5785_dev_info *di);

#endif /* _MT5785_H_ */
