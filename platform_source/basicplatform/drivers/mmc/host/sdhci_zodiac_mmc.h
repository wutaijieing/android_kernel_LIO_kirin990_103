/*
 * zodiac emmc sdhci controller interface.
 * Copyright (c) Zodiac Technologies Co., Ltd. 2017-2019. All rights reserved.
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

#ifndef __SDHCI_ZODIAC_SCORPIO_H
#define __SDHCI_ZODIAC_SCORPIO_H

#ifdef CONFIG_MMC_SDHCI_ZODIAC_SCORPIO
/* hsdt1 sd reset */
#define PERRSTEN0	0x20
#define PERRSTDIS0	0x24
#define IP_RST_SD	(0x1 << 1)
#define IP_HRST_SD	(0x1 << 0)

#define PERRSTEN1 0x2C
#define PERRSTDIS1 0x30
#define PERRSTSTAT1 0x34

#define IP_HRST_SDIO (0x1 << 2)
#define IP_RST_SDIO (0x1 << 3)

#define CLKDIV1	0xAC
#define SDCLK_3_2 0x0
#define PPLL2	0x1
#define SDPLL	0x2
#define MASK_PLL	(0x3 << 5)
#define MASK_DIV_SDPLL	(0xF << 1)

#define PEREN0	0x0
#define PERDIS0	0x4
#define PERSTAT0	0xC
#define GT_HCLK_SD	0x1

#define PEREN1	0x10
#define PERDIS1	0x14
#define PERSTAT1	0x1C
#endif

#ifdef CONFIG_MMC_SDHCI_ZODIAC_ANDROMEDA
#define PERIPRST1 0x60
#define PERIPDIS1 0x64
#define PERIPSTAT1 0x68
#define IP_RST_EMMC_BIT  (0x1 << 0)
#define IP_PRST_EMMC_BIT (0x1 << 14)
#endif

#ifdef CONFIG_MMC_SDHCI_ZODIAC_APUS
#define PERRSTEN0 0x200
#define PERRSTEN1 0x20C
#define PERRSTEN2 0x218
#define PERRSTEN5 0x09C

#define PERRSTDIS0 0x204
#define PERRSTDIS1 0x210
#define PERRSTDIS2 0x21C
#define PERRSTDIS5 0x0A0

#define PERRSTSTAT0 0x208
#define PERRSTSTAT1 0x214
#define PERRSTSTAT2 0x220
#define PERRSTSTAT5 0x0A4

#define IP_RST_EMMC  (0x1 << 1)
#define IP_RST_EMMC_BUS_BIT (0x1 << 25)
#define IP_RST_SDIO  (0x1 << 3)
#define IP_ARST_SDIO (0x1 << 2)
#endif

#define SDEMMC_CRG_CTRL	0x80
#define TUNING_SAMPLE_MASK	(0x1F << 8)
#define TUNING_SAMPLE_MASKBIT	0x8

#define CLK_CTRL_TIMEOUT_SHIFT 16
#define CLK_CTRL_TIMEOUT_MASK (0xf << CLK_CTRL_TIMEOUT_SHIFT)
#define CLK_CTRL_TIMEOUT_MIN_EXP 13

#define SDHCI_ZODIAC_MIN_FREQ 100000
#define SDHCI_ZODIAC_CLK_FREQ 960000000
#define MUX_SDSIM_VCC_STATUS_2_9_5_V 0
#define MUX_SDSIM_VCC_STATUS_1_8_0_V 1
#define LEVER_SHIFT_GPIO  90
#define LEVER_SHIFT_3_0V 0
#define LEVER_SHIFT_1_8V 1


#define TUNING_PHASE_INVALID (-1)
#define TUNING_LOOP_COUNT 10
#define MAX_TUNING_LOOP 40

