/*
 * This file define info or valiable for smmuv3 tbu
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

#ifndef _IVP_SMMU_V3_H_
#define _IVP_SMMU_V3_H_

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#else
#include "ivp_dsm_stub.h"
#endif

// tbu reg definition
#define   SMMU_TBU_SCR                      0x1000
#define   TBU_BYPASS                        (1<<3)
#define   TBU_MTLB_HITMP_DIS                (1<<2)
#define   TBU_HAZARD_DIS                    (1<<1)
#define   TBU_UARCH_NON_SEC                 (1<<0)
#define   TBU_NS_UARCH_MASK                 (1<<0)

#define   SMMU_TBU_CR                       0x0000
/* MAX_TOK_TRANS_IVP get from nmanager_SMMUv3 TBU max_tok_trans */
#define   MAX_TOK_TRANS_IVP                 (16 - 1)
#define   TBU_EN_REQ_MASK                   (1<<0)
#define   TBU_EN_REQ_ENABLE                 (1<<0)

#define   SMMU_TBU_CRACK                    0x0004
#define   TBU_EN_ACK                        (1<<0)
#define   TBU_CONNECTED_MASK                (1<<1)
#define   TBU_CONNECTED                     (1<<1)
#define   TBU_DISCONNECTED                  (0<<1)
#define   TBU_TOK_TRANS_MASK                (0xFF<<8)
#define   TBU_TOK_TRANS_SHIFT               8

#define   MAX_CHECK_TIMES                   1000

#define   PREFSLOT_FULL_LEVEL               16
#define   PREFSLOT_FULL_LEVEL_OFFSET        24
#define   FETCHSLOT_FULL_LEVEL              16
#define   FETCHSLOT_FULL_LEVEL_OFFSET       16
#define   DEV_IVP_BUFF_SIZE                 256

enum smmu_state {
	SMMU_STATE_DISABLE,    // SMMU bypass mode
	SMMU_STATE_ENABLE,    // SMMU enable
};

typedef void(*smmu_err_handler_t)(void);

struct ivp_smmu_dev {
	struct device  *dev;
	void __iomem   *reg_base;
	unsigned long   reg_size;
	spinlock_t      spinlock;
	irq_handler_t   isr;
	unsigned int    version;
	unsigned int    irq;
	unsigned int    state;
	smmu_err_handler_t  err_handler;
};

extern struct dsm_client *client_ivp;

void ivp_dsm_error_notify(int error_no);

int  ivp_smmu_trans_enable(struct ivp_smmu_dev *smmu_dev);
int  ivp_smmu_trans_disable(struct ivp_smmu_dev *smmu_dev);

struct ivp_smmu_dev *ivp_smmu_get_device(unsigned long select);


#endif /* _IVP_SMMU_V3H_  */
