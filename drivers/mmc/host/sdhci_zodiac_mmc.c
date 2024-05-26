/*
 * ZODIAC Secure Digital Host Controller Interface.
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
#include <linux/bootdevice.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/mmc/cmdq_hci.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/card.h>
#include <soc_pmctrl_interface.h>
#include <linux/regulator/consumer.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <mntn_subtype_exception.h>
#include <linux/mmc/dsm_emmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdhci_zodiac_mux_sdsim.h>

#include "sdhci-pltfm.h"
#include "sdhci_zodiac_mmc.h"

#define DRIVER_NAME "sdhci-zodiac"
struct sdhci_host *sdhci_host_from_sd_module;
struct sdhci_host *sdhci_sdio_host;

int g_sdio_reset_ip;
EXPORT_SYMBOL(g_sdio_reset_ip);

#define SDHCI_DUMP(f, x...) \
	pr_err("%s: " DRIVER_NAME ": " f, mmc_hostname(host->mmc), ## x)

void sdhci_qic_dumpregs(struct sdhci_host *host)
{
	SDHCI_DUMP("MMC_DFX_BUSY:  0x%08x | MMC_DFX_OST:  0x%08x \n",
		readl(host->qicmmc + DFX_BUSY), readl(host->qicmmc + DFX_OST));
	SDHCI_DUMP("TB_LITE2DMC_RD_BUSY:  0x%08x | TB_LITE2DMC_RD_OST:  0x%08x \n",
		readl(host->rd_liteddr + DFX_BUSY), readl(host->rd_liteddr + DFX_OST));
	SDHCI_DUMP("TB_LITE2DMC_WR_BUSY:  0x%08x | TB_LITE2DMC_WR_OST:  0x%08x \n",
		readl(host->wr_liteddr + DFX_BUSY), readl(host->wr_liteddr + DFX_OST));
	SDHCI_DUMP("IB_LITE2DDR_RD_BUSY:  0x%08x | IB_LITE2DDR_RD_OST:  0x%08x \n",
		readl(host->rd_ddrqic + DFX_BUSY), readl(host->rd_ddrqic + DFX_OST));
	SDHCI_DUMP("IB_LITE2DDR_WR_BUSY:  0x%08x | IB_LITE2DDR_WR_OST:  0x%08x \n",
		readl(host->wr_ddrqic + DFX_BUSY), readl(host->wr_ddrqic + DFX_OST));
}

void sdhci_zodiac_dumpregs(struct sdhci_host *host)
{
	pr_err(DRIVER_NAME ": Start dump maintenance and test register\n");
	SDHCI_DUMP("tuning ctrl:  0x%08x\n", sdhci_readl(host, SDEMMC_CRG_CTRL));
	SDHCI_DUMP("==================== emmc dfx ====================");
	SDHCI_DUMP("0x80:  0x%08x | 0x84:  0x%08x \n",
		sdhci_readl(host,0x80), sdhci_readl(host,0x84));
	SDHCI_DUMP("0x88:  0x%08x | 0x90:  0x%08x \n",
		sdhci_readl(host,0x88), sdhci_readl(host,0x90));
	SDHCI_DUMP("0x94:  0x%08x | 0x98:  0x%08x \n",
		sdhci_readl(host,0x94), sdhci_readl(host,0x98));
	SDHCI_DUMP("0xA0:  0x%08x | 0xE0:  0x%08x \n",
		sdhci_readl(host,0xA0), sdhci_readl(host,0xE0));
	SDHCI_DUMP("0xFC:  0x%08x | 0x800:  0x%08x \n",
		sdhci_readl(host,0xFC), sdhci_readl(host,0x800));
	SDHCI_DUMP("0x804:  0x%08x | 0x808:  0x%08x \n",
		sdhci_readl(host,0x804), sdhci_readl(host,0x808));
	SDHCI_DUMP("0x80C:  0x%08x | 0x810:  0x%08x \n",
		sdhci_readl(host,0x80C), sdhci_readl(host,0x810));
	SDHCI_DUMP("0x814:  0x%08x | 0x818:  0x%08x \n",
		sdhci_readl(host,0x814), sdhci_readl(host,0x818));
	SDHCI_DUMP("0x81c:  0x%08x | 0x820:  0x%08x \n",
		sdhci_readl(host,0x81c), sdhci_readl(host,0x820));
	SDHCI_DUMP("0x824:  0x%08x | 0x828:  0x%08x \n",
		sdhci_readl(host,0x824), sdhci_readl(host,0x828));
	SDHCI_DUMP("0x82c:  0x%08x | 0x830:  0x%08x \n",
		sdhci_readl(host,0x82c), sdhci_readl(host,0x830));
	SDHCI_DUMP("0x834:  0x%08x | 0x838:  0x%08x \n",
		sdhci_readl(host,0x834), sdhci_readl(host,0x838));
	SDHCI_DUMP("0x83c:  0x%08x | 0x840:  0x%08x \n",
		sdhci_readl(host,0x83c), sdhci_readl(host,0x840));
	SDHCI_DUMP("0x844:  0x%08x | 0x848:  0x%08x \n",
		sdhci_readl(host,0x844), sdhci_readl(host,0x848));
	SDHCI_DUMP("0x84c:  0x%08x | 0x850:  0x%08x \n",
		sdhci_readl(host,0x84c), sdhci_readl(host,0x850));
	SDHCI_DUMP("0x854:  0x%08x | 0x858:  0x%08x \n",
		sdhci_readl(host,0x854), sdhci_readl(host,0x858));
	SDHCI_DUMP("0x85c:  0x%08x | 0x860:  0x%08x \n",
		sdhci_readl(host,0x85c), sdhci_readl(host,0x860));
	SDHCI_DUMP("0x864:  0x%08x | 0x900:  0x%08x \n",
		sdhci_readl(host,0x864), sdhci_readl(host,0x900));
	SDHCI_DUMP("0x904:  0x%08x | 0x908:  0x%08x \n",
		sdhci_readl(host,0x904), sdhci_readl(host,0x908));
	SDHCI_DUMP("0x90c:  0x%08x | 0x910:  0x%08x \n",
		sdhci_readl(host,0x90c), sdhci_readl(host,0x910));
	SDHCI_DUMP("0x914:  0x%08x | 0x918:  0x%08x \n",
		sdhci_readl(host,0x914), sdhci_readl(host,0x918));
	SDHCI_DUMP("0x91c:  0x%08x | 0x920:  0x%08x \n",
		sdhci_readl(host,0x91c), sdhci_readl(host,0x920));
	SDHCI_DUMP("0x924:  0x%08x | 0x928:  0x%08x \n",
		sdhci_readl(host,0x924), sdhci_readl(host,0x928));
	SDHCI_DUMP("0x92c:  0x%08x | 0x930:  0x%08x \n",
		sdhci_readl(host,0x92c), sdhci_readl(host,0x930));
	SDHCI_DUMP("0x934:  0x%08x | 0x938:  0x%08x \n",
		sdhci_readl(host,0x934), sdhci_readl(host,0x938));
	SDHCI_DUMP("0x93c:  0x%08x | 0x940:  0x%08x \n",
		sdhci_readl(host,0x93c), sdhci_readl(host,0x940));
	SDHCI_DUMP("0x944:  0x%08x | 0x948:  0x%08x \n",
		sdhci_readl(host,0x944), sdhci_readl(host,0x948));
	SDHCI_DUMP("0x94c:  0x%08x | 0x950:  0x%08x \n",
		sdhci_readl(host,0x94c), sdhci_readl(host,0x950));
#ifdef CONFIG_MMC_SDHCI_ZODIAC_APUS
	SDHCI_DUMP("NONSEC_STATUS20:0x%08x | NONSEC_STATUS22:0x%08x \n",
		sdhci_actrl_readl(host, NONSEC_STATUS20), sdhci_actrl_readl(host, NONSEC_STATUS22));
	sdhci_qic_dumpregs(host);
#endif
	pr_err(DRIVER_NAME ": Dump end\n");
}

int sdhci_check_himntn(void)
{
#ifdef CONFIG_DFX_PLATFORM_MAINTAIN
	return check_mntn_switch(MNTN_SD2JTAG) ||
			check_mntn_switch(MNTN_SD2DJTAG) ||
			check_mntn_switch(MNTN_SD2UART6);
#else
		return 0;
#endif
}

void sdhci_sdio_card_detect(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = NULL;
	struct sdhci_zodiac_data *sdhci_zodiac = NULL;

	if (!host) {
		pr_err("%s sdio detect, host is null,can not used to detect sdio\n", __func__);
		return;
	}

	if (host->mmc) {
		host->mmc->sdio_present = 1;
		dev_err(&host->mmc->class_dev, "sdio_present = %d\n",
			host->mmc->sdio_present);
	}

	pltfm_host = sdhci_priv(host);
	sdhci_zodiac = pltfm_host->priv;
	queue_work(sdhci_zodiac->card_workqueue, &sdhci_zodiac->card_work);
}

/* interface for wifi */
void dw_mci_sdio_card_detect_change(void)
{
	sdhci_sdio_card_detect(sdhci_sdio_host);
}
EXPORT_SYMBOL(dw_mci_sdio_card_detect_change);

