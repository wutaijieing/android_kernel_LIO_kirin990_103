/*
 * ras_device.c
 *
 * RAS device driver
 *
 * Copyright (c) 2021 Huawei Technologies Co., Ltd.
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
#include <asm/cputype.h>
#include <linux/kernel.h>
#include <linux/edac.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include "ras_common.h"

#define RAS_DEVICE_STR	"ras_edac"

static unsigned long g_cpu_edac_cpuhp_state;

static int ras_starting_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct irq_nodes *pdata = hlist_entry_safe(node, struct irq_nodes, node);
	struct irq_node *irq_data = NULL;
	cpumask_t op_mask;
	int ret;
	unsigned int i;

	if (pdata == NULL) {
		pr_err("%s failed. pdata is %s\n", __func__, "NULL");
		return 0;
	}

	for (i = 0; i < pdata->nr_inst; i++) {
		irq_data = pdata->irq_data + i;

		/* cpu_online_mask & ~cpumask */
		cpumask_set_cpu(cpu, &op_mask);
		cpumask_andnot(&op_mask, cpu_online_mask, &op_mask);
		ret = cpumask_and(&op_mask, &op_mask, &irq_data->support_cpus);
		if (ret != 0)
			continue;

		if (irq_data->fault_irq != 0) {
			ret = irq_set_affinity_hint(irq_data->fault_irq,
						    &irq_data->support_cpus);
			if (ret != 0) {
				pr_err("%s: set fault-irq affinity failed\n", __func__);
				return ret;
			}
		}

		if (irq_data->err_irq != 0) {
			ret = irq_set_affinity_hint(irq_data->err_irq, &irq_data->support_cpus);
			if (ret != 0) {
				pr_err("%s: set err-irq affinity failed\n", __func__);
				return ret;
			}
		}
	}
	return 0;
}

static int ras_stopping_cpu(unsigned int cpu __maybe_unused,
			    struct hlist_node *node __maybe_unused)
{
	return 0;
}

static void parse(int irq, struct edac_device_ctl_info *edac_dev)
{
	struct irq_nodes *pdata = edac_dev->pvt_info;
	struct irq_node *irq_data = pdata->irq_data;
	struct edac_device_instance *instance = NULL;
	struct err_record *err_data = NULL;
	int inst_nr, block_nr;
	u64 errstatus, errmisc;
	unsigned int i;

	/* get inst_nr, get block_nr */
	for (i = 0; i < edac_dev->nr_instances; i++) {
		if (irq == irq_data[i].fault_irq || irq == irq_data[i].err_irq) {
			inst_nr = (int)i;
			break;
		}
	}

	if (i == edac_dev->nr_instances) {
		edac_device_printk(edac_dev, KERN_CRIT, "There is no error\n");
		return;
	}

	instance = edac_dev->instances + inst_nr;
	for (i = 0; i < instance->nr_blocks; i++) {
		if (pdata->access_type == MEM) {
			errstatus = readq(pdata->base + ERR1STATUS);
			errmisc = readq(pdata->base + ERR1MISC0);
		} else {
			write_errselr_el1(pdata->idx_blocks[i]);
			dsb(sy);
			isb();
			errstatus = read_erxstatus_el1();
			errmisc = read_erxmisc0_el1();
			isb();
		}

		if (ERR_STATUS_GET_FIELD(errstatus, V) != 0U) {
			edac_device_printk(edac_dev, KERN_CRIT, "ERRXSTATUS_EL1: 0x%llx\n", errstatus);
			edac_device_printk(edac_dev, KERN_CRIT, "ERRXMISC_EL1: 0x%llx\n", errmisc);
			edac_device_printk(edac_dev, KERN_CRIT, "Instanche name level: %s\n", instance->name);
			edac_device_printk(edac_dev, KERN_CRIT, "Block level: L%u\n", i + 1);

			block_nr = (int)i;
			err_data = irq_data[inst_nr].err_record + block_nr;
			err_data->errstatus = errstatus;
			err_data->errmisc = errmisc;

			if (ERR_STATUS_GET_FIELD(err_data->errstatus, CE) != 0U) {
				ras_device_handle_ce(edac_dev, inst_nr, block_nr, edac_dev->ctl_name);
				errstatus = errstatus | (ERR_STATUS_CE_CLE << ERR_STATUS_CE_SHIFT);
			}

			if (ERR_STATUS_GET_FIELD(err_data->errstatus, DE) != 0U)
				ras_device_handle_de(edac_dev, inst_nr, block_nr, edac_dev->ctl_name);

			if (ERR_STATUS_GET_FIELD(err_data->errstatus, UE) != 0U)
				ras_device_handle_ue(edac_dev, inst_nr, block_nr, edac_dev->ctl_name);

			if (pdata->access_type == MEM) {
				writeq(errstatus, pdata->base + ERR1STATUS);
				writeq(pdata->init_misc_data[i], pdata->base + ERR1MISC0);
			} else {
				write_errselr_el1(pdata->idx_blocks[i]);
				dsb(sy);
				isb();
				write_erxstatus_el1(errstatus);
				write_erxmisc0_el1(pdata->init_misc_data[i]);
				isb();
			}
		}
	}
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	struct edac_device_ctl_info *edac_dev = dev_id;

	parse(irq, edac_dev);
	return IRQ_HANDLED;
}

