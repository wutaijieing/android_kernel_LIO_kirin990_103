/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: initialization and save trace log of pattern trace
 * Create: 2021-10-28
 */

#ifndef _DMSS_PT_DRV_H_
#define _DMSS_PT_DRV_H_

#include "dmsspt_registers_chamaeleon.h"

#define ERR			(-1)
#define MID_GROUP_SIZE		4
#define CPU_CORE_NUM_MAX	8
#define NAME_LEN_MAX		16
#define DMSSPT_SHOW_LEN_MAX	128
#define NODE_WIDTH_1_BITS	1
#define NODE_WIDTH_8_BITS	8
#define NODE_WIDTH_16_BITS	16
#define NODE_WIDTH_32_BITS	32
#define ADDR_SHIFT_MODE_MASK	0x300
#define ADDR_SHIFT_MODE_1	1
#define ADDR_SHIFT_MODE_2	2
#define DDR_SIZE_3G512M	0xE0000000ULL

#define DMSS_INTLV_MIN_BYTES	128
#define DMSS_INTLV_BYTES(intlv_gran)	(DMSS_INTLV_MIN_BYTES * (0x01 << (intlv_gran)))

void dmsspt_stop_store(unsigned int val);

#endif
