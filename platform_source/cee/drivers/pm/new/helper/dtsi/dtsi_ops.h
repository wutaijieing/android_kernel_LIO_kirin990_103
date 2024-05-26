/*
 * dtsi_ops.h
 *
 * debug information of suspend
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

#ifndef __H_PM_DTSI_OPS_H__
#define __H_PM_DTSI_OPS_H__

#include <linux/compiler_types.h>

const struct device_node *lp_dn_root(void);
void __iomem *lp_map_dn(struct device_node *from,
			const char *dn_name);
const char **lp_read_dtsi_strings(const struct device_node *np,
				  const char *propname,
				  const char *propsize_name,
				  int *size);
struct device_node *lp_find_node_index(struct device_node *from,
				       const char *pre,
				       int index);
void __iomem *lp_map_dn_index(struct device_node *from,
			      const char *pre,
			      int index);
int lp_read_property_u32_array(const struct device_node *np,
			       const char *name,
			       u32 **arr);
bool lp_is_fpga(void);

#define sr_unsupported() (lp_dn_root() == NULL)

#endif /* __H_PM_DTSI_OPS_H__ */
