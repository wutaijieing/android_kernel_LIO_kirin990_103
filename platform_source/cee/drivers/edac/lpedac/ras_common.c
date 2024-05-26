/*
 * ras_common.c
 *
 * RAS COMMON API
 *
 * Copyright (c) 2019-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <asm/page.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/sysctl.h>
#include <linux/timer.h>
#include "ras_common.h"

static bool is_bus_error(u64 status)
{
	if (ERR_STATUS_GET_FIELD(status, SERR) == BUS_ERROR)
		return true;

	return false;
}

static void ras_device_panic(struct irq_nodes *pdata, int inst_nr,
			     int type)
{
	int cpu;

	if (pdata->mod_id == SHARE_L2) {
		rdr_syserr_process_for_ap((u32)(MODID_AP_S_CPU01_L2_CE +
						inst_nr * 2 + type), 0ULL, 0ULL);
	} else if (pdata->mod_id == SHARE_L3) {
		rdr_syserr_process_for_ap((u32)(MODID_AP_S_L3_CE + type), 0ULL, 0ULL);
	} else {
		cpu = smp_processor_id();
		rdr_syserr_process_for_ap((u32)(MODID_AP_S_CPU0_CE + cpu * 2 + type),
					  0ULL, 0ULL);
	}
}

static bool ras_nr_check(struct edac_device_ctl_info *edac_dev,
		  int inst_nr, int block_nr)
{
	struct edac_device_instance *instance = NULL;

	if (((u32)inst_nr >= edac_dev->nr_instances) || (inst_nr < 0)) {
		edac_device_printk(edac_dev, KERN_ERR,
				   "INTERNAL ERROR: 'instance' out of range "
				   "(%d >= %d)\n", inst_nr,
				   edac_dev->nr_instances);
		return false;
	}

	instance = edac_dev->instances + inst_nr;
	if (((u32)block_nr >= instance->nr_blocks) || (block_nr < 0)) {
		edac_device_printk(edac_dev, KERN_ERR,
				   "INTERNAL ERROR: instance %d 'block' "
				   "out of range (%d >= %d)\n",
				   inst_nr, block_nr,
				   instance->nr_blocks);
		return false;
	}

	return true;
}

void ras_device_handle_ce(struct edac_device_ctl_info *edac_dev,
			  int inst_nr, int block_nr, const char *msg)
{
	struct edac_device_instance *instance = NULL;
	struct edac_device_block *block = NULL;
	struct err_record *err_data = NULL;
	struct irq_nodes *pdata = NULL;
	struct irq_node *irq_data = NULL;

	if (!ras_nr_check(edac_dev, inst_nr, block_nr))
		return;

	instance = edac_dev->instances + inst_nr;
	if (instance->nr_blocks > 0) {
		block = instance->blocks + block_nr;
		block->counters.ce_count++;
	}

	/* Propagate the count up the 'totals' tree */
	instance->counters.ce_count++;
	edac_dev->counters.ce_count++;

	if (edac_dev->log_ce)
		edac_device_printk(edac_dev, KERN_WARNING,
				   "CE: %s instance: %s block: %s '%s'\n",
				   edac_dev->ctl_name, instance->name,
				   block ? block->name : "N/A", msg);
	pdata = edac_dev->pvt_info;
	irq_data = pdata->irq_data + inst_nr;
	err_data = irq_data->err_record + block_nr;

	bbox_diaginfo_record(ECC_CE, NULL,
			     "EDAC %s: CE instance: %s block %s errxstatus=0x%llx, errxmisc=0x%llx",
			     edac_dev->ctl_name, instance->name, block ? block->name : "N/A",
			     err_data->errstatus, err_data->errmisc);

	if (edac_dev->panic_on_ue)
		ras_device_panic(pdata, inst_nr, (u32)CE);
}
EXPORT_SYMBOL_GPL(ras_device_handle_ce);

