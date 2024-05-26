/*
 * pub_def.h
 *
 * define public funcs used in pm module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#ifndef __H_PM_PUB_DEF_H__
#define __H_PM_PUB_DEF_H__

#include <linux/bits.h>
#include <linux/slab.h>
#include <pm_def.h>

#define _lowpm_free(arr, free_func)     \
	do {                            \
		if (arr != NULL)        \
			free_func(arr); \
		arr = NULL;             \
	} while (0)

#define _lowpm_free_array(arr, size, iter, free_item_func, free_array_func) \
	do {                                                                \
		for (iter = 0; iter < size; iter++)                         \
			_lowpm_free(arr[iter], free_item_func);             \
		_lowpm_free(arr, free_array_func);                          \
	} while (0)

#define lowpm_kfree(arr) \
	_lowpm_free(arr, kfree)

#define lowpm_kfree_array(arr, size, iter) \
	_lowpm_free_array(arr, size, iter, kfree, kfree)

#define lowpm_iounmap_array(arr, size, iter) \
	_lowpm_free_array(arr, size, iter, iounmap, kfree)

static inline int first_setted_bit(unsigned int nr)
{
	int i;
	unsigned int n;

	for (i = 0, n = nr; i < BITS_PER_LONG && ((n & 1) == 0); i++, n = n >> 1)
		;
	return i;
}

#define no_used(x) (void)(x)

#ifdef CONFIG_PRODUCT_CDC_ACE
#define pmu_write_sr_tick(pos)
#else
#define pmu_write_sr_tick(pos) lp_pmic_writel(PMUOFFSET_SR_TICK, pos)
#endif

#endif /* __H_PM_PUB_DEF_H__ */
