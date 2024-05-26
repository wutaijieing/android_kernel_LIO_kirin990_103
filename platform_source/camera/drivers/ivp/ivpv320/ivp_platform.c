/*
 * This file contains api of control ivp, eg power on/off, qos
 * setup clk setup, get mem info etc
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

#include "ivp_platform.h"
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "ivp_log.h"
#include "ivp_reg.h"
#include "ivp_atf.h"
#include "securec.h"
#include "ivp_manager.h"

#define REMAP_ADD                        0xE9100000
#define DEAD_FLAG                        0xDEADBEEF
#define SIZE_16K                         (16 * 1024)

u32 noc_ivp_reg_read(struct ivp_device *ivp_devp, unsigned int off)
{
	char __iomem *reg = ivp_devp->io_res.noc_ivp_base_addr + off;
	u32 val = readl(reg);
	return val;
}

void noc_ivp_reg_write(struct ivp_device *ivp_devp, unsigned int off, u32 val)
{
	char __iomem *reg = ivp_devp->io_res.noc_ivp_base_addr + off;

	writel(val, reg);
}
static int ivp_get_dynimic_buff(struct ivp_device *ivp_devp)
{
	int index;
	int ret;
	dma_addr_t phys_addr = 0;
	struct ivp_common *ivp_comm = &ivp_devp->ivp_comm;

	ivp_comm->vaddr_memory = NULL;
	/*lint -save -e598 -e648*/
	ret = dma_set_mask_and_coherent(&ivp_comm->ivp_pdev->dev,
		DMA_BIT_MASK(DMA_64BIT));
	if (ret) {
		ivp_err("dma_set failed");
		return -EFAULT;
	}
	/*lint -restore*/
	ivp_comm->vaddr_memory = dma_alloc_coherent(&ivp_comm->ivp_pdev->dev,
		ivp_comm->dynamic_mem_size, &phys_addr, GFP_KERNEL);
	if (!ivp_comm->vaddr_memory) {
		ivp_err("get vaddr_memory failed");
		return -EINVAL;
	}
	ivp_comm->sects[BASE_SECT_INDEX].acpu_addr = phys_addr >> IVP_MMAP_SHIFT;
	ivp_info("BASE_SECT_INDEX addr:0x%lx", ivp_comm->sects[BASE_SECT_INDEX].acpu_addr);

	for (index = BASE_SECT_INDEX + 1; index < ivp_comm->sect_count; index++) {
		ivp_comm->sects[index].acpu_addr =
			((ivp_comm->sects[index - 1].acpu_addr << IVP_MMAP_SHIFT) +
			ivp_comm->sects[index - 1].len) >> IVP_MMAP_SHIFT;
		ivp_dbg("ivp sections 0x%lx", ivp_comm->sects[index].acpu_addr);
	}

	return ret;
}

int ivp_get_memory_section(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp)
{
	int ret;
	int index;
	unsigned int size = 0;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, (!ivp_devp || !plat_dev),
		"invalid input param ivp_devp or plat_dev");

	ivp_comm = &ivp_devp->ivp_comm;
	ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_DYNAMIC_MEM, &size);
	if (ret || !size) {
		ivp_err("get failed/not use dynamic mem, ret:%d", ret);
		return ret;
	}
	ivp_comm->dynamic_mem_size = size;

	ivp_comm->ivp_meminddr_len = (int)size;

	ret = ivp_get_dynimic_buff(ivp_devp);
	if (ret)
		ivp_err("ivp_get_dynimic_buff fail %d", ret);

	for (index = DRAM0_SECT_INDEX; index < ivp_comm->sect_count; index++)
		ivp_info("acpu addr:0x%lx, ivp addr:0x%x, length:0x%x",
			ivp_comm->sects[index].acpu_addr, ivp_comm->sects[index].ivp_addr,
			ivp_comm->sects[index].len);
	return ret;
}

void ivp_free_memory_section(struct ivp_device *ivp_devp)
{
	dma_addr_t dma_addr;
	struct ivp_common *ivp_comm = NULL;

	if (!ivp_devp) {
		ivp_err("invalid input param ivp_devp");
		return;
	}

	ivp_comm = &ivp_devp->ivp_comm;

	dma_addr = ivp_comm->sects[BASE_SECT_INDEX].acpu_addr << IVP_MMAP_SHIFT;

	if (ivp_comm->vaddr_memory) {
		dma_free_coherent(&ivp_comm->ivp_pdev->dev,
			ivp_comm->dynamic_mem_size,
			ivp_comm->vaddr_memory, dma_addr);
		ivp_comm->vaddr_memory = NULL;
	}
}

