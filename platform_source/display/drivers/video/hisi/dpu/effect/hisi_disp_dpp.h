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

#ifndef HISI_DISP_DPP_H
#define HISI_DISP_DPP_H

#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include "isr/hisi_isr_dpp.h"
#include "hisi_chrdev.h"
#include "hisi_disp_composer.h"

#define HISI_DISP_DPP_DEV_NAME  "disp_dpp"
#define HISI_DTS_COMP_DISP_DPP0 "hisilicon,disp_dpp0"
#define HISI_DTS_COMP_DISP_DPP1 "hisilicon,disp_dpp1"
#define HISI_DPP0_CHRDEV_NAME   "disp_dpp0"
#define HISI_DPP1_CHRDEV_NAME   "disp_dpp1"

enum disp_dpp_id {
	DISP_DPP_0 = 0,
	DISP_DPP_1,
	DISP_DPP_ID_MAX,
};

struct dpp_operator_node {
	struct list_head head;

	struct hisi_comp_operator *dpp_operator;
};

struct hisi_dpp_data {
	const char *name;
	uint32_t id;
	struct hisi_disp_isr isr_ctrl[COMP_ISR_MAX];
	struct workqueue_struct *hiace_end_wq;
	struct work_struct hiace_end_work;
	char __iomem *dpu_base;

	atomic_t ref_cnt;
};

struct hisi_dpp_priv_ops {

};

struct hisi_disp_dpp {
	struct platform_device *pdev;

	struct list_head dpp_operators_list; // struct dpp_operator_node
	struct hisi_dpp_data dpp_data;
	struct hisi_dpp_priv_ops *priv_ops;
	struct hisi_disp_chrdev rm_chrdev;
};
int hisi_disp_dpp_on(void);
#endif /* HISI_DISP_DPP_H */
