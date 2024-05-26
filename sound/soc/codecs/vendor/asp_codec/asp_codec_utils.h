/*
 * asp_codec_utils.h -- asp codec utils driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
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

#ifndef __ASP_CODEC_UTILS_H__
#define __ASP_CODEC_UTILS_H__

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/soc.h>

#define PAGE_SOCCODEC         0x52000
#define PAGE_ASPCFG           0x4E000
#define PAGE_AO_IOC           0x9C000
#define PAGE_IOC              0x02000
#define PAGE_PMU_CODEC        0x0
#define PAGE_VIRCODEC         0x8000

#define SOCCODEC_START_OFFSET 0x00
#define SOCCODEC_END_OFFSET   0x28C
#define VIRCODEC_START_OFFSET 0
#define VIRCODEC_END_OFFSET   (VIR_REG_OFFSET_BUTT - 1)
#define ASPCFG_START_OFFSET   0x000
#define ASPCFG_END_OFFSET     0x2A8
#define AOIOC_START_OFFSET    0x000
#define AOIOC_END_OFFSET      0xF7C
#define IOC_START_OFFSET      0x000
#define IOC_END_OFFSET        0xF38

#define MAX_VAL_ON_BIT(bit) ((1U << (bit)) - 1)
#define MASK_ON_BIT(bit, offset) ((MAX_VAL_ON_BIT(bit)) << (offset))

int asp_codec_base_addr_map(struct platform_device *pdev);
void asp_codec_base_addr_unmap(void);
void asp_codec_clr_reg_bits(unsigned int reg, unsigned int value);
unsigned int asp_codec_reg_read(struct snd_soc_component *codec,
	unsigned int reg);
int asp_codec_reg_update(unsigned int reg, unsigned int mask,
	unsigned int value);
int asp_codec_reg_write(struct snd_soc_component *codec, unsigned int reg,
	unsigned int value);
void asp_codec_set_reg_bits(unsigned int reg, unsigned int value);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
unsigned int asp_codec_reg_read_by_codec(struct snd_soc_codec *codec,
	unsigned int reg);
int asp_codec_reg_write_by_codec(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value);
#endif

#endif

