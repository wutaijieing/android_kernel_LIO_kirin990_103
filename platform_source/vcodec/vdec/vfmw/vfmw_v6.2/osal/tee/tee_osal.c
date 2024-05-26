/*
 * tee_osal.c
 *
 * This is for tee_osal interface.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include "vfmw.h"
#include "vfmw_osal.h"
#include "tee_osal.h"
#include "sre_hwi.h"
#include "secure_gic_common.h"
#include "malloc.h"
#include "sys_timer.h"
#include "pthread.h"
#include "sec_smmu_com.h"
#include "timer_export.h"
#include "dbg.h"

extern UINT32 HM_IntLock(void);
extern void HM_IntRestore(UINT32 uvIntSave);

#define OS_MAX_EVENT (OS_MAX_CHAN  * 4 * 1)
#define OS_MAX_SEMA (OS_MAX_CHAN  * 4 * 4)
#define OS_MAX_MAP_LENGTH (64 * 1024 * 1024)

static uint16_t g_event_used[OS_MAX_EVENT] = {0};
static osal_event g_os_event[OS_MAX_EVENT];
static uint16_t g_sema_used[OS_MAX_SEMA] = {0};
static osal_sema g_os_sema[OS_MAX_SEMA];

#define PRN_OS PRN_ALWS

#define os_get_id_by_handle(handle) ((uint32_t)(handle) & 0xffff)
#define os_get_unid_by_handle(handle) ((uint32_t)(handle) >> 16)

static tee_irq_spin_lock g_os_lock;
static tee_irq_spin_lock g_os_spin_lock[OS_MAX_SPIN_LOCK];
static uint16_t g_spin_lock_used[OS_MAX_SPIN_LOCK] = {0};

#define tee_get_timeout_stamp(millisec, timeout_stamp) \
	do { \
		clock_gettime(CLOCK_MONOTONIC, &(timeout_stamp)); \
		(timeout_stamp).tv_sec += (long)((millisec) / 1000); \
		(timeout_stamp).tv_nsec += (long)(((millisec) % 1000) * 1000000); \
		if ((timeout_stamp).tv_nsec >= 1000000000LL) { \
			(timeout_stamp).tv_sec += 1; \
			(timeout_stamp).tv_nsec -= 1000000000LL; \
		} \
	} while (0)

int32_t tee_spin_lock_irq(tee_irq_spin_lock *lock, unsigned long *lock_flag);
int32_t tee_spin_unlock_irq(tee_irq_spin_lock *lock, unsigned long *lock_flag);

static int32_t tee_get_idle_id(uint16_t used[], int32_t num)
{
	int32_t id;
	static uint16_t unid = 1;
	int32_t ret;
	unsigned long lock_flag;

	ret = tee_spin_lock_irq(&g_os_lock, &lock_flag);
	if (ret != OSAL_OK)
		return -1;

	for (id = 0; id < num; id++) {
		if (used[id] == 0) {
			if (unid >= 0x7000)
				unid = 1;
			used[id] = unid++;
			break;
		}
	}

	ret = tee_spin_unlock_irq(&g_os_lock, &lock_flag);
	if (ret != OSAL_OK)
		return -1;

	if (id < num)
		return id;
	else
		return -1;
}

static int32_t tee_create_task(vcodec_thread *task, thread_attr task_attr,
	void *task_function, void *arg)
{
	pthread_attr_t attr;
	int32_t ret = OSAL_OK;

	if (!task || !task_function)
		return OSAL_ERR;

	task->thread_attr = task_attr;
	(void)pthread_attr_init(&attr);

	if (task->thread_attr == NON_AUTO_DESTROY)
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	else
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	task->thread_status = THREAD_RUNNING;
	if (pthread_create(&(task->thread_id), &attr, task_function, arg) != 0) {
		task->thread_status = THREAD_DESTORY;
		ret = OSAL_ERR;
	}

	pthread_attr_destroy(&attr);

	return ret;
}

static void tee_mb(void)
{
}

static int8_t *tee_map_reg(UADDR phy_addr, uint32_t length)
{
	unused_param(length);
	return (int8_t *)phy_addr;
}

static void tee_unmap_reg(uint8_t *vir_addr)
{
	unused_param(vir_addr);
}

void tee_msleep(uint32_t milli_sec)
{
	__SRE_SwMsleep(milli_sec);
}

static int32_t tee_cond_init(void *vcodec_cond)
{
	int32_t ret;
	pthread_attr_t attr;

	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		dprint(PRN_FATAL, "pthread_attr_init error !");
		return ret;
	}

	ret = pthread_cond_init((osal_cond *)vcodec_cond, (pthread_condattr_t *)&attr);
	if (ret != 0) {
		dprint(PRN_FATAL, "pthread_cond_init error !");
		return OSAL_ERR;
	}

	pthread_attr_destroy(&attr);
	return OSAL_OK;
}

static inline int32_t tee_cond_destroy(void *vcodec_cond)
{
	if (!vcodec_cond)
		return VF_ERR_SYS;
	return (pthread_cond_destroy((osal_cond *)vcodec_cond) == 0) ? OSAL_OK : VF_ERR_SYS;
}

int32_t tee_mutexlock_init(osal_irq_mutex_lock *intr_mutex)
{
	if (!intr_mutex)
		return VF_ERR_SYS;

	if (intr_mutex->is_init == 1)
		return OSAL_OK;

	intr_mutex->is_init = 1;

	if (pthread_mutex_init(&(intr_mutex->irq_lock), 0) != 0) {
		intr_mutex->is_init = 0;
		return VF_ERR_SYS;
	}

	return OSAL_OK;
}

int32_t tee_mutexlock_destroy(osal_irq_mutex_lock *intr_mutex)
{
	if (!intr_mutex)
		return VF_ERR_SYS;

	if (intr_mutex->is_init == 0)
		return OSAL_OK;

	intr_mutex->is_init = 0;

	if (pthread_mutex_destroy(&(intr_mutex->irq_lock)) != 0) {
		intr_mutex->is_init = 1;
		return VF_ERR_SYS;
	}

	return OSAL_OK;
}

int32_t tee_mutexlock_lock(osal_irq_mutex_lock *intr_mutex)
{
	if (!intr_mutex)
		return VF_ERR_SYS;

	if (intr_mutex->is_init == 0) {
		if (tee_mutexlock_init(intr_mutex) != OSAL_OK)
			return VF_ERR_SYS;
		intr_mutex->is_init = 1;
	}

	return (pthread_mutex_lock(&(intr_mutex->irq_lock)) == 0) ? OSAL_OK : VF_ERR_SYS;
}

int32_t tee_mutexlock_unlock(osal_irq_mutex_lock *intr_mutex)
{
	if (!intr_mutex)
		return VF_ERR_SYS;

	return (pthread_mutex_unlock(&(intr_mutex->irq_lock)) == 0) ? OSAL_OK : VF_ERR_SYS;
}

/* *************************SpinLock************************************* */
void tee_spin_lock_irq_init(tee_irq_spin_lock *lock)
{
	if (!lock)
		return;

	lock->irq_lock = 0;
	lock->is_init = 1;
}

