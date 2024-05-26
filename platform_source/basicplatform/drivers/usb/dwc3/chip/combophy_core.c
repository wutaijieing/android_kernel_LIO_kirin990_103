/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: ComboPHY Core Module on platform
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_drivers/usb/chip_usb_phy.h>
#include <linux/phy/phy.h>

#define INVALID_REG_VALUE 0xFFFFFFFFu

int combophy_set_mode(struct phy *phy,
		      enum tcpc_mux_ctrl_type mode_type,
		      enum typec_plug_orien_e typec_orien)
{
	struct chip_combophy *combophy = NULL;

	if (!phy)
		return -EINVAL;

	combophy = phy_get_drvdata(phy);
	if (combophy && combophy->set_mode)
		return combophy->set_mode(combophy, mode_type, typec_orien);

	return -EINVAL;
}

int combophy_init(struct phy *phy)
{
	return phy_init(phy);
}

int combophy_exit(struct phy *phy)
{
	return phy_exit(phy);
}

int combophy_register_debugfs(struct phy *phy, struct dentry *root)
{
	struct chip_combophy *combophy = NULL;

	if (!phy)
		return -EINVAL;

	combophy = phy_get_drvdata(phy);
	if (combophy && combophy->register_debugfs)
		return combophy->register_debugfs(combophy, root);

	return 0;
}

void combophy_regdump(struct phy *phy)
{
	struct chip_combophy *combophy = NULL;

	if (!phy)
		return;

	combophy = phy_get_drvdata(phy);
	if (combophy && combophy->regdump)
		combophy->regdump(combophy);
}
