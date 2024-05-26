/*
 * ISP CPE
 *
 * Copyright (c) 2017 IPP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of_irq.h>
#include <linux/iommu.h>
#include <linux/pm_wakeup.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/genalloc.h>
#include <linux/mm_iommu.h>
#include <linux/version.h>
#include <platform_include/isp/linux/hipp_common.h>
#include <linux/delay.h>
#include <linux/types.h>
#include "gf.h"
#include "orb.h"
#include "adapter_common.h"
#include "ipp.h"
#include "memory.h"
#include "ipp_core.h"

struct ipp_adapter_t {
	uint32_t cmd;
	int (*handler)(unsigned long args);
};

struct hispcpe_s *hispcpe_dev;

unsigned int kmsgcat_mask = (INFO_BIT | ERROR_BIT); /* DEBUG_BIT not set */
/*lint -e21 -e846 -e514 -e778 -e866 -e84 -e429 -e613 -e668*/
module_param_named(kmsgcat_mask, kmsgcat_mask, int, 0664);

static struct hispcpe_s *get_cpedev_bymisc(struct miscdevice *mdev)
{
	struct hispcpe_s *dev = NULL;
	if (mdev == NULL) {
		pr_err("[%s] Failed : mdev.%pK\n", __func__, mdev);
		return NULL;
	}
	dev = container_of(mdev, struct hispcpe_s, miscdev);
	return dev;
}

void __iomem *hipp_get_regaddr(unsigned int type)
{
	struct hispcpe_s *dev = hispcpe_dev;
	if (type >= hipp_min(MAX_HISP_CPE_REG, dev->reg_num)) {
		pr_err("[%s] unsupported type.0x%x\n", __func__, type);
		return NULL;
	}
	return dev->reg[type] ? dev->reg[type] : NULL; /*lint !e661 !e662 */
}

int get_hipp_smmu_info(int *sid, int *ssid)
{
	struct hispcpe_s *dev = hispcpe_dev;
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -EINVAL;
	}
	*sid = dev->sid;
	*ssid = dev->ssid;
	return 0;
}

int hispcpe_reg_set(unsigned int mode, unsigned int offset, unsigned int value)
{
	struct hispcpe_s *dev = hispcpe_dev;
	void __iomem *reg_base = NULL;
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -1;
	}
	if (mode > MAX_HISP_CPE_REG - 1) {
		pr_err("[%s] Failed : mode.%d\n", __func__, mode);
		return -1;
	}
	D("mode = %d, value = 0x%x\n", mode, value);
	reg_base = dev->reg[mode];
	if (reg_base == NULL) {
		pr_err("[%s] Failed : reg.NULL, mode.%d\n", __func__, mode);
		return -1;
	}
	writel(value, reg_base + offset);
	return ISP_IPP_OK;
}

unsigned int hispcpe_reg_get(unsigned int mode, unsigned int offset)
{
	struct hispcpe_s *dev = hispcpe_dev;
	unsigned int value;
	void __iomem *reg_base = NULL;
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return 1;
	}
	if (mode > MAX_HISP_CPE_REG - 1) {
		pr_err("[%s] Failed : mode.%d\n", __func__, mode);
		return ISP_IPP_OK;
	}
	reg_base = dev->reg[mode];
	if (reg_base == NULL) {
		pr_err("[%s] Failed : reg.NULL, mode.%d\n", __func__, mode);
		return 1;
	}
	value = readl(reg_base + offset);
	return value;
}

unsigned int get_clk_rate(unsigned int mode, unsigned int index)
{
	unsigned int type;
	unsigned int rate = 0;
	struct hispcpe_s *dev = hispcpe_dev;
	if ((mode > (IPP_CLK_MAX - 1)) || (index > (MAX_CLK_RATE - 1))) {
		pr_err("[%s] Failed : mode.%d, index.%d\n",
			__func__, mode, index);
		return rate;
	}
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
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

static int ipp_setclk_enable(struct hispcpe_s *dev, unsigned int devtype)
{
	int ret;
	int i = 0;
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -EINVAL;
	}
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
		dev->jpeg_rate[i] = get_clk_rate(IPPCORE_CLK, CLK_RATE_OFF);
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
	return ISP_IPP_OK;
}

