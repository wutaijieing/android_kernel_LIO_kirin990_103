/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: This file describe HISI GPU callback headers
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2021-1-2
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef EFUSE_H
#define EFUSE_H

#define GPU_FAILED_CORES_EFUSE_OFFSET 2324
#define GPU_FAILED_CORES_EFUSE_SIZE 24

#define LITE_CHIP_EFUSE_OFFSET 2365
#define LITE_CHIP_EFUSE_SIZE 3

#define LAST_BUFFER_LITE_EFUSE_OFFSET 2304
#define LAST_BUFFER_LITE_EFUSE_SIZE 2

enum SOC_SPEC_TYPE {
	NORMAL_CHIP = 0,
	LITE_CHIP,
	LITE_NORMAL_CHIP,
	WIFI_ONLY_CHIP,
	WIFI_ONLY_NORMAL_CHIP,
	LITE2_CHIP,
	LITE2_NORMAL_CHIP,
	PC_CHIP,
	PC_NORMAL_CHIP,
	LSD_CHIP,
	LSD_NORMAL_CHIP,
	SD_CHIP,
	SD_NORMAL_CHIP,
	SOC_SPEC_NUM,
	UNKNOWN_CHIP = SOC_SPEC_NUM
};

typedef struct{
	u32 efuse_fail_bit;
	u32 clip_core_mask;
	u32 clip_core_mask_plus;
} shader_present_struct;

// Fail core identified in efuse
#define EFUSE_BIT0_FAIL                0x1
#define EFUSE_BIT1_FAIL                0x2
#define EFUSE_BIT2_FAIL                0x4
#define EFUSE_BIT3_FAIL                0x8
#define EFUSE_BIT4_FAIL                0x10
#define EFUSE_BIT5_FAIL                0x20
#define EFUSE_BIT6_FAIL                0x40
#define EFUSE_BIT7_FAIL                0x80
#define EFUSE_BIT8_FAIL                0x100
#define EFUSE_BIT9_FAIL                0x200
#define EFUSE_BIT10_FAIL               0x400
#define EFUSE_BIT11_FAIL               0x800
#define EFUSE_BIT12_FAIL               0x1000
#define EFUSE_BIT13_FAIL               0x2000
#define EFUSE_BIT14_FAIL               0x4000
#define EFUSE_BIT15_FAIL               0x8000
#define EFUSE_BIT16_FAIL               0x10000
#define EFUSE_BIT17_FAIL               0x20000
#define EFUSE_BIT18_FAIL               0x40000
#define EFUSE_BIT19_FAIL               0x80000
#define EFUSE_BIT20_FAIL               0x100000
#define EFUSE_BIT21_FAIL               0x200000
#define EFUSE_BIT22_FAIL               0x400000
#define EFUSE_BIT23_FAIL               0x800000

// The location of core in the core_mask
#define CLIP_BIT0_CORE                  0x77777776
#define CLIP_BIT1_CORE                  0x77777775
#define CLIP_BIT2_CORE                  0x77777773
#define CLIP_BIT4_CORE                  0x77777767
#define CLIP_BIT5_CORE                  0x77777757
#define CLIP_BIT6_CORE                  0x77777737
#define CLIP_BIT8_CORE                  0x77777677
#define CLIP_BIT9_CORE                  0x77777577
#define CLIP_BIT10_CORE                 0x77777377
#define CLIP_BIT12_CORE                 0x77776777
#define CLIP_BIT13_CORE                 0x77775777
#define CLIP_BIT14_CORE                 0x77773777
#define CLIP_BIT16_CORE                 0x77767777
#define CLIP_BIT17_CORE                 0x77757777
#define CLIP_BIT18_CORE                 0x77737777
#define CLIP_BIT20_CORE                 0x77677777
#define CLIP_BIT21_CORE                 0x77577777
#define CLIP_BIT22_CORE                 0x77377777
#define CLIP_BIT24_CORE                 0x76777777
#define CLIP_BIT25_CORE                 0x75777777
#define CLIP_BIT26_CORE                 0x73777777
#define CLIP_BIT28_CORE                 0x67777777
#define CLIP_BIT29_CORE                 0x57777777
#define CLIP_BIT30_CORE                 0x37777777

#define CLIP_BIT0_CORE_PLUS             0x37777776
#define CLIP_BIT1_CORE_PLUS             0x37777775
#define CLIP_BIT2_CORE_PLUS             0x37777773
#define CLIP_BIT4_CORE_PLUS             0x37777767
#define CLIP_BIT5_CORE_PLUS             0x37777757
#define CLIP_BIT6_CORE_PLUS             0x37777737
#define CLIP_BIT8_CORE_PLUS             0x37777677
#define CLIP_BIT9_CORE_PLUS             0x37777577
#define CLIP_BIT10_CORE_PLUS            0x37777377
#define CLIP_BIT12_CORE_PLUS            0x37776777
#define CLIP_BIT13_CORE_PLUS            0x37775777
#define CLIP_BIT14_CORE_PLUS            0x37773777
#define CLIP_BIT16_CORE_PLUS            0x37767777
#define CLIP_BIT17_CORE_PLUS            0x37757777
#define CLIP_BIT18_CORE_PLUS            0x37737777
#define CLIP_BIT20_CORE_PLUS            0x37677777
#define CLIP_BIT21_CORE_PLUS            0x37577777
#define CLIP_BIT22_CORE_PLUS            0x37377777
#define CLIP_BIT24_CORE_PLUS            0x36777777
#define CLIP_BIT25_CORE_PLUS            0x35777777
#define CLIP_BIT26_CORE_PLUS            0x33777777
#define CLIP_BIT28_CORE_PLUS            0x27777777
#define CLIP_BIT29_CORE_PLUS            0x17777777
#define CLIP_BIT30_CORE_PLUS            0x17777777

#define LITE_CHIP_CORE_MASK             0x17777777
#define ALL_SHADERE_CORE_MASK           0x77777777
#define LSD_LITE_CHIP_CORE_MASK         0x177777

int get_gpu_efuse_cfg(struct kbase_device * const kbdev, u32 *shader_present_mask);
enum SOC_SPEC_TYPE get_current_chip_type(struct kbase_device * const kbdev);

#endif