static int32_t tee_spin_lock_init(int32_t *handle)
{
	int32_t id;

	id = tee_get_idle_id(g_spin_lock_used, OS_MAX_SPIN_LOCK);
	if (id < 0)
		return OSAL_ERR;

	tee_spin_lock_irq_init(&g_os_spin_lock[id]);

	*handle = id + (g_spin_lock_used[id] << 16); // 16: id offset

	return OSAL_OK;
}

static int32_t tee_spin_lock(int32_t handle, unsigned long *flag)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_SPIN_LOCK || g_spin_lock_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return OSAL_ERR;
	}

	return tee_spin_lock_irq(&g_os_spin_lock[id], flag);
}

static int32_t tee_spin_unlock(int32_t handle, unsigned long *flag)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_SPIN_LOCK || g_spin_lock_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return OSAL_ERR;
	}

	return tee_spin_unlock_irq(&g_os_spin_lock[id], flag);
}

static void tee_spin_lock_exit(int32_t handle)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_SPIN_LOCK || g_spin_lock_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return;
	}

	g_spin_lock_used[id] = 0;
}

/* *************************Time*********************************** */
static uint64_t tee_get_time_in_ms(void)
{
	uint64_t tmp;
	uint64_t curr_ms;
	timeval_t currenttime;

	tmp = SRE_ReadTimestamp();
	currenttime.tval.sec = (tmp >> 32); // 32: time offset
	currenttime.tval.nsec = tmp & 0xffffffff;

	curr_ms = (uint64_t)(currenttime.tval.sec * 1000 +
		currenttime.tval.nsec / 1000000); // 1s = 1000000us 1s = 1000ms

	return curr_ms;
}

