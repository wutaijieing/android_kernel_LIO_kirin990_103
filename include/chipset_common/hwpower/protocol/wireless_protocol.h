/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_protocol.h
 *
 * wireless protocol driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _WIRELESS_PROTOCOL_H_
#define _WIRELESS_PROTOCOL_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>

#define WIRELESS_LOG_BUF_SIZE     128
#define WIRELESS_TX_FWVER_LEN     32
#define WIRELESS_TX_BD_STR_LEN    32 /* 32bit bigdata */

enum wireless_protocol_type {
	WIRELESS_PROTOCOL_BEGIN = 0,
	WIRELESS_PROTOCOL_QI = WIRELESS_PROTOCOL_BEGIN, /* qi */
	WIRELESS_PROTOCOL_A4WP, /* a4wp */
	WIRELESS_PROTOCOL_END,
};

enum wireless_protocol_read_flag {
	WIRELESS_NOT_RD_FLAG = 0,
	WIRELESS_HAS_RD_FLAG = 2,
};

enum wireless_protocol_byte_num {
	WIRELESS_BYTE_ONE = 1,
	WIRELESS_BYTE_TWO = 2,
	WIRELESS_BYTE_FOUR = 4,
	WIRELESS_BYTE_EIGHT = 8,
	WIRELESS_BYTE_SIXTEEN = 16,
};

enum wireless_protocol_retry_num {
	WIRELESS_RETRY_ONE = 1,
	WIRELESS_RETRY_TWO = 2,
	WIRELESS_RETRY_THREE = 3,
	WIRELESS_RETRY_FOUR = 4,
	WIRELESS_RETRY_FIVE = 5,
	WIRELESS_RETRY_SIX = 6,
};

enum wireless_protocol_chip_vendor {
	WIRELESS_CHIP_IDT,
	WIRELESS_CHIP_ST,
};

enum wireless_protocol_device_id {
	WIRELESS_DEVICE_ID_BEGIN = 0,
	WIRELESS_DEVICE_ID_IDTP9221 = WIRELESS_DEVICE_ID_BEGIN,
	WIRELESS_DEVICE_ID_STWLC68,
	WIRELESS_DEVICE_ID_IDTP9415,
	WIRELESS_DEVICE_ID_NU1619,
	WIRELESS_DEVICE_ID_STWLC88,
	WIRELESS_DEVICE_ID_CPS7181,
	WIRELESS_DEVICE_ID_CPS4057,
	WIRELESS_DEVICE_ID_MT5735,
	WIRELESS_DEVICE_ID_CPS4029,
	WIRELESS_DEVICE_ID_MT5727,
	WIRELESS_DEVICE_ID_STWLC33,
	WIRELESS_DEVICE_ID_CPS4067,
	WIRELESS_DEVICE_ID_MT5785,
	WIRELESS_DEVICE_ID_END,
};

struct wireless_protocol_device_data {
	unsigned int id;
	const char *name;
};

struct wlprot_acc_fop_range {
	int base_min;
	int base_max;
	int ext1_min;
	int ext1_max;
	int ext2_min;
	int ext2_max;
};

struct wlprot_acc_cap {
	u8 adp_type;
	u8 ext_type;
	int vmax;
	int imax;
	bool can_boost;
	bool cable_ok;
	bool no_need_cert;
	bool support_scp;
	bool support_12v;
	bool support_extra_cap;
	bool support_fan;
	bool support_tec;
	bool support_fod_status;
	bool support_get_ept;
	bool support_fop_range;
	struct wlprot_acc_fop_range fop_range;
};

struct wireless_protocol_tx_alarm {
	u8 src;
	u32 plim;
	u32 vlim;
	u8 reserved;
};

enum wireless_protocol_tx_alarm_src {
	TX_ALARM_SRC_TEMP = 0,
	TX_ALARM_SRC_PLIM,
	TX_ALARM_SRC_FAN,
	TX_ALARM_SRC_FOD,
	TX_ALARM_SRC_TIME,
	TX_ALARM_SRC_QVAL,
};

enum wireless_evt {
	RX_NO_EVENT = 0,
	RX_STATR_CHARGING
};

enum wireless_device_type {
	WIRELESS_DEVICE_UNKNOWN = 0,
	WIRELESS_DEVICE_PHONE,
	WIRELESS_DEVICE_EAR,
	WIRELESS_DEVICE_PAD,
};

