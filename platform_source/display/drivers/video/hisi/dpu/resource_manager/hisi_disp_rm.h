/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HISI_DISP_RM_H
#define HISI_DISP_RM_H

#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include "hisi_chrdev.h"
#include "hisi_disp_config.h"

#define HISI_DISP_RM_DEV_NAME "dpu_rm"
#define HISI_DTS_COMP_DPU_RM  "hisilicon,dpu_resource"

struct hisi_disp_resource_type;
typedef bool (*request_resource_cb)(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index);

enum {
	RESOURCE_STATE_IDLE = 0,
	RESOURCE_STATE_ACTIVE,
	RESOURCE_STATE_BUSY,
	RESOURCE_STATE_BANNED,
};

struct hisi_resource_fsm {

};

struct hisi_resource_ruler {

};

struct hisi_disp_resource {
	struct list_head list_node;

	atomic_t ref_cnt;
	struct semaphore sem_resouce;

	union operator_id id;
	uint32_t max_lbuf;
	uint32_t using_scene; /* which scene is using the operator */
	uint32_t pipeline_id; /* which pipeline is using the operator */
	uint32_t curr_state;

	/* TODO: need redefine the resource ops and fsm */
	struct hisi_resource_fsm fsm;
	struct hisi_resource_ruler ruler;

	struct hisi_comp_operator *data;
};

struct hisi_disp_resource_type {
	uint32_t type;
	uint32_t caps;
	request_resource_cb request_resource_ops;

	struct list_head resource_list; // struct hisi_disp_resource
};

struct hisi_rm_priv_ops {

};

struct hisi_disp_rm {
	struct platform_device *pdev;
	struct hisi_disp_chrdev rm_chrdev;

	struct hisi_disp_resource_type operator_resource[COMP_OPS_TYPE_MAX];

	struct hisi_rm_priv_ops *priv_ops;
};

struct hisi_disp_rm *hisi_disp_rm_platform_device(void);


#endif /* HISI_DISP_RM_H */