#define BLKSZ_LARGE 128
#define BLKSZ_SMALL 64
#define SDHCI_MAX_COUNT 0xFFF
#define SDHCI_DMD_ERR_MASK 0xffffffff
#define SDHCI_DSM_ERR_INT_STATUS 16
#define SDHCI_ERR_CMD_INDEX_MASK 0x3f
#define SDHCI_SAM_PHASE_MID 2
#define SDHCI_TUNING_MOVE_STEP 2
#define SDHCI_TUNING_RECORD 0x1
#define SDHCI_TUNING_MAX 63
#define CMDQ_SEND_STATUS_CQSSC1 0x10100
#define SDHCI_ADMA_SUPPORT_BIT 64
#define SDHCI_SDMA_SUPPORT_BIT 32
#define MMC_BLOCK_SIZE 512
#define SDHCI_PINCTL_CLEAR_BIT (0xF << 4)
#define SDHCI_WRITE_PINCTL_FLAG (0x7 << 4)
#define MMC_I2C_ADAPTER_NUM 2
#define SDHCI_I2C_MASTER_LENTH 8
#define SDHCI_ZODIAC_CLK_STABLE_TIME 20

/* SDHCI_CRG_CTRL0 used for zodiac */
#define SDHCI_CRG_CTRL0 0x80
#define SDHCI_ZODIAC_CLK_DIV_OFFSET 24
#define SDHCI_ZODIAC_CLK_DLY_DRV_OFFSET 16
#define SDHCI_ZODIAC_CLK_DLY_SAMPLE_OFFSET 8
#define SDHCI_ZODIAC_CLK_DIV_MASK (0xf << 24)
#define SDHCI_ZODIAC_CLK_DLY_DRV_MASK (0x1f << 16)
#define SDHCI_ZODIAC_CLK_DLY_SAMPLE_MASK (0x1f << 8)
#define SDHCI_ZODIAC_CLK_400K 400000
#define SDHCI_SDEMMC_CLK_DIV_10M (0x1 << 24)
#define SDHCI_SDEMMC_CLK_DLY_SAMPLE_180 (0x7 << 8)
#define SDHCI_ZODIAC_TIMING_MODE 11

#define SDEMMC_TIMING_CTRL_1 0x94
#define SDEMMC_NSB_CFG_MASK 0xff0000
#define SDEMMC_NSB_CFG_LONG 0x8C0000
#define SDEMMC_NSB_CFG_SHORT 0x100000

#define ACTRL_BITMSK_NONSEC_CTRL1 0x400
#define SDCARD_SEL18_BITMASK 27
#define SDCARD_SEL18_OFFSET 11

#define ZODIAC_DFX_REG1 0x800
#define ZODIAC_DFX_REG2 0x900
#define SDHCI_CMDQ_REG_BASE 0xE00
#if defined(CONFIG_DFX_DEBUG_FS)
extern void proc_sd_test_init(void);
extern void test_sd_delete_host_caps(struct mmc_host *host);
#endif

#define NONSEC_STATUS20 0x140
#define NONSEC_STATUS22 0x148

#define DFX_BUSY 0x700
#define DFX_OST 0x704
#define TB_LITE_DDR2DMC_RD 0xFA360000
#define TB_LITE_DDR2DMC_WR 0xFA361000
#define IB_MMC 0xFA390000
#define IB_LITE_DDR2DMC_RD 0xFF903000
#define IB_LITE_DDR2DMC_WR 0xFF904000

struct sdhci_crg_ctrl {
	unsigned long clk_src;
	unsigned long clk_dly_sample;
	unsigned long clk_dly_drv;
	unsigned long clk_div;
	unsigned int  max_clk;
};

