/*
 * regulator.c
 *
 * This is for vdec regulator manager.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "omxvdec.h"
#include "regulator.h"

#include <linux/mm_iommu.h>
#include <linux/iommu.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>

#include "platform.h"
#ifdef CONFIG_VCODEC_FREQ_LEVEL
#include "soc_spec_info.h"
#endif

/*lint -e774*/
#define VDEC_REGULATOR_NAME     "ldo_vdec"
#define MEDIA_REGULATOR_NAME    "ldo_media"
#define VCODEC_CLOCK_NAME       "clk_vdec"
#define VCODEC_CLK_RATE         "dec_clk_rate"
#ifdef VCODEC_SMMUV300
#define SMMU_TCU_REGULATOR_NAME "ldo_smmu_tcu"
#endif
#ifdef PLATFORM_VCODECV520
#define DEFAULT_CLK_RATE "default_clk_rate"
#endif

static uint32_t  g_clock_values[] = {450000000, 300000000, 185000000, 166000000};
#ifdef LOWER_FREQUENCY_SUPPORT
static uint32_t  g_vdec_clk_rate_lower = 166000000;
#endif
static uint32_t  g_vdec_clk_rate_l  = 185000000;
static uint32_t  g_vdec_clk_rate_n  = 300000000;
static uint32_t  g_vdec_clk_rate_h  = 450000000;
static vcodec_bool g_vdec_power_on    = VCODEC_FALSE;

static struct  clk          *g_vdec_clk         = VCODEC_NULL;
static struct  regulator    *g_vdec_regulator   = VCODEC_NULL;
static struct  regulator    *g_media_regulator  = VCODEC_NULL;

struct mutex g_regulator_mutex;
static vfmw_dts_config_s g_dts_config;
static clk_rate_e g_resume_clk_type = VDEC_CLK_RATE_LOW;

#ifdef PLATFORM_VCODECV300
#define VDEC_QOS_MODE                0xE893000C
#define VDEC_QOS_BANDWIDTH           0xE8930010
#define VDEC_QOS_SATURATION          0xE8930014
#define VDEC_QOS_EXTCONTROL          0xE8930018

static uint32_t g_vdec_qos_mode        = 0x1;
static uint32_t g_vdec_qos_band_width   = 0x1000;
static uint32_t g_vdec_qos_saturation  = 0x20;
static uint32_t g_vdec_qos_ext_control  = 0x1;
#endif

#ifdef CONFIG_ES_VDEC_LOW_FREQ
static uint32_t g_vdec_low_freq        = 332000000;
#endif

#ifdef PLATFORM_VCODECV520
static g_default_clk_rate = 104000000;
#endif
#ifdef VCODEC_SMMUV300
static struct regulator *g_smmu_tcu_regulator = VCODEC_NULL;
struct regulator* get_tcu_regulator(void)
{
	return g_smmu_tcu_regulator;
}

static struct smmu_cfg_info g_smmu_cfg_info = { 0 };
struct smmu_cfg_info* get_smmu_cfg_info(void)
{
	return &g_smmu_cfg_info;
}
#endif

#ifdef VCODEC_VDEC_SMMU_SUPPORT
#ifndef VCODEC_SMMUV300
extern omx_vdec_entry g_omx_vdec_entry;
/* func: iommu enable intf */
static uint64_t vdec_get_smmu_base_phy(void)
{
	return (uint64_t)kernel_domain_get_ttbr(g_omx_vdec_entry.device);
}
#endif
#endif

static int32_t read_clock_rate_value(
	struct device_node *np, uint32_t index, uint32_t *clock)
{
	int32_t ret;

	ret = of_property_read_u32_index(np, VCODEC_CLK_RATE, index, clock);
	if (ret) {
		dprint(PRN_FATAL, "read clock rate[%d] failed\n", index);
		*clock = g_clock_values[index];
		return VCODEC_FAILURE;
	}

	return 0;
}