static void sdhci_zodiac_gpio_value_set(int value, int gpio_num)
{
	int err;

	err = gpio_request(gpio_num, "gpio_num");
	if (err < 0) {
		pr_err("Can`t request gpio number %d\n", gpio_num);
		return;
	}

	pr_info("%s mmc gpio num: %d, lever_value: %d\n", __func__, gpio_num, value);
	gpio_direction_output(gpio_num, value);
	gpio_set_value(gpio_num, value);
	gpio_free(gpio_num);
}

static void sdhci_zodiac_nsb_cfg(struct sdhci_host *host, unsigned int value)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	unsigned int reg;

	if (sdhci_zodiac->sdhci_host_id != SDHCI_SD_ID)
		return;

	reg = sdhci_readl(host, SDEMMC_TIMING_CTRL_1);
	reg &= ~SDEMMC_NSB_CFG_MASK;
	reg |= value;
	sdhci_writel(host, reg, SDEMMC_TIMING_CTRL_1);
}

static void sdhci_zodiac_sel18_sdcard(struct sdhci_host *host, int set)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	unsigned int reg;

	if (sdhci_zodiac->sdhci_host_id != SDHCI_SD_ID)
		return;

	/* sdcard io select 1.8v bit 11 set b1 */
	reg = readl(host->actrl + ACTRL_BITMSK_NONSEC_CTRL1);
	if (set)
		reg |= 0x1 << SDCARD_SEL18_OFFSET;
	else
		reg &= ~(0x1 << SDCARD_SEL18_OFFSET);
	reg |= 0x1 << SDCARD_SEL18_BITMASK;
	writel(reg, host->actrl + ACTRL_BITMSK_NONSEC_CTRL1);
}

static unsigned int sdhci_zodiac_get_min_clock(struct sdhci_host *host)
{
	return SDHCI_ZODIAC_MIN_FREQ;
}

static void sdhci_zodiac_set_fpga_clk(struct sdhci_host *host, unsigned int clock)
{
	u16 clk = 0;

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	clk |= SDHCI_CLOCK_INT_EN;
	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);

	if (clock > SDHCI_ZODIAC_CLK_400K)
		sdhci_writel(host, SDHCI_SDEMMC_CLK_DIV_10M | SDHCI_SDEMMC_CLK_DLY_SAMPLE_180, SDHCI_CRG_CTRL0);
	else
		sdhci_writel(host, SDHCI_SDEMMC_CLK_DLY_SAMPLE_180, SDHCI_CRG_CTRL0);
}

static void sdhci_zodiac_disable_clk(struct sdhci_host *host)
{
	u16 reg;
	ktime_t timeout;

	if (host->quirks2 & SDHCI_QUIRK2_ZODIAC_FPGA)
		return;

	reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	timeout = ktime_add_ms(ktime_get(), SDHCI_ZODIAC_CLK_STABLE_TIME);
	while (1) {
		bool timedout = ktime_after(ktime_get(), timeout);
		reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
		if (!(reg & SDHCI_CLOCK_INT_STABLE))
			break;
		if (timedout) {
			pr_err("%s: Internal clock never stabilised.\n",
				mmc_hostname(host->mmc));
				sdhci_dumpregs(host);
				return;
		}
		udelay(10);
	}
}

static void sdhci_zodiac_enable_clk(struct sdhci_host *host)
{
	u16 reg;
	ktime_t timeout;

	if (host->quirks2 & SDHCI_QUIRK2_ZODIAC_FPGA) {
		sdhci_zodiac_set_fpga_clk(host, host->clock);
		return;
	}

	reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	timeout = ktime_add_ms(ktime_get(), SDHCI_ZODIAC_CLK_STABLE_TIME);
	while (1) {
		bool timedout = ktime_after(ktime_get(), timeout);

		reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
		if (reg & SDHCI_CLOCK_INT_STABLE)
			break;
		if (timedout) {
			pr_err("%s: Internal clock never stabilised.\n",
			       mmc_hostname(host->mmc));
			sdhci_dumpregs(host);
			return;
		}
		udelay(10);
	}

	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);
}

static void sdhci_zodiac_select_clk_src(struct sdhci_host *host, struct mmc_ios *ios)
{
	u32 reg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	struct sdhci_crg_ctrl *crg = NULL;

	if (IS_ERR(sdhci_zodiac->ciu_clk)) {
		pr_err("%s failed, ciu_clk err\n", __func__);
		return;
	}

	if (ios->timing >= SDHCI_ZODIAC_TIMING_MODE) {
		pr_err("%s failed, timing err %d\n", __func__, ios->timing);
		return;
	}

	crg = sdhci_zodiac->pcrgctrl + ios->timing;
	clk_disable_unprepare(sdhci_zodiac->ciu_clk);

	if (clk_set_rate(sdhci_zodiac->ciu_clk, crg->clk_src))
		pr_err("%s clk set rate failed, timing = %d\n", __func__, ios->timing);

	reg = sdhci_readl(host, SDHCI_CRG_CTRL0);
	reg &= ~(SDHCI_ZODIAC_CLK_DLY_SAMPLE_MASK | SDHCI_ZODIAC_CLK_DLY_DRV_MASK | SDHCI_ZODIAC_CLK_DIV_MASK);
	reg |= (crg->clk_div << SDHCI_ZODIAC_CLK_DIV_OFFSET) | \
			(crg->clk_dly_drv << SDHCI_ZODIAC_CLK_DLY_DRV_OFFSET) | \
			(crg->clk_dly_sample << SDHCI_ZODIAC_CLK_DLY_SAMPLE_OFFSET);
	host->max_clk = crg->max_clk;

	sdhci_writel(host, reg, SDHCI_CRG_CTRL0);

	if (clk_prepare_enable(sdhci_zodiac->ciu_clk))
		pr_err("%s ciu clk prepare enable failed\n", __func__);
}

static void sdhci_zodiac_set_clock(struct sdhci_host *host, unsigned int clock)
{
	u16 clk;

	if (host->quirks2 & SDHCI_QUIRK2_ZODIAC_FPGA) {
		sdhci_zodiac_set_fpga_clk(host, clock);
		return;
	}

	host->mmc->actual_clock = 0;
	sdhci_zodiac_disable_clk(host);
	/* clock = 0 means just turning it off */
	if (clock == 0)
		return;
	sdhci_zodiac_select_clk_src(host, &host->mmc->ios);
	clk = sdhci_calc_clk(host, clock, &host->mmc->actual_clock);
	sdhci_enable_clk(host, clk);

	/* need to wait 8 sd clk befor a new command,
	 * 8 clk equal to 20us when 400k */
	mdelay(1);
}

static unsigned int sdhci_zodiac_get_timeout_clock(struct sdhci_host *host)
{
	u32 div;
	unsigned long freq;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	div = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	div = (div & CLK_CTRL_TIMEOUT_MASK) >> CLK_CTRL_TIMEOUT_SHIFT;

	freq = clk_get_rate(sdhci_zodiac->ciu_clk);
	freq /= (u32)(1 << (CLK_CTRL_TIMEOUT_MIN_EXP + div));

	dev_info(&host->mmc->class_dev, "%s: freq=%lx\n", __func__, freq);

	return freq;
}

void sdhci_zodiac_od_enable(struct sdhci_host *host)
{
	/* configure OD enable */
	pr_debug("%s: end!\n", __func__);
}

int sdhci_disable_open_drain(struct sdhci_host *host)
{
	pr_debug("%s\n", __func__);
	return 0;
}