static void ivp_hw_remap_ivp2ddr(struct ivp_device *ivp_devp,
	unsigned int ivp_addr, unsigned int len, unsigned long ddr_addr)
{
	unsigned int ivp_addr_base = ((ivp_addr / SIZE_1MB) << IVP_ADDR_SHIFT) +
		(ivp_addr / SIZE_1MB);
	unsigned int ddr_addr_base = (u32)(((ddr_addr / SIZE_1MB) << IVP_ADDR_SHIFT) +
		(ddr_addr / SIZE_1MB)); /* reg is 32bit */
	unsigned int remap_len = (len << IVP_REMP_LEN_SHIFT) + len;

	ivp_dbg("ivp addr:0x%x remap:0x%x acpu_addr:0x%x",
		ivp_addr_base, remap_len, ddr_addr_base);
	ivp_reg_write(ivp_devp, ADDR_IVP_CFG_SEC_REG_START_REMAP_ADDR,
		ivp_addr_base);
	ivp_reg_write(ivp_devp, ADDR_IVP_CFG_SEC_REG_REMAP_LENGTH,
		remap_len);
	ivp_reg_write(ivp_devp, ADDR_IVP_CFG_SEC_REG_DDR_REMAP_ADDR,
		ddr_addr_base);
}

int ivp_remap_addr_ivp2ddr(struct ivp_device *ivp_devp,
	unsigned int ivp_addr, int len, unsigned long ddr_addr)
{
	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_dbg("ivp_addr:0x%x, len:0x%x, ddr_addr:0x%lx",
		ivp_addr, len, ddr_addr);
	if ((ivp_addr & MASK_1MB) != 0 ||
		(ddr_addr & MASK_1MB) != 0 ||
		len >= MAX_DDR_LEN * SIZE_1MB) {
		ivp_err("not aligned");
		return -EINVAL;
	}
	len = (len + SIZE_1MB - 1) / SIZE_1MB - 1;
	ivp_hw_remap_ivp2ddr(ivp_devp, ivp_addr, (u32)len, ddr_addr);

	return 0;
}
static int ivp_read_qos_cfg(struct ivp_device *ivp_devp)
{
	if (ivp_reg_read(ivp_devp, IVP_REG_OFF_IVP_SYSTEM_QOS_CFG) !=
		IVP_SYS_QOS_CFG_VALUE) {
		ivp_warn("IVP_REG_OFF_IVP_SYSTEM_QOS_CFG read back value:0x%x",
			ivp_reg_read(ivp_devp, IVP_REG_OFF_IVP_SYSTEM_QOS_CFG));
		return READ_BACK_IVP_SYS_QOS_CFG_ERROR;
	}

	return 0;
}

static void ivp_set_qos_cfg(struct ivp_device *ivp_devp)
{
	ivp_reg_write(ivp_devp, IVP_REG_OFF_IVP_SYSTEM_QOS_CFG,
		IVP_SYS_QOS_CFG_VALUE);
	if (ivp_read_qos_cfg(ivp_devp) != 0)
		ivp_err("ivp_read_qos_cfg failed, may get a perf problem");
}