static int32_t vdec_init_clock_rate(struct device *dev)
{
	int32_t ret;
	struct clk *pvdec_clk  = VCODEC_NULL;

	pvdec_clk = devm_clk_get(dev, VCODEC_CLOCK_NAME);
	if (IS_ERR_OR_NULL(pvdec_clk)) {
		dprint(PRN_FATAL, "%s can not get clock\n", __func__);
		return VCODEC_FAILURE;
	}

	g_vdec_clk = pvdec_clk;
	ret  = read_clock_rate_value(dev->of_node, 0, &g_vdec_clk_rate_h);
	ret += read_clock_rate_value(dev->of_node, READ_CLOCK_RATE_INDEX_ONE, &g_vdec_clk_rate_n);
	ret += read_clock_rate_value(dev->of_node, READ_CLOCK_RATE_INDEX_TWO, &g_vdec_clk_rate_l);
#ifdef LOWER_FREQUENCY_SUPPORT
	ret += read_clock_rate_value(dev->of_node, READ_CLOCK_RATE_INDEX_THREE, &g_vdec_clk_rate_lower);
#endif
#ifdef PLATFORM_VCODECV520
	ret += of_property_read_u32(dev->of_node, DEFAULT_CLK_RATE, &g_default_clk_rate);
#endif
	return_fail_if_cond_is_ture(ret, "read clock failed");

#ifdef CONFIG_ES_VDEC_LOW_FREQ
	g_vdec_clk_rate_h = g_vdec_low_freq;
#endif

	return 0;
}

static int32_t vdec_get_dts_config_info(
	struct device *dev, vfmw_dts_config_s *dts_config)
{
	int32_t ret;
	struct device_node *np_crg = VCODEC_NULL;
	struct device_node *np     = dev->of_node;
	struct resource res;

	return_fail_if_cond_is_ture(dev->of_node == VCODEC_NULL,
		"device node is null");
	return_fail_if_cond_is_ture(dts_config == VCODEC_NULL,
		"dts config is null");

	dts_config->vdec_irq_num_norm = irq_of_parse_and_map(np, 0);
	return_fail_if_cond_is_ture(dts_config->vdec_irq_num_norm == 0,
		"get irq num failed");

	dts_config->vdec_irq_num_prot = VDEC_TRQ_NUM_PROT;
	dts_config->vdec_irq_num_safe = VDEC_TRQ_NUM_SAFE;

	/* Get reg base addr & size, return 0 if success */
	ret = of_address_to_resource(np, 0, &res);
	return_fail_if_cond_is_ture(ret, "of_address_to_resource failed");

	dts_config->vdh_reg_base_addr = res.start;
	dts_config->vdh_reg_range = resource_size(&res);

#ifdef VCODEC_VDEC_SMMU_SUPPORT
#ifndef VCODEC_SMMUV300
	/* Get reg base addr, return 0 if failed */
	dts_config->smmu_page_base_addr = vdec_get_smmu_base_phy();
	return_fail_if_cond_is_ture(dts_config->smmu_page_base_addr == 0,
		"get smmu base addr failed");
#endif
#endif

	np_crg = of_find_compatible_node(VCODEC_NULL, VCODEC_NULL, "hisilicon,crgctrl");
	return_fail_if_cond_is_ture(np_crg == NULL, "can't find crgctrl node");

	ret = of_address_to_resource(np_crg, 0, &res);
	return_fail_if_cond_is_ture(ret, "of_address_to_resource failed");
	dts_config->pericrg_reg_base_addr = res.start;

	/* Check if is FPGA platform */
	ret = of_property_read_u32(np, "vdec_fpga", &dts_config->is_fpga);
	if (ret) {
		dts_config->is_fpga = 0;
		dprint(PRN_ALWS, "current is not fpga\n");
	}

	ret = of_property_read_u32(np, "vdec_qos_mode",
		&dts_config->vdec_qos_mode);
	if (ret)
		dprint(PRN_ALWS, "vdecQosMode is %d\n", dts_config->vdec_qos_mode);

	ret = vdec_init_clock_rate(dev);
	return_fail_if_cond_is_ture(ret != 0, "init clock failed");

	return 0;
}

#ifdef VCODEC_SMMUV300
static int32_t vdec_get_smmu_config_info(struct device *dev, struct smmu_cfg_info *smmu_config)
{
	int32_t ret;
	struct device_node *np     = dev->of_node;
	return_fail_if_cond_is_ture(dev->of_node == VCODEC_NULL,
		"device node is null");
	return_fail_if_cond_is_ture(smmu_config == VCODEC_NULL,
		"smmu config is null");

	ret = of_property_read_u32(np, "smmu_tbu_offset", &smmu_config->smmu_tbu_offset);
	return_fail_if_cond_is_ture(ret, "failed to get smmu tbu offset");

	ret = of_property_read_u32(np, "smmu_sid_offset", &smmu_config->smmu_sid_offset);
	return_fail_if_cond_is_ture(ret, "failed to get smmu sid offset");

	ret = of_property_read_u32(np, "smmu_norm_sid", &smmu_config->smmu_norm_sid);
	return_fail_if_cond_is_ture(ret, "failed to get smmu norm sid");

	ret = of_property_read_u32(np, "smmu_need_cfg", &smmu_config->smmu_need_cfg);
	return_fail_if_cond_is_ture(ret, "failed to get smmu cfg flag");

	return 0;
}
#endif

