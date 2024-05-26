/*
 * Copyright (c) 2019-2019 Hisilicon Technologies CO., Ltd.
 * Description: hisi usb phy driver support.
 * Author: Xiaodong Guo
 * Create:2019-09-24
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "hisi_usb_phy_inner.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/kern_levels.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeup.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/version.h>
#include <linux/platform_drivers/usb_phy_chip.h>
#include <platform_include/basicplatform/linux/hisi_usb_phy_driver.h>

#ifdef CONFIG_USB_PHY_CHIP_FIX_BC_DETECT
#include <chipset_common/hwpower/hardware_monitor/water_detect.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <pmic_interface.h>
#endif

#define UPC_MAX_VENDOR_PHY	10
#define UPC_VENDOR_NAME_LEN	50

#define UPC_BC_CFG_ELEMENT_NUM	5

#define HIMASK_ENABLE(val)	((val) << 16 | (val))
#define HIMASK_DISABLE(val)	((val) << 16)

#define HISI_UPC_MISC_POR_SOFT_RST	0x04
#define HISI_UPC_MISC_BC_CTRL		0x1C
#define HISI_UPC_MISC_VALID_CTRL	0x24
#define HISI_UPC_MISC_CTRL		0x00011000
#define HISI_UPC_MISC_OFFSET(offset)	((HISI_UPC_MISC_CTRL) + (offset))

#define HISI_UPC_CFG_VBUS_VALID		(0x30u)

#define SNSP_UPC_PLL_CFG_COMMON		5
#define SNSP_UPC_MISC_PLL_CFG		0x18
#define SNSP_UPC_MISC_TEST_MODE0	0x1C
#define SNSP_UPC_MISC_CTRL		0x00010000
#define SNSP_UPC_MISC_OFFSET(offset)	((SNSP_UPC_MISC_CTRL) + (offset))

#define err(fmt, args ...) pr_err("[USB_PHY]"fmt, ##args)
#define info(fmt, args ...) pr_info("[USB_PHY]"fmt, ##args)
#define dbg(fmt, args ...) pr_info("[USB_PHY]"fmt, ##args)

#ifndef UNUSED_VAR
#define UNUSED_VAR(var)    ((void)(var))
#endif

enum usb_phy_bc_ops {
	OPS_EN_BC = 0,
	OPS_CFG_UTMI_XCVR = 1,
	OPS_CFG_UTMI_OPMODE = 2,
	OPS_CFG_UTMI_SUSPEND = 3,
	OPS_EN_BG = 4,
	OPS_EN_DCD = 5,
	OPS_EN_VDATDECTEN = 6,
	OPS_DIS_VDATDETSRCEN = 7,
	OPS_DMPULLDOWN_UP = 8,
	OPS_CHECK_FSVPLUS_STAT = 9,
	OPS_DMPULLDOWN_DOWN = 10,
	OPS_DIS_DCD = 11,
	OPS_VDAT_EN = 12,
	OPS_CHRSEL_DIS = 13,
	OPS_CHECK_CHGDET_STAT = 14,
	OPS_VDAT_DIS = 15,
	OPS_CHRSEL_EN = 16,
	OPS_UTMI_NORMAL_MODE = 17,
	OPS_DIS_BC = 18,
	OPS_DPPULLDOWN_UP = 19,
	OPS_DPPULLDOWN_DOWN = 20,
	OPS_DIS_BG = 21,
};

enum usb_eye_para_ops {
	OPS_EYE_PARA_VDIS = 0,
	OPS_EYE_PARA_VTX = 1,
	OPS_EYE_PARA_VSQ = 2,
};

struct upc_bc_cfg {
	unsigned int ops;
	unsigned int base;
	unsigned int bit_start;
	unsigned int bit_len;
	unsigned int value;
};

struct upc_phy_vendor {
	unsigned int	chip_id;
	unsigned int	bc_cfgs_num;
	unsigned int	eye_para_num;
	unsigned int	vdp_src_enable;
	struct upc_bc_cfg	*bc_cfgs;
	struct upc_bc_cfg	*eye_para_cfgs;
	struct mutex	usb_phy_lock;
};

struct usb_phy_dev {
	unsigned int phy_dev_rst;
	unsigned int phy_vendor_num;
	struct upc_phy_vendor *vendor;
};

struct upc_phy_power {
	bool non_stby_wd_in;
	bool stby_wd_in;
	int vbus_pw_cnt;
	int vbus_connect_irq;
	int vbus_disconnect_irq;
	struct mutex vbus_cnt_lock;
	struct work_struct vbus_present_work;
	struct delayed_work vbus_absent_work;
	struct regulator *vbus_regulator;
	struct notifier_block nb;
	struct wakeup_source *vbus_wakelock;
};

static struct usb_phy_dev *g_upc_usb_phy = NULL;
static int g_remove_burr_cnt;

#ifdef CONFIG_PD_NOTIFY_TEMPORARY
__attribute__((weak)) int power_event_bnc_register(unsigned int type,
		struct notifier_block *nb)
{
	return 0;
}

__attribute__((weak)) int power_event_bnc_unregister(unsigned int type,
		struct notifier_block *nb)
{
	return 0;
}
#endif

static char *__charger_type_string(int charger_type)
{
	switch (charger_type) {
	case CHARGER_TYPE_NONE: return "none";
	case CHARGER_TYPE_SDP: return "sdp";
	case CHARGER_TYPE_DCP: return "dcp";
	case CHARGER_TYPE_CDP: return "cdp";
	case CHARGER_TYPE_UNKNOWN: return "unknown";
	case PLEASE_PROVIDE_POWER: return "sourcing vbus";
	default: return "unknown value";
	}
}

static unsigned int upc_calcu_clr_val(unsigned int bit_len)
{
	unsigned int val = 0;
	unsigned int i;

	for (i = 0; i < bit_len; i++) {
		val = val << 1;
		val = val | BIT(0);
	}
	return val;
}

static inline void __upc_readl_reg(unsigned int *val, unsigned int addr)
{
	_hi6502_readl_u32(val, addr);
	info("[RD] val [0x%x], addr [0x%x]\n", *val, addr);
}

static inline void __upc_writel_reg(unsigned int val, unsigned int addr)
{
	_hi6502_writel_u32(val, addr);
	info("[WR] val [0x%x], addr [0x%x]\n", val, addr);
}

static void upc_reg_set(unsigned int ops, struct upc_bc_cfg *cfgs)
{
	unsigned int index = ops;
	unsigned int temp = 0;
	unsigned int wr_val, clr_bit;

	if (ops != cfgs[index].ops) {
		info("%s: ops is %u, index is %u\n", __func__, ops, index);
		return;
	}

	__upc_readl_reg(&temp, cfgs[index].base);

	clr_bit = upc_calcu_clr_val(cfgs[index].bit_len);
	clr_bit = clr_bit << cfgs[index].bit_start;
	/* set reg-bit to be 0 */
	temp = temp & (~clr_bit);
	/* calculator set reg val */
	wr_val = (cfgs[index].value) << (cfgs[index].bit_start);
	/* calculator set val */
	temp = temp | wr_val;

	__upc_writel_reg(temp, cfgs[index].base);
}

