/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300_eng.c
 *
 * ns3300 driver for factory or debug
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

#include <linux/cpu.h>
#include <linux/cpumask.h>
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
#include <linux/pm_qos.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <huawei_platform/power/power_mesg_srv.h>
#include <chipset_common/hwpower/battery/battery_type_identify.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>

#include "ns3300.h"
#include "platform/board.h"
#include "swi/bsp_swi.h"
#include "swi/ns3300_swi.h"
#include "auth/ns3300_auth.h"
#include "ns3300_eng.h"

#define NS3300_SN_BIN_LEN              12
#define NS3300_SN_ASC_LEN              16
#define NS3300_NV_LOCK_CNT             2
#define NS3300_NV_LOCK0_MASK           0xFF
#define NS3300_ACT_ADDR                ((SWI_USER_DATA) + 0)
#define NS3300_SN_ADDR                 ((SWI_USER_DATA) + 0x64)
#define NS3300_SN_HEXASC_MASK          0xfb80
#define LOW4BITS_MASK                  0x0f
#define HIGH4BITS_MASK                 0xf0
#define ODD_MASK                       0x0001
#define BIT_P_BYT                      8
/* CRC */
#define CRC16_INIT_VAL                 0xffff
#define NS3300_WAIT_CPU_SWITCH_DONE    10 /* 10 ms */
#define NS3300_CORE_NUM                8
#define NS3300_DEBUG_LOCK_KEY          331131
#define CMD_DELAY_10MS                 10000

enum ns3300_sysfs_type {
	NS3300_SYSFS_BEGIN = 0,
	/* read only */
	NS3300_SYSFS_ECCE,
	/* read and write */
	NS3300_SYSFS_DATA,
	NS3300_SYSFS_ADDR,
	NS3300_SYSFS_DEBUG_LOCK,
	NS3300_SYSFS_END,
	NS3300_SYSFS_USER_PAGE_LOCK,
};

static struct ns3300_dev *g_ns3300_eng_di;
static struct pm_qos_request g_pm_qos;
static unsigned long g_irq_flags;
static u32 g_debug_key;
static u32 g_debug_addr;

static int ns3300_cpu_plug(bool plug, unsigned int cpu)
{
	int cnt = 20; /* retry count for cpu plug */
	int ret;

	do {
		if (plug)
			ret = cpu_up(cpu);
		else
			ret = cpu_down(cpu);

		if (ret == -EBUSY) {
			hwlog_info("cpu %d busy\n", cpu);
			usleep_range(5000, 5100); /* delay 5ms for cpu ready */
		} else {
			return ret;
		}
	} while (--cnt > 0);

	return ret;
}

static int ns3300_cpu_single_core_set(void)
{
	int cpu_id;
	int num_cpu_on;
	int ret;

	num_cpu_on = num_online_cpus();
	if (num_cpu_on == 1) {
		hwlog_info("[%s] only one core online\n", __func__);
		return 0;
	}

	for (cpu_id = 1; cpu_id < NS3300_CORE_NUM; cpu_id++) {
		ret = ns3300_cpu_plug(false, cpu_id);
		if (ret)
			hwlog_err("cpu_down fail, id %d : ret %d\n",
				cpu_id, ret);
	}
	msleep(NS3300_WAIT_CPU_SWITCH_DONE);
	num_cpu_on = num_online_cpus();
	if (num_cpu_on != 1) {
		hwlog_err("[%s] cpu_down fail, num_cpu_on : %d\n", __func__,
			num_cpu_on);
		return -EINVAL;
	}
	hwlog_info("[%s] %d cores online\n", __func__, num_cpu_on);

	return 0;
}

