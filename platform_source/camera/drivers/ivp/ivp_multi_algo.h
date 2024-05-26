/*
 * ulti-algorithm memory management for ivp
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _IVP_MULTI_ALGO_H_
#define _IVP_MULTI_ALGO_H_
#include <linux/module.h>
#include "ivp_platform.h"
#include "elf.h"

/* two cores use same ddr-zone for text sections */
extern bool dyn_core_loaded[2];

struct record_node {
	bool *mem_pool;
	unsigned int pool_size;
	unsigned int node_size;
	unsigned int start_zone;
	unsigned int size_zone;
};

void init_algo_mem_info(struct ivp_device *pdev,
		const unsigned int algo_id);

int ivp_algo_mem_init(struct ivp_device *pdev);

int ivp_free_algo_zone(struct ivp_device *pdev,
		unsigned int algo_id, unsigned int segment_type);

int ivp_alloc_algo_mem(struct ivp_device *pdev,
		const XT_Elf32_Phdr header, const unsigned int algo_id);
#endif  /* _IVP_MULTI_ALGO_H_ */