static unsigned int upc_reg_single_bit_judge(unsigned int ops,
	struct upc_bc_cfg *cfgs)
{
	unsigned int index = ops;
	unsigned int temp = 0;

	if (ops != cfgs[index].ops)
		info("%s: cfg is err, ops is %u, index is %u\n", __func__,
			ops, index);

	__upc_readl_reg(&temp, cfgs[index].base);

	if (temp & BIT(cfgs[index].bit_start))
		return 1;
	else
		return 0;
}

static int __get_phy_chip_index(unsigned int *index)
{
	unsigned int temp;
	unsigned int phy_type;

	phy_type = hi6502_chip_type();
	for (temp = 0; temp < g_upc_usb_phy->phy_vendor_num; temp++)
		if (phy_type == g_upc_usb_phy->vendor[temp].chip_id)
			break;

	if (temp == g_upc_usb_phy->phy_vendor_num) {
		err("%s get phy cfg ops FAILED!\n", __func__);
		return -ENODEV;
	}

	*index = temp;
	return 0;
}

void upc_hisi_bc_dplus_pulldown(void)
{
	unsigned int index = 0;
	int ret;
	struct upc_bc_cfg *cfgs = NULL;

	dbg("%s: +\n", __func__);

	ret = __get_phy_chip_index(&index);
	if (ret) {
		err("%s get phy cfg FAILED!\n", __func__);
		return;
	}

	cfgs = g_upc_usb_phy->vendor[index].bc_cfgs;

	/* enable BC */
	upc_reg_set(OPS_EN_BC, cfgs);
	upc_reg_set(OPS_DMPULLDOWN_UP, cfgs);
	upc_reg_set(OPS_DPPULLDOWN_UP, cfgs);
	/* disable BC */
	upc_reg_set(OPS_DIS_BC, cfgs);
	dbg("%s: -\n", __func__);
}

void upc_hisi_bc_dplus_pullup(void)
{
	unsigned int index = 0;
	int ret;
	struct upc_bc_cfg *cfgs = NULL;

	dbg("%s +\n", __func__);

	ret = __get_phy_chip_index(&index);
	if (ret) {
		err("%s get phy cfg FAILED!\n", __func__);
		return;
	}

	cfgs = g_upc_usb_phy->vendor[index].bc_cfgs;

	/* enable BC */
	upc_reg_set(OPS_EN_BC, cfgs);
	upc_reg_set(OPS_DMPULLDOWN_DOWN, cfgs);
	upc_reg_set(OPS_DPPULLDOWN_DOWN, cfgs);
	/* disable BC */
	upc_reg_set(OPS_DIS_BC, cfgs);
	dbg("%s -\n", __func__);
}

/*
 * BC1.2 Spec:
 * If a PD detects that D+ is greater than VDAT_REF, it knows that it is
 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
 * to VDP_UP through RDP_UP
 */
void upc_hisi_bc_disable_vdp_src(void)
{
	unsigned int index = 0;
	int ret;
	struct upc_bc_cfg *cfgs = NULL;

	if (g_upc_usb_phy == NULL) {
		err("%s g_upc_usb_phy is NULL!\n", __func__);
		return;
	}

	dbg("%s: +\n", __func__);

	ret = __get_phy_chip_index(&index);
	if (ret) {
		err("%s get phy cfg FAILED!\n", __func__);
		return;
	}

	mutex_lock(&g_upc_usb_phy->vendor[index].usb_phy_lock);

	if (g_upc_usb_phy->vendor[index].vdp_src_enable == 0)
		goto out;

	g_upc_usb_phy->vendor[index].vdp_src_enable = 0;

	cfgs = g_upc_usb_phy->vendor[index].bc_cfgs;

	dbg("disable VDP_SRC\n");
	upc_reg_set(OPS_VDAT_DIS, cfgs);
	upc_reg_set(OPS_CHRSEL_DIS, cfgs);
	upc_reg_set(OPS_CFG_UTMI_SUSPEND, cfgs);
	/* enable_vdp_src rely on bandgap enable, disable should disable bc bandgap */
	upc_reg_set(OPS_DIS_BG, cfgs);
	upc_reg_set(OPS_DIS_BC, cfgs);

out:
	dbg("%s: -\n", __func__);
	mutex_unlock(&g_upc_usb_phy->vendor[index].usb_phy_lock);
}

void upc_hisi_bc_enable_vdp_src(void)
{
	unsigned int index = 0;
	int ret;
	struct upc_bc_cfg *cfgs = NULL;

	if (g_upc_usb_phy == NULL) {
		err("%s g_upc_usb_phy is NULL!\n", __func__);
		return;
	}

	dbg("%s +\n", __func__);

	ret = __get_phy_chip_index(&index);
	if (ret) {
		err("%s get phy cfg FAILED!\n", __func__);
		return;
	}

	mutex_lock(&g_upc_usb_phy->vendor[index].usb_phy_lock);

	if (g_upc_usb_phy->vendor[index].vdp_src_enable != 0)
		goto out;

	g_upc_usb_phy->vendor[index].vdp_src_enable = 1;

	cfgs = g_upc_usb_phy->vendor[index].bc_cfgs;

	dbg("enable VDP_SRC\n");
	/* enable_vdp_src rely on bc bandgap enable */
	upc_reg_set(OPS_EN_BG, cfgs);
	upc_reg_set(OPS_VDAT_EN, cfgs);
	upc_reg_set(OPS_CHRSEL_DIS, cfgs);

out:
	dbg("%s -\n", __func__);
	mutex_unlock(&g_upc_usb_phy->vendor[index].usb_phy_lock);
}


