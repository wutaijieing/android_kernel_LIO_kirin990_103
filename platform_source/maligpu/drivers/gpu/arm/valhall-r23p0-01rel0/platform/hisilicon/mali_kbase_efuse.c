/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:gpu efuse configuration
 * Author : gpu
 * Create : 2021/1/28
 */

#include <mali_kbase.h>
#include "mali_kbase_efuse.h"
#include "mali_kbase_defs.h"

#include <platform_include/see/efuse_driver.h>
#include <string.h>

#include <linux/of.h>

struct kbase_efuse_soc_spec {
	enum SOC_SPEC_TYPE type;
	char* str_type;
};

static const struct kbase_efuse_soc_spec s_soc_specs[] = {
	{NORMAL_CHIP, "normal"},
	{LITE_CHIP, "lite"},
	{LITE_NORMAL_CHIP, "lite-normal"},
	{WIFI_ONLY_CHIP, "wifi-only"},
	{WIFI_ONLY_NORMAL_CHIP, "wifi-only-normal"},
	{LITE2_CHIP, "lite2"},
	{LITE2_NORMAL_CHIP, "lite2-normal"},
	{PC_CHIP, "pc"},
	{PC_NORMAL_CHIP, "pc-normal"},
	{LSD_CHIP, "lsd"},
	{LSD_NORMAL_CHIP, "lsd-normal"},
	{SD_CHIP, "sd"},
	{SD_NORMAL_CHIP, "sd-normal"},
};

/**
 *  stack0 tile0 bit[0] ->core 0
 *  stack0 tile1 bit[1] ->core 3
 *  stack0 tile2 bit[2] ->core 6
 *  stack0 tile3 bit[3] ->core 9
 *  stack1 tile0 bit[4] ->core 1
 *  stack1 tile1 bit[5] ->core 4
 *  stack1 tile2 bit[5] ->core 7
 *  stack1 tile3 bit[7] ->core 10
 *  stack2 tile0 bit[8] ->core 2
 *  stack2 tile1 bit[9] ->core 5
 *  stack2 tile2 bit[10]->core 8
 *  stack2 tile3 bit[11]->core 11
 *  stack4 tile0 bit[12]->core 12
 *  stack4 tile1 bit[13]->core 15
 *  stack4 tile2 bit[14]->core 18
 *  stack4 tile3 bit[15]->core 21
 *  stack5 tile0 bit[16]->core 13
 *  stack5 tile1 bit[17]->core 16
 *  stack5 tile2 bit[18]->core 19
 *  stack5 tile3 bit[19]->core 22
 *  stack6 tile0 bit[20]->core 14
 *  stack6 tile1 bit[21]->core 17
 *  stack6 tile2 bit[22]->core 20
 *  stack6 tile3 bit[23]->core 23
 */
shader_present_struct g_clip_core[] = {
	{ EFUSE_BIT0_FAIL, CLIP_BIT0_CORE, CLIP_BIT0_CORE_PLUS },
	{ EFUSE_BIT1_FAIL, CLIP_BIT4_CORE, CLIP_BIT4_CORE_PLUS },
	{ EFUSE_BIT2_FAIL, CLIP_BIT8_CORE, CLIP_BIT8_CORE_PLUS },
	{ EFUSE_BIT3_FAIL, CLIP_BIT12_CORE, CLIP_BIT12_CORE_PLUS },
	{ EFUSE_BIT4_FAIL, CLIP_BIT1_CORE, CLIP_BIT1_CORE_PLUS },
	{ EFUSE_BIT5_FAIL, CLIP_BIT5_CORE, CLIP_BIT5_CORE_PLUS },
	{ EFUSE_BIT6_FAIL, CLIP_BIT9_CORE, CLIP_BIT9_CORE_PLUS },
	{ EFUSE_BIT7_FAIL, CLIP_BIT13_CORE, CLIP_BIT13_CORE_PLUS },
	{ EFUSE_BIT8_FAIL, CLIP_BIT2_CORE, CLIP_BIT2_CORE_PLUS },
	{ EFUSE_BIT9_FAIL, CLIP_BIT6_CORE, CLIP_BIT6_CORE_PLUS },
	{ EFUSE_BIT10_FAIL, CLIP_BIT10_CORE, CLIP_BIT10_CORE_PLUS },
	{ EFUSE_BIT11_FAIL, CLIP_BIT14_CORE, CLIP_BIT14_CORE_PLUS },
	{ EFUSE_BIT12_FAIL, CLIP_BIT16_CORE, CLIP_BIT16_CORE_PLUS },
	{ EFUSE_BIT13_FAIL, CLIP_BIT20_CORE, CLIP_BIT20_CORE_PLUS },
	{ EFUSE_BIT14_FAIL, CLIP_BIT24_CORE, CLIP_BIT24_CORE_PLUS },
	{ EFUSE_BIT15_FAIL, CLIP_BIT28_CORE, CLIP_BIT28_CORE_PLUS },
	{ EFUSE_BIT16_FAIL, CLIP_BIT17_CORE, CLIP_BIT17_CORE_PLUS },
	{ EFUSE_BIT17_FAIL, CLIP_BIT21_CORE, CLIP_BIT21_CORE_PLUS },
	{ EFUSE_BIT18_FAIL, CLIP_BIT25_CORE, CLIP_BIT25_CORE_PLUS },
	{ EFUSE_BIT19_FAIL, CLIP_BIT29_CORE, CLIP_BIT29_CORE_PLUS },
	{ EFUSE_BIT20_FAIL, CLIP_BIT18_CORE, CLIP_BIT18_CORE_PLUS },
	{ EFUSE_BIT21_FAIL, CLIP_BIT22_CORE, CLIP_BIT22_CORE_PLUS },
	{ EFUSE_BIT22_FAIL, CLIP_BIT26_CORE, CLIP_BIT26_CORE_PLUS },
	{ EFUSE_BIT23_FAIL, CLIP_BIT30_CORE, CLIP_BIT30_CORE_PLUS }
};