static void sdhci_zodiac_hw_reset(struct sdhci_host *host)
{
	sdhci_zodiac_hardware_reset(host);
	sdhci_zodiac_hardware_disreset(host);
}

int sdhci_chk_busy_before_send_cmd(struct sdhci_host *host,
	struct mmc_command *cmd)
{
	unsigned long timeout;

	/* We shouldn't wait for busy for stop commands */
	if ((cmd->opcode != MMC_STOP_TRANSMISSION) && (cmd->opcode != MMC_SEND_STATUS) &&
	    (cmd->opcode != MMC_GO_IDLE_STATE)) {
		/* Wait busy max 10 s */
		timeout = 10000;
		while (!(sdhci_readl(host, SDHCI_PRESENT_STATE) & SDHCI_DATA_0_LVL_MASK)) {
			if (timeout == 0) {
				dev_err(&host->mmc->class_dev, "%s: wait busy 10s time out.\n",
						mmc_hostname(host->mmc));
				sdhci_dumpregs(host);
				cmd->error = -ENOMSG;
				queue_work(host->complete_wq, &host->complete_work);
				return -ENOMSG;
			}
			timeout--;
			mdelay(1);
		}
	}
	return 0;
}

int sdhci_zodiac_enable_dma(struct sdhci_host *host)
{
	u16 ctrl;

	if (host->runtime_suspended)
		return 0;

#ifndef CONFIG_MMC_SDHCI_ZODIAC
	sdhci_enable_v4_mode(host);
#endif
	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	if (host->flags & SDHCI_USE_64_BIT_DMA)
		ctrl |= SDHCI_CTRL_64BIT_ADDR;
	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

	ctrl = (u16)sdhci_readb(host, SDHCI_HOST_CONTROL);
	ctrl &= ~SDHCI_CTRL_DMA_MASK;
	if (host->flags & SDHCI_USE_ADMA)
		ctrl |= SDHCI_CTRL_ADMA2;
	else
		ctrl |= SDHCI_CTRL_SDMA;
	sdhci_writeb(host, (u8)ctrl, SDHCI_HOST_CONTROL);

	return 0;
}

static void sdhci_zodiac_restore_transfer_para(struct sdhci_host *host)
{
	u16 mode;

	sdhci_zodiac_enable_dma(host);
	sdhci_set_transfer_irqs(host);

	mode = SDHCI_TRNS_BLK_CNT_EN;
	mode |= SDHCI_TRNS_MULTI;
	if (host->flags & SDHCI_REQ_USE_DMA)
		mode |= SDHCI_TRNS_DMA;
	sdhci_writew(host, mode, SDHCI_TRANSFER_MODE);
	/* Set the DMA boundary value and block size */
	sdhci_writew(host, SDHCI_MAKE_BLKSZ((u16)SDHCI_DEFAULT_BOUNDARY_ARG,
		512), SDHCI_BLOCK_SIZE);
}

void sdhci_zodiac_select_card_type(struct sdhci_host *host)
{

}

static int sdhci_zodiac_execute_hwtuning(struct sdhci_host *host, u32 opcode)
{
	int i;
	int err = -EIO;

	for (i = 0; i < MAX_TUNING_LOOP; i++) {
		u16 ctrl;
		sdhci_zodiac_disable_clk(host);
		sdhci_zodiac_enable_clk(host);
		sdhci_send_tuning(host, opcode);

		if (!host->tuning_done) {
			pr_err("%s: Tuning timeout, falling back to fixed sampling clock\n",
					mmc_hostname(host->mmc));
			sdhci_abort_tuning(host, opcode);
			err = -EIO;
			goto out;
		}

		ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		if (!(ctrl & SDHCI_CTRL_EXEC_TUNING)) {
			if (ctrl & SDHCI_CTRL_TUNED_CLK)
				return 0; /* Success! */
			err = -EIO;
			break;
		}

		/* Spec does not require a delay between tuning cycles */
		if (host->tuning_delay > 0)
			mdelay(host->tuning_delay);
	}

	pr_info("%s: Tuning failed, falling back to fixed sampling clock\n",
			mmc_hostname(host->mmc));
	sdhci_reset_tuning(host);

out:
	return err;
}

void sdhci_zodiac_set_uhs_signaling(struct sdhci_host *host, unsigned int timing)
{
	u16 ctrl;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl &= ~SDHCI_CTRL_UHS_MASK;
	if (timing == MMC_TIMING_MMC_HS200)
		ctrl |= SDHCI_CTRL_HS_HS200;
	else if (timing == MMC_TIMING_MMC_HS400)
		ctrl |= SDHCI_CTRL_HS_HS400;
	else if (timing == MMC_TIMING_MMC_HS)
		ctrl |= SDHCI_CTRL_HS_SDR;
	else if (timing == MMC_TIMING_MMC_DDR52)
		ctrl |= SDHCI_CTRL_HS_DDR;

	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
}

static void sdhci_zodiac_init_tuning_para(struct sdhci_zodiac_data *sdhci_zodiac)
{
	sdhci_zodiac->tuning_count = 0;
	sdhci_zodiac->tuning_phase_record = 0;
	sdhci_zodiac->tuning_phase_best = TUNING_PHASE_INVALID;

	sdhci_zodiac->tuning_move_phase = TUNING_PHASE_INVALID;
	sdhci_zodiac->tuning_move_count = 0;
	sdhci_zodiac->tuning_phase_max = 10;
	sdhci_zodiac->tuning_phase_min = 0;
	sdhci_zodiac->tuning_loop_max = TUNING_LOOP_COUNT;

}

static void sdhci_zodiac_set_tuning_phase(struct sdhci_host *host, u32 val)
{
	u32 reg_val;

	sdhci_zodiac_disable_clk(host);

	reg_val = sdhci_readl(host, SDEMMC_CRG_CTRL);
	reg_val &= ~TUNING_SAMPLE_MASK;
	reg_val |= (val << TUNING_SAMPLE_MASKBIT);
	sdhci_writel(host, reg_val, SDEMMC_CRG_CTRL);

	sdhci_zodiac_enable_clk(host);
}

static void sdhci_zodiac_save_tuning_phase(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	sdhci_zodiac->tuning_phase_record |= 0x1 << (sdhci_zodiac->tuning_count);
}

static void sdhci_zodiac_add_tuning_phase(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	sdhci_zodiac->tuning_count++;

	sdhci_zodiac_set_tuning_phase(host, sdhci_zodiac->tuning_count);
}

#define cror(data, shift) ((data >> shift) | (data << (10 - shift)))
static int sdhci_zodiac_find_best_phase(struct sdhci_host *host)
{
	u64 tuning_phase_record;
	u32 loop = 0;
	u32 max_loop;
	u32 max_continuous_len = 0;
	u32 current_continuous_len = 0;
	u32 max_continuous_start = 0;
	u32 current_continuous_start = 0;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	max_loop = sdhci_zodiac->tuning_loop_max;

	dev_err(&host->mmc->class_dev, "zodiac tuning_phase_record: 0x%llx,\n",
									sdhci_zodiac->tuning_phase_record);
	tuning_phase_record = sdhci_zodiac->tuning_phase_record;
	/* bit0 and bit9 continuous in circles */
	if ((tuning_phase_record & (0x1 << sdhci_zodiac->tuning_phase_min)) &&
		(tuning_phase_record & (0x1 << (sdhci_zodiac->tuning_phase_max - 1))))
		max_loop = sdhci_zodiac->tuning_loop_max * SDHCI_SAM_PHASE_MID;

	do {
		if (tuning_phase_record & 0x1) {
			if (!current_continuous_len)
				current_continuous_start = loop;

			current_continuous_len++;
			if (loop == max_loop - 1) {
				if (current_continuous_len > max_continuous_len) {
					max_continuous_len = current_continuous_len;
					max_continuous_start = current_continuous_start;
				}
			}
		} else {
			if (current_continuous_len) {
				if (current_continuous_len > max_continuous_len) {
					max_continuous_len = current_continuous_len;
					max_continuous_start = current_continuous_start;
				}

				current_continuous_len = 0;
			}
		}

		loop++;
		tuning_phase_record = cror(tuning_phase_record, 1);
	} while (loop < max_loop);

	if (!max_continuous_len)
		return -1;

	sdhci_zodiac->tuning_phase_best = max_continuous_start + max_continuous_len / 2;
	dev_err(&host->mmc->class_dev, "%s: soft tuning best phase:0x%x\n",
			dev_name(sdhci_zodiac->dev), sdhci_zodiac->tuning_phase_best);

	return 0;
}

