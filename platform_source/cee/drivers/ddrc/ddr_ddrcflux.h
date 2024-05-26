/*
 * ddr_ddrcflux.h
 *
 * ddrc flux
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
#ifndef __HISI_DDR_DDRCFLUX_H__
#define __HISI_DDR_DDRCFLUX_H__

#define SLICE_INFO_LEN		6

enum ddrc_flux_para {
	STA_ID = 0,
	STA_ID_MASK,
	INTERVAL,
	SUM_TIME,
	IRQ_AFFINITY,
	DDRC_UNSEC_PASS,
	DMSS_ENABLE,
	QOSBUF_ENABLE,
	DMC_ENABLE,
	DMCCMD_ENABLE,
	DMCDATA_ENABLE,
	DMCLATENCY_ENABLE,
	DDRC_FLUX_MAX,
};

struct slice_info {
	char name[SLICE_INFO_LEN];
};

#endif /* __HISI_DDR_DDRCFLUX_H__ */
