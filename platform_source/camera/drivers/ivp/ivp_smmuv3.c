/*
 * This file implements api for smmuv3 tbu
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

#include "ivp_smmuv3.h"
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/scatterlist.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mm_iommu.h>
#include "ivp_log.h"
#include "ivp.h"
//lint -save -e750 -e753 -e750 -e528 -e144 -e82 -e64 -e785 -e715 -e712 -e40
//lint -save -e63 -e732 -e42 -e550 -e438 -e834 -e648 -e747 -e778 -e50 -e838
//lint -save -e571

struct ivp_smmu_dev *g_smmu_dev[IVP_CORE_MAX_NUM];
struct dsm_client *client_ivp;

struct dsm_client_ops client_ops5 = {
	.poll_state = NULL,
	.dump_func = NULL,
};

struct dsm_dev dev_ivp = {
	.name = "dsm_ivp",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &client_ops5,
	.buff_size = DEV_IVP_BUFF_SIZE,
};

void ivp_dsm_error_notify(int error_no)
{
	if (!dsm_client_ocuppy(client_ivp)) {
		dsm_client_record(client_ivp, "ivp");
		dsm_client_notify(client_ivp, error_no);
		ivp_info("[I/DSM] %s ivp dsm error notify, error no:%d",
			client_ivp->client_name, error_no);
	}
}

static int reg_read_check(struct ivp_smmu_dev *smmu_dev,
	unsigned int reg, int value)
{
	unsigned int check_times = 0;
	unsigned int ret;

	do {
		ret = readl(smmu_dev->reg_base + reg);
		if (ret & (unsigned int)value)
			break;
		if (++check_times >= MAX_CHECK_TIMES) {
			ivp_err("check value failed reg:0x%pK 0x%x 0x%x",
				smmu_dev->reg_base + reg, ret, value);
			return -1;
		}
		udelay(1);
	} while (1);
	return 0;
}

static int ivp_smmuv3_tbu_disconnect(struct ivp_smmu_dev *smmu_dev)
{
	unsigned int reg;
	reg = readl(smmu_dev->reg_base + SMMU_TBU_CRACK);
	if (TBU_CONNECTED != (reg & TBU_CONNECTED_MASK)) {
		ivp_err("tbu disconnected status no need disconnect");
		return 0;
	}

	reg = readl(smmu_dev->reg_base + SMMU_TBU_CR);
	reg &= ~TBU_EN_REQ_MASK;
	writel(reg, smmu_dev->reg_base + SMMU_TBU_CR);

	if (reg_read_check(smmu_dev, SMMU_TBU_CRACK, TBU_EN_ACK)) {
		ivp_err("check failed for SMMU_TBU_CRACK bit TBU_EN_ACK");
		return -EIO;
	}

	reg = readl(smmu_dev->reg_base + SMMU_TBU_CRACK);
	if (TBU_DISCONNECTED != (reg & TBU_CONNECTED_MASK)) {
		ivp_err("tbu disconnect tcu failed");
		return -EIO;
	}

	ivp_info("tbu disconnect tcu succed");
	return 0;
}

static int ivp_smmuv3_tbu_init(struct ivp_smmu_dev *smmu_dev)
{
	int ret = 0;
	unsigned int reg;
	unsigned int tok_trans_gnt;

	ivp_info("init tbu for %s", smmu_dev->dev->init_name);

	reg = readl(smmu_dev->reg_base + SMMU_TBU_CR);
	reg |= (MAX_TOK_TRANS_IVP << TBU_TOK_TRANS_SHIFT);
	writel(reg, smmu_dev->reg_base + SMMU_TBU_CR);

	reg = readl(smmu_dev->reg_base + SMMU_TBU_CR);
	reg |= TBU_EN_REQ_ENABLE;
	writel(reg, smmu_dev->reg_base + SMMU_TBU_CR);

	if (reg_read_check(smmu_dev, SMMU_TBU_CRACK, TBU_EN_ACK)) {
		ivp_err("check failed for SMMU_TBU_CRACK bit TBU_EN_ACK");
		ret = -1;
		goto exit;
	}

	reg = readl(smmu_dev->reg_base + SMMU_TBU_CRACK);
	if (TBU_CONNECTED == (reg & TBU_CONNECTED_MASK)) {
		ivp_info("tbu connected tcu succed");
	} else {
		ivp_err("tbu connected tcu failed");
		ret = -EIO;
		goto exit;
	}

	reg = readl(smmu_dev->reg_base + SMMU_TBU_CRACK);

	tok_trans_gnt = (reg & TBU_TOK_TRANS_MASK) >> TBU_TOK_TRANS_SHIFT;
	if (tok_trans_gnt < MAX_TOK_TRANS_IVP) {
		ivp_err("tok_trans_gnt:0x%x smaller than config value:0x%x",
			tok_trans_gnt, MAX_TOK_TRANS_IVP);
		reg = readl(smmu_dev->reg_base + SMMU_TBU_CR);
		ivp_err("reg SMMU_TBU_CR:0x%x", reg);
		/* disconnect tbu when token set fail */
		(void)ivp_smmuv3_tbu_disconnect(smmu_dev);
		return -EIO;
	}