static int sdhci_zodiac_tuning_move_clk(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	int loop = 0;
	int ret = 0;
	int move_step = 2;

	/* soft tuning fail or error then return error */
	if (sdhci_zodiac->tuning_phase_best >= sdhci_zodiac->tuning_loop_max) {
		dev_err(&host->mmc->class_dev, "%s: soft tuning fail, can not move tuning, tuning_init_sample=%d\n",
				__func__, sdhci_zodiac->tuning_phase_best);
		return -1;
	}
	/*
	  * first move tuning, set soft tuning optimum sample. When second or more move
	  * tuning, use the sample near optimum sample
	  */
	for (loop = 0; loop < 2; loop++) {
		sdhci_zodiac->tuning_move_count++;
		sdhci_zodiac->tuning_move_phase = sdhci_zodiac->tuning_phase_best
			+ ((sdhci_zodiac->tuning_move_count % 2) ? move_step : -move_step)
			   * (sdhci_zodiac->tuning_move_count / 2);

		if ((sdhci_zodiac->tuning_move_phase >= sdhci_zodiac->tuning_phase_max)
			|| (sdhci_zodiac->tuning_move_phase < sdhci_zodiac->tuning_phase_min)) {
			continue;
		} else {
			break;
		}
	}

	if ((sdhci_zodiac->tuning_move_phase >= sdhci_zodiac->tuning_loop_max)
		|| (sdhci_zodiac->tuning_move_phase < sdhci_zodiac->tuning_phase_min)) {
		dev_err(&host->mmc->class_dev, "%s: tuning move fail.\n", __func__);
		sdhci_zodiac->tuning_move_phase = TUNING_PHASE_INVALID;
		ret = -1;
	} else {
		sdhci_zodiac_set_tuning_phase(host, sdhci_zodiac->tuning_move_phase);
		dev_err(&host->mmc->class_dev, "%s: tuning move to phase=%d\n",
								__func__, sdhci_zodiac->tuning_move_phase);
	}

	return ret;
}

static int sdhci_zodiac_tuning_move(struct sdhci_host *host, int is_move_strobe, int flag)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	/* set tuning_strobe_move_count to 0, next tuning move will begin from optimum sample */
	if (flag) {
		sdhci_zodiac->tuning_move_count = 0;
		return 0;
	}

	if (is_move_strobe) {
		dev_err(&host->mmc->class_dev, "%s sd need not strobe tuning move\n", __func__);
		return 0;
	} else {
		return sdhci_zodiac_tuning_move_clk(host);
	}
}

static const u8 tuning_blk_pattern_4bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const u8 tuning_blk_pattern_8bit[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static void sdhci_zodiac_tuning_before_set(struct sdhci_host *host)
{
	u16 reg_val;

	reg_val = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg_val &= ~SDHCI_CTRL_TUNED_CLK;
	sdhci_writew(host, reg_val, SDHCI_HOST_CONTROL2);
}

static void sdhci_zodiac_send_tuning(struct sdhci_host *host, u32 opcode,
	u8 *tuning_blk, int blksz, const u8 *tuning_blk_pattern)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;

	cmd.opcode = opcode;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.error = 0;

	data.blksz = (u32)blksz;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	data.error = 0;
	data.timeout_ns = 50000000; /* 50ms */

	sg_init_one(&sg, tuning_blk, blksz);

	mrq.cmd = &cmd;
	mrq.data = &data;

	mmc_wait_for_req(host->mmc, &mrq);
	/*
	 * no cmd or data error and tuning data is ok,
	 * then set sample flag
	 */
	if (!cmd.error && !data.error && tuning_blk &&
		(memcmp(tuning_blk, tuning_blk_pattern, sizeof(blksz)) == 0))
		sdhci_zodiac_save_tuning_phase(host);
}

static int sdhci_zodiac_soft_tuning(struct sdhci_host *host, u32 opcode, bool set)
{
	struct sdhci_zodiac_data *sdhci_zodiac = ((struct sdhci_pltfm_host *)sdhci_priv(host))->priv;
	unsigned int tuning_loop_counter;
	int ret;
	const u8 *tuning_blk_pattern = NULL;
	u8 *tuning_blk = NULL;
	int blksz = 0;

	if (set == false) {
		/* cmdq blocksize 512byte , multi dma trans */
		sdhci_writew(host, SDHCI_MAKE_BLKSZ((u16)SDHCI_DEFAULT_BOUNDARY_ARG,
			MMC_BLOCK_SIZE), SDHCI_BLOCK_SIZE);
		sdhci_writew(host, SDHCI_TRNS_DMA | SDHCI_TRNS_BLK_CNT_EN | SDHCI_TRNS_MULTI, SDHCI_TRANSFER_MODE);
		return 0;
	}

	sdhci_zodiac_init_tuning_para(sdhci_zodiac);
	tuning_loop_counter = sdhci_zodiac->tuning_loop_max;

	host->flags |= SDHCI_EXE_SOFT_TUNING;
	dev_err(&host->mmc->class_dev, "%s: %s: now, start tuning soft...\n",
								dev_name(sdhci_zodiac->dev), __func__);

	if (opcode == MMC_SEND_TUNING_BLOCK_HS200) {
		if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_8) {
			tuning_blk_pattern = tuning_blk_pattern_8bit;
			blksz = BLKSZ_LARGE;
		} else if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_4) {
			tuning_blk_pattern = tuning_blk_pattern_4bit;
			blksz = BLKSZ_SMALL;
		}
	} else {
		tuning_blk_pattern = tuning_blk_pattern_4bit;
		blksz = BLKSZ_SMALL;
	}
	tuning_blk = kzalloc(blksz, GFP_KERNEL);
	if (!tuning_blk) {
		ret = -ENOMEM;
		goto err;
	}

	sdhci_zodiac_tuning_before_set(host);
	sdhci_zodiac_set_tuning_phase(host, 0);

	while (tuning_loop_counter--) {
		sdhci_zodiac_send_tuning(host, opcode, tuning_blk, blksz, tuning_blk_pattern);
		sdhci_zodiac_add_tuning_phase(host);
	}

	if (tuning_blk)
		kfree(tuning_blk);

	ret = sdhci_zodiac_find_best_phase(host);
	if (ret) {
		dev_err(&host->mmc->class_dev, "can not find best phase\n");
		goto err;
	}

	sdhci_zodiac_set_tuning_phase(host, sdhci_zodiac->tuning_phase_best);

err:
	/* restore block size after tuning */
	sdhci_writew(host, SDHCI_MAKE_BLKSZ((u16)SDHCI_DEFAULT_BOUNDARY_ARG,
		MMC_BLOCK_SIZE), SDHCI_BLOCK_SIZE);

	host->flags &= ~SDHCI_EXE_SOFT_TUNING;
	return ret;
}

static int sdhci_zodiac_platform_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	int err;

	if (host->tuning_delay < 0)
		host->tuning_delay = opcode == MMC_SEND_TUNING_BLOCK;

	sdhci_start_tuning(host);
	err = sdhci_zodiac_execute_hwtuning(host, opcode);
	sdhci_end_tuning(host);
	sdhci_zodiac_disable_clk(host);
	sdhci_zodiac_enable_clk(host);

	return err;
}

/*
 * Synopsys controller's max busy timeout is 6.99s. Although we do not
 * use the timeout clock, but the function mmc_do_calc_max_discard use it,
 * if the the card's default erase timeout is more than 6.99s, the discard
 * cannot be supported.
 * As we set the request timeout 30s, so we set a busy timout value which
 * is less than it, choose 1 << 29, the busy clk is 20M,
 * timeout 1 << 29 / 20000 = 26.8s
 */
u32 sdhci_zodiac_get_max_timeout_count(struct sdhci_host *host)
{
	return 1 << 29;
}

static void sdhci_zodiac_set_pinctrl_idle(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	int ret;

	if (sdhci_check_himntn()) {
		dev_info(&host->mmc->class_dev, "%s himntn enable sd pinctrl return\n", __func__);
		return;
	}

	if ((sdhci_zodiac->pinctrl) && (sdhci_zodiac->pins_idle)) {
		ret = pinctrl_select_state(sdhci_zodiac->pinctrl, sdhci_zodiac->pins_idle);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s could not set idle pins\n", __func__);
	}
}

