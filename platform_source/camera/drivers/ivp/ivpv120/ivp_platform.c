/*
 * This file implements ivp initialization and control functions
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
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include "ivp_reg.h"
#include "ivp_log.h"
#include "ivp_manager.h"

int ivp_get_memory_section(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp)
{
	int i;
	int ret;
	unsigned int size = 0;
	dma_addr_t dma_addr = 0;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, (!ivp_devp || !plat_dev),
		"invalid input param ivp_devp or plat_dev");

	ivp_comm = &ivp_devp->ivp_comm;
	ivp_comm->vaddr_memory = NULL;
	ret = of_property_read_u32(plat_dev->dev.of_node, OF_IVP_DYNAMIC_MEM, &size);
	if (ret || !size) {
		ivp_err("get failed/not use dynamic mem, ret:%d, size:%u", ret, size);
		return -EINVAL;
	}
	ivp_comm->dynamic_mem_size = size;
	ivp_comm->ivp_meminddr_len = (int)size;

	ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_DYNAMIC_MEM_SEC_SIZE, &size);
	if (ret || !size) {
		ivp_err("get failed/not use dynamic mem, ret:%d, size:%u", ret, size);
		return -EINVAL;
	}

	ivp_comm->dynamic_mem_section_size = size;
	if ((ivp_comm->dynamic_mem_section_size *
		(unsigned int)(ivp_comm->sect_count - DDR_SECTION_INDEX)) !=
		ivp_comm->dynamic_mem_size) {
		ivp_err("dynamic_mem should be sect_count-3 times dynamic_mem_section");
		return -EINVAL;
	}

	/*lint -save -e598 -e648*/
	dma_set_mask_and_coherent(&ivp_comm->ivp_pdev->dev, DMA_BIT_MASK(DMA_64BIT));
	/*lint -restore*/
	ivp_comm->vaddr_memory = dma_alloc_coherent(&ivp_comm->ivp_pdev->dev,
		ivp_comm->dynamic_mem_size, &dma_addr, GFP_KERNEL);
	if (!ivp_comm->vaddr_memory) {
		ivp_err("get vaddr_memory failed");
		return -EINVAL;
	}

	for (i = DDR_SECTION_INDEX; i < ivp_comm->sect_count; i++) {
		if (i == DDR_SECTION_INDEX) {
			ivp_comm->sects[i].acpu_addr = dma_addr >> IVP_MMAP_SHIFT;
		} else {
			ivp_comm->sects[i].acpu_addr =
				((ivp_comm->sects[i - 1].acpu_addr << IVP_MMAP_SHIFT) +
				ivp_comm->sects[i - 1].len) >> IVP_MMAP_SHIFT;
			ivp_comm->sects[i].ivp_addr = ivp_comm->sects[i - 1].ivp_addr +
				ivp_comm->sects[i - 1].len;
		}
		ivp_comm->sects[i].len = ivp_comm->dynamic_mem_section_size;
		ivp_dbg("ivp sections 0x%lx", ivp_comm->sects[i].acpu_addr);
	}

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
	dma_addr = ivp_comm->sects[DDR_SECTION_INDEX].acpu_addr << IVP_MMAP_SHIFT;
	if (ivp_comm->vaddr_memory) {
		dma_free_coherent(&ivp_comm->ivp_pdev->dev,
			ivp_comm->dynamic_mem_size,
			ivp_comm->vaddr_memory, dma_addr);
		ivp_comm->vaddr_memory = NULL;
	}
}

static inline void ivp_hw_remap_ivp2ddr(struct ivp_device *ivp_devp,
	unsigned int ivp_addr, unsigned int len, unsigned long ddr_addr)
{
	ivp_reg_write(ivp_devp, ADDR_IVP_CFG_SEC_REG_START_REMAP_ADDR,
		ivp_addr / SIZE_1MB);
	ivp_reg_write(ivp_devp, ADDR_IVP_CFG_SEC_REG_REMAP_LENGTH, len);
	ivp_reg_write(ivp_devp, ADDR_IVP_CFG_SEC_REG_DDR_REMAP_ADDR,
		(u32)(ddr_addr / SIZE_1MB)); /* reg is 32bit */
}

