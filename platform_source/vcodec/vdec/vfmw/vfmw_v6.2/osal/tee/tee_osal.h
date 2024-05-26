/*
 * tee_osal.h
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

#ifndef VFMW_SOS_HEADER
#define VFMW_SOS_HEADER

#include "secmem.h"
#include "vfmw.h"
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "drv_module.h"

#define OSAL_OK  0
#define OSAL_ERR 1
#define UNCACHE_MODE       0
#define DRM_PROTECT_ID     0
#define HEAP_SEC_DRM_BASE  0x5A000000U
#define HEAP_SEC_DRM_SIZE  0x26000000U // (512 * 1024 * 1024)

#define OFS_BASE1 0x300
#define OFS_BASE2 0x400
#define MDMA_BASE 0x80
#define REG_OFFSET 0x060

#define OS_MAX_CHAN      (VFMW_CHAN_NUM)
#define OS_MAX_SPIN_LOCK (OS_MAX_CHAN * 4 * 1)

#define rd_vreg(base, reg, dat) \
	do { \
		dat = *((volatile uint32_t *)((uint8_t *)(base) + (reg))); \
	} while (0)

#define wr_vreg(base, reg, dat) \
	do { \
		*((volatile uint32_t *)((uint8_t *)(base) + (reg))) = dat; \
	} while (0)

typedef int32_t wait_qhead;
typedef int32_t spin_lock;
/*======================================================================*/
/*                              define                                  */
/*======================================================================*/
typedef pthread_mutex_t osal_mutex_lock;
typedef pthread_cond_t osal_cond;
typedef pthread_t osal_thread;
typedef sem_t osal_sema;

#define  VF_OK                   1
#define  VF_NO_MEM              (-1)
#define  VF_ERR_PARAM           (-2)
#define  VF_TIME_OUT            (-3)
#define  VF_VDM_ERR             (-4)
#define  VF_FORCE_STOP          (-5)
#define  VF_ERR_SYS             (-20)
#define  VF_ERR_SPS             (-21)
#define  VF_ERR_PPS             (-22)
#define  VF_ERR_SLICE           (-23)


#define VDM_REG_PHY_ADDR     VDH_BASE_PHY_ADDR
#define VDH_REG_RANGE        0x1000
#define SCD_REG_PHY_ADDR     (VDH_BASE_PHY_ADDR + 0x10000 + 0x3000)
#define SCD_REG_SEC_PHY_ADDR SCD_REG_PHY_ADDR
#define SCD_REG_RANGE        0x400
#define VDH_CRG_REG_PHY_ADDR (VDH_BASE_PHY_ADDR + 0x10000 + 0x3400)
#define VDH_CRG_REG_RANGE    0x200

/* MMU REG */
#define SMMU_SID_REG_PHY_ADDR         (VDH_BASE_PHY_ADDR + 0x16900)
#define SMMU_SID_REG_RANGE            0x800
#define DP_MONIT_REG_PHY_ADDR         (VDH_BASE_PHY_ADDR + 0x17100)
#define DP_MONIT_REG_RANGE            0x800
#define SMMU_TBU_REG_PHY_ADDR         (VDH_BASE_PHY_ADDR + 0x17900)
#define SMMU_TBU_REG_RANGE            0x1000

/*======================================================================*/
/*                              enum e                                  */
/*======================================================================*/
typedef enum {
	THREAD_INIT = 0,
	THREAD_RUNNING,
	THREAD_STOPPING,
	THREAD_EXCEPTION,
	THREAD_DESTORY = THREAD_INIT,
} thread_status;

typedef enum {
	AUTO_DESTROY = 1,
	NON_AUTO_DESTROY,
} thread_attr;

typedef enum {
	MEM_MAP_TYPE_SEC = 0,
	MEM_MAP_TYPE_NSEC,
	MEM_MAP_TYPE_NCACHE,
	MEM_MAP_TYPE_CACHE,
} mem_map_type;

/*======================================================================*/
/*                            struct define                             */
/*======================================================================*/
typedef struct osal_irq_mutex_lock_s {
	osal_mutex_lock irq_lock;
	int32_t is_init;
} osal_irq_mutex_lock;

typedef struct vcodec_event_s {
	osal_irq_mutex_lock pmutx;
	osal_cond pcond;
	int32_t mutex_flag;
	int32_t cond_flag;
} vcodec_event;

typedef struct vcodec_thread_s {
	osal_irq_mutex_lock thread_lock;
	osal_thread thread_id;
	thread_status thread_status;
	thread_attr thread_attr;
} vcodec_thread;

typedef struct {
	spin_lock irq_lock;
	unsigned long irq_lockflags;
	int32_t is_init;
} kern_irq_lock;

typedef struct osal_event_s {
	vcodec_event queue_head;
	int32_t flag;
} osal_event;

typedef struct {
	vdec_dts dts_info;
} vdec_plat;

typedef kern_irq_lock tee_irq_spin_lock;

int32_t tee_mem_kernel_map(void *mcl_para, void *out_mem_param);
int32_t tee_mem_kernel_unmap(void *mcl_para);
int32_t tee_mem_nsmem_map(void *mcl_para, void *out_mem_param);
int32_t tee_mem_nsmem_unmap(void *mcl_para);

void osal_intf_init(void);
void osal_intf_exit(void);

int32_t tee_mem_ionmap(void *mcl_para, void *out_mem_param);
int32_t tee_mem_ion_unmap(void *mcl_para);
uint8_t tee_get_cur_sec_mode(void);

#endif
