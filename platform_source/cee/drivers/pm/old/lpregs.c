/*
 * lpreg.c
 *
 * debug information of suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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

#include "lpregs.h"
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpu_pm.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/clockchips.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/irqflags.h>
#include <linux/arm-cci.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/slab.h>
#include <linux/console.h>
#include <linux/pm_wakeup.h>
#include <soc_crgperiph_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#ifdef SOC_CRGPERIPH_PEREN2_gt_pclk_uart0_START
#include <soc_hi_uart_v500_interface.h>
#else
#include <soc_uart_interface.h>
#endif
#include <soc_sctrl_interface.h>
#include <linux/debugfs.h>
#include <linux/seq_buf.h>
#include <linux/uaccess.h>
#ifdef CONFIG_PMIC_SPMI
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#endif
#include <securec.h>
#include <linux/platform_drivers/lowpm_gpio_auto_gen.h>

#ifdef CONFIG_POWER_DUBAI
#include <huawei_platform/power/dubai/dubai_wakeup_stats.h>
#endif

#include <pm_def.h>
#include <m3_interrupts.h>
#ifdef CONFIG_LPMCU_BB
#include <m3_rdr_ddr_map.h>
#include <lpmcu_runtime.h>
#endif

#define WAKE_IRQ_NUM_MAX		32

#define IPC_PROCESSOR_MAX		6
#define NSIPC_MAILBOX_MAX		27
#define AP_IPC_PROC_MAX		2
#define PROCESSOR_IOMCU_LENGTH		5
#define IPC_MBXDATA_MIN		0
#define IPC_MBXDATA_MAX		8
#ifdef CONFIG_POWER_DUBAI
#define IPC_MBXDATA_TAG		2
#define get_sharemem_data(m)		((m) & 0xFFFF)
#define get_sharemem_source(m)		((m) & 0xFF)
#endif
#define ipc_mbx_source_offset(m)		((m) << 6)
#define ipc_mbx_dstatus_offset(m)		(0x0C + ((m) << 6))
#define ipc_mbxdata_offset(m, idex)		(0x20 + 4 * (idex) + ((m) << 6))

#define NO_SEQFILE		0
#define lowpm_msg(seq_file, fmt, args ...) \
	do { \
		if (seq_file == NO_SEQFILE) \
			pr_info(fmt"\n", ##args); \
		else \
			seq_printf(seq_file, fmt"\n", ##args); \
	} while (0)
#define lowpm_msg_cont(seq_file, fmt, args ...) \
	do { \
		if (seq_file == NO_SEQFILE) \
			pr_cont(fmt, ##args); \
		else \
			seq_printf(seq_file, fmt, ##args); \
	} while (0)

#ifdef SOC_CRGPERIPH_PERRSTDIS2_ip_rst_uart0_START
#define CRG_PERRSTDIS2_ip_prst_uart0_BIT	SOC_CRGPERIPH_PERRSTDIS2_ip_rst_uart0_START
#else
#define CRG_PERRSTDIS2_ip_prst_uart0_BIT	SOC_CRGPERIPH_PERRSTDIS2_ip_prst_uart0_START
#endif

#define MODULE_NAME		"hisilicon,lowpm_func"

#define UART_LPM3_IDX		6
#define UART_CRGPERI_IDX		0
#define ADDR_SIZE		0x4
#define UART_BAUD_DIVINT_VAL		0xa
#define UART_BAUD_DIVFRAC_VAL		0x1b
#define UART_RXIFLSEL_VAL		0x92
#define UART_WLEN_VAL		0x3
#define TICKMARK_STEP		4
#define TICKMARK_US		31 /* 1000000 / 32768 */

/* struct g_sysreg_bases to hold the base address of some system registers. */
struct sysreg_base_addr {
	void __iomem *uart_base;
	void __iomem *pctrl_base;
	void __iomem *sysctrl_base;
	void __iomem **gpio_base;
	void __iomem *crg_base;
	void __iomem *pmic_base;
	void __iomem *reserved_base;
	void __iomem *nsipc_base;
#ifdef AP_AO_NSIPC_IRQ
	void __iomem *ao_nsipc_base;
#endif
};

static struct sysreg_base_addr g_sysreg_base;
static unsigned int g_lpmcu_irq_num;
static const char **g_lpmcu_irq_name = NULL;

static unsigned int g_lp_fpga_flag;

static int g_suspended;
static struct wakeup_source *g_lowpm_wake_lock = NULL;

static char *g_processor_name[IPC_PROCESSOR_MAX] = {
	"gic1",
	"gic2",
	"iomcu",
	"lpm3",
	"hifi",
	"modem"
};

#ifdef CONFIG_SR_DEBUG_LPREG

#define DEBG_SUSPEND_PRINTK		BIT(0)
#define DEBG_SUSPEND_IO_SHOW		BIT(1)
#define DEBG_SUSPEND_PMU_SHOW		BIT(2)
#define DEBG_SUSPEND_WAKELOCK		BIT(8)
#define DEBG_SUSPEND_CLK_SHOW		BIT(10)

#define DEBUG_LOG_BUF_SIZE		(1024 * 1024)
#define BUFFER_LENGTH		40
#define pmussi_reg(x)		(g_sysreg_base.pmic_base + ((x) << 2))

#define lowpm_kfree_array(arr, size, iter) \
	do { \
		for (iter = 0; iter < size; iter++) { \
			if (arr[iter] != NULL) { \
				kfree(arr[iter]); \
				arr[iter] = NULL; \
			} \
		} \
		kfree(arr); \
		arr = NULL; \
	} while (0)

#define gpio_dir(x)		((x) + 0x400)
#define gpio_data(x, y)		((x) + (1 << (2 + (y))))
#define gpio_bit(x, y)		((x) << (y))
#define gpio_is_set(x, y)		(((x) >> (y)) & 0x1)
#define GPIO_GROUP_ID_OFFSET		3
#define IOCG_MASK		0x3
#define RANGE_START		1
#define RANGE_STEP		2

struct lp_clk {
	unsigned int reg_base;
	unsigned int reg_off;
	unsigned int bit_idx;
	const char *clk_name;
	unsigned int status;
};

enum pm_clk_status {
	NULL_CLOSE = 0,
	AP_CLOSE,
	LPM3_CLOSE,
	AP_ON,
	AO_ON,
	ON_OFF,
	MODEM_ON,
};

static unsigned int g_io_group_num;
static unsigned int g_lp_pmu_num;
static unsigned int g_pmu_addr_end;
static unsigned int g_lp_clk_num;
static const char *g_plat_name = NULL;
#ifdef CONFIG_PM_SEC_IO
static void __iomem **g_mg_reg = NULL;
static void __iomem **g_cg_reg = NULL;
static int g_mg_sec_reg_num;
static int g_cg_sec_reg_num;
#endif

#ifdef CONFIG_PM_SEC_GPIO
static unsigned int *g_sec_gpio_array = NULL;
static int g_sec_gpio_num;
#endif

/*
 * note: g_pmu_idx[k] must be matched with g_pmuregs_lookups[k],
 * otherwise the array may be out of range.
 */
static struct pmu_idx **g_pmu_idx = NULL;
static struct pmuregs **g_pmuregs_lookups = NULL;
static struct lp_clk **g_clk_lookups = NULL;
static unsigned int g_usavedcfg;

static int g_chipid[CHIPID_DIG_NUMS];

#endif

