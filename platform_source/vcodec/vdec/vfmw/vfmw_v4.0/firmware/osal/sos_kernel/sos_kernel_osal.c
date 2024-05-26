/*
 * sos_kernel_osal.c
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

#include "sos_kernel_osal.h"

#include "public.h"

#ifdef ENV_SOS_KERNEL

/* SpinLock */
sos_irq_spin_lock g_sos_lock_thread;
sos_irq_spin_lock g_sos_lock_record;
sos_irq_spin_lock g_sos_lock_vo_queue;
sos_irq_spin_lock g_sos_lock_fsp;
sos_irq_spin_lock g_sos_lock_destroy;

/* Extern */
extern vfmw_osal_func_ptr g_vfmw_osal_fun_ptr;

/* secure world printk */
#define SOS_PRINT   uart_printf_func

/* Redefine Unused Functions */
void sos_m_sleep(UINT32 msecs)
{
}

void sos_u_delay(ULONG usecs)
{
}

void sos_mb(void)
{
}

SINT32 sos_init_wait_que(mutex_type mutext_type, SINT32 init_val)
{
	return OSAL_OK;
}

SINT32 sos_wake_up_wait_que(mutex_type mutex_type)
{
	return OSAL_OK;
}

SINT32 sos_wait_wait_que(mutex_type mutex_type, SINT32 wait_time_in_ms)
{
	return OSAL_OK;
}

void sos_sema_init(sem_type type)
{
}

SINT32 sos_sem_down(sem_type type)
{
	return OSAL_OK;
}

void sos_sema_up(sem_type type)
{
}

SINT32 sos_mem_malloc(
	UINT8 *mem_name, UINT32 len, UINT32 align,
	UINT32 is_cached, mem_desc_s *mem_desc)
{
	dprint(PRN_FATAL, "Not support %s\n", __func__);
	return OSAL_ERR;
}

SINT32 sos_mem_free(mem_desc_s *mem_desc)
{
	dprint(PRN_FATAL, "Not support %s\n", __func__);
	return OSAL_ERR;
}

/* sos_register_map(): map register, return virtual address */
SINT8 *sos_register_map(UINT32 phy_addr, UINT32 size)
{
	return (UINT8 *)(intptr_t)phy_addr;
}

/* sos_register_unmap(): unmap register */
void sos_register_unmap(UINT8 *vir_addr, UINT32 size)
{
}

/*  get_spin_lock_by_enum(): Get Lock by enum */
sos_irq_spin_lock *get_spin_lock_by_enum(spin_lock_type lock_type)
{
	sos_irq_spin_lock *sos_lock = NULL;

	switch (lock_type) {
	case G_SPINLOCK_THREAD:
		sos_lock = &g_sos_lock_thread;
		break;

	case G_SPINLOCK_RECORD:
		sos_lock = &g_sos_lock_record;
		break;

	case G_SPINLOCK_VOQUEUE:
		sos_lock = &g_sos_lock_vo_queue;
		break;

	case G_SPINLOCK_FSP:
		sos_lock = &g_sos_lock_fsp;
		break;

	case G_SPINLOCK_DESTROY:
		sos_lock = &g_sos_lock_destroy;
		break;

	default:
		dprint(PRN_ERROR, "%s unkown spin_lock_type %d\n", __func__, lock_type);
		break;
	}

	return sos_lock;
}

/* sos_spin_lock_init(): Init Lock by enum */
void sos_spin_lock_init(spin_lock_type lock_type)
{
	sos_irq_spin_lock *sos_spin_lock = NULL;
	int32_t ret;

	sos_spin_lock = get_spin_lock_by_enum(lock_type);
	if (sos_spin_lock != NULL) {
		ret = memset_s(sos_spin_lock, sizeof(*sos_spin_lock), 0,
			sizeof(*sos_spin_lock));
		if (ret != EOK) {
			dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
			return;
		}
	}
}
/* sos_spin_lock(): Acquire Lock by enum */
SINT32 sos_spin_lock(spin_lock_type lock_type)
{
	sos_irq_spin_lock *sos_spin_lock = NULL;

	sos_spin_lock = get_spin_lock_by_enum(lock_type);
	if (sos_spin_lock != NULL) {
		sos_spin_lock->irq_lock = SRE_IntLock();
		return OSAL_OK;
	} else {
		return OSAL_ERR;
	}
}

/* sos_spin_unlock(): Release Lock by enum */
SINT32 sos_spin_unlock(spin_lock_type lock_type)
{
	sos_irq_spin_lock *sos_spin_lock = NULL;

	sos_spin_lock = get_spin_lock_by_enum(lock_type);
	if (sos_spin_lock != NULL) {
		SRE_IntRestore(sos_spin_lock->irq_lock);
		return OSAL_OK;
	} else {
		return OSAL_ERR;
	}
}

/* sos_get_time_in_ms(): Get system time in ms */
UINT64 sos_get_time_in_ms(void)
{
	UINT64    tmp;
	UINT64    curr_ms;
	timeval_t current_time;

	tmp = SRE_ReadTimestamp();
	current_time.tval.sec  = (tmp >> TIME_STAMP_LEFT_MOVE);
	current_time.tval.nsec = tmp & 0xffffffff;
	curr_ms = current_time.tval.sec * MM_COUNT_OF_A_S +
		current_time.tval.nsec / UM_COUNT_OF_A_MM;

	return curr_ms;
}

/* sos_get_time_in_us(): Get system time in us */
UINT64 sos_get_time_in_us(void)
{
	UINT64    tmp;
	UINT64    curr_us;
	timeval_t current_time;

	tmp = SRE_ReadTimestamp();
	current_time.tval.sec  = (tmp >> TIME_STAMP_LEFT_MOVE);
	current_time.tval.nsec = tmp & 0xffffffff;
	curr_us = current_time.tval.sec * UM_COUNT_OF_A_MM +
		current_time.tval.nsec / MM_COUNT_OF_A_S;

	return curr_us;
}