static void __hisi_charger_primary_detection(struct upc_bc_cfg *cfgs,
	enum chip_charger_type *type)
{
	unsigned int ret;
	/* Primary Detection */
	if (*type == CHARGER_TYPE_NONE) {
		/* VDATASRCENB\VDATTDETENB enable, chrgsel set 0 */
		upc_reg_set(OPS_VDAT_EN, cfgs);
		upc_reg_set(OPS_CHRSEL_DIS, cfgs);
		msleep(40);
		/* check chgdet state */
		ret = upc_reg_single_bit_judge(OPS_CHECK_CHGDET_STAT, cfgs);
		if (ret == 0)
			*type = CHARGER_TYPE_SDP;

		/* disable vdect */
		upc_reg_set(OPS_VDAT_DIS, cfgs);
		dbg("Primary Detection end!\n");
	}
}

static void __hisi_charger_secondary_detection(struct upc_bc_cfg *cfgs,
	enum chip_charger_type *type)
{
	unsigned int ret;
	/* Secondary Detection */
	if (*type == CHARGER_TYPE_NONE) {
		/* VDATASRCENB\VDATTDETENB enable, chrgsel set 1 */
		upc_reg_set(OPS_VDAT_EN, cfgs);
		upc_reg_set(OPS_CHRSEL_EN, cfgs);

		msleep(40);

		/* check chgdet state */
		ret = upc_reg_single_bit_judge(OPS_CHECK_CHGDET_STAT, cfgs);
		if (ret == 0)
			*type = CHARGER_TYPE_CDP;
		else
			*type = CHARGER_TYPE_DCP;

		/* disable vdect */
		upc_reg_set(OPS_VDAT_DIS, cfgs);
		upc_reg_set(OPS_CHRSEL_DIS, cfgs);
		dbg("Secondary Detection end!\n");
	}
}

static void __enable_dcd(struct upc_bc_cfg *cfgs)
{
	dbg("DCD start!\n");
	/* enable DCD */
	upc_reg_set(OPS_EN_BG, cfgs);
	upc_reg_set(OPS_EN_DCD, cfgs);
	upc_reg_set(OPS_EN_VDATDECTEN, cfgs);
	upc_reg_set(OPS_DIS_VDATDETSRCEN, cfgs);
}

static void __disable_dcd(struct upc_bc_cfg *cfgs)
{
	/* disable DCD */
	upc_reg_set(OPS_DIS_DCD, cfgs);
	dbg("DCD end!\n");
}

static int upc_hisi_charger_wait_dcd(struct upc_bc_cfg *cfgs)
{
	unsigned long jiffies_expire;
	unsigned int reg_temp;
	unsigned int i = 0;

	/* wait fsvplus state */
	jiffies_expire = jiffies + msecs_to_jiffies(900);
	msleep(50);
	while (1) {
		reg_temp = upc_reg_single_bit_judge(OPS_CHECK_FSVPLUS_STAT, cfgs);
		if (reg_temp == 0) {
			i++;
			if (i >= 10)
				return 0;
		} else {
			i = 0;
		}

		msleep(10);
		if (time_after(jiffies, jiffies_expire)) {
			dbg("DCD timeout!\n");
			break;
		}
	}

	return -ETIMEDOUT;
}

static enum chip_charger_type upc_hisi_charger_bc(struct upc_bc_cfg *cfgs)
{
	enum chip_charger_type type = CHARGER_TYPE_NONE;

	dbg("+\n");

	/* enable BC */
	upc_reg_set(OPS_EN_BC, cfgs);

	/* phy suspend, set utmi interface signal: xcvr\opmode\suspend */
	upc_reg_set(OPS_CFG_UTMI_XCVR, cfgs);
	upc_reg_set(OPS_CFG_UTMI_OPMODE, cfgs);
	upc_reg_set(OPS_CFG_UTMI_SUSPEND, cfgs);

	/* dmpulldown set up */
	upc_reg_set(OPS_DMPULLDOWN_UP, cfgs);

	/* enable DCD */
	__enable_dcd(cfgs);

	if (upc_hisi_charger_wait_dcd(cfgs) < 0)
		type = CHARGER_TYPE_UNKNOWN;

	/* disable DCD */
	__disable_dcd(cfgs);

	/* dmpulldown set down */
	upc_reg_set(OPS_DMPULLDOWN_DOWN, cfgs);

	/* Primary Detection */
	__hisi_charger_primary_detection(cfgs, &type);

	/* Secondary Detection */
	__hisi_charger_secondary_detection(cfgs, &type);

	if (type == CHARGER_TYPE_CDP)
		/* VDATASRCENB\VDATTDETENB enable, chrgsel set 0 */
		upc_hisi_bc_enable_vdp_src();

	/* If a PD detects that D+ is greater than VDAT_REF, it knows that it is
	 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
	 * to VDP_UP through RDP_UP
	 */
	if (type == CHARGER_TYPE_DCP) {
		dbg("charger is DCP, enable VDP_SRC\n");
		/*
		 * customize if keep D+ and D- short after bc1.2,
		 * whether vdp_src is disable or not, enable vdp_src
		 * VDATASRCENB\VDATTDETENB enable, chrgsel set 0
		 */
		upc_hisi_bc_enable_vdp_src();
	} else {
		upc_reg_set(OPS_UTMI_NORMAL_MODE, cfgs);
		msleep(10);
		/* CDP:enable vdp src rely on bc bandgap enable */
		if (type != CHARGER_TYPE_CDP)
			upc_reg_set(OPS_DIS_BG, cfgs);
		upc_reg_set(OPS_DIS_BC, cfgs);
	}

	dbg("charger type: %s\n", __charger_type_string(type));
	dbg("-\n");

	return type;
}

enum chip_charger_type upc_detect_charger_type(void)
{
	struct upc_bc_cfg *detect_cfg = NULL;
	enum chip_charger_type type = CHARGER_TYPE_NONE;
	unsigned int phy_type, i;

	phy_type = hi6502_chip_type();

	for (i = 0; i < g_upc_usb_phy->phy_vendor_num; i++) {
		if (phy_type == g_upc_usb_phy->vendor[i].chip_id) {
			detect_cfg = g_upc_usb_phy->vendor[i].bc_cfgs;
			if (detect_cfg == NULL) {
				err("%s: error cfgs\n", __func__);
				return CHARGER_TYPE_NONE;
			}
			type = upc_hisi_charger_bc(detect_cfg);
			return type;
		}
	}