static void sdhci_zodiac_set_pinctrl_normal(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	int ret;

	if (sdhci_check_himntn()) {
		dev_info(&host->mmc->class_dev, "%s himntn enable sd pinctrl return\n", __func__);
		return;
	}

	if ((sdhci_zodiac->pinctrl) && (sdhci_zodiac->pins_default)) {
		ret = pinctrl_select_state(sdhci_zodiac->pinctrl, sdhci_zodiac->pins_default);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s could not set default pins\n", __func__);
	}
}

static void sdhci_zodiac_power_off(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	int ret = 0;

	sdhci_zodiac_set_pinctrl_idle(host);

	sdhci_writeb(host, 0, SDHCI_POWER_CONTROL);
	if (!IS_ERR(mmc->supply.vmmc)) {
		ret = regulator_disable(mmc->supply.vmmc);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s mmc vmmc disable failure\n", __func__);
	}

	if (!IS_ERR(mmc->supply.vqmmc)) {
		ret = regulator_disable(mmc->supply.vqmmc);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s mmc vqmmc disable failure\n", __func__);
	}

	if (sdhci_zodiac->lever_shift)
		sdhci_zodiac_gpio_value_set(LEVER_SHIFT_3_0V, LEVER_SHIFT_GPIO);
}

static void sdhci_zodiac_power_up(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	int ret = 0;

	if (!IS_ERR(mmc->supply.vqmmc)) {
		if (sdhci_zodiac->mux_sdsim) {
			sdhci_zodiac_sel18_sdcard(host, 1);
			/* set vqmmc 1.8V */
			ret = regulator_set_voltage(mmc->supply.vqmmc, 1800000, 1800000);
		} else {
			sdhci_zodiac_nsb_cfg(host, SDEMMC_NSB_CFG_LONG);
			sdhci_zodiac_sel18_sdcard(host, 0);
			/* set vqmmc 2.95V */
			ret = regulator_set_voltage(mmc->supply.vqmmc, 2950000, 2950000);
		}
		if (ret)
			dev_info(&host->mmc->class_dev, "%s mmc vqmmc set failure\n", __func__);

		ret = regulator_enable(mmc->supply.vqmmc);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s mmc vqmmc enable failure\n", __func__);
		usleep_range(1000, 1500);
	}

	if (sdhci_zodiac->lever_shift)
		sdhci_zodiac_gpio_value_set(LEVER_SHIFT_1_8V, LEVER_SHIFT_GPIO);

	if (!IS_ERR(mmc->supply.vmmc)) {
		if (sdhci_zodiac->mux_sdsim && sdhci_zodiac->mux_vcc_status == MUX_SDSIM_VCC_STATUS_1_8_0_V)
			/* set vmmc 1.8V */
			ret = regulator_set_voltage(mmc->supply.vmmc, 1800000, 1800000);
		else
			/* set vmmc 2.95V */
			ret = regulator_set_voltage(mmc->supply.vmmc, 2950000, 2950000);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s mmc vmmc set failure\n", __func__);

		ret = regulator_enable(mmc->supply.vmmc);
		if (ret)
			dev_info(&host->mmc->class_dev, "%s mmc vmmc enable failure\n", __func__);
		usleep_range(1000, 1500);
	}

	sdhci_writeb(host, SDHCI_POWER_ON, SDHCI_POWER_CONTROL);
	sdhci_zodiac_set_pinctrl_normal(host);
}

static void sdhci_zodiac_set_power(struct sdhci_host *host, unsigned char mode, unsigned short vdd)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	if (sdhci_zodiac->old_power_mode == mode) /* no need change power */
		return;

	switch (mode) {
	case MMC_POWER_OFF:
		dev_info(&host->mmc->class_dev, "%s: %s set io to lowpower\n", dev_name(sdhci_zodiac->dev), __func__);
		sdhci_zodiac_power_off(host);
		break;
	case MMC_POWER_UP:
		dev_info(&host->mmc->class_dev, "%s: %s set io to normal\n", dev_name(sdhci_zodiac->dev), __func__);
		sdhci_zodiac_power_up(host);
		break;
	case MMC_POWER_ON:
		break;
	default:
		dev_info(&host->mmc->class_dev, "%s: %s unknown power supply mode\n", dev_name(sdhci_zodiac->dev), __func__);
		break;
	}
	sdhci_zodiac->old_power_mode = mode;
}

static int sdhci_zodiac_get_cd_status(struct sdhci_host *host)
{
	int status;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	if (sdhci_zodiac->sdhci_host_id == SDHCI_SDIO_ID)
		return host->mmc->sdio_present;

	/* cd_vol = 1 means sdcard gpio detect pin active-high */
	if (sdhci_zodiac->cd_vol)
		status = !gpio_get_value(sdhci_zodiac->gpio_cd);
	else /* cd_vol = 0 means sdcard gpio detect pin active-low */
		status = gpio_get_value(sdhci_zodiac->gpio_cd);

	dev_info(&host->mmc->class_dev, "%s: %s sd status = %d from gpio_get_value(gpio_cd)\n",
			dev_name(sdhci_zodiac->dev), __func__, status);

	/* If sd to jtag func enabled, make the SD always not present */
	if ((sdhci_zodiac->sdhci_host_id == SDHCI_SD_ID) && sdhci_check_himntn()) {
		dev_info(&host->mmc->class_dev, "%s: %s sd status set to 1 here because jtag is enabled\n", dev_name(sdhci_zodiac->dev), __func__);
		status = 1;
	}

#ifdef CONFIG_MMC_SDHCI_MUX_SDSIM
	if (sdhci_zodiac->sdhci_host_id == SDHCI_SD_ID)
		status = sd_sim_detect_run(host, status, MODULE_SD,
				SLEEP_MS_TIME_FOR_DETECT_UNSTABLE);
#endif
	dev_info(&host->mmc->class_dev, " sd status = %u\n", status);

	if (!status) {
		dev_info(&host->mmc->class_dev, "card is present\n");
	} else {
		dev_info(&host->mmc->class_dev, "card is not present\n");
		sdhci_zodiac->card_detect = CARD_UNDETECT;
	}

	return !status;
}

static int sdhci_zodiac_get_cd(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	pm_runtime_get_sync(mmc_dev(host->mmc));
	if (sdhci_zodiac->card_detect == CARD_UNDETECT)
		sdhci_zodiac->card_detect = sdhci_zodiac_get_cd_status(host);

	pm_runtime_mark_last_busy(mmc_dev(host->mmc));
	pm_runtime_put_autosuspend(mmc_dev(host->mmc));

	return sdhci_zodiac->card_detect;
}

void sdhci_zodiac_voltage_switch(struct sdhci_host *host)
{
	sdhci_zodiac_sel18_sdcard(host, 1);
	sdhci_zodiac_nsb_cfg(host, SDEMMC_NSB_CFG_SHORT);
}

static struct sdhci_ops sdhci_zodiac_ops = {
	.get_min_clock = sdhci_zodiac_get_min_clock,
	.set_clock = sdhci_zodiac_set_clock,
	.enable_dma = sdhci_zodiac_enable_dma,
	.get_max_clock = sdhci_pltfm_clk_get_max_clock,
	.get_timeout_clock = sdhci_zodiac_get_timeout_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.set_uhs_signaling = sdhci_zodiac_set_uhs_signaling,
	.tuning_soft = sdhci_zodiac_soft_tuning,
	.tuning_move = sdhci_zodiac_tuning_move,
	.platform_execute_tuning = sdhci_zodiac_platform_execute_tuning,
	.check_busy_before_send_cmd = sdhci_chk_busy_before_send_cmd,
	.restore_transfer_para = sdhci_zodiac_restore_transfer_para,
	.hw_reset = sdhci_zodiac_hw_reset,
	.select_card_type = sdhci_zodiac_select_card_type,
	.dump_vendor_regs = sdhci_zodiac_dumpregs,
	.get_max_timeout_count = sdhci_zodiac_get_max_timeout_count,
	.set_power = sdhci_zodiac_set_power,
	.sdhci_get_cd = sdhci_zodiac_get_cd,
	.voltage_switch = sdhci_zodiac_voltage_switch,
};

