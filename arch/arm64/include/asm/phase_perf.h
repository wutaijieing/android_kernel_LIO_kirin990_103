/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_perf.h - arm64-specific Kernel Phase
 *
 * Copyright (c) 2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_PHASE_PERF_H
#define __ASM_PHASE_PERF_H

/* pmu events number */
#define L1I_CACHE_REFILL		0x0001
#define L1D_CACHE			0x0004
#define INST_RETIRED			0x0008
#define BR_MIS_PRED			0x0010
#define CYCLES				0x0011
#define L2D_CACHE			0x0016
#define INST_SPEC			0x001b
#define STALL_FRONTEND			0x0023
#define STALL_BACKEND			0x0024
#define OP_SPEC				0x003b
#define IF_RSURC_CHK_OK			0x104b
#define DSP_STALL			0x200e
#define ROB_FLUSH			0x2010
#define FETCH_BUBBLE			0x2011
#define FSU_ISQ_STALL			0x2015
#define RETIRED_LOAD			0x2037
#define STQ_STALL			0x50b6

#endif
