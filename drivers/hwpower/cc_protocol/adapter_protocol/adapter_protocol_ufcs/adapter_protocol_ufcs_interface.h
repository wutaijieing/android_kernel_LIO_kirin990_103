/* SPDX-License-Identifier: GPL-2.0 */
/*
 * adapter_protocol_ufcs_interface.h
 *
 * ufcs protocol common interface function
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

#ifndef _ADAPTER_PROTOCOL_UFCS_INTERFACE_H_
#define _ADAPTER_PROTOCOL_UFCS_INTERFACE_H_

int hwufcs_check_dev_id(void);
int hwufcs_detect_adapter(void);
int hwufcs_soft_reset_master(void);
int hwufcs_write_msg(u8 *data, u8 len, u8 flag);
int hwufcs_read_msg(u8 *data, u8 len);
int hwufcs_wait_msg_ready(u8 flag);
int hwufcs_get_rx_length(u8 *len);
int hwufcs_end_read_msg(void);
int hwufcs_set_communicating_flag(bool flag);
bool hwufcs_need_check_ack(void);

#endif /* _ADAPTER_PROTOCOL_UFCS_INTERFACE_H_ */