/**
 * struct sdhci_zodiac_data
 * @clk:						Pointer to the sd clock
 * @clock:					record current clock rate
 * @tuning_current_sample:		record current sample when soft tuning
 * @tuning_init_sample:			record the optimum sample of soft tuning
 * @tuning_sample_count:		record the tuning count for soft tuning
 * @tuning_move_sample:		record the move sample when data or cmd error occor
 * @tuning_move_count:		record the move count
 * @tuning_sample_flag:			record the sample OK or NOT of soft tuning
 * @tuning_strobe_init_sample:	default strobe sample
 * @tuning_strobe_move_sample:	record the strobe move sample when data or cmd error occor
 * @tuning_strobe_move_count:	record the strobe move count
 */
struct sdhci_zodiac_data {
	struct sdhci_host *host;
	struct clk *ciu_clk;
	struct clk *biu_clk;
	unsigned int clock;
	struct sdhci_crg_ctrl *pcrgctrl;
	/* pinctrl handles */
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_idle;
	struct workqueue_struct	*card_workqueue;
	struct work_struct	card_work;
	struct device	*dev;

	unsigned int tuning_loop_max;
	unsigned int tuning_count;
	unsigned int tuning_phase_best;
	unsigned int tuning_phase_record;

	unsigned int tuning_move_phase;
	unsigned int tuning_move_count;
	unsigned int tuning_phase_max;
	unsigned int tuning_phase_min;

	int old_power_mode;
	unsigned int mux_sdsim;
	int mux_vcc_status;
	unsigned int	lever_shift;

	int sdhci_host_id;
#define SDHCI_EMMC_ID	0
#define SDHCI_SD_ID		1
#define SDHCI_SDIO_ID	2
	unsigned int	flags;
#define PINS_DETECT_ENABLE	(1 << 0)	/* NM card 4-pin detect control */
	int card_detect;
#define CARD_REMOVED 0
#define CARD_ON 1
#define CARD_UNDETECT 2
	unsigned int cd_vol;
	int gpio_cd;
};

extern int check_mntn_switch(int feature);
extern void sdhci_dumpregs(struct sdhci_host *host);
extern int config_sdsim_gpio_mode(enum sdsim_gpio_mode gpio_mode);
extern void sdhci_set_default_irqs(struct sdhci_host *host);
extern int sdhci_send_command_direct(struct mmc_host *mmc, struct mmc_request *mrq);
extern void sdhci_retry_req(struct sdhci_host *host, struct mmc_request *mrq);
extern void sdhci_set_vmmc_power(struct sdhci_host *host, unsigned short vdd);


void sdhci_zodiac_hardware_reset(struct sdhci_host *host);
void sdhci_zodiac_hardware_disreset(struct sdhci_host *host);
int sdhci_zodiac_get_resource(struct platform_device *pdev, struct sdhci_host *host);

#define sdhci_mmc_sys_writel(host, val, reg)                                   \
	writel((val), (host)->mmc_sys + (reg))
#define sdhci_mmc_sys_readl(host, reg) readl((host)->mmc_sys + (reg))

#define sdhci_hsdtcrg_writel(host, val, reg)					\
	writel((val), (host)->hsdtcrg + (reg))
#define sdhci_hsdtcrg_readl(host, reg) readl((host)->hsdtcrg + (reg))

#define sdhci_hsdt1crg_writel(host, val, reg)                                   \
	writel((val), (host)->hsdt1crg + (reg))
#define sdhci_hsdt1crg_readl(host, reg) readl((host)->hsdt1crg + (reg))

#define sdhci_litecrg_writel(host, val, reg)                                   \
	writel((val), (host)->litecrg + (reg))
#define sdhci_litecrg_readl(host, reg) readl((host)->litecrg + (reg))

#define sdhci_actrl_writel(host, val, reg)                                   \
	writel((val), (host)->actrl + (reg))
#define sdhci_actrl_readl(host, reg) readl((host)->actrl + (reg))

#define sdhci_crgctrl_writel(host, val, reg)                                   \
	writel((val), (host)->crgctrl + (reg))
#define sdhci_crgctrl_readl(host, reg) readl((host)->crgctrl + (reg))
#endif