int ivp_remap_addr_ivp2ddr(struct ivp_device *ivp_devp,
	unsigned int ivp_addr, int len, unsigned long ddr_addr)
{
	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_dbg("ivp_addr:%#x, len:%d, ddr_addr:%#lx",
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

int ivp_poweron_pri(struct ivp_device *ivp_devp)
{
	int ret;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_comm = &ivp_devp->ivp_comm;

	/* 0.Enable the power */
	ret = regulator_enable(ivp_devp->ivp_fake_regulator);
	if (ret) {
		ivp_err("ivp_fake_regulator enable failed %d", ret);
		return ret;
	}

	/* 1.Set Clock rate */
	ret = clk_set_rate(ivp_comm->clk, (unsigned long)ivp_comm->clk_rate);
	if (ret) {
		ivp_err("set rate %#x fail, ret:%d", ivp_comm->clk_rate, ret);
		goto err_clk_set_rate;
	}

	/* 2.Enable the clock */
	ret = clk_prepare_enable(ivp_comm->clk);
	if (ret) {
		ivp_err("clk prepare enable failed, ret = %d", ret);
		goto err_clk_set_rate;
	}

	ivp_info("set core success to: %ld", clk_get_rate(ivp_comm->clk));
	/* 3.Enable the power */
	ret = regulator_enable(ivp_comm->regulator);
	if (ret) {
		ivp_err("regularot enable failed %d", ret);
		goto err_regulator_enable_ivp;
	}

	ivp_reg_write(ivp_devp, IVP_REG_OFF_MEM_CTRL3, IVP_MEM_CTRL3_VAL);
	ivp_reg_write(ivp_devp, IVP_REG_OFF_MEM_CTRL4, IVP_MEM_CTRL4_VAL);

	return ret;

err_regulator_enable_ivp:
	clk_disable_unprepare(ivp_comm->clk);

err_clk_set_rate:
	ret = regulator_disable(ivp_devp->ivp_fake_regulator);
	if (ret)
		ivp_err("ivp_fake_regulator disable failed %d", ret);

	return -EINVAL;
}

int ivp_poweroff_pri(struct ivp_device *ivp_devp)
{
	int ret;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, !ivp_devp, "invalid input param ivp_devp");

	ivp_comm = &ivp_devp->ivp_comm;

	ivp_hw_runstall(ivp_devp, IVP_RUNSTALL_STALL);
	ivp_hw_enable_reset(ivp_devp);
	ret = regulator_disable(ivp_comm->regulator);
	if (ret)
		ivp_err("regulator power off failed %d", ret);

	ret = clk_set_rate(ivp_comm->clk,
		(unsigned long)ivp_comm->lowfrq_pd_clk_rate);
	if (ret)
		ivp_warn("set lfrq pd rate %#x fail, ret:%d",
			ivp_comm->lowfrq_pd_clk_rate, ret);

	clk_disable_unprepare(ivp_comm->clk);

	ret = regulator_disable(ivp_devp->ivp_fake_regulator);
	if (ret)
		ivp_err("ivp_fake_regulator power off failed %d", ret);

	return ret;
}

int ivp_setup_regulator(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp)
{
	struct regulator *ivp_fake_regulator = NULL;
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

	ivp_fake_regulator = devm_regulator_get(&plat_dev->dev,
		IVP_MEDIA_REGULATOR);
	if (IS_ERR(ivp_fake_regulator)) {
		ivp_err("Get ivp regulator failed");
		return -ENODEV;
	} else {
		ivp_devp->ivp_fake_regulator = ivp_fake_regulator;
	}

	return 0;
}

int ivp_setup_clk(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp)
{
	int ret;
	u32 clk_rate = 0;
	struct ivp_common *ivp_comm = NULL;

	loge_and_return_if_cond(-EINVAL, (!ivp_devp || !plat_dev),
		"invalid input param ivp_devp or plat_dev");

	ivp_comm = &ivp_devp->ivp_comm;
	ivp_comm->clk = devm_clk_get(&plat_dev->dev, OF_IVP_CLK_NAME);
	if (IS_ERR(ivp_comm->clk)) {
		ivp_err("get clk failed");
		return -ENODEV;
	}
	ret = of_property_read_u32(plat_dev->dev.of_node, OF_IVP_CLK_RATE_NAME,
		&clk_rate);
	if (ret) {
		ivp_err("get rate failed, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_comm->clk_rate = clk_rate;
	ivp_info("get clk rate: %u", clk_rate);

	ret = of_property_read_u32(plat_dev->dev.of_node,
		OF_IVP_LOWFREQ_CLK_RATE_NAME, &clk_rate);
	if (ret) {
		ivp_err("get rate failed, ret:%d", ret);
		return -ENOMEM;
	}
	ivp_comm->lowfrq_pd_clk_rate = clk_rate;
	ivp_info("get lowfrq pd clk rate: %u", clk_rate);

	return ret;
}

int ivp_change_clk(struct ivp_device *ivp_devp __attribute__((unused)),
	unsigned int level __attribute__((unused)))
{
	ivp_info("ivp change clk do nothing");
	return 0;
}