int ivp_poweron_pri(struct ivp_device *ivp_devp)
{
	int ret;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_comm = &ivp_devp->ivp_comm;

	/* 0.Enable the power */
	if ((ret = regulator_enable(ivp_devp->ivp_media2_regulator)) != 0) {
		ivp_err("ivp_media2_regulator enable failed %d", ret);
		return ret;
	}
	if ((ret = regulator_enable(ivp_devp->ivp_smmu_tcu_regulator)) != 0) {
		ivp_err("ivp_smmu_tcu_regulator enable failed %d", ret);
		goto error_tcu_poweron;
	}

	/* 1.Set Clock rate */
	if ((ret = clk_set_rate(ivp_comm->clk,
		(unsigned long)ivp_devp->low_clk_rate)) != 0) {
		ivp_err("set rate %#x fail, ret:%d", ivp_devp->low_clk_rate, ret);
		goto err_clk_set_rate;
	}

	/* 2.Enable the clock */
	if ((ret = clk_prepare_enable(ivp_comm->clk)) != 0) {
		ivp_err("clk prepare enable failed, ret = %d", ret);
		goto err_clk_prepare_enable;
	}
	ivp_info("set core success to:%ld", clk_get_rate(ivp_comm->clk));
	/* 3.Enable the power */
	if ((ret = regulator_enable(ivp_comm->regulator)) != 0) {
		ivp_err("regularot enable failed %d", ret);
		goto err_regulator_enable_ivp;
	}
	ivp_set_qos_cfg(ivp_devp);

	return ret;

err_regulator_enable_ivp:
	clk_disable_unprepare(ivp_comm->clk);

err_clk_prepare_enable:
	if ((ret = clk_set_rate(ivp_comm->clk,
		(unsigned long)ivp_comm->lowfrq_pd_clk_rate)) != 0)
		ivp_err("err set lowfrq_pd_clk_rate rate %#x fail %d",
			ivp_comm->lowfrq_pd_clk_rate, ret);

err_clk_set_rate:
	if ((ret = regulator_disable(ivp_devp->ivp_smmu_tcu_regulator)) != 0)
		ivp_err("ivp_smmu_tcu_regulator disable failed %d", ret);

error_tcu_poweron:
	if ((ret = regulator_disable(ivp_devp->ivp_media2_regulator)) != 0)
		ivp_err("ivp_media2_regulator disable failed %d", ret);
	return -EINVAL;
}

int ivp_poweroff_pri(struct ivp_device *ivp_devp)
{
	int ret;
	int times = 0;
	unsigned int waiti;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_comm = &ivp_devp->ivp_comm;

	waiti = ivp_reg_read(ivp_devp, IVP_REG_OFF_PWAITMODE);
	while (((waiti & IVP_DSP_PWAITMODE) == 0) && (times < 3)) { /* 3 times */
		udelay(100); /* 100us */
		ivp_err("ivp core is not in wfi mode, 0x%x", waiti);
		waiti = ivp_reg_read(ivp_devp, IVP_REG_OFF_PWAITMODE);
		times++;
	}

	ret = regulator_disable(ivp_comm->regulator);
	if (ret)
		ivp_err("regulator power off failed %d", ret);

	ret = clk_set_rate(ivp_comm->clk, IVP_POWEROFF_TRANS_CLK_RATE);
	if (ret)
		ivp_warn("set poweroff transitional clk rate %#x fail, ret:%d",
			IVP_POWEROFF_TRANS_CLK_RATE, ret);

	ret = clk_set_rate(ivp_comm->clk,
		(unsigned long)ivp_comm->lowfrq_pd_clk_rate);
	if (ret)
		ivp_warn("set lfrq pd rate %#x fail, ret:%d",
			ivp_comm->lowfrq_pd_clk_rate, ret);

	clk_disable_unprepare(ivp_comm->clk);

	ret = regulator_disable(ivp_devp->ivp_smmu_tcu_regulator);
	if (ret)
		ivp_err("ivp_smmu_tcu_regulator power off failed %d", ret);

	ret = regulator_disable(ivp_devp->ivp_media2_regulator);
	if (ret)
		ivp_err("ivp_media2_regulator power off failed %d", ret);

	return ret;
}

int ivp_setup_regulator(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp)
{
	struct regulator *ivp_media2_regulator = NULL;
	struct regulator *regulator = NULL;

	loge_and_return_if_cond(-EINVAL, (!ivp_devp || !plat_dev),
		"invalid input param ivp_devp or plat_dev");

	regulator = devm_regulator_get(&plat_dev->dev, IVP_REGULATOR);
	if (IS_ERR(regulator)) {
		ivp_err("Get ivp regulator failed");
		return -ENODEV;
	} else {
		ivp_devp->ivp_comm.regulator = regulator;
	}

	ivp_media2_regulator = devm_regulator_get(&plat_dev->dev,
		IVP_MEDIA_REGULATOR);
	if (IS_ERR(ivp_media2_regulator)) {
		ivp_err("Get ivp media regulator failed");
		return -ENODEV;
	} else {
		ivp_devp->ivp_media2_regulator = ivp_media2_regulator;
	}

	ivp_devp->ivp_smmu_tcu_regulator = devm_regulator_get(&plat_dev->dev,
		IVP_SMMU_TCU_REGULATOR);
	if (IS_ERR(ivp_devp->ivp_smmu_tcu_regulator)) {
		ivp_err("Get ivp smmu tcu regulator failed");
		return -ENODEV;
	}

	return 0;
}