static int ras_device_info(struct platform_device *pdev)
{
	struct edac_device_ctl_info *edac_dev = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct irq_nodes *pdata = edac_dev->pvt_info;
	struct edac_device_instance *instances = edac_dev->instances;
	struct resource *r = NULL;
	int ret;

	ret = of_property_read_u32(np, "mode-id", &pdata->mod_id);
	if (ret != 0) {
		dev_err(dev, "find mode-id for target error\n");
		goto err;
	}

	dev_dbg(dev, "mod-id is %lx\n", pdata->mod_id);

	pdata->idx_blocks = devm_kzalloc(dev, sizeof(u32) * instances->nr_blocks, GFP_KERNEL);
	if (pdata->idx_blocks == NULL) {
		dev_err(dev, "fail to alloc mem for idx_blocks\n");
		ret = -ENOMEM;
		goto err;
	}

	pdata->init_misc_data = devm_kzalloc(dev, sizeof(u64) * instances->nr_blocks, GFP_KERNEL);
	if (pdata->init_misc_data == NULL) {
		dev_err(dev, "fail to alloc mem for misc_data_init\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = of_property_read_u32_array(np, "idx-blocks", pdata->idx_blocks, instances->nr_blocks);
	if (ret != 0) {
		dev_err(dev, "read idx-blocks for target error\n");
		goto err;
	}

	ret = of_property_read_u64_array(np, "misc-data-init", pdata->init_misc_data, instances->nr_blocks);
	if (ret != 0) {
		dev_err(dev, "read idx-blocks for target error\n");
		goto err;
	}

	ret = of_property_read_u32(np, "access-type", &pdata->access_type);
	if (ret != 0) {
		dev_err(dev, "read access-type for target error\n");
		goto err;
	}

	pdata->base = NULL;
	if (pdata->access_type == MEM) {
		r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (r == NULL) {
			dev_err(dev, "Unable to get mem resource\n");
			ret = -ENODEV;
			goto err;
		}

		if (devm_request_mem_region(dev, r->start,
					    resource_size(r), dev_name(dev)) == NULL) {
			dev_err(dev, "Error while requesting mem region\n");
			ret = -EBUSY;
			goto err;
		}

		pdata->base = devm_ioremap(dev, r->start, resource_size(r));
		if (pdata->base == NULL) {
			dev_err(dev, "Unable to map regs\n");
			ret = -ENOMEM;
			goto err;
		}
	}

err:
	return ret;
}

static void inst_irq_parse(struct platform_device *pdev, struct device_node *child, int nr_instances)
{
	struct edac_device_ctl_info *edac_dev = platform_get_drvdata(pdev);
	struct edac_device_instance *instance = NULL;
	struct device_node *dn = NULL;
	struct property *prop = NULL;
	struct irq_nodes *pdata = edac_dev->pvt_info;
	struct irq_node *irq_data = NULL;
	int len = 0;
	int cpu, i;

	irq_data = pdata->irq_data + nr_instances;
	instance = edac_dev->instances + nr_instances;

	irq_data->fault_irq = of_irq_get(child, 0);
	if (irq_data->fault_irq <= 0) {
		irq_data->fault_irq = 0;
		dev_err(&pdev->dev, "instance %s get fault_irq fail\n", instance->name);
	}

	irq_data->err_irq = of_irq_get(child, 1);
	if (irq_data->err_irq <= 0) {
		irq_data->err_irq = 0;
		dev_err(&pdev->dev, "instance %s get err_irq fail\n", instance->name);
	}

	dev_dbg(&pdev->dev, "%s fault_irq = %d, err_irq = %d\n",
		instance->name, irq_data->fault_irq, irq_data->err_irq);

	/* if irq affinity is all cpu, set interrupt-affinity is NULL */
	prop = of_find_property(child, "interrupt-affinity", &len);
	if (prop == NULL || len == 0) {
		irq_data->support_cpus = *cpu_online_mask;
		dev_err(&pdev->dev, "share node support_cpus is all cpu\n");
		return;
	}

	len = prop->length / (int)sizeof(u32);
	for (i = 0; i < len; i++) {
		dn = of_parse_phandle(child, "interrupt-affinity", i);
		if (dn == NULL)
			continue;
		cpu = of_cpu_node_to_id(dn);
		cpumask_set_cpu(cpu, &irq_data->support_cpus);
	}
	cpumask_and(&irq_data->support_cpus, cpu_online_mask, &irq_data->support_cpus);
	dev_dbg(&pdev->dev, "support_cpus = %x\n", (unsigned int)irq_data->support_cpus.bits[0]);
}

static int inst_irq_request(struct platform_device *pdev, int instance)
{
	struct edac_device_ctl_info *edac_dev = platform_get_drvdata(pdev);
	struct irq_nodes *pdata = edac_dev->pvt_info;
	struct edac_device_instance *dev_inst = NULL;
	struct irq_node *irq_data = pdata->irq_data;
	const irq_handler_t handler = irq_handler;
	int ret = 0;

	dev_inst = edac_dev->instances + instance;
	if (cpumask_empty(&irq_data[instance].support_cpus)) {
		dev_err(&pdev->dev, "Not supported, request %s failed \n", dev_inst->name);
		return 0;
	}

	if (irq_data[instance].fault_irq != 0) {
		ret = devm_request_irq(&pdev->dev, irq_data[instance].fault_irq, handler,
				       IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_NO_SUSPEND,
				       dev_inst->name, edac_dev);
		if (ret != 0) {
			dev_err(&pdev->dev, "request %s fault-irq failed \n", dev_inst->name);
			goto err;
		}

		ret = irq_set_affinity_hint(irq_data[instance].fault_irq, &irq_data[instance].support_cpus);
		if (ret != 0) {
			dev_err(&pdev->dev, "%s set fault-irq affinity failed\n", dev_inst->name);
			goto err;
		}
	}

	if (irq_data[instance].err_irq != 0) {
		ret = devm_request_irq(&pdev->dev, irq_data[instance].err_irq, handler,
				       IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_NO_SUSPEND,
				       dev_inst->name, edac_dev);
		if (ret != 0) {
			dev_err(&pdev->dev, "request %s err-irq failed\n", dev_inst->name);
			goto err;
		}

		ret = irq_set_affinity_hint(irq_data[instance].err_irq, &irq_data[instance].support_cpus);
		if (ret != 0) {
			dev_err(&pdev->dev, "%s set fault-irq affinity failed\n", dev_inst->name);
			goto err;
		}
	}

err:
	return ret;
}

static int ras_err_reg(struct platform_device *pdev, int nr_instance)
{
	struct err_record *err = NULL;
	struct edac_device_ctl_info *edac_dev = platform_get_drvdata(pdev);
	struct edac_device_instance *instances = NULL;
	struct irq_nodes *pdata = edac_dev->pvt_info;

	instances = edac_dev->instances + nr_instance;
	(pdata->irq_data + nr_instance)->nr_block = instances->nr_blocks;
	err = devm_kzalloc(&pdev->dev, sizeof(struct err_record) * instances->nr_blocks, GFP_KERNEL);
	if (!err) {
		dev_err(&pdev->dev, "edac_dev init fail!\n");
		return -ENOMEM;
	}

	(pdata->irq_data + nr_instance)->err_record = err;

	return 0;
}

static int ras_irq_nodes(struct platform_device *pdev)
{
	struct edac_device_ctl_info *edac_dev = platform_get_drvdata(pdev);
	struct irq_nodes *pdata = edac_dev->pvt_info;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child = NULL;
	int instance = 0;
	int ret;

	ret = ras_device_info(pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "read info for target error\n");
		goto err;
	}

	pdata->irq_data = devm_kzalloc(&pdev->dev, sizeof(struct irq_node) * pdata->nr_inst, GFP_KERNEL);
	if (!pdata->irq_data) {
		dev_err(&pdev->dev, "irq_data request fail!\n");
		return -ENOMEM;
	}

	for_each_child_of_node(np, child) {
		ret = ras_err_reg(pdev, instance);
		if (ret != 0) {
			dev_err(&pdev->dev, "ras err reg init read fail\n");
			goto err;
		}
		inst_irq_parse(pdev, child, instance);

		ret = inst_irq_request(pdev, instance);
		if (ret != 0) {
			dev_err(&pdev->dev, "inst irq request fail\n");
			goto err;
		}

		instance++;
	}

	dev_err(&pdev->dev, "parse nodes sucess\n");
err:
	return ret;
}

static const struct of_device_id ras_device_match_table[] = {
	{ .compatible = "hisilicon,cpu-edac-ecc", },
	{},
};

static int ras_device_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct edac_device_ctl_info *edac_dev = NULL;
	struct irq_nodes *pdata = NULL;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *id = NULL;
	int nr_blocks, nr_instances;
	int ret;

	ret = of_property_read_u32(np, "nr-blocks", &nr_blocks);
	if (ret != 0) {
		dev_err(dev, "read nr-blocks fail!\n");
		goto err;
	}
	nr_instances = of_get_child_count(np);

	dev_dbg(dev, "nr_blocks = %d, nr_instances = %d\n", nr_blocks, nr_instances);
	edac_dev = edac_device_alloc_ctl_info(sizeof(*pdata), (char *)np->name, nr_instances,
					      "L", nr_blocks, 1, NULL, 0,
					      edac_device_alloc_index());
	if (edac_dev == NULL) {
		dev_err(dev, "edac_dev init fail!\n");
		return -ENOMEM;
	}

	pdata = edac_dev->pvt_info;
	pdata->nr_inst = edac_dev->nr_instances;

	edac_dev->dev = dev;

	platform_set_drvdata(pdev, edac_dev);

	id = of_match_device(ras_device_match_table, dev);
	edac_dev->mod_name = RAS_DEVICE_STR;
	edac_dev->ctl_name = id ? id->compatible : "unknown";
	edac_dev->dev_name = dev_name(dev);
#ifdef CONFIG_RAS_EDAC_DEBUG
	edac_dev->panic_on_ue = 1;
#else
	edac_dev->panic_on_ue = 0;
#endif
	ret = ras_irq_nodes(pdev);
	if (ret != 0) {
		dev_err(dev, "get irq nodes failed\n");
		goto out_mem;
	}

	ret = edac_device_add_device(edac_dev);
	if (ret != 0) {
		dev_err(dev, "add device failed\n");
		goto out_mem;
	}

	ret = cpuhp_state_add_instance(g_cpu_edac_cpuhp_state,
				       &pdata->node);
	if (ret != 0) {
		dev_err(dev, "cuphp add instance failed\n");
		goto out_mem;
	}

	dev_err(dev, "success\n");

	return 0;

out_mem:
	edac_device_free_ctl_info(edac_dev);
err:
	return ret;
}