static int ipp_setclk_disable(struct hispcpe_s *dev, unsigned int devtype)
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
	return ISP_IPP_OK;
}

int hipp_core_powerup(unsigned int type)
{
	int ret;
	struct hispcpe_s *dev = hispcpe_dev;
	if (dev == NULL)
		return -ENODEV;
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
	ret = hipp_adapter_cfg_qos_cvdr();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_adapter_cfg_qos_cvdr\n", __func__);
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

int hipp_core_powerdn(unsigned int type)
{
	int ret;
	struct hispcpe_s *dev = hispcpe_dev;
	if (dev == NULL) {
		pr_err("[%s] Failed: hispcpe_dev NULL\n", __func__);
		return -ENODEV;
	}
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
	return ISP_IPP_OK;
}

static int hispipp_cfg_check(unsigned long args)
{
	int ret;
	struct ipp_cfg_s ipp_cfg = { 0 };
	void __user *args_cfgcheck = (void __user *)(uintptr_t) args;
	pr_info("[%s] +\n", __func__);
	if (args_cfgcheck == NULL) {
		pr_err("[%s] args_cfgcheck.%pK\n", __func__, args_cfgcheck);
		return -EINVAL;
	}
	ipp_cfg.platform_version = 300;
	ipp_cfg.mapbuffer = MEM_HISPCPE_SIZE;
	ipp_cfg.mapbuffer_sec = MEM_HIPPTOF_SEC_SIZE;
	ret = copy_to_user(args_cfgcheck, &ipp_cfg, sizeof(struct ipp_cfg_s));
	if (ret != 0) {
		pr_err("[%s] copy_to_user.%d\n", __func__, ret);
		return -EFAULT;
	}
	pr_info("[%s] -\n", __func__);
	return ISP_IPP_OK;
}

static int is_need_transition_rate(
	unsigned int cur, unsigned int *tran_rate)
{
	int i = 0;
	for (i = 0; i < HIPP_TRAN_NUM; i++) {
		if (jpeg_trans_rate[i].source_rate == cur) {
			*tran_rate = jpeg_trans_rate[i].transition_rate;
			pr_info("[%s] : transition_rate.%d.%d M\n",
				__func__, *tran_rate / 1000000,
				*tran_rate % 1000000);
			return ISP_IPP_OK;
		}
	}
	return ISP_IPP_ERR;
}

static unsigned int hipp_set_rate_check(unsigned int devtype,
	unsigned int clktype)
{
	struct hispcpe_s *dev = hispcpe_dev;
	unsigned int set_rate;
	if (dev == NULL) {
		pr_err("[%s] Failed : hispcpe_dev.NULL\n", __func__);
		return 0;
	}
	if (clktype > (MAX_CLK_RATE - 1)) {
		pr_err("[%s] Failed : clktype.%d\n", __func__, clktype);
		return 0;
	}
	if (devtype > (MAX_HIPP_COM - 1)) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return 0;
	}
	set_rate = get_clk_rate(IPPCORE_CLK, clktype);
	return set_rate;
}

int hipp_set_rate(unsigned int devtype, unsigned int clktype)
{
	struct hispcpe_s *dev = hispcpe_dev;
	unsigned int set_rate;
	unsigned int max_rate;
	unsigned int transition_rate = 0;
	int ret;
	unsigned int jpeg_rate_buf[MAX_HIPP_COM] = { 0 };
	int i = 0;
	set_rate = hipp_set_rate_check(devtype, clktype);
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
		return ISP_IPP_OK;
	}
	ret = is_need_transition_rate(dev->jpeg_current_rate, &transition_rate);
	if (ret == 0) {
		ret = clk_set_rate(dev->ippclk[IPPCORE_CLK], transition_rate);
		if (ret < 0) {
			pr_err("[%s] Failed: set transition_rate\n", __func__);
			return ISP_IPP_ERR;
		}
		dev->jpeg_current_rate = transition_rate;
	}
	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK], set_rate);
	if (ret < 0) {
		pr_err("[%s] Failed: set_rate.%d.%d M\n",
			__func__, set_rate / 1000000, set_rate % 1000000);
		return ISP_IPP_ERR;
	}
	dev->jpeg_current_rate = set_rate;
	dev->jpeg_rate[devtype] = set_rate;
	return ISP_IPP_OK;
}

