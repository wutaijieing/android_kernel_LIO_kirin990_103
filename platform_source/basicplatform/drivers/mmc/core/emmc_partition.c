/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "card.h"
#include "core.h"
#include "mmc_ops.h"
#include <linux/mmc/mmc.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include "emmc_partition.h"

static struct mmc_card *g_card = NULL;

void emmc_get_boot_partition_type(struct mmc_card *card)
{
	int ret = 0;
	int boot_partition_type = 0;
	unsigned char *ext_csd = NULL;
	static char type_str[4][30] = {{"Device not boot enabled"},{"XLOADER_A"},{"XLOADER_B"},{"ERROR_VALUE"}};

	g_card = card;
	if (mmc_can_ext_csd(card) == 0)
		return 0;

	ret = mmc_get_ext_csd(card, &ext_csd);
	if (ret) {
		dev_err(&card->dev, "%s: Failed getting ext_csd\n", __func__);
		emmc_boot_partition_type = XLOADER_A;
		if (ext_csd != NULL)
			kfree(ext_csd);
		return;
	}

	boot_partition_type = (ext_csd[EXT_CSD_PART_CONFIG] >> BOOT_PARTITION_ENABLE_OFFSET) &
				BOOT_PARTITION_ENABLE_MASK;
	if (boot_partition_type >= ERROR_VALUE) {
		dev_err(&card->dev, "%s: Failed getting boot partition type\n", __func__);
		boot_partition_type = ERROR_VALUE;
	}

	card->ext_csd.part_config = ext_csd[EXT_CSD_PART_CONFIG];
	emmc_boot_partition_type = boot_partition_type;

	dev_info(&card->dev, "%s: emmc boot partition type is %s\n", __func__,
		type_str[emmc_boot_partition_type]);

	kfree(ext_csd);
}

int emmc_set_boot_partition_type(unsigned int boot_partition_type)
{
	int ret = -1;
	unsigned char part_config = 0;

	if ((boot_partition_type != XLOADER_A) && (boot_partition_type != XLOADER_B)) {
		dev_err(&g_card->dev, "[%s]:invalid boot partition value\n", __func__);
		return ret;
	}

	if (g_card == NULL) {
		dev_err(&g_card->dev, "[%s]:g_card is NULL\n", __func__);
		return ret;
	}

	if (emmc_boot_partition_type  == boot_partition_type) {
		pr_err("emmc_set_partition type same %d\n",boot_partition_type);
		return 0;
	}

	mmc_get_card(g_card);
	part_config = g_card->ext_csd.part_config;
	part_config &= ~(BOOT_PARTITION_ENABLE_MASK << BOOT_PARTITION_ENABLE_OFFSET);
	part_config |= (unsigned char)boot_partition_type << BOOT_PARTITION_ENABLE_OFFSET;

	ret = mmc_switch(g_card, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_PART_CONFIG, part_config,
			 g_card->ext_csd.part_time);
	if (ret) {
		dev_err(&g_card->dev, "%s: Failed setting boot partition type\n",
			__func__);
		mmc_put_card(g_card);
		return ret;
	}

	g_card->ext_csd.part_config = part_config;
	emmc_boot_partition_type = boot_partition_type;
	mmc_put_card(g_card);

	return ret;
}