static uint64_t tee_get_time_in_us(void)
{
	uint64_t tmp;
	uint64_t curr_us;
	timeval_t currenttime;

	tmp = SRE_ReadTimestamp();
	currenttime.tval.sec = (tmp >> 32); // 32: time offset
	currenttime.tval.nsec = tmp & 0xffffffff;

	curr_us = (uint64_t)(currenttime.tval.sec * 1000000 +
		currenttime.tval.nsec / 1000); // 1s = 1000000us 1s = 1000ms
	return curr_us;
}

int32_t tee_mem_ionmap(void *mcl_para, void *out_mem_param)
{
	int32_t ret;

	if (!mcl_para || !out_mem_param) {
		dprint(PRN_FATAL, "invalid param pMem_Para\n");
		return -1;
	}
	mem_record *mem_para = mcl_para;
	if (mem_para->length > OS_MAX_MAP_LENGTH || mem_para->length == 0) {
		dprint(PRN_FATAL, "invalid  mcl_para's member <length> %d\n", mem_para->length);
		return -1;
	}

	if (mem_para->buff_id == 0) {
		dprint(PRN_FATAL, "invalid  buff_id\n");
		return -1;
	}

	struct mem_chunk_list mcl;

	mcl.protect_id = DRM_PROTECT_ID;
	mcl.cache = 0;
	mcl.buff_id = mem_para->buff_id;
	mcl.size = mem_para->length;
	mcl.smmuid = SMMU_MEDIA2;
	mcl.sid = SECSMMU_STREAMID_VCODEC;
	mcl.ssid = SECSMMU_SUBSTREAMID_VDEC;
	ret = sion_map_iommu(&mcl);
	if (ret) {
		dprint(PRN_FATAL, "map iova failed, ret: 0x%x\n", ret);
		return ret;
	}

	((mem_record *)out_mem_param)->phy_addr = mcl.va;

	return ret;
}

int32_t tee_mem_ion_unmap(void *mcl_para)
{
	if (!mcl_para) {
		dprint(PRN_FATAL, "invalid param pMem_Para\n");
		return -1;
	}

	int32_t ret;
	mem_record *mem_para = mcl_para;
	if (mem_para->length > OS_MAX_MAP_LENGTH || mem_para->length == 0) {
		dprint(PRN_FATAL, "invalid  mcl_para's member <length>\n");
		return -1;
	}

	if (mem_para->buff_id == 0) {
		dprint(PRN_FATAL, "invalid  buff_id\n");
		return -1;
	}

	struct mem_chunk_list mcl;

	mcl.protect_id = DRM_PROTECT_ID;
	mcl.buff_id = mem_para->buff_id;
	mcl.size = mem_para->length;
	mcl.smmuid = SMMU_MEDIA2;
	mcl.sid = SECSMMU_STREAMID_VCODEC;
	mcl.ssid = SECSMMU_SUBSTREAMID_VDEC;
	ret = sion_unmap_iommu(&mcl);
	if (ret)
		dprint(PRN_FATAL, "unmap iommu failed ret: %d, buff_id: %x\n",
			ret, mcl.buff_id);
	mem_para->phy_addr = 0;
	return ret;
}

int tee_hal_request_irq(unsigned int irq, void *handler)
{
	int ret = SRE_HwiCreate(irq, 0xa0, 0, (HWI_PROC_FUNC)handler, 0);
	if (ret) {
		dprint(PRN_FATAL, "Create interrupt failed, irq: 0x%x, ret: 0x%x\n",
			irq, ret);
		return OSAL_ERR;
	}

	ret = SRE_HwiEnable(irq);
	if (ret) {
		dprint(PRN_FATAL, "Eenable interrupt failed, irq: 0x%x, ret: 0x%x\n",
			irq, ret);

		if (SRE_HwiDelete(irq) != TEE_SUCCESS)
			dprint(PRN_FATAL, "Delete interrupt failed, irq: 0x%x\n", irq);

		return OSAL_ERR;
	}

	return OSAL_OK;
}

static int32_t tee_request_irq(uint32_t irq_num, vfmw_irq_handler_t handler)
{
	uint32_t ret;

	ret = tee_hal_request_irq(irq_num, (void *)handler);
	if (ret != 0)
		dprint(PRN_ERROR, "tee_hal_request_irq failed, irq_num = %u\n", irq_num);

	return ret;
}