static void ns3300_turn_on_all_cpu_cores(void)
{
	int cpu_id;
	int num_cpu_on;
	int ret;

	num_cpu_on = num_online_cpus();
	if (num_cpu_on == NS3300_CORE_NUM) {
		hwlog_info("[%s] all cores online\n", __func__);
		return;
	}

	for (cpu_id = 1; cpu_id < NS3300_CORE_NUM; cpu_id++) {
		ret = ns3300_cpu_plug(true, cpu_id);
		if (ret)
			hwlog_err("cpu_up fail, id %d : ret %d\n",
					cpu_id, ret);
	}
	msleep(NS3300_WAIT_CPU_SWITCH_DONE);
	num_cpu_on = num_online_cpus();
	if (num_cpu_on != NS3300_CORE_NUM)
		hwlog_err("[%s] cpu_up fail, num_cpu_on : %d\n", __func__,
				num_cpu_on);
	else
		hwlog_info("[%s] %d cores online\n", __func__, num_cpu_on);
}

static void ns3300_dev_on(struct ns3300_dev *di)
{
	pm_qos_add_request(&g_pm_qos, PM_QOS_CPU_DMA_LATENCY, 0);
	kick_all_cpus_sync();
	if (di->sysfs_mode)
		bat_type_apply_mode(BAT_ID_SN);

	swi_power_down();
	swi_power_up();
}

static void ns3300_dev_off(struct ns3300_dev *di)
{
	swi_cmd_power_down();
	hwlog_info("power off by command\n");
	if (di->sysfs_mode)
		bat_type_release_mode(false);
	pm_qos_remove_request(&g_pm_qos);
}

void ns3300_dev_lock(void)
{
	hwlog_info("[%s]\n", __func__);
	spin_lock_irqsave(&g_ns3300_eng_di->onewire_lock, g_irq_flags);
}

void ns3300_dev_unlock(void)
{
	hwlog_info("[%s]\n", __func__);
	spin_unlock_irqrestore(&g_ns3300_eng_di->onewire_lock, g_irq_flags);
}

#define ns3300_sysfs_fld(_name, n, m, show, store) \
{ \
	.attr = __ATTR(_name, m, show, store), \
	.name = NS3300_SYSFS_##n, \
}

#define ns3300_sysfs_fld_ro(_name, n) \
	ns3300_sysfs_fld(_name, n, 0444, ns3300_sysfs_show, NULL)

#define ns3300_sysfs_fld_rw(_name, n) \
	ns3300_sysfs_fld(_name, n, 0644, ns3300_sysfs_show, \
			ns3300_sysfs_store)

#define ns3300_sysfs_fld_wo(_name, n) \
	ns3300_sysfs_fld(_name, n, 0200, NULL, ns3300_sysfs_store)

struct ns3300_sysfs_fld_info {
	struct device_attribute attr;
	uint8_t name;
};

static ssize_t ns3300_sysfs_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t ns3300_sysfs_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static struct ns3300_sysfs_fld_info ns3300_sysfs_fld_tbl[] = {
	ns3300_sysfs_fld_ro(ecce, ECCE),
	ns3300_sysfs_fld_rw(data, DATA),
	ns3300_sysfs_fld_rw(addr, ADDR),
	ns3300_sysfs_fld_rw(debug_lock, DEBUG_LOCK),
	ns3300_sysfs_fld_ro(user_page1_8_lock, USER_PAGE_LOCK),
};

#define NS3300_SYSFS_ATTRS_SIZE (ARRAY_SIZE(ns3300_sysfs_fld_tbl) + 1)

static struct attribute *ns3300_sysfs_attrs[NS3300_SYSFS_ATTRS_SIZE];

static const struct attribute_group ns3300_sysfs_attr_group = {
	.attrs = ns3300_sysfs_attrs,
};

static struct ns3300_sysfs_fld_info *ns3300_sysfs_fld_lookup(
		const char *name)
{
	int s;
	int e = ARRAY_SIZE(ns3300_sysfs_fld_tbl);

	for (s = 0; s < e; s++) {
		if (!strncmp(name, ns3300_sysfs_fld_tbl[s].attr.attr.name,
					strlen(name)))
			break;
	}

	if (s >= e)
		return NULL;

	return &ns3300_sysfs_fld_tbl[s];
}

static bool ns3300_nv_lock_status(struct ns3300_dev *di, uint8_t *status)
{
	return swi_cmd_read_page_lock(status, 0);
}

