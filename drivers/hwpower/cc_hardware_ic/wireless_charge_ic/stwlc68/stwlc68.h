/* SPDX-License-Identifier: GPL-2.0 */
/*
 * stwlc68.h
 *
 * stwlc68 macro, addr etc.
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

#ifndef _STWLC68_H_
#define _STWLC68_H_

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
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/hardware_ic/boost_5v.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <chipset_common/hwpower/hardware_ic/wireless_ic_iout.h>
#include <chipset_common/hwpower/wireless_charge/wireless_firmware.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <chipset_common/hwpower/wireless_charge/wireless_acc_types.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <huawei_platform/power/wireless/wireless_charger.h>
#include <huawei_platform/power/wireless/wireless_direct_charger.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_time.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include "stwlc68_chip.h"

struct stwlc68_chip_info {
	u16 chip_id;
	u8 chip_rev;
	u8 cust_id;
	u16 sram_id;
	u16 svn_rev;
	u16 cfg_id;
	u16 pe_id;
	u16 patch_id;
	u8 cut_id;
};

struct stwlc68_global_val {
	bool otp_skip_soak_recovery_flag;
	int ram_rom_status;
	int sram_bad_addr;
	bool sram_chk_complete;
	bool rx_stop_chrg_flag;
	bool irq_abnormal_flag;
	bool tx_open_flag;
	struct hwqi_handle *qi_hdl;
};

struct stwlc68_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	struct mutex mutex_irq;
	struct wakeup_source *wakelock;
	struct stwlc68_global_val g_val;
	u8 rx_fod_5v[STWLC68_RX_FOD_LEN];
	u8 pu_rx_fod_5v[STWLC68_RX_FOD_LEN];
	u8 rx_fod_9v[STWLC68_RX_FOD_LEN];
	u8 rx_fod_9v_cp60[STWLC68_RX_FOD_LEN];
	u8 rx_fod_9v_cp39s[STWLC68_RX_FOD_LEN];
	u8 rx_fod_12v[STWLC68_RX_FOD_LEN];
	u8 rx_fod_15v[STWLC68_RX_FOD_LEN];
	u8 rx_fod_15v_cp39s[STWLC68_RX_FOD_LEN];
	u8 rx_ldo_cfg_5v[STWLC68_LDO_CFG_LEN];
	u8 rx_ldo_cfg_9v[STWLC68_LDO_CFG_LEN];
	u8 rx_ldo_cfg_12v[STWLC68_LDO_CFG_LEN];
	u8 rx_ldo_cfg_sc[STWLC68_LDO_CFG_LEN];
	u8 rx_offset_9v;
	u8 rx_offset_15v;
	unsigned int ic_type;
	bool pu_shell_flag;
	bool need_chk_pu_shell;
	int rx_ss_good_lth;
	int tx_fod_th_5v;
	int gpio_en;
	int gpio_en_valid_val;
	int gpio_sleep_en;
	bool gpio_sleep_en_pending;
	int gpio_int;
	int gpio_pwr_good;
	bool gpio_pwr_good_pending;
	int irq_int;
	int irq_active;
	u16 irq_val;
	u16 ept_type;
	int irq_cnt;
	int support_cp;
	int dev_type;
	int tx_ocp_val;
	int tx_ovp_val;
	int tx_uvp_th;
	u16 tx_ping_interval;
	int tx_max_fop;
	int tx_min_fop;
	int rx_ready_ext_pwr_en;
	u16 chip_id;
	struct delayed_work sram_scan_work;
};

/* stwlc68 i2c */
int stwlc68_read_block(struct stwlc68_dev_info *di, u16 reg, u8 *data, u8 len);
int stwlc68_write_block(struct stwlc68_dev_info *di, u16 reg, u8 *data, u8 data_len);
int stwlc68_4addr_read_block(struct stwlc68_dev_info *di, u32 addr, u8 *data, u8 len);
int stwlc68_4addr_write_block(struct stwlc68_dev_info *di, u32 addr, u8 *data, u8 data_len);
int stwlc68_read_byte(struct stwlc68_dev_info *di, u16 reg, u8 *data);
int stwlc68_write_byte(struct stwlc68_dev_info *di, u16 reg, u8 data);
int stwlc68_read_word(struct stwlc68_dev_info *di, u16 reg, u16 *data);
int stwlc68_write_word(struct stwlc68_dev_info *di, u16 reg, u16 data);
int stwlc68_write_word_mask(struct stwlc68_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data);

/* stwlc68 common */
void stwlc68_enable_irq(struct stwlc68_dev_info *di);
void stwlc68_disable_irq_nosync(struct stwlc68_dev_info *di);
void stwlc68_chip_enable(bool enable, void *dev_data);
bool stwlc68_is_chip_enable(void *dev_data);
void stwlc68_chip_reset(void *dev_data);
int stwlc68_get_mode(struct stwlc68_dev_info *di, u8 *mode);
struct device_node *stwlc68_dts_dev_node(void *dev_data);

/* stwlc68 chip_info */
int stwlc68_get_cfg_id(struct stwlc68_dev_info *di, u16 *cfg_id);
int stwlc68_get_patch_id(struct stwlc68_dev_info *di, u16 *patch_id);
int stwlc68_get_cut_id(struct stwlc68_dev_info *di, u8 *cut_id);
int stwlc68_get_chip_info(struct stwlc68_dev_info *di, struct stwlc68_chip_info *info);
int stwlc68_get_chip_info_str(char *info_str, int len, void *dev_data);
int stwlc68_get_chip_fw_version(u8 *data, int len, void *dev_data);

/* stwlc68 dts */
int stwlc68_parse_dts(struct device_node *np, struct stwlc68_dev_info *di);

/* stwlc68 tx */
int stwlc68_sw2tx(struct stwlc68_dev_info *di);
int stwlc68_tx_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di);
void stwlc68_tx_mode_irq_handler(struct stwlc68_dev_info *di);

/* stwlc68 rx */
bool stwlc68_rx_is_tx_exist(void *dev_data);
void stwlc68_rx_mode_irq_handler(struct stwlc68_dev_info *di);
void stwlc68_rx_shutdown_handler(struct stwlc68_dev_info *di);
void stwlc68_rx_probe_check_tx_exist(struct stwlc68_dev_info *di);
void stwlc68_rx_handle_abnormal_irq(struct stwlc68_dev_info *di);
int stwlc68_rx_chip_init(unsigned int init_type, unsigned int tx_type, void *dev_data);
int stwlc68_rx_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di);

/* stwlc68 fw */
void stwlc68_fw_sram_scan_work(struct work_struct *work);
int stwlc68_fw_sram_update(struct stwlc68_dev_info *di, u32 sram_mode);
int stwlc68_fw_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di);

/* stwlc68 qi */
int stwlc68_qi_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di);

#endif /* _STWLC68_H_ */