	/*
	 * vendor phy is not exist in dts cfg, maybe it is ST PHY,
	 * return CHARGER_TYPE_SDP to ensure USB enum is OK
	 */
	err("[%s] err usb phy chip\n", __func__);
	return CHARGER_TYPE_SDP;
}

void upc_misc_phy_reg_dump(void)
{
	unsigned int phy_type = hi6502_chip_type();
	if (phy_type == HISI_USB_PHY_CHIP) {
		/*
		 * HISI_UPC reg dump details is 0x10000 ~ 0x10380(USB PHY INFO)
		 * MISC CTRL details is 0x11000 ~ 0x11028(USB MISC CTRL)
		 */
		upc_dump_reg(0x10000, 0x18);
		upc_dump_reg(0x100F0, 0x04);
		upc_dump_reg(0x10100, 0x04);
		upc_dump_reg(0x10200, 0x48);
		upc_dump_reg(0x10300, 0x40);
		upc_dump_reg(0x10350, 0x1);
		upc_dump_reg(0x10360, 0x20);
		upc_dump_reg(0x11000, 0x28);
	} else if (phy_type == SNOPSYS_USB_PHY_CHIP) {
		/* SNOPSYS PHY usb misc ctrl details is 0x10000 ~ 0x1003C */
		upc_dump_reg(0x10000, 0x3C);
	} else {
		err("[%s] other usb phy chip\n", __func__);
	}
}

/* upc eye param bit mask */
#define UPC_HISI_EYE_SET_HSTX_DEEN_BIT		0x1
#define UPC_HISI_EYE_SET_HSTX_DE_BIT		0xF
#define UPC_HISI_EYE_SET_HSTX_MBIAS_BIT		0xF
#define UPC_HISI_EYE_DISCREF_BIT		0x7
#define UPC_HISI_EYE_SET_VTX_BIT		0x7
#define UPC_HISI_EYE_SET_VSQ_BIT		0x7
/* upc eye param bit start, analyse eye_param */
#define UPC_HISI_EYE_HSTX_DEEN_START		0x14
#define UPC_HISI_EYE_HSTX_DE_START		0x10
#define UPC_HISI_EYE_HSTX_MBIAS_START		0xC
#define UPC_HISI_EYE_DISCREF_START		0x8
#define UPC_HISI_EYE_VTX_START			0x4
#define UPC_HISI_EYE_VSQ_START			0x0

#define SNOPS_PARAM_NUM 	1

struct upc_eye_param {
	unsigned int bit_mask;
	unsigned int bit_start;
};

const struct upc_eye_param upc_eye_param_bit[] = {
	{ UPC_HISI_EYE_SET_HSTX_MBIAS_BIT, UPC_HISI_EYE_HSTX_MBIAS_START },
	{ UPC_HISI_EYE_DISCREF_BIT, UPC_HISI_EYE_DISCREF_START },
	{ UPC_HISI_EYE_SET_VTX_BIT, UPC_HISI_EYE_VTX_START },
	{ UPC_HISI_EYE_SET_VSQ_BIT, UPC_HISI_EYE_VSQ_START },
	{ UPC_HISI_EYE_SET_HSTX_DE_BIT, UPC_HISI_EYE_HSTX_DE_START },
	{ UPC_HISI_EYE_SET_HSTX_DEEN_BIT, UPC_HISI_EYE_HSTX_DEEN_START }
};

static inline void upc_cal_hisi_eye_param(unsigned int *set_param,
	unsigned int mask_bit, unsigned int start_bit, unsigned int eye_param)
{
	*set_param = (eye_param & (mask_bit << start_bit)) >> start_bit;
}

static int upc_get_hisi_eye_param(struct upc_bc_cfg *cfgs,
	unsigned int size, unsigned int hisi_param)
{
	unsigned int i;
	unsigned int params = 0;

	if (size != ARRAY_SIZE(upc_eye_param_bit))
		return -EINVAL;
	/* get eye param */
	for (i = 0; i < size; i++) {
		upc_cal_hisi_eye_param(&params, upc_eye_param_bit[i].bit_mask,
			upc_eye_param_bit[i].bit_start, hisi_param);
		cfgs[i].value = params;
	}

	return 0;
}

static int upc_set_snsp_eye_param(struct upc_bc_cfg *cfgs,
	unsigned int size, unsigned int snops_param)
{
	if (size != SNOPS_PARAM_NUM)
		return -EINVAL;

	cfgs[0].value = snops_param;

	return 0;
}

void upc_set_eye_diagram_param(unsigned int snops_param,
	unsigned int hisi_param)
{
	unsigned int phy_type;
	unsigned int index;
	unsigned int i;
	unsigned int cfgs_size;
	int ret;
	struct upc_bc_cfg *cfgs = NULL;

	if (g_upc_usb_phy == NULL) {
		err("%s g_upc_usb_phy is NULL!\n", __func__);
		return;
	}

	dbg("%s +\n", __func__);

	phy_type = hi6502_chip_type();

	for (index = 0; index < g_upc_usb_phy->phy_vendor_num; index++)
		if (phy_type == g_upc_usb_phy->vendor[index].chip_id)
			break;

	if (index == g_upc_usb_phy->phy_vendor_num) {
		err("%s get phy cfg ops FAILED!\n", __func__);
		return;
	}

	cfgs_size = g_upc_usb_phy->vendor[index].eye_para_num;

	cfgs = g_upc_usb_phy->vendor[index].eye_para_cfgs;
	if (cfgs == NULL || cfgs_size == 0)
		return;

	mutex_lock(&g_upc_usb_phy->vendor[index].usb_phy_lock);

	if (phy_type == HISI_USB_PHY_CHIP)
		ret = upc_get_hisi_eye_param(cfgs, cfgs_size, hisi_param);
	else if (phy_type == SNOPSYS_USB_PHY_CHIP)
		ret = upc_set_snsp_eye_param(cfgs, cfgs_size, snops_param);
	else
		ret = -ENODEV;

	if (ret != 0){
		err("[%s] err usb phy chip\n", __func__);
		goto out;
	}

	for (i = 0; i < cfgs_size; i++)
		upc_reg_set(i, cfgs);

out:
	mutex_unlock(&g_upc_usb_phy->vendor[index].usb_phy_lock);
}