static int ns3300_sysfs_read_reg(struct ns3300_dev *di, uint8_t *test_val)
{
	int ret;

	if (g_debug_key != NS3300_DEBUG_LOCK_KEY) {
		hwlog_info("[%s] debug_key not matched %d\n", __func__, g_debug_key);
		di->sysfs_mode = false;
		return -EINVAL;
	}

	if (ns3300_cpu_single_core_set())
		hwlog_err("%s: single core set fail\n", __func__);
	ns3300_dev_on(di);
	ret = swi_send_rra_rd(g_debug_addr, test_val);
	ns3300_dev_off(di);
	ns3300_turn_on_all_cpu_cores();
	hwlog_info("[%s] NS3300_SYSFS_DATA = 0x%x, ret = %d\n", __func__, *test_val, ret);
	return ret;
}

static int ns3300_sysfs_do_ecc(struct ns3300_dev *di)
{
	int ret;

	if (g_debug_key != NS3300_DEBUG_LOCK_KEY) {
		hwlog_info("[%s] debug_key not matched %d\n",
			__func__, g_debug_key);
		di->sysfs_mode = false;
		return -EINVAL;
	}
	if (ns3300_cpu_single_core_set())
		hwlog_err("%s: single core set fail\n", __func__);
	ns3300_dev_on(di);
	ret = ns3300_do_authentication();
	ns3300_dev_off(di);
	ns3300_turn_on_all_cpu_cores();
	return ret;
}

static int ns3300_sysfs_read_pglk(struct ns3300_dev *di, uint8_t *status)
{
	if (g_debug_key != NS3300_DEBUG_LOCK_KEY) {
		hwlog_info("[%s] debug_key not matched %d\n",
			__func__, g_debug_key);
		di->sysfs_mode = false;
		return -EINVAL;
	}
	if (!ns3300_nv_lock_status(di, status)) {
		hwlog_err("[%s] read pglk fail\n");
		return -EINVAL;
	}

	return 0;
}

static ssize_t ns3300_sysfs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	int str_len = 0;
	struct ns3300_dev *di = g_ns3300_eng_di;
	struct ns3300_sysfs_fld_info *info = NULL;
	uint8_t test_val = 0;

	info = ns3300_sysfs_fld_lookup(attr->attr.name);
	if (!info || !di)
		return -EINVAL;

	di->sysfs_mode = true;
	switch (info->name) {
	case NS3300_SYSFS_ECCE:
		ret = ns3300_sysfs_do_ecc(di);
		str_len = scnprintf(buf, PAGE_SIZE, "NS3300_SYSFS_ECCE ret = %d\n", ret);
		hwlog_info("[%s] NS3300_SYSFS_ECCE %d\ntrue=%d\n", __func__, ret, true);
		break;
	case NS3300_SYSFS_DEBUG_LOCK:
		str_len = scnprintf(buf, PAGE_SIZE, "NS3300_SYSFS_DEBUG_LOCK = %d\n", g_debug_key);
		break;
	case NS3300_SYSFS_ADDR:
		str_len = scnprintf(buf, PAGE_SIZE, "NS3300_SYSFS_ADDR = 0x%x\n", g_debug_addr);
		break;
	case NS3300_SYSFS_DATA:
		ret = ns3300_sysfs_read_reg(di, &test_val);
		str_len = scnprintf(buf, PAGE_SIZE, "NS3300_SYSFS_DATA = 0x%x ret = %d\n", test_val, ret);
		break;
	case NS3300_SYSFS_USER_PAGE_LOCK:
		hwlog_info("NS3300_SYSFS_USER_PAGE_LOCK\n");
		ret = ns3300_sysfs_read_pglk(di, &test_val);
		if (!ret)
			str_len = scnprintf(buf, PAGE_SIZE, "pglk:0x%x\n", test_val);
		else
			str_len = scnprintf(buf, PAGE_SIZE, "read pglk fail\n");
		break;
	default:
		break;
	}

	di->sysfs_mode = false;
	return str_len;
}

