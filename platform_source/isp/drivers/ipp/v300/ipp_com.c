/*
 * Hisilicon IPP Common Driver
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
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
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/genalloc.h>
#include <linux/uaccess.h>
#include <linux/mm_iommu.h>
#include <platform_include/isp/linux/hipp_common.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/syscalls.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include "ipp_smc.h"

#define DTSNAME_HIPPCOM    "hisilicon,ippcom"

#define IPP_DEV_MASK 0x0F

enum HIPP_CLK_TYPE {
	IPPCORE_CLK = 0,
	IPP_CLK_MAX
};

enum hipp_clk_rate_type_e {
	HIPPCORE_RATE_TBR = 0,
	HIPPCORE_RATE_NOR = 1,
	HIPPCORE_RATE_HSVS = 2,
	HIPPCORE_RATE_SVS = 3,
	HIPPCORE_RATE_OFF = 4,
	MAX_HIPP_CLKRATE_TYPE
};

#define HIPP_TRAN_NUM 4
struct transition_info {
	unsigned int source_rate;
	unsigned int transition_rate;
};

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
	struct regulator *media2_supply;
	struct regulator *cpe_supply;
	struct regulator *smmu_tcu_supply;

	unsigned int clock_num;
	struct clk *ippclk[IPP_CLK_MAX];
	unsigned int ippclk_value[IPP_CLK_MAX];
	unsigned int ippclk_normal[IPP_CLK_MAX];
	unsigned int ippclk_hsvs[IPP_CLK_MAX];
	unsigned int ippclk_svs[IPP_CLK_MAX];
	unsigned int ippclk_off[IPP_CLK_MAX];
	unsigned int jpeg_current_rate;
	unsigned int jpeg_rate[MAX_HIPP_COM];
};

static struct hippcomdev_s *ippcomdev;

int __attribute__((weak)) hipp_special_cfg(void)
{
	return 0;
}

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
		pr_err("[%s] Failed : drv.NULL\n", __func__);
		return NULL;
	}

	ret = invalid_ipptype(drv->type);
	if (ret < 0) {
		pr_err("[%s] Failed : invalid_ipptype.%d\n", __func__, ret);
		return NULL;
	}

	dev = (struct hippcomdev_s *)drv->comdev;
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return NULL;
	}

	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ippcom not initialized\n", __func__);
		return NULL;
	}

	if (drv != dev->drv[drv->type]) {
		pr_err("[%s] Failed : Mismatch drv type.%d\n", __func__, drv->type);
		return NULL;
	}

	return dev;
}

static void do_ippdump(struct hippcomdev_s *dev)
{
	struct hipp_common_s *drv = NULL;
	int i = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return;
	}

	pr_info("[%s] : refs_smmu.0x%x, initialized.0x%x\n",
		__func__, dev->refs_smmu[HIPP_TBU_IPP], dev->initialized);

	for (i = 0; i < MAX_HIPP_COM; i++) {
		drv = dev->drv[i];

		if (drv == NULL) {
			pr_info("[%s] : index.%d Not Registered\n", __func__, i);
			continue;
		}

		pr_info("[%s] : index.%d: %s.type.0x%x, mode.0x%x",
			__func__, i, drv->name, drv->type, drv->tbu_mode);
	}
}

static int hippsmmu_enable(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;

	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
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
			pr_err("[%s] Failed : atfhipp_smmu_enable, ret.%d\n",
				__func__, ret);
			mutex_unlock(&dev->mutex_smmu);
			return ret;
		}
	}

	dev->refs_smmu[drv->tbu_mode]++;
	pr_info("[%s] : refs_smmu mode.%d, refs.%d\n",
		__func__, drv->tbu_mode, dev->refs_smmu[drv->tbu_mode]);
	mutex_unlock(&dev->mutex_smmu);
	return 0;
}

static int hippsmmu_disable(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;

	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("[%s] : + %s\n", __func__, drv->name);

	if (dev->refs_power == 0) {
		pr_err("[%s] Failed : ippsubsys powerdown\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&dev->mutex_smmu);

	if (dev->refs_smmu[drv->tbu_mode] == 0) {
		pr_err("[%s]: smmu no need to disable,refs_smmu.%d\n",
			__func__, dev->refs_smmu[drv->tbu_mode]);
		mutex_unlock(&dev->mutex_smmu);
		return 0;
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

	pr_info("[%s]: refs_smmu mode.%d, num .%d\n",
		__func__, drv->tbu_mode, dev->refs_smmu[drv->tbu_mode]);
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
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
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

	return 0;
}

static int hippsmmu_setpref(struct hipp_common_s *drv,
	unsigned int swid, unsigned int len)
{
	struct hippcomdev_s *dev = NULL;
	int ret;

	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
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

	return 0;
}

static void hipp_dump(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;

	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
		return;
	}

	mutex_lock(&dev->mutex_smmu);
	do_ippdump(dev);
	mutex_unlock(&dev->mutex_smmu);
}

static unsigned int get_clk_rate(struct hippcomdev_s *dev,
	unsigned int mode, unsigned int index)
{
	unsigned int type;
	unsigned int rate = 0;

	if ((mode > (IPP_CLK_MAX - 1)) || (index > (MAX_CLK_RATE - 1))) {
		pr_err("[%s] Failed : mode.%d, index.%d\n",
			__func__, mode, index);
		return rate;
	}

	type = mode * MAX_CLK_RATE + index;

	switch (type) {
	case HIPPCORE_RATE_TBR:
		rate = dev->ippclk_value[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_NOR:
		rate = dev->ippclk_normal[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_HSVS:
		rate = dev->ippclk_hsvs[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_SVS:
		rate = dev->ippclk_svs[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_OFF:
		rate = dev->ippclk_off[IPPCORE_CLK];
		break;

	default:
		break;
	}

	return rate;
}

static int ipp_setclk_enable(struct hippcomdev_s *dev, unsigned int devtype)
{
	int ret;
	int i = 0;

	if (dev->ippclk[IPPCORE_CLK] == NULL) {
		pr_err("[%s] Failed : jpgclk.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippclk_off[IPPCORE_CLK] == 0) {
		pr_err("[%s] Failed : jpgclk.value.0\n", __func__);
		return -EINVAL;
	}

	if (devtype > MAX_HIPP_COM - 1) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return -EINVAL;
	}

	for (i = 0; i < MAX_HIPP_COM; i++)
		dev->jpeg_rate[i] = get_clk_rate(dev, IPPCORE_CLK, CLK_RATE_OFF);

	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK],
			   dev->ippclk_off[IPPCORE_CLK]);
	if (ret < 0) {
		pr_err("[%s] Failed: clk_set_rate %d.%d\n",
			__func__, dev->ippclk_off[IPPCORE_CLK], ret);
		return -EINVAL;
	}

	ret = clk_prepare_enable(dev->ippclk[IPPCORE_CLK]);
	if (ret < 0) {
		pr_err("[%s] Failed: clk_prepare_enable.%d\n", __func__, ret);
		return -EINVAL;
	}

	dev->jpeg_current_rate = dev->ippclk_off[IPPCORE_CLK];
	dev->jpeg_rate[devtype] = dev->ippclk_off[IPPCORE_CLK];

	return 0;
}

static int ipp_setclk_disable(struct hippcomdev_s *dev, unsigned int devtype)
{
	int ret;
	unsigned int i = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippclk[IPPCORE_CLK] == NULL) {
		pr_err("[%s] Failed : jpgclk.NULL\n", __func__);
		return -EINVAL;
	}

	if (devtype > MAX_HIPP_COM - 1) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return -EINVAL;
	}

	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK],
			   dev->ippclk_off[IPPCORE_CLK]);
	if (ret < 0) {
		pr_err("[%s] Failed: clk_set_rate %d.%d\n",
			__func__, dev->ippclk_off[IPPCORE_CLK], ret);
		return ret;
	}

	clk_disable_unprepare(dev->ippclk[IPPCORE_CLK]);
	dev->jpeg_rate[devtype] = dev->ippclk_off[IPPCORE_CLK];
	dev->jpeg_current_rate = dev->ippclk_off[IPPCORE_CLK];

	for (i = 0; i < MAX_HIPP_COM; i++) {
		if (dev->jpeg_rate[i] != dev->ippclk_off[IPPCORE_CLK])
			pr_err("[%s] ippclk_check: type.%d, rate.%d.%d M\n",
				__func__, i, dev->jpeg_rate[i] / 1000000,
				dev->jpeg_rate[i] % 1000000);
	}

	return 0;
}

static int hipp_core_powerup(struct hippcomdev_s *dev, unsigned int type)
{
	int ret;

	pr_info("%s: +\n", __func__);

	ret = regulator_enable(dev->media2_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: media2.%d\n", __func__, ret);
		return -ENODEV;
	}

	ret = regulator_enable(dev->smmu_tcu_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: smmu tcu.%d\n", __func__, ret);
		goto fail_tcu_poweron;
	}

	ret = ipp_setclk_enable(dev, type);
	if (ret < 0) {
		pr_err("[%s] Failed: ipp_setclk_enable.%d\n", __func__, ret);
		goto fail_ipp_setclk;
	}

	ret = regulator_enable(dev->cpe_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: cpe.%d\n", __func__, ret);
		goto fail_cpe_supply;
	}

	ret = hipp_special_cfg();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_special_cfg\n", __func__);
		goto fail_ipp_cfg;
	}

	return 0;

fail_ipp_cfg:
	ret = regulator_disable(dev->cpe_supply);
	if (ret != 0)
		pr_err("[%s]Failed: cpe disable.%d\n", __func__, ret);

fail_cpe_supply:
	ret = ipp_setclk_disable(dev, type);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_setclk_disable.%d\n", __func__, ret);

fail_ipp_setclk:
	ret = regulator_disable(dev->smmu_tcu_supply);
	if (ret != 0)
		pr_err("[%s] Failed: smmu tcu disable.%d\n", __func__, ret);

fail_tcu_poweron:
	ret = regulator_disable(dev->media2_supply);
	if (ret != 0)
		pr_err("[%s] Failed: media2 disable.%d\n", __func__, ret);

	return -EINVAL;
}

int hipp_smmuv3_pu(struct hipp_common_s *drv)
{
	int ret = 0;
	struct hippcomdev_s *dev = NULL;

	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&dev->mutex_power);

	if (dev->refs_power == 0) {
		ret = hipp_core_powerup(dev, drv->type);
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

static int hipp_core_powerdn(struct hippcomdev_s *dev, unsigned int type)
{
	int ret;

	ret = regulator_disable(dev->cpe_supply);
	if (ret != 0)
		pr_err("[%s] Failed: orb regulator_disable.%d\n",
			__func__, ret);

	ret = ipp_setclk_disable(dev, type);
	if (ret < 0)
		pr_err("[%s] Failed: ipp_setclk_enable.%d\n",
			__func__, ret);

	ret = regulator_disable(dev->smmu_tcu_supply);
	if (ret != 0)
		pr_err("[%s] Failed: smmu tcu regulator_enable.%d\n",
			__func__, ret);

	ret = regulator_disable(dev->media2_supply);
	if (ret != 0)
		pr_err("[%s] Failed: vcodec regulator_enable.%d\n",
			__func__, ret);

	return 0;
}

int hipp_smmuv3_pd(struct hipp_common_s *drv)
{
	struct hippcomdev_s *dev = NULL;
	int ret = 0;

	dev = hipp_drv2dev(drv);
	if (dev == NULL) {
		pr_err("[%s] Failed : hipp_drv2dev.NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&dev->mutex_power);
	if (dev->refs_power == 0) {
		pr_err("[%s] : ipp no need to pn,refs_power.%d\n",
			__func__, dev->refs_power);
		mutex_unlock(&dev->mutex_power);
		return 0;
	}

	dev->refs_power--;

	if (dev->refs_power == 0) {
		ret = hipp_core_powerdn(dev, drv->type);
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

static int is_need_transition_rate(
	unsigned int cur, unsigned int *tran_rate)
{
	int i = 0;
	struct transition_info jpeg_trans_rate[HIPP_TRAN_NUM] = {
		{640000000, 214000000},
		{480000000, 160000000},
		{400000000, 200000000},
	};

	for (i = 0; i < HIPP_TRAN_NUM; i++) {
		if (jpeg_trans_rate[i].source_rate == cur) {
			*tran_rate = jpeg_trans_rate[i].transition_rate;
			pr_info("[%s] : transition_rate.%d.%d M\n",
			       __func__, *tran_rate / 1000000,
			       *tran_rate % 1000000);

			return 0;
		}
	}

	return -1;
}

static unsigned int hipp_set_rate_check(struct hippcomdev_s *dev,
	unsigned int devtype, unsigned int clktype)
{
	unsigned int set_rate;

	if (clktype > (MAX_CLK_RATE - 1)) {
		pr_err("[%s] Failed : clktype.%d\n", __func__, clktype);
		return 0;
	}

	if (devtype > (MAX_HIPP_COM - 1)) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return 0;
	}

	set_rate = get_clk_rate(dev, IPPCORE_CLK, clktype);

	return set_rate;
}

int hipp_set_rate(struct hippcomdev_s *dev, unsigned int devtype, unsigned int clktype)
{
	unsigned int set_rate;
	unsigned int max_rate;
	unsigned int transition_rate = 0;
	int ret;
	unsigned int jpeg_rate_buf[MAX_HIPP_COM] = { 0 };
	int i = 0;

	set_rate = hipp_set_rate_check(dev, devtype, clktype);
	if (set_rate == 0) {
		pr_err("[%s] Failed : hipp_set_rate_check\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < MAX_HIPP_COM; i++)
		jpeg_rate_buf[i] = dev->jpeg_rate[i];

	jpeg_rate_buf[devtype] = set_rate;
	max_rate = jpeg_rate_buf[0];

	for (i = 1; i < MAX_HIPP_COM; i++) {
		if (max_rate < jpeg_rate_buf[i])
			max_rate = jpeg_rate_buf[i];
	}

	pr_info("[%s]: set_rate.%d.%dM, jpg_cur_rate.%d.%dM, max_rate.%d.%dM\n",
	     __func__, set_rate / 1000000, set_rate % 1000000,
	     dev->jpeg_current_rate / 1000000, dev->jpeg_current_rate % 1000000,
	     max_rate / 1000000, max_rate % 1000000);

	if (max_rate == dev->jpeg_current_rate) {
		pr_info("[%s] : no need change jpgclk.\n", __func__);
		dev->jpeg_rate[devtype] = set_rate;
		return 0;
	}

	ret = is_need_transition_rate(dev->jpeg_current_rate, &transition_rate);
	if (ret == 0) {
		ret = clk_set_rate(dev->ippclk[IPPCORE_CLK], transition_rate);
		if (ret < 0) {
			pr_err("[%s] Failed: set transition_rate\n", __func__);
			return ret;
		}

		dev->jpeg_current_rate = transition_rate;
	}

	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK], set_rate);
	if (ret < 0) {
		pr_err("[%s] Failed: set_rate.%d.%d M\n",
			__func__, set_rate / 1000000, set_rate % 1000000);
		return ret;
	}

	dev->jpeg_current_rate = set_rate;
	dev->jpeg_rate[devtype] = set_rate;
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

	ret = hipp_set_rate(dev, drv->type, clktype);
	if (ret < 0) {
		pr_err("[%s] Failed : hipp_set_rate ret.%d\n", __func__, ret);
		mutex_unlock(&dev->mutex_jpgclk);
		return ret;
	}

	mutex_unlock(&dev->mutex_jpgclk);
	return 0;
}

static unsigned long hipp_smmuv3_iommu_map(struct hipp_common_s *drv,
	struct dma_buf *modulebuf, int prot, unsigned long *out_size)
{
	unsigned long iova;
	unsigned long size = 0;
	int i = 0;
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

	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get, buf.%pK\n",
			__func__, modulebuf);
		return 0;
	}

	iova = kernel_iommu_map_dmabuf(drv->dev, modulebuf, prot, &size);
	if (iova == 0) {
		pr_err("[%s] Failed : kernel_iommu_map_dmabuf\n", __func__);
		return 0;
	}

	*out_size = size;
	pr_info("[%s] : iova.0x%lx, size.0x%lx\n", __func__, iova, size);

	return iova;
}

static int hipp_smmuv3_iommu_unmap(struct hipp_common_s *drv,
	struct dma_buf *modulebuf, unsigned long iova)
{
	int ret;
	int i = 0;
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

	if (iova == 0) {
		pr_err("[%s] Failed : iova.0\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get, buf.%pK\n",
			__func__, modulebuf);
		return -EINVAL;
	}

	ret = kernel_iommu_unmap_dmabuf(drv->dev, modulebuf, iova);
	if (ret < 0)
		pr_err("[%s] Failed : kernel_iommu_unmap_dmabuf, ret.%d\n",
			__func__, ret);

	return ret;
}

static void *hipp_map_kernel(struct hipp_common_s *drv, struct dma_buf *modulebuf)
{
	void *virt_addr = NULL;
	int ret;

	pr_info("[%s] enter\n", __func__);

	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get, modulebuf.%pK\n",
			__func__, modulebuf);
		return NULL;
	}

	ret = dma_buf_begin_cpu_access(modulebuf, DMA_FROM_DEVICE);
	if (ret < 0) {
		pr_err("[%s] dma access failed: ret.%d\n", __func__, ret);
		return NULL;
	}

	virt_addr = dma_buf_kmap(modulebuf, 0);
	if (virt_addr == NULL) {
		pr_err("[%s] Failed : dma_buf_kmap.%pK\n", __func__, virt_addr);
		goto err_dma_buf_kmap;
	}

	return virt_addr;

err_dma_buf_kmap:
	ret = dma_buf_end_cpu_access(modulebuf, DMA_FROM_DEVICE);
	if (ret < 0)
		pr_err("[%s] dma access failed: ret.%d\n", __func__, ret);

	return NULL;
}

static int hipp_unmap_kernel(struct hipp_common_s *drv,
	struct dma_buf *modulebuf, void *virt_addr)
{
	int ret;

	pr_info("[%s] +\n", __func__);

	if (virt_addr == NULL) {
		pr_err("[%s] Failed : virt_addr.NULL\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : modulebuf.%pK\n", __func__, modulebuf);
		return -EINVAL;
	}

	dma_buf_kunmap(modulebuf, 0, virt_addr);
	ret = dma_buf_end_cpu_access(modulebuf, DMA_FROM_DEVICE);
	if (ret < 0)
		pr_err("[%s] Failed : dma access ret.%d\n", __func__, ret);

	pr_info("[%s] -\n", __func__);

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
	if (ret < 0)
		return ret;

	if (dev  == NULL) {
		pr_err("[%s]Failed : dev.NULL\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&dev->mutex_dev);
	if (dev->drv[type] == NULL) {
		pr_err("[%s] Failed : %d has not been Registered\n",
			__func__, type);
		mutex_unlock(&dev->mutex_dev);
		return -EINVAL;
	}

	pr_debug("[%s] : drv name.%s\n", __func__, dev->drv[type]->name);
	kfree(dev->drv[type]);
	dev->drv[type] = NULL;
	mutex_unlock(&dev->mutex_dev);

	return 0;
}

static int hippcom_earlydts_init(struct hippcomdev_s *dev)
{
	struct device_node *np = NULL;
	char *name = DTSNAME_HIPPCOM;
	int ret;

	if (dev == NULL) {
		pr_err("[%s] Failed : hipp com dev.NULL\n", __func__);
		return -ENODEV;
	}

	np = of_find_compatible_node(NULL, NULL, name);
	if (np == NULL) {
		pr_err("[%s] Failed : %s.of_node.NULL\n", __func__, name);
		return -ENXIO;
	}

	ret = of_property_read_u32(np, "drv-num", (unsigned int *)(&dev->drvnum));
	if (ret != 0) {
		pr_err("[%s] Failed: drv-num property.%d\n", __func__, ret);
		return -EINVAL;
	}

	if (dev->drvnum > MAX_HIPP_COM) {
		pr_err("[%s] Failed: drvnum.%d > %d\n",
			__func__, dev->drvnum, MAX_HIPP_COM);
		return -EINVAL;
	}

	ret = of_property_read_string_array(np, "drv-names",
		dev->drvname, dev->drvnum);
	if (ret < dev->drvnum) { //lint !e574
		pr_err("[%s] Failed : drv-names string_array.%d < %d\n",
			__func__, ret, dev->drvnum);
		return -EINVAL;
	}

	return 0;
}

static int hispcpe_getdts_pwr(struct platform_device *pdev,
	struct hippcomdev_s *comdev)
{
	struct device *dev = NULL;

	dev = &pdev->dev;
	if (dev == NULL) {
		pr_err("[%s] Failed : dev NULL\n", __func__);
		return -ENXIO;
	}

	comdev->media2_supply = devm_regulator_get(dev, "ipp-media2");
	if (IS_ERR_OR_NULL(comdev->media2_supply)) {
		pr_err("[%s] Failed : MEDIA2 devm_regulator_get.%pK\n",
			__func__, comdev->media2_supply);
		return -EINVAL;
	}

	comdev->cpe_supply = devm_regulator_get(dev, "ipp-cpe");
	if (IS_ERR_OR_NULL(comdev->cpe_supply)) {
		pr_err("[%s] Failed : CPE devm_regulator_get.%pK\n",
			__func__, comdev->cpe_supply);
		return -EINVAL;
	}

	comdev->smmu_tcu_supply = devm_regulator_get(dev, "ipp-smmu-tcu");
	if (IS_ERR_OR_NULL(comdev->smmu_tcu_supply)) {
		pr_err("[%s] Failed : smmu tcu devm_regulator_get.%pK\n",
			__func__, comdev->smmu_tcu_supply);
		return -EINVAL;
	}

	comdev->ippclk[IPPCORE_CLK] = devm_clk_get(dev, "clk_jpg_func");
	if (IS_ERR_OR_NULL(comdev->ippclk[IPPCORE_CLK])) {
		pr_err("[%s] Failed: get jpg_clk", __func__);
		return -EINVAL;
	}

	pr_info("[%s] -\n", __func__);

	return 0;
}

static int hispcpe_getdts_clk(struct platform_device *pdev,
	struct hippcomdev_s *comdev)
{
	struct device_node *np = NULL;
	int ret;

	np = pdev->dev.of_node;
	if (np == NULL) {
		pr_err("%s: of node NULL", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "clock-num",
		(unsigned int *)(&comdev->clock_num));
	if (ret < 0) {
		pr_err("[%s] Failed: clock-num.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value",
		comdev->ippclk_value, comdev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock-value.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-normal",
		comdev->ippclk_normal, comdev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock nor.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-hsvs",
		comdev->ippclk_hsvs, comdev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock hsvs.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-svs",
		comdev->ippclk_svs, comdev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock svs.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-off",
		comdev->ippclk_off, comdev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock off.%d\n", __func__, ret);
		return -EINVAL;
	}

	pr_info("[%s] -\n", __func__);
	return 0;
}

static int hippcom_probe(struct platform_device *pdev)
{
	int ret;
	struct hippcomdev_s *dev = NULL;

	pr_info("%s: +\n", __func__);

	if (pdev == NULL)
		return -ENXIO;

	dev = ippcomdev;
	if (dev == NULL) {
		pr_err("[%s] Failed : g_ippcomdev.NULL\n", __func__);
		return -ENOMEM;
	}

	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);

	ret = hispcpe_getdts_pwr(pdev, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: hispcpe_getdts_pwr.%d", __func__, ret);
		return ret;
	}

	ret = hispcpe_getdts_clk(pdev, dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_getdts_clk.%d\n", __func__, ret);
		return ret;
	}

	dev->initialized = 1;
	pr_info("%s: -\n", __func__);

	return 0;
}

static int hippcom_remove(struct platform_device *pdev)
{
	struct hippcomdev_s *dev = NULL;

	dev = (struct hippcomdev_s *)platform_get_drvdata(pdev);
	if (dev == NULL) {
		pr_err("[%s] Failed : platform_get_drvdata.NULL\n", __func__);
		return -ENODEV;
	}

	dev->initialized = 0;

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
		pr_err("[%s] Failed : hippcom_earlydts_init.%d\n", __func__, ret);
		goto free_dev;
	}

	ippcomdev = dev;
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


	if (dev != NULL) {
		mutex_destroy(&dev->mutex_jpgclk);
		mutex_destroy(&dev->mutex_power);
		mutex_destroy(&dev->mutex_smmu);
		mutex_destroy(&dev->mutex_dev);
		kfree(dev);
	}

	ippcomdev = NULL;
}

module_platform_driver(hippcom_pdrvr);
subsys_initcall(hippcom_init);
module_exit(hippcom_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Hisilicon IPP Common Driver");
MODULE_AUTHOR("ipp");