void upc_phy_reset(void)
{
	unsigned int phy_type = hi6502_chip_type();
	unsigned int temp = 0;

	if (phy_type == HISI_USB_PHY_CHIP || (phy_type == SNOPSYS_USB_PHY_CHIP)) {
		__upc_writel_reg(HIMASK_DISABLE(BIT(1)),
			g_upc_usb_phy->phy_dev_rst);
		if (phy_type == SNOPSYS_USB_PHY_CHIP) {
			/* close t28 pll cfg, for power save */
			__upc_readl_reg(&temp,
				SNSP_UPC_MISC_OFFSET(SNSP_UPC_MISC_PLL_CFG));
			temp = temp | BIT(SNSP_UPC_PLL_CFG_COMMON);
			__upc_writel_reg(temp,
				SNSP_UPC_MISC_OFFSET(SNSP_UPC_MISC_PLL_CFG));
		}
	} else {
		info("[%s] usb st phy rst\n", __func__);
		usb_misc_ulpi_reset(); /* only used by ST USB PHY */
	}
}

void upc_phy_unreset(void)
{
	unsigned int phy_type = hi6502_chip_type();
	unsigned int temp = 0;

	if (phy_type == HISI_USB_PHY_CHIP || (phy_type == SNOPSYS_USB_PHY_CHIP)) {
		if (phy_type == SNOPSYS_USB_PHY_CHIP) {
			/* open t28 pll cfg */
			__upc_readl_reg(&temp,
				SNSP_UPC_MISC_OFFSET(SNSP_UPC_MISC_PLL_CFG));
			temp = temp & (~BIT(SNSP_UPC_PLL_CFG_COMMON));
			__upc_writel_reg(temp,
				SNSP_UPC_MISC_OFFSET(SNSP_UPC_MISC_PLL_CFG));
		}
		__upc_writel_reg(HIMASK_ENABLE(BIT(1)),
			g_upc_usb_phy->phy_dev_rst);
		if (phy_type == HISI_USB_PHY_CHIP) {
			/* keep vbus_vaild signal pulled up */
			info("%s: set vbus valid\n", __func__);
			__upc_readl_reg(&temp,
				HISI_UPC_MISC_OFFSET(HISI_UPC_MISC_VALID_CTRL));
			temp |= HISI_UPC_CFG_VBUS_VALID;
			__upc_writel_reg(temp,
					HISI_UPC_MISC_OFFSET(HISI_UPC_MISC_VALID_CTRL));
		}
	} else {
		info("[%s] usb st phy unrst\n", __func__);
		usb_misc_ulpi_unreset(); /* only used by ST USB PHY */
	}
}

bool upc_phy_driver_is_ready(void)
{
	if (g_upc_usb_phy == NULL)
		return false;
	else
		return true;
}

static void upc_hisi_remove_pw_burr(enum upc_burr_ops_type enable)
{
	UNUSED_VAR(enable);
}

static void upc_snps_remove_pw_burr(enum upc_burr_ops_type enable)
{
	struct upc_bc_cfg burring_cfg[] = {
		/* index reg_base bit_start bit_len default_value */
		/* cfg_siddq bit[2], bit_len: 1 */
		{ 0, SNSP_UPC_MISC_OFFSET(SNSP_UPC_MISC_TEST_MODE0), 2, 1, 0x0 },
		/* cfg_por_reset bit[7], bit_len: 1 */
		{ 1, SNSP_UPC_MISC_OFFSET(SNSP_UPC_MISC_PLL_CFG), 7, 1, 0x0 },
	};

	/* cfg_por_reset & cfg_siddq cfg [1] means close or reset */
	if (enable == UPC_BURR_PW_OFF) {
		burring_cfg[0].value = 0x1;
		burring_cfg[1].value = 0x1;
	}

	upc_reg_set(0, burring_cfg);
	upc_reg_set(1, burring_cfg);
}

/* this func also config utmi phy power reset, which can save power if usb close */
void upc_remove_pw_burr_enable(enum upc_burr_ops_type enable,
	enum upc_burr_cfg_status_type cfg_mode)
{
	unsigned int phy_type = hi6502_chip_type();

	if (cfg_mode == UPC_BURR_RESUME_RESTORE) {
		if (g_remove_burr_cnt) {
			info("[%s] USB burr already config\n", __func__);
			return;
		}
	}

	/* refresh burr remove count */
	if (enable == UPC_BURR_PW_ON)
		g_remove_burr_cnt = 1;
	else
		g_remove_burr_cnt = 0;

	if (phy_type == HISI_USB_PHY_CHIP)
		upc_hisi_remove_pw_burr(enable);
	else if (phy_type == SNOPSYS_USB_PHY_CHIP)
		upc_snps_remove_pw_burr(enable);
	else
		info("[%s] usb st phy, do nothing\n", __func__);
}

static int init_usb_phy_reg(struct device_node *node,
	const char *prop_name, struct upc_bc_cfg *reg, unsigned int reg_num)
{
	unsigned int i, j;
	unsigned int data[5] = {0};
	int ret;

	for (i = 0; i < reg_num; i++) {
		for (j = 0; j < UPC_BC_CFG_ELEMENT_NUM; j++) {
			ret = of_property_read_u32_index(node, prop_name,
				j + i * UPC_BC_CFG_ELEMENT_NUM, &data[j]);
			if (ret) {
				err("%s: %s reg read failed\n",
					__func__, prop_name);
				return -EINVAL;
			}
		}
		reg[i].ops = data[0];
		reg[i].base = data[1];
		reg[i].bit_start = data[2];
		reg[i].bit_len = data[3];
		reg[i].value = data[4];
	}
	return 0;
}

static int usb_phy_get_dts_cfg(struct usb_phy_dev *usb_phy,
	struct device *dev, struct device_node *node)
{
	int ret = 0;
	unsigned int i;
	struct device_node *son = NULL;
	struct upc_phy_vendor *temp = NULL;
	struct upc_bc_cfg *reg = NULL;

