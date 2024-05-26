/*
 * vfmw_osal.h
 *
 * This is for vfmw_osal interface.
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

#ifndef VFMW_OSAL
#define VFMW_OSAL

#include "vfmw.h"
#include "linux_osal.h"
#include <linux/io.h>
#include <linux/printk.h>

#define OSAL_OK  0
#define OSAL_ERR 1

typedef enum {
	OS_RDONLY = 00000000,
	OS_WRONLY = 00000001,
	OS_RDWR = 00000002,
	OS_CREATE = 00000100,
	OS_TRUNC = 00001000,
	OS_APPEND = 00002000,
	OS_LARGEFILE = 0400000
} fo_mode;

typedef void *os_file;
typedef int32_t os_sema;
typedef int32_t os_event;
typedef int32_t os_lock;

typedef irq_handler_t vfmw_irq_handler_t;
typedef uint64_t (*fn_osal_get_time_in_ms)(void);
typedef uint64_t (*fn_osal_get_time_in_us)(void);
typedef int32_t (*fn_osal_spin_lock_init)(uint32_t *);
typedef void (*fn_osal_spin_lock_exit)(uint32_t);
typedef int32_t (*fn_osal_spin_lock)(uint32_t, unsigned long *);
typedef int32_t (*fn_osal_spin_unlock)(uint32_t, unsigned long *);
typedef int32_t (*fn_osal_sema_init)(uint32_t *);
typedef void (*fn_osal_sema_exit)(uint32_t);
typedef int32_t (*fn_osal_sema_down)(uint32_t);
typedef int32_t (*fn_osal_sema_try)(uint32_t);
typedef void (*fn_osal_sema_up)(uint32_t);
typedef void *(*fn_osal_file_open)(const char *, int32_t, int32_t);
typedef void (*fn_osal_file_close)(void *);
typedef int32_t (*fn_osal_file_read)(char *, uint32_t, void *);
typedef int32_t (*fn_osal_file_write)(const char *, uint32_t, void *);
typedef void (*fn_osal_mb)(void);
typedef void (*fn_osal_udelay)(unsigned long);
typedef void (*fn_osal_msleep)(uint32_t);
typedef int32_t (*fn_osal_event_init)(uint32_t *, int32_t);
typedef void (*fn_osal_event_exit)(uint32_t);
typedef int32_t (*fn_osal_event_give)(uint32_t);
typedef int32_t (*fn_osal_event_wait)(uint32_t, uint64_t);
typedef int8_t *(*fn_osal_reg_map)(UADDR, uint32_t);
typedef void (*fn_osal_reg_unmap)(uint8_t *);
typedef void *(*fn_osal_create_thread)(void *, void *, int8_t *);
typedef int32_t (*fn_osal_stop_task)(void *);
typedef int32_t (*fn_osal_kthread_should_stop)(void);
typedef int32_t (*fn_osal_request_irq)(uint32_t, vfmw_irq_handler_t, unsigned long, const char *, void *);
typedef void (*fn_osal_free_irq)(uint32_t, void *);
typedef int32_t (*fn_osal_proc_init)(void);
typedef void (*fn_osal_proc_exit)(void);
typedef int32_t (*fn_osal_proc_create)(uint8_t *, void *, void *);
typedef void (*fn_osal_proc_destroy)(uint8_t *);
typedef void (*fn_osal_dump_proc)(void *, int32_t, int32_t *, int8_t, const int8_t *, ...);

typedef struct {
	int32_t check1;

	fn_osal_get_time_in_ms get_time_in_ms;
	fn_osal_get_time_in_us get_time_in_us;
	fn_osal_spin_lock_init os_spin_lock_init;
	fn_osal_spin_lock_exit os_spin_lock_exit;
	fn_osal_spin_lock os_spin_lock;
	fn_osal_spin_unlock os_spin_unlock;
	fn_osal_sema_init sema_init;
	fn_osal_sema_exit sema_exit;
	fn_osal_sema_down sema_down;
	fn_osal_sema_try sema_try;
	fn_osal_sema_up sema_up;
	fn_osal_file_open file_open;
	fn_osal_file_close file_close;
	fn_osal_file_read file_read;
	fn_osal_file_write file_write;
	fn_osal_mb os_mb;
	fn_osal_udelay usdelay;
	fn_osal_msleep msleep;
	fn_osal_event_init event_init;
	fn_osal_event_exit event_exit;
	fn_osal_event_give event_give;
	fn_osal_event_wait event_wait;
	fn_osal_reg_map reg_map;
	fn_osal_reg_unmap reg_unmap;
	fn_osal_create_thread create_thread;
	fn_osal_stop_task stop_thread;
	fn_osal_kthread_should_stop thread_should_stop;
	fn_osal_request_irq request_irq;
	fn_osal_free_irq free_irq;
	fn_osal_proc_init proc_init;
	fn_osal_proc_exit proc_exit;
	fn_osal_proc_create proc_create;
	fn_osal_proc_destroy proc_destroy;
	fn_osal_dump_proc dump_proc;

	int32_t check2;
} vfmw_osal_ops;

extern vfmw_osal_ops g_vfmw_osal_ops;

#define OS_MB                 g_vfmw_osal_ops.os_mb
#define OS_UDELAY             g_vfmw_osal_ops.usdelay
#define OS_MSLEEP             msleep
#define OS_EVENT_INIT         g_vfmw_osal_ops.event_init
#define OS_EVENT_EXIT         g_vfmw_osal_ops.event_exit
#define OS_EVENT_GIVE         g_vfmw_osal_ops.event_give
#define OS_EVENT_WAIT         g_vfmw_osal_ops.event_wait
#define OS_KMAP_REG           g_vfmw_osal_ops.reg_map
#define OS_KUNMAP_REG         g_vfmw_osal_ops.reg_unmap
#define OS_CREATE_THREAD      g_vfmw_osal_ops.create_thread
#define OS_STOP_THREAD        g_vfmw_osal_ops.stop_thread
#define OS_THREAD_SHOULD_STOP g_vfmw_osal_ops.thread_should_stop
#define OS_REQUEST_IRQ        g_vfmw_osal_ops.request_irq
#define OS_FREE_IRQ           g_vfmw_osal_ops.free_irq
#define OS_GET_TIME_MS        g_vfmw_osal_ops.get_time_in_ms
#define OS_GET_TIME_US        g_vfmw_osal_ops.get_time_in_us
#define OS_SPIN_LOCK_INIT     g_vfmw_osal_ops.os_spin_lock_init
#define OS_SPIN_LOCK_EXIT     g_vfmw_osal_ops.os_spin_lock_exit
#define OS_SPIN_LOCK          g_vfmw_osal_ops.os_spin_lock
#define OS_SPIN_UNLOCK        g_vfmw_osal_ops.os_spin_unlock
#define OS_SEMA_INIT          g_vfmw_osal_ops.sema_init
#define OS_SEMA_EXIT          g_vfmw_osal_ops.sema_exit
#define OS_SEMA_DOWN          g_vfmw_osal_ops.sema_down
#define OS_SEMA_TRY           g_vfmw_osal_ops.sema_try
#define OS_SEMA_UP            g_vfmw_osal_ops.sema_up
#define OS_PRINT              printk
#define OS_FOPEN              g_vfmw_osal_ops.file_open
#define OS_FCLOSE             g_vfmw_osal_ops.file_close
#define OS_FREAD              g_vfmw_osal_ops.file_read
#define OS_FWRITE             g_vfmw_osal_ops.file_write
#define OS_PROC_INIT          g_vfmw_osal_ops.proc_init
#define OS_PROC_EXIT          g_vfmw_osal_ops.proc_exit
#define OS_PROC_CREATE        g_vfmw_osal_ops.proc_create
#define OS_PROC_DESTROY       g_vfmw_osal_ops.proc_destroy
#define OS_DUMP_PROC          g_vfmw_osal_ops.dump_proc

sec_mode get_current_sec_mode(void);

#endif
