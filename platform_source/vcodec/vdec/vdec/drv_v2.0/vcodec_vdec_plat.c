/*
 * vcodec_vdec_plat.c
 *
 * This is for vdec platform
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
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

#include "vcodec_vdec_plat.h"
#include <linux/sched/clock.h>
#include "vcodec_vdec_regulator.h"

enum {
	PCTRL_PERI_VDEC_IS_FPGA,
	PCTRL_PERI_VDEC_IS_ES,
	PCTRL_PERI_VDEC_STATE,
	PCTRL_PERI_VDEC_MASK,
	PCTRL_PERI_VDEC_VALUE,
	PCTRL_PERI_VDEC_MAX
};

const char *g_device_state_name[PCTRL_PERI_VDEC_MAX] = {
	"vdec_fpga",
	"vdec_es",
	"pctrl_peri_state",
	"pctrl_peri_vdec_mask",
	"pctrl_peri_vdec_value"
};

int32_t vdec_device_probe(void *plt)
{
	int32_t i;
	int32_t ret;
	uint32_t state_info[PCTRL_PERI_VDEC_MAX] = {0};
	struct platform_device *plt_dev = (struct platform_device *)plt;
	struct device *dev = &plt_dev->dev;

	vdec_plat_ops *ops = get_vdec_plat_ops();
	if (ops->vcodec_vdec_device_bypass()) {
		dprint(PRN_FATAL, "bypass vdec device\n");
		return VCODEC_FAILURE;
	}

	for (i = 0; i < PCTRL_PERI_VDEC_MAX; i++) {
		ret = of_property_read_u32(dev->of_node, g_device_state_name[i], &state_info[i]);
		if (ret) {
			dprint(PRN_FATAL, "get %s failed\n", g_device_state_name[i]);
			return VCODEC_FAILURE;
		}
	}

	if (!state_info[PCTRL_PERI_VDEC_IS_FPGA]) {
		dprint(PRN_ALWS, "current is %s not fpga platform\n",
			state_info[PCTRL_PERI_VDEC_IS_ES] ? "es" : "cs");
		return 0;
	}

#ifdef PLATFORM_VCODECV700
#ifdef PCIE_FPGA_VERIFY
	dprint(PRN_ALWS, "current is pcie fpga platform\n");
	return 0;
#else
	return VCODEC_FAILURE;
#endif
#else
	ret = get_vdec_peri_state(state_info);
	return ret;
#endif
}

int32_t get_vdec_peri_state(uint32_t *state_info)
{
	uint32_t peri_vdec_value;
	uint32_t *pctrl_peri_state_vaddr = VCODEC_NULL;
	int32_t ret;
	pctrl_peri_state_vaddr = ioremap(state_info[PCTRL_PERI_VDEC_STATE], 4); // 4: Map space size
	if (!pctrl_peri_state_vaddr) {
		dprint(PRN_FATAL, "ioremap vdec state reg failed\n");
		return VCODEC_FAILURE;
	}

	peri_vdec_value = readl(pctrl_peri_state_vaddr) & state_info[PCTRL_PERI_VDEC_MASK];
	iounmap(pctrl_peri_state_vaddr);

	if (peri_vdec_value == state_info[PCTRL_PERI_VDEC_VALUE]) {
		dprint(PRN_ALWS, "current is %s fpga platform\n",
			state_info[PCTRL_PERI_VDEC_IS_ES] ? "es" : "cs");
		ret = 0;
	} else {
		dprint(PRN_FATAL, "vdec module is not exist");
		ret = VCODEC_FAILURE;
	}
	return ret;
}
