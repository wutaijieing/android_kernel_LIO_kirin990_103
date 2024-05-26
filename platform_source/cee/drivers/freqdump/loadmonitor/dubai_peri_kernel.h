/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Description: dubai peri data file
 *  Author: lixiangtao
 *  Create: 2020-07-07
 */
#ifndef __DUBAI_PERI__
#define __DUBAI_PERI__

#include <linux/kernel.h>
#include <loadmonitor_plat.h>

struct channel_data {
	u32 volt0;
	u32 volt1;
	u32 volt2;
	u32 volt3;
};

struct peri_data_size {
	u32 vote_col; /* volt level */
	u32 vote_row; /* channel + 1 */
};

void format_dubai_peri_data(struct dubai_peri *peri);
int clear_dubai_peri_data(void);
int ioctrl_get_dubai_peri_data(void __user *argp);
int ioctrl_get_dubai_peri_size(void __user *argp);

#endif