void ras_device_handle_de(struct edac_device_ctl_info *edac_dev,
			  int inst_nr, int block_nr, const char *msg)
{
	struct edac_device_instance *instance = NULL;
	struct edac_device_block *block = NULL;
	struct err_record *err_data = NULL;
	struct irq_nodes *pdata = NULL;
	struct irq_node *irq_data = NULL;

	if (!ras_nr_check(edac_dev, inst_nr, block_nr))
		return;

	instance = edac_dev->instances + inst_nr;
	if (instance->nr_blocks > 0)
		block = instance->blocks + block_nr;

	edac_device_printk(edac_dev, KERN_WARNING,
			   "DE: %s instance: %s block: %s '%s'\n",
			   edac_dev->ctl_name, instance->name,
			   block ? block->name : "N/A", msg);

	pdata = edac_dev->pvt_info;
	irq_data = pdata->irq_data + inst_nr;
	err_data = irq_data->err_record + block_nr;

	bbox_diaginfo_record(ECC_DE, NULL,
			     "EDAC %s: DE instance: %s block %s errxstatus=0x%llx, errxmisc=0x%llx",
			     edac_dev->ctl_name, instance->name, block ? block->name : "N/A",
			     err_data->errstatus, err_data->errmisc);
}
EXPORT_SYMBOL_GPL(ras_device_handle_de);

void ras_device_handle_ue(struct edac_device_ctl_info *edac_dev,
			  int inst_nr, int block_nr, const char *msg)
{
	struct edac_device_instance *instance = NULL;
	struct edac_device_block *block = NULL;
	struct err_record *err_data = NULL;
	struct irq_nodes *pdata = NULL;
	struct irq_node *irq_data = NULL;

	if (!ras_nr_check(edac_dev, inst_nr, block_nr))
		return;

	instance = edac_dev->instances + inst_nr;
	pdata = edac_dev->pvt_info;
	irq_data = pdata->irq_data + inst_nr;
	err_data = irq_data->err_record + block_nr;

	if (is_bus_error(err_data->errstatus) && pdata->mod_id == SHARE_L3) {
		edac_device_printk(edac_dev, KERN_INFO,
				   "Bus error: ACPU not secure read security.");
		return;
	}

	if (instance->nr_blocks > 0) {
		block = instance->blocks + block_nr;
		block->counters.ue_count++;
	}

	/* Propagate the count up the 'totals' tree */
	instance->counters.ue_count++;
	edac_dev->counters.ue_count++;

	if (edac_dev->log_ue)
		edac_device_printk(edac_dev, KERN_EMERG,
				   "UE: %s instance: %s block: %s '%s'\n",
				   edac_dev->ctl_name, instance->name,
				   block ? block->name : "N/A", msg);

	bbox_diaginfo_record(ECC_UE, NULL,
			     "EDAC %s: UE instance: %s block %s errxstatus=0x%llx, errxmisc=0x%llx",
			     edac_dev->ctl_name, instance->name, block ? block->name : "N/A",
			     err_data->errstatus, err_data->errmisc);

	if (edac_dev->panic_on_ue)
		ras_device_panic(pdata, inst_nr, (u32)UE);
}
EXPORT_SYMBOL_GPL(ras_device_handle_ue);

static int ras_device_register_exception(void)
{
	unsigned int ret;
	unsigned int i, j;

	for (i = 0; i < ARRAY_SIZE(cpu_edac_einfo); i++) {
		ret = rdr_register_exception(&cpu_edac_einfo[i]);
		/* if error, return 0; otherwise return e_modid_end */
		if (ret != cpu_edac_einfo[i].e_modid_end) {
			pr_err("register cpu edac exception failed %u\n", i);
			goto err;
		}
	}

	return 0;
err:
	for (j = 0; j < i; j++)
		(void)rdr_unregister_exception(cpu_edac_einfo[j].e_modid);
	return -EPERM;
}

static void ras_device_unregister_exception(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cpu_edac_einfo); i++)
		(void)rdr_unregister_exception(cpu_edac_einfo[i].e_modid);
}

static int __init ras_common_init(void)
{
	int err;

	err = ras_device_register_exception();
	if (err != 0)
		goto out_exception;

	return 0;

out_exception:
	ras_device_unregister_exception();
	return err;
}

static void __exit ras_common_exit(void)
{
	ras_device_unregister_exception();
}

late_initcall(ras_common_init);

module_exit(ras_common_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("RAS COMMON driver");