struct wireless_device_info {
	char tx_fwver[WIRELESS_TX_FWVER_LEN];
	char tx_bd_info[WIRELESS_TX_BD_STR_LEN];
	int tx_id;
	int tx_type;
	int tx_adp_type;
};

struct wireless_protocol_ops {
	const char *type_name;
	int (*send_rx_evt)(unsigned int ic_type, u8 rx_evt);
	int (*send_rx_vout)(unsigned int ic_type, int rx_vout);
	int (*send_rx_iout)(unsigned int ic_type, int rx_iout);
	int (*send_rx_boost_succ)(unsigned int ic_type);
	int (*send_rx_ready)(unsigned int ic_type);
	int (*send_tx_capability)(unsigned int ic_type, u8 *cap, int len);
	int (*send_tx_alarm)(unsigned int ic_type, u8 *alarm, int len);
	int (*send_tx_fw_version)(unsigned int ic_type, u8 *fw, int len);
	int (*send_lightstrap_ctrl_msg)(unsigned int ic_type, u8 *msg, int len);
	int (*send_cert_confirm)(unsigned int ic_type, bool succ_flag);
	int (*send_charge_state)(unsigned int ic_type, u8 state);
	int (*send_fod_status)(unsigned int ic_type, int status);
	int (*send_charge_mode)(unsigned int ic_type, u8 mode);
	int (*set_fan_speed_limit)(unsigned int ic_type, u8 limit);
	int (*set_rpp_format)(unsigned int ic_type, u8 pmax);
	int (*get_ept_type)(unsigned int ic_type, u16 *type);
	int (*get_rpp_format)(unsigned int ic_type, u8 *format);
	int (*get_tx_fw_version)(unsigned int ic_type, char *ver, int len);
	int (*get_tx_bigdata_info)(unsigned int ic_type, char *info, int len);
	int (*get_tx_id)(unsigned int ic_type, int *id);
	int (*get_tx_type)(unsigned int ic_type, int *type);
	int (*get_tx_adapter_type)(unsigned int ic_type, int *type);
	int (*get_tx_capability)(unsigned int ic_type, struct wlprot_acc_cap *cap);
	int (*get_tx_max_power)(unsigned int ic_type, int *power);
	int (*get_tx_adapter_capability)(unsigned int ic_type, int *vout, int *iout);
	int (*get_tx_cable_type)(unsigned int ic_type, int *type, int *iout);
	int (*get_tx_fop_range)(unsigned int ic_type, struct wlprot_acc_fop_range *fop);
	int (*auth_encrypt_start)(unsigned int ic_type, int key, u8 *random, int r_size, u8 *hash, int h_size);
	int (*fix_tx_fop)(unsigned int ic_type, int fop);
	int (*unfix_tx_fop)(unsigned int ic_type);
	int (*reset_dev_info)(unsigned int ic_type);
	/* for wireless accessory */
	int (*acc_set_tx_dev_state)(unsigned int ic_type, u8 state);
	int (*acc_set_tx_dev_info_cnt)(unsigned int ic_type, u8 cnt);
	int (*acc_get_tx_dev_state)(unsigned int ic_type, u8 *state);
	int (*acc_get_tx_dev_no)(unsigned int ic_type, u8 *no);
	int (*acc_get_tx_dev_mac)(unsigned int ic_type, u8 *mac, int len);
	int (*acc_get_tx_dev_model_id)(unsigned int ic_type, u8 *id, int len);
	int (*acc_get_tx_dev_submodel_id)(unsigned int ic_type, u8 *id);
	int (*acc_get_tx_dev_version)(unsigned int ic_type, u8 *ver);
	int (*acc_get_tx_dev_business)(unsigned int ic_type, u8 *bus);
	int (*acc_get_tx_dev_info_cnt)(unsigned int ic_type, u8 *cnt);
	/* for wireless tx auth */
	int (*get_rx_random)(unsigned int ic_type, u8 *random, int len);
	int (*set_tx_cipherkey)(unsigned int ic_type, u8 *cipherkey, int len);
};

struct wireless_protocol_dev {
	struct wireless_device_info info;
	unsigned int total_ops;
	struct wireless_protocol_ops *p_ops[WIRELESS_PROTOCOL_END];
};

