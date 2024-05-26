/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300_auth.h
 *
 * ns3300 authentication head file
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

#ifndef _NS3300_AUTH_H_
#define _NS3300_AUTH_H_

#include <linux/types.h>
#include "../swi/ns3300_swi.h"

bool ns3300_read_uid(uint8_t *uid, unsigned int uid_len);
bool ns3300_read_cert(uint8_t *ecp_sign, uint8_t *gf2n_publickey);
bool ns3300_read_publickey(uint8_t *publickey, unsigned int pki_len);
bool ns3300_verify_cert_with_publickey(uint8_t *gfp_sign, uint8_t *gf2n_publickey,
	uint8_t *uid, uint8_t *publickey);
bool ns3300_do_authentication(void);

#endif /* _NS3300_AUTH_H_ */