	for (i = 0; i < usb_phy->phy_vendor_num; i++) {
		temp = &usb_phy->vendor[i];
		son = of_get_next_child(node, son);
		if (son == NULL) {
			err("%s son and usb_phy's num are mismatching\n",
				__func__);
			ret = -ENODEV;
			goto out;
		}

		ret = of_property_read_u32(son, "chip_type", &(temp->chip_id));
		if (ret) {
			err("%s has error tag <chip_type>\n", __func__);
			goto out;
		}

		ret = of_property_read_u32(son, "upc_bc_cfgs_num", &temp->bc_cfgs_num);
		if (ret) {
			err("%s has error tag <upc_bc_cfgs_num>\n",
				__func__);
			goto out;
		}

		reg = devm_kzalloc(dev,
			(sizeof(struct upc_bc_cfg) * temp->bc_cfgs_num), GFP_KERNEL);
		if (reg == NULL) {
			err("%s: reg malloc failed\n", __func__);
			ret = -ENOMEM;
			goto out;
		}

		ret = init_usb_phy_reg(son, "upc_bc_cfgs", reg, temp->bc_cfgs_num);
		if (ret) {
			err("%s has error tag <upc_bc_cfgs>\n", __func__);
			goto out;
		}
		temp->bc_cfgs = reg;

		ret = of_property_read_u32(son, "upc_eye_para_num", &temp->eye_para_num);
		if (ret) {
			err("%s has error tag <upc_eye_para_num>\n",
				__func__);
			goto out;
		}

		reg = NULL;
		reg = devm_kzalloc(dev,
			(sizeof(struct upc_bc_cfg) * temp->eye_para_num), GFP_KERNEL);
		if (reg == NULL) {
			err("%s: upc_eye_reg malloc failed\n", __func__);
			ret = -ENOMEM;
			goto out;
		}

		ret = init_usb_phy_reg(son, "upc_eye_para_cfgs", reg, temp->eye_para_num);
		if (ret) {
			err("%s has error tag <upc_eye_para_cfgs>\n", __func__);
			goto out;
		}
		temp->eye_para_cfgs = reg;
		temp->vdp_src_enable = 0;

		mutex_init(&temp->usb_phy_lock);
	}
out:
	return ret;
}

static int upc_usb_phy_reset(struct phy *phy)
{
	struct usb_phy_dev *usb_phy = phy_get_drvdata(phy);
	unsigned int phy_type = hi6502_chip_type();
	if (phy_type == HISI_USB_PHY_CHIP) {
		__upc_writel_reg(HIMASK_DISABLE(BIT(1)),
			usb_phy->phy_dev_rst);
		udelay(100);
		__upc_writel_reg(HIMASK_ENABLE(BIT(1)),
			usb_phy->phy_dev_rst);
	}

	return 0;
}

static struct phy_ops upc_usb_phy_ops = {
	.reset = upc_usb_phy_reset,
	.owner = THIS_MODULE,
};

#ifdef CONFIG_USB_PHY_CHIP_FIX_BC_DETECT

#if defined(HISI_UPC_BC_FIX_FACTORY)
/* ensure no impact on production line, set delay as 500ms */
#define PHY_POWER_DELAY_MS		500UL
#else
/* ensure the safety of normal scenes, set delay as 10s */
#define PHY_POWER_DELAY_MS		10000UL
#endif


static struct upc_phy_power *g_upc_phy_power;
static void open_avdd_power(void)
{
	unsigned int val;

	val = pmic_read_reg(PMIC_LDO23_ONOFF_ECO_ADDR(0));
	val = val | BIT(PMIC_LDO23_ONOFF_ECO_reg_ldo23_en_END);
	pmic_write_reg(PMIC_LDO23_ONOFF_ECO_ADDR(0), val);
}

static void print_avdd_power(void)
{
	unsigned int val;

	val = pmic_read_reg(PMIC_LDO23_ONOFF_ECO_ADDR(0));
	info("upc_avdd stat : 0x%x\n", val);
}

static void upc_vbus_absent_handler(struct work_struct *work)
{
	int ret;
	struct upc_phy_power *phy_power = container_of(work,
		struct upc_phy_power, vbus_absent_work.work);

	mutex_lock(&(phy_power->vbus_cnt_lock));

	info("upc_avdd non_stby_wd_in-%d stby_wd_in-%d\n",
		phy_power->non_stby_wd_in, phy_power->stby_wd_in);
	/* if water detect in, then not close avdd33 */
	if (phy_power->non_stby_wd_in || phy_power->stby_wd_in)
		goto unlock;

	if (pmic_get_vbus_status()) {
		info("upc_avdd usb is inserted, no need to close power\n");
		goto unlock;
	}

	if (phy_power->vbus_pw_cnt == UPC_AVDD33_ENABLE) {
		ret = regulator_disable(phy_power->vbus_regulator);
		if (ret != 0) {
			err("upc_avdd regulator disable err %d\n", ret);
			goto unlock;
		}
		phy_power->vbus_pw_cnt = UPC_AVDD33_DISABLE;
		info("upc_avdd regulator disable\n");
	} else {
		info("upc_avdd regulator has been disabled before\n");
	}

unlock:
	print_avdd_power();
	mutex_unlock(&(phy_power->vbus_cnt_lock));
}

static void upc_vbus_present_handler(struct work_struct *work)
{
	int ret;
	struct upc_phy_power *phy_power = container_of(work,
		struct upc_phy_power, vbus_present_work);

	/* cancle delay work in case it disable avdd */
	cancel_delayed_work_sync(&(phy_power->vbus_absent_work));

	mutex_lock(&(phy_power->vbus_cnt_lock));

	info("upc_avdd non_stby_wd_in-%d stby_wd_in-%d\n",
		phy_power->non_stby_wd_in, phy_power->stby_wd_in);

	if (phy_power->vbus_pw_cnt == UPC_AVDD33_DISABLE) {
		ret = regulator_enable(phy_power->vbus_regulator);
		if (ret != 0) {
			err("upc_avdd regulator enable err %d\n", ret);
			goto unlock;
		}
		phy_power->vbus_pw_cnt = UPC_AVDD33_ENABLE;
		info("upc_avdd regulator enable\n");
	} else {
		info("upc_avdd regulator has been enabled before\n");
	}
	print_avdd_power();

unlock:
	mutex_unlock(&(phy_power->vbus_cnt_lock));
}