void relax_ipp_wakelock(void)
{
	struct hispcpe_s *dev = hispcpe_dev;
	if (dev == NULL)
		return;
	mutex_lock(&dev->ipp_wakelock_mutex);
	if (dev->ipp_wakelock->active) {
		__pm_relax(&dev->ipp_wakelock);
		pr_info("ipp power up wake unlock.\n");
	}
	mutex_unlock(&dev->ipp_wakelock_mutex);
}

static int hipp_cmd_pwrup(unsigned long args)
{
	struct power_para_s para = { 0 };
	struct hispcpe_s *dev = hispcpe_dev;
	void __user *args_pwup = NULL;
	int ret = 0;
	args_pwup = (void __user *)(uintptr_t)args;
	if (args_pwup == NULL) {
		pr_err("[%s] args_pwup.NULL\n", __func__);
		return -EINVAL;
	}
	ret = copy_from_user(&para, args_pwup, sizeof(struct power_para_s));
	if (ret < 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}
	pr_info("[%s]: HISP_IPP_PWRUP.pw_flag.%d\n", __func__, para.pw_flag);
	mutex_lock(&dev->ipp_wakelock_mutex);
	if (!dev->ipp_wakelock->active) {
		__pm_stay_awake(&dev->ipp_wakelock);
		pr_info("ipp power up wake lock.\n");
	}
	mutex_unlock(&dev->ipp_wakelock_mutex);
	ret = hipp_powerup();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_powerup.%d\n", __func__, ret);
		relax_ipp_wakelock();
	}
	return ret;
}

static int hipp_cmd_pwrdn(unsigned long args)
{
	int ret = 0;
	ret = hipp_powerdn();
	if (ret < 0)
		pr_err("[%s] Failed : hipp_powerdn.%d\n", __func__, ret);
	relax_ipp_wakelock();
	return ret;
}

static int hipp_cmd_cfg_check(unsigned long args)
{
	int ret;
	ret = hispipp_cfg_check(args);
	if (ret < 0)
		pr_err("[%s] Failed : hispipp_cfg_check.%d\n", __func__, ret);
	return ret;
}

static int hipp_cmd_map_buf(unsigned long args)
{
	int ret;
	ret = hispcpe_map_kernel(args);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_map_kernel.%d\n",	__func__, ret);
		return ret;
	}
	ret = cpe_init_memory();
	if (ret < 0) {
		pr_err("[%s] Failed : cpe_init_memory.%d\n", __func__, ret);
		ret = hispcpe_unmap_kernel();
		if (ret != 0)
			pr_err("[%s] Failed : hispcpe_umap_kernel.%d\n",
				__func__, ret);
		return -EFAULT;
	}
	return 0;
}

static int hipp_cmd_unmap_buf(unsigned long args)
{
	int ret;
	ret = hispcpe_unmap_kernel();
	if (ret < 0)
		pr_err("[%s] Failed: hispcpe_umap_kernel.%d\n", __func__, ret);
	return ret;
}

static int hipp_cmd_map_iommu(unsigned long args)
{
	void __user *map_iommu = NULL;
	struct memory_block_s buf = { 0 };
	int ret;
	map_iommu = (void __user *)(uintptr_t)args;
	if (map_iommu == NULL) {
		pr_err("[%s] args_map_iommu.NULL\n", __func__);
		return -EINVAL;
	}
	ret = copy_from_user(&buf, map_iommu, sizeof(struct memory_block_s));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}
	ret = hipp_adapter_map_iommu(&buf);
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_adapter_map_iommu.%d\n",
			__func__, ret);
		return ret;
	}
	ret = copy_to_user(map_iommu, &buf, sizeof(struct memory_block_s));
	if (ret != 0) {
		pr_err("[%s] copy_to_user.%d\n", __func__, ret);
		return -EFAULT;
	}
	return 0;
}