static ssize_t ns3300_sysfs_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ns3300_sysfs_fld_info *info = NULL;
	struct ns3300_dev *di = g_ns3300_eng_di;
	u32 val = 0;

	info = ns3300_sysfs_fld_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;
	di->sysfs_mode = true;
	switch (info->name) {
	case NS3300_SYSFS_DEBUG_LOCK:
		kstrtou32(buf, 10, &g_debug_key);
		hwlog_info("[%s] NS3300_SYSFS_DEBUG_LOCK %d\n",
			__func__, g_debug_key);
		break;
	case NS3300_SYSFS_ADDR:
		kstrtou32(buf, 16, &g_debug_addr);
		hwlog_info("[%s] NS3300_SYSFS_ADDR 0x%x\n",
			__func__, g_debug_addr);
		break;
	case NS3300_SYSFS_DATA:
		if (g_debug_key != NS3300_DEBUG_LOCK_KEY) {
			hwlog_info("[%s] debug_key not matched %d\n",
				__func__, g_debug_key);
			di->sysfs_mode = false;
			return -EINVAL;
		}
		kstrtou32(buf, 16, &val);
		hwlog_info("[%s] write 0x%x to 0x%x\n",
			__func__, val, g_debug_addr);
		if (ns3300_cpu_single_core_set())
			hwlog_err("%s: single core set fail\n", __func__);
		ns3300_dev_on(g_ns3300_eng_di);
		swi_write_byte(g_debug_addr, val);
		ns3300_udelay(CMD_DELAY_10MS);
		ns3300_dev_off(g_ns3300_eng_di);
		ns3300_turn_on_all_cpu_cores();
		break;
	default:
		break;
	}
	di->sysfs_mode = false;
	return count;
}

static void ns3300_sysfs_init_attrs(void)
{
	int s;
	int e = ARRAY_SIZE(ns3300_sysfs_fld_tbl);

	for (s = 0; s < e; s++)
		ns3300_sysfs_attrs[s] = &ns3300_sysfs_fld_tbl[s].attr.attr;

	ns3300_sysfs_attrs[e] = NULL;
}

static int ns3300_sysfs_create(struct ns3300_dev *di)
{
	ns3300_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &ns3300_sysfs_attr_group);
}

int ns3300_eng_init(struct ns3300_dev *di)
{
	int ret;

	if (!di)
		return -EINVAL;

	ret = swi_init(di);
	if (ret) {
		hwlog_err("%s: swi init fail\n", __func__);
		return ret;
	}

	ret = ns3300_sysfs_create(di);
	if (ret) {
		hwlog_err("%s: sysfs create fail\n", __func__);
		return ret;
	}

	spin_lock_init(&di->onewire_lock);
	g_ns3300_eng_di = di;
	return 0;
}

int ns3300_eng_deinit(struct ns3300_dev *di)
{
	return 0;
}

static int ns3300_act_write(struct ns3300_dev *di, uint8_t *act, unsigned int act_len)
{
	uint8_t r_act[NS3300_ACT_LEN] = { 0 };

	if (!swi_write_user(NS3300_ACT_ADDR, act_len, act)) {
		hwlog_err("%s: write fail\n", __func__);
		return -EINVAL;
	}

	if (!swi_read_space(NS3300_ACT_ADDR, NS3300_ACT_LEN, r_act)) {
		hwlog_err("%s: read fail\n", __func__);
		return -EINVAL;
	}

	if (memcmp(r_act, act, NS3300_ACT_LEN)) {
		hwlog_err("%s: cmp fail\n", __func__);
		return -EINVAL;
	}

	di->mem.act_sig_ready = false;
	return 0;
}

static void ns3300_crc16_cal(uint8_t *msg, int len, uint16_t *crc)
{
	int i, j;
	uint16_t crc_mul = 0xA001; /* G(x) = x ^ 16 + x ^ 15 + x ^ 2 + 1 */

	*crc = CRC16_INIT_VAL;
	for (i = 0; i < len; i++) {
		*crc ^= *(msg++);
		for (j = 0; j < BIT_P_BYT; j++) {
			if (*crc & ODD_MASK)
				*crc = (*crc >> 1) ^ crc_mul;
			else
				*crc >>= 1;
		}
	}
}

