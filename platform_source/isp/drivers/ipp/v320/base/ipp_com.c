/*
 * IPP Common Driver
 *
 * Copyright (c) 2018 IPP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/genalloc.h>
#include <linux/uaccess.h>
#include <linux/mm_iommu.h>
#include <platform_include/isp/linux/hipp_common.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include "ipp.h"
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/syscalls.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>

#define DTSNAME_HIPPCOM    "hisilicon,ippcom"

#define IPP_DEV_MASK 0x0F

struct hippcomdev_s {
	struct platform_device *pdev;
	int initialized;
	unsigned int drvnum;
	const char *drvname[MAX_HIPP_COM];
	struct hipp_common_s *drv[MAX_HIPP_COM];
	unsigned int refs_smmu[MAX_HIPP_TBU];
	unsigned int refs_power;
	struct mutex mutex_dev;
	struct mutex mutex_smmu;
	struct mutex mutex_power;
	struct mutex mutex_jpgclk;
};

static struct hippcomdev_s *ippcomdev;

static int invalid_ipptype(unsigned int type)
{
	if (type >= MAX_HIPP_COM) {
		pr_err("[%s] Failed : Invalid Type.%d\n", __func__, type);
		return -EINVAL;
	}
	if (((1 << type) & IPP_DEV_MASK) == 0) {
		pr_err("[%s] Failed : Invalid Type.%d\n", __func__, type);
		return -EINVAL;
	}
	return 0;
}

static struct hippcomdev_s *hipp_drv2dev(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret;
	if (drv == NULL) {
		E("Failed : drv.%pK\n", drv);
		return NULL;
	}
	ret = invalid_ipptype(drv->type);
	if (ret < 0) {
		E("Failed : invalid_ipptype.%d\n", ret);
		return NULL;
	}
	dev = (struct hippcomdev_s *)drv->comdev;
	if (dev == NULL) {
		E("Failed : dev.%pK\n", dev);
		return NULL;
	}
	if (drv != dev->drv[drv->type]) {
		E("Failed : Mismatch drv.(%pK, %pK), type.%d\n",
			drv, dev->drv[drv->type], drv->type);
		return NULL;
	}
	return dev;
}

static int do_ippdump(struct hippcomdev_s *dev)
{
	struct hipp_common_s *drv = NULL;
	int i = 0;
	if (dev == NULL) {
		E("Failed : dev.%pK\n", dev);
		return -ENOMEM;
	}
	I("Device: refs_smmu.0x%x, initialized.0x%x\n",
		dev->refs_smmu[HIPP_TBU_IPP], dev->initialized);
	for (i = 0; i < MAX_HIPP_COM; i++) {
		drv = dev->drv[i];
		if (drv == NULL) {
			E("%d Not Registered\n", i);
			continue;
		}
		I("Driver.%d: %s.type.0x%x, initialized.0x%x, mode.0x%x",
			i, drv->name, drv->type,
			drv->initialized, drv->tbu_mode);
	}
	return 0;
}

static int hippsmmu_enable(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return -ENOMEM;
	}
	pr_info("[%s] : name.%s, refs.%d\n", __func__,
		drv->name, dev->refs_smmu[drv->tbu_mode]);
	if (dev->refs_power == 0) {
		pr_err("[%s] Failed : ippsubsys powerdown\n", __func__);
		return -EINVAL;
	}
	mutex_lock(&dev->mutex_smmu);
	if (dev->refs_smmu[drv->tbu_mode] == 0) {
		ret = atfhipp_smmu_enable(drv->tbu_mode);
		if (ret != 0) {
			pr_err("[%s] Failed : atfhipp_smmu_enable, name.%s, ret.%d\n",
				__func__, drv->name, ret);
			mutex_unlock(&dev->mutex_smmu);
			return ret;
		}
	}
	dev->refs_smmu[drv->tbu_mode]++;
	pr_info("[%s] : refs_smmu mode.%d, num.%d\n",
		__func__, drv->tbu_mode, dev->refs_smmu[drv->tbu_mode]);
	mutex_unlock(&dev->mutex_smmu);
	I("- %s\n", drv->name);
	return 0;
}

static int hippsmmu_disable(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return -ENOMEM;
	}
	pr_info("[%s] : + %s\n", __func__, drv->name);
	if (dev->refs_power == 0) {
		pr_err("[%s] Failed : ippsubsys powerdown\n", __func__);
		return -EINVAL;
	}
	mutex_lock(&dev->mutex_smmu);
	if (dev->refs_smmu[drv->tbu_mode] == 0) {
		pr_err("[%s] : smmu no need to disable,refs_smmu.%d\n",
			__func__, dev->refs_smmu[drv->tbu_mode]);
		mutex_unlock(&dev->mutex_smmu);
		return -EINVAL;
	}
	dev->refs_smmu[drv->tbu_mode]--;
	if (dev->refs_smmu[drv->tbu_mode] == 0) {
		ret = atfhipp_smmu_disable(drv->tbu_mode);
		if (ret != 0) {
			pr_err("[%s] Failed : atfhipp_smmu_disable.%d, mode.%d\n",
				__func__, ret, drv->tbu_mode);
			mutex_unlock(&dev->mutex_smmu);
			return ret;
		}
	}
	I("refs_smmu mode.%d, num .%d\n",
		drv->tbu_mode, dev->refs_smmu[drv->tbu_mode]);
	mutex_unlock(&dev->mutex_smmu);
	return 0;
}

static int hippsmmu_setsid(struct hipp_common_s *drv,
	unsigned int swid, unsigned int len,
	unsigned int sid, unsigned int ssid)
{
	struct hippcomdev_s *dev = NULL;
	int ret;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return -ENOMEM;
	}
	pr_info("[%s] : name.%s, len.%d, mode.%d\n",
		__func__, drv->name, len, drv->tbu_mode);
	ret = atfhipp_smmu_smrx(swid, len, sid, ssid, drv->tbu_mode);
	if (ret != 0) {
		pr_err("[%s] Failed : atfhipp_smmu_smrx.%s, ret.%d\n",
			__func__, drv->name, ret);
		drv->dump(drv);
		return ret;
	}
	I("- %s\n", drv->name);
	return 0;
}

static int hippsmmu_setpref(struct hipp_common_s *drv,
	unsigned int swid, unsigned int len)
{
	struct hippcomdev_s *dev = NULL;
	int ret;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return -ENOMEM;
	}
	pr_info("[%s] : name.%s, swid.%d, len.%d\n",
		__func__, drv->name, swid, len);
	ret = atfhipp_smmu_pref(swid, len);
	if (ret != 0) {
		pr_err("[%s] Failed : atfhipp_smmu_pref.%s, ret.%d\n",
			__func__, drv->name, ret);
		drv->dump(drv);
		return ret;
	}
	I("- %s\n", drv->name);
	return 0;
}

static void hipp_dump(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	unsigned int ret;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return;
	}
	I("+ %s\n", drv->name);
	mutex_lock(&dev->mutex_smmu);
	ret = do_ippdump(dev);
	if (ret != 0)
		E("Failed : do_ippdump.(%pK).%pK\n", drv, dev);
	mutex_unlock(&dev->mutex_smmu);
	I("- %s\n", drv->name);
}

int hipp_smmuv3_pu(struct hipp_common_s *drv)
{
	int ret = 0;
	struct hippcomdev_s *dev = NULL;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return -EINVAL;
	}
	mutex_lock(&dev->mutex_power);
	if (dev->refs_power == 0) {
		ret = hipp_core_powerup(drv->type);
		if (ret < 0) {
			pr_err("[%s] Failed : hipp_core_powerup, name.%s, ret.%d\n",
				__func__, drv->name, ret);
			mutex_unlock(&dev->mutex_power);
			return ret;
		}
	}
	dev->refs_power++;
	pr_info("[%s] : refs_power.%d\n", __func__, dev->refs_power);
	mutex_unlock(&dev->mutex_power);
	return 0;
}

int hipp_smmuv3_pd(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		E("Failed : hipp_drv2dev.(%pK).%pK\n", drv, dev);
		return -EINVAL;
	}
	mutex_lock(&dev->mutex_power);
	if (dev->refs_power == 0) {
		pr_err("[%s] : ipp no need to pn,refs_power.%d\n",
			__func__, dev->refs_power);
		mutex_unlock(&dev->mutex_power);
		return -EINVAL;
	}
	dev->refs_power--;
	if (dev->refs_power == 0) {
		ret = hipp_core_powerdn(drv->type);
		if (ret != 0) {
			pr_err("[%s] Failed : hipp_core_powerdn.%d\n",
				__func__, ret);
			mutex_unlock(&dev->mutex_power);
			return ret;
		}
	}
	pr_info("[%s] : refs_power num.%d\n", __func__, dev->refs_power);
	mutex_unlock(&dev->mutex_power);
	return 0;
}

static int hipp_jpgclk_set_rate(struct hipp_common_s *drv, unsigned int clktype)
{
	struct hippcomdev_s *dev = NULL;
	int ret;
	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev\n", __func__);
		return -EINVAL;
	}
	if (clktype > (MAX_CLK_RATE - 1)) {
		pr_err("[%s] : clktype.%d\n", __func__, clktype);
		return -EINVAL;
	}
	pr_info("[%s] : drv->name.%s\n", __func__, drv->name);
	mutex_lock(&dev->mutex_power);
	if (dev->refs_power == 0) {
		pr_err("[%s] : ippcore not poweron.\n", __func__);
		mutex_unlock(&dev->mutex_power);
		return -EINVAL;
	}
	mutex_unlock(&dev->mutex_power);
	mutex_lock(&dev->mutex_jpgclk);
	ret = hipp_set_rate(drv->type, clktype);
	if (ret < 0) {
		pr_err("[%s] Failed : hipp_set_rate ret.%d\n", __func__, ret);
		mutex_unlock(&dev->mutex_jpgclk);
		return ret;
	}
	mutex_unlock(&dev->mutex_jpgclk);
	return 0;
}

static unsigned long hipp_smmuv3_iommu_map(struct hipp_common_s *drv,
	int sharefd, int prot, unsigned long *out_size)
{
	unsigned long iova;
	unsigned long size = 0;
	int i = 0;
	struct dma_buf *modulebuf = NULL;
	struct hippcomdev_s *comdev = NULL;
	pr_info("[%s] +\n", __func__);
	if (drv == NULL) {
		pr_err("[%s] Failed : drv is NULL\n", __func__);
		return 0;
	}
	comdev = (struct hippcomdev_s *)drv->comdev;
	if (comdev == NULL) {
		pr_err("[%s] Failed : comdev is NULL\n", __func__);
		return 0;
	}
	for (i = 0; i < MAX_HIPP_COM; i++) {
		if (comdev->drv[i] == drv)
			break;
	}
	if (i == MAX_HIPP_COM) {
		pr_err("[%s] Failed : drv is invalid\n", __func__);
		return 0;
	}
	if (drv->dev == NULL) {
		pr_err("[%s] Failed : drv->dev is NULL\n", __func__);
		return 0;
	}
	if (sharefd < 0) {
		pr_err("[%s] Failed : sharefd.%d\n", __func__, sharefd);
		return 0;
	}
	pr_info("[%s] : sharefd.%d, prot.%d\n", __func__, sharefd, prot);
	modulebuf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get, buf.%pK\n",
			__func__, modulebuf);
		return 0;
	}
	iova = kernel_iommu_map_dmabuf(drv->dev, modulebuf, prot, &size);
	if (iova == 0) {
		pr_err("[%s] Failed : iommu_map_dmabuf\n", __func__);
		goto err_dma_buf_get;
	}
	*out_size = size;
	pr_info("[%s] : iova.0x%lx, size.0x%lx\n", __func__, iova, size);
err_dma_buf_get:
	dma_buf_put(modulebuf);
	return iova;
}

static int hipp_smmuv3_iommu_unmap(struct hipp_common_s *drv,
	int sharefd, unsigned long iova)
{
	int ret;
	int i = 0;
	struct dma_buf *modulebuf = NULL;
	struct hippcomdev_s *comdev = NULL;
	pr_info("[%s] +\n", __func__);
	if (drv == NULL) {
		pr_err("[%s] Failed : drv is NULL\n", __func__);
		return -EINVAL;
	}
	comdev = (struct hippcomdev_s *)drv->comdev;
	if (comdev == NULL) {
		pr_err("[%s] Failed : comdev is NULL\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < MAX_HIPP_COM; i++) {
		if (comdev->drv[i] == drv)
			break;
	}
	if (i == MAX_HIPP_COM) {
		pr_err("[%s] Failed : drv is invalid\n", __func__);
		return -EINVAL;
	}
	if (drv->dev == NULL) {
		pr_err("[%s] Failed : drv->dev is NULL\n", __func__);
		return -EINVAL;
	}
	if (sharefd < 0) {
		pr_err("[%s] Failed : sharefd.%d\n", __func__, sharefd);
		return -EINVAL;
	}
	if (iova == 0) {
		pr_err("[%s] Failed : iova.0\n", __func__);
		return -EINVAL;
	}
	pr_info("[%s] : sharefd.%d\n", __func__, sharefd);
	modulebuf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get, buf.%pK\n",
			__func__, modulebuf);
		return -EINVAL;
	}
	ret = kernel_iommu_unmap_dmabuf(drv->dev, modulebuf, iova);
	if (ret < 0)
		pr_err("[%s] Failed : iommu_unmap_dmabuf, ret.%d\n",
			__func__, ret);
	dma_buf_put(modulebuf);
	return ret;
}

static void *hipp_map_kernel(struct hipp_common_s *drv, int sharefd)
{
	void *virt_addr = NULL;
	int ret;
	struct dma_buf *modulebuf = NULL;
	I("+\n");
	if (sharefd < 0) {
		pr_err("[%s] Failed : sharefd.%d\n", __func__, sharefd);
		return NULL;
	}
	modulebuf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get, modulebuf.%pK\n",
			__func__, modulebuf);
		return NULL;
	}
	ret = dma_buf_begin_cpu_access(modulebuf, DMA_FROM_DEVICE);
	if (ret < 0) {
		pr_err("[%s] dma access failed: ret.%d\n", __func__, ret);
		goto err_dma_buf_begin;
	}
	virt_addr = dma_buf_kmap(modulebuf, 0);
	if (virt_addr == NULL) {
		pr_err("[%s] Failed : dma_buf_kmap.%pK\n", __func__, virt_addr);
		goto err_dma_buf_kmap;
	}
	dma_buf_put(modulebuf);
	pr_info("[%s] : virt_addr = 0x%pK", __func__, virt_addr);
	I("-\n");
	return virt_addr;
err_dma_buf_kmap:
	ret = dma_buf_end_cpu_access(modulebuf, DMA_FROM_DEVICE);
	if (ret < 0)
		pr_err("[%s] dma access failed: ret.%d\n", __func__, ret);
err_dma_buf_begin:
	dma_buf_put(modulebuf);
	return NULL;
}

static int hipp_unmap_kernel(struct hipp_common_s *drv,
	int sharefd, void *virt_addr)
{
	int ret;
	struct dma_buf *modulebuf = NULL;
	I("+\n");
	if ((sharefd < 0) || (virt_addr == NULL)) {
		pr_err("[%s] Failed : sharefd.%d, virt_addr.%pK\n",
			__func__, sharefd, virt_addr);
		return -EINVAL;
	}
	modulebuf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : modulebuf.%pK\n", __func__, modulebuf);
		return -EINVAL;
	}
	dma_buf_kunmap(modulebuf, 0, virt_addr);
	ret = dma_buf_end_cpu_access(modulebuf, DMA_FROM_DEVICE);
	if (ret < 0)
		pr_err("[%s] Failed : dma access ret.%d\n", __func__, ret);
	dma_buf_put(modulebuf);
	I("-\n");
	return 0;
}

struct hipp_common_s *hipp_common_register(unsigned int type,
	struct device *dev)
{
	int ret;
	struct hipp_common_s *drv = NULL;
	struct hippcomdev_s *comdev = ippcomdev;
	pr_info("[%s]: type.%d\n", __func__, type);
	if (comdev == NULL) {
		pr_err("[%s] Failed : comdev NULL\n", __func__);
		return NULL;
	}
	ret = invalid_ipptype(type);
	if (ret < 0)
		return NULL;
	if (!comdev->initialized)
		pr_err("[%s] warning : comDevice no initialized\n", __func__);
	mutex_lock(&comdev->mutex_dev);
	if (comdev->drv[type] != NULL) {
		mutex_unlock(&comdev->mutex_dev);
		pr_err("[%s] Failed : dev registered type.%d\n",
			__func__, type);
		return comdev->drv[type];
	}
	drv = kzalloc(sizeof(struct hipp_common_s), GFP_KERNEL);
	if (drv == NULL) {
		mutex_unlock(&comdev->mutex_dev);
		pr_err("[%s] Failed : kzalloc NULL\n", __func__);
		return NULL;
	}
	drv->type = type;
	drv->tbu_mode = HIPP_TBU_IPP;
	drv->name = comdev->drvname[type];
	drv->iommu_map = hipp_smmuv3_iommu_map;
	drv->iommu_unmap = hipp_smmuv3_iommu_unmap;
	drv->dump = hipp_dump;
	drv->power_up = hipp_smmuv3_pu;
	drv->power_dn = hipp_smmuv3_pd;
	drv->set_jpgclk_rate = hipp_jpgclk_set_rate;
	drv->setsid_smmu = hippsmmu_setsid;
	drv->set_pref = hippsmmu_setpref;
	drv->enable_smmu = hippsmmu_enable;
	drv->disable_smmu = hippsmmu_disable;
	drv->kmap = hipp_map_kernel;
	drv->kunmap = hipp_unmap_kernel;
	drv->dev = dev;
	drv->comdev = (void *)comdev;
	drv->initialized = 1;
	comdev->drv[type] = drv;
	mutex_unlock(&comdev->mutex_dev);
	pr_info("[%s] -\n", __func__);
	return drv;
}

int hipp_common_unregister(unsigned int type)
{
	int ret;
	struct hippcomdev_s *dev = ippcomdev;
	ret = invalid_ipptype(type);
	if (ret < 0) {
		E("Failed : invalid_ipptype.%d\n", ret);
		return ret;
	}
	if (dev  == NULL) {
		E("Failed : dev.%pK\n", dev);
		return -ENODEV;
	}
	mutex_lock(&dev->mutex_dev);
	if (dev->drv[type] == NULL) {
		pr_err("[%s] Failed : %d has not been Registered\n",
			__func__, type);
		mutex_unlock(&dev->mutex_dev);
		return -EINVAL;
	}
	I("+ %s\n", dev->drv[type]->name);
	kfree(dev->drv[type]);
	dev->drv[type] = NULL;
	mutex_unlock(&dev->mutex_dev);
	pr_info("[%s] -\n", __func__);
	return 0;
}

static int hippcom_earlydts_init(struct hippcomdev_s *dev)
{
	struct device_node *np = NULL;
	char *name = DTSNAME_HIPPCOM;
	int ret;
	int index;
	I("+\n");
	if (dev == NULL) {
		E("Failed : hipp com dev.%pK\n", dev);
		return -ENODEV;
	}
	np = of_find_compatible_node(NULL, NULL, name);
	if (np == NULL) {
		E("Failed : %s.of_find_compatible_node.%pK\n", name, np);
		return -ENXIO;
	}
	ret = of_property_read_u32(np, "drv-num",
			(unsigned int *)(&dev->drvnum));
	if (ret != 0) {
		E("Failed: drv-num of_property_read_u32.%d\n", ret);
		return -EINVAL;
	}
	if (dev->drvnum > MAX_HIPP_COM) {
		E("Failed: drvnum.%d > %d, Please Check Codes/DTS\n",
			dev->drvnum, MAX_HIPP_COM);
		return -EINVAL;
	}
	ret = of_property_read_string_array(np, "drv-names",
			dev->drvname, dev->drvnum);
	if (ret < dev->drvnum) { //lint !e574
		E("Failed : drv-names of_string_array.%d < %d\n",
			ret, dev->drvnum);
		return -EINVAL;
	}
	for (index = 0; index < dev->drvnum; index++)//lint !e574
		pr_info("[%s] HippDrvList.index.%d, name.%s\n",
			__func__, index, dev->drvname[index]);
	return 0;
}

static int hippcom_probe(struct platform_device *pdev)
{
	struct hippcomdev_s *dev = NULL;
	I("+\n");
	pr_info("%s: +\n", __func__);
	dev = ippcomdev;
	if (dev == NULL) {
		E("Failed : g_ippcomdev.NULL\n");
		return -ENOMEM;
	}
	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);
	dev->initialized = 1;
	I("-\n");
	return 0;
}

static int hippcom_remove(struct platform_device *pdev)
{
	struct hippcomdev_s *dev = NULL;
	I("+\n");
	dev = (struct hippcomdev_s *)platform_get_drvdata(pdev);
	if (dev == NULL) {
		E("Failed : platform_get_drvdata, pdev.%pK\n", pdev);
		return -ENODEV;
	}
	dev->initialized = 0;
	I("-\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id hippcom_of_id[] = {
	{.compatible = DTSNAME_HIPPCOM},
	{}
};
#endif

static struct platform_driver hippcom_pdrvr = {
	.probe          = hippcom_probe,
	.remove         = hippcom_remove,
	.driver         = {
		.name           = "hippcom",
		.owner          = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(hippcom_of_id),
#endif
	},
};

static int __init hippcom_init(void)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;
	pr_info("[%s] : +", __func__);
	dev = kzalloc(sizeof(struct hippcomdev_s), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;
	dev->initialized = 0;
	mutex_init(&dev->mutex_dev);
	mutex_init(&dev->mutex_smmu);
	mutex_init(&dev->mutex_power);
	mutex_init(&dev->mutex_jpgclk);
	ret = hippcom_earlydts_init(dev);
	if (ret != 0) {
		E("Failed : hippcom_earlydts_init.%d\n", ret);
		goto free_dev;
	}
	ippcomdev = dev;
	pr_info("[%s] : -", __func__);
	return 0;
free_dev:
	mutex_destroy(&dev->mutex_jpgclk);
	mutex_destroy(&dev->mutex_power);
	mutex_destroy(&dev->mutex_smmu);
	mutex_destroy(&dev->mutex_dev);
	kfree(dev);
	dev = NULL;
	return ret;
}

static void __exit hippcom_exit(void)
{
	struct hippcomdev_s *dev = ippcomdev;
	I("+\n");
	if (dev != NULL) {
		mutex_destroy(&dev->mutex_jpgclk);
		mutex_destroy(&dev->mutex_power);
		mutex_destroy(&dev->mutex_smmu);
		mutex_destroy(&dev->mutex_dev);
		kfree(dev);
	}
	ippcomdev = NULL;
	I("-\n");
}

module_platform_driver(hippcom_pdrvr);
subsys_initcall(hippcom_init);
module_exit(hippcom_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("IPP Common Driver");
MODULE_AUTHOR("ipp");