static int hipp_cmd_unmap_iommu(unsigned long args)
{
	void __user *unmap_iommu = NULL;
	struct memory_block_s buf = {0};
	int ret = 0;
	unmap_iommu = (void __user *)(uintptr_t)args;
	if (unmap_iommu == NULL) {
		pr_err("[%s] args_unmap_iommu.NULL\n", __func__);
		return -EINVAL;
	}
	ret = copy_from_user(&buf, unmap_iommu,	sizeof(struct memory_block_s));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}
	ret = hipp_adapter_unmap_iommu(&buf);
	if (ret < 0) {
		pr_err("[%s] Failed : hipp_adapter_unmap_iommu.%d\n",
			__func__, ret);
		return -EFAULT;
	}
	return 0;
}

static const struct ipp_adapter_t g_ipp_adapt[] = {
	{HISP_IPP_PWRUP, hipp_cmd_pwrup},
	{HISP_IPP_PWRDN, hipp_cmd_pwrdn},
	{HISP_IPP_GF_REQ, hipp_cmd_gf_req},
	{HISP_IPP_ORB_REQ, hipp_cmd_orb_req},
	{HISP_IPP_CFG_CHECK, hipp_cmd_cfg_check},
	{HISP_IPP_MAP_BUF, hipp_cmd_map_buf},
	{HISP_IPP_UNMAP_BUF, hipp_cmd_unmap_buf},
	{HISP_CPE_MAP_IOMMU, hipp_cmd_map_iommu},
	{HISP_CPE_UNMAP_IOMMU, hipp_cmd_unmap_iommu}
};

static long hispcpe_ioctl(struct file *filp,
	unsigned int cmd, unsigned long args)
{
	int ret = 0;
	unsigned int nums;
	unsigned int index;
	struct hispcpe_s *dev = NULL;
	int (*filter)(unsigned long args) = NULL;
	dev = get_cpedev_bymisc((struct miscdevice *)filp->private_data);
	if (dev != hispcpe_dev) {
		pr_err("[%s] Failed : dev.invalid\n", __func__);
		return -EINVAL;
	}
	if (dev->initialized == 0) {
		pr_err("[%s] Failed : IPP Device Not Exist.0\n", __func__);
		return -ENXIO;
	}
	nums = ARRAY_SIZE(g_ipp_adapt);
	for (index = 0; index < nums; index++) {
		if (g_ipp_adapt[index].cmd == cmd) {
			filter = g_ipp_adapt[index].handler;
			break;
		}
	}
	if (filter == NULL) {
		pr_err("[%s] Failed : not supported.0x%x\n", __func__, cmd);
		return -EINVAL;
	}
	ret = filter(args);
	return ret;
}

static int hispcpe_open(struct inode *inode, struct file *filp)
{
	struct hispcpe_s *dev = NULL;
	D("+\n");
	dev = get_cpedev_bymisc((struct miscdevice *)filp->private_data);
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}
	if (!dev->initialized) {
		pr_err("[%s] Failed : CPE Device Not Exist.%d\n",
			__func__, dev->initialized);
		return -ENXIO;
	}
	if (atomic_read(&dev->open_refs) != 0) {
		pr_err("[%s] Failed: Opened, open_refs.0x%x\n",
			__func__, atomic_read(&dev->open_refs));
		return -EBUSY;
	}
	atomic_inc(&dev->open_refs);
	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_release(struct inode *inode, struct file *filp)
{
	struct hispcpe_s *dev = NULL;
	D("+\n");
	dev = get_cpedev_bymisc((struct miscdevice *)filp->private_data);
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}
	if (!dev->initialized) {
		pr_err("[%s] Failed : CPE Device Not Exist.%d\n",
			__func__, dev->initialized);
		return -ENXIO;
	}
	if (atomic_read(&dev->open_refs) <= 0) {
		pr_err("[%s] Failed: Closed, open_refs.0x%x\n",
			__func__, atomic_read(&dev->open_refs));
		return -EBUSY;
	}
	atomic_set(&dev->open_refs, 0);
	hipp_adapter_exception();
	D("-\n");
	return ISP_IPP_OK;
}

static const struct file_operations hispcpe_fops = {
	.owner          = THIS_MODULE,
	.open           = hispcpe_open,
	.release        = hispcpe_release,
	.unlocked_ioctl = hispcpe_ioctl,
};