static struct sdhci_pltfm_data sdhci_zodiac_pdata = {
	.ops = &sdhci_zodiac_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN | SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN | SDHCI_QUIRK2_USE_1_8_V_VMMC,
};

#ifdef CONFIG_PM_SLEEP
/*
 * sdhci_zodiac_suspend - Suspend method for the driver
 * @dev:	Address of the device structure
 * Returns 0 on success and error value on error
 *
 * Put the device in a low power state.
 */
static int sdhci_zodiac_suspend(struct device *dev)
{
	int ret;
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	dev_info(dev, "%s: suspend +\n", __func__);
	pm_runtime_get_sync(dev);

	ret = sdhci_suspend_host(host);
	if (ret)
		return ret;

	sdhci_zodiac_disable_clk(host);
	if (!(host->quirks2 & SDHCI_QUIRK2_ZODIAC_FPGA))
		clk_disable_unprepare(sdhci_zodiac->ciu_clk);
	sdhci_zodiac_hardware_reset(host);

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	dev_info(dev, "%s: suspend -\n", __func__);

	return 0;
}

/*
 * sdhci_zodiac_resume - Resume method for the driver
 * @dev:	Address of the device structure
 * Returns 0 on success and error value on error
 *
 * Resume operation after suspend
 */
static int sdhci_zodiac_resume(struct device *dev)
{
	int ret;
	u8 pwr;
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	dev_info(dev, "%s: resume +\n", __func__);
	pm_runtime_get_sync(dev);

	sdhci_zodiac_hardware_disreset(host);

	if (!(host->quirks2 & SDHCI_QUIRK2_ZODIAC_FPGA)) {
		ret = clk_prepare_enable(sdhci_zodiac->ciu_clk);
		if (ret) {
			dev_err(dev, "Cannot enable SD clock.\n");
			return ret;
		}
		dev_info(dev, "%s: sdhci_zodiac->clk=%lu.\n", __func__, clk_get_rate(sdhci_zodiac->ciu_clk));
	}

	pwr = sdhci_readb(host, SDHCI_HOST_CONTROL);
	pwr |= SDHCI_POWER_ON;
	sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);
	sdhci_zodiac_enable_clk(host);
	dev_info(dev, "%s: host->mmc->ios.clock=%d, timing=%d.\n", __func__, host->mmc->ios.clock, host->mmc->ios.timing);

	ret = sdhci_resume_host(host);
	if (ret)
		return ret;
	/*
	 * use soft tuning sample send wake up cmd then retuning
	 * phase set need to after sdhci_resume_host,because phase may
	 * be clear by host control2 set.The flow need to be provided
	 * by soc,fix me;
	 */
	if (host->mmc->ios.timing >= MMC_TIMING_MMC_HS200) {
		dev_info(dev, "%s: tuning_move_sample=%d, tuning_init_sample=%d\n",
			__func__, sdhci_zodiac->tuning_move_phase, sdhci_zodiac->tuning_phase_best);
		if (sdhci_zodiac->tuning_move_phase == TUNING_PHASE_INVALID)
			sdhci_zodiac->tuning_move_phase = sdhci_zodiac->tuning_phase_best;
		sdhci_zodiac_set_tuning_phase(host, sdhci_zodiac->tuning_move_phase);

		dev_err(dev, "resume,host_clock = %d, mmc_clock = %d\n", host->clock, host->mmc->ios.clock);
	}

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	dev_info(dev, "%s: resume -\n", __func__);

	return 0;
}
#endif /* ! CONFIG_PM_SLEEP */

#ifdef CONFIG_PM
int sdhci_zodiac_runtime_suspend_host(struct sdhci_host *host)
{
	unsigned long flags;

	mmc_retune_timer_stop(host->mmc);
	mmc_retune_needed(host->mmc);

	spin_lock_irqsave(&host->lock, flags);
	host->ier &= SDHCI_INT_CARD_INT;
	sdhci_writel(host, host->ier, SDHCI_INT_ENABLE);
	sdhci_writel(host, host->ier, SDHCI_SIGNAL_ENABLE);
	spin_unlock_irqrestore(&host->lock, flags);

	synchronize_hardirq(host->irq);

	spin_lock_irqsave(&host->lock, flags);
	host->runtime_suspended = true;
	spin_unlock_irqrestore(&host->lock, flags);

	return 0;
}

static int sdhci_zodiac_runtime_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	int ret = 0;

	ret = sdhci_zodiac_runtime_suspend_host(host);

	sdhci_zodiac_disable_clk(host);

	if (!IS_ERR(sdhci_zodiac->ciu_clk))
		clk_disable_unprepare(sdhci_zodiac->ciu_clk);

	return ret;
}

static int sdhci_zodiac_runtime_resume_host(struct sdhci_host *host)
{
	unsigned long flags;

	sdhci_set_default_irqs(host);

	spin_lock_irqsave(&host->lock, flags);
	host->runtime_suspended = false;
#ifdef CONFIG_MMC_CQ_HCI
	if (host->mmc->cmdq_ops && host->mmc->cmdq_ops->restore_irqs)
		host->mmc->cmdq_ops->restore_irqs(host->mmc);
#endif
	spin_unlock_irqrestore(&host->lock, flags);

	return 0;
}

static int sdhci_zodiac_runtime_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;


	if (!IS_ERR(sdhci_zodiac->ciu_clk)) {
		if (clk_prepare_enable(sdhci_zodiac->ciu_clk))
			pr_warn("%s: %s: clk_prepare_enable sdhci_zodiac->ciu_clk failed.\n", dev_name(dev), __func__);
	}

	sdhci_zodiac_enable_clk(host);

	return sdhci_zodiac_runtime_resume_host(host);
}
#endif

static const struct dev_pm_ops sdhci_zodiac_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdhci_zodiac_suspend, sdhci_zodiac_resume)
	SET_RUNTIME_PM_OPS(sdhci_zodiac_runtime_suspend, sdhci_zodiac_runtime_resume,
						   NULL)
};

static void sdhci_zodiac_populate_dt(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	struct device_node *np = pdev->dev.of_node;
	u32 value = 0;

	if (!np) {
		dev_err(&pdev->dev, "can not find device node\n");
		return;
	}

	if (of_property_read_u32(np, "cd-vol", &value)) {
		dev_info(&pdev->dev, "%s: %s cd-vol property not found, using value of 0 as default\n", dev_name(sdhci_zodiac->dev), __func__);
		value = 0;
	}
	sdhci_zodiac->cd_vol = value;

	if (of_property_read_u32(np, "mux-sdsim", &value)) {
		dev_info(&pdev->dev, "%s: %s mux-sdsim property not found, using value of 0 as default\n", dev_name(sdhci_zodiac->dev), __func__);
		value = 0;
	}
	sdhci_zodiac->mux_sdsim = value;
	dev_info(&pdev->dev, "%s dts mux-sdsim = %d\n", dev_name(sdhci_zodiac->dev), sdhci_zodiac->mux_sdsim);

	if (of_find_property(np, "zodiac-fpga", NULL)) {
		dev_info(&pdev->dev, "%s %s zodiac-fpga found\n", dev_name(sdhci_zodiac->dev), __func__);
		host->quirks2 |= SDHCI_QUIRK2_ZODIAC_FPGA;
	}

	if (of_find_property(np, "zodiac-sbc", NULL)) {
		dev_info(&pdev->dev, "%s %s zodiac-sbc found\n", dev_name(sdhci_zodiac->dev), __func__);
		host->quirks2 |= SDHCI_QUIRK2_ZODIAC_SBC;
	}

	if (of_find_property(np, "pins_detect_enable", NULL)) {
		dev_info(&pdev->dev, "%s %s pins_detect_enable found\n", dev_name(sdhci_zodiac->dev), __func__);
		sdhci_zodiac->flags |= PINS_DETECT_ENABLE;
	}

	if (of_find_property(np, "sdio_support_uhs", NULL)) {
		dev_info(&pdev->dev, "sdhci:%d %s find sdio_support_uhs in dts\n", sdhci_zodiac->sdhci_host_id,  __func__);
		host->mmc->caps |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104);
	}

	if (of_find_property(np, "disable-wp", NULL)) {
		dev_info(&pdev->dev, "%s %s disable-wp is set in dts\n", dev_name(sdhci_zodiac->dev), __func__);
		host->mmc->caps2 |= MMC_CAP2_NO_WRITE_PROTECT;
	}

	if (of_find_property(np, "caps2-support-wifi", NULL)) {
		dev_info(&pdev->dev, "sdhci:%d %s find caps2-support-wifi in dts\n", sdhci_zodiac->sdhci_host_id,  __func__);
		host->mmc->caps3 |= MMC_CAP3_SUPPORT_WIFI;
	}

	if (of_find_property(np, "caps2-wifi-support-cmd11", NULL)
		&& of_property_count_u8_elems(np, "caps2-wifi-support-cmd11") <= 0) {
		dev_info(&pdev->dev, "sdhci:%d %s find wifi support cmd11 in dts\n",
						sdhci_zodiac->sdhci_host_id,  __func__);
		host->mmc->caps3 |= MMC_CAP3_SUPPORT_WIFI_CMD11;
	}

	if (of_find_property(np, "caps-wifi-no-lowpwr", NULL)
		&& of_property_count_u8_elems(np, "caps-wifi-no-lowpwr") <= 0) {
		dev_info(&pdev->dev, "sdhci:%d %s find wifi support no_lowpwr in dts\n",
						sdhci_zodiac->sdhci_host_id,  __func__);
		host->mmc->caps3 |= MMC_CAP3_WIFI_NO_LOWPWR;
	}

	sdhci_zodiac->mux_vcc_status = MUX_SDSIM_VCC_STATUS_2_9_5_V;
}

