/*
 * linux_osal.c
 *
 * This is for linux_osal proc.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
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

#include "vfmw_osal.h"
#include "linux_osal.h"
#include "linux_proc.h"
#include "dbg.h"
#include <linux/platform_drivers/mm_ion.h>
#include <linux/sched/clock.h>

#define OS_MAX_CHAN VFMW_CHAN_NUM
#define OS_MAX_SEMA (OS_MAX_CHAN * 4 * 4)
#define OS_MAX_EVENT (OS_MAX_CHAN * 4 * 1)
#define OS_MAX_SPIN_LOCK (OS_MAX_CHAN * 4 * 1)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define OS_IO_REMAP_NO_CACHE ioremap
#else
#define OS_IO_REMAP_NO_CACHE ioremap_nocache
#endif

static osal_irq_spin_lock g_os_lock;
static osal_sema g_os_sema[OS_MAX_SEMA];
static osal_event g_os_event[OS_MAX_EVENT];
static osal_irq_spin_lock g_os_spin_lock[OS_MAX_SPIN_LOCK];

static uint16_t g_sema_used[OS_MAX_SEMA] = {0};
static uint16_t g_event_used[OS_MAX_EVENT] = {0};
static uint16_t g_spin_lock_used[OS_MAX_SPIN_LOCK] = {0};

inline int32_t linux_spin_lock_irq(osal_irq_spin_lock *lock, unsigned long *lock_flag)
{
	if (lock->is_init == 0) {
		spin_lock_init(&lock->irq_lock);
		lock->is_init = 1;
	}
	spin_lock_irqsave(&lock->irq_lock, *lock_flag);

	return OSAL_OK;
}

inline int32_t linux_spin_unlock_irq(osal_irq_spin_lock *lock, const unsigned long *lock_flag)
{
	spin_unlock_irqrestore(&lock->irq_lock, *lock_flag);

	return OSAL_OK;
}

/* get kernel idle obj id for obj set */
int32_t linux_get_idle_id(uint16_t used[], int32_t num, uint32_t *idle_id)
{
	int32_t id;
	static uint16_t unid = 1;
	unsigned long lock_flag;

	linux_spin_lock_irq(&g_os_lock, &lock_flag);

	for (id = 0; id < num; id++) {
		if (used[id] == 0) {
			if (unid >= 0x7000)
				unid = 1;

			used[id] = unid++;
			break;
		}
	}

	linux_spin_unlock_irq(&g_os_lock, &lock_flag);

	if (id < num) {
		*idle_id = id;
		return OSAL_OK;
	} else {
		return OSAL_ERR;
	}
}

uint64_t linux_get_time_in_ms(void)
{
	uint64_t sys_time;

	sys_time = sched_clock();
	do_div(sys_time, 1000000);

	return sys_time;
}

uint64_t linux_get_time_in_us(void)
{
	uint64_t sys_time;

	sys_time = sched_clock();
	do_div(sys_time, 1000);

	return sys_time;
}

void *linux_create_thread(void *task_func, void *param, int8_t *task_name)
{
	void *task = NULL;

	task = kthread_create(task_func, param, task_name);
	if (IS_ERR(task)) {
		dprint(PRN_ERROR, "can not create thread!\n");
		return NULL;
	}

	wake_up_process(task);

	return task;
}

int32_t linux_stop_task(void *param)
{
	osal_task *task = (osal_task *)param;

	if (!task) {
		dprint(PRN_FATAL, "param is invalid\n");
		return OSAL_ERR;
	}

	kthread_stop(task);

	return OSAL_OK;
}

int32_t linux_kthread_should_stop(void)
{
	return kthread_should_stop();
}

int32_t linux_request_irq(uint32_t irq, vfmw_irq_handler_t handler,
	unsigned long flags, const char *name, void *dev)
{
	if (request_irq(irq, (irq_handler_t)handler, flags, name, dev) != 0) {
		dprint(PRN_ERROR, "%s request irq failed !\n", name);
		return OSAL_ERR;
	}

	return OSAL_OK;
}

void linux_free_irq(uint32_t irq, void *dev)
{
	free_irq(irq, dev);
}

int32_t linux_init_event(uint32_t *handle, int32_t init_val)
{
	uint32_t id;
	int32_t ret;
	osal_event *event = VCODEC_NULL;

	ret = linux_get_idle_id(g_event_used, OS_MAX_EVENT, &id);
	if (ret != OSAL_OK)
		return OSAL_ERR;

	event = &g_os_event[id];
	event->flag = init_val;
	init_waitqueue_head(&(event->queue_head));

	*handle = id + (g_event_used[id] << 16);

	return OSAL_OK;
}

