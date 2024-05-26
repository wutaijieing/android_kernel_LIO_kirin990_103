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
#ifndef HISI_COMPOSER_NOTIFY_H
#define HISI_COMPOSER_NOTIFY_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/export.h>

#include "hisi_disp_composer.h"

enum {
	DISP_EVENT_UNBLANK = 0,
	DISP_EVENT_BLANK,
	DISP_EVENT_ONLINE_PLAY,
};

struct disp_event {
	struct hisi_composer_device *comp;
	void *data;
};

int composer_register_listener(struct notifier_block *nb);
int composer_unregister_listener(struct notifier_block *nb);
int composer_notifier_call_chain(unsigned long val, void *v);


#endif /* HISI_COMPOSER_NOTIFY_H */
