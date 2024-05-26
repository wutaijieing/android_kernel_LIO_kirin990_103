/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef COMPOSER_MGR_H
#define COMPOSER_MGR_H

#include <linux/kthread.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/of_reserved_mem.h>
#include <dpu/soc_dpu_define.h>
#include "dkmd_comp.h"
#include "online/dpu_comp_online.h"
#include "offline/dpu_comp_offline.h"
#include "present/dpu_comp_present.h"
#include "peri/dkmd_peri.h"

#define DTS_PATH_LOGO_BUFFER "/reserved-memory/logo-buffer"

typedef union {
	volatile uint64_t status;
	struct {
		uint8_t value[DEVICE_COMP_MAX_COUNT];
	} refcount;
} dpu_comp_status_t;

struct composer_manager {
	/* private data */
	struct platform_device *device;

	struct device_node *parent_node;

	/* Base address register, power supply, clock is public */
	char __iomem *dpu_base;
	char __iomem *actrl_base;
	char __iomem *media1_ctrl_base;

	struct regulator *disp_ch1subsys_regulator;
	struct regulator *dsssubsys_regulator;
	struct regulator *vivobus_regulator;
	struct regulator *media1_subsys_regulator;
	struct regulator *regulator_smmu_tcu;

	struct regulator *dsisubsys_regulator;
	struct regulator *dpsubsys_regulator;
	struct regulator *dss_lite1_regulator;

	/* For exclusive display device power reference count */
	dpu_comp_status_t power_status;
	struct semaphore power_sem;
	struct list_head isr_list;
	uint32_t hardware_reseted;

	int32_t sdp_irq;
	struct dkmd_isr sdp_isr_ctrl;

	int32_t m1_qic_irq;
	struct dkmd_isr m1_qic_isr_ctrl;

	struct mutex tbu_sr_lock;

	struct mutex idle_lock;
	bool idle_enable;
	uint32_t idle_func_flag;
	int32_t idle_expire_count;
	dpu_comp_status_t active_status;

	/* Receive each scene registration */
	struct composer_scene *scene[DPU_SCENE_OFFLINE_2 + 1];
};

/**
 * @brief Each device has its own composer's data structures, interrupt handling,
 * realization of vsync, timeline process is not the same;
 * so, this data structure need to dynamic allocation, and at the same time keep
 * saving composer manager pointer and link next connector pointer.
 *
 */
struct dpu_composer {
	/* composer ops_handle function pointer */
	struct composer comp;

	/* record every time processing data */
	void *present_data;

	/* pointer for connector which will be used for composer */
	struct dkmd_connector_info *conn_info;

	/* save composer manager pointer */
	struct composer_manager *comp_mgr;

	/* public each scene kernel processing threads */
	struct kthread_worker handle_worker;
	struct task_struct *handle_thread;

	struct dkmd_isr isr_ctrl;
	struct dkmd_attr attrs;

	/* link initialize cmdlist id */
	uint32_t init_scene_cmdlist;

	int32_t (*overlay)(struct dpu_composer *dpu_comp, struct disp_frame *frame);
	int32_t (*create_fence)(struct dpu_composer *dpu_comp);
};

static inline struct dpu_composer *to_dpu_composer(struct composer *comp)
{
	return container_of(comp, struct dpu_composer, comp);
}

int32_t composer_manager_set_fastboot(struct composer *comp);
int32_t composer_manager_on(struct composer *comp);
int32_t composer_manager_off(struct composer *comp);
int32_t composer_manager_present(struct composer *comp, void *in_frame);

void composer_manager_reset_hardware(struct dpu_composer *dpu_comp);
void composer_dpu_power_on_sub(struct dpu_composer *dpu_comp);
void composer_dpu_power_off_sub(struct dpu_composer *dpu_comp);
void composer_dpu_free_logo_buffer(struct dpu_composer *dpu_comp);

#endif
