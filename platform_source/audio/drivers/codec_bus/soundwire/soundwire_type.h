/*
 * soundwire_type.h -- soundwire_type driver
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SOUNDWIRE_TYPE_H__
#define __SOUNDWIRE_TYPE_H__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/completion.h>
#include <linux/pm_wakeup.h>

#include "codec_bus.h"
#include "codec_bus_inner.h"

struct soundwire_priv {
	struct device *dev;
	void __iomem *asp_cfg_mem;
	void __iomem *sdw_mst_mem;
	void __iomem *sdw_slv_mem;
	void __iomem *sdw_fifo_mem;
	enum framer_type cur_frame;
	struct completion enum_comp;
	struct wakeup_source *wake_lock;
};

#endif