/* SHARE FUNC */

/* func: regulator probe entry */
int32_t vdec_regulator_probe(struct device *dev)
{
	int32_t ret;

	g_vdec_regulator = VCODEC_NULL;
	g_media_regulator = VCODEC_NULL;

	if (dev == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s, invalid params", __func__);
		return VCODEC_FAILURE;
	}

	g_media_regulator = devm_regulator_get(dev, MEDIA_REGULATOR_NAME);
	if (g_media_regulator == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s get media regulator failed\n", __func__);
		return VCODEC_FAILURE;
	} else if (IS_ERR(g_media_regulator)) {
		dprint(PRN_FATAL, "%s get media regulator failed,error no :%ld\n", __func__, PTR_ERR(g_media_regulator));
		g_media_regulator = VCODEC_NULL;
		return VCODEC_FAILURE;
	}

	g_vdec_regulator = devm_regulator_get(dev, VDEC_REGULATOR_NAME);
	if (g_vdec_regulator == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s get regulator failed\n", __func__);
		return VCODEC_FAILURE;
	} else if (IS_ERR(g_vdec_regulator)) {
		dprint(PRN_FATAL, "%s get regulator failed, error no :%ld\n", __func__, PTR_ERR(g_vdec_regulator));
		goto clean_regulator;
	}

#ifdef VCODEC_SMMUV300
	g_smmu_tcu_regulator = devm_regulator_get(dev, SMMU_TCU_REGULATOR_NAME);
	if (IS_ERR_OR_NULL(g_smmu_tcu_regulator)) {
		dprint(PRN_FATAL, "%s get regulator failed\n", __func__);
		goto clean_regulator;
	}

	ret = vdec_get_smmu_config_info(dev, &g_smmu_cfg_info);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s get smmu config info failed\n", __func__);
		goto clean_regulator;
	}
#endif

	ret = memset_s(&g_dts_config, sizeof(g_dts_config), 0, sizeof(g_dts_config));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
		goto clean_regulator;
	}
	ret = vdec_get_dts_config_info(dev, &g_dts_config);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s Regulator_GetDtsConfigInfo failed\n", __func__);
		goto clean_regulator;
	}

	ret = vfmw_set_dts_config(&g_dts_config);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s vfmw_set_dts_config failed\n", __func__);
		goto clean_regulator;
	}
	vdec_init_mutex(&g_regulator_mutex);

	return 0;
clean_regulator:
#ifdef VCODEC_SMMUV300
	g_smmu_tcu_regulator = VCODEC_NULL;
#endif
	g_vdec_regulator = VCODEC_NULL;
	g_media_regulator = VCODEC_NULL;
	return VCODEC_FAILURE;
}

/* func: regulator deinitialize */
int32_t vdec_regulator_remove(struct device *dev)
{
	vdec_mutex_lock(&g_regulator_mutex);

	g_vdec_regulator  = VCODEC_NULL;
	g_media_regulator = VCODEC_NULL;
	g_vdec_clk        = VCODEC_NULL;

	vdec_mutex_unlock(&g_regulator_mutex);

	return 0;
}

#ifdef PLATFORM_VCODECV300
static int32_t vdec_config_qos(void)
{
	uint32_t *qos_addr = VCODEC_NULL;

	qos_addr = (uint32_t *)ioremap(VDEC_QOS_MODE, IORE_MAP_PARA);
	if (qos_addr == NULL) {
		dprint(PRN_FATAL, "ioremap VDEC_QOS_MODE reg failed!\n");
		return VCODEC_FAILURE;
	}
	writel(g_vdec_qos_mode, qos_addr);
	iounmap(qos_addr);

	qos_addr = (uint32_t *)ioremap(VDEC_QOS_BANDWIDTH, IORE_MAP_PARA);
	if (qos_addr == NULL) {
		dprint(PRN_FATAL, "ioremap VDEC_QOS_BANDWIDTH reg failed!\n");
		return VCODEC_FAILURE;
	}
	writel(g_vdec_qos_band_width, qos_addr);
	iounmap(qos_addr);

	qos_addr = (uint32_t *)ioremap(VDEC_QOS_SATURATION, IORE_MAP_PARA);
	if (qos_addr == NULL) {
		dprint(PRN_FATAL, "ioremap VDEC_QOS_SATURATION reg failed!\n");
		return VCODEC_FAILURE;
	}
	writel(g_vdec_qos_saturation, qos_addr);
	iounmap(qos_addr);

	qos_addr = (uint32_t *)ioremap(VDEC_QOS_EXTCONTROL, IORE_MAP_PARA);
	if (qos_addr == NULL) {
		dprint(PRN_FATAL, "ioremap VDEC_QOS_EXTCONTROL reg failed!\n");
		return VCODEC_FAILURE;
	}
	writel(g_vdec_qos_ext_control, qos_addr);
	iounmap(qos_addr);

	return 0;
}
#endif

