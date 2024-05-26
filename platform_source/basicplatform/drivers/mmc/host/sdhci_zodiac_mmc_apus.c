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

static struct sdhci_crg_ctrl g_zodiac_crgctrl_cfg[SDHCI_ZODIAC_TIMING_MODE] = {
/*   [clk_src] [clk_dly_sample] [clk_dly_drv] [clk_div] [max_clk] */
	964000000,     0x7,            0x0,         0x4,       192000000
};

void sdhci_zodiac_hardware_reset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	if (!host->litecrg) {
		pr_err("%s: litecrg is null, can't reset mmc!\n", __func__);
		return;
	}

	if (!host->crgctrl) {
		pr_err("%s: crgctrl is null, can't reset mmc!\n", __func__);
		return;
	}

	if (!host->hsdtcrg) {
		pr_err("%s: hsdtcrg is null, can't reset mmc!\n", __func__);
		return;
	}

	switch (sdhci_zodiac->sdhci_host_id) {
	case SDHCI_SD_ID:
		/* sd reset */
		pr_err("%s: apus not support sd!\n", __func__);
		break;
	case SDHCI_SDIO_ID:
		/* sdio reset */
		sdhci_crgctrl_writel(host, IP_RST_SDIO, PERRSTEN5);
		sdhci_hsdtcrg_writel(host, IP_ARST_SDIO, PERRSTEN0);
		udelay(100);
		if ((sdhci_crgctrl_readl(host, PERRSTSTAT5) & IP_RST_SDIO) !=
				IP_RST_SDIO)
			pr_err("%s: sdio iprst status err after wait 100us\n", __func__);
		if ((sdhci_hsdtcrg_readl(host, PERRSTSTAT0) & IP_ARST_SDIO) !=
				IP_ARST_SDIO)
			pr_err("%s: sdio bus reset status err after wait 100us\n", __func__);
		break;
	case SDHCI_EMMC_ID:
		/* emmc reset */
		sdhci_litecrg_writel(host, IP_RST_EMMC, PERRSTEN2);
		sdhci_litecrg_writel(host, IP_RST_EMMC_BUS_BIT, PERRSTEN1);
		udelay(100);
		if ((sdhci_litecrg_readl(host, PERRSTSTAT2) & IP_RST_EMMC) !=
			IP_RST_EMMC)
			pr_err("%s: emmc iprst status err after wait 100us\n", __func__);
		if ((sdhci_litecrg_readl(host, PERRSTSTAT1) & IP_RST_EMMC_BUS_BIT) !=
			IP_RST_EMMC_BUS_BIT)
			pr_err("%s: emmc bus rst status err after wait 100us\n", __func__);

		break;
	default:
		break;
	}
}

void sdhci_zodiac_hardware_disreset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	if (!host->litecrg) {
		pr_err("%s: litecrg is null, can't disreset mmc!\n", __func__);
		return;
	}

	if (!host->crgctrl) {
		pr_err("%s: crgctrl is null, can't disreset mmc!\n", __func__);
		return;
	}

	if (!host->hsdtcrg) {
		pr_err("%s: hsdtcrg is null, can't disreset mmc!\n", __func__);
		return;
	}

	switch (sdhci_zodiac->sdhci_host_id) {
	case SDHCI_SD_ID:
		/* sd disreset */
		pr_err("%s: apus not support sd!\n", __func__);
		break;
	case SDHCI_SDIO_ID:
		/* sdio disreset */
		sdhci_crgctrl_writel(host, IP_RST_SDIO, PERRSTDIS5);
		sdhci_hsdtcrg_writel(host, IP_ARST_SDIO, PERRSTDIS0);
		udelay(100);
		if (sdhci_crgctrl_readl(host, PERRSTSTAT5) & IP_RST_SDIO)
			pr_err("%s: sdio iprst status err after wait 100us\n", __func__);
		if (sdhci_hsdtcrg_readl(host, PERRSTSTAT0) & IP_ARST_SDIO)
			pr_err("%s: sdio bus reset status err after wait 100us\n", __func__);
		break;
	case SDHCI_EMMC_ID:
		/* emmc disreset */
		sdhci_litecrg_writel(host, IP_RST_EMMC, PERRSTDIS2);
		sdhci_litecrg_writel(host, IP_RST_EMMC_BUS_BIT, PERRSTDIS1);
		udelay(100);
		if (sdhci_litecrg_readl(host, PERRSTSTAT1) & IP_RST_EMMC)
			pr_err("%s: emmc disrst status err after wait 100us\n", __func__);
		if (sdhci_litecrg_readl(host, PERRSTSTAT2) & IP_RST_EMMC_BUS_BIT)
			pr_err("%s: emmc bus disrst status err after wait 100us\n", __func__);
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

	np = of_find_compatible_node(NULL, NULL, "hisilicon,lite_crg");
	if (!np) {
		pr_err("can't find lite crg!\n");
		return -1;
	}

	host->litecrg = of_iomap(np, 0);
	if (!host->litecrg) {
		pr_err("litecrg iomap error!\n");
		return -1;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,actrl");
	if (!np) {
		pr_err("can't find actrl!\n");
		return -1;
	}

	host->actrl = of_iomap(np, 0);
	if (!host->actrl) {
		pr_err("actrl iomap error!\n");
		return -1;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (!np) {
		pr_err("can't find crgctrl!\n");
		return -1;
	}

	host->crgctrl = of_iomap(np, 0);
	if (!host->actrl) {
		pr_err("crgctrl iomap error!\n");
		return -1;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hsdt_crg");
	if (!np) {
		pr_err("can't find hsdtcrg!\n");
		return -1;
	}

	host->hsdtcrg = of_iomap(np, 0);
	if (!host->hsdtcrg) {
		pr_err("hsdtcrg iomap error!\n");
		return -1;
	}

	host->qicmmc = ioremap(IB_MMC, 0x1000);
	if (!host->qicmmc) {
		pr_err("qicmmc iomap error!\n");
		return -1;
	}

	host->rd_liteddr = ioremap(TB_LITE_DDR2DMC_RD, 0x1000);
	if (!host->rd_liteddr) {
		pr_err("rd_liteddr iomap error!\n");
		return -1;
	}

	host->wr_liteddr = ioremap(TB_LITE_DDR2DMC_WR, 0x1000);
	if (!host->wr_liteddr) {
		pr_err("wr_liteddr iomap error!\n");
		return -1;
	}

	host->rd_ddrqic = ioremap(IB_LITE_DDR2DMC_RD, 0x1000);
	if (!host->rd_ddrqic) {
		pr_err("rd_ddrqic iomap error!\n");
		return -1;
	}

	host->wr_ddrqic = ioremap(IB_LITE_DDR2DMC_WR, 0x1000);
	if (!host->wr_ddrqic) {
		pr_err("wr_ddrqic iomap error!\n");
		return -1;
	}
	sdhci_zodiac->pcrgctrl = g_zodiac_crgctrl_cfg;

	return 0;
}