exit:
	ivp_info("exit");
	return ret;
}

static int smmuv3_tbu_deinit(struct ivp_smmu_dev *smmu_dev)
{
	ivp_info("enter");
	return ivp_smmuv3_tbu_disconnect(smmu_dev);
}

/* Enable the translation mode of SMMU */
int ivp_smmu_trans_enable(struct ivp_smmu_dev *smmu_dev)
{
	unsigned long flags = 0;

	if (!smmu_dev) {
		ivp_err("smmuv_dev is invalid");
		return -EPERM;
	}

	if (request_irq(smmu_dev->irq, smmu_dev->isr, 0,
		"ivp-smmu-isr", (void *)smmu_dev))
		ivp_err("failed to request IRQ[%d]", smmu_dev->irq);

	spin_lock_irqsave(&smmu_dev->spinlock, flags);

	/* Check the smmu state */
	if (smmu_dev->state == SMMU_STATE_ENABLE) {
		spin_unlock_irqrestore(&smmu_dev->spinlock, flags);
		ivp_err("smmu enable twice");
		return -EBUSY;
	}

	if (ivp_smmuv3_tbu_init(smmu_dev)) {
		spin_unlock_irqrestore(&smmu_dev->spinlock, flags);
		ivp_err("smmuv3 tbu init fail");
		return -EPERM;
	}
	smmu_dev->state = SMMU_STATE_ENABLE;
	spin_unlock_irqrestore(&smmu_dev->spinlock, flags);

	return 0;
}

/* deinit smmuv3 */
int ivp_smmu_trans_disable(struct ivp_smmu_dev *smmu_dev)
{
	unsigned long flags = 0;

	if (smmu_dev == NULL) {
		ivp_err("smmuv_dev is invalid");
		return -EPERM;
	}

	spin_lock_irqsave(&smmu_dev->spinlock, flags);
	smmu_dev->state = SMMU_STATE_DISABLE;
	smmuv3_tbu_deinit(smmu_dev);
	spin_unlock_irqrestore(&smmu_dev->spinlock, flags);

	/* free the irq */
	disable_irq(smmu_dev->irq);
	free_irq(smmu_dev->irq, (void *)smmu_dev);

	return 0;
}

/* Return the smmu device handler to upper layer */
struct ivp_smmu_dev *ivp_smmu_get_device(unsigned long select)
{
	if (select >= IVP_CORE_MAX_NUM) {
		ivp_err("invalid input param select %lu", select);
		return NULL;
	}
	return g_smmu_dev[select];
}

/* SMMU ISR, handler that SMMU reports fault to */
static irqreturn_t ivp_smmu_isr(int irq __attribute__((unused)),
	void *dev_id __attribute__((unused)))
{
	ivp_err("do nothing");
	return 0;
}

static int ivp_get_smmu_device_info(struct platform_device *plat_dev,
	struct ivp_smmu_dev *smmu_dev)
{
	unsigned int *num = NULL;
	struct resource *res = NULL;
	struct device_node *np = plat_dev->dev.of_node;

	num = (unsigned int *)of_get_property(np, "smmu-version", NULL);
	if (num) {
		smmu_dev->version = be32_to_cpu(*num);
		ivp_info("smmu version is %u", be32_to_cpu(*num));
	}

	res = platform_get_resource(plat_dev, IORESOURCE_MEM, 0);
	if (!res) {
		ivp_err("platform_get_resource err");
		return -ENOENT;/*lint !e429*/
	}
	ivp_dbg("smmu name:%s addr:0x%x", res->name, res->start);

	smmu_dev->reg_base = devm_ioremap_resource(&plat_dev->dev, res);
	if (IS_ERR(smmu_dev->reg_base)) {
		ivp_err("remap resource err!");
		return PTR_ERR(smmu_dev->reg_base);/*lint !e429*/
	}
	smmu_dev->reg_size = resource_size(res);

