/*
 * ipc_wake.c
 *
 * wake up source ipc name
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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

#include "pm.h"
#include "pub_def.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/register/register_ops.h"

#include <m3_rdr_ddr_map.h>
#ifdef CONFIG_POWER_DUBAI
#include <huawei_platform/power/dubai/dubai_wakeup_stats.h>
#endif

#include <lpmcu_runtime.h>
#include <soc_ipc_interface.h>
#include <linux/bits.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

enum ipc_type {
	PERI_NS_NORMAL_IPC = 0,
	AO_NS_IPC = 200,
	NPU_NS_IPC = 300,
	PERI_NS_CFG_IPC = 400
};

struct ipc_mgx_info {
	int index;
	bool used;
	const char *name;
};

struct wake_ipc_info {
	const char *desc;
	int cpunum;
	const char **cpuname;
	int mbxnum;
	struct ipc_mgx_info *mbxs;
	const int runtime_offset;
	const enum LP_REG_DOMAIN reg_domain;
};

static struct wake_ipc_info g_ns_ipc = {
	.desc = "nsipc",
	.runtime_offset = AP_NSIPC_IRQ_OFFSET,
	.reg_domain = IPC,
};

static struct wake_ipc_info g_ao_ns_ipc = {
	.desc = "ao nsipc",
	.runtime_offset = AP_AO_NSIPC_IRQ_OFFSET,
	.reg_domain = AO_IPC,
};

#define _runtime_read(offset) runtime_read(M3_RDR_SYS_CONTEXT_RUNTIME_VAR_OFFSET + (offset))

#define get_sharemem_data(m) ((m) & 0xFFFF)
#define get_sharemem_source(m) ((m) & 0xFF)
static void notify_dubai_sensor_wakeup(struct seq_file *s, const struct wake_ipc_info *ipc, const unsigned int mbx)
{
#ifdef CONFIG_POWER_DUBAI
	unsigned int ipc_data, source, mem;

	ipc_data = read_reg(ipc->reg_domain, SOC_IPC_MBX_DATA0_ADDR(0, mbx));
	mem = get_sharemem_data(ipc_data);

	ipc_data = read_reg(ipc->reg_domain, SOC_IPC_MBX_DATA2_ADDR(0, mbx));
	source = get_sharemem_source(ipc_data);

	dubai_log_wakeup_info("DUBAI_TAG_SENSORHUB_WAKEUP",
			      "mem=%u source=%u", mem, source);

#endif
}

static bool is_same_rproc(int cpuid, const struct wake_ipc_info *ipc_info, const char *rprocname)
{
	if (unlikely(ipc_info == NULL))
		return false;
	if (unlikely(cpuid >= ipc_info->cpunum))
		return false;
	if (unlikely(ipc_info->cpuname == NULL))
		return false;
	if (unlikely(ipc_info->cpuname[cpuid] == NULL))
		return false;

	return strncmp(ipc_info->cpuname[cpuid],
		       rprocname, strlen(rprocname)) == 0;
}
#define SENSOR_RPROC_NAME "SENSORHUB"
static bool is_sensor(int cpuid, const struct wake_ipc_info *ipc_info)
{
	return is_same_rproc(cpuid, ipc_info, SENSOR_RPROC_NAME);
}

#define IPC_MBXDATA_MAX 8
#define ACPU_RPROC_NAME "ACPU"
static bool is_acpu(int cpuid, const struct wake_ipc_info *ipc_info)
{
	return is_same_rproc(cpuid, ipc_info, ACPU_RPROC_NAME);
}

/*
 * The chip supports multipoint-to-multipoint communication,
 * but the software supports only point-to-point communication
 */
#define proc_mask_to_id(mask) first_setted_bit(mask)

#define ipc_mbx_data_offset(mbx, idex) SOC_IPC_MBX_DATA0_ADDR(4 * (idex), mbx)