static int ns3300_eng_write_act(struct ns3300_dev *di,
		enum res_type type, const struct power_genl_attr *res)
{
	uint8_t act[NS3300_ACT_LEN] = { 0 };
	uint16_t crc_act;
	int ret = 0;

	memcpy(act, res->data, res->len);

	/* attach crc16 suffix */
	ns3300_crc16_cal(act, res->len, &crc_act);
	memcpy(act + sizeof(act) - sizeof(crc_act), (uint8_t *)&crc_act,
		sizeof(crc_act));

	switch (type) {
	case RES_ACT:
		ret = ns3300_act_write(di, act, sizeof(act));
		if (ret) {
			hwlog_err("%s: act write fail\n", __func__);
			ret = -EIO;
		}
		di->mem.act_sig_ready = false;
		break;
	default:
		hwlog_err("%s: invalid option\n", __func__);
		ret = -EINVAL;
	}
	return ret;
}

int ns3300_eng_set_act_signature(struct ns3300_dev *di,
		enum res_type type, const struct power_genl_attr *res)
{
	int ret;
	uint8_t lk_status = 0;

	if (!power_cmdline_is_factory_mode())
		return 0;

	if (!di || !res) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (res->len > NS3300_ACT_LEN) {
		hwlog_err("%s: input act_sig oversize\n", __func__);
		return -EINVAL;
	}

	ret = ns3300_cpu_single_core_set();
	if (ret) {
		hwlog_err("%s: single core set fail\n", __func__);
		goto cpu_single_fail;
	}
	ns3300_dev_on(di);

	if (!ns3300_nv_lock_status(di, &lk_status)) {
		hwlog_err("%s: pglk iqr fail\n", __func__);
		ret = -EIO;
		goto act_sig_out;
	}

	if (lk_status) {
		hwlog_err("%s: smpg locked, act set abandon\n", __func__);
		ret = 0;
		goto act_sig_out;
	}

	ret = ns3300_eng_write_act(di, type, res);
act_sig_out:
	ns3300_dev_off(di);
cpu_single_fail:
	ns3300_turn_on_all_cpu_cores();
	return ret;
}

