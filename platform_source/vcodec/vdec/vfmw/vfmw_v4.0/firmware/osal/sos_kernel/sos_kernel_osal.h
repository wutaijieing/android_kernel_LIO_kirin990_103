/*
 * sos_kernel_osal.h
 *
 * This is for vfmw tee evironment abstract module.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __VFMW_SOS_HEADER__
#define __VFMW_SOS_HEADER__

#include "sre_hwi.h"
#include "debug.h"
#include "sre_mem.h"
#include "sre_task.h"
#include "sre_ticktimer.h"
#include "sre_base.h"
#include "boot.h"
#include "sre_sem.h"
#include "sre_debug.h"
#include "sre_sys.h"
#include "sre_securemmu.h"
#include "sre_taskmem.h"
#include "init_ext.h"
#include "vfmw.h"

#define TIME_STAMP_LEFT_MOVE 32

typedef HWI_PROC_FUNC osal_irq_handler_t;

/* struct define */
typedef SINT32  osal_file;
typedef SINT32 *osal_task;

typedef struct {
	SINT32        irq_lock;
	unsigned long irq_lockflags;
	SINT32        is_init;
} sos_irq_spin_lock;

/* secure os extern function */
extern int uart_printf_func(const char *fmt, ...);

#endif
