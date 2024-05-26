/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300.c
 *
 * ns3300 driver
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/power_mesg_srv.h>
#include <chipset_common/hwpower/battery/battery_type_identify.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>

#include "ns3300.h"
#include "ns3300_eng.h"
#include "platform/board.h"
#include "swi/bsp_swi.h"
#include "swi/ns3300_swi.h"
#include "auth/ns3300_auth.h"

static struct ns3300_dev *g_ns3300_di;
static struct batt_ct_ops_list g_ns3300_ctlist_node;

static int ns3300_ic_ck(struct ns3300_dev *di)
{
	struct batt_ic_para *ic_para = batt_get_ic_para();

	if (!di)
		return -ENODEV;

	if (!ic_para) {
		hwlog_err("%s: ic_para NULL\n", __func__);
		return -EINVAL;
	}

	if (ic_para->ic_type != NATIONSTECH_NS3300_TYPE)
		return -EINVAL;

	di->mem.ic_type = (enum batt_ic_type)ic_para->ic_type;

	memcpy(di->mem.uid, ic_para->uid, NS3300_UID_LEN);
	di->mem.uid_ready = true;
	return 0;
}

static int ns3300_batt_name_check(void)
{
	char *batt_name = batt_get_batt_name();

	if (!batt_name) {
		hwlog_info("%s: no ic name\n", __func__);
		return 0;
	}
	if (strncmp(batt_name, NS3300_IC_NAME, strlen(NS3300_IC_NAME))) {
		hwlog_err("%s: ic error\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static void ns3300_batt_info_parse(struct ns3300_dev *di)
{
	uint8_t info0[sizeof(struct batt_info_para)] = { 0 };
	uint8_t batt_type[NS3300_BATTTYP_LEN] = { 0 };
	struct batt_info_para *info = batt_get_info_para();

	if (!di)
		return;

	if (!info) {
		hwlog_err("%s: batt info NULL\n", __func__);
		di->mem.source = BATTERY_UNKNOWN;
		return;
	}

	if (!memcmp(info0, (uint8_t *)info, sizeof(struct batt_info_para))) {
		hwlog_err("%s: batt info all zero\n", __func__);
		di->mem.source = BATTERY_UNKNOWN;
		return;
	}

	di->mem.cet_rdy = CERT_READY;

	memcpy(batt_type, info->type, NS3300_BATTTYP_LEN);
	di->mem.batt_type[0] = batt_type[1];
	di->mem.batt_type[1] = batt_type[0];
	di->mem.batttp_ready = true;

	di->mem.source = info->source;

	memcpy(di->mem.sn, info->sn, NS3300_SN_ASC_LEN);
	if ((di->mem.sn[0] == ASCII_0) && (strlen(info->sn) == 1)) {
		hwlog_err("%s: no sn info\n", __func__);
		di->mem.sn_ready = false;
	} else {
		di->mem.sn_ready = true;
	}
}

static void ns3300_cert_info_parse(struct ns3300_dev *di)
{
	uint8_t cert0[sizeof(struct batt_cert_para)] = { 0 };
	struct batt_cert_para *cert = batt_get_cert_para();

	if (!di)
		return;

	if (!cert) {
		hwlog_err("%s: cert info NULL\n", __func__);
		return;
	}

	if (!memcmp(cert0, (uint8_t *)cert, sizeof(struct batt_cert_para))) {
		hwlog_err("%s: cert signature all zero\n", __func__);
		return;
	}

	if (cert->key_result == KEY_PASS)
		di->mem.ecce_pass = true;
	else
		return;

	memcpy(di->mem.act_sign, cert->signature, NS3300_ACT_LEN);
	di->mem.act_sig_ready = true;
}

static enum batt_ic_type ns3300_get_ic_type(void)
{
	struct ns3300_dev *di = g_ns3300_di;

	if (!di)
		return LOCAL_IC_TYPE;

	return di->mem.ic_type;
}

static int ns3300_get_uid(struct platform_device *pdev,
	const unsigned char **uuid, unsigned int *uuid_len)
{
	struct ns3300_dev *di = g_ns3300_di;

	if (!di)
		return -ENODEV;

	if (!uuid || !uuid_len || !pdev) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (!di->mem.uid_ready) {
		hwlog_err("%s: no uid available\n", __func__);
		return -EINVAL;
	}

	*uuid = di->mem.uid;
	*uuid_len = NS3300_UID_LEN;
	return 0;
}

static int ns3300_get_batt_type(struct platform_device *pdev,
	const unsigned char **type, unsigned int *type_len)
{
	struct ns3300_dev *di = g_ns3300_di;

	if (!di)
		return -ENODEV;

	if (!pdev || !type || !type_len) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (!di->mem.batttp_ready) {
		hwlog_err("%s: no batt_type info available\n", __func__);
		return -EINVAL;
	}

	*type = di->mem.batt_type;
	*type_len = NS3300_BATTTYP_LEN;
	hwlog_info("[%s]Btp0:0x%x; Btp1:0x%x\n", __func__, di->mem.batt_type[0],
		di->mem.batt_type[1]);

	return 0;
}

static int ns3300_get_batt_sn(struct platform_device *pdev,
	struct power_genl_attr *res, const unsigned char **sn,
	unsigned int *sn_size)
{
	struct ns3300_dev *di = g_ns3300_di;

	if (!di)
		return -EINVAL;

	if (!pdev || !sn || !sn_size) {
		hwlog_err("[%s] pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (!di->mem.sn_ready) {
		if (ns3300_eng_get_batt_sn(di)) {
			hwlog_info("[%s] operation banned in user mode\n", __func__);
			return -EINVAL;
		}
	}

	hwlog_info("[%s] sn in mem already\n", __func__);
	*sn = di->mem.sn;
	*sn_size = NS3300_SN_ASC_LEN;
	return 0;
}

static int ns3300_certification(struct platform_device *pdev,
	struct power_genl_attr *res, enum key_cr *result)
{
	struct ns3300_dev *di = g_ns3300_di;

	if (!di)
		return -ENODEV;

	if (!pdev || !res || !result) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (!di->mem.ecce_pass) {
		hwlog_err("%s: ecce_pass %d\n", __func__, di->mem.ecce_pass);
		*result = KEY_FAIL_UNMATCH;
		return 0;
	}

	*result = KEY_PASS;
	return 0;
}

static int ns3300_set_act_signature(struct platform_device *pdev,
	enum res_type type, const struct power_genl_attr *res)
{
	return ns3300_eng_set_act_signature(g_ns3300_di, type, res);
}

static inline int ns3300_act_read(struct ns3300_dev *di)
{
	if (di->mem.act_sig_ready)
		return 0;
	return ns3300_eng_act_read(di);
}

static int ns3300_prepare(struct platform_device *pdev, enum res_type type,
	struct power_genl_attr *res)
{
	struct ns3300_dev *di = NULL;

	di = platform_get_drvdata(pdev);
	if (!di)
		return -ENODEV;

	if (!pdev || !res) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	switch (type) {
	case RES_CT:
		if (!di->mem.uid_ready) {
			hwlog_err("%s: res_ct read fail\n", __func__);
			goto prepare_fail;
		}
		res->data = (const unsigned char *)di->mem.uid;
		res->len = NS3300_UID_LEN;
		break;
	case RES_ACT:
		if (ns3300_act_read(di)) {
			hwlog_err("%s: res_act read fail\n", __func__);
			goto prepare_fail;
		}
		res->data = (const unsigned char *)di->mem.act_sign;
		res->len = NS3300_ACT_LEN;
		break;
	default:
		hwlog_err("%s: invalid option\n", __func__);
		res->data = NULL;
		res->len = 0;
	}

	return 0;
prepare_fail:
	return -EINVAL;
}

static int ns3300_set_batt_as_org(struct ns3300_dev *di)
{
	return ns3300_eng_set_batt_as_org(di);
}

static int ns3300_set_cert_ready(struct ns3300_dev *di)
{
	return ns3300_eng_set_cert_ready(di);
}

static int ns3300_set_batt_safe_info(struct platform_device *pdev,
	enum batt_safe_info_t type, void *value)
{
	int ret;
	enum batt_source cell_type;
	struct ns3300_dev *di = NULL;

	if (!pdev || !value) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}
	di = platform_get_drvdata(pdev);
	if (!di)
		return -EINVAL;

	switch (type) {
	case BATT_CHARGE_CYCLES:
		break;
	case BATT_SPARE_PART:
		cell_type = di->mem.source;
		if (cell_type == BATTERY_ORIGINAL) {
			hwlog_info("[%s]: has been org, quit work\n", __func__);
			goto battinfo_set_succ;
		}

		if (*((enum batt_source *)value) == BATTERY_ORIGINAL) {
			ret = ns3300_set_batt_as_org(di);
			if (ret) {
				hwlog_err("%s: set_as_org fail\n", __func__);
				goto battinfo_set_fail;
			}
		}
		break;
	case BATT_CERT_READY:
		ret = ns3300_set_cert_ready(di);
		if (ret) {
			hwlog_err("%s: set_cert_ready fail\n", __func__);
			goto battinfo_set_fail;
		}
		di->mem.sn_ready = false;
		break;
	default:
		hwlog_err("%s: invalid option\n", __func__);
		goto battinfo_set_fail;
	}

battinfo_set_succ:
	return 0;
battinfo_set_fail:
	return -EINVAL;
}

static int ns3300_get_batt_safe_info(struct platform_device *pdev,
	enum batt_safe_info_t type, void *value)
{
	struct ns3300_dev *di = NULL;

	di = platform_get_drvdata(pdev);
	if (!di)
		return -ENODEV;

	if (!pdev || !value) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	switch (type) {
	case BATT_CHARGE_CYCLES:
		*(int *)value = BATT_INVALID_CYCLES;
		break;
	case BATT_SPARE_PART:
		*(enum batt_source *)value = di->mem.source;
		break;
	case BATT_CERT_READY:
		*(enum batt_cert_state *)value = di->mem.cet_rdy;
		break;
	default:
		hwlog_err("%s: invalid option\n", __func__);
		goto battinfo_get_fail;
	}

	return 0;
battinfo_get_fail:
	return -EINVAL;
}

static int ns3300_ct_ops_register(struct platform_device *pdev,
	struct batt_ct_ops *bco)
{
	int ret;
	struct ns3300_dev *di = NULL;

	if (!bco || !pdev) {
		hwlog_err("%s: bco/pdev NULL\n", __func__);
		return -ENXIO;
	}
	di = platform_get_drvdata(pdev);
	if (!di)
		return -EINVAL;

	ret = ns3300_ic_ck(di);
	if (ret) {
		hwlog_err("%s: ic unmatch\n", __func__);
		return -ENXIO;
	}

	hwlog_info("[%s]: ic matched\n", __func__);

	ns3300_batt_info_parse(di);
	ns3300_cert_info_parse(di);

	bco->get_ic_type = ns3300_get_ic_type;
	bco->get_ic_uuid = ns3300_get_uid;
	bco->get_batt_type = ns3300_get_batt_type;
	bco->get_batt_sn = ns3300_get_batt_sn;
	bco->certification = ns3300_certification;
	bco->set_act_signature = ns3300_set_act_signature;
	bco->prepare = ns3300_prepare;
	bco->set_batt_safe_info = ns3300_set_batt_safe_info;
	bco->get_batt_safe_info = ns3300_get_batt_safe_info;
	bco->power_down = NULL;
	return 0;
}

static int ns3300_parse_dts(struct ns3300_dev *di, struct device_node *np)
{
	int ret;

	ret = of_property_read_u32(np, "tau", &di->tau);
	if (ret) {
		hwlog_err("%s: tau not given in dts\n", __func__);
		di->tau = NS3300_DEFAULT_TAU;
	}
	hwlog_info("[%s] tau = %u\n", __func__, di->tau);
	ret = of_property_read_u16(np, "cyc_full", &di->cyc_full);
	if (ret) {
		hwlog_err("%s: cyc_full not given in dts\n", __func__);
		di->cyc_full = NS3300_DFT_FULL_CYC;
	}
	hwlog_info("[%s] cyc_full = 0x%x\n", __func__, di->cyc_full);
	return 0;
}
static int ns3300_gpio_init(struct platform_device *pdev,
	struct ns3300_dev *di)
{
	int ret;

	di->onewire_gpio = of_get_named_gpio(pdev->dev.of_node,
		"onewire-gpio", 0);

	if (!gpio_is_valid(di->onewire_gpio)) {
		hwlog_err("onewire_gpio is not valid\n");
		return -EINVAL;
	}
	/* get the gpio */
	ret = devm_gpio_request(&pdev->dev, di->onewire_gpio, "onewire_sle");
	if (ret) {
		hwlog_err("gpio request failed %d\n", di->onewire_gpio);
		return ret;
	}
	gpio_direction_output(di->onewire_gpio, HIGH_VOLTAGE);

	return 0;
}

static int ns3300_remove(struct platform_device *pdev)
{
	struct ns3300_dev *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;
	ns3300_eng_deinit(di);
	return 0;
}

static int ns3300_probe(struct platform_device *pdev)
{
	int ret;
	struct ns3300_dev *di = NULL;
	struct device_node *np = NULL;

	hwlog_info("[%s] probe begin\n", __func__);

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	ret = ns3300_batt_name_check();
	if (ret) {
		hwlog_err("%s: batt name unmatch\n", __func__);
		return ret;
	}
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	g_ns3300_di = di;

	di->dev = &pdev->dev;
	np = pdev->dev.of_node;

	ret = ns3300_parse_dts(di, np);
	if (ret) {
		hwlog_err("%s: dts parse fail\n", __func__);
		goto dts_parse_fail;
	}
	ret = ns3300_gpio_init(pdev, di);
	if (ret) {
		hwlog_err("%s: gpio init fail\n", __func__);
		goto gpio_init_fail;
	}

	ret = ns3300_eng_init(di);
	if (ret) {
		hwlog_err("%s: swi init fail\n", __func__);
		goto eng_init_fail;
	}

	g_ns3300_ctlist_node.ic_memory_release = NULL;
	g_ns3300_ctlist_node.ic_dev = pdev;
	g_ns3300_ctlist_node.ct_ops_register = ns3300_ct_ops_register;
	add_to_aut_ic_list(&g_ns3300_ctlist_node);

	platform_set_drvdata(pdev, di);
	hwlog_info("[%s] probe ok\n", __func__);
	return 0;
eng_init_fail:
gpio_init_fail:
dts_parse_fail:
	kfree(di);
	g_ns3300_di = NULL;
	return ret;
}

static const struct of_device_id ns3300_match_table[] = {
	{
		.compatible = "nationstech,ns3300",
		.data = NULL,
	},
	{},
};

static struct platform_driver ns3300_driver = {
	.probe = ns3300_probe,
	.remove = ns3300_remove,
	.driver = {
		.name = "nationstech,ns3300",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ns3300_match_table),
	},
};

static int __init ns3300_init(void)
{
	return platform_driver_register(&ns3300_driver);
}

static void __exit ns3300_exit(void)
{
	platform_driver_unregister(&ns3300_driver);
}

subsys_initcall_sync(ns3300_init);
module_exit(ns3300_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ns3300 ic");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
