/* SPDX-License-Identifier: GPL-2.0 */
/*
 * stm32g031_fw.h
 *
 * stm32g031 firmware header file
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

#ifndef _STM32G031_FW_H_
#define _STM32G031_FW_H_

/* hw_id reg=0xE0/F0 */
#define STM32G031_FW_HW_ID_REG               0xE0
#define STM32G031_FW_UFCS_HW_ID_REG          0xF0

/* ver_id reg=0xE1/F1 */
#define STM32G031_FW_VER_ID_REG              0xE1
#define STM32G031_FW_UFCS_VER_ID_REG         0xF1

#define STM32G031_FW_PAGE_SIZE               128
#define STM32G031_FW_CMD_SIZE                2
#define STM32G031_FW_ERASE_SIZE              3
#define STM32G031_FW_ADDR_SIZE               5
#define STM32G031_FW_READ_OPTOPN_SIZE        2
#define STM32G031_FW_ACK_COUNT               5
#define STM32G031_FW_RETRY_COUNT             3
#define STM32G031_FW_GPIO_HIGH               1
#define STM32G031_FW_GPIO_LOW                0
#define STM32G031_FW_INT_GPIO_FOR_INT        1
#define STM32G031_FW_INT_GPIO_FOR_WAKEUP     0

/* cmd */
#define STM32G031_FW_OPTION_ADDR             0x1FFF7800
#define STM32G031_FW_BOOTN_ADDR              0x1FFF7803
#define STM32G031_FW_MTP_ADDR                0x08000000
#define STM32G031_FW_MTP_CHECK_ADDR          0x0800FFF0
#define STM32G031_FW_GET_VER_CMD             0x01FE
#define STM32G031_FW_WRITE_CMD               0x32CD
#define STM32G031_FW_ERASE_CMD               0x45BA
#define STM32G031_FW_GO_CMD                  0x21DE
#define STM32G031_FW_READ_UNPROTECT_CMD      0x936C
#define STM32G031_FW_READ_CMD                0x11EE
#define STM32G031_FW_ACK_VAL                 0x79
#define STM32G031_FW_NBOOT_VAL               0xFE
#define STM32G031_FW_MTP_CHECK_FAIL_VAL      0xFF

static const u8 g_stm32g031_option_data[] = {
	0xAA, 0xFE, 0xFF, 0xFE,
};
#define STM32G031_OPTION_SIZE                ARRAY_SIZE(g_stm32g031_option_data)

struct mtp_info {
	u8 ver_id_reg;
	u8 hw_id_reg;
	u8 ver_id;
	const u8 *mtp_data;
	int mtp_size;
};

int stm32g031_fw_set_hw_config(struct stm32g031_device_info *di);
int stm32g031_fw_get_hw_config(struct stm32g031_device_info *di);
int stm32g031_fw_get_ver_id(struct stm32g031_device_info *di);
int stm32g031_fw_update_online_mtp(struct stm32g031_device_info *di,
	u8 *mtp_data, int mtp_size, int ver_id);
int stm32g031_fw_update_mtp_check(struct stm32g031_device_info *di);

#endif /* _STM32G031_FW_H_ */
