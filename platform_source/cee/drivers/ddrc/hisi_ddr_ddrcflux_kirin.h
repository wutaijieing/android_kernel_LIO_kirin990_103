/*
 * hisi_ddr_ddrcflux_kirin.h
 *
 * ddrcflux head files
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef __HISI_DDR_DDRCFLUX_KIRIN_H__
#define __HISI_DDR_DDRCFLUX_KIRIN_H__

#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_common.h"

#if defined(CONFIG_DDR_LACERTA)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_lacerta.h"
#include "timer.h"
#elif defined(CONFIG_DDR_CETUS)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_cetus.h"
#include "timer.h"
#elif defined(CONFIG_DDR_PERSEUS)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_perseus.h"
#include "timer.h"
#elif defined(CONFIG_DDR_DRACO)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_draco.h"
#include "timer.h"
#elif defined(CONFIG_DDR_TRIONES)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_triones.h"
#include "timer.h"
#elif defined(CONFIG_DDR_URSA)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_ursa.h"
#include "timer.h"
#elif defined(CONFIG_DDR_CASSIOPEIA)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_cassiopeia.h"
#include "timer.h"
#elif defined(CONFIG_DDR_CEPHEUS)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_cepheus.h"
#include "timer.h"
#elif defined(CONFIG_DDR_MONOCEROS)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_monoceros.h"
#include "timer.h"
#elif defined(CONFIG_DDR_PISCES)
#include "hisi_ddrflux_reg/ddr_ddrcflux_registers_pisces.h"
#include "timer.h"
#elif defined(CONFIG_DDR_CHAMAELEON)
#include "hisi_ddrflux_reg/hisi_ddr_ddrcflux_registers_chamaeleon.h"
#include "timer_v5.h"
#endif

#endif /* __HISI_DDR_DDRCFLUX_KIRIN_H__ */