/* func: enable regulator */
int32_t vdec_regulator_enable(void)
{
	int32_t ret;

	vdec_mutex_lock(&g_regulator_mutex);

	if (g_vdec_power_on == VCODEC_TRUE) {
		vdec_mutex_unlock(&g_regulator_mutex);
		return 0;
	}

	if (g_vdec_clk == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s: invalid g_vdec_clk is NULL\n", __func__);
		goto error_exit;
	}

	if (IS_ERR_OR_NULL(g_media_regulator)) {
		dprint(PRN_FATAL, "%s : g_media_regulator is NULL", __func__);
		goto error_exit;
	}

	if (IS_ERR_OR_NULL(g_vdec_regulator)) {
		dprint(PRN_FATAL, "%s : g_vdec_regulator is NULL", __func__);
		goto error_exit;
	}
	ret = regulator_enable(g_media_regulator);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s enable media regulator failed\n", __func__);
		goto error_exit;
	}
	ret = clk_prepare_enable(g_vdec_clk);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s clk_prepare_enable failed\n", __func__);
		goto error_regulator_disable;
	}
#ifndef PLATFORM_VCODECV520
	ret = clk_set_rate(g_vdec_clk, g_vdec_clk_rate_l);
	if (ret) {
		dprint(PRN_FATAL, "%s Failed to clk_set_rate:%u, return %d\n", __func__, g_vdec_clk_rate_l, ret);
		goto error_unprepare_clk;
	}
#endif
	ret = regulator_enable(g_vdec_regulator);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s enable regulator failed\n", __func__);
		goto error_unprepare_clk;
	}

#ifdef PLATFORM_VCODECV300
	// If vdec_config_qos fail, it only effects performance
	ret = vdec_config_qos();
	if (ret != 0)
		dprint(PRN_FATAL, "%s config qos failed\n", __func__);
#endif

	dprint(PRN_ALWS, "vdec regulator enable\n");

	g_vdec_power_on = VCODEC_TRUE;

	vdec_mutex_unlock(&g_regulator_mutex);

	return 0;

error_unprepare_clk:
	clk_disable_unprepare(g_vdec_clk);

error_regulator_disable:
	if (regulator_disable(g_media_regulator))
		dprint(PRN_ALWS, "%s disable media regulator failed\n", __func__);
error_exit:
	vdec_mutex_unlock(&g_regulator_mutex);

	return VCODEC_FAILURE;
}

/* func: disable regulator */
int32_t vdec_regulator_disable(void)
{
	int32_t ret;
	uint32_t vdec_clk_rate;

	vdec_mutex_lock(&g_regulator_mutex);

	if (g_vdec_power_on == VCODEC_FALSE) {
		vdec_mutex_unlock(&g_regulator_mutex);
		return 0;
	}

	if (g_vdec_clk == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s g_vdec_clk is NULL\n", __func__);
		goto error_exit;
	}

	if (IS_ERR_OR_NULL(g_media_regulator)) {
		dprint(PRN_FATAL, "%s : g_media_regulator is NULL", __func__);
		goto error_exit;
	}

	if (IS_ERR_OR_NULL(g_vdec_regulator)) {
		dprint(PRN_FATAL, "%s : g_vdec_regulator is NULL", __func__);
		goto error_exit;
	}

	ret = regulator_disable(g_vdec_regulator);
	if (ret != 0)
		dprint(PRN_FATAL, "%s disable regulator failed\n", __func__);

#ifdef PLATFORM_VCODECV520
	vdec_clk_rate = g_default_clk_rate;
#elif defined LOWER_FREQUENCY_SUPPORT
	vdec_clk_rate = g_vdec_clk_rate_lower;
#else
	vdec_clk_rate = g_vdec_clk_rate_l;
#endif
	ret  = clk_set_rate(g_vdec_clk, vdec_clk_rate);
	if (ret)
		dprint(PRN_FATAL, "%s Failed to clk_set_rate:%u, return %d\n", __func__, vdec_clk_rate, ret);

	clk_disable_unprepare(g_vdec_clk);

	ret = regulator_disable(g_media_regulator);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s disable media regulator failed\n", __func__);
		goto error_exit;
	}

	g_vdec_power_on = VCODEC_FALSE;

	dprint(PRN_ALWS, "vdec regulator disable\n");

	vdec_mutex_unlock(&g_regulator_mutex);

	return 0;