static struct miscdevice hispcpe_miscdev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name   = KBUILD_MODNAME,
	.fops   = &hispcpe_fops,
};

static int hispcpe_register_irq(struct hispcpe_s *dev)
{
	int i = 0, ret = 0;
	unsigned int min;
	min = hipp_min(MAX_HISP_CPE_IRQ, dev->irq_num);
	for (i = 0; i < min; i++) { /*lint !e574 */
		pr_info("[%s] : Hipp.%d, IRQ.%d\n", __func__, i, dev->irq[i]);
		if (i == CPE_IRQ_0)
			ret = hipp_adapter_register_irq(dev->irq[i]);
		if (ret != 0) {
			pr_err("[%s] Failed : %d.request_irq.%d\n",
				__func__, i, ret);
			return ret;
		}
	}
	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_ioremap_reg(struct hispcpe_s *dev)
{
	struct device *device = NULL;
	int i = 0;
	unsigned int min;
	D("+\n");
	device = &dev->pdev->dev;
	min = hipp_min(MAX_HISP_CPE_REG, dev->reg_num);
	for (i = 0; i < min; i++) { /*lint !e574 */
		dev->reg[i] = devm_ioremap_resource(device, dev->r[i]);
		if (dev->reg[i] == NULL) {
			pr_err("[%s] Failed : %d.devm_ioremap_resource.%pK\n",
				__func__, i, dev->reg[i]);
			return -ENOMEM;
		}
	}
	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_resource_init(struct hispcpe_s *dev)
{
	int ret;
	ret = hispcpe_register_irq(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_register_irq.%d\n",
			__func__, ret);
		return ret;
	}
	ret = hispcpe_ioremap_reg(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_ioremap_reg.%d\n",
			__func__, ret);
		return ret;
	}
	return ISP_IPP_OK;
}

static int hispcpe_getdts_pwr(struct hispcpe_s *dev)
{
	struct device *device = NULL;
	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}
	device = &dev->pdev->dev;
	dev->media2_supply = devm_regulator_get(device, "ipp-media2");
	if (IS_ERR(dev->media2_supply)) {
		pr_err("[%s] Failed : MEDIA2 devm_regulator_get.%pK\n",
			__func__, dev->media2_supply);
		return -EINVAL;
	}
	dev->cpe_supply = devm_regulator_get(device, "ipp-cpe");
	if (IS_ERR(dev->cpe_supply)) {
		pr_err("[%s] Failed : CPE devm_regulator_get.%pK\n",
			__func__, dev->cpe_supply);
		return -EINVAL;
	}
	dev->smmu_tcu_supply = devm_regulator_get(device, "ipp-smmu-tcu");
	if (IS_ERR(dev->smmu_tcu_supply)) {
		pr_err("[%s] Failed : smmu tcu devm_regulator_get.%pK\n",
			__func__, dev->smmu_tcu_supply);
		return -EINVAL;
	}
	pr_info("[%s] Hisp CPE cpe_supply.%pK\n", __func__, dev->cpe_supply);
	dev->ippclk[IPPCORE_CLK] = devm_clk_get(device, "clk_jpg_func");
	if (IS_ERR(dev->ippclk[IPPCORE_CLK])) {
		pr_err("[%s] Failed: get jpg_clk", __func__);
		return -EINVAL;
	}
	return 0;
}

