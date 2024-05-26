/* SPDX-License-Identifier: GPL-2.0 */
/*
 * hc32l110_fw.h
 *
 * hc32l110 firmware header file
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

#ifndef _HC32L110_FW_H_
#define _HC32L110_FW_H_

#define HC32L110_FW_DEV_ID_REG              0xE0

#define HC32L110_FW_VER_ID_REG              0xE1

#define HC32L110_FW_PAGE_SIZE               128
#define HC32L110_FW_CMD_SIZE                2
#define HC32L110_FW_ERASE_SIZE              3
#define HC32L110_FW_ADDR_SIZE               5
#define HC32L110_FW_ACK_COUNT               10
#define HC32L110_FW_RETRY_COUNT             3
#define HC32L110_FW_ERASE_ACK_COUNT         2

/* cmd */
#define HC32L110_FW_MTP_ADDR                0x00002000
#define HC32L110_FW_GET_VER_CMD             0x01FE
#define HC32L110_FW_WRITE_CMD               0x32CD
#define HC32L110_FW_ERASE_CMD               0x45BA
#define HC32L110_FW_GO_CMD                  0x21DE
#define HC32L110_FW_ACK_VAL                 0x79

int hc32l110_fw_get_dev_id(struct hc32l110_device_info *di);
int hc32l110_fw_get_ver_id(struct hc32l110_device_info *di);
int hc32l110_fw_update_empty_mtp(struct hc32l110_device_info *di);
int hc32l110_fw_update_latest_mtp(struct hc32l110_device_info *di);
int hc32l110_fw_update_online_mtp(struct hc32l110_device_info *di,
	u8 *mtp_data, int mtp_size, int ver_id);

#endif /* _HC32L110_FW_H_ */