int ns3300_eng_act_read(struct ns3300_dev *di)
{
	uint16_t crc_act_read;
	uint16_t crc_act_cal = 0;
	int ret;

	if (!di) {
		hwlog_err("%s: pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (!power_cmdline_is_factory_mode())
		return -1;

	ret = ns3300_cpu_single_core_set();
	if (ret) {
		hwlog_err("%s: single core set fail\n", __func__);
		goto cpu_single_fail;
	}
	ns3300_dev_on(di);

	if (!swi_read_space(NS3300_ACT_ADDR, NS3300_ACT_LEN, di->mem.act_sign)) {
		hwlog_err("%s act read fail\n", __func__);
		ret = -EIO;
		goto act_sig_out;
	}

	memcpy((u8 *)&crc_act_read,
			&di->mem.act_sign[NS3300_ACT_LEN - sizeof(crc_act_read)],
			sizeof(crc_act_read));
	ns3300_crc16_cal(di->mem.act_sign,
			(int)(di->mem.act_sign[0] + 1), &crc_act_cal);
	if (crc_act_cal != crc_act_read) {
		hwlog_err("%s act crc fail\n", __func__);
		ret = -EINVAL;
		goto act_sig_out;
	}
	ret = 0;
	di->mem.act_sig_ready = true;
act_sig_out:
	ns3300_dev_off(di);
cpu_single_fail:
	ns3300_turn_on_all_cpu_cores();
	return ret;
}

static void ns3300_bin2prt(uint8_t *sn_print, uint8_t *sn_bin, int sn_print_size)
{
	uint8_t hex;
	uint8_t byt_cur;
	uint8_t byt_cur_bin = 0;
	uint8_t bit_cur = 0;

	for (byt_cur = 0; byt_cur < NS3300_SN_ASC_LEN; byt_cur++) {
		if (!(NS3300_SN_HEXASC_MASK & BIT(byt_cur))) {
			if (!(bit_cur % BIT_P_BYT))
				sn_print[byt_cur] = sn_bin[byt_cur_bin];
			else
				sn_print[byt_cur] =
					((sn_bin[byt_cur_bin] & LOW4BITS_MASK) << 4) |
					((sn_bin[byt_cur_bin + 1] & HIGH4BITS_MASK) >> 4);
			bit_cur += BIT_P_BYT;
			byt_cur_bin += 1;
		} else {
			if (bit_cur % BIT_P_BYT == 0) {
				hex = (sn_bin[byt_cur_bin] & HIGH4BITS_MASK) >> 4;
			} else {
				hex = sn_bin[byt_cur_bin] & LOW4BITS_MASK;
				byt_cur_bin += 1;
			}
			snprintf(sn_print + byt_cur, sn_print_size - byt_cur, "%X", hex);
			bit_cur += BIT_P_BYT / 2; /* bit index in current byte */
		}
	}
}

int ns3300_eng_get_batt_sn(struct ns3300_dev *di)
{
	int ret;
	uint8_t sn_bin[NS3300_SN_BIN_LEN] = { 0 };
	/* extra 1 bytes for \0 */
	char sn_print[NS3300_SN_ASC_LEN + 1] = { 0 };

	if (!di) {
		hwlog_err("[%s] pointer NULL\n", __func__);
		return -EINVAL;
	}

	if (!power_cmdline_is_factory_mode())
		return -1;

	ret = ns3300_cpu_single_core_set();
	if (ret) {
		hwlog_err("%s: single core set fail\n", __func__);
		goto cpu_single_fail;
	}
	ns3300_dev_on(di);

	if (!swi_read_space(NS3300_SN_ADDR, NS3300_SN_BIN_LEN, sn_bin)) {
		hwlog_err("%s act read fail\n", __func__);
		ret = -EIO;
		goto act_sig_out;
	}

	ns3300_bin2prt(sn_print, sn_bin, NS3300_SN_ASC_LEN + 1);
	memcpy(di->mem.sn, sn_print, NS3300_SN_ASC_LEN);
	di->mem.sn_ready = true;
	ret = 0;
act_sig_out:
	ns3300_dev_off(di);
cpu_single_fail:
	ns3300_turn_on_all_cpu_cores();
	return ret;
}

int ns3300_eng_set_batt_as_org(struct ns3300_dev *di)
{
	int ret;

	if (!di)
		return -EINVAL;

	if (!power_cmdline_is_factory_mode())
		return 0;

	ret = ns3300_cpu_single_core_set();
	if (ret) {
		hwlog_err("%s: single core set fail\n", __func__);
		goto org_set_fail;
	}

	ns3300_dev_on(di);
	if (!swi_cmd_lock_cnt()) {
		ret = -EIO;
		hwlog_err("%s: set_batt_as_org fail\n", __func__);
	} else {
		ret = 0;
		di->mem.source = BATTERY_ORIGINAL;
	}
	ns3300_dev_off(di);
org_set_fail:
	ns3300_turn_on_all_cpu_cores();
	return ret;
}

int ns3300_eng_set_cert_ready(struct ns3300_dev *di)
{
	int ret;
	uint8_t nv_info[NS3300_NV_LOCK_CNT] = { NS3300_NV_LOCK0_MASK, 0 };

	if (!di)
		return -EINVAL;

	if (!power_cmdline_is_factory_mode())
		return 0;

	ret = ns3300_cpu_single_core_set();
	if (ret) {
		hwlog_err("%s: single core set fail\n", __func__);
		goto cert_ready_set_fail;
	}

	ns3300_dev_on(di);
	if (!swi_cmd_lock_user(nv_info, NS3300_NV_LOCK_CNT)) {
		ret = -EIO;
		hwlog_err("%s: set_cert_ready fail\n", __func__);
	} else {
		ret = 0;
		di->mem.cet_rdy = CERT_READY;
	}
	ns3300_dev_off(di);
cert_ready_set_fail:
	ns3300_turn_on_all_cpu_cores();
	return ret;
}
