/*
 * Zodiac Secure Digital Host Controller Interface.
 * Copyright (c) Zodiac Technologies Co., Ltd. 2019-2020. All rights reserved.
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
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>

#include "sdhci-pltfm.h"
#include "sdhci_zodiac_mmc.h"

static struct sdhci_crg_ctrl g_zodiac_crgctrl_cfg[SDHCI_ZODIAC_TIMING_MODE] = {0};

void sdhci_zodiac_hardware_reset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	if (!host->hsdt1crg) {
		pr_err("%s: hsdt1 is null, can't reset mmc!\n", __func__);
		return;
	}

	switch (sdhci_zodiac->sdhci_host_id) {
	case SDHCI_SD_ID:
		/* sd reset */
		pr_err("%s: andromeda not support sd!\n", __func__);
		break;
	case SDHCI_SDIO_ID:
		/* sdio reset */
		pr_err("%s: andromeda not support sdio!\n", __func__);
		break;
	case SDHCI_EMMC_ID:
		/* emmc reset */
		sdhci_hsdt1crg_writel(host, IP_RST_EMMC_BIT | IP_PRST_EMMC_BIT, PERIPRST1);
		udelay(100);
		if ((sdhci_hsdt1crg_readl(host, PERIPSTAT1) & (IP_RST_EMMC_BIT | IP_PRST_EMMC_BIT)) !=
			(IP_RST_EMMC_BIT | IP_PRST_EMMC_BIT))
			pr_err("%s: emmc iprst status err after wait 100us\n", __func__);
		break;
	default:
		break;
	}
}

void sdhci_zodiac_hardware_disreset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	/* sd dis-reset */
	if (!host->hsdt1crg) {
		pr_err("%s: hsdt1 is null, can't reset mmc!\n", __func__);
		return;
	}

	switch (sdhci_zodiac->sdhci_host_id) {
	case SDHCI_SD_ID:
		/* sd disreset */
		pr_err("%s: andromeda not support sd!\n", __func__);
		break;
	case SDHCI_SDIO_ID:
		/* sdio disreset */
		pr_err("%s: andromeda not support sdio!\n", __func__);
		break;
	case SDHCI_EMMC_ID:
		/* emmc disreset */
		sdhci_hsdt1crg_writel(host, IP_RST_EMMC_BIT | IP_PRST_EMMC_BIT, PERIPDIS1);
		udelay(100);
		if (sdhci_hsdt1crg_readl(host, PERIPSTAT1) & (IP_RST_EMMC_BIT | IP_PRST_EMMC_BIT))
			pr_err("%s: emmc disrst status err after wait 100us\n", __func__);
		break;
	default:
		break;
	}

}

int sdhci_zodiac_get_resource(struct platform_device *pdev, struct sdhci_host *host)
{
	struct device_node *np = NULL;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hsdt1_crg");
	if (!np) {
		pr_err("can't find hsdt1 crg!\n");
		return -1;
	}

	host->hsdt1crg = of_iomap(np, 0);
	if (!host->hsdt1crg) {
		pr_err("hsdt1crg iomap error!\n");
		return -1;
	}

	sdhci_zodiac->pcrgctrl = g_zodiac_crgctrl_cfg;

	return 0;
}