#ifdef CONFIG_WIRELESS_PROTOCOL
struct wireless_protocol_dev *wireless_get_protocol_dev(void);
int wireless_protocol_ops_register(struct wireless_protocol_ops *ops);
int wireless_send_charge_event(unsigned int ic_type, int prot, u8 evt);
int wireless_send_rx_vout(unsigned int ic_type, int prot, int rx_vout);
int wireless_send_rx_iout(unsigned int ic_type, int prot, int rx_iout);
int wireless_send_rx_boost_succ(unsigned int ic_type, int prot);
int wireless_send_rx_ready(unsigned int ic_type, int prot);
int wireless_send_tx_capability(unsigned int ic_type, int prot, u8 *cap, int len);
int wireless_send_tx_alarm(unsigned int ic_type, int prot, u8 *alarm, int len);
int wireless_send_tx_fw_version(unsigned int ic_type, int prot, u8 *fw, int len);
int wireless_send_lightstrap_ctrl_msg(unsigned int ic_type, int prot, u8 *msg, int len);
int wireless_send_cert_confirm(unsigned int ic_type, int prot, bool succ_flag);
int wireless_send_charge_state(unsigned int ic_type, int prot, u8 state);
int wireless_send_fod_status(unsigned int ic_type, int prot, int status);
int wireless_send_charge_mode(unsigned int ic_type, int prot, u8 mode);
int wireless_set_fan_speed_limit(unsigned int ic_type, int prot, u8 limit);
int wireless_set_rpp_format(unsigned int ic_type, int prot, u8 pmax);
int wireless_get_ept_type(unsigned int ic_type, int prot, u16 *type);
int wireless_get_rpp_format(unsigned int ic_type, int prot, u8 *format);
char *wireless_get_tx_fw_version(unsigned int ic_type, int prot);
int wireless_get_tx_bigdata_info(unsigned int ic_type, int prot);
int wireless_get_tx_id(unsigned int ic_type, int prot);
int wireless_get_tx_type(unsigned int ic_type, int prot);
int wireless_get_tx_adapter_type(unsigned int ic_type, int prot);
int wireless_get_tx_capability(unsigned int ic_type, int prot, struct wlprot_acc_cap *cap);
int wireless_get_tx_max_power(unsigned int ic_type, int prot, int *power);
int wireless_get_tx_adapter_capability(unsigned int ic_type, int prot, int *vout, int *iout);
int wireless_get_tx_cable_type(unsigned int ic_type, int prot, int *type, int *iout);
int wireless_get_tx_fop_range(unsigned int ic_type, int prot, struct wlprot_acc_fop_range *fop);
int wireless_auth_encrypt_start(unsigned int ic_type, int prot,
	int key, u8 *random, int r_size, u8 *hash, int h_size);
int wireless_fix_tx_fop(unsigned int ic_type, int prot, int fop);
int wireless_unfix_tx_fop(unsigned int ic_type, int prot);
int wireless_reset_dev_info(unsigned int ic_type, int prot);
/* for wireless accessory */
int wireless_acc_set_tx_dev_state(unsigned int ic_type, int prot, u8 state);
int wireless_acc_set_tx_dev_info_cnt(unsigned int ic_type, int prot, u8 cnt);
int wireless_acc_get_tx_dev_state(unsigned int ic_type, int prot, u8 *state);
int wireless_acc_get_tx_dev_no(unsigned int ic_type, int prot, u8 *no);
int wireless_acc_get_tx_dev_mac(unsigned int ic_type, int prot, u8 *mac, int len);
int wireless_acc_get_tx_dev_model_id(unsigned int ic_type, int prot, u8 *id, int len);
int wireless_acc_get_tx_dev_submodel_id(unsigned int ic_type, int prot, u8 *id);
int wireless_acc_get_tx_dev_version(unsigned int ic_type, int prot, u8 *ver);
int wireless_acc_get_tx_dev_business(unsigned int ic_type, int prot, u8 *bus);
int wireless_acc_get_tx_dev_info_cnt(unsigned int ic_type, int prot, u8 *cnt);
/* for wireless tx auth */
int wireless_get_rx_random(unsigned int ic_type, int prot, u8 *random, int len);
int wireless_set_tx_cipherkey(unsigned int ic_type, int prot, u8 *cipherkey, int len);
#else
static inline struct wireless_protocol_dev *wireless_get_protocol_dev(void)
{
	return NULL;
}

static inline int wireless_protocol_ops_register(struct wireless_protocol_ops *ops)
{
	return -ENOTSUPP;
}

