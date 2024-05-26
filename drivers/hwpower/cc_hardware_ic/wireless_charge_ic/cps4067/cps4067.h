/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cps4067.h
 *
 * cps4067 macro, addr etc.
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

#ifndef _CPS4067_H_
#define _CPS4067_H_

#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_firmware.h>
#include <chipset_common/hwpower/wireless_charge/wireless_test_hw.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_client.h>
#include <chipset_common/hwpower/protocol/wireless_protocol_qi.h>
#include <chipset_common/hwpower/hardware_ic/wireless_ic_iout.h>
#include <chipset_common/hwpower/hardware_ic/wireless_ic_fod.h>
#include <chipset_common/hwpower/hardware_ic/boost_5v.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include "cps4067_chip.h"

#define CPS4067_PINCTRL_LEN                   2

/* for coil test */
#define CPS4067_COIL_TEST_PING_INTERVAL       0
#define CPS4067_COIL_TEST_PING_FREQ           115
/* for battery heating */
#define CPS4067_BAT_HEATING_PING_INTERVAL     0
#define CPS4067_BAT_HEATING_PING_FREQ         100

struct cps4067_chip_info {
	u16 chip_id;
	u16 mtp_ver;
};

struct cps4067_fw_info {
	u16 mtp_ver;
	u16 mtp_crc;
};

struct cps4067_global_val {
	bool mtp_latest;
	bool mtp_chk_complete;
	bool irq_abnormal;
	bool rx_stop_chrg;
	bool tx_stop_chrg;
	bool tx_open_flag;
	struct hwqi_handle *qi_hdl;
};

struct cps4067_tx_fod_para {
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

struct cps4067_rx_cm_cfg {
	u8 l_volt;
	u8 h_volt;
	u8 fac_h_volt;
};

struct cps4067_rx_polar_cfg {
	u8 l_volt;
	u8 h_volt;
	u8 fac_h_volt;
};

struct cps4067_rx_mod_cfg {
	struct cps4067_rx_cm_cfg cm;
	struct cps4067_rx_polar_cfg polar;
};

struct cps4067_rx_ldo_cfg {
	u8 l_volt[CPS4067_RX_LDO_CFG_LEN]; /* 5V buck */
	u8 m_volt[CPS4067_RX_LDO_CFG_LEN]; /* 9V buck */
	u8 sc[CPS4067_RX_LDO_CFG_LEN];
};

struct cps4067_tx_init_para {
	u16 ping_interval;
	u16 ping_freq;
};

struct cps4067_mtp_check_delay {
	u32 user;
	u32 fac;
};

struct cps4067_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct mutex mutex_irq;
	struct wakeup_source *wakelock;
	struct work_struct irq_work;
	struct delayed_work mtp_check_work;
	struct cps4067_fw_info fw_mtp;
	struct cps4067_global_val g_val;
	struct cps4067_rx_mod_cfg rx_mod_cfg;
	struct cps4067_rx_ldo_cfg rx_ldo_cfg;
	struct cps4067_tx_init_para tx_init_para;
	struct cps4067_tx_fod_para tx_fod;
	struct cps4067_mtp_check_delay mtp_check_delay;
	unsigned int ic_type;
	int rx_ss_good_lth;
	int gpio_en;
	int gpio_en_valid_val;
	int gpio_sleep_en;
	int gpio_int;
	int gpio_pwr_good;
	int irq_int;
	bool irq_active;
	int irq_cnt;
	u32 irq_val;
	u16 tx_ept_type;
	int tx_full_bri_ith;
	bool need_ignore_irq;
	u32 tx_ping_freq;
	u16 chip_id;
};

/* cps4067 i2c */
int cps4067_read_byte(struct cps4067_dev_info *di, u16 reg, u8 *data);
int cps4067_write_byte(struct cps4067_dev_info *di, u16 reg, u8 data);
int cps4067_read_byte_mask(struct cps4067_dev_info *di, u16 reg, u8 mask, u8 shift, u8 *data);
int cps4067_write_byte_mask(struct cps4067_dev_info *di, u16 reg, u8 mask, u8 shift, u8 data);
int cps4067_read_word(struct cps4067_dev_info *di, u16 reg, u16 *data);
int cps4067_write_word(struct cps4067_dev_info *di, u16 reg, u16 data);
int cps4067_write_word_mask(struct cps4067_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data);
int cps4067_read_dword(struct cps4067_dev_info *di, u16 reg, u32 *data);
int cps4067_write_dword(struct cps4067_dev_info *di, u16 reg, u32 data);
int cps4067_read_dword_mask(struct cps4067_dev_info *di, u16 reg, u32 mask, u32 shift, u32 *data);
int cps4067_write_dword_mask(struct cps4067_dev_info *di, u16 reg, u32 mask, u32 shift, u32 data);
int cps4067_read_block(struct cps4067_dev_info *di, u16 reg, u8 *data, u8 len);
int cps4067_write_block(struct cps4067_dev_info *di, u16 reg, u8 *data, u8 data_len);
int cps4067_hw_read_block(struct cps4067_dev_info *di, u32 addr, u8 *data, u8 len);
int cps4067_hw_write_block(struct cps4067_dev_info *di, u32 addr, u8 *data, u8 data_len);

/* cps4067 common */
int cps4067_get_chip_info(struct cps4067_dev_info *di, struct cps4067_chip_info *info);
int cps4067_get_chip_info_str(char *info_str, int len, void *dev_data);
void cps4067_enable_irq(struct cps4067_dev_info *di);
void cps4067_disable_irq_nosync(struct cps4067_dev_info *di);
void cps4067_chip_enable(bool enable, void *dev_data);
bool cps4067_is_chip_enable(void *dev_data);
struct device_node *cps4067_dts_dev_node(void *dev_data);
bool cps4067_is_pwr_good(struct cps4067_dev_info *di);
int cps4067_get_mode(struct cps4067_dev_info *di, u8 *mode);

/* cps4067 dts */
int cps4067_parse_dts(struct device_node *np, struct cps4067_dev_info *di);

/* cps4067 rx */
bool cps4067_rx_is_tx_exist(void *dev_data);
void cps4067_rx_probe_check_tx_exist(struct cps4067_dev_info *di);
void cps4067_rx_shutdown_handler(struct cps4067_dev_info *di);
void cps4067_rx_mode_irq_handler(struct cps4067_dev_info *di);
void cps4067_rx_abnormal_irq_handler(struct cps4067_dev_info *di);
int cps4067_rx_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di);

/* cps4067 tx */
void cps4067_tx_mode_irq_handler(struct cps4067_dev_info *di);
int cps4067_tx_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di);

/* cps4067 fw */
int cps4067_fw_sram_update(void *dev_data);
int cps4067_fw_get_mtp_ver(struct cps4067_dev_info *di, u16 *fw);
void cps4067_fw_mtp_check_work(struct work_struct *work);
int cps4067_fw_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di);

/* cps4067 qi */
int cps4067_qi_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di);

/* cps4067 hw_test */
int cps4067_hw_test_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di);

#endif /* _CPS4067_H_ */
