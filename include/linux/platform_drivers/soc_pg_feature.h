/*
 * parse soc pg feature
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2022. All rights reserved.
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
#ifndef _SOC_PG_FEATURE_H_
#define _SOC_PG_FEATURE_H_

#define NEED_LOAD_MODEM   0
#define NOT_LOAD_MODEM    1

#define NEED_NOTHING      0
#define NEED_VOLTAGE_FREQ 1
#define NEED_AVS_STRATEGY 2
#define NOT_SUPPOST       3

#define ERROR             (-1)
#define OK                0

#define MODEM_CHIP_TYPE_ES "es"
#define MODEM_CHIP_TYPE_CS "cs"

int get_pg_soc_is_modem_need_load(void);
int get_soc_modem_regulator_strategy(void);

#endif /* _SOC_PG_FEATURE_H_ */