void linux_exit_event(uint32_t handle)
{
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (handle == 0 || id >= OS_MAX_EVENT || g_event_used[id] != unid) {
		dprint(PRN_ERROR, "handle = %x error\n", handle);
		dump_stack();
		return;
	}

	g_event_used[id] = 0;
}

int32_t linux_give_event(uint32_t handle)
{
	osal_event *event = NULL;
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_EVENT || g_event_used[id] != unid) {
		dprint(PRN_ERROR, "handle = %x error\n", handle);
		dump_stack();
		return OSAL_ERR;
	}

	event = &g_os_event[id];
	event->flag = 1;
	wake_up(&(event->queue_head));

	return OSAL_ERR;
}

int32_t linux_wait_event(uint32_t handle, uint64_t ms_wait_time)
{
	int32_t l_ret;
	osal_event *event = NULL;
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_EVENT || g_event_used[id] != unid) {
		dprint(PRN_ERROR, "Handle = %x error\n", handle);
		dump_stack();
		return OSAL_ERR;
	}

	event = &g_os_event[id];
	l_ret = wait_event_timeout(event->queue_head, (event->flag != 0),
		msecs_to_jiffies(ms_wait_time));
	if (event->flag != 0) {
		event->flag = 0;
		return OSAL_OK;
	}
	event->flag = 0;

	return (l_ret > 0) ? OSAL_OK : OSAL_ERR;
}

int8_t *linux_ioremap(UADDR phy_addr, uint32_t len)
{
	return (int8_t *)OS_IO_REMAP_NO_CACHE(phy_addr, len);
}

void linux_iounmap(uint8_t *vir_addr)
{
	iounmap(vir_addr);
}

void *linux_fopen(const char *file_name, int32_t flags, int32_t mode)
{
	struct file *filp = NULL;

	vfmw_assert_ret(file_name, NULL);

	filp = filp_open(file_name, flags, mode);

	return (IS_ERR(filp)) ? NULL : (void *)filp;
}

void linux_fclose(void *fp)
{
	mm_segment_t oldfs;
	struct file *filp = (struct file *)fp;

	if (filp) {
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		vfs_fsync(filp, 0);
		set_fs(oldfs);
		filp_close(filp, NULL);
	}
}

int32_t linux_fread(char *buf, uint32_t len, void *fp)
{
	int readlen;
	mm_segment_t oldfs;
	struct file *filp = (struct file *)fp;

	if (!filp)
		return -ENOENT;

	if (!buf)
		return -ENOENT;

	if (((filp->f_flags & O_ACCMODE) & (O_RDONLY | O_RDWR)) == 0)
		return -EACCES;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	readlen = vfs_read(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);

	return readlen;
}

int32_t linux_fwrite(const char *buf, uint32_t len, void *fp)
{
	int writelen;
	mm_segment_t oldfs;
	struct file *filp = (struct file *)fp;

	if (!filp)
		return -ENOENT;

	if (!buf)
		return -ENOENT;

	if (((filp->f_flags & O_ACCMODE) & (O_WRONLY | O_RDWR)) == 0)
		return -EACCES;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	writelen = vfs_write(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);

	return writelen;
}

int32_t linux_sema_init(uint32_t *handle)
{
	uint32_t id;
	int32_t ret;

	ret = linux_get_idle_id(g_sema_used, OS_MAX_SEMA, &id);
	if (ret != OSAL_OK)
		return OSAL_ERR;

	sema_init(&g_os_sema[id], 1);
	*handle = id + (g_sema_used[id] << 16);

	return OSAL_OK;
}

void linux_sema_exit(uint32_t handle)
{
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (handle == 0 || id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_ERROR, "handle = %x error\n", handle);
		dump_stack();
		return;
	}

	g_sema_used[id] = 0;
}

int32_t linux_sema_down(uint32_t handle)
{
	int32_t ret;
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_ERROR, "handle = %x error\n", handle);
		dump_stack();
		return OSAL_ERR;
	}

	ret = down_interruptible(&g_os_sema[id]);

	return ret;
}

int32_t linux_sema_try(uint32_t handle)
{
	int32_t ret;
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_ERROR, "Handle = %x error\n", handle);
		dump_stack();
		return OSAL_ERR;
	}

	ret = down_trylock(&g_os_sema[id]);

	return (ret == 0) ? OSAL_OK : OSAL_ERR;
}

void linux_sema_up(uint32_t handle)
{
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_ERROR, "handle = %x error\n", handle);
		dump_stack();
		return;
	}

	up(&g_os_sema[id]);
}