static int32_t tee_hal_delete_irq(unsigned int irq)
{
	int ret;

	ret = SRE_HwiDisable(irq);
	if (ret) {
		dprint(PRN_FATAL, "Disable interrupt failed, irq: 0x%x, ret: 0x%x\n", irq, ret);
		return OSAL_ERR;
	}

	ret = SRE_HwiDelete(irq);
	if (ret) {
		dprint(PRN_FATAL, "Delete interrupt failed, irq: 0x%x, ret: 0x%x\n", irq, ret);
		return OSAL_ERR;
	}

	return ret;
}

static void tee_delete_irq(uint32_t irq_num)
{
	(void)tee_hal_delete_irq(irq_num);
}

static void tee_dump_proc(void *page, int32_t page_count,
	int32_t *used_bytes, int8_t from_shr, const int8_t *format, ...)
{
	unused_param(page);
	unused_param(page_count);
	unused_param(used_bytes);
	unused_param(from_shr);
	unused_param(format);
}

/* ****************************Sema***************************** */
static int32_t tee_sema_init(int32_t *handle)
{
	int32_t id;

	id = tee_get_idle_id(g_sema_used, OS_MAX_SEMA);
	if (id < 0)
		return OSAL_ERR;

	sem_init(&g_os_sema[id], 0, 1);
	*handle = id + (g_sema_used[id] << 16); // 16: id offset

	return OSAL_OK;
}

static int32_t tee_sema_down(int32_t handle)
{
	int32_t ret;
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return OSAL_ERR;
	}

	ret = sem_wait(&g_os_sema[id]);

	return ret;
}

static int32_t tee_sema_try(int32_t handle)
{
	int32_t ret;
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_OS, "Handle = %x error\n", handle);
		return OSAL_ERR;
	}

	ret = sem_trywait(&g_os_sema[id]);

	return (ret == 0) ? OSAL_OK : OSAL_ERR;
}

static void tee_sema_up(int32_t handle)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return;
	}

	sem_post(&g_os_sema[id]);
}

static void tee_sema_exit(int32_t handle)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (handle == 0 || id >= OS_MAX_SEMA || g_sema_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return;
	}

	g_sema_used[id] = 0;
}

int32_t init_event(osal_event *kernel_event, int32_t init_val)
{
	kernel_event->flag = init_val;

	if (kernel_event->queue_head.mutex_flag != 1) {
		if (tee_mutexlock_init(&(kernel_event->queue_head.pmutx)) != 0)
			return OSAL_ERR;
		kernel_event->queue_head.mutex_flag = 1;
	}

	if (kernel_event->queue_head.cond_flag != 1) {
		if (tee_cond_init(&(kernel_event->queue_head.pcond)) != 0) {
			tee_mutexlock_destroy(&(kernel_event->queue_head.pmutx));
			kernel_event->queue_head.mutex_flag = 0;
			return OSAL_ERR;
		}
		kernel_event->queue_head.cond_flag = 1;
	}

	return OSAL_OK;
}

int32_t deinit_event(osal_event *kernel_event)
{
	if (kernel_event->queue_head.mutex_flag == 1) {
		tee_mutexlock_destroy(&(kernel_event->queue_head.pmutx));
		kernel_event->queue_head.mutex_flag = 0;
	}

	if (kernel_event->queue_head.cond_flag == 1) {
		tee_cond_destroy(&(kernel_event->queue_head.pcond));
		kernel_event->queue_head.cond_flag = 0;
	}

	return OSAL_OK;
}

int32_t set_event(osal_event *kernel_event)
{
	if (!kernel_event)
		return VF_ERR_SYS;

	kernel_event->flag = 1;

	return OSAL_OK;
}

int32_t wait_event(osal_event *kernel_event, uint64_t millisec)
{
	int32_t ret;
	uint64_t count = 0;
	struct timespec timeout_stamp = {0, 0};

	tee_get_timeout_stamp(millisec, timeout_stamp);
	do {
		__SRE_SwMsleep(1);
		count++;
	} while ((kernel_event->flag == 0) && count < millisec);
	if (kernel_event->flag) {
		ret = OSAL_OK;
		kernel_event->flag = 0;
	} else {
		ret = OSAL_ERR;
	}

	return ret;
}

/* ************************EventQueue***************************** */
static int32_t tee_init_waitque(int32_t *handle, int32_t init_val)
{
	int32_t id = tee_get_idle_id(g_event_used, OS_MAX_EVENT);

	if (id < 0)
		return OSAL_ERR;

	if (init_event(&g_os_event[id], init_val) != OSAL_OK)
		return OSAL_ERR;

	*handle = id + (g_event_used[id] << 16); // 16: id offset

	return OSAL_OK;
}