static int get_ipc_addr(void)
{
	int ret;
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "lowpm,ipc");
	if (np == NULL) {
		pr_err("%s: dts[%s] node not found\n", __func__, "lowpm,ipc");
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.nsipc_base = of_iomap(np, 0);
	pr_info("%s: nsipc_base:%pK\n", __func__, g_sysreg_base.nsipc_base);
	if (g_sysreg_base.nsipc_base == NULL) {
		pr_err("%s: nsipc_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);

#ifdef AP_AO_NSIPC_IRQ
	np = of_find_compatible_node(NULL, NULL, "lowpm,ao_ipc");
	if (np == NULL) {
		pr_err("%s: dts[%s] node not found\n",
		       __func__, "lowpm,ao_ipc");
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.ao_nsipc_base = of_iomap(np, 0);
	pr_info("%s: ao_nsipc_base:%pK\n",
		__func__, g_sysreg_base.ao_nsipc_base);
	if (g_sysreg_base.ao_nsipc_base == NULL) {
		pr_err("%s: ao_nsipc_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);
#endif

	return 0;

err_put_node:
	of_node_put(np);
err:
	pr_err("%s[%d]: fail!\n", __func__, __LINE__);
	return ret;
}

static struct device_node *find_uart_device_node(void)
{
	unsigned int uart_idx;
	struct device_node *np = NULL;

	uart_idx = get_console_index();
	switch (uart_idx) {
	case UART_CRGPERI_IDX:
		np = of_find_compatible_node(NULL, NULL, "arm,pl011");
		if (np != NULL)
			return np;
		np = of_find_compatible_node(NULL, NULL, "hisilicon,uart");
		if (np == NULL)
			pr_err("%s: dts[%s] node not found\n",
				__func__, "arm,pl011");
		break;
	case UART_LPM3_IDX:
		np = of_find_compatible_node(NULL, NULL,
					     "hisilicon,lowpm_func");
		if (np == NULL)
			pr_err("%s: dts[%s] node not found\n",
				__func__, "hisilicon,lowpm_func");
		break;
	default:
		pr_err("%s:uart default:%d\n", __func__, uart_idx);
	}
	return np;
}

static int map_sysregs(void)
{
	int ret;
	struct device_node *np = NULL;
	np = find_uart_device_node();
	if (np == NULL) {
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.uart_base = of_iomap(np, 0);
	pr_debug("%s: uart_base:%pK\n", __func__, g_sysreg_base.uart_base);
	if (g_sysreg_base.uart_base == NULL) {
		pr_err("%s: uart_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (np == NULL) {
		pr_err("%s: dts[%s] node not found\n",
			__func__, "hisilicon,sysctrl");
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.sysctrl_base = of_iomap(np, 0);
	pr_debug("%s: sysctrl_base:%pK\n",
		 __func__, g_sysreg_base.sysctrl_base);
	if (g_sysreg_base.sysctrl_base == NULL) {
		pr_err("%s: sysctrl_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisilicon,pctrl");
	if (np == NULL) {
		pr_err("%s: dts[%s] node not found\n",
		       __func__, "hisilicon,pctrl");
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.pctrl_base = of_iomap(np, 0);
	pr_debug("%s: pctrl_base:%pK\n", __func__, g_sysreg_base.pctrl_base);
	if (g_sysreg_base.pctrl_base == NULL) {
		pr_err("%s: pctrl_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (np == NULL) {
		pr_err("%s: dts[%s] node not found\n",
		       __func__, "hisilicon,crgctrl");
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.crg_base = of_iomap(np, 0);
	pr_debug("%s: crg_base:%pK\n", __func__, g_sysreg_base.crg_base);
	if (g_sysreg_base.crg_base == NULL) {
		pr_err("%s: crg_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisilicon,pmu");
	if (np == NULL) {
		pr_err("%s: dts[%s] node not found\n",
		       __func__, "hisilicon,pmu");
		ret = -ENODEV;
		goto err;
	}
	g_sysreg_base.pmic_base = of_iomap(np, 0);
	pr_debug("%s: pmic_base:%pK\n", __func__, g_sysreg_base.pmic_base);
	if (g_sysreg_base.pmic_base == NULL) {
		pr_err("%s: pmic_base of iomap fail!\n", __func__);
		ret = -ENOMEM;
		goto err_put_node;
	}
	of_node_put(np);

	ret = get_ipc_addr();
	if (ret != 0) {
		pr_err("%s: %d get_ipc_addr failed.\n", __func__, __LINE__);
		goto err;
	}
	return 0;

err_put_node:
	of_node_put(np);
err:
	pr_err("%s[%d]: fail!\n", __func__, __LINE__);
	return ret;
}

#define UART_FIFO_TX_LEVEL      3 /* FIFO≤3/4 full */
#define UART_FIFO_RX_LEVEL      0 /* FIFO≥1/8 full */
#define UART_FRAME_CFG_VAL  0x3
#define HM_EN(n)	(0x10001U << (n))
#define HM_DIS(n)	(0x10000U << (n))
void debuguart_reinit(void)
{
	unsigned int uart_idx, uregv;

	if (console_suspend_enabled == 1 ||
	    g_sysreg_base.uart_base == NULL ||
	    g_sysreg_base.crg_base == NULL)
		return;

	uart_idx = get_console_index();
	if (uart_idx == UART_LPM3_IDX) {
		/* enable clk */
		uregv = (unsigned int)readl(SOC_SCTRL_SCLPMCUCLKEN_ADDR(g_sysreg_base.sysctrl_base)) |
					    BIT(SOC_SCTRL_SCLPMCUCLKEN_uart_clk_clkoff_sys_n_START);
		writel(uregv, SOC_SCTRL_SCLPMCUCLKEN_ADDR(g_sysreg_base.sysctrl_base));

		/* reset undo */
		writel(BIT(SOC_SCTRL_SCLPMCURSTDIS_uart_soft_rst_req_START),
		       SOC_SCTRL_SCLPMCURSTDIS_ADDR(g_sysreg_base.sysctrl_base));
	} else if (uart_idx == UART_CRGPERI_IDX) {
#ifndef SOC_CRGPERIPH_PEREN2_gt_pclk_uart0_START
		/* enable clk */
		writel(BIT(SOC_CRGPERIPH_PEREN2_gt_clk_uart0_START),
		       SOC_CRGPERIPH_PEREN2_ADDR(g_sysreg_base.crg_base));
		/* reset undo */
		writel(BIT(CRG_PERRSTDIS2_ip_prst_uart0_BIT),
		       SOC_CRGPERIPH_PERRSTDIS2_ADDR(g_sysreg_base.crg_base));
#else
		writel(BIT(SOC_CRGPERIPH_PERCLKEN2_gt_pclk_uart0_START),
			SOC_CRGPERIPH_PERCLKEN2_ADDR(g_sysreg_base.crg_base));
		writel(BIT(SOC_CRGPERIPH_PERRSTDIS2_ip_prst_uart0_START),
		       SOC_CRGPERIPH_PERRSTDIS2_ADDR(g_sysreg_base.crg_base));
#endif
	} else {
		return;
	}

#ifndef SOC_CRGPERIPH_PEREN2_gt_pclk_uart0_START
	/* disable uart */
	writel(0x0, SOC_UART_UARTCR_ADDR(g_sysreg_base.uart_base));

	/* set baudrate: 19.2M 115200 */
	writel(UART_BAUD_DIVINT_VAL, SOC_UART_UARTIBRD_ADDR(g_sysreg_base.uart_base));
	writel(UART_BAUD_DIVFRAC_VAL, SOC_UART_UARTFBRD_ADDR(g_sysreg_base.uart_base));

	/* disable FIFO */
	writel(0x0, SOC_UART_UARTLCR_H_ADDR(g_sysreg_base.uart_base));

	/* set FIFO depth: 1/2 */
	writel(UART_RXIFLSEL_VAL, SOC_UART_UARTIFLS_ADDR(g_sysreg_base.uart_base));

	/* set int mask */
	uregv = BIT(SOC_UART_UARTIMSC_rtim_START) | BIT(SOC_UART_UARTIMSC_rxim_START);
	writel(uregv, SOC_UART_UARTIMSC_ADDR(g_sysreg_base.uart_base));

	/* enable FIFO */
	writel(BIT(SOC_UART_UARTLCR_H_fen_START) |
	       (UART_WLEN_VAL << SOC_UART_UARTLCR_H_wlen_START),
	       SOC_UART_UARTLCR_H_ADDR(g_sysreg_base.uart_base));

	/* enable uart trans */
	uregv = BIT(SOC_UART_UARTCR_uarten_START) |
		BIT(SOC_UART_UARTCR_rxe_START) |
		BIT(SOC_UART_UARTCR_txe_START);
	writel(uregv, SOC_UART_UARTCR_ADDR(g_sysreg_base.uart_base));
#else
	/* disable uart */
	writel(HM_DIS(SOC_HI_UART_V500_UART_CONFIG_uart_en_START),
	       SOC_HI_UART_V500_UART_CONFIG_ADDR(g_sysreg_base.uart_base));

	/* clear all interrupts, enable all interupt */
	writel(0xFF, SOC_HI_UART_V500_UART_INT_CLR_ADDR(g_sysreg_base.uart_base));
	writel(BIT(SOC_HI_UART_V500_UART_INT_MSK_rx_int_msk_START) |
	       BIT(SOC_HI_UART_V500_UART_INT_MSK_rx_timeout_int_msk_START) |
	       BIT(SOC_HI_UART_V500_UART_INT_MSK_frame_err_int_msk_START) |
	       BIT(SOC_HI_UART_V500_UART_INT_MSK_break_err_int_msk_START) |
	       BIT(SOC_HI_UART_V500_UART_INT_MSK_overrun_err_int_msk_START),
	       SOC_HI_UART_V500_UART_INT_MSK_ADDR(g_sysreg_base.uart_base));

	/* set baudrate: 19.2M 115200 */
	writel(UART_BAUD_DIVINT_VAL |
	       (UART_BAUD_DIVFRAC_VAL << SOC_HI_UART_V500_UART_BAUD_CFG_baud_div_frac_START),
	       SOC_HI_UART_V500_UART_BAUD_CFG_ADDR(g_sysreg_base.uart_base));

	/* cfg fifo and frame */
	writel(UART_FIFO_TX_LEVEL |
	       (UART_FIFO_RX_LEVEL << SOC_HI_UART_V500_UART_FIFO_CFG_rx_fifo_level_sel_START),
	       SOC_HI_UART_V500_UART_FIFO_CFG_ADDR(g_sysreg_base.uart_base));

	/* data_bit is 8bit, parity_bit is none, stop_bit is 1bit-stop */
	writel(UART_FRAME_CFG_VAL,
	       SOC_HI_UART_V500_UART_FRAME_CFG_ADDR(g_sysreg_base.uart_base));

	/* enable uart trans */
	writel(HM_EN(SOC_HI_UART_V500_UART_CONFIG_uart_tx_en_START) |
	       HM_EN(SOC_HI_UART_V500_UART_CONFIG_uart_rx_en_START) |
	       HM_EN(SOC_HI_UART_V500_UART_CONFIG_uart_en_START),
	       SOC_HI_UART_V500_UART_CONFIG_ADDR(g_sysreg_base.uart_base));
#endif
}


static unsigned int proc_mask_to_id(unsigned int mask)
{
	unsigned int i;

	for (i = 0; i < IPC_PROCESSOR_MAX; i++) {
		if ((mask & BIT(i)) != 0)
			break;
	}

	return i;
}

static void ipc_mbx_irq_show(struct seq_file *s, const void __iomem *base, unsigned int mbx)
{
	unsigned int ipc_source, ipc_dest, ipc_data, src_id, dest_id;
	unsigned int i;
#ifdef CONFIG_POWER_DUBAI
	unsigned int source;
	unsigned int mem = 0;
#endif

	ipc_source = readl(base + ipc_mbx_source_offset(mbx));
	src_id = proc_mask_to_id(ipc_source);
	if (src_id < AP_IPC_PROC_MAX) {
		/* is ack irq */
		ipc_dest = readl(base + ipc_mbx_dstatus_offset(mbx));
		dest_id = proc_mask_to_id(ipc_dest);
		if (dest_id < IPC_PROCESSOR_MAX)
			lowpm_msg(s, "SR:mailbox%u: ack from %s",
				  mbx, g_processor_name[dest_id]);
		else
			lowpm_msg(s, "SR:mailbox%u: ack from unknown(dest_id = %u)",
				  mbx, dest_id);
	} else if (src_id < IPC_PROCESSOR_MAX) {
		/* is receive irq */
		lowpm_msg(s, "SR:mailbox%u: send by %s",
			  mbx, g_processor_name[src_id]);
	} else {
		lowpm_msg(s, "SR:mailbox%u: src id unknown(src_id = %u)",
			  mbx, src_id);
	}

	for (i = 0; i < IPC_MBXDATA_MAX; i++) {
		ipc_data = readl(base + ipc_mbxdata_offset(mbx, i));
#ifdef CONFIG_POWER_DUBAI
		if (src_id < IPC_PROCESSOR_MAX &&
		    strncmp(g_processor_name[src_id], "iomcu",
			    PROCESSOR_IOMCU_LENGTH) == 0) {
			if (i == IPC_MBXDATA_MIN)
				mem = get_sharemem_data(ipc_data);
			else if (i == IPC_MBXDATA_TAG) {
				source = get_sharemem_source(ipc_data);
				dubai_log_wakeup_info("DUBAI_TAG_SENSORHUB_WAKEUP",
							 "mem=%u source=%u",
							 mem, source);
			}
		}
#endif
		lowpm_msg(s, "SR:[MBXDATA%u]:0x%x", i, ipc_data);
	}
}

static void combo_ipc_irq_show(struct seq_file *s, unsigned int nsipc_state)
{
	unsigned int mbx;

	/* ns ipc irq detail show */
	lowpm_msg(s, "SR:nsipc irq state:0x%x", nsipc_state);
	if (g_sysreg_base.nsipc_base == NULL) {
		lowpm_msg(s, "SR:nsipc base is NULL");
		return;
	}
	for (mbx = 0; mbx <= NSIPC_MAILBOX_MAX; mbx++) {
		if ((nsipc_state & BIT(mbx)) > 0)
			ipc_mbx_irq_show(s, g_sysreg_base.nsipc_base, mbx);
	}
}

#ifdef AP_AO_NSIPC_IRQ
static char *g_aoipc_processor_name[AO_IPC_PROCESSOR_MAX] = {
	"iomcu",
	"gic",
	"isp"
};

static unsigned int ao_proc_mask_to_id(unsigned int mask)
{
	unsigned int i;

	for (i = 0; i < AO_IPC_PROCESSOR_MAX; i++) {
		if ((mask & BIT(i)) != 0)
			break;
	}

	return i;
}

static void ao_ipc_mbx_irq_show(struct seq_file *s, const void __iomem *base,
				unsigned int mbx)
{
	unsigned int ipc_source, ipc_dest, ipc_data, src_id, dest_id;
	unsigned int i;
#ifdef CONFIG_POWER_DUBAI
	unsigned int source;
	unsigned int mem = 0;
#endif

	ipc_source = readl(base + ipc_mbx_source_offset(mbx));
	src_id = ao_proc_mask_to_id(ipc_source);
	if (src_id == AP_IPC_PROC_BIT) {
		/* is ack irq */
		ipc_dest = readl(base + ipc_mbx_dstatus_offset(mbx));
		dest_id = ao_proc_mask_to_id(ipc_dest);
		if (dest_id < AO_IPC_PROCESSOR_MAX)
			lowpm_msg(s, "SR:mailbox%u: ack from %s",
				  mbx, g_aoipc_processor_name[dest_id]);
		else
			lowpm_msg(s, "SR:mailbox%u: ack from unknown(dest_id = %u)",
				  mbx, dest_id);
	} else if (src_id < AO_IPC_PROCESSOR_MAX) {
		/* is receive irq */
		lowpm_msg(s, "SR:mailbox%u: send by %s",
			  mbx, g_aoipc_processor_name[src_id]);
	} else {
		lowpm_msg(s, "SR:mailbox%u: src id unknown(src_id = %u)",
			  mbx, src_id);
	}

	for (i = 0; i < IPC_MBXDATA_MAX; i++) {
		ipc_data = readl(base + ipc_mbxdata_offset(mbx, i));
#ifdef CONFIG_POWER_DUBAI
		if (src_id < AO_IPC_PROCESSOR_MAX &&
		    strncmp(g_aoipc_processor_name[src_id], "iomcu",
			    PROCESSOR_IOMCU_LENGTH) == 0) {
			if (i == IPC_MBXDATA_MIN) {
				mem = get_sharemem_data(ipc_data);
			} else if (i == IPC_MBXDATA_TAG) {
				source = get_sharemem_source(ipc_data);
				dubai_log_wakeup_info("DUBAI_TAG_SENSORHUB_WAKEUP",
							 "mem=%u source=%u",
							 mem, source);
			}
		}
#endif
		lowpm_msg(s, "SR:[MBXDATA%u]:0x%x", i, ipc_data);
	}
}

static void combo_ao_ipc_irq_show(struct seq_file *s,
				  unsigned int ao_nsipc_state)
{
	unsigned int mbx;

	/* ns ipc irq detail show */
	lowpm_msg(s, "SR:ao nsipc irq state:0x%x", ao_nsipc_state);
	if (g_sysreg_base.ao_nsipc_base == NULL) {
		lowpm_msg(s, "SR:ao nsipc base is NULL");
		return;
	}
	for (mbx = 0; mbx <= AO_NSIPC_MAILBOX_MAX; mbx++)
		if ((ao_nsipc_state & BIT(mbx)) > 0)
			ao_ipc_mbx_irq_show(s, g_sysreg_base.ao_nsipc_base, mbx);
}
#endif

#ifdef CONFIG_LPMCU_BB

#ifdef LPMCU_TICKMARK_FEATURE
#define NEWLINE_INTERVAL		10
static void lpmcu_tickmark_show(struct seq_file *s)
{
	int i;

	lowpm_msg(s, "lpm suspend time[%dus]: 0x%x, 0x%x.",
		  (readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			 TICK_DEEPSLEEP_WFI_ENTER * TICKMARK_STEP) -
		   readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			 TICK_SYS_SUSPEND_ENTRY * TICKMARK_STEP)) * TICKMARK_US,
		  readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			TICK_SYS_SUSPEND_ENTRY * TICKMARK_STEP),
		  readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			TICK_DEEPSLEEP_WFI_ENTER * TICKMARK_STEP));

	lowpm_msg(s, "lpm resume time[%dus]: 0x%x, 0x%x.",
		  (readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			 TICK_SYS_WAKEUP_END * TICKMARK_STEP) -
		   readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			 TICK_SYS_WAKEUP * TICKMARK_STEP)) * TICKMARK_US,
		  readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			TICK_SYS_WAKEUP * TICKMARK_STEP),
		  readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
			TICK_SYS_WAKEUP_END * TICKMARK_STEP));

	lowpm_msg(s, "lpm sr time: ");
	for (i = 0; i < TICK_MARK_END_FLAG; i++) {
		lowpm_msg_cont(s, "[%u]0x%x ",
			       i, readl(M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR +
					i * TICKMARK_STEP));
		if (i % NEWLINE_INTERVAL == 0)
			lowpm_msg_cont(s, "\n");
	}
	lowpm_msg_cont(s, "\n");
}
#endif

static int pm_mask_bit(unsigned int val, unsigned int mask)
{
	if ((val & mask) != 0)
		return 1;
	else
		return 0;
}

static void pm_show_wake_status(struct seq_file *s)
{
	unsigned int wake_status;

	wake_status = readl(g_sysreg_base.reserved_base + WAKE_STATUS_OFFSET);
	lowpm_msg(s,
		  "SR:wakelock status[0x%x], ap:%d, modem:%d, hifi:%d, iomcu:%d, general_see:%d, hotplug:%d,%d.",
		  wake_status,
		  pm_mask_bit(wake_status, AP_MASK),
		  pm_mask_bit(wake_status, MODEM_MASK),
		  pm_mask_bit(wake_status, HIFI_MASK),
		  pm_mask_bit(wake_status, IOMCU_MASK),
		  pm_mask_bit(wake_status, GENERAL_SEE_MASK),
		  pm_mask_bit(wake_status, HOTPLUG_MASK(0)),
		  pm_mask_bit(wake_status, HOTPLUG_MASK(1)));
#ifdef CONFIG_POWER_DUBAI
	dubai_log_wakeup_info("DUBAI_TAG_WAKE_STATUS",
				 "ap=%d modem=%d hifi=%d iomcu=%d general_see=%d hotplug0=%d hotplug1=%d",
				  pm_mask_bit(wake_status, AP_MASK),
				  pm_mask_bit(wake_status, MODEM_MASK),
				  pm_mask_bit(wake_status, HIFI_MASK),
				  pm_mask_bit(wake_status, IOMCU_MASK),
				  pm_mask_bit(wake_status, GENERAL_SEE_MASK),
				  pm_mask_bit(wake_status, HOTPLUG_MASK(0)),
				  pm_mask_bit(wake_status, HOTPLUG_MASK(1)));
#endif
	lowpm_msg(s, "SR:system sleeped %u times.",
		  readl(g_sysreg_base.reserved_base + SYS_DSLEEP_CNT_OFFSET));
	lowpm_msg(s,
		  "SR:wake times, system:%u, woken up by ap:%u, modem:%u, hifi:%u, iomcu:%u, lpm3:%u.",
		  readl(g_sysreg_base.reserved_base + SYS_DWAKE_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + AP_WAKE_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + MODEM_WAKE_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + HIFI_WAKE_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + IOMCU_WAKE_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + LPM3_WAKE_CNT_OFFSET));
}

static void pm_show_wake_irqs(struct seq_file *s)
{
	unsigned int wake_irq, wake_irq1, ap_irq, bit_shift;
#ifdef AP_WAKE_IRQ_PIE
	unsigned int ap_pie_irq;
#endif
	unsigned int i;

	wake_irq = readl(g_sysreg_base.reserved_base + WAKE_IRQ_OFFSET);
	wake_irq1 = readl(g_sysreg_base.reserved_base + WAKE_IRQ1_OFFSET);
	lowpm_msg(s, "SR:WAKE_IRQ:0x%x, WAKE_IRQ1:0x%x", wake_irq, wake_irq1);
	for (i = 0; i < WAKE_IRQ_NUM_MAX; i++) {
		bit_shift = 1 << i;
		if ((((wake_irq & AP_WAKE_INT_MASK) |
		      (wake_irq1 & AP_WAKE_INT_MASK1)) & bit_shift) != 0)
			lowpm_msg(s, "LAST DEEPSLEEP WAKE BIT: %d(AP)", i);

		if (((wake_irq & IOMCU_WAKE_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST DEEPSLEEP WAKE BIT: %d(IOMCU)", i);

		if (((wake_irq & MODEM_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST DEEPSLEEP WAKE BIT: %d(MODEM)", i);

		if (((wake_irq1 & MODEM_DRX_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST DEEPSLEEP WAKE DRX_BIT: %d(MODEM)", i);

		if (((wake_irq & HIFI_WAKE_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST DEEPSLEEP WAKE BIT: %d(HIFI)", i);
	}

	wake_irq = readl(g_sysreg_base.reserved_base + SLEEP_WAKE_IRQ_OFFSET);
	wake_irq1 = readl(g_sysreg_base.reserved_base + SLEEP_WAKE_IRQ1_OFFSET);
	for (i = 0; i < WAKE_IRQ_NUM_MAX; i++) {
		bit_shift = 1 << i;
		if ((((wake_irq & AP_WAKE_INT_MASK) |
		      (wake_irq1 & AP_WAKE_INT_MASK1)) & bit_shift) != 0)
			lowpm_msg(s, "LAST SLEEP WAKE BIT: %d(AP)", i);

		if (((wake_irq & IOMCU_WAKE_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST SLEEP WAKE BIT: %d(IOMCU)", i);

		if (((wake_irq & MODEM_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST SLEEP WAKE BIT: %d(MODEM)", i);

		if (((wake_irq1 & MODEM_DRX_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST SLEEP WAKE DRX_BIT: %d(MODEM)", i);

		if (((wake_irq & HIFI_WAKE_INT_MASK) & bit_shift) != 0)
			lowpm_msg(s, "LAST SLEEP WAKE BIT: %d(HIFI)", i);
	}

	ap_irq = readl(g_sysreg_base.reserved_base + AP_WAKE_IRQ_OFFSET);
	if (g_lpmcu_irq_num > ap_irq && g_lpmcu_irq_name != NULL) {
#ifdef CONFIG_POWER_DUBAI
		dubai_log_irq_wakeup(DUBAI_IRQ_WAKEUP_TYPE_LPMCU, g_lpmcu_irq_name[ap_irq], -1);
#endif
		lowpm_msg(s, "SR:AP WAKE IRQ(LPM3 NVIC): %d (%s)",
			  ap_irq, g_lpmcu_irq_name[ap_irq]);
	} else {
		lowpm_msg(s, "SR:AP WAKE IRQ(LPM3 NVIC): %d (no exist)", ap_irq);
	}

#ifdef AP_WAKE_IRQ_PIE
	ap_pie_irq = readl(g_sysreg_base.reserved_base + AP_WAKE_IRQ_PIE_OFFSET);
	lowpm_msg(s, "PIE VALUE: 0x%x", ap_pie_irq);
#endif

	if (ap_irq == IRQ_COMB_GIC_IPC) {
		combo_ipc_irq_show(s, readl(g_sysreg_base.reserved_base +
					    AP_NSIPC_IRQ_OFFSET));
#ifdef AP_AO_NSIPC_IRQ
		combo_ao_ipc_irq_show(s, readl(g_sysreg_base.reserved_base +
					       AP_AO_NSIPC_IRQ_OFFSET));
#endif
	}
}
#endif

void pm_status_show(struct seq_file *s)
{
#ifdef CONFIG_LPMCU_BB
	if (g_sysreg_base.reserved_base == NULL) {
		if (M3_RDR_SYS_CONTEXT_BASE_ADDR != 0) {
			g_sysreg_base.reserved_base = M3_RDR_SYS_CONTEXT_RUNTIME_VAR_ADDR;
		} else{
			pr_err("%s: M3_RDR_SYS_CONTEXT_BASE_ADDR is NULL\n", __func__);
			return;
		}
	}

	pm_show_wake_status(s);

	lowpm_msg_cont(s,
		       "SR:sr times of sub system, ap:s-%u, r-%u, modem:s-%u, r-%u, hifi:s-%u, r-%u, iomcu:s-%u, r-%u.",
		       readl(g_sysreg_base.reserved_base + AP_SUSPEND_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + AP_RESUME_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + MODEM_SUSPEND_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + MODEM_RESUME_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + HIFI_SUSPEND_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + HIFI_RESUME_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + IOMCU_SUSPEND_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + IOMCU_RESUME_CNT_OFFSET));
#if defined(GENERAL_SEE_SUSPEND_CNT_OFFSET) && defined(GENERAL_SEE_RESUME_CNT_OFFSET)
	lowpm_msg_cont(s,
		       "general_see:s-%u, r-%u.",
		       readl(g_sysreg_base.reserved_base + GENERAL_SEE_SUSPEND_CNT_OFFSET),
		       readl(g_sysreg_base.reserved_base + GENERAL_SEE_RESUME_CNT_OFFSET));
#endif
	lowpm_msg_cont(s, "\n");

#if defined(CCPUNR_SUSPEND_CNT_OFFSET) && defined(CCPUNR_RESUME_CNT_OFFSET)
	lowpm_msg(s, "SR:sr times of sub system, modem_nr:s-%u, r-%u.",
		  readl(g_sysreg_base.reserved_base + CCPUNR_SUSPEND_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + CCPUNR_RESUME_CNT_OFFSET));
	lowpm_msg(s, "SR:wakelock status, modem_nr:%d.",
		  (readl(g_sysreg_base.reserved_base + WAKE_STATUS_OFFSET) &
		   MODEM_NR_MASK) ? 1 : 0);
#if defined(MODEM_NR_WAKE_CNT_OFFSET)
	lowpm_msg(s, "SR:woken up by modem_nr:%u.",
		  readl(g_sysreg_base.reserved_base + MODEM_NR_WAKE_CNT_OFFSET));
#endif
#endif
#if defined(OTHER_WAKE_CNT_OFFSET)
	lowpm_msg(s, "SR:woken up by other:%u.",
		  readl(g_sysreg_base.reserved_base + OTHER_WAKE_CNT_OFFSET));
#endif

	if (g_sysreg_base.sysctrl_base != NULL) {
		lowpm_msg(s, "SR:SCINT_STAT:0x%x.",
			  readl(g_sysreg_base.sysctrl_base + SOC_SCTRL_SCINT_STAT_ADDR(0)));
		lowpm_msg(s, "SR:SCINT_STAT1:0x%x.",
			  readl(g_sysreg_base.sysctrl_base + SOC_SCTRL_SCINT_STAT1_ADDR(0)));
	}

	pm_show_wake_irqs(s);

	lowpm_msg(s, "sleep 0x%x times.",
		  readl(g_sysreg_base.reserved_base + SYS_SLEEP_CNT_OFFSET));

#if defined(SYS_SLEEP_CAN_ENTER_CNT_OFFSET) && defined(SYS_SLEEP_TIME_OFFSET)
	lowpm_msg(s, "sleep try %d times; enter time %d",
		  readl(g_sysreg_base.reserved_base + SYS_SLEEP_CAN_ENTER_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + SYS_SLEEP_TIME_OFFSET));
#endif

#if defined (SLEEP_FAIL_ITEM_OFFSET) && defined (SLEEP_FAIL_ITEM_VALUE_OFFSET)
	lowpm_msg(s, "sleep fail to enter for items 0x%x, item value is 0x%x",
		  readl(g_sysreg_base.reserved_base + SLEEP_FAIL_ITEM_OFFSET),
		  readl(g_sysreg_base.reserved_base + SLEEP_FAIL_ITEM_VALUE_OFFSET));
#endif

#if defined(FCM_OFF_CNT_OFFSET) && defined(FCM_ON_CNT_OFFSET)
	lowpm_msg(s, "CLUSTER/FCM power down 0x%x; up 0x%x.",
		  readl(g_sysreg_base.reserved_base + FCM_OFF_CNT_OFFSET),
		  readl(g_sysreg_base.reserved_base + FCM_ON_CNT_OFFSET));
#endif

#ifdef LPMCU_TICKMARK_FEATURE
	lpmcu_tickmark_show(s);
#endif
#endif
}

#if defined(CONFIG_SR_DEBUG_LPREG) || (!defined(CONFIG_SR_DEBUG_SLEEP))
static void set_wakelock(int iflag)
{
	if (iflag == 1 && g_lowpm_wake_lock->active == 0)
		__pm_stay_awake(g_lowpm_wake_lock);
	else if (iflag == 0 && g_lowpm_wake_lock->active != 0)
		__pm_relax(g_lowpm_wake_lock);
}
#endif

#ifdef CONFIG_SR_DEBUG_LPREG
static void lp_read_chipid(void)
{
	int ret, i;

	for (i = 0; i < CHIPID_DIG_NUMS; i++)
		g_chipid[i] = 0;
	ret = of_property_read_u32_array(of_find_node_by_path("/"),
					 "hisi,chipid", &g_chipid[0],
					 CHIPID_DIG_NUMS);
	if (ret != 0) {
		pr_err("%s: not find chipid, but may not wrong.\n", __func__);
		return;
	}

	lowpm_msg(NO_SEQFILE, "%s: chipid is:\t", __func__);
	for (i = 0; i < CHIPID_DIG_NUMS; i++)
		lowpm_msg_cont(NO_SEQFILE, "%x ", g_chipid[i]);

	lowpm_msg_cont(NO_SEQFILE, "\n");
}

static int compare_chipid(int *chipid)
{
	int i;

	for (i = 0; i < CHIPID_DIG_NUMS; i++) {
		if (chipid[i] != g_chipid[i])
			return -1;
	}
	return 0;
}

static int map_io_regs(void)
{
	unsigned int i;
	int ret, err;
	struct device_node *np = NULL;
	char io_buffer[BUFFER_LENGTH];

	lp_read_chipid();

	/* find io group max num */
	i = 0;
	while (1) {
		err = snprintf_s(io_buffer, BUFFER_LENGTH * sizeof(char),
				 (BUFFER_LENGTH - 1) * sizeof(char),
				 "arm,primecell%u", i);
		if (err < 0) {
			pr_err("%s: snprintf_s io_buffer err.\n", __func__);
			ret = err;
			goto err;
		}
		np = of_find_compatible_node(NULL, NULL, io_buffer);
		if (np == NULL) {
			pr_err("%s[%d]: gpio%d not find.\n",
			       __func__, __LINE__, i);
			if (i == 0) {
				pr_err("%s: no group num!\n", __func__);
				ret = -ENODEV;
				goto err;
			}
			break;
		}
		i++;
		of_node_put(np);
	}
	g_io_group_num = i;
	pr_info("%s: find gpio group num: %u\n", __func__, g_io_group_num);

	g_sysreg_base.gpio_base = kcalloc(g_io_group_num,
					  sizeof(void __iomem *), GFP_KERNEL);
	if (g_sysreg_base.gpio_base == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < g_io_group_num; i++) {
		err = snprintf_s(io_buffer, BUFFER_LENGTH * sizeof(char),
				 (BUFFER_LENGTH - 1) * sizeof(char),
				 "arm,primecell%u", i);
		if (err < 0) {
			pr_err("%s: snprintf_s io_buffer err.\n", __func__);
			ret = err;
			goto err_free_io;
		}
		np = of_find_compatible_node(NULL, NULL, io_buffer);
		if (np == NULL) {
			pr_err("%s[%d]: gpio%d not find.\n",
			       __func__, __LINE__, i);
			ret = -ENODEV;
			goto err_free_io;
		}
		g_sysreg_base.gpio_base[i] = of_iomap(np, 0);
		pr_debug("%s: g_sysreg_base.gpio_base[%d] %pK\n",
			 __func__, i, g_sysreg_base.gpio_base[i]);
		if (g_sysreg_base.gpio_base[i] == NULL) {
			pr_err("%s: gpio%d iomap err.\n", __func__, i);
			ret = -ENOMEM;
			goto err_put_node;
		}
		of_node_put(np);
	}

	return 0;

err_put_node:
	of_node_put(np);
err_free_io:
	kfree(g_sysreg_base.gpio_base);
	g_sysreg_base.gpio_base = NULL;
err:
	pr_err("%s: failed!\n", __func__);
	return ret;
}

#ifdef CONFIG_PM_SEC_IO
static bool find_addr_in_range_table(void __iomem **table, int table_size,
				     void __iomem *addr)
{
	int j;

	for (j = RANGE_START; j < table_size; j += RANGE_STEP)
		if (addr >= table[j - 1] && addr <= table[j])
			return true;
	return false;
}
#endif


static int show_iomg_sec(struct seq_file *s, struct iocfg_lp *iocfg)
{
#ifdef CONFIG_PM_SEC_IO
	void __iomem *addr = NULL;

	if (g_mg_sec_reg_num > 0 && g_mg_reg != NULL) {
		addr = (void __iomem *)(uintptr_t)(iocfg->iomg_base +
						   iocfg->iomg_offset);
		if (find_addr_in_range_table(g_mg_reg,
					     g_mg_sec_reg_num, addr)) {
			lowpm_msg_cont(s, "sec iomg");
			return 1;
		}
	}
	return 0;
#else
	return 0;
#endif
}

static void show_iomg_reg(struct seq_file *s, struct iocfg_lp *iocfg)
{
	void __iomem *addr = NULL;
	int value;

	lowpm_msg_cont(s, "iomg:");

	if (iocfg->iomg_offset == -1) {
		lowpm_msg_cont(s, "null");
		goto end;
	}

	if (show_iomg_sec(s, iocfg) == 1)
		goto end;
	/*lint -e446*/
	addr = ioremap(iocfg->iomg_base + iocfg->iomg_offset, ADDR_SIZE);
	/*lint +e446*/
	if (addr == NULL) {
		lowpm_msg_cont(s, "ioremap fail");
		goto end;
	}
	value = readl(addr);
	iounmap(addr);

	lowpm_msg_cont(s, "Func-%d", value);
	if (value == FUNC1)
		lowpm_msg_cont(s, "(%s)", iocfg->gpio_func1);
	else if (value == FUNC2)
		lowpm_msg_cont(s, "(%s)", iocfg->gpio_func2);
	else if (value == FUNC3)
		lowpm_msg_cont(s, "(%s)", iocfg->gpio_func3);

	lowpm_msg_cont(s, "\t");

	/* check conflict */
	if (value != iocfg->iomg_val) {
		lowpm_msg_cont(s, " -E [Func-%d]", iocfg->iomg_val);
		if (iocfg->iomg_val == FUNC1)
			lowpm_msg_cont(s, "(%s)", iocfg->gpio_func1);
		if (iocfg->iomg_val == FUNC2)
			lowpm_msg_cont(s, "(%s)", iocfg->gpio_func2);
		if (iocfg->iomg_val == FUNC3)
			lowpm_msg_cont(s, "(%s)", iocfg->gpio_func3);
	}
end:
	lowpm_msg_cont(s, "\t");
}

static int show_iocg_sec(struct seq_file *s, struct iocfg_lp *iocfg)
{
#ifdef CONFIG_PM_SEC_IO
	void __iomem *addr = NULL;

	if (g_cg_sec_reg_num > 0 && g_cg_reg != NULL) {
		addr = (void __iomem *)(uintptr_t)(iocfg->iocg_base +
						   iocfg->iocg_offset);
		if (find_addr_in_range_table(g_cg_reg,
					     g_cg_sec_reg_num, addr)) {
			lowpm_msg_cont(s, "sec iocg");
			return 1;
		}
	}
#endif
	return 0;
}

static void show_iocg_reg(struct seq_file *s, struct iocfg_lp *iocfg)
{
	void __iomem *addr = NULL;
	int value;

	lowpm_msg_cont(s, "iocg:");

	if (show_iocg_sec(s, iocfg) == 1)
		goto end;

	/*lint -e446*/
	addr = ioremap(iocfg->iocg_base + iocfg->iocg_offset, ADDR_SIZE);
	/*lint +e446*/
	if (addr == NULL) {
		lowpm_msg_cont(s, "ioremap fail");
		goto end;
	}
	value = readl(addr);
	iounmap(addr);
	lowpm_msg_cont(s, "[0x%x]iocg = %s",
		       value, pulltype[((unsigned int)value) & IOCG_MASK]);

	if (value != iocfg->iocg_val && iocfg->iocg_val != -1)
		lowpm_msg_cont(s, " -E [%s]", pulltype[iocfg->iocg_val]);

end:
	lowpm_msg_cont(s, "\t");
}

static int show_gpio_sec(struct seq_file *s, struct iocfg_lp *iocfg)
{
#ifdef CONFIG_PM_SEC_GPIO
	int j;
	unsigned int gpio_id = (iocfg->gpio_group_id << GPIO_GROUP_ID_OFFSET) +
			       iocfg->ugpio_bit;

	for (j = RANGE_START; j < g_sec_gpio_num; j = j + RANGE_STEP) {
		if (gpio_id >= g_sec_gpio_array[j - 1] &&
		    gpio_id <= g_sec_gpio_array[j]) {
			lowpm_msg_cont(s, "sec gpio");
			return 1;
		}
	}
	return 0;
#else
	return 0;
#endif
}

static void show_gpio_info(struct seq_file *s, struct iocfg_lp *iocfg)
{
	void __iomem *addr = NULL;
	void __iomem **addr1 = NULL;
	int value, data;

	lowpm_msg_cont(s, "gpio:");

	if (show_gpio_sec(s, iocfg) == 1)
		return;

	if (iocfg->iomg_val == FUNC0) {
		addr = gpio_dir(g_sysreg_base.gpio_base[iocfg->gpio_group_id]);
		addr1 = gpio_data(g_sysreg_base.gpio_base[iocfg->gpio_group_id],
				  iocfg->ugpio_bit);

		value = gpio_is_set((u32)readl(addr), iocfg->ugpio_bit);
		data = gpio_is_set((u32)readl(addr1), iocfg->ugpio_bit);
		lowpm_msg_cont(s, "[0x%x 0x%x]gpio-%s",
			       value, data, value ? "O" : "I");

		if (value != 0)
			lowpm_msg_cont(s, "%s", data ? "H" : "L");

		if (value != iocfg->gpio_dir) {
			lowpm_msg_cont(s, "\t-E [%s",
				       iocfg->gpio_dir ? "O" : "I");
			if (iocfg->gpio_dir > 0 && data != iocfg->gpio_val)
				lowpm_msg_cont(s, "%s",
					       iocfg->gpio_val ? "H" : "L");
			lowpm_msg_cont(s, "]");
		}
	}
}

static struct iocfg_lp *dbg_get_iocfg_by_name(struct seq_file *s)
{
	struct iocfg_lp *iocfg_lookups = NULL;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(iocfg_tables); i++) {
		if (strnstr(g_plat_name, "es", strlen(g_plat_name)) &&
		    strnstr(iocfg_tables[i].boardname, "chip_es",
			    sizeof(iocfg_tables[i].boardname))) {
			iocfg_lookups = iocfg_tables[i].iocfg;
			lowpm_msg(s, "chip es board");
			break;
		}
		if (strnstr(g_plat_name, "cs2", strlen(g_plat_name)) &&
		    strnstr(iocfg_tables[i].boardname, "chip_cs2",
			    sizeof(iocfg_tables[i].boardname))) {
			iocfg_lookups = iocfg_tables[i].iocfg;
			lowpm_msg(s, "chip cs2 board");
			break;
		}
		if (strnstr(g_plat_name, "cs", strlen(g_plat_name)) &&
		    strnstr(iocfg_tables[i].boardname, "chip_cs",
			    sizeof(iocfg_tables[i].boardname)) &&
		    !strnstr(iocfg_tables[i].boardname,	"chip_cs2",
			     sizeof(iocfg_tables[i].boardname))) {
			iocfg_lookups = iocfg_tables[i].iocfg;
			lowpm_msg(s, "chip cs board");
			break;
		}
		if (strnstr(iocfg_tables[i].boardname, "chip",
			    sizeof(iocfg_tables[i].boardname))) {
			iocfg_lookups = iocfg_tables[i].iocfg;
			lowpm_msg(s, "chip board");
			break;
		}
	}

	return iocfg_lookups;
}

static void dbg_disp_iocfg_tables(struct seq_file *s)
{
	unsigned int i, j;

	lowpm_msg(s, "iocfg table length is %lu: ", ARRAY_SIZE(iocfg_tables));
	for (i = 0; i < ARRAY_SIZE(iocfg_tables); i++) {
		lowpm_msg_cont(s, "chipid:< ");
		for (j = 0; j < CHIPID_DIG_NUMS; j++)
			lowpm_msg_cont(s, "%X ", iocfg_tables[i].chipid[j]);
		lowpm_msg_cont(s, ">; boardname:%s\n",
			       iocfg_tables[i].boardname);
	}
}

static struct iocfg_lp *dbg_get_iocfg_by_chipid(struct seq_file *s)
{
	struct iocfg_lp *iocfg_lookups = NULL;
	int *chipid = NULL;
	unsigned int i;

	dbg_disp_iocfg_tables(s);
	/* get iocfg table */
	for (i = 0; i < ARRAY_SIZE(iocfg_tables); i++) {
		chipid = iocfg_tables[i].chipid;
		if (compare_chipid(chipid) == 0) {
			iocfg_lookups = iocfg_tables[i].iocfg;
			lowpm_msg(s, "find by chipid, board name is %s.\n",
				  iocfg_tables[i].boardname);
			break;
		}
	}

	return iocfg_lookups;
}

void io_status_show(struct seq_file *s)
{
	unsigned int i, gpio_id;
	unsigned int len = 0;
	struct iocfg_lp *iocfg_lookups = NULL;
	struct iocfg_lp *iocfg = NULL;

	if ((g_usavedcfg & DEBG_SUSPEND_IO_SHOW) != DEBG_SUSPEND_IO_SHOW)
		return;

	if (g_lp_fpga_flag == 1) {
		lowpm_msg(s, "gpio not support fpga.");
		return;
	}

	iocfg_lookups = dbg_get_iocfg_by_chipid(s);
	if (iocfg_lookups == NULL)
		iocfg_lookups = dbg_get_iocfg_by_name(s);

	if (iocfg_lookups == NULL) {
		lowpm_msg(s, "canot find iocfg table.");
		return;
	}

	/* calc iocfg_lookups[] length */
	for (i = 0; iocfg_lookups[i].iomg_offset != CFG_END; i++)
		len = i;

	lowpm_msg(s, "io list length is %u :", len);

	for (i = 0; i < len; i++) {
		iocfg = &iocfg_lookups[i];
		gpio_id = (iocfg->gpio_group_id << GPIO_GROUP_ID_OFFSET) +
			  iocfg->ugpio_bit;

		lowpm_msg_cont(s, "gpio_id:%d\tgpio_logic:%s\t",
			       gpio_id, iocfg->gpio_logic);

		show_iomg_reg(s, iocfg);
		show_iocg_reg(s, iocfg);
		show_gpio_info(s, iocfg);

		lowpm_msg_cont(s, "\n");
	}
}

void pmu_status_show(struct seq_file *s)
{
	unsigned int i, uregv;
	unsigned int k = 0;
	struct pmu_idx *pmuidx = NULL;
	struct pmuregs *pmureg = NULL;

	if ((g_usavedcfg & DEBG_SUSPEND_PMU_SHOW) != DEBG_SUSPEND_PMU_SHOW)
		return;

	lowpm_msg(s, "PMU_CTRL register list length is %d:", g_pmu_addr_end);

	for (i = 0; i <= g_pmu_addr_end; i++) {
#ifdef	CONFIG_PMIC_SPMI
		uregv = pmic_read_reg(i);
#else
		uregv = readl(pmussi_reg(i));
#endif
		lowpm_msg_cont(s, "PMU_CTRL 0x%02X=0x%02X\t", i, uregv);
		/*
		 * NOTE!! g_pmuregs_lookups[k] must be matched with g_pmu_idx[k],
		 * otherwise the array may be out of range.
		 */
		if (k < g_lp_pmu_num && g_pmu_idx != NULL &&
		    g_pmuregs_lookups != NULL) {
			pmuidx = g_pmu_idx[k];
			pmureg = g_pmuregs_lookups[k];
			if (pmuidx == NULL || pmureg == NULL ||
			    pmureg->ucoffset != i) {
				lowpm_msg_cont(s, "\n");
				continue;
			}
			lowpm_msg_cont(s, "[%s]", pmuidx->name);
			if ((uregv & pmureg->cmask) == pmureg->cval)
				lowpm_msg_cont(s, "enabled  %s\t",
					       (pmuidx->status & AP_DIS) ? "-E" : "");
			lowpm_msg_cont(s, "[%s]owner:%s",
				       pmuidx->status_name, pmuidx->module);
			k++;
		}
		lowpm_msg_cont(s, "\n");
	}
}

static void clk_showone(struct seq_file *s, struct lp_clk *clk)
{
	u32 regval, bitval, regoff, bit_idx;
	void __iomem *regbase = NULL;

	if (clk == NULL) {
		lowpm_msg_cont(s, "null ptr clk");
		return;
	}

	switch (clk->reg_base) {
	case SOC_ACPU_SCTRL_BASE_ADDR:
		lowpm_msg_cont(s, "[%s]", "SYSCTRL");
		regbase = g_sysreg_base.sysctrl_base;
		break;
	case SOC_ACPU_PERI_CRG_BASE_ADDR:
		lowpm_msg_cont(s, "[%s]", "CRGPERI");
		regbase = g_sysreg_base.crg_base;
		break;
	case SOC_ACPU_PMC_BASE_ADDR:
		lowpm_msg_cont(s, "[%s]", "PMCCTRL");
		regbase = g_sysreg_base.pctrl_base;
		break;
	default:
		lowpm_msg_cont(s, "invalid base address");
		return;
	}

	regoff = clk->reg_off;
	bit_idx = clk->bit_idx;
	regval = readl(regbase + regoff);
	bitval = regval & BIT(bit_idx);

	lowpm_msg_cont(s, " offset:0x%x regval:0x%x bit_idx:%s%d state:%d %s",
		       regoff, regval, bit_idx > 9 ? "" : "0",
		       bit_idx, bitval ? 1 : 0, clk->clk_name);

	if (bitval != 0) {
		if (clk->status == NULL_CLOSE || clk->status == AP_CLOSE)
			lowpm_msg_cont(s, " -E[OFF]");
	} else {
		if (clk->status == AP_ON || clk->status == AO_ON)
			lowpm_msg_cont(s, " -E[ON]");
	}
	if (clk->status == ON_OFF)
		lowpm_msg_cont(s, "[on/off]");
	else if (clk->status == MODEM_ON)
		lowpm_msg_cont(s, "[MODEM: on or off]");
	lowpm_msg_cont(s, "\n");
}

void clk_status_show(struct seq_file *s)
{
	unsigned int i;

	if ((g_usavedcfg & DEBG_SUSPEND_CLK_SHOW) != DEBG_SUSPEND_CLK_SHOW)
		return;

	lowpm_msg(s, "clk list length is %u:", g_lp_clk_num);

	for (i = 0; i < g_lp_clk_num; i++)
		clk_showone(s, g_clk_lookups[i]);
}

static int pmu_fill_buffer(char *buffer, int len, int index)
{
	int ret;

	ret = snprintf_s(buffer, len * sizeof(char), (len - 1) * sizeof(char),
			 "hisilicon, lp_pmu%u", index);
	if (ret < 0) {
		pr_err("%s: snprintf_s buffer err.\n", __func__);
		return ret;
	}
	return 0;
}

static int init_pmu_idx_lookup(struct device_node *np, struct pmu_idx *pmu,
			       struct pmuregs *pmu_reg, int index)
{
	int ret;

	ret = of_property_read_string(np, "lp-pmu-name", &(pmu->name));
	if (ret != 0) {
		pr_err("%s, [%d]no lp-pmu-name!\n", __func__, index);
		return -ENODEV;
	}
	ret = of_property_read_string(np, "lp-pmu-module", &(pmu->module));
	if (ret != 0) {
		pr_err("%s, [%d]no lp-pmu-module!\n", __func__, index);
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "lp-pmu-status-value", &(pmu->status));
	if (ret != 0) {
		pr_err("%s, [%d]no lp-pmu-status-value!\n", __func__, index);
		return -ENODEV;
	}
	ret = of_property_read_string(np, "lp-pmu-status", &(pmu->status_name));
	if (ret != 0) {
		pr_err("%s, [%d]no lp-pmu-status!\n", __func__, index);
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "offset", &(pmu_reg->ucoffset));
	if (ret != 0) {
		pr_err("%s, [%d]no offset!\n", __func__, index);
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "mask", &(pmu_reg->cmask));
	if (ret != 0) {
		pr_err("%s, [%d]no mask!\n", __func__, index);
		return -ENODEV;
	}
	pmu_reg->cval = pmu_reg->cmask;
	return 0;
}

static int init_pmu_lookup_table(void)
{
	int ret = 0;
	u32 i;
	struct device_node *np = NULL;
	char buffer[BUFFER_LENGTH];

	g_pmu_idx = kzalloc(g_lp_pmu_num * sizeof(struct pmu_idx *),
				 GFP_KERNEL);
	if (g_pmu_idx == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	g_pmuregs_lookups = kzalloc(g_lp_pmu_num * sizeof(struct pmuregs *),
				    GFP_KERNEL);
	if (g_pmuregs_lookups == NULL) {
		ret = -ENOMEM;
		goto err_free_pmu_idx;
	}

	/* init pmu idx and lookup table */
	for (i = 0; i < g_lp_pmu_num; i++) {
		ret = pmu_fill_buffer(buffer, BUFFER_LENGTH, i);
		if (ret != 0)
			goto err_free_lookups;

		np = of_find_compatible_node(NULL, NULL, buffer);
		if (np == NULL) {
			pr_err("%s[%d]: lp_pmu%d not find.\n",
			       __func__, __LINE__, i);
			ret = -ENODEV;
			goto err_free_lookups;
		}
		g_pmu_idx[i] = kzalloc(sizeof(struct pmu_idx), GFP_KERNEL);
		if (g_pmu_idx[i] == NULL) {
			ret = -ENOMEM;
			goto err_put_node;
		}
		g_pmuregs_lookups[i] = kzalloc(sizeof(struct pmuregs),
					       GFP_KERNEL);
		if (g_pmuregs_lookups[i] == NULL) {
			ret = -ENOMEM;
			goto err_put_node;
		}
		ret = init_pmu_idx_lookup(np, g_pmu_idx[i],
					  g_pmuregs_lookups[i], i);
		if (ret != 0)
			goto err_put_node;
		of_node_put(np);
	}

	pr_info("%s: %d init success.\n", __func__, __LINE__);
	return ret;

err_put_node:
	of_node_put(np);
err_free_lookups:
	lowpm_kfree_array(g_pmuregs_lookups, g_lp_pmu_num, i);
err_free_pmu_idx:
	lowpm_kfree_array(g_pmu_idx, g_lp_pmu_num, i);
err:
	pr_err("%s init failed.\n", __func__);
	return ret;
}

static int init_pmu_table(void)
{
	int ret, err;
	u32 i;
	struct device_node *np = NULL;
	char buffer[BUFFER_LENGTH];

	np = of_find_compatible_node(NULL, NULL, "hisilicon,lowpm_func");
	if (np == NULL) {
		pr_err("%s[%d]: lowpm_func No compatible node found\n",
		       __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}

	ret = of_property_read_u32(np, "pmu-addr-end", &g_pmu_addr_end);
	if (ret != 0) {
		pr_err("%s[%d], no pmu-addr-end!\n", __func__, __LINE__);
		ret = -ENODEV;
		of_node_put(np);
		goto err;
	}
	of_node_put(np);

	/* find lp_pmu max num */
	i = 0;
	while (1) {
		err = snprintf_s(buffer, BUFFER_LENGTH * sizeof(char),
				 (BUFFER_LENGTH - 1) * sizeof(char),
				 "hisilicon, lp_pmu%u", i);
		if (err < 0) {
			pr_err("%s: snprintf_s buffer err.\n", __func__);
			ret = err;
			goto err;
		}
		np = of_find_compatible_node(NULL, NULL, buffer);
		if (np == NULL) {
			pr_err("%s[%d]: lp_pmu%d not find.\n",
			       __func__, __LINE__, i);
			ret = -ENODEV;
			break;
		}
		i++;
		of_node_put(np);
	}
	g_lp_pmu_num = i;
	pr_info("%s: find lp pmu num: %u\n", __func__, g_lp_pmu_num);

	ret = init_pmu_lookup_table();
	if (ret != 0) {
		pr_err("%s[%d]: init_pmu_lookup_table failed!!\n",
		       __func__, __LINE__);
		goto err;
	}

	pr_info("%s, init pmu table success.\n", __func__);
	return ret;

err:
	pr_err("%s init failed.\n", __func__);
	return ret;
}

static int clk_fill_buffer(char *buffer, int len, int index)
{
	int ret;

	ret = snprintf_s(buffer, len * sizeof(char), (len - 1) * sizeof(char),
			 "hisilicon, lp_clk%d", index);
	if (ret < 0) {
		pr_err("%s: snprintf_s buffer err.\n", __func__);
		return ret;
	}
	return 0;
}

static int init_clk_lookup_table(u32 reg_num)
{
	int ret = 0;
	u32 i, j, k;
	struct device_node *np = NULL;
	u32 reg_base, reg_offset, bit_num;
	char buffer[BUFFER_LENGTH];

	g_clk_lookups = kzalloc(g_lp_clk_num * sizeof(struct lp_clk *),
				GFP_KERNEL);
	if (g_clk_lookups == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	/* init lp clk table */
	k = 0;
	for (i = 0; i < reg_num; i++) {
		if (k >= g_lp_clk_num) {
			pr_err("%s[%d]: k >= g_lp_clk_num, k:%d,i:%d!\n",
			       __func__, __LINE__, k, i);
			goto err_free_lookups;
		}
		ret = clk_fill_buffer(buffer, BUFFER_LENGTH, i);
		if (ret != 0)
			goto err_free_lookups;
		np = of_find_compatible_node(NULL, NULL, buffer);
		if (np == NULL) {
			pr_err("%s[%d]: lp_clk%d not find.\n",
			       __func__, __LINE__, i);
			goto err_free_lookups;
		}
		ret = of_property_read_u32(np, "lp-clk-base", &reg_base);
		if (ret != 0) {
			pr_err("%s, [%d]no lp-clk-base!\n", __func__, i);
			goto err_put_node;
		}
		ret = of_property_read_u32(np, "offset", &reg_offset);
		if (ret != 0) {
			pr_err("%s, [%d]no offset!\n", __func__, i);
			goto err_put_node;
		}
		ret = of_property_read_u32(np, "bit-num", &bit_num);
		if (ret != 0) {
			pr_err("%s, [%d]no bit-num!\n", __func__, i);
			goto err_put_node;
		}

		for (j = 0; j < bit_num; j++) {
			g_clk_lookups[k] = kzalloc(sizeof(struct lp_clk),
						   GFP_KERNEL);
			if (g_clk_lookups[k] == NULL) {
				ret = -ENOMEM;
				goto err_put_node;
			}

			g_clk_lookups[k]->reg_base = reg_base;
			g_clk_lookups[k]->reg_off = reg_offset;
			g_clk_lookups[k]->bit_idx = j;

			ret = of_property_read_string_index(np, "lp-clk-name",
							    j, &(g_clk_lookups[k]->clk_name));
			if (ret != 0) {
				pr_err("%s, [%d %d]no lp-clk-name!\n",
				       __func__, i, j);
				goto err_put_node;
			}

			(void)of_property_read_u32_index(np, "lp-clk-status-value",
							 j, &(g_clk_lookups[k]->status));

			k++;
		}
		of_node_put(np);
	}

	pr_info("%s: %d init success.\n", __func__, __LINE__);
	return ret;

err_put_node:
	of_node_put(np);
err_free_lookups:
	lowpm_kfree_array(g_clk_lookups, g_lp_clk_num, i);
err:
	pr_err("%s init failed.\n", __func__);
	return ret;
}

static int init_clk_table(void)
{
	int ret, err;
	u32 i, bit_num, clk_reg_num;
	struct device_node *np = NULL;
	char buffer[BUFFER_LENGTH];

	np = of_find_compatible_node(NULL, NULL, "hisilicon,lowpm_func");
	if (np == NULL) {
		pr_err("%s: lowpm_func No compatible node found\n",
		       __func__);
		ret = -ENODEV;
		goto err;
	}
	of_node_put(np);

	/* find lp_clk max num */
	i = 0;
	while (1) {
		err = snprintf_s(buffer, BUFFER_LENGTH * sizeof(char),
				 (BUFFER_LENGTH - 1) * sizeof(char),
				 "hisilicon, lp_clk%u", i);
		if (err < 0) {
			pr_err("%s: snprintf_s buffer err.\n", __func__);
			ret = err;
			goto err;
		}
		np = of_find_compatible_node(NULL, NULL, buffer);
		if (np == NULL) {
			pr_err("%s[%d]: lp_clk%d not find.\n",
			       __func__, __LINE__, i);
			break;
		}
		ret = of_property_read_u32(np, "bit-num", &bit_num);
		if (ret != 0) {
			pr_err("%s, [%d]no bit-num!\n", __func__, i);
			of_node_put(np);
			goto err;
		}
		g_lp_clk_num += bit_num;
		i++;
		of_node_put(np);
	}
	clk_reg_num = i;
	pr_info("%s: find lp clk num: %u, lp clk reg num: %u\n",
		__func__, g_lp_clk_num, clk_reg_num);

	ret = init_clk_lookup_table(clk_reg_num);
	if (ret != 0) {
		pr_err("%s[%d]: init_clk_lookup_table failed!!\n",
		       __func__, __LINE__);
		goto err;
	}

	pr_info("%s, init clk table success.\n", __func__);

	return ret;

err:
	pr_err("%s init failed.\n", __func__);
	return ret;
}

#ifdef CONFIG_PM_SEC_IO
static int init_sec_io(struct device_node *np)
{
	int ret = 0;
	int i;

	/* find iomg sec reg range */
	g_mg_sec_reg_num = of_property_count_u32_elems(np, "mg-sec-reg");
	if (g_mg_sec_reg_num < 0) {
		pr_err("%s[%d], no mg-sec-reg!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}
	pr_info("%s[%d], find mg-sec-reg num: %d\n",
		__func__, __LINE__, g_mg_sec_reg_num);
	g_mg_reg = kcalloc(g_mg_sec_reg_num, sizeof(void __iomem *),
			   GFP_KERNEL);
	if (g_mg_reg == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	for (i = 0; i < g_mg_sec_reg_num; i++) {
		ret = of_property_read_u32_index(np, "mg-sec-reg",
						 i, (u32 *)&g_mg_reg[i]);
		if (ret != 0) {
			pr_err("%s[%d], i = %d, no mg-sec-reg!\n",
			       __func__, __LINE__, i);
			ret = -ENODEV;
			goto err_free_iomg;
		}
	}

	/* find iocg sec reg range */
	g_cg_sec_reg_num = of_property_count_u32_elems(np, "cg-sec-reg");
	if (g_cg_sec_reg_num < 0) {
		pr_err("%s[%d], no mg-sec-reg!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err_free_iomg;
	}
	pr_info("%s[%d], find cg-sec-reg num: %d\n",
		__func__, __LINE__, g_cg_sec_reg_num);
	g_cg_reg = kcalloc(g_cg_sec_reg_num, sizeof(void __iomem *),
			   GFP_KERNEL);
	if (g_cg_reg == NULL) {
		ret = -ENOMEM;
		goto err_free_iomg;
	}
	for (i = 0; i < g_cg_sec_reg_num; i++) {
		ret = of_property_read_u32_index(np, "cg-sec-reg",
						 i, (u32 *)&g_cg_reg[i]);
		if (ret != 0) {
			pr_err("%s[%d], i = %d, no cg-sec-reg!\n",
			       __func__, __LINE__, i);
			ret = -ENODEV;
			goto err_free_iocg;
		}
	}

	pr_info("%s:%d init sec ioc success!\n", __func__, __LINE__);
	return ret;

err_free_iocg:
	kfree(g_cg_reg);
	g_cg_reg = NULL;
err_free_iomg:
	kfree(g_mg_reg);
	g_mg_reg = NULL;
err:
	pr_err("%s: init fail!\n", __func__);
	return ret;
}
#endif

#ifdef CONFIG_PM_SEC_GPIO
static int init_sec_gpio(struct device_node *np)
{
	int ret = 0;
	int i;

	/* find sec gpio num range */
	g_sec_gpio_num = of_property_count_u32_elems(np, "sec-gpio-no");
	if (g_sec_gpio_num < 0) {
		pr_err("%s[%d], no sec-gpio-no!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}
	pr_info("%s[%d], find sec-gpio-no num: %d\n",
		__func__, __LINE__, g_sec_gpio_num);
	g_sec_gpio_array = kcalloc(g_sec_gpio_num, sizeof(unsigned int),
				   GFP_KERNEL);
	if (g_sec_gpio_array == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	for (i = 0; i < g_sec_gpio_num; i++) {
		ret = of_property_read_u32_index(np, "sec-gpio-no",
						 i, (u32 *)&g_sec_gpio_array[i]);
		if (ret != 0) {
			pr_err("%s[%d], i = %d, no sec-gpio-no!\n",
			       __func__, __LINE__, i);
			ret = -ENODEV;
			goto err_free_gpio;
		}
	}

	pr_info("%s:%d init sec GPIO success!\n", __func__, __LINE__);
	return ret;

err_free_gpio:
	kfree(g_sec_gpio_array);
	g_sec_gpio_array = NULL;
err:
	pr_err("%s: init fail!\n", __func__);
	return ret;
}
#endif
#endif

#ifdef CONFIG_SR_DEBUG_LPREG
static ssize_t dbg_cfg_write(struct file *filp, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	int ret;
	char buf[BUFFER_LENGTH];

	if (buffer == NULL) {
		pr_info("%s: %d buffer kzalloc fail.\n", __func__, __LINE__);
		return -EFAULT;
	}

	if (count >= sizeof(buf)) {
		pr_info("[%s]error, count[%lu]!\n", __func__, count);
		return -EFAULT;
	}

	if (copy_from_user(buf, buffer, count) != 0) {
		pr_info("[%s]error!\n", __func__);
		return -EFAULT;
	}
	buf[count] = '\0';

	ret = kstrtouint(buf, 0, &g_usavedcfg);
	if (ret != 0) {
		pr_err("%s %d, invalid input", __func__, __LINE__);
		return -EINVAL;
	}

	pr_info("%s %d, g_usavedcfg=0x%x\n", __func__, __LINE__, g_usavedcfg);

	/* suspend print enable */
	console_suspend_enabled = ~(g_usavedcfg & DEBG_SUSPEND_PRINTK);
	set_wakelock(!!(g_usavedcfg & DEBG_SUSPEND_WAKELOCK));

	*ppos += count;

	return count;
}

static void cfg_show(struct seq_file *s)
{
	lowpm_msg(s,
		  "0x1<<0 enable suspend console\n"
		  "0x1<<1 ENABLE IO STATUS SHOW\n"
		  "0x1<<2 ENABLE PMU STATUS SHOW\n"
		  "0x1<<8 ENABLE a wakelock\n"
		  "0x1<<10 ENABLE CLK STATUS SHOW\n"
		  "0x1<<31 ENABLE DEBUG INFO\n"
		  "g_usavedcfg=%x", g_usavedcfg);
}

static int dbg_cfg_show(struct seq_file *s, void *unused)
{
	cfg_show(s);
	return 0;
}

static int dbg_cfg_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_cfg_show, &inode->i_private);
}

const struct file_operations dbg_cfg_fops = {
	.owner = THIS_MODULE,
	.open = dbg_cfg_open,
	.read = seq_read,
	.write = dbg_cfg_write,
	.llseek = seq_lseek,
	.release = single_release,
};


static int dbg_pm_status_show(struct seq_file *s, void *unused)
{
	pm_status_show(s);
	return 0;
}

static int dbg_pm_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_pm_status_show, &inode->i_private);
}

static const struct file_operations dbg_pm_status_fops = {
	.owner = THIS_MODULE,
	.open = dbg_pm_status_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int dbg_io_show(struct seq_file *s, void *unused)
{
	io_status_show(s);
	return 0;
}

static int dbg_io_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, dbg_io_show, NULL, DEBUG_LOG_BUF_SIZE);
}

static const struct file_operations dbg_iomux_fops = {
	.open = dbg_io_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int dbg_pmu_show(struct seq_file *s, void *unused)
{
	pmu_status_show(s);
	return 0;
}

static int dbg_pmu_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, dbg_pmu_show, NULL, DEBUG_LOG_BUF_SIZE);
}

static const struct file_operations debug_pmu_fops = {
	.open = dbg_pmu_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int dbg_clk_show(struct seq_file *s, void *unused)
{
	clk_status_show(s);
	return 0;
}

static int dbg_clk_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, dbg_clk_show, NULL, DEBUG_LOG_BUF_SIZE);
}

static const struct file_operations debug_clk_fops = {
	.open = dbg_clk_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int init_lowpm_data(void)
{
	int ret;
	u32 i;
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,lowpm_func");
	if (np == NULL) {
		pr_err("%s: lowpm_func No compatible node found\n",
		       __func__);
		ret = -ENODEV;
		goto err;
	}
#ifdef CONFIG_SR_DEBUG_LPREG
	ret = of_property_read_string(np, "plat-name", &g_plat_name);
	if (ret != 0) {
		pr_err("%s[%d], no plat-name!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err_put_node;
	}
	ret = map_io_regs();
	if (ret != 0) {
		pr_err("%s: %d map_io_regs failed.\n", __func__, __LINE__);
		goto err_put_node;
	}
#ifdef CONFIG_PM_SEC_IO
	ret = init_sec_io(np);
	if (ret < 0) {
		pr_err("%s:%d init sec IO failed!\n", __func__, __LINE__);
		goto err_put_node;
	}
#endif

#ifdef CONFIG_PM_SEC_GPIO
	ret = init_sec_gpio(np);
	if (ret < 0) {
		pr_err("%s:%d init sec GPIO failed!\n", __func__, __LINE__);
		goto err_put_node;
	}
#endif
	ret = init_pmu_table();
	if (ret != 0)
		goto err_put_node;

	ret = init_clk_table();
	if (ret != 0)
		goto err_put_node;
#endif

	ret = of_property_count_strings(np, "lpmcu-irq-table");
	if (ret < 0) {
		pr_err("%s, not find irq-table property!\n", __func__);
		goto err_put_node;
	}

	g_lpmcu_irq_num = ret;
	pr_info("%s, lpmcu-irq-table num: %d!\n", __func__, g_lpmcu_irq_num);

	g_lpmcu_irq_name = kcalloc(g_lpmcu_irq_num, sizeof(char *), GFP_KERNEL);
	if (g_lpmcu_irq_name == NULL) {
		ret = -ENOMEM;
		goto err_put_node;
	}

	for (i = 0; i < g_lpmcu_irq_num; i++) {
		ret = of_property_read_string_index(np, "lpmcu-irq-table",
						    i, &g_lpmcu_irq_name[i]);
		if (ret != 0) {
			pr_err("%s, no lpmcu-irq-table %d!\n", __func__, i);
			goto err_free;
		}
	}

	of_node_put(np);
	pr_info("%s, init lowpm data success.\n", __func__);

	return ret;

err_free:
	kfree(g_lpmcu_irq_name);
	g_lpmcu_irq_name = NULL;
err_put_node:
	of_node_put(np);
err:
	pr_err("%s: init fail!\n", __func__);
	return ret;
}

static int lowpm_func_probe(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	int ret;
#ifdef CONFIG_SR_DEBUG_LPREG
	struct dentry *pdentry = NULL;
#endif

	pr_info("[%s] %d enter.\n", __func__, __LINE__);

	g_suspended = 0;

	ret = map_sysregs();
	if (ret != 0) {
		pr_err("%s: %d map_sysregs failed.\n", __func__, __LINE__);
		goto err;
	}

	ret = init_lowpm_data();
	if (ret != 0) {
		pr_err("%s: %d init_lowpm_data failed.\n", __func__, __LINE__);
		goto err;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	g_lowpm_wake_lock = wakeup_source_register(NULL, "lowpm_func");
#else
	g_lowpm_wake_lock = wakeup_source_register("lowpm_func");
#endif
	if (!g_lowpm_wake_lock) {
		pr_err("%s: wakeup source %s register failed.\n", __func__, "lowpm_func");
		ret = -ENOMEM;
		goto err;
	}

#ifndef CONFIG_SR_DEBUG_SLEEP
	set_wakelock(1);
#endif

#ifdef CONFIG_SR_DEBUG_LPREG
	pdentry = debugfs_create_dir("lowpm_func", NULL);
	if (pdentry == NULL) {
		pr_info("%s %d error can not create debugfs lowpm_func.\n",
			__func__, __LINE__);
		ret = -ENOMEM;
		goto err;
	}

	(void)debugfs_create_file("clk", S_IRUSR, pdentry,
				  NULL, &debug_clk_fops);

	(void)debugfs_create_file("pmu", S_IRUSR, pdentry,
				  NULL, &debug_pmu_fops);

	(void)debugfs_create_file("io", S_IRUSR, pdentry,
				  NULL, &dbg_iomux_fops);

	(void)debugfs_create_file("cfg", S_IRUSR | S_IWUSR, pdentry,
				  NULL, &dbg_cfg_fops);

	(void)debugfs_create_file("debug_sr", S_IRUSR, pdentry,
				  NULL, &dbg_pm_status_fops);
#endif

	np = of_find_compatible_node(NULL, NULL, "hisilicon,lowpm_func");
	if (np == NULL) {
		pr_err("%s: lowpm_func No compatible node found\n",
		       __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32_array(np, "fpga_flag", &g_lp_fpga_flag, 1);
	if (ret != 0)
		pr_err("%s , no fpga_flag.\n", __func__);
	of_node_put(np);

	pr_info("[%s] %d leave.\n", __func__, __LINE__);

	return ret;

err:
	pr_err("%s: fail!\n", __func__);
	return ret;
}

static int lowpm_func_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int lowpm_func_suspend(struct platform_device *pdev,
			      pm_message_t state)
{
	g_suspended = 1;
	return 0;
}

static int lowpm_func_resume(struct platform_device *pdev)
{
	g_suspended = 0;
	return 0;
}
#endif

static const struct of_device_id lowpm_func_match[] = {
	{ .compatible = MODULE_NAME },
	{},
};

static struct platform_driver lowpm_func_drv = {
	.probe = lowpm_func_probe,
	.remove = lowpm_func_remove,
#ifdef CONFIG_PM
	.suspend = lowpm_func_suspend,
	.resume = lowpm_func_resume,
#endif
	.driver = {
		.name = MODULE_NAME,
		.of_match_table = of_match_ptr(lowpm_func_match),
	},
};

static int __init lowpmreg_init(void)
{
	return platform_driver_register(&lowpm_func_drv);
}

static void __exit lowpmreg_exit(void)
{
	platform_driver_unregister(&lowpm_func_drv);
}


module_init(lowpmreg_init);
module_exit(lowpmreg_exit);