static irqreturn_t upc_vbus_intr(int irq, void *data)
{
	bool ret = false;
	struct upc_phy_power *phy_power = (struct upc_phy_power *)data;

	if (irq == phy_power->vbus_connect_irq) {
		info("upc_avdd vbus_connect\n");
		/* open avdd in interrupt as quickly as we can */
		open_avdd_power();
		ret = queue_work(system_highpri_wq, &(phy_power->vbus_present_work));
		if (ret == false)
			err("upc_avdd present work already running\n");
	} else {
		info("upc_avdd vbus_disconnect\n");
		ret = mod_delayed_work(system_wq, &phy_power->vbus_absent_work,
			msecs_to_jiffies(PHY_POWER_DELAY_MS));
		if (ret == false)
			err("upc_avdd absent work is idle or running\n");
	}

	return IRQ_HANDLED;
}

void upc_disable_vbus_power(void)
{
	if (!g_upc_phy_power)
		return;

	if (delayed_work_pending(&g_upc_phy_power->vbus_absent_work)) {
		info("upc_avdd absent work is running\n");
		__pm_wakeup_event(g_upc_phy_power->vbus_wakelock,
			PHY_POWER_DELAY_MS);
	} else {
		info("upc_avdd absent work is not running\n");
	}
	return;
}

static void __update_wd_dry_stat(unsigned long event, bool *state,
	struct delayed_work *absent_work)
{
	bool ret = false;

	*state = false;
	/* system_freezable_wq's work will exec before system suspend */
	ret = mod_delayed_work(system_wq, absent_work,
		msecs_to_jiffies(PHY_POWER_DELAY_MS));
	if (ret == false)
		err("%s %lu absent work is idle or running\n", __func__, event);
}

static void __update_wd_moist_stat(unsigned long event, bool *state,
	struct work_struct *present_work)
{
	bool ret = false;

	*state = true;

	ret = queue_work(system_highpri_wq, present_work);
	if (ret == false)
		err("%s %lu present work already running\n", __func__, event);
}

static int upc_water_detect_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	unsigned int wd_flags;

	if (!g_upc_phy_power)
		return 0;

	if ((event != POWER_NE_WD_REPORT_UEVENT) || !data)
		return 0;

	mutex_lock(&(g_upc_phy_power->vbus_cnt_lock));

	wd_flags = *(unsigned int *)data;
	info("upc_avdd wd_flag-%u non_stby_wd-%d stby_wd-%d +\n", wd_flags,
		g_upc_phy_power->non_stby_wd_in, g_upc_phy_power->stby_wd_in);

	switch (wd_flags) {
	case WD_NON_STBY_DRY:
		__update_wd_dry_stat(WD_NON_STBY_DRY,
			&g_upc_phy_power->non_stby_wd_in,
			&g_upc_phy_power->vbus_absent_work);
		break;

	case WD_NON_STBY_MOIST:
		__update_wd_moist_stat(WD_NON_STBY_MOIST,
			&g_upc_phy_power->non_stby_wd_in,
			&g_upc_phy_power->vbus_present_work);
		break;

	case WD_STBY_DRY:
		__update_wd_dry_stat(WD_STBY_DRY,
			&g_upc_phy_power->stby_wd_in,
			&g_upc_phy_power->vbus_absent_work);
		break;

	case WD_STBY_MOIST:
		__update_wd_moist_stat(WD_STBY_MOIST,
			&g_upc_phy_power->stby_wd_in,
			&g_upc_phy_power->vbus_present_work);
		break;

	default:
		break;
	}
	info("upc_avdd non_stby_wd-%d stby_wd-%d -\n",
		g_upc_phy_power->non_stby_wd_in, g_upc_phy_power->stby_wd_in);

	mutex_unlock(&(g_upc_phy_power->vbus_cnt_lock));
	return 0;
}

static void __phy_power_init(struct upc_phy_power *phy_power)
{
	int ret;

	mutex_lock(&phy_power->vbus_cnt_lock);
	/*
	 * if avdd33 has been enabled, vbus connect irq happened before this func,
	 * we have no need to enable regulator again
	 */
	if (phy_power->vbus_pw_cnt == UPC_AVDD33_ENABLE)
		goto unlock;
	/*
	 * if avdd33 has been enabled before probe_func request irq action,
	 * we need enable regulator to ensure others won't close avdd33 via regulator api
	 */
	if (pmic_get_vbus_status()) { /* usb is inserted */
		info("upc_avdd usb is inserted\n");
		ret = regulator_enable(phy_power->vbus_regulator);
		if (ret != 0) {
			phy_power->vbus_pw_cnt = UPC_AVDD33_DISABLE;
			err("upc_avdd regulator enable err %d\n", ret);
		} else {
			phy_power->vbus_pw_cnt = UPC_AVDD33_ENABLE;
			info("upc_avdd regulator enable\n");
		}
	} else { /* usb is not inserted */
		info("upc_avdd usb is not inserted\n");
		phy_power->vbus_pw_cnt = UPC_AVDD33_DISABLE;
	}

unlock:
	mutex_unlock(&phy_power->vbus_cnt_lock);
}

