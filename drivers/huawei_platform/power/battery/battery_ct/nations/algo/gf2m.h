/* SPDX-License-Identifier: GPL-2.0 */
/*
 * GF2m.h
 *
 * GF2m head file
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
#ifndef _GF2M_H_
#define _GF2M_H_
#include <linux/types.h>

#include "common.h"

#define GF2M_163         163
#define GF2M_BYTE_SIZE   8
#define GF2M_WORD_SIZE   32
#define GF2M_NUM_BIT     ((GF2M_163) + 1)
#define GF2M_NUM_BYTE    (((GF2M_NUM_BIT + GF2M_BYTE_SIZE) - 1) / GF2M_BYTE_SIZE)
#define GF2M_NUM_WORD    (((GF2M_NUM_BIT + GF2M_WORD_SIZE) - 1) / GF2M_WORD_SIZE)
#define GF2M_MAX_LONG    ((GF2M_NUM_WORD) + 1)
#define HASH_MAX_LONG    ((512 + 31) >> 5)

struct gf2m_bigint {
	uint32_t d[GF2M_MAX_LONG];
};

struct ec_point {
	struct gf2m_bigint x;
	struct gf2m_bigint y;
};

struct ec_curve {
	struct gf2m_bigint x;
	struct gf2m_bigint y;
	struct gf2m_bigint z;
};

int ec2m_163_dsa_sign(unsigned char *e, unsigned int e_byte_len,
	unsigned char *pri_key, unsigned char *r, unsigned char *s);
int ec2m_163_dsa_verify(unsigned char *e1, unsigned int e_byte_len,
	unsigned char *pub_key, unsigned char *r, unsigned char *s);

#endif /* _GF2M_H_ */
