/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
 *
 * Sample implementation of slimbus Platform Services for a bare-metal
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


#ifndef __SLIMBUS_DRV_REGS_H__
#define __SLIMBUS_DRV_REGS_H__

#include "slimbus_drv_regs_macro.h"

struct configuration {
	volatile uint32_t config_mode;          /* 0x0 - 0x4 */
	volatile uint32_t config_ea;            /* 0x4 - 0x8 */
	volatile uint32_t config_pr_tp;         /* 0x8 - 0xc */
	volatile uint32_t config_fr;            /* 0xc - 0x10 */
	volatile uint32_t config_dport;         /* 0x10 - 0x14 */
	volatile uint32_t config_cport;         /* 0x14 - 0x18 */
	volatile uint32_t config_ea2;           /* 0x18 - 0x1c */
	volatile uint32_t config_thr;           /* 0x1c - 0x20 */
};

struct command_status {
	volatile uint32_t command;              /* 0x0 - 0x4 */
	volatile uint32_t state;                /* 0x4 - 0x8 */
	volatile uint32_t ie_state;             /* 0x8 - 0xc */
	volatile uint32_t mch_usage;            /* 0xc - 0x10 */
};

struct interrupts {
	volatile uint32_t int_en;               /* 0x0 - 0x4 */
	volatile uint32_t interrupt;            /* 0x4 - 0x8 */
};

struct message_fifos {
	volatile uint32_t mc_fifo[16];          /* 0x0 - 0x40 */
};

struct port_interrupts {
	volatile uint32_t p_int_en[16];         /* 0x0 - 0x40 */
	volatile uint32_t p_int[16];            /* 0x40 - 0x80 */
};

struct port_state {
	volatile uint32_t p_state_0;            /* 0x0 - 0x4 */
	volatile uint32_t p_state_1;            /* 0x4 - 0x8 */
};

struct port_fifo_space {
	volatile uint32_t port_fifo[16];        /* 0x0 - 0x40 */
};

struct slimbus_drv_regs {
	struct configuration configuration;         /* 0x0 - 0x20 */
	struct command_status command_status;       /* 0x20 - 0x30 */
	volatile char pad_0[0x8];                   /* 0x30 - 0x38 */
	struct interrupts interrupts;               /* 0x38 - 0x40 */
	struct message_fifos message_fifos;         /* 0x40 - 0x80 */
	struct port_interrupts port_interrupts;     /* 0x80 - 0x100 */
	struct port_state port_state[64];           /* 0x100 - 0x300 */
	volatile char pad_1[0xd00];                 /* 0x300 - 0x1000 */
	struct port_fifo_space port_fifo_space[64]; /* 0x1000 - 0x2000 */
};

#endif /* __SLIMBUS_DRV_REGS_H__ */
