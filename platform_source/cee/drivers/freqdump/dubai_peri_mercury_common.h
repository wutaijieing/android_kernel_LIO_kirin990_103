/*
 * dubai_peri_mercury_common.h
 *
 * dubai_peri
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DUBAI_PERI_MERCURY_COMMON__
#define __DUBAI_PERI_MERCURY_COMMON__

#include <linux/kernel.h>

#define CH_MAX			32
#define PERI_VOTE_DATA		32
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

union peri_vote_union {
	struct {
		unsigned int vote0;
		unsigned int vote1;
	} value;
	struct {
		/* 00 0.6v;01 0.65v;10 0.7v;11 0.8 */
		unsigned int  data0  : 2;
		unsigned int  data1  : 2;
		unsigned int  data2  : 2;
		unsigned int  data3  : 2;
		unsigned int  data4  : 2;
		unsigned int  data5  : 2;
		unsigned int  data6  : 2;
		unsigned int  data7  : 2;
		unsigned int  data8  : 2;
		unsigned int  data9  : 2;
		unsigned int  data10  : 2;
		unsigned int  data11  : 2;
		unsigned int  data12  : 2;
		unsigned int  data13  : 2;
		unsigned int  data14  : 2;
		unsigned int  data15  : 2;
		unsigned int  data16  : 2;
		unsigned int  data17  : 2;
		unsigned int  data18  : 2;
		unsigned int  data19  : 2;
		unsigned int  data20  : 2;
		unsigned int  data21  : 2;
		unsigned int  data22  : 2;
		unsigned int  data23  : 2;
		unsigned int  data24  : 2;
		unsigned int  data25  : 2;
		unsigned int  data26  : 2;
		unsigned int  data27  : 2;
		unsigned int  data28  : 2;
		unsigned int  data29  : 2;
		unsigned int  data30  : 2;
		unsigned int  data31  : 2;
	} reg;
};

void format_dubai_peri_data(u32 peri0, u32 peri1);
int clear_dubai_peri_data(void);
int ioctrl_get_dubai_peri_data(void __user *argp);
int ioctrl_get_dubai_peri_size(void __user *argp);

#endif