	res = platform_get_resource(plat_dev, IORESOURCE_IRQ, 0);
	if (!res) {
		ivp_err("get smmu irq failed!");
		return -ENOENT;/*lint !e429*/
	}
	smmu_dev->irq = (unsigned int)res->start;

	return 0;
}
static int ivp_smmu_probe(struct platform_device *plat_dev)
{
	struct ivp_smmu_dev *smmu_dev  = NULL;
	int ret;

	smmu_dev = devm_kzalloc(&plat_dev->dev,
		sizeof(*smmu_dev), GFP_KERNEL);
	if (!smmu_dev) {
		ivp_err("devm_kzalloc is failed");
		return -ENOMEM;
	}
	smmu_dev->dev = &plat_dev->dev;
	smmu_dev->state = SMMU_STATE_DISABLE;
	smmu_dev->dev->init_name = "ivp_smmu";

	smmu_dev->isr = ivp_smmu_isr;
	smmu_dev->err_handler = NULL;
	spin_lock_init(&smmu_dev->spinlock);

	ret = ivp_get_smmu_device_info(plat_dev, smmu_dev);
	if (ret) {
		ivp_err("ivp_get_smmu_device_info fail %d", ret);
		devm_kfree(&plat_dev->dev, smmu_dev);
		return ret;
	}
	g_smmu_dev[IVP_CORE0_ID] = smmu_dev;

	platform_set_drvdata(plat_dev, smmu_dev);
	ivp_info("ivp smmu probes success");

	if (!client_ivp)
		client_ivp = dsm_register_client(&dev_ivp);

	return ret;
}

static int ivp_smmu_remove(struct platform_device *plat_dev)
{
	struct ivp_smmu_dev *smmu_dev =
		(struct ivp_smmu_dev *)platform_get_drvdata(plat_dev);
	if (!smmu_dev) {
		ivp_err("smmu_dev invalid");
		return -ENODEV;
	}

	ivp_info("node name:%s", smmu_dev->dev->init_name);
	if (strcmp(smmu_dev->dev->init_name, "ivp_smmu") == 0) {
		g_smmu_dev[0] = NULL;
	} else if (strcmp(smmu_dev->dev->init_name, "ivp1_smmu") == 0) {
		g_smmu_dev[1] = NULL;
	} else {
		ivp_err("err node name:%s\n", smmu_dev->dev->init_name);
		return -EINVAL;
	}

	return 0;
}

static const struct of_device_id ivp_smmu_of_id[] = {
	{
		.compatible = "hisilicon,ivp-smmu",
	},
	{ },
};

static struct platform_driver ivp_smmu_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ivp-smmu",
		.of_match_table = ivp_smmu_of_id,
	},
	.probe  =  ivp_smmu_probe,
	.remove =  ivp_smmu_remove,
};

module_platform_driver(ivp_smmu_driver);
#ifdef IVP_DUAL_CORE
static int ivp1_smmu_probe(struct platform_device *plat_dev)
{
	struct ivp_smmu_dev *smmu_dev  = NULL;
	int ret;

	smmu_dev = devm_kzalloc(&plat_dev->dev, sizeof(*smmu_dev), GFP_KERNEL);
	if (!smmu_dev) {
		ivp_err("devm_kzalloc failed");
		return -ENOMEM;
	}
	smmu_dev->dev = &plat_dev->dev;
	smmu_dev->state = SMMU_STATE_DISABLE;
	smmu_dev->dev->init_name = "ivp1_smmu";

	smmu_dev->isr = ivp_smmu_isr;
	smmu_dev->err_handler = NULL;
	spin_lock_init(&smmu_dev->spinlock);

	ret = ivp_get_smmu_device_info(plat_dev, smmu_dev);
	if (ret) {
		ivp_err("ivp_get_device_info fail %d", ret);
		devm_kfree(&plat_dev->dev, smmu_dev);
		return ret;
	}
	g_smmu_dev[IVP_CORE1_ID] = smmu_dev;
	ivp_info("ivp1 smmu probes success");

	if (!client_ivp)
		client_ivp = dsm_register_client(&dev_ivp);

	return ret;
}
static const struct of_device_id ivp1_smmu_of_id[] = {
	{
		.compatible = "hisilicon,ivp1-smmu",
	},
	{ },
};

static struct platform_driver ivp1_smmu_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ivp1-smmu",
		.of_match_table = ivp1_smmu_of_id,
	},
	.probe  =  ivp1_smmu_probe,
	.remove =  ivp_smmu_remove,
};
module_platform_driver(ivp1_smmu_driver);
#endif
MODULE_LICENSE("GPL v2");
//lint -restore