SINT32 sos_map_addr_check(UINT64 phy, UINT32 size, UINT32 is_normal_mem)
{
	if (phy == 0 || size == 0) {
		dprint(PRN_FATAL, "%s invalid param phy : %pK, size : %u\n", __func__, (void *)(uintptr_t)phy, size);
		return OSAL_ERR;
	}

	if (nomap_phy_addr_check(phy, size)) {
		dprint(PRN_FATAL, "%s nomap_phy_addr_check:invalid param phy : %pK, size : %u\n", __func__,
			(void *)(uintptr_t)phy, size);
		return OSAL_ERR;
	}

	if ((is_normal_mem == 1) || ((is_normal_mem == 0) &&
		(phy >= HEAP_SEC_DRM_BASE) &&
		((phy + size) > HEAP_SEC_DRM_BASE) &&
		((phy + size) <= (HEAP_SEC_DRM_BASE + HEAP_SEC_DRM_SIZE)))) {
		return OSAL_OK;
	}

	dprint(PRN_FATAL, "%s invalid param phy : %pK, size : %u\n", __func__, (void *)(uintptr_t)phy, size);
	return OSAL_ERR;
}

/* Secure IRQ Request & Free */
SINT32 sos_request_irq(
	UINT32 irq_num, osal_irq_handler_t pfn_handler,
	ULONG flags, const char *name, void *dev)
{
	UINT32 ret;

	ret = SRE_HwiCreate(irq_num, 0x0, 0x0, pfn_handler, 0);
	if (ret != 0) {
		dprint(PRN_FATAL, "SRE_HwiCreate failed, irq_num : %d\n", irq_num);
		return ret;
	}

	ret = SRE_HwiEnable(irq_num);
	if (ret != 0)
		dprint(PRN_FATAL, "SRE_HwiEnable failed, irq_num : %d\n", irq_num);

	return ret;
}

void sos_free_irq(UINT32 irq_num, void *dev)
{
	UINT32 ret;

	ret = SRE_HwiDelete(irq_num);
	if (ret != 0)
		dprint(PRN_FATAL, "SRE_HwiDelete failed, irq_num : %d\n", irq_num);
}

/*  Secure OS kernel interface initialization */
SINT32 sos_init_interface(void)
{
	int32_t ret;

	ret = memset_s(&g_vfmw_osal_fun_ptr, sizeof(g_vfmw_osal_fun_ptr),
		0, sizeof(g_vfmw_osal_fun_ptr));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
		return OSAL_ERR;
	}

	g_vfmw_osal_fun_ptr.pfun_osal_get_time_in_ms   = sos_get_time_in_ms;
	g_vfmw_osal_fun_ptr.pfun_osal_get_time_in_us   = sos_get_time_in_us;
	g_vfmw_osal_fun_ptr.pfun_osal_spin_lock_init   = sos_spin_lock_init;
	g_vfmw_osal_fun_ptr.pfun_osal_spin_lock        = sos_spin_lock;
	g_vfmw_osal_fun_ptr.pfun_osal_spin_unlock      = sos_spin_unlock;
	g_vfmw_osal_fun_ptr.pfun_osal_sema_init        = sos_sema_init;
	g_vfmw_osal_fun_ptr.pfun_osal_sema_down        = sos_sem_down;
	g_vfmw_osal_fun_ptr.pfun_osal_sema_up          = sos_sema_up;
	g_vfmw_osal_fun_ptr.pfun_osal_print            = SOS_PRINT;
	g_vfmw_osal_fun_ptr.pfun_osal_m_sleep          = sos_m_sleep;
	g_vfmw_osal_fun_ptr.pfun_osal_mb               = sos_mb;
	g_vfmw_osal_fun_ptr.pfun_osal_u_delay          = sos_u_delay;
	g_vfmw_osal_fun_ptr.pfun_osal_init_event       = sos_init_wait_que;
	g_vfmw_osal_fun_ptr.pfun_osal_give_event       = sos_wake_up_wait_que;
	g_vfmw_osal_fun_ptr.pfun_osal_wait_event       = sos_wait_wait_que;
	g_vfmw_osal_fun_ptr.pfun_osal_mem_alloc        = sos_mem_malloc;
	g_vfmw_osal_fun_ptr.pfun_osal_mem_free         = sos_mem_free;
	g_vfmw_osal_fun_ptr.pfun_osal_register_map     = sos_register_map;
	g_vfmw_osal_fun_ptr.pfun_osal_register_unmap   = sos_register_unmap;
	g_vfmw_osal_fun_ptr.pfun_osal_request_irq      = sos_request_irq;
	g_vfmw_osal_fun_ptr.pfun_osal_free_irq         = sos_free_irq;
	g_vfmw_osal_fun_ptr.pfun_osal_mmap             = NULL;
	g_vfmw_osal_fun_ptr.pfun_osal_mmap_cache       = NULL;
	g_vfmw_osal_fun_ptr.pfun_osal_mun_map          = NULL;
	g_vfmw_osal_fun_ptr.pfun_osal_alloc_vir_mem    = NULL;
	g_vfmw_osal_fun_ptr.pfun_osal_free_vir_mem     = NULL;
	g_vfmw_osal_fun_ptr.pfun_osal_proc_init        = NULL;
	g_vfmw_osal_fun_ptr.pfun_osal_proc_exit        = NULL;

	return OSAL_OK;
}

/*  Init Function Declare  */
DECLARE_TC_DRV(SecureVfmw, 0, 0, 0, 0, sos_init_interface, NULL);

#endif
