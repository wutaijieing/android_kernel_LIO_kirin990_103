// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_interfere.c
 *
 * interference handler of wireless charging
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/workqueue.h>

#define HWLOG_TAG wireless_rx_intfr
HWLOG_REGIST();

#define WLRX_INTFR_CFG_ROW         8
#define WLRX_INTFR_CFG_COL         6
#define WLRX_INTFR_WORK_DELAY      3000 /* ms */

/*
 * interference src
 * src: b0, front_camera open:0x1 close:0xfe
 * src: b1, GSM          open:0x2 close:0xfd
 * src: b2, back_camera  open:0x3 close:0xfc
 *
 */

struct wlrx_intfr {
	u32 src_open;
	u32 src_close;
	int fixed_fop;
	int vtx;
	int vrx;
	int irx;
};

struct wlrx_intfr_dts {
	int src_level;
	struct wlrx_intfr *src_cfg;
};

struct wlrx_intfr_dev {
	struct wlrx_intfr_dts *dts;
	struct delayed_work work;
	struct wlrx_intfr intfr;
	u8 cur_src;
};

static struct wlrx_intfr_dev *g_rx_intfr_di[WLTRX_DRV_MAX];

static struct wlrx_intfr_dev *wlrx_intfr_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_intfr_di[drv_type];
}

u8 wlrx_intfr_get_src(unsigned int drv_type)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return 0;

	return di->cur_src;
}

int wlrx_intfr_get_fixed_fop(unsigned int drv_type)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return WLRX_INTFR_DFT_FIXED_FOP;

	return di->intfr.fixed_fop;
}

int wlrx_intfr_get_vtx(unsigned int drv_type)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return 0;

	return di->intfr.vtx;
}

int wlrx_intfr_get_vrx(unsigned int drv_type)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return 0;

	return di->intfr.vrx;
}

int wlrx_intfr_get_irx(unsigned int drv_type)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return 0;

	return di->intfr.irx;
}

void wlrx_intfr_update_pctrl(unsigned int drv_type, struct wlrx_pctrl *pctrl)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!pctrl || !di)
		return;

	if (di->intfr.vtx > 0)
		pctrl->vtx = min(pctrl->vtx, di->intfr.vtx);
	if (di->intfr.vrx > 0)
		pctrl->vrx = min(pctrl->vrx, di->intfr.vrx);
	if (di->intfr.irx > 0)
		pctrl->irx = min(pctrl->irx, di->intfr.irx);
}

static void wlrx_intfr_work(struct work_struct *work)
{
	int i;
	struct wlrx_intfr intfr;
	struct wlrx_pctrl *product_pcfg = wlrx_pctrl_get_product_cfg(WLTRX_DRV_MAIN);
	struct wlrx_intfr_dev *di = container_of(work, struct wlrx_intfr_dev, work.work);

	if (!di || !product_pcfg)
		return;

	intfr.fixed_fop = WLRX_INTFR_DFT_FIXED_FOP;
	intfr.vtx = product_pcfg->vtx;
	intfr.vrx = product_pcfg->vrx;
	intfr.irx = product_pcfg->irx;
	for (i = 0; i < di->dts->src_level; i++) {
		if (!test_bit(i, (unsigned long *)&di->cur_src))
			continue;
		if (di->dts->src_cfg[i].fixed_fop >= 0)
			intfr.fixed_fop = di->dts->src_cfg[i].fixed_fop;
		if (di->dts->src_cfg[i].vtx > 0)
			intfr.vtx = min(intfr.vtx, di->dts->src_cfg[i].vtx);
		if (di->dts->src_cfg[i].vrx > 0)
			intfr.vrx = min(intfr.vrx, di->dts->src_cfg[i].vrx);
		if (di->dts->src_cfg[i].irx > 0)
			intfr.irx = min(intfr.irx, di->dts->src_cfg[i].irx);
	}

	di->intfr.fixed_fop = intfr.fixed_fop;
	di->intfr.vtx = intfr.vtx;
	di->intfr.vrx = intfr.vrx;
	di->intfr.irx = intfr.irx;
	hwlog_info("[intfr_settings] fop=%d tx_vmax=%d rx_vmax=%d imax=%d\n",
		di->intfr.fixed_fop, di->intfr.vtx, di->intfr.vrx, di->intfr.irx);
}

void wlrx_intfr_handle_settings(unsigned int drv_type, u8 src_state)
{
	int i;
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return;

	for (i = 0; i < di->dts->src_level; i++) {
		if (src_state == di->dts->src_cfg[i].src_open) {
			di->cur_src |= BIT(i);
			break;
		} else if (src_state == di->dts->src_cfg[i].src_close) {
			di->cur_src &= ~BIT(i);
			break;
		}
	}
	if (i >= di->dts->src_level)
		return;

	if (delayed_work_pending(&di->work))
		return;

	hwlog_info("[handle_settings] delay %dms\n", WLRX_INTFR_WORK_DELAY);
	schedule_delayed_work(&di->work, msecs_to_jiffies(WLRX_INTFR_WORK_DELAY));
}

void wlrx_intfr_clear_settings(unsigned int drv_type)
{
	struct wlrx_intfr_dev *di = wlrx_intfr_get_di(drv_type);

	if (!di)
		return;

	di->cur_src = 0;
	schedule_delayed_work(&di->work, msecs_to_jiffies(0));
}

static void wlrx_inftr_parse_src_cfg(const struct device_node *np, struct wlrx_intfr_dts *dts)
{
	int i, len, level;

	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"interference_para", WLRX_INTFR_CFG_ROW, WLRX_INTFR_CFG_COL);
	if (len < 0)
		return;

	level = len / WLRX_INTFR_CFG_COL;
	dts->src_cfg = kcalloc(level, sizeof(*dts->src_cfg), GFP_KERNEL);
	if (!dts->src_cfg)
		return;

	len = power_dts_read_string_array(power_dts_tag(HWLOG_TAG), np,
		"interference_para", (int *)dts->src_cfg, level, WLRX_INTFR_CFG_COL);
	if (len <= 0)
		return;

	dts->src_level = level;
	for (i = 0; i < dts->src_level; i++)
		hwlog_info("interfer_cfg[%d] [src] open:0x%-2x close:0x%-2x\t"
			"[limit] fop:%-3d vtx:%-5d vrx:%-5d irx:%-4d\n", i,
			dts->src_cfg[i].src_open, dts->src_cfg[i].src_close,
			dts->src_cfg[i].fixed_fop, dts->src_cfg[i].vtx,
			dts->src_cfg[i].vrx, dts->src_cfg[i].irx);
}

static int wlrx_intfr_parse_dts(const struct device_node *np, struct wlrx_intfr_dts **dts)
{
	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return -ENOMEM;

	wlrx_inftr_parse_src_cfg(np, *dts);
	return 0;
}

static void wlrx_intfr_kfree_dev(struct wlrx_intfr_dev *di)
{
	if (!di)
		return;

	if (di->dts) {
		kfree(di->dts->src_cfg);
		kfree(di->dts);
	}

	kfree(di);
}

int wlrx_intfr_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_intfr_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_intfr_parse_dts(dev->of_node, &di->dts);
	if (ret)
		goto exit;

	g_rx_intfr_di[drv_type] = di;
	INIT_DELAYED_WORK(&di->work, wlrx_intfr_work);
	return 0;

exit:
	wlrx_intfr_kfree_dev(di);
	return ret;
}

void wlrx_intfr_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_intfr_kfree_dev(g_rx_intfr_di[drv_type]);
	g_rx_intfr_di[drv_type] = NULL;
}
