/* SPDX-License-Identifier: GPL-2.0 */
/*
 * adapter_protocol_fcp.h
 *
 * fcp protocol driver
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

#ifndef _ADAPTER_PROTOCOL_FCP_H_
#define _ADAPTER_PROTOCOL_FCP_H_

#include <linux/bitops.h>
#include <linux/errno.h>

/* adapter type information register */
#define HWFCP_ADP_TYPE0               0x7e
#define HWFCP_ADP_TYPE0_AB_MASK       (BIT(7) | BIT(5) | BIT(4))
#define HWFCP_ADP_TYPE0_B_MASK        BIT(4)
#define HWFCP_ADP_TYPE0_B_SC_MASK     BIT(3)
#define HWFCP_ADP_TYPE0_B_LVC_MASK    BIT(2)

#define HWFCP_ADP_TYPE1               0x80
#define HWFCP_ADP_TYPE1_B_MASK        BIT(4)

#define HWFCP_COMPILEDVER_HBYTE       0x7c /* xx */
#define HWFCP_COMPILEDVER_LBYTE       0x7d /* yy.zz */
#define HWFCP_COMPILEDVER_XX_MASK     (BIT(7) | BIT(6) | BIT(5) | \
	BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define HWFCP_COMPILEDVER_XX_SHIFT    0
#define HWFCP_COMPILEDVER_YY_MASK     (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define HWFCP_COMPILEDVER_YY_SHIFT    4
#define HWFCP_COMPILEDVER_ZZ_MASK     (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define HWFCP_COMPILEDVER_ZZ_SHIFT    0

#define HWFCP_FWVER_HBYTE             0x8a /* xx */
#define HWFCP_FWVER_LBYTE             0x8b /* yy.zz */
#define HWFCP_FWVER_XX_MASK           (BIT(7) | BIT(6) | BIT(5) | \
	BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define HWFCP_FWVER_XX_SHIFT          0
#define HWFCP_FWVER_YY_MASK           (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define HWFCP_FWVER_YY_SHIFT          4
#define HWFCP_FWVER_ZZ_MASK           (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define HWFCP_FWVER_ZZ_SHIFT          0

#define HWFCP_RESERVED_0X70           0x70
#define HWFCP_RESERVED_0X71           0x71
#define HWFCP_RESERVED_0X72           0x72
#define HWFCP_RESERVED_0X73           0x73
#define HWFCP_RESERVED_0X74           0x74
#define HWFCP_RESERVED_0X75           0x75
#define HWFCP_RESERVED_0X76           0x76
#define HWFCP_RESERVED_0X77           0x77
#define HWFCP_RESERVED_0X78           0x78
#define HWFCP_RESERVED_0X79           0x79
#define HWFCP_RESERVED_0X7A           0x7a
#define HWFCP_RESERVED_0X7B           0x7b
#define HWFCP_RESERVED_0X7F           0x7f

#define HWFCP_DVCTYPE                 0x00

#define HWFCP_SPEC_VER                0x01 /* xx.yy.zz */
#define HWFCP_SPEC_XX_MASK            (BIT(7) | BIT(6) | BIT(5))
#define HWFCP_SPEC_XX_SHIFT           5
#define HWFCP_SPEC_YY_MASK            (BIT(4) | BIT(3) | BIT(2))
#define HWFCP_SPEC_YY_SHIFT           2
#define HWFCP_SPEC_ZZ_MASK            (BIT(1) | BIT(0))
#define HWFCP_SPEC_ZZ_SHIFT           0

#define HWFCP_SCNTL                   0x02

#define HWFCP_SSTAT                   0x03
#define HWFCP_CRCRX_MASK              BIT(1)
#define HWFCP_CRCRX_SHIFT             1
#define HWFCP_PARRX_MASK              BIT(0)
#define HWFCP_PARRX_SHIFT             0

#define HWFCP_CRCRX_ERR               1
#define HWFCP_CRCRX_NOERR             0
#define HWFCP_PARRX_ERR               1
#define HWFCP_PARRX_NOERR             0

#define HWFCP_ID_OUT0                 0x04
#define HWFCP_ID_OUT1                 0x05

/* adapter control information register */
#define HWFCP_CAPABILOTIES            0x20

#define HWFCP_DISCRETE_CAPABILOTIES0  0x21

#define HWFCP_CAPABILOTIES_5V_9V      0x1
#define HWFCP_CAPABILOTIES_5V_9V_12V  0x2

#define HWFCP_MAX_PWR                 0x22
#define HWFCP_MAX_PWR_STEP            500 /* step: 500mw */

#define HWFCP_ADAPTER_STATUS          0x28
#define HWFCP_UVP_MASK                BIT(3)
#define HWFCP_UVP_SHIFT               3
#define HWFCP_OVP_MASK                BIT(2)
#define HWFCP_OVP_SHIFT               2
#define HWFCP_OCP_MASK                BIT(1)
#define HWFCP_OCP_SHIFT               1
#define HWFCP_OTP_MASK                BIT(0)
#define HWFCP_OTP_SHIFT               0

#define HWFCP_VOUT_STATUS             0x29

#define HWFCP_OUTPUT_CONTROL          0x2b

#define HWFCP_VOUT_CONFIG_ENABLE      1
#define HWFCP_IOUT_CONFIG_ENABLE      1

#define HWFCP_VOUT_CONFIG             0x2c
#define HWFCP_VOUT_STEP               100 /* step: 100mv */