static void sdhci_zodiac_get_pinctrl(struct platform_device *pdev, struct sdhci_zodiac_data *sdhci_zodiac)
{
	sdhci_zodiac->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(sdhci_zodiac->pinctrl)) {
		dev_info(&pdev->dev, "%s: could not get pinctrl\n", dev_name(sdhci_zodiac->dev));
		sdhci_zodiac->pinctrl = NULL;
	}

	if (sdhci_zodiac->pinctrl) {
		sdhci_zodiac->pins_default = pinctrl_lookup_state(sdhci_zodiac->pinctrl, PINCTRL_STATE_DEFAULT);
		/* enable pins to be muxed in and configured */
		if (IS_ERR(sdhci_zodiac->pins_default)) {
			dev_info(&pdev->dev, "%s: could not get default pinstate\n",  dev_name(sdhci_zodiac->dev));
			sdhci_zodiac->pins_default = NULL;
		}
		sdhci_zodiac->pins_idle = pinctrl_lookup_state(sdhci_zodiac->pinctrl, PINCTRL_STATE_IDLE);
		if (IS_ERR(sdhci_zodiac->pins_idle)) {
			dev_info(&pdev->dev,  "%s: could not get default pinstate\n", dev_name(sdhci_zodiac->dev));
			sdhci_zodiac->pins_idle = NULL;
		}
	}
}

static int sdhci_zodiac_clock_init(struct sdhci_host *host, struct platform_device *pdev, struct sdhci_zodiac_data *sdhci_zodiac)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;

	if (host->quirks2 & SDHCI_QUIRK2_ZODIAC_FPGA)
		return 0;

	if (sdhci_zodiac->sdhci_host_id == SDHCI_SD_ID) {
		sdhci_zodiac->biu_clk = devm_clk_get(&pdev->dev, "biu");
		if (IS_ERR(sdhci_zodiac->biu_clk)) {
			dev_err(&pdev->dev, "biu clock not found.\n");
			ret = PTR_ERR(sdhci_zodiac->biu_clk);
			return ret;
		}

		ret = clk_prepare_enable(sdhci_zodiac->biu_clk);
		if (ret) {
			dev_err(&pdev->dev, "Unable to enable biu clock.\n");
			return ret;
		}
	}

	sdhci_zodiac->ciu_clk = devm_clk_get(&pdev->dev, "ciu");
	if (IS_ERR(sdhci_zodiac->ciu_clk)) {
		dev_err(&pdev->dev, "ciu clock not found.\n");
		ret = PTR_ERR(sdhci_zodiac->ciu_clk);
		return ret;
	}

	if (of_device_is_available(np)) {
		if (of_property_read_u32(np, "board-mmc-bus-clk", &host->clock)) {
			dev_err(&pdev->dev, "board-mmc-bus-clk cannot get from dts, use the default value!\n");
			host->clock = SDHCI_ZODIAC_CLK_FREQ;
		}
	}

	ret = clk_set_rate(sdhci_zodiac->ciu_clk, host->clock);
	if (ret)
		dev_err(&pdev->dev, "Error setting desired host->clock=%u, get clk=%lu\n",
			host->clock, clk_get_rate(sdhci_zodiac->ciu_clk));
	ret = clk_prepare_enable(sdhci_zodiac->ciu_clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable SD clock.\n");
		return ret;
	}
	dev_info(&pdev->dev, "%s: ciu_clk=%lu\n", __func__, clk_get_rate(sdhci_zodiac->ciu_clk));

	return 0;
}

/*
 * BUG: device rename krees old name, which would be realloced for other
 * device, pdev->name points to freed space, driver match may cause a panic
 * for wrong device
 */
static int sdhci_zodiac_rename(struct platform_device *pdev, struct sdhci_zodiac_data *sdhci_zodiac)
{
	int ret = -1;
	static const char *const hi_mci0 = "hi_mci.0";
	static const char *const hi_mci1 = "hi_mci.1";
	static const char *const hi_mci2 = "hi_mci.2";

	switch (sdhci_zodiac->sdhci_host_id) {
	case SDHCI_EMMC_ID:
		pdev->name = hi_mci0;
		ret = device_rename(&pdev->dev, hi_mci0);
		if (ret < 0) {
			dev_err(&pdev->dev, "dev set name %s fail\n", hi_mci0);
			goto out;
		}
		break;
	case SDHCI_SD_ID:
		pdev->name = hi_mci1;
		ret = device_rename(&pdev->dev, hi_mci1);
		if (ret < 0) {
			dev_err(&pdev->dev, "dev set name %s fail\n", hi_mci0);
			goto out;
		}

		break;
	case SDHCI_SDIO_ID:
		pdev->name = hi_mci2;
		ret = device_rename(&pdev->dev, hi_mci2);
		if (ret < 0) {
			dev_err(&pdev->dev, "dev set name %s fail\n", hi_mci2);
			goto out;
		}

		break;
	default:
		dev_err(&pdev->dev, "%s: sdhci host id out of range!!!\n", dev_name(&pdev->dev));
		goto out;
	}

out:
	dev_err(&pdev->dev, "%s: %s: ret = %d\n", dev_name(&pdev->dev), __func__, ret);
	return ret;
}

void sdhci_zodiac_work_routine_card(struct work_struct *work)
{
	struct sdhci_zodiac_data *sdhci_zodiac = container_of(work, struct sdhci_zodiac_data, card_work);
	struct sdhci_host *host = sdhci_zodiac->host;
	struct mmc_host *mmc = host->mmc;

	sdhci_zodiac->card_detect = sdhci_zodiac_get_cd_status(host);
	pr_err("%s: %s card %s\n", __func__, dev_name(sdhci_zodiac->dev), sdhci_zodiac->card_detect ? "inserted" : "removed");

	pm_runtime_get_sync(mmc_dev(mmc));
	if (mmc->ops->card_event)
		mmc->ops->card_event(host->mmc);

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	mmc_detect_change(mmc, msecs_to_jiffies(200));
}

static irqreturn_t sdhci_zodiac_card_detect(int irq, void *data)
{
	struct sdhci_zodiac_data *sdhci_zodiac = (struct sdhci_zodiac_data *)data;

	queue_work(sdhci_zodiac->card_workqueue, &sdhci_zodiac->card_work);
	return IRQ_HANDLED;
}

static void sdhci_zodiac_set_external_host(struct sdhci_zodiac_data *sdhci_zodiac)
{
	struct sdhci_host *host = sdhci_zodiac->host;

	if (sdhci_zodiac->sdhci_host_id == SDHCI_SD_ID) {
		sdhci_host_from_sd_module = host;
		dev_err(sdhci_zodiac->dev, "%s: %s sdhci_host_from_sd_module is inited\n",
									dev_name(sdhci_zodiac->dev), __func__);
		sema_init(&sem_mux_sdsim_detect, 1);
	} else if (sdhci_zodiac->sdhci_host_id == SDHCI_SDIO_ID) {
		sdhci_sdio_host = host;
		dev_err(sdhci_zodiac->dev, "%s: %s sdio host for exteral is inited\n",
									dev_name(sdhci_zodiac->dev), __func__);
	}
}