inline void linux_spin_lock_irq_init(osal_irq_spin_lock *lock)
{
	if (!lock)
		return;

	spin_lock_init(&lock->irq_lock);
	lock->is_init = 1;
}

int32_t linux_spin_lock_init(uint32_t *handle)
{
	uint32_t id;
	int32_t ret;

	ret = linux_get_idle_id(g_spin_lock_used, OS_MAX_SPIN_LOCK, &id);
	if (ret != OSAL_OK)
		return OSAL_ERR;

	linux_spin_lock_irq_init(&g_os_spin_lock[id]);

	*handle = id + (g_spin_lock_used[id] << 16);

	return OSAL_OK;
}

void linux_spin_lock_exit(uint32_t handle)
{
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (handle == 0 || id >= OS_MAX_SPIN_LOCK || g_spin_lock_used[id] != unid) {
		dprint(PRN_ERROR, "Handle = %x error\n", handle);
		dump_stack();
		return;
	}

	g_spin_lock_used[id] = 0;
}

int32_t linux_spin_lock(uint32_t handle, unsigned long *lock_flag)
{
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_SPIN_LOCK || g_spin_lock_used[id] != unid) {
		dprint(PRN_ERROR, "Handle = %x error\n", handle);
		dump_stack();
		return OSAL_ERR;
	}

	return linux_spin_lock_irq(&g_os_spin_lock[id], lock_flag);
}

int32_t linux_spin_unlock(uint32_t handle, unsigned long *lock_flag)
{
	int32_t id = handle & 0xffff;
	uint32_t unid = handle >> 16;

	if (id >= OS_MAX_SPIN_LOCK || g_spin_lock_used[id] != unid) {
		dprint(PRN_ERROR, "Handle = %x error\n", handle);
		dump_stack();
		return OSAL_ERR;
	}

	return linux_spin_unlock_irq(&g_os_spin_lock[id], lock_flag);
}

void linux_mb(void)
{
	mb();
}

void linux_udelay(unsigned long usecs)
{
	udelay(usecs);
}

sec_mode linux_get_cur_sec_mode(void)
{
	return NO_SEC_MODE;
}

extern vfmw_osal_ops g_vfmw_osal_ops;

void osal_intf_init(void)
{
	vfmw_osal_ops *ops = &g_vfmw_osal_ops;

	(void)memset_s(ops, sizeof(vfmw_osal_ops), 0, sizeof(vfmw_osal_ops));

	linux_spin_lock_irq_init(&g_os_lock);

	ops->get_time_in_ms = linux_get_time_in_ms;
	ops->get_time_in_us = linux_get_time_in_us;

	ops->os_spin_lock_init = linux_spin_lock_init;
	ops->os_spin_lock_exit = linux_spin_lock_exit;
	ops->os_spin_lock = linux_spin_lock;
	ops->os_spin_unlock = linux_spin_unlock;
	ops->sema_init = linux_sema_init;
	ops->sema_exit = linux_sema_exit;
	ops->sema_down = linux_sema_down;
	ops->sema_try = linux_sema_try;
	ops->sema_up = linux_sema_up;
	ops->file_open = linux_fopen;
	ops->file_close = linux_fclose;
	ops->file_read = linux_fread;
	ops->file_write = linux_fwrite;

	ops->os_mb = linux_mb;
	ops->usdelay = linux_udelay;

	ops->event_init = linux_init_event;
	ops->event_exit = linux_exit_event;
	ops->event_give = linux_give_event;
	ops->event_wait = linux_wait_event;

	ops->reg_map = linux_ioremap;
	ops->reg_unmap = linux_iounmap;

	ops->create_thread = linux_create_thread;
	ops->stop_thread = linux_stop_task;
	ops->thread_should_stop = linux_kthread_should_stop;

	ops->request_irq = linux_request_irq;
	ops->free_irq  = linux_free_irq;
#ifdef VFMW_PROC_SUPPORT
	ops->proc_init = (fn_osal_proc_init)linux_proc_init;
	ops->proc_exit = (fn_osal_proc_exit)linux_proc_exit;
	ops->proc_create = (fn_osal_proc_create)linux_proc_create;
	ops->proc_destroy = (fn_osal_proc_destroy)linux_proc_destroy;
#endif
	ops->dump_proc = (fn_osal_dump_proc)linux_proc_dump;
}

void osal_intf_exit(void)
{
	(void)memset_s(&g_vfmw_osal_ops, sizeof(g_vfmw_osal_ops),
		0, sizeof(vfmw_osal_ops));
}