static int32_t tee_wakeup_waitque(int32_t handle)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_EVENT || g_event_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return OSAL_ERR;
	}

	return set_event(&g_os_event[id]);
}

static int32_t tee_wait_waitque(int32_t handle, uint64_t wait_time_ms)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (id >= OS_MAX_EVENT || g_event_used[id] != unid) {
		dprint(PRN_OS, "Handle = %x error\n", handle);
		return OSAL_ERR;
	}

	return wait_event(&g_os_event[id], wait_time_ms);
}

static void tee_exit_waitque(int32_t handle)
{
	int32_t id = os_get_id_by_handle(handle);
	uint32_t unid = os_get_unid_by_handle(handle);

	if (handle == 0 || id >= OS_MAX_EVENT || g_event_used[id] != unid) {
		dprint(PRN_OS, "handle = %x error\n", handle);
		return;
	}

	g_event_used[id] = 0;
}

void tee_udelay(unsigned long usecs)
{
	unused_param(usecs);
}

uint8_t tee_get_cur_sec_mode(void)
{
	return SEC_MODE;
}

static void osal_init_time_intf(vfmw_osal_ops *ops)
{
	ops->get_time_in_ms = tee_get_time_in_ms;
	ops->get_time_in_us = tee_get_time_in_us;
	ops->msleep = tee_msleep;
	ops->usdelay = tee_udelay;
}

static void osal_init_spin_intf(vfmw_osal_ops *ops)
{
	ops->os_spin_lock_init = tee_spin_lock_init;
	ops->os_spin_lock = tee_spin_lock;
	ops->os_spin_unlock = tee_spin_unlock;
	ops->os_spin_lock_exit = tee_spin_lock_exit;
}

static void osal_init_sema_intf(vfmw_osal_ops *ops)
{
	ops->sema_init = tee_sema_init;
	ops->sema_down = tee_sema_down;
	ops->sema_up = tee_sema_up;
	ops->sema_try = tee_sema_try;
	ops->sema_exit = tee_sema_exit;
}

static void osal_init_sys_intf(vfmw_osal_ops *ops)
{
	ops->os_mb = tee_mb;
}

static void osal_init_event_intf(vfmw_osal_ops *ops)
{
	ops->event_init = tee_init_waitque;
	ops->event_give = tee_wakeup_waitque;
	ops->event_wait = tee_wait_waitque;
	ops->event_exit = tee_exit_waitque;
}

static void osal_init_mem_intf(vfmw_osal_ops *ops)
{
	ops->reg_map = tee_map_reg;
	ops->reg_unmap = tee_unmap_reg;
}

static void osal_init_irq_intf(vfmw_osal_ops *ops)
{
	ops->create_thread = tee_create_task;
	ops->request_irq = tee_request_irq;
	ops->free_irq = tee_delete_irq;
}

static void osal_init_comm_intf(vfmw_osal_ops *ops)
{
	ops->dump_proc = tee_dump_proc;
}

void osal_intf_init(void)
{
	vfmw_osal_ops *ops = &g_vfmw_osal_ops;

	(void)memset_s(ops, sizeof(vfmw_osal_ops), 0, sizeof(vfmw_osal_ops));

	tee_spin_lock_irq_init(&g_os_lock);

	osal_init_time_intf(ops);
	osal_init_spin_intf(ops);
	osal_init_sema_intf(ops);
	osal_init_sys_intf(ops);
	osal_init_event_intf(ops);
	osal_init_mem_intf(ops);
	osal_init_irq_intf(ops);
	osal_init_comm_intf(ops);
}

void osal_intf_exit(void)
{
	(void)memset_s(&g_vfmw_osal_ops, sizeof(vfmw_osal_ops), 0, sizeof(vfmw_osal_ops));
}

inline int32_t tee_spin_lock_irq(tee_irq_spin_lock *lock, unsigned long *lock_flag)
{
	unused_param(lock_flag);
	if (lock->is_init == 0) {
		lock->irq_lock = 0;
		lock->is_init = 1;
	}
	lock->irq_lock = HM_IntLock();
	return OSAL_OK;
}

inline int32_t tee_spin_unlock_irq(tee_irq_spin_lock *lock, unsigned long *lock_flag)
{
	unused_param(lock_flag);
	HM_IntRestore(lock->irq_lock);
	return OSAL_OK;
}

