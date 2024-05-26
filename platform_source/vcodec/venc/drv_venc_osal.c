/*
 * drv_venc_osal.c
 *
 * This is for venc drv.
 *
 * Copyright (c) 2009-2020 Huawei Technologies CO., Ltd.
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

#include "drv_venc_osal.h"
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/sched/clock.h>

/*lint -e747 -e712 -e732 -e715 -e774 -e845 -e438 -e838*/

int32_t venc_drv_osal_irq_init(uint32_t irq, irqreturn_t (*callback)(int32_t, void *))
{
	int32_t ret;

	if (irq == 0) {
		VCODEC_FATAL_VENC("params is invaild");
		return VCODEC_FAILURE;
	}

	ret = request_irq(irq, callback, 0, "DT_device", NULL);
	if (ret) {
		VCODEC_FATAL_VENC("request irq[%d] failed ret = 0x%x", irq, ret);
		return VCODEC_FAILURE;
	}
	return 0;
}

void venc_drv_osal_irq_free(uint32_t irq)
{
	free_irq(irq, NULL);
}

int32_t venc_drv_osal_lock_create(spinlock_t **phlock)
{
	spinlock_t *plock = NULL;

	plock = vmalloc(sizeof(spinlock_t));
	if (!plock) {
		VCODEC_FATAL_VENC("vmalloc failed");
		return VCODEC_FAILURE;
	}

	spin_lock_init(plock);
	*phlock = plock;

	return 0;
}

void venc_drv_osal_lock_destroy(spinlock_t *hlock)
{
	if (hlock)
		vfree((void *)hlock);
}

int32_t venc_drv_osal_init_event(vedu_osal_event_t *event)
{
	event->is_done = false;
	init_waitqueue_head(&(event->queue_head));
	return 0;
}

int32_t venc_drv_osal_give_event(vedu_osal_event_t *event)
{
	event->is_done = true;
	wake_up_all(&(event->queue_head));
	return 0;
}

uint64_t osal_get_sys_time_in_ms(void)
{
	uint64_t sys_time;

	sys_time = sched_clock();
	do_div(sys_time, 1000000);

	return sys_time;
}

uint64_t osal_get_sys_time_in_us(void)
{
	uint64_t sys_time;

	sys_time = sched_clock();
	do_div(sys_time, 1000);

	return sys_time;
}

uint64_t osal_get_during_time(uint64_t start_time)
{
	uint64_t end_time = osal_get_sys_time_in_us();

	return time_period(start_time, end_time);
}

void vcodec_sleep_ms(uint32_t millisec)
{
	msleep(millisec);
}

uint32_t *vcodec_mmap(uint32_t addr, uint32_t range)
{
	uint32_t *res_addr = NULL;

	res_addr = (uint32_t *)ioremap(addr, range);//lint !e446
	return res_addr;
}

void vcodec_munmap(uint32_t *mem_addr)
{
	if (!mem_addr) {
		VCODEC_FATAL_VENC("params is invaild");
		return;
	}

	iounmap(mem_addr);
}

int32_t vcodec_strncmp(const char *str_name, const char *dst_name, int32_t size)
{
	if (str_name && dst_name)
		return strncmp(str_name, dst_name, size);

	return VCODEC_FAILURE;
}

void *vcodec_mem_valloc(uint32_t mem_size)
{
	void *mem_addr = NULL;

	if (mem_size)
		mem_addr = vmalloc(mem_size);

	return mem_addr;
}

void vcodec_mem_vfree(const void *mem_addr)
{
	if (mem_addr)
		vfree(mem_addr);
}

void vcodec_venc_init_sem(void *sem)
{
	if (sem)
		sema_init((struct semaphore *)sem, 1);
}

int32_t vcodec_venc_down_interruptible(void *sem)
{
	int32_t ret = -1;

	if (sem)
		ret = down_interruptible((struct semaphore *)sem);

	if (ret)
		VCODEC_FATAL_VENC("ret %d down interruptible fail", ret);

	return  ret;
}

void  vcodec_venc_up_interruptible(void *sem)
{
	if (sem)
		up((struct semaphore *)sem);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
void osal_init_timer(struct timer_list *timer, void (*function)(struct timer_list *))
{
	if (!timer) {
		VCODEC_FATAL_VENC("input timer is invailed");
		return;
	}

	if (!function) {
		VCODEC_FATAL_VENC("input callback function is invailed");
		return;
	}

	timer_setup(timer, function, 0);
}
#else
void osal_init_timer(struct timer_list *timer,
		void (*function)(unsigned long),
		unsigned long data)
{
	if (!timer) {
		VCODEC_FATAL_VENC("input timer is invailed");
		return;
	}

	if (!function) {
		VCODEC_FATAL_VENC("input callback function is invailed");
		return;
	}
	setup_timer(timer, function, data);
}
#endif

void osal_add_timer(struct timer_list *timer, uint64_t time_in_ms)
{
	if (timer == NULL) {
		VCODEC_FATAL_VENC("input timer is availed");
		return;
	}

	mod_timer(timer, jiffies + msecs_to_jiffies(time_in_ms));
}

int32_t osal_del_timer(struct timer_list *timer, bool is_sync)
{
	if (timer == NULL) {
		VCODEC_FATAL_VENC("input timer is availed");
		return VCODEC_FAILURE;
	}

	if (!timer_pending(timer))
		return VCODEC_FAILURE;

	if (is_sync)
		del_timer_sync(timer);
	else
		del_timer(timer);

	return 0;
}

void *osal_fopen(const char *file_name, int32_t flags, int32_t mode)
{
	struct file *filp = NULL;
	filp = filp_open(file_name, flags, mode);
	return (IS_ERR(filp)) ? NULL : (void *) filp;
}

void osal_fclose(void *fp)
{
	mm_segment_t oldfs;
	struct file *filp = (struct file *) fp;

	if (filp) {
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		vfs_fsync(filp, 0);
		set_fs(oldfs);
		filp_close(filp, NULL);
	}
}

int32_t osal_fread(void *fp, char *buf, int32_t len)
{
	int32_t readlen;
	mm_segment_t oldfs;
	struct file *filp = (struct file *) fp;

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

int32_t osal_fwrite(void *fp, const char *buf, int32_t len)
{
	int32_t writelen;
	mm_segment_t oldfs;
	struct file *filp = (struct file *) fp;

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