int ivp_setup_clk(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp)
{
	int ret;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, (!ivp_devp || !plat_dev),
		"invalid input param ivp_devp or plat_dev");

	ivp_comm = &ivp_devp->ivp_comm;
	ivp_comm->clk = devm_clk_get(&plat_dev->dev, OF_IVP_CLK_NAME);
	if (IS_ERR(ivp_comm->clk)) {
		ivp_err("get clk failed");
		return -ENODEV;
	}
#ifndef CONFIG_ES_LOW_FREQ
	ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_CLK_RATE_NAME, &ivp_comm->clk_rate);
#else
	ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_LOW_CLK_PU_RATE_NAME, &ivp_comm->clk_rate);
#endif
	if (ret) {
		ivp_err("get clk rate failed, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_info("get clk rate: %u", ivp_comm->clk_rate);

	if ((ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_MIDDLE_CLK_RATE_NAME, &ivp_devp->middle_clk_rate)) != 0) {
		ivp_err("get middle freq rate failed, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_info("get middle freq clk rate: %u", ivp_devp->middle_clk_rate);

	if ((ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_LOW_CLK_RATE_NAME, &ivp_devp->low_clk_rate)) != 0) {
		ivp_err("get low freq rate failed, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_info("get low freq clk rate: %u", ivp_devp->low_clk_rate);

	if ((ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_ULTRA_LOW_CLK_RATE_NAME,
		&ivp_devp->ultra_low_clk_rate)) != 0) {
		ivp_err("get ultra low freq clk rate, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_info("get ultra low freq clk rate: %u",
		ivp_devp->ultra_low_clk_rate);

	if ((ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_LOWFREQ_CLK_RATE_NAME, &ivp_comm->lowfrq_pd_clk_rate)) != 0) {
		ivp_err("get lowfreq pd clk rate failed, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_info("get lowfrq pd clk rate: %u", ivp_comm->lowfrq_pd_clk_rate);

	return ret;
}

int ivp_change_clk(struct ivp_device *ivp_devp, unsigned int level)
{
	int ret;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_comm = &ivp_devp->ivp_comm;

	ivp_info("change freq level from %u to %u", ivp_comm->clk_level, level);
	if (ivp_comm->clk_level == IVP_CLK_LEVEL_LOW) {
		ret = clk_set_rate(ivp_comm->clk,
			(unsigned long)ivp_comm->lowfrq_pd_clk_rate);
		if (ret != 0) {
			ivp_err("set low freq pd level rate %#x fail, ret:%d",
				ivp_comm->lowfrq_pd_clk_rate, ret);
			return ret;
		}
	}

	ivp_comm->clk_level = level;
	switch (ivp_comm->clk_level) {
	case IVP_CLK_LEVEL_ULTRA_LOW:
		ivp_info("ivp freq to ultra low level");
		ivp_comm->clk_usrsetrate = ivp_devp->ultra_low_clk_rate;
		break;

	case IVP_CLK_LEVEL_LOW:
		ivp_info("ivp freq to low level");
		ivp_comm->clk_usrsetrate = ivp_devp->low_clk_rate;
		break;

	case IVP_CLK_LEVEL_MEDIUM:
		ivp_info("ivp freq to media level");
		ivp_comm->clk_usrsetrate = ivp_devp->middle_clk_rate;
		break;

	case IVP_CLK_LEVEL_HIGH:
		ivp_info("ivp freq to high level");
		ivp_comm->clk_usrsetrate = ivp_comm->clk_rate;
		break;

	default:
		ivp_info("use default freq");
		ivp_comm->clk_usrsetrate = ivp_comm->clk_rate;
		break;
	}

	/* Set Clock rate */
	ivp_info("set clock rate");
	ret = clk_set_rate(ivp_comm->clk, (unsigned long)ivp_comm->clk_usrsetrate);
	if (!ret) {
		ivp_info("set core success to: %ld", clk_get_rate(ivp_comm->clk));
		return ret;
	}

	ivp_info("try set core freq to: %ld",
		(unsigned long)ivp_devp->low_clk_rate);
	ret = clk_set_rate(ivp_comm->clk,
		(unsigned long)ivp_devp->low_clk_rate);
	if (ret)
		ivp_err("set low rate %#x fail, ret:%d",
			ivp_devp->low_clk_rate, ret);

	return ret;
}