#define HWFCP_IOUT_CONFIG             0x2d
#define HWFCP_IOUT_STEP               100 /* step: 100ma */

#define HWFCP_DISCRETE_CAPABILOTIES1  0x2f

/* output voltage configure information register */
#define HWFCP_OUTPUT_V0               0x30
#define HWFCP_OUTPUT_V1               0x31
#define HWFCP_OUTPUT_V2               0x32
#define HWFCP_OUTPUT_V3               0x33
#define HWFCP_OUTPUT_V4               0x34
#define HWFCP_OUTPUT_V5               0x35
#define HWFCP_OUTPUT_V6               0x36
#define HWFCP_OUTPUT_V7               0x37
#define HWFCP_OUTPUT_V8               0x38
#define HWFCP_OUTPUT_V9               0x39
#define HWFCP_OUTPUT_VA               0x3a
#define HWFCP_OUTPUT_VB               0x3b
#define HWFCP_OUTPUT_VC               0x3c
#define HWFCP_OUTPUT_VD               0x3d
#define HWFCP_OUTPUT_VE               0x3e
#define HWFCP_OUTPUT_VF               0x3f
#define hwfcp_output_v_reg(n)         (HWFCP_OUTPUT_V0 + n)
#define HWFCP_OUTPUT_V_STEP           100 /* step: 100mv */

#define HWFCP_DISCRETE_UVP0           0x40
#define HWFCP_DISCRETE_UVP1           0x41
#define HWFCP_DISCRETE_UVP2           0x42
#define HWFCP_DISCRETE_UVP3           0x43
#define HWFCP_DISCRETE_UVP4           0x44
#define HWFCP_DISCRETE_UVP5           0x45
#define HWFCP_DISCRETE_UVP6           0x46
#define HWFCP_DISCRETE_UVP7           0x47
#define HWFCP_DISCRETE_UVP8           0x48
#define HWFCP_DISCRETE_UVP9           0x49
#define HWFCP_DISCRETE_UVPA           0x4a
#define HWFCP_DISCRETE_UVPB           0x4b
#define HWFCP_DISCRETE_UVPC           0x4c
#define HWFCP_DISCRETE_UVPD           0x4d
#define HWFCP_DISCRETE_UVPE           0x4e
#define HWFCP_DISCRETE_UVPF           0x4f
#define hwfcp_discrete_uvp_reg(n)     (HWFCP_DISCRETE_UVP0 + n)
#define HWFCP_DISCRETE_UVP_STEP       100 /* step: 100mv */

/* output current configure information register */
#define HWFCP_OUTPUT_I0               0x50
#define HWFCP_OUTPUT_I1               0x51
#define HWFCP_OUTPUT_I2               0x52
#define HWFCP_OUTPUT_I3               0x53
#define HWFCP_OUTPUT_I4               0x54
#define HWFCP_OUTPUT_I5               0x55
#define HWFCP_OUTPUT_I6               0x56
#define HWFCP_OUTPUT_I7               0x57
#define HWFCP_OUTPUT_I8               0x58
#define HWFCP_OUTPUT_I9               0x59
#define HWFCP_OUTPUT_IA               0x5a
#define HWFCP_OUTPUT_IB               0x5b
#define HWFCP_OUTPUT_IC               0x5c
#define HWFCP_OUTPUT_ID               0x5d
#define HWFCP_OUTPUT_IE               0x5e
#define HWFCP_OUTPUT_IF               0x5f
#define hwfcp_output_i_reg(n)         (HWFCP_OUTPUT_I0 + n)
#define HWFCP_OUTPUT_I_STEP           100 /* step: 100ma */

enum hwfcp_error_code {
	HWFCP_DETECT_OTHER = -1,
	HWFCP_DETECT_SUCC = 0,
	HWFCP_DETECT_FAIL = 1,
};

struct hwfcp_device_info {
	int support_mode; /* adapter support mode */
	unsigned int vid; /* vendor id */
	int volt_cap;
	int max_volt;
	int max_pwr;
	int vid_rd_flag;
	int volt_cap_rd_flag;
	int max_volt_rd_flag;
	int max_pwr_rd_flag;
	int rw_error_flag;
};

struct hwfcp_ops {
	const char *chip_name;
	void *dev_data;
	int (*reg_read)(int reg, int *val, int num, void *dev_data);
	int (*reg_write)(int reg, const int *val, int num, void *dev_data);
	int (*detect_adapter)(void *dev_data);
	int (*soft_reset_master)(void *dev_data);
	int (*soft_reset_slave)(void *dev_data);
	int (*get_master_status)(void *dev_data);
	int (*stop_charging_config)(void *dev_data);
	int (*is_accp_charger_type)(void *dev_data);
	int (*pre_init)(void *dev_data); /* process non protocol flow */
	int (*post_init)(void *dev_data); /* process non protocol flow */
	int (*pre_exit)(void *dev_data); /* process non protocol flow */
	int (*post_exit)(void *dev_data); /* process non protocol flow */
};

struct hwfcp_dev {
	struct device *dev;
	struct hwfcp_device_info info;
	int dev_id;
	struct hwfcp_ops *p_ops;
};

#ifdef CONFIG_ADAPTER_PROTOCOL_FCP
int hwfcp_ops_register(struct hwfcp_ops *ops);
#else
static inline int hwfcp_ops_register(struct hwfcp_ops *ops)
{
	return -EPERM;
}
#endif /* CONFIG_ADAPTER_PROTOCOL_FCP */

#endif /* _ADAPTER_PROTOCOL_FCP_H_ */
