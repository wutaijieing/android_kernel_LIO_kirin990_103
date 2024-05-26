/*
 * drv_tele_mntn_common.h
 *
 * header for tele mntn common
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/tele_mntn.h>
#include <linux/types.h>
#ifndef __DRV_TELE_MNTN_COMMON_H__
#define __DRV_TELE_MNTN_COMMON_H__

#define BIT_32(x)	((unsigned int)(1U << ((unsigned int)(x))))
#define BIT_64(x)	((unsigned long long)(1ULL << ((unsigned int)(x))))
#define TELE_MNTN_OK	0
#define U32_BITS_LIMIT	32
#define U64_BITS_LIMIT	64
#define TELE_MNTN_AXI_SIZE	0x400
#define TELE_MNTN_ERRO	(TELE_MNTN_ERR_BASE | BIT_32(1))
#define TELE_MNTN_ERR_BASE	0x40000000
#define TELE_MNTN_ALREADY_INIT	(TELE_MNTN_ERR_BASE | BIT_32(2))
#define TELE_MNTN_NOT_INIT	(TELE_MNTN_ERR_BASE | BIT_32(5))
#define TELE_MNTN_PARA_INVALID	(TELE_MNTN_ERR_BASE | BIT_32(6))
#define TELE_MNTN_PROTECT_ERR	(TELE_MNTN_ERR_BASE | BIT_32(8))
#define TELE_MNTN_BUF_FULL	(TELE_MNTN_ERR_BASE | BIT_32(10))
#define TELE_MNTN_LOCK_FAIL	(TELE_MNTN_ERR_BASE | BIT_32(12))
#define TELE_MNTN_FUNC_OFF	(TELE_MNTN_ERR_BASE | BIT_32(14))
#define TELE_MNTN_MUTEX_VIRT_ADDR_NULL	(TELE_MNTN_ERR_BASE | BIT_32(7))
#define TELE_MNTN_USER_DATA_OVERFLOW	(TELE_MNTN_ERR_BASE | BIT_32(9))
#define TELE_MNTN_WAKEUP_ACORE_SUCC	(TELE_MNTN_ERR_BASE | BIT_32(11))
#define TELE_MNTN_DDR_SELF_REFRESH	(TELE_MNTN_ERR_BASE | BIT_32(13))

/* a aligned by p bytes */
#define TELE_MNTN_ALIGN(a, p)	(((a) + ((p) - 1)) & ~((p) - 1))
/* a truncation alignment by p bytes */
#define TELE_MNTN_SHORTCUT_ALIGN(a, p)	((a) & ~((p) - 1))
/* logger aligned by 8 bytes */
#define TELE_MNTN_ALIGN_SIZE	sizeof(unsigned long long)
#define TELE_MNTN_LOCK_TIMEOUT	1000
enum tele_mntn_buf_type {
	TELE_MNTN_BUF_CONTROLLABLE = 0xA,
	TELE_MNTN_BUF_INCONTROLLABLE,
};

struct tele_mntn_queue {
	unsigned char *base; /* queue base address */
	unsigned int length; /* queue length,unit byte */
	unsigned char *front; /* write pointer, absolute address, in */
	unsigned char *rear; /* read pointer, absolute address, out */
};

int tele_mntn_common_init(void);
void tele_mntn_common_exit(void);
unsigned long long tele_mntn_func_sw_get(void);
unsigned long long tele_mntn_func_sw_set(unsigned long long sw);
unsigned long long tele_mntn_func_sw_bit_set(unsigned int bit);
unsigned long long tele_mntn_func_sw_bit_clr(unsigned int bit);
bool tele_mntn_is_func_on(enum tele_mntn_type_id type_id);
unsigned int get_rtc_time(void);
struct lpmcu_tele_mntn_stru_s *lpmcu_tele_mntn_get(void);
#endif