u32 get_efuse_fail_core_num(u32 efuse_bit)
{
	u32 cnt = 0;
	while (efuse_bit) {
		cnt += efuse_bit & 1;
		efuse_bit >>= 1;
	}
	return cnt;
}

int clip_8_gpu_cores(u32 *core_mask, u32 abnormal_core_cnt, u32 efuse_abnormal_core_mask)
{
	u32 i;
	u32 left_hotplug_core_num = 8u;
	u32 mask = *core_mask;

	if (abnormal_core_cnt > 8u) {
		return ERROR;
	}

	left_hotplug_core_num -= abnormal_core_cnt;
	for (i = 0u; i < sizeof(g_clip_core) / sizeof(shader_present_struct); ++i) {
		if (abnormal_core_cnt == 0)
			break;

		if (g_clip_core[i].efuse_fail_bit & efuse_abnormal_core_mask) {
			mask = mask & g_clip_core[i].clip_core_mask;
			abnormal_core_cnt--;
		}
	}

	i = 0u;
	while (left_hotplug_core_num != 0) {
		if (mask & (0x1U << (31 - i))) {
			mask &= ~(0x1U << (31 - i));
			left_hotplug_core_num--;
		}
		i++;
	}
	*core_mask = mask;

	return OK;
}

int clip_2_gpu_cores(u32 *core_mask, u32 abnormal_core_cnt, u32 efuse_abnormal_core_mask)
{
	int ret = OK;
	u32 i;
	/** 1. if more than 2 abnormal cores, return error;
	 *  2. if 2 abnormal cores, clip the 2 abnormal cores;
	 *  3. if 1 abnormal core, clip the 1 abnormal core and 1 normal core;
	 *  4. if 0 abnormal cores, clip two normal cores.
	 */
	if (abnormal_core_cnt > 2u) {
		ret = ERROR;
	} else if (abnormal_core_cnt == 2u) {
		for (i = 0u; i < sizeof(g_clip_core) / sizeof(shader_present_struct); ++i) {
			if (g_clip_core[i].efuse_fail_bit & efuse_abnormal_core_mask)
				*core_mask = *core_mask & g_clip_core[i].clip_core_mask;
		}
	} else if (abnormal_core_cnt == 1u) {
		for (i = 0u; i < sizeof(g_clip_core) / sizeof(shader_present_struct); ++i) {
			if (g_clip_core[i].efuse_fail_bit & efuse_abnormal_core_mask) {
				*core_mask = *core_mask & g_clip_core[i].clip_core_mask_plus;
				break;
			}
		}
	} else {
		*core_mask = LITE_CHIP_CORE_MASK;
	}

	return ret;
}

int get_shader_present_mask(u32* shader_present_mask, u32 efuse_abnormal_core_mask,
	enum SOC_SPEC_TYPE soc_spec_type)
{
	int ret = OK;
	u32 core_mask = ALL_SHADERE_CORE_MASK;
	u32 abnormal_core_cnt;