static void ipc_mbx_info_show(struct seq_file *s, unsigned int mbx, const struct wake_ipc_info *ipc)
{
	int i, src_id, dest_id;
	unsigned int ipc_source, ipc_dest;
	const int cpunum = ipc->cpunum;

	lp_msg_cont(s, "mbx-%d:%s, ", mbx, ipc->mbxs[mbx].name);

	ipc_source = read_reg(ipc->reg_domain, SOC_IPC_MBX_SOURCE_ADDR(0, mbx));
	src_id = proc_mask_to_id(ipc_source);
	ipc_dest = read_reg(ipc->reg_domain, SOC_IPC_MBX_DSTATUS_ADDR(0, mbx));
	dest_id = proc_mask_to_id(ipc_dest);
	if (src_id >= cpunum || dest_id >= cpunum) {
		lp_msg_cont(s, "ipc cpu id invalid:src[%d]dst[%d]max[%d])\n", src_id, dest_id, cpunum);
		return;
	}

	if (is_acpu(src_id, ipc))
		lp_msg_cont(s, "ack from %s to %s. ", ipc->cpuname[dest_id], ipc->cpuname[src_id]);
	else
		lp_msg_cont(s, "from %s to %s. ", ipc->cpuname[src_id], ipc->cpuname[dest_id]);

	lp_msg_cont(s, "data:");
	for (i = 0; i < IPC_MBXDATA_MAX; i++)
		lp_msg_cont(s, "[0x%x]", read_reg(ipc->reg_domain, ipc_mbx_data_offset(mbx, i)));

	lp_msg_cont(s, "\n");

	if (is_sensor(src_id, ipc))
		notify_dubai_sensor_wakeup(s, ipc, mbx);
}

static void combo_ipc_irq_show(struct seq_file *s, const struct wake_ipc_info *ipc)
{
	int i;
	unsigned int ipc_state;

	ipc_state = _runtime_read(ipc->runtime_offset);

	lp_msg(s, "SR:%s begin", ipc->desc);
	lp_msg(s, "irq state:0x%x", ipc_state);

	for (i = 0; i < ipc->mbxnum; i++) {
		if ((ipc_state & BIT((unsigned int)ipc->mbxs[i].index)) == 0)
			continue;

		ipc_mbx_info_show(s, ipc->mbxs[i].index, ipc);
	}
	lp_msg(s, "SR:%s end", ipc->desc);
}

void show_wake_ipc(struct seq_file *s)
{
	int wake_irq;

	wake_irq = (int)_runtime_read(AP_WAKE_IRQ_OFFSET);
	if (wake_irq != IRQ_COMB_GIC_IPC)
		return;

	lp_msg(s, "SR: wakeup by ipc, information as follows:");
	combo_ipc_irq_show(s, &g_ns_ipc);
	combo_ipc_irq_show(s, &g_ao_ns_ipc);
}

static int read_ipc_rproc_info(struct device_node *ipc_dn, struct wake_ipc_info *ipc_info)
{
	ipc_info->cpuname = lp_read_dtsi_strings(ipc_dn, "rproc_name", "rproc_num", &ipc_info->cpunum);
	if (ipc_info->cpuname == NULL) {
		lp_err("read rproc name failed");
		return -ENOMEM;
	}
	return 0;
}

static void init_ipc_mbx_info(struct wake_ipc_info *ipc_info)
{
	int i;

	for (i = 0; i < ipc_info->mbxnum; i++) {
		ipc_info->mbxs[i].index = i;
		ipc_info->mbxs[i].used = false;
		ipc_info->mbxs[i].name = "reserved";
	}
}

