/* SPDX-License-Identifier: GPL-2.0 */
/*
 * direct_charge_ic.h
 *
 * direct charge ic module
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

#ifndef _DIRECT_CHARGE_IC_H_
#define _DIRECT_CHARGE_IC_H_

#include <linux/errno.h>

#define CHARGE_PATH_MAX_NUM        2
#define CHARGE_IC_MAIN             BIT(0)
#define CHARGE_IC_AUX              BIT(1)
#define CHARGE_MULTI_IC            (CHARGE_IC_MAIN | CHARGE_IC_AUX)
#define CHIP_DEV_NAME_LEN          32

enum dc_ic_type {
	CHARGE_IC_TYPE_MAIN = 0,
	CHARGE_IC_TYPE_AUX,
	CHARGE_IC_TYPE_MAX,
};

#define IC_MODE_MAX_NUM            3
enum dc_working_mode {
	UNDEFINED_MODE = 0x0,
	LVC_MODE = 0x1,
	SC_MODE = 0x2,
	SC4_MODE = 0x4,
};

#define CHARGE_IC_MAX_NUM          3
enum dc_ic_index {
	IC_ONE = 0,
	IC_TWO,
	IC_THREE,
	IC_TOTAL,
};

enum dc_ic_info {
	DC_IC_INFO_IC_INDEX,
	DC_IC_INFO_PATH_INDEX,
	DC_IC_INFO_MAX_IBAT,
	DC_IC_INFO_IBAT_POINT,
	DC_IC_INFO_VBAT_POINT,
	DC_IC_INFO_TOTAL,
};

#define PARA_NAME_LEN_MAX         32
enum dc_ic_mode_info {
	DC_IC_MODE_INDEX = 0,
	DC_IC_MODE_IC_PARA_NAME,
	DC_IC_MODE_TOTAL,
};

struct dc_ic_para {
	unsigned int ic_index;
	int path_index;
	int max_ibat;
	int ibat_sample_point;
	int vbat_sample_point;
};

struct dc_ic_mode_para {
	char ic_mode[PARA_NAME_LEN_MAX];
	char ic_para_index[PARA_NAME_LEN_MAX];
	struct dc_ic_para ic_para[CHARGE_IC_MAX_NUM];
	int ic_para_size;
};

struct dc_ic_dev_info {
	int ic_id;
	int flag;
};

struct dc_ic_dev {
	struct device *dev;
	struct dc_ic_mode_para mode_para[IC_MODE_MAX_NUM];
	int mode_num;
	int use_coul_ibat;
	int use_coul_vbat;
	int vbat_en_gpio_output_value;
	int vbat_en_gpio;
	int use_two_stage;
	bool para_flag;
};

#define SENSE_R_1_MOHM          10 /* 1 mohm */
#define SENSE_R_2_MOHM          20 /* 2 mohm */
#define SENSE_R_2P5_MOHM        25 /* 2.5 mohm */
#define SENSE_R_5_MOHM          50 /* 5 mohm */
#define SENSE_R_10_MOHM         100 /* 10 mohm */

#define IBUS_DEGLITCH_5MS       5
#define UCP_RISE_500MA          500

struct dc_ic_ops {
	void *dev_data;
	const char *dev_name;
	int (*ic_init)(void *dev_data);
	int (*ic_exit)(void *dev_data);
	int (*ic_enable)(int enable, void *dev_data);
	int (*irq_enable)(int enable, void *dev_data);
	int (*ic_adc_enable)(int enable, void *dev_data);
	int (*ic_discharge)(int enable, void *dev_data);
	int (*is_ic_close)(void *dev_data);
	int (*ic_enable_prepare)(void *dev_data);
	int (*config_ic_watchdog)(int time, void *dev_data);
	int (*kick_ic_watchdog)(void *dev_data);
	int (*get_ic_id)(void *dev_data);
	int (*get_ic_status)(void *dev_data);
	int (*set_ic_buck_enable)(int enable, void *dev_data);
	int (*ic_reg_reset_and_init)(void *dev_data);
	int (*set_ic_freq)(int freq, void *dev_data);
	int (*get_ic_freq)(void *dev_data);
	int (*set_ic_thld)(int enable, void *dev_data);
};

struct dc_batinfo_ops {
	void *dev_data;
	int (*init)(void *dev_data);
	int (*exit)(void *dev_data);
	int (*get_bat_btb_voltage)(void *dev_data);
	int (*get_bat_package_voltage)(void *dev_data);
	int (*get_vbus_voltage)(int *vbus, void *dev_data);
	int (*get_bat_current)(int *ibat, void *dev_data);
	int (*get_ic_ibus)(int *ibus, void *dev_data);
	int (*get_ic_temp)(int *temp, void *dev_data);
	int (*get_ic_vout)(int *vout, void *dev_data);
	int (*get_ic_vusb)(int *vusb, void *dev_data);
};

#ifdef CONFIG_DIRECT_CHARGER
int dc_ic_ops_register(int mode, unsigned int index, struct dc_ic_ops *ops);
int dc_batinfo_ops_register(unsigned int index, struct dc_batinfo_ops *ops, int id);
struct dc_ic_ops *dc_ic_get_ic_ops(int mode, unsigned int index);
struct dc_batinfo_ops *dc_ic_get_battinfo_ops(unsigned int index);
int dc_ic_get_ic_index(int mode, unsigned int path, unsigned int *index, int len);
int dc_ic_get_ic_index_for_ibat(int mode, unsigned int path, unsigned int *index, int len);
int dc_ic_get_ic_max_ibat(int mode, unsigned int index, int *ibat);
bool dc_ic_get_ibat_from_coul(int *ibat);
bool dc_ic_get_vbat_from_coul(int *vbat);
bool dc_ic_use_two_stage(void);
struct dc_ic_dev_info *dc_ic_get_ic_info(void);
#else
static inline int dc_ic_ops_register(int mode, unsigned int index, struct dc_ic_ops *ops)
{
	return -EPERM;
}

static inline int dc_batinfo_ops_register(unsigned int index, struct dc_batinfo_ops *ops, int id)
{
	return -EPERM;
}

static inline struct dc_ic_ops *dc_ic_get_ic_ops(int mode, unsigned int index)
{
	return NULL;
}

static inline struct dc_batinfo_ops *dc_ic_get_battinfo_ops(int mode, unsigned int index)
{
	return NULL;
}

static inline int dc_ic_get_ic_index(int mode, unsigned int path, unsigned int *index, int len)
{
	return -EPERM;
}

static inline int dc_ic_get_ic_index_for_ibat(int mode, unsigned int path, unsigned int *index, int len)
{
	return -EPERM;
}

static inline int dc_ic_get_ic_max_ibat(int mode, unsigned int index, int *ibat)
{
	return -EPERM;
}

static inline bool dc_ic_get_ibat_from_coul(int *ibat)
{
	return false;
}

static inline bool dc_ic_get_vbat_from_coul(int *vbat)
{
	return false;
}

static inline bool dc_ic_use_two_stage(void)
{
	return false;
}

static inline struct dc_ic_dev_info *dc_ic_get_ic_info(void)
{
	return NULL;
}
#endif /* CONFIG_DIRECT_CHARGER */

#endif /* _DIRECT_CHARGE_IC_H_ */