static void phy_power_init(struct device *dev)
{
	struct upc_phy_power *phy_power = NULL;
	struct device_node *vbus_np = NULL;
	int ret;

	phy_power = devm_kzalloc(dev, sizeof(*phy_power), GFP_KERNEL);
	if (!phy_power) {
		err("%s no mem for usb phy power\n", __func__);
		return;
	}
	/* init phy power init state */
	phy_power->vbus_pw_cnt = UPC_AVDD33_DISABLE;
	phy_power->stby_wd_in = false;
	phy_power->non_stby_wd_in = false;

	phy_power->vbus_regulator = devm_regulator_get(dev, "vbus");
	if (IS_ERR_OR_NULL(phy_power->vbus_regulator)) {
		err("%s get regulator err\n", __func__);
		goto free_mem;
	}

	mutex_init(&phy_power->vbus_cnt_lock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	phy_power->vbus_wakelock = wakeup_source_register(dev, "upc_vbus_wakelock");
#else
	phy_power->vbus_wakelock = wakeup_source_register("upc_vbus_wakelock");
#endif
	if (!phy_power->vbus_wakelock)
		err("%s wakeup source register failed\n", __func__);

	INIT_WORK(&phy_power->vbus_present_work, upc_vbus_present_handler);
	INIT_DELAYED_WORK(&phy_power->vbus_absent_work, upc_vbus_absent_handler);

	/* find vbus power node in pmu dts */
	vbus_np = of_find_compatible_node(NULL, NULL, "hisilicon-usbvbus");
	if (vbus_np == NULL) {
		err("%s %s node get failed\n", __func__, "hisilicon-usbvbus");
		goto free_wakelock_regulator;
	}

	phy_power->vbus_connect_irq = of_irq_get_byname(vbus_np, "connect");
	phy_power->vbus_disconnect_irq = of_irq_get_byname(vbus_np, "disconnect");
	info("%s vbus irq: %d %d\n", __func__,
		phy_power->vbus_connect_irq, phy_power->vbus_disconnect_irq);
	if (phy_power->vbus_connect_irq <= 0 || phy_power->vbus_disconnect_irq <= 0)
		goto free_wakelock_regulator;

	ret = request_irq(phy_power->vbus_connect_irq, upc_vbus_intr,
		IRQF_SHARED | IRQF_NO_SUSPEND, "upc_vbus_connect", phy_power);
	if (ret) {
		err("%s: request vbus_connect irq err, ret %d\n", __func__, ret);
		goto free_wakelock_regulator;
	}

	ret = request_irq(phy_power->vbus_disconnect_irq, upc_vbus_intr,
		IRQF_SHARED | IRQF_NO_SUSPEND, "upc_vbus_disconnect", phy_power);
	if (ret) {
		err("%s: request vbus_connect irq err, ret %d\n", __func__, ret);
		goto free_connect_irq;
	}

	g_upc_phy_power = phy_power;

	/* handler water detect */
	phy_power->nb.notifier_call = upc_water_detect_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_WD, &phy_power->nb);
	if (ret) {
		err("%s: wd notifier register err, ret %d\n", __func__, ret);
		phy_power->nb.notifier_call = NULL;
	}

	__phy_power_init(phy_power);

	info("phy power fixed init ok, delay time is %lu\n", PHY_POWER_DELAY_MS);

	return;

free_connect_irq:
	free_irq(phy_power->vbus_connect_irq, phy_power);

free_wakelock_regulator:
	wakeup_source_unregister(phy_power->vbus_wakelock);
	devm_regulator_put(phy_power->vbus_regulator);

free_mem:
	devm_kfree(dev, phy_power);
}
#endif /* CONFIG_USB_PHY_CHIP_FIX_BC_DETECT */

static int usb_phy_probe(struct platform_device *pdev)
{
	struct usb_phy_dev *usb_phy = NULL;
	struct upc_phy_vendor *temp = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct phy *phy = NULL;
	struct phy_provider *phy_provider = NULL;
	int ret;

	if (!hi6502_is_ready()) {
		err("usb phy chip is not ready\n");
		return -EPROBE_DEFER;
	}

	dbg("%s ++\n", __func__);
	if (!node) {
		err("dts[%s] node not found\n", "hisilicon,usb_phy_device");
		ret = -ENODEV;
		goto out;
	}

	usb_phy = devm_kzalloc(dev, sizeof(struct usb_phy_dev), GFP_KERNEL);
	if (!usb_phy) {
		err("no mem for usb phy device\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = of_property_read_u32(node, "phy_dev_reset", &usb_phy->phy_dev_rst);
	if (ret) {
		err("%s has no tag <phy_dev_reset>\n", __func__);
		goto out;
	}

	ret = of_property_read_u32(node, "phy_vendor_num", &usb_phy->phy_vendor_num);
	if (ret || usb_phy->phy_vendor_num == 0 ||
		usb_phy->phy_vendor_num >  UPC_MAX_VENDOR_PHY) {
		err("%s has error tag <phy_vendor_num>\n", __func__);
		goto out;
	}

	temp = devm_kzalloc(dev,
		(sizeof(struct upc_phy_vendor) * usb_phy->phy_vendor_num), GFP_KERNEL);
	if (temp == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	usb_phy->vendor = temp;

	ret = usb_phy_get_dts_cfg(usb_phy, dev, node);
	if (ret) {
		err("%s get dts config error, please check dts\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_no_callbacks(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		err("pm_runtime_get_sync failed\n");
		goto err_pm_put;
	}

	pm_runtime_forbid(dev);

	phy = devm_phy_create(dev, NULL, &upc_usb_phy_ops);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		goto err_pm_allow;
	}

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		ret = PTR_ERR(phy_provider);
		goto err_pm_allow;
	}

	phy_set_drvdata(phy, usb_phy);

	platform_set_drvdata(pdev, usb_phy);

	g_upc_usb_phy = usb_phy;

#ifdef CONFIG_USB_PHY_CHIP_FIX_BC_DETECT
	phy_power_init(dev);
#endif

	dbg("%s --\n", __func__);
	return 0;

err_pm_allow:
	pm_runtime_allow(dev);
err_pm_put:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
out:
	dbg("%s -- ret %d\n", __func__, ret);
	return ret;
}

static int usb_phy_remove(struct platform_device *pdev)
{
	dbg("%s ++\n", __func__);
	pm_runtime_allow(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	platform_set_drvdata(pdev, NULL);

#ifdef CONFIG_USB_PHY_CHIP_FIX_BC_DETECT
	if (g_upc_phy_power) {
		free_irq(g_upc_phy_power->vbus_connect_irq, g_upc_phy_power);
		free_irq(g_upc_phy_power->vbus_disconnect_irq, g_upc_phy_power);
		if (g_upc_phy_power->nb.notifier_call)
			power_event_bnc_unregister(POWER_BNT_WD, &g_upc_phy_power->nb);
		wakeup_source_unregister(g_upc_phy_power->vbus_wakelock);
		g_upc_phy_power->vbus_wakelock = NULL;
		g_upc_phy_power = NULL;
	}
#endif

	dbg("%s --\n", __func__);
	return 0;
}

static const struct of_device_id usb_phy_of_match[] = {
	{ .compatible = "hisilicon,usb_phy_device", },
	{},
};

MODULE_DEVICE_TABLE(of, usb_phy_of_match);


static struct platform_driver usb_phy_driver = {
	.probe = usb_phy_probe,
	.remove = usb_phy_remove,
	.driver = {
			   .name = "usb_phy_device",
			   .owner = THIS_MODULE,
			   .of_match_table = of_match_ptr(usb_phy_of_match),
		},
};

module_platform_driver(usb_phy_driver);

MODULE_AUTHOR("Xiaodong Guo <guoxiaodong2@huawei.com>");
MODULE_DESCRIPTION("usb phy driver");
MODULE_LICENSE("GPL");