static int read_ipc_mbx_info(struct device_node *ipc_dn, struct wake_ipc_info *ipc_info)
{
	int i, mbx_index_offset, mbx_index, used;
	struct device_node *mbx_dn = NULL;

	ipc_info->mbxnum = of_get_child_count(ipc_dn);
	if (ipc_info->mbxnum < 0) {
		lp_err("count mailbox num failed");
		return -ENOMEM;
	}
	ipc_info->mbxs = kcalloc(ipc_info->mbxnum, sizeof(struct ipc_mgx_info), GFP_KERNEL);
	if (ipc_info->mbxs == NULL)
		return -ENOMEM;
	init_ipc_mbx_info(ipc_info);

	mbx_index_offset = 0;
	if (of_property_read_u32(ipc_dn, "ipc_type", &mbx_index_offset) != 0 ||
	    mbx_index_offset < 0) {
		lp_err("read offset %d failed", mbx_index_offset);
		return -ENODEV;
	}

	used = mbx_index = i = 0;
	for_each_child_of_node(ipc_dn, mbx_dn) {
		if (of_property_read_s32(mbx_dn, "index", &mbx_index) != 0) {
			lp_err("read mbx index failed for mbx:%d", i);
			return -ENODEV;
		}
		mbx_index -= mbx_index_offset;
		if (mbx_index < 0 || mbx_index > ipc_info->mbxnum) {
			lp_err("child %d: invalid mbx index %d, max mbx is %d", i, mbx_index, ipc_info->mbxnum);
			return -EINVAL;
		}
		ipc_info->mbxs[i].index = mbx_index;

		if (of_property_read_s32(mbx_dn, "used", &used) != 0) {
			lp_err("child %d: read mbx used status failed for mbx:%d", i, mbx_index);
			return -ENODEV;
		}
		ipc_info->mbxs[i].used = (used == 1);

		if ((of_property_read_string(mbx_dn, "rproc", &(ipc_info->mbxs[mbx_index].name)) != 0) &&
		    ipc_info->mbxs[i].used) {
			lp_err("child %d: read mbx info failed for mbx:%d", i, mbx_index);
			return -ENODEV;
		}
		i++;
	}
	return 0;
}

static int init_ns_ipc_table(struct device_node *ipc_dn, struct wake_ipc_info *ipc_info)
{
	int ret;

	ret = read_ipc_rproc_info(ipc_dn, ipc_info);
	if (ret != 0) {
		lp_err("read rproc info failed %d", ret);
		return -ENODEV;
	}

	ret = read_ipc_mbx_info(ipc_dn, ipc_info);
	if (ret != 0) {
		lp_err("read mbx info failed %d", ret);
		return -ENODEV;
	}

	return 0;
}

const static struct of_device_id ipc_dev_match[] = {
	{ .compatible = "hisilicon,HiIPCV230", },
	{},
};

static int map_ipc_reg_base(void)
{
	if (map_reg_base(g_ns_ipc.reg_domain) != 0) {
		lp_err("map ns ipc reg failed");
		return -EFAULT;
	}
	if (map_reg_base(g_ao_ns_ipc.reg_domain) != 0) {
		lp_err("map ao ns ipc reg failed");
		return -EFAULT;
	}
	return 0;
}
int init_ipc_table(const struct device_node *lp_dn)
{
	int ipc_type, ret;
	struct device_node *dn = NULL;

	no_used(lp_dn);

	if (map_ipc_reg_base() != 0) {
		lp_err("map reg failed");
		return -EFAULT;
	}

	for_each_matching_node(dn, ipc_dev_match) {
		ret = of_property_read_u32(dn, "ipc_type", &ipc_type);
		if (ret != 0) {
			lp_err("read ipc_type failed %d", ret);
			return -ENODEV;
		}

		if (ipc_type == PERI_NS_NORMAL_IPC) {
			ret = init_ns_ipc_table(dn, &g_ns_ipc);
			if (ret != 0) {
				lp_err("init ns ipc failed %d", ret);
				return -ENODEV;
			}
			lp_info("init ns ipc success");
			continue;
		}

		if (ipc_type == AO_NS_IPC) {
			ret = init_ns_ipc_table(dn, &g_ao_ns_ipc);
			if (ret != 0) {
				lp_err("init ao ns ipc failed %d", ret);
				return -ENODEV;
			}
			lp_info("init ao ns ipc success");
			continue;
		}
	}

	return 0;
}