static int hispcpe_getdts_clk(struct hispcpe_s *dev)
{
	struct device_node *np = NULL;
	int ret;
	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}
	np = dev->pdev->dev.of_node;
	if (np == NULL) {
		pr_err("%s: of node NULL", __func__);
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "clock-num",
			(unsigned int *)(&dev->clock_num));
	if (ret < 0) {
		pr_err("[%s] Failed: clock-num.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32_array(np, "clock-value",
			dev->ippclk_value, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock-value.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32_array(np, "clock-value-normal",
			dev->ippclk_normal, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock nor.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32_array(np, "clock-value-hsvs",
			dev->ippclk_hsvs, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock hsvs.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32_array(np, "clock-value-svs",
			dev->ippclk_svs, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock svs.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32_array(np, "clock-value-off",
			dev->ippclk_off, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock off.%d\n", __func__, ret);
		return -EINVAL;
	}
	return 0;
}

int hispcpe_get_irq(unsigned int index)
{
	struct device_node *np = NULL;
	char *name = DTS_NAME_IPP;
	int irq;
	np = of_find_compatible_node(NULL, NULL, name);
	if (np == NULL) {
		pr_err("[%s] Failed : %s.of_find_compatible_node.%pK\n",
			__func__, name, np);
		return -ENXIO;
	}
	irq = irq_of_parse_and_map(np, index);
	if (!irq) {
		pr_err("[%s] Failed : irq_map.%d\n", __func__, irq);
		return -ENXIO;
	}
	pr_info("%s: comp.%s, cpe irq.%d.\n", __func__, name, irq);
	return irq;
}

static int hispcpe_getdts_irq(struct hispcpe_s *dev)
{
	struct device *device = NULL;
	unsigned int i;
	int irq;
	int ret;
	unsigned int min;
	D("+\n");
	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}
	device = &dev->pdev->dev;
	ret = of_property_read_u32(device->of_node, "irq-num",
			(unsigned int *)(&dev->irq_num));
	if (ret < 0) {
		pr_err("[%s] Failed: irq-num.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("[%s] Hisp irq_num.%d\n", __func__, dev->irq_num);
	min = hipp_min(MAX_HISP_CPE_IRQ, dev->irq_num);
	for (i = 0; i < min; i++) {
		irq = hispcpe_get_irq(i);
		if (irq <= 0) {
			pr_err("[%s] Failed : platform_get_irq.%d\n",
				__func__, irq);
			return -EINVAL;
		}
		dev->irq[i] = irq;
		pr_info("[%s] Hisp CPE %d.IRQ.%d\n", __func__, i, dev->irq[i]);
	}
	I("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_getdts_reg(struct hispcpe_s *dev)
{
	struct device *device = NULL;
	int i = 0, ret;
	unsigned int min;
	D("+\n");
	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}
	device = &dev->pdev->dev;
	ret = of_property_read_u32(device->of_node, "reg-num",
			(unsigned int *)(&dev->reg_num));
	if (ret < 0) {
		pr_err("[%s] Failed: reg-num.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("[%s] Hipp reg_num.%d\n", __func__, dev->reg_num);
	min = hipp_min(MAX_HISP_CPE_REG, dev->reg_num);
	for (i = 0; i < min; i++) { /*lint !e574 */
		dev->r[i] = platform_get_resource(dev->pdev, IORESOURCE_MEM, i);
		if (dev->r[i] == NULL) {
			pr_err("[%s] Failed : platform_get_resource.%pK\n",
				__func__, dev->r[i]);
			return -ENXIO;
		}
	}
	ret = of_property_read_u32(device->of_node, "sid-num",
			(unsigned int *)(&dev->sid));
	if (ret < 0) {
		pr_err("[%s] Failed: ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32(device->of_node, "ssid-num",
			(unsigned int *)(&dev->ssid));
	if (ret < 0) {
		pr_err("[%s] Failed: ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	I("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_getdts(struct hispcpe_s *dev)
{
	int ret;
	ret = hispcpe_getdts_pwr(dev);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_getdts_pwr.%d\n", __func__, ret);
		return ret;
	}
	ret = hispcpe_getdts_clk(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_getdts_clk.%d\n", __func__, ret);
		return ret;
	}
	ret = hispcpe_getdts_irq(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : irq.%d\n", __func__, ret);
		return ret;
	}
	ret = hispcpe_getdts_reg(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : reg.%d\n", __func__, ret);
		return ret;
	}
	return ISP_IPP_OK;
}

static int hispcpe_attach_misc(struct hispcpe_s *dev,
	struct miscdevice *mdev)
{
	I("+\n");
	if (dev == NULL || mdev == NULL) {
		pr_err("[%s] Failed : dev.%pK, mdev.%pK\n",
			__func__, dev, mdev);
		return -EINVAL;
	}
	dev->miscdev = hispcpe_miscdev;
	I("-\n");
	return ISP_IPP_OK;
}

//lint -save -e454 -e455
int ippdev_lock(void)
{
	D("+\n");
	if (hispcpe_dev == NULL) {
		pr_err("[%s], NONE cpe_mem_info!\n", __func__);
		return ISP_IPP_ERR;
	}
	mutex_lock(&hispcpe_dev->dev_lock);
	D("-\n");
	return ISP_IPP_OK;
}

int ippdev_unlock(void)
{
	D("+\n");
	if (hispcpe_dev == NULL) {
		pr_err("[%s], NONE cpe_mem_info!\n", __func__);
		return ISP_IPP_ERR;
	}
	mutex_unlock(&hispcpe_dev->dev_lock);
	D("-\n");
	return ISP_IPP_OK;
}
//lint -restore

static int hipp_client_probe(struct platform_device *pdev)
{
	int ret;
	ret = hipp_adapter_probe(pdev);
	if (ret < 0) {
		pr_err("[%s] Failed : hipp_adapter_probe.%d\n", __func__, ret);
		return ret;
	}
	return 0;
}

static void hipp_client_remove(void)
{
	hipp_adapter_remove();
}

static int hispcpe_probe(struct platform_device *pdev)
{
	struct hispcpe_s *dev = NULL;
	int ret;
	pr_info("[%s] +\n", __func__);
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));//lint !e598
	if (ret < 0) {
		pr_err("[%s] Failed : dma_set\n", __func__);
		return ret;
	}
	dev = kzalloc(sizeof(struct hispcpe_s), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;
	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);
	ret = hipp_client_probe(pdev);
	if (ret < 0) {
		pr_err("[%s] Failed : hipp_adapter_probe.%d\n", __func__, ret);
		goto free_dev;
	}
	ret = hispcpe_getdts(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_getdts.%d\n", __func__, ret);
		goto clean_ipp_client;
	}
	ret = hispcpe_resource_init(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : resource.%d\n", __func__, ret);
		goto clean_ipp_client;
	}
	dev->initialized = 0;
	hispcpe_attach_misc(dev, &hispcpe_miscdev);
	ret = misc_register((struct miscdevice *)&dev->miscdev);
	if (ret != 0) {
		pr_err("[%s] Failed : misc_register.%d\n", __func__, ret);
		goto clean_ipp_client;
	}
	atomic_set(&dev->open_refs, 0);
	mutex_init(&dev->dev_lock);
	dev->ipp_wakelock = wakeup_source_register(&pdev->dev, "ipp_wakelock");
	if (!dev->ipp_wakelock) {
		pr_err("ipp_wakelock register fail\n");
		goto clean_ipp_client;
	}
	mutex_init(&dev->ipp_wakelock_mutex);
	dev->initialized = 1;
	hispcpe_dev = dev;
	pr_info("[%s] -\n", __func__);
	return ISP_IPP_OK;
clean_ipp_client:
	hipp_client_remove();
free_dev:
	kfree(dev);
	return -ENODEV;
}

static int hispcpe_remove(struct platform_device *pdev)
{
	struct hispcpe_s *dev = NULL;
	I("+\n");
	dev = (struct hispcpe_s *)platform_get_drvdata(pdev);
	if (dev == NULL) {
		pr_err("[%s] Failed : drvdata, pdev.%pK\n", __func__, pdev);
		return -ENODEV;
	}
	misc_deregister(&dev->miscdev);
	atomic_set(&dev->open_refs, 0);
	wakeup_source_unregister(dev->ipp_wakelock);
	mutex_destroy(&dev->ipp_wakelock_mutex);
	hipp_client_remove();
	dev->initialized = 0;
	kfree(dev);
	dev = NULL;
	I("-\n");
	return ISP_IPP_OK;
}

#ifdef CONFIG_OF
static const struct of_device_id ipp_of_id[] = {
	{.compatible = DTS_NAME_IPP},
	{}
};
#endif

static struct platform_driver hispcpe_pdrvr = {
	.probe          = hispcpe_probe,
	.remove         = hispcpe_remove,
	.driver         = {
		.name           = "ipp",
		.owner          = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ipp_of_id),
#endif
	},
};

module_platform_driver(hispcpe_pdrvr);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ISP CPE Driver");
MODULE_AUTHOR("isp");

/*lint +e21 +e846 +e514 +e778 +e866 +e84 +e429 +e613 +e668*/