	abnormal_core_cnt = get_efuse_fail_core_num(efuse_abnormal_core_mask);
	switch (soc_spec_type) {
	case UNKNOWN_CHIP:
		ret = ERROR;
		break;
	case NORMAL_CHIP:
		break;
	case LITE_NORMAL_CHIP:
	case WIFI_ONLY_NORMAL_CHIP:
	case LITE2_NORMAL_CHIP:
	case PC_NORMAL_CHIP:
		core_mask = LITE_CHIP_CORE_MASK;
		break;
	case LITE_CHIP:
	case WIFI_ONLY_CHIP:
	case LITE2_CHIP:
	case PC_CHIP:
		ret = clip_2_gpu_cores(&core_mask, abnormal_core_cnt, efuse_abnormal_core_mask);
		break;
	case LSD_CHIP:
		ret = clip_8_gpu_cores(&core_mask, abnormal_core_cnt, efuse_abnormal_core_mask);
		break;
	case LSD_NORMAL_CHIP:
		core_mask = LSD_LITE_CHIP_CORE_MASK;
		break;
	case SD_CHIP:
	case SD_NORMAL_CHIP:
		core_mask = 0;
		break;
	default:
		ret = ERROR;
		break;
	}

	*shader_present_mask = core_mask;
	return ret;
}

enum SOC_SPEC_TYPE convert_to_enum(const char *soc_spec_str)
{
	u32 i;
	for (i = 0u; i < SOC_SPEC_NUM; i++)
		if (strcmp(soc_spec_str, s_soc_specs[i].str_type) == 0)
			return s_soc_specs[i].type;

	return UNKNOWN_CHIP;
}

#define EFUSE_BUF_SIZE 4
// use buf[2]:7-0 buf[1]:31-16
#define get_gpu_failed_cores(buf) (((buf[1] >> 16) | ((buf[2] & 0xff) << 16)) ^ 0xffffff)

enum SOC_SPEC_TYPE get_current_chip_type(struct kbase_device * const kbdev)
{
	int ret;
	enum SOC_SPEC_TYPE soc_spc_type;
	const char *soc_spec_str = NULL;
	struct device_node *np = NULL;

	/* read soc spec from dts*/
	np = of_find_compatible_node(NULL, NULL, "hisilicon, soc_spec");
	if (np) {
		ret = of_property_read_string(np, "soc_spec_set", &soc_spec_str);
		if (ret) {
			dev_err(kbdev->dev, "Failed to get sco spec set!\n");
			return UNKNOWN_CHIP;
		}
	} else {
		dev_err(kbdev->dev, "not find device node hisilicon, soc_spec!\n");
		return UNKNOWN_CHIP;
	}
	dev_info(kbdev->dev, "gpu soc_spec_set=[%s]!\n", soc_spec_str);

	soc_spc_type = convert_to_enum(soc_spec_str);
	return soc_spc_type;
}

int get_gpu_efuse_cfg(struct kbase_device * const kbdev, u32 *shader_present_mask)
{
	int ret;
	u32 efuse_abnormal_core_mask;
	enum SOC_SPEC_TYPE soc_spc_type;
	u32 buf[EFUSE_BUF_SIZE] = {0};

	if (!shader_present_mask) {
		dev_err(kbdev->dev, "%s, ptr is null.\n", __func__);
		return ERROR;
	}

	soc_spc_type = get_current_chip_type(kbdev);

	ret = efuse_read_value(buf, EFUSE_BUF_SIZE, EFUSE_FN_RD_PARTIAL_PASSP2);
	if (ret) {
		dev_err(kbdev->dev, "%s, efuse_read_value failed\n", __func__);
		return ERROR;
	}
	efuse_abnormal_core_mask = get_gpu_failed_cores(buf);
	dev_info(kbdev->dev, "gpu efuse_abnormal_core_mask=[0x%x], buf[1]=0x%x, buf[2]=0x%x!\n",
		efuse_abnormal_core_mask, buf[1], buf[2]);

	ret = get_shader_present_mask(shader_present_mask, efuse_abnormal_core_mask, soc_spc_type);
	if (ret) {
		dev_err(kbdev->dev, "buf[1]=0x%x, buf[2]=0x%x!\n", buf[1], buf[2]);
		dev_err(kbdev->dev, "get_shader_present_mask failed, return soc type=[%u], efuse_abnormal_core_mask=0x%x.\n",
			soc_spc_type, efuse_abnormal_core_mask);
		return ERROR;
	}

	dev_err(kbdev->dev, "return soc type=[%u], efuse_abnormal_core_mask=0x%x, gpu core_mask:0x%x.\n",
		soc_spc_type, efuse_abnormal_core_mask, *shader_present_mask);
	return ret;
}
