/*
* Copyright @ Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
* Description: dmsspt's platform file
* Create: 2021-1-14
*/

#ifndef _DMSS_PT_DRV_H_
#define _DMSS_PT_DRV_H_

#if defined(CONFIG_DDR_LACERTA)
#include "dmss_pt_reg/dmsspt_registers_lacerta.h"
#elif defined(CONFIG_DDR_CETUS)
#include "dmss_pt_reg/dmsspt_registers_cetus.h"
#elif defined(CONFIG_DDR_PERSEUS)
#include "dmss_pt_reg/dmsspt_registers_perseus.h"
#elif defined(CONFIG_DDR_DRACO)
#include "dmss_pt_reg/dmsspt_registers_draco.h"
#elif defined(CONFIG_DDR_TRIONES)
#include "dmss_pt_reg/dmsspt_registers_triones.h"
#elif defined(CONFIG_DDR_URSA)
#include "dmss_pt_reg/dmsspt_registers_ursa.h"
#elif defined(CONFIG_DDR_CASSIOPEIA)
#include "dmss_pt_reg/dmsspt_registers_cassiopeia.h"
#elif defined(CONFIG_DDR_CEPHEUS)
#include "dmss_pt_reg/dmsspt_registers_cepheus.h"
#elif defined(CONFIG_DDR_MONOCEROS)
#include "dmss_pt_reg/dmsspt_registers_monoceros.h"
#elif defined(CONFIG_DDR_PISCES)
#include "dmss_pt_reg/dmsspt_registers_pisces.h"
#endif

#define ERR			(-1)
#define MID_GROUP_SIZE		4
#define CPU_CORE_NUM_MAX	8
#define NAME_LEN_MAX		16
#define DMSSPT_SHOW_LEN_MAX	128
#define NODE_WIDTH_1_BITS	1
#define NODE_WIDTH_8_BITS	8
#define NODE_WIDTH_16_BITS	16
#define NODE_WIDTH_32_BITS	32
#define ADDR_SHIFT_MODE_MASK	3
#define ADDR_SHIFT_MODE_1	1
#define ADDR_SHIFT_MODE_2	2
#define DDR_SIZE_3G512M	0xE0000000ULL

#define DMSS_INTLV_MIN_BYTES	128
#define DMSS_INTLV_BYTES(intlv_gran)	(DMSS_INTLV_MIN_BYTES * (0x01 << (intlv_gran)))

void dmsspt_stop_store(unsigned int val);

#endif
