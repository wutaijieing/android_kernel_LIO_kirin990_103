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
#ifndef HISI_OVERLAY_CORE_H
#define HISI_OVERLAY_CORE_H

#include "hisi_drv_utils.h"

struct hisi_composer_device;
struct hisi_composer_data;
typedef int (*priv_present_cb)(struct hisi_composer_device *composer_dev, void *data);

/* the ops for different platform */
struct hisi_composer_priv_ops {
	power_cb prepare_on;
	power_cb overlay_on;
	power_cb post_ov_on;

	power_cb prepare_off;
	power_cb overlay_off;
	power_cb post_ov_off;

	priv_present_cb overlay_prepare;
	priv_present_cb overlay_present;
	priv_present_cb overlay_post;
};

void hisi_online_composer_register_private_ops(struct hisi_composer_priv_ops *priv_ops);
void hisi_offline_composer_register_private_ops(struct hisi_composer_priv_ops *priv_ops);
void hisi_online_init(struct hisi_composer_data *data);
#endif /* HISI_OVERLAY_CORE_H */