static int ras_device_remove(struct platform_device *pdev)
{
	struct edac_device_ctl_info *edac_dev = platform_get_drvdata(pdev);
	struct irq_nodes *pdata = edac_dev->pvt_info;

	cpuhp_state_remove_instance(g_cpu_edac_cpuhp_state,
				    &pdata->node);
	edac_device_del_device(edac_dev->dev);

	edac_device_free_ctl_info(edac_dev);

	return 0;
}

static struct platform_driver ras_device_driver = {
	.probe = ras_device_probe,
	.remove = ras_device_remove,
	.driver = {
		.name = "ras_device_probe",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ras_device_match_table),
	},
};

static int __init ras_device_init(void)
{
	int ret;

	ret = cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
				      "edac/ecc:starting",
				      ras_starting_cpu,
				      ras_stopping_cpu);
	if (ret < 0) {
		pr_err("CPU hotplug notifier registered failed: %d\n", ret);
		return ret;
	}

	g_cpu_edac_cpuhp_state = ret;

	ret = platform_driver_register(&ras_device_driver);
	if (ret != 0)
		cpuhp_remove_multi_state(g_cpu_edac_cpuhp_state);

	return ret;
}
late_initcall(ras_device_init);

static void __exit ras_device_exit(void)
{
	platform_driver_unregister(&ras_device_driver);
	cpuhp_remove_multi_state(g_cpu_edac_cpuhp_state);
}
module_exit(ras_device_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("RAS EDAC driver");