static int sdhci_zodiac_cd_detect_init(struct platform_device *pdev, struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
	struct device_node *np =  pdev->dev.of_node;
	int err;
	int gpio;

	if (sdhci_zodiac->sdhci_host_id != SDHCI_SD_ID)
		return 0;

	gpio = of_get_named_gpio(np, "cd-gpio", 0);
	dev_err(&pdev->dev, "%s: %s gpio = %d\n", dev_name(sdhci_zodiac->dev), __func__, gpio);

	if (gpio_is_valid(gpio)) {
		if (devm_gpio_request_one(&pdev->dev, gpio, GPIOF_IN, "sdhci-zodiac-cd")) {
			dev_err(&pdev->dev, "%s: %s gpio [%d] request failed\n",
						dev_name(sdhci_zodiac->dev), __func__, gpio);
		} else {
			sdhci_zodiac->gpio_cd = gpio;
			err = devm_request_irq(&pdev->dev, gpio_to_irq(gpio), sdhci_zodiac_card_detect,
					IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
					IRQF_NO_SUSPEND | IRQF_SHARED, DRIVER_NAME, sdhci_zodiac);
			if (err)
				pr_err("%s: %s request gpio irq error\n", dev_name(sdhci_zodiac->dev), __func__);
		}
	} else {
		dev_info(&pdev->dev, "%s: %s cd gpio not available",
					dev_name(sdhci_zodiac->dev), __func__);
	}
	return 0;
}

#ifdef CONFIG_MMC_CQ_HCI
void sdhci_zodiac_cmdq_init(struct platform_device *pdev)
{
	int ret;
	struct sdhci_host *host = platform_get_drvdata(pdev);

	if (host->mmc->caps2 & MMC_CAP2_CMD_QUEUE) {
		host->cq_host = cmdq_pltfm_init(pdev, host->ioaddr + SDHCI_CMDQ_REG_BASE);
		if (IS_ERR(host->cq_host)) {
			ret = PTR_ERR(host->cq_host);
			dev_err(&pdev->dev, "cmd queue platform init failed %u\n", ret);
			host->mmc->caps2 &= ~MMC_CAP2_CMD_QUEUE;
			return;
		}

		if (!(host->quirks2 & SDHCI_QUIRK2_BROKEN_64_BIT_DMA))
			host->cq_host->caps |= CMDQ_TASK_DESC_SZ_128;
	}
}
#endif

static int sdhci_zodiac_probe(struct platform_device *pdev)
{
	int ret;
	struct sdhci_host *host = NULL;
	struct sdhci_pltfm_host *pltfm_host = NULL;
	struct sdhci_zodiac_data *sdhci_zodiac = NULL;

#if defined(CONFIG_DFX_DEBUG_FS) && (!defined(CONFIG_PRODUCT_ARMPC))
	proc_sd_test_init();
#endif
	sdhci_zodiac = devm_kzalloc(&pdev->dev, sizeof(*sdhci_zodiac), GFP_KERNEL);
	if (!sdhci_zodiac)
		return -ENOMEM;

	sdhci_zodiac->dev = &pdev->dev;
	sdhci_zodiac->card_detect = CARD_UNDETECT;
	sdhci_zodiac->sdhci_host_id = of_alias_get_id(sdhci_zodiac->dev->of_node, "mshc");
	pr_err("%s: zodiac sdhci host id : %d\n", dev_name(sdhci_zodiac->dev), sdhci_zodiac->sdhci_host_id);

	ret = sdhci_zodiac_rename(pdev, sdhci_zodiac);
	if (ret)
		goto err_mshc_free;

	host = sdhci_pltfm_init(pdev, &sdhci_zodiac_pdata, sizeof(*sdhci_zodiac));
	if (IS_ERR(host)) {
		ret = PTR_ERR(host);
		goto err_mshc_free;
	}

	pltfm_host = sdhci_priv(host);
	pltfm_host->priv = sdhci_zodiac;
	sdhci_zodiac->host = host;
	host->mmc->sdio_present = 0;

	ret = sdhci_zodiac_get_resource(pdev, host);
	if (ret)
		goto err_pltfm_free;

	sdhci_get_of_property(pdev);
	sdhci_zodiac_populate_dt(pdev);
	sdhci_zodiac_get_pinctrl(pdev, sdhci_zodiac);
	sdhci_zodiac_cd_detect_init(pdev, host);
	sdhci_zodiac_set_external_host(sdhci_zodiac);

	ret = sdhci_zodiac_clock_init(host, pdev, sdhci_zodiac);
	if (ret)
		goto err_pltfm_free;

	pltfm_host->clk = sdhci_zodiac->ciu_clk;

	sdhci_zodiac_hardware_reset(host);
	sdhci_zodiac_hardware_disreset(host);

#ifdef CONFIG_MMC_CQ_HCI
	sdhci_zodiac_cmdq_init(pdev);
#endif

	if (host->quirks2 & SDHCI_QUIRK2_ZODIAC_SBC)
		host->quirks2 |= SDHCI_QUIRK2_ACMD23_BROKEN;
	else
		host->quirks2 |= SDHCI_QUIRK2_HOST_NO_CMD23;

	host->quirks |= SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC;
	host->mmc->caps2 |= MMC_CAP2_NO_PRESCAN_POWERUP;
	if (host->mmc->pm_caps & MMC_PM_KEEP_POWER) {
		host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
		host->quirks2 |= SDHCI_QUIRK2_HOST_OFF_CARD_ON;
	}

	if (!(host->quirks2 & SDHCI_QUIRK2_BROKEN_64_BIT_DMA))
		host->dma_mask = DMA_BIT_MASK(64);
	else
		host->dma_mask = DMA_BIT_MASK(32);

	mmc_dev(host->mmc)->dma_mask = &host->dma_mask;

	sdhci_zodiac->card_workqueue = alloc_workqueue("sdhci-zodiac-card", WQ_MEM_RECLAIM, 1);
	if (!sdhci_zodiac->card_workqueue)
		goto clk_disable_all;

	INIT_WORK(&sdhci_zodiac->card_work, sdhci_zodiac_work_routine_card);

	ret = sdhci_add_host(host);
	if (ret)
		goto err_workqueue;

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, 1);

	pr_err("%s: %s: done! ret = %d\n", dev_name(sdhci_zodiac->dev), __func__, ret);

	return 0;

err_workqueue:
	destroy_workqueue(sdhci_zodiac->card_workqueue);
clk_disable_all:
	clk_disable_unprepare(sdhci_zodiac->ciu_clk);
err_pltfm_free:
	sdhci_pltfm_free(pdev);
err_mshc_free:
	devm_kfree(&pdev->dev, sdhci_zodiac);

	pr_err("%s: error = %d!\n", __func__, ret);

	return ret;
}

static int sdhci_zodiac_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;

	dev_info(&pdev->dev, "%s:\n", __func__);

	pm_runtime_get_sync(&pdev->dev);
	sdhci_remove_host(host, 1);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	destroy_workqueue(sdhci_zodiac->card_workqueue);
	clk_disable_unprepare(sdhci_zodiac->ciu_clk);
	clk_disable_unprepare(sdhci_zodiac->biu_clk);

	sdhci_pltfm_free(pdev);

	return 0;
}

static const struct of_device_id sdhci_zodiac_of_match[] = {
	{.compatible = "hisilicon,scorpio-sdhci"},
	{.compatible = "hisilicon,andromeda-sdhci"},
	{.compatible = "hisilicon,apus-sdhci"},
	{}
};

MODULE_DEVICE_TABLE(of, sdhci_zodiac_of_match);

static struct platform_driver sdhci_zodiac_driver = {
	.driver = {
			   .name = "sdhci_zodiac",
			   .of_match_table = sdhci_zodiac_of_match,
			   .pm = &sdhci_zodiac_dev_pm_ops,
			   },
	.probe = sdhci_zodiac_probe,
	.remove = sdhci_zodiac_remove,
};

module_platform_driver(sdhci_zodiac_driver);

MODULE_DESCRIPTION("Driver for the zodiac SDHCI Controller");
MODULE_AUTHOR("zodiac");
MODULE_LICENSE("GPL");
