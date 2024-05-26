/*
 * drv_tele_mntn_gut.h
 *
 * header for tele mntn gut
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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

#ifndef __DRV_TELE_MNTN_GUT_H__
#define __DRV_TELE_MNTN_GUT_H__

#include <drv_tele_mntn_platform.h>

#define TELE_MNTN_PROTECT1	0x55AA55AA
#define TELE_MNTN_PROTECT2	0x5A5A5A5A
/* mailbox protection word length, unit byte */
#define TELE_MNTN_PROTECT_LEN	(unsigned int)(sizeof(unsigned int))
#define TELE_MNTN_PROTECT	0x43415254 /* TRAC */
#define TELE_MNTN_INIT_MAGIC	0x20140407
#define TELE_MNTN_VALID_MAGIC	0x20142137
#define TELE_MNTN_HEAD_SIZE	(unsigned int)(sizeof(struct tele_mntn_head))
#define TELE_MNTN_DATA_SIZE	(unsigned int)(sizeof(struct tele_mntn_data))
#define tele_mntn_second_to_slice(s)	((s) * 32768)

struct tele_mntn_head {
	unsigned int protect_word1; /* protected word 0x55AA55AA */
	/*
	 * The distance between the queue to be written and
	 * the queue (excluding protection word) header, unit 32bit
	 */
	unsigned int front;
	/*
	 * The distance between the queue to be read and
	 * the queue (excluding protection word) header, unit 32bit
	 */
	unsigned int rear;
	unsigned int protect_word2; /* protected word 0x55AA55AA */
};

struct tele_mntn_data {
	unsigned int protect;
	unsigned int type_id : 7;
	unsigned int len : 7;
	unsigned int idex : 16;
	unsigned int cpu_id : 2;
	unsigned int rtc;
	unsigned int slice;
};

struct tele_mntn_cfg {
	unsigned long long data_addr; /* memory address */
	unsigned int data_size; /* size */
	unsigned int wake_threshold;
	unsigned int wake_interval_threshold; /* time interval between wakeups, s */
	unsigned int user_data_threshold;
	unsigned int ipc_int_num;
	unsigned int global_lock_num;
};

struct tele_mntn_handle {
	unsigned int init_flag;
	unsigned int idex;
	unsigned char *data_virt_addr;
	unsigned int prior_wakeup_acore_slice;
	struct tele_mntn_head head;
	struct tele_mntn_queue queue;
	struct tele_mntn_data data;
	spinlock_t local_lock; /* for tele mntn write log */
	unsigned long local_flag;
	unsigned char *global_lock;
	unsigned long global_flag;
};

int tele_mntn_init(void);
void tele_mntn_exit(void);
int tele_mntn_get_head(struct tele_mntn_queue *queue);
int tele_mntn_set_head(const struct tele_mntn_queue *queue);
#endif
