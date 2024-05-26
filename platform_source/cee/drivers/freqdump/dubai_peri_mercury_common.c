/*
 * dubai_peri_mercury_common.c
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

#include "dubai_peri_mercury_common.h"
#include <linux/types.h>
#include <linux/uaccess.h>
#include <securec.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)	"[dubai]:" fmt

#define volt_level(a)	a
#define ONE_SECOND	1

static struct channel_data g_peri_volt[CH_MAX + 1];

void set_peri_data(unsigned int peri_volt, struct channel_data *ch,
		   struct channel_data *sum)
{
	if (ch == NULL || sum == NULL)
		return;

	switch (peri_volt) {
	case volt_level(0):
		ch->volt0 += ONE_SECOND;
		sum->volt0 += ONE_SECOND;
		break;
	case volt_level(1):
		ch->volt1 += ONE_SECOND;
		sum->volt1 += ONE_SECOND;
		break;
	case volt_level(2):
		ch->volt2 += ONE_SECOND;
		sum->volt2 += ONE_SECOND;
		break;
	case volt_level(3):
		ch->volt3 += ONE_SECOND;
		sum->volt3 += ONE_SECOND;
		break;
	default:
		pr_err("%s,out of peri volt:%d\n", __func__, peri_volt);
	}
}

void set_dubai_peri_data(union peri_vote_union *peri)
{
	if (peri == NULL)
		return;

	set_peri_data(peri->reg.data0,  &g_peri_volt[0],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data1,  &g_peri_volt[1],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data2,  &g_peri_volt[2],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data3,  &g_peri_volt[3],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data4,  &g_peri_volt[4],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data5,  &g_peri_volt[5],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data6,  &g_peri_volt[6],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data7,  &g_peri_volt[7],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data8,  &g_peri_volt[8],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data9,  &g_peri_volt[9],  &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data10, &g_peri_volt[10], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data11, &g_peri_volt[11], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data12, &g_peri_volt[12], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data13, &g_peri_volt[13], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data14, &g_peri_volt[14], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data15, &g_peri_volt[15], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data16, &g_peri_volt[16], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data17, &g_peri_volt[17], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data18, &g_peri_volt[18], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data19, &g_peri_volt[19], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data20, &g_peri_volt[20], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data21, &g_peri_volt[21], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data22, &g_peri_volt[22], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data23, &g_peri_volt[23], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data24, &g_peri_volt[24], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data25, &g_peri_volt[25], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data26, &g_peri_volt[26], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data27, &g_peri_volt[27], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data28, &g_peri_volt[28], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data29, &g_peri_volt[29], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data30, &g_peri_volt[30], &g_peri_volt[CH_MAX]);
	set_peri_data(peri->reg.data31, &g_peri_volt[31], &g_peri_volt[CH_MAX]);
}

void format_dubai_peri_data(u32 peri0, u32 peri1)
{
	union peri_vote_union peri;

	peri.value.vote0 = peri0;
	peri.value.vote1 = peri1;
	set_dubai_peri_data(&peri);
}

int clear_dubai_peri_data(void)
{
	errno_t ret;

	ret = memset_s(g_peri_volt, sizeof(g_peri_volt), 0,
		       sizeof(g_peri_volt));
	if (ret != EOK)
		pr_err("%s,memset peri data err:%d\n", __func__, ret);
	return ret;
}

int ioctrl_get_dubai_peri_data(void __user *argp)
{
	int ret = 0;

	if (copy_to_user(argp, g_peri_volt, sizeof(g_peri_volt)) != 0) {
		pr_err("peri data get  failed!\n");
		ret = -EFAULT;
	}
	return ret;
}

int ioctrl_get_dubai_peri_size(void __user *argp)
{
	int ret = 0;
	struct peri_data_size peri_size;

	peri_size.vote_col = sizeof(struct channel_data) / sizeof(u32);
	peri_size.vote_row = CH_MAX + 1;
	if (copy_to_user(argp, &peri_size, sizeof(peri_size)) != 0) {
		pr_err("peri size get  failed!\n");
		ret = -EFAULT;
	}
	return ret;
}