static inline int wireless_send_charge_event(unsigned int ic_type, int prot, u8 evt)
{
	return -ENOTSUPP;
}

static inline int wireless_send_rx_vout(unsigned int ic_type, int prot, int rx_vout)
{
	return -ENOTSUPP;
}

static inline int wireless_send_rx_iout(unsigned int ic_type, int prot, int rx_iout)
{
	return -ENOTSUPP;
}

static inline int wireless_send_rx_boost_succ(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_send_rx_ready(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_send_tx_capability(unsigned int ic_type, int prot, u8 *cap, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_send_tx_alarm(unsigned int ic_type, int prot, u8 *alarm, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_send_tx_fw_version(unsigned int ic_type, int prot, u8 *fw, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_send_lightstrap_ctrl_msg(unsigned int ic_type, int prot, u8 *msg, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_send_cert_confirm(unsigned int ic_type, int prot, bool succ_flag)
{
	return -ENOTSUPP;
}

static inline int wireless_send_charge_state(unsigned int ic_type, int prot, u8 state)
{
	return -ENOTSUPP;
}

static inline int wireless_send_fod_status(unsigned int ic_type, int prot, int status)
{
	return -ENOTSUPP;
}

static inline int wireless_send_charge_mode(unsigned int ic_type, int prot, u8 mode)
{
	return -ENOTSUPP;
}

static inline int wireless_set_fan_speed_limit(unsigned int ic_type, int prot, u8 limit)
{
	return -ENOTSUPP;
}

static inline int wireless_set_rpp_format(unsigned int ic_type, int prot, u8 pmax)
{
	return -ENOTSUPP;
}

static inline int wireless_get_ept_type(unsigned int ic_type, int prot, u16 *type)
{
	return -ENOTSUPP;
}

static inline int wireless_get_rpp_format(unsigned int ic_type, int prot, u8 *format)
{
	return -ENOTSUPP;
}

static inline char *wireless_get_tx_fw_version(unsigned int ic_type, int prot)
{
	return NULL;
}

static inline int wireless_get_tx_bigdata_info(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_id(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_type(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_adapter_type(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_capability(unsigned int ic_type, int prot, struct wlprot_acc_cap *cap)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_max_power(unsigned int ic_type, int prot, int *power)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_adapter_capability(unsigned int ic_type, int prot, int *vout,
	int *iout)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_cable_type(unsigned int ic_type, int prot, int *type, int *iout)
{
	return -ENOTSUPP;
}

static inline int wireless_get_tx_fop_range(unsigned int ic_type, int prot,
	struct wlprot_acc_fop_range *fop)
{
	return -ENOTSUPP;
}

static inline int wireless_auth_encrypt_start(unsigned int ic_type, int prot,
	int key, u8 *random, int r_size, u8 *hash, int h_size)
{
	return -ENOTSUPP;
}

static inline int wireless_fix_tx_fop(unsigned int ic_type, int prot, int fop)
{
	return -ENOTSUPP;
}

static inline int wireless_unfix_tx_fop(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_reset_dev_info(unsigned int ic_type, int prot)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_set_tx_dev_state(unsigned int ic_type, int prot, u8 state)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_set_tx_dev_info_cnt(unsigned int ic_type, int prot, u8 cnt)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_state(unsigned int ic_type, int prot, u8 *state)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_no(unsigned int ic_type, int prot, u8 *no)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_mac(unsigned int ic_type, int prot, u8 *mac, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_model_id(unsigned int ic_type, int prot, u8 *id, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_submodel_id(unsigned int ic_type, int prot, u8 *id)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_version(unsigned int ic_type, int prot, u8 *ver)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_business(unsigned int ic_type, int prot, u8 *bus)
{
	return -ENOTSUPP;
}

static inline int wireless_acc_get_tx_dev_info_cnt(unsigned int ic_type, int prot, u8 *cnt)
{
	return -ENOTSUPP;
}

static inline int wireless_get_rx_random(unsigned int ic_type, int prot, u8 *random, int len)
{
	return -ENOTSUPP;
}

static inline int wireless_set_tx_cipherkey(unsigned int ic_type, int prot, u8 *cipherkey, int len)
{
	return -ENOTSUPP;
}
#endif /* CONFIG_WIRELESS_PROTOCOL */

#endif /* _WIRELESS_PROTOCOL_H_ */