error_exit:
	vdec_mutex_unlock(&g_regulator_mutex);

	return VCODEC_FAILURE;
}

/* func: get decoder clock rate */
void vdec_regulator_get_clk_rate(clk_rate_e *clk_rate)
{
	vdec_mutex_lock(&g_regulator_mutex);
	*clk_rate = g_resume_clk_type;
	vdec_mutex_unlock(&g_regulator_mutex);
}
int32_t vdec_regulator_set_clk_rate(clk_rate_e e_clk_rate)
{
	int32_t ret;
	uint32_t current_clk_rate;
	uint32_t need_clk_rate      = 0;
	clk_rate_e need_clk_type;

	vdec_mutex_lock(&g_regulator_mutex);

	if (IS_ERR_OR_NULL(g_vdec_clk)) {
		dprint(PRN_ERROR,  "Couldn't get clk [%s]\n", __func__);
		goto error_exit;
	}

	switch (e_clk_rate) {
#ifdef LOWER_FREQUENCY_SUPPORT
	case VDEC_CLK_RATE_LOWER:
		need_clk_rate = g_vdec_clk_rate_lower;
		break;
#endif
	case VDEC_CLK_RATE_LOW:
		need_clk_rate = g_vdec_clk_rate_l;
		break;

	case VDEC_CLK_RATE_NORMAL:
		need_clk_rate = g_vdec_clk_rate_n;
		break;

	case VDEC_CLK_RATE_HIGH:
		need_clk_rate = g_vdec_clk_rate_h;
		break;

	default:
		dprint(PRN_ERROR, "[%s] unsupport clk rate enum %d\n", __func__, e_clk_rate);
		goto error_exit;
	}

#ifdef CONFIG_VCODEC_FREQ_LEVEL
	if (get_module_freq_level(kernel, peri) == FREQ_LEVEL2 && need_clk_rate == g_vdec_clk_rate_h)
		need_clk_rate = g_vdec_clk_rate_n;
#endif

	current_clk_rate = (uint32_t) clk_get_rate(g_vdec_clk);
	if (need_clk_rate != current_clk_rate) {
		need_clk_type = e_clk_rate;
		ret = clk_set_rate(g_vdec_clk, need_clk_rate);
		if ((ret != 0) && (need_clk_rate == g_vdec_clk_rate_h) &&
			(current_clk_rate != g_vdec_clk_rate_n)) {
			dprint(PRN_ALWS, "[%s] failed set clk to %u Hz,fail code is %d\n", __func__, need_clk_rate, ret);
			need_clk_rate = g_vdec_clk_rate_n;
			need_clk_type = VDEC_CLK_RATE_NORMAL;
			ret = clk_set_rate(g_vdec_clk, need_clk_rate);
		}

#ifdef VDEC_AVS_LOW_CFG
		if ((ret != 0) && (need_clk_rate == g_vdec_clk_rate_n) &&
			(current_clk_rate != g_vdec_clk_rate_l)) {
			dprint(PRN_ALWS, "[%s] failed set clk to %u Hz,fail code is %d\n", __func__, need_clk_rate, ret);
			need_clk_rate = g_vdec_clk_rate_l;
			need_clk_type = VDEC_CLK_RATE_LOW;
			ret = clk_set_rate(g_vdec_clk, need_clk_rate);
		}
#endif
		if (ret == 0) {
			g_resume_clk_type = need_clk_type;
		} else {
			dprint(PRN_ERROR, "[%s] failed set clk to %u Hz,fail code is %d\n", __func__, need_clk_rate, ret);
			goto error_exit;
		}
	}

	vdec_mutex_unlock(&g_regulator_mutex);

	return 0;

error_exit:
	vdec_mutex_unlock(&g_regulator_mutex);

	return VCODEC_FAILURE;
}
