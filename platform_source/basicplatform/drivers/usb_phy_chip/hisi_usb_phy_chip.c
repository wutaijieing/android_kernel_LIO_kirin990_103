/*
 * Copyright (c) 2019-2019 Hisilicon Technologies CO., Ltd.
 * Description: hisi usb phy chip driver support.
 * Author: Xiaodong Guo
 * Create:2019-09-18
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/kern_levels.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <platform_include/see/bl31_smc.h>
#include <linux/platform_drivers/usb_phy_chip.h>
#include <platform_include/basicplatform/linux/hisi_usb_phy_driver.h>
#include <linux/platform_drivers/usb/usb_misc_ctrl.h>

#include <soc_usb_misc_ctrl_interface.h>
#include <soc_ioc_hsdt1_interface.h>

#define HI6502_DO_RESET 1UL
#define HI6502_DO_UNRESET 0UL
#define HI6502_STATE_RESET 0UL
#define HI6502_STATE_UNRESET 1UL

#define UPC_OPS_NUM		2
#define UPC_REG_MIN		0
#define UPC_REG_MAX		1

#define UPC_TEST_FLAG		0
#define UPC_MAGIC_VALUE	0x8421
#define UPC_REG_BASE_START	0x10000
#define UPC_REG_BASE_END	0x1C000
#define UPC_LEN_PER_READL	0x4

#define HIMASK_ENABLE(val)	((val) << 16 | (val))
#define HIMASK_DISABLE(val)	((val) << 16)

#define HISI_UPC_USB_PHY_BASE		0x10000
#define USBPHY_RX_ACTIVE_CFG_BIT	8
#define HISI_USB_PHYREG_SM_DBG		(HISI_UPC_USB_PHY_BASE + 0x220)
#define USBPHY_CLEAR_HS_M_VALUE		0xFFFFFFC0
#define USBPHY_HS_M_VALUE		0x6
#define HISI_USB_PHYREG_RX_SYNC_DET_CFG	(HISI_UPC_USB_PHY_BASE + 0x228)

#define UPC_IOC_BASE				0x13000
#define UPC_IOC_IOCG_034			0x8AC
#define UPC_IOCG_PD_BIT				1

/* IOC HSDT1 */
#define USB2_ULPI_CLK_IOC			0x30
#define USB2_ULPI_DATA0_IOC			0x34
#define USB2_ULPI_DATA1_IOC			0x38
#define USB2_ULPI_DATA2_IOC			0x3c
#define USB2_ULPI_DATA3_IOC			0x40
#define USB2_ULPI_DATA4_IOC			0x44
#define USB2_ULPI_DATA5_IOC			0x48
#define USB2_ULPI_DATA6_IOC			0x4c
#define USB2_ULPI_DATA7_IOC			0x50
#define USB2_ULPI_DIR_IOC			0x54
#define USB2_ULPI_NXT_IOC			0x58
#define USB2_ULPI_STP_IOC			0x5c
#define USB2_ULPI_REF_CLK_IOC			0x64
#define USB2_ULPI_DATA0_PIN_CTRL		0x834
#define USB2_ULPI_DATA1_PIN_CTRL		0x838
#define USB2_ULPI_DATA2_PIN_CTRL		0x83C
#define USB2_ULPI_DATA3_PIN_CTRL		0x840
#define USB2_ULPI_DATA4_PIN_CTRL		0x844
#define USB2_ULPI_DATA5_PIN_CTRL		0x848
#define USB2_ULPI_DATA6_PIN_CTRL		0x84C
#define USB2_ULPI_DATA7_PIN_CTRL		0x850
#define USB2_ULPI_STP_PIN_CTRL			0x85C

#define IO_PARAM_NUM		12
#define IOCG_032		0x888
#define IOCG_DS_START		4
#define IOCG_REG_OFFSET		4
#define IOCG_DS_CFG_MASK	0xFFFFFF8FU

/* IOC_HSDT1 func defination */
#define IOC_HSDT1_USB_FUN			1

#define UPC_SCTRL_BASE		0x15000
#define UPC_SCTRL_TRIM_CTRL	((UPC_SCTRL_BASE)+ 0x20)
#define SC_USBTRIM_OVRD_EN	BIT(12)
#define SC_RG_RPT_TRIM_MASK	0x1F
#define SC_RG_RPT_TRIM_SHIFT	0
#define SC_RG_RT_TRIM_MASK	0x3E0
#define SC_RG_RT_TRIM_SHIFT	5

#define err(fmt, args ...) pr_err("[HI6502]"fmt, ##args)
#define info(fmt, args ...) pr_info("[HI6502]"fmt, ##args)
#define dbg(fmt, args ...) pr_info("[HI6502]"fmt, ##args)

struct usb_phy_chip_device {
	bool ioc_func_cfg_stub;
	bool is_ulpi_ioc_config;
	unsigned int module_stub;
	unsigned int usb_misc_need_rst;
	unsigned int i2c_bus;
	unsigned int i2c_slaveid;
	unsigned int upc_chip_type_addr;
	unsigned int ulpi_iocg_val[IO_PARAM_NUM];
	unsigned int gpio_clk_sel[UPC_OPS_NUM];
	unsigned int usb_trim;
	unsigned int usb_host_trim;
	struct clk *gt_clk_upc_38p4;
	struct platform_device *pdev;
	struct i2c_adapter *upc_i2c_adap;
	struct i2c_client *upc_i2c_client;
	void __iomem *rst_base;
	void __iomem *ioc_base;
};

static DEFINE_MUTEX(upc_sr_lock);
static DEFINE_MUTEX(upc_open_lock);

static const struct i2c_client *g_upc_i2c_client;
static unsigned int g_upc_chip_type_addr;
static unsigned int g_upc_reg_limit[UPC_OPS_NUM];
static unsigned int g_module_stub;
static unsigned int g_upc_inited;
static struct usb_phy_chip_device *g_upc_device;
static unsigned int g_upc_chip_type = UNKNOWN_CHIP;
/* usb_phy_chip opened in xloader, default status is open */
static int g_upc_enable_cnt = 1;
static bool g_upc_sr_status = false;
/* USB PHY CHIP i2c information, addr are init in probe func */
static struct i2c_board_info g_upc_i2c_board_info = {
	.type = "i2c_usb_phy_chip",
	.addr = 0,
};

void _hi6502_readl_u32(unsigned int *val, unsigned int addr)
{
	unsigned int temp;
	int ret;
	struct i2c_msg msgs[2]; /* addr, val */

#if UPC_TEST_FLAG
	err("%s: val [0x%x], addr [0x%x]\n", __func__, *val, addr);
	*val = UPC_MAGIC_VALUE;
#endif
	if (g_module_stub)
		return;

	if (g_upc_i2c_client == NULL || addr < g_upc_reg_limit[UPC_REG_MIN] ||
		addr > g_upc_reg_limit[UPC_REG_MAX]) {
		info("%s input is null\n", __func__);
		return;
	}

	temp = cpu_to_be32(addr);
	msgs[0].addr = g_upc_i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].len = (int)sizeof(unsigned int);
	msgs[0].buf = (char *)(&temp);

	msgs[1].addr = g_upc_i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = (int)sizeof(unsigned int);
	msgs[1].buf = (char *)(&temp);

	ret = i2c_transfer(g_upc_i2c_client->adapter, msgs, 2);
	if (ret < 0) {
		*val = 0;
		err("%s transfer err: val [0x%x], addr [0x%x]\n", __func__, *val, addr);
		return;
	}
	*val = temp;
}

void hi6502_readl_u32(unsigned int *val, unsigned int addr)
{
	/* wait hi6502 resume */
	mutex_lock(&upc_sr_lock);
	_hi6502_readl_u32(val, addr);
	mutex_unlock(&upc_sr_lock);
}

void upc_dump_reg(unsigned int start_addr, unsigned int len)
{
	unsigned int i;
	unsigned int val = 0;

	if (start_addr < UPC_REG_BASE_START || start_addr >= UPC_REG_BASE_END
		|| (start_addr + len) > UPC_REG_BASE_END) {
		err("%s, input err!\n", __func__);
		return;
	}
	for (i = 0; i <= len; i = i + UPC_LEN_PER_READL)
		_hi6502_readl_u32(&val, start_addr + i);
}

void _hi6502_writel_u32(unsigned int val, unsigned int addr)
{
	int ret;
	struct i2c_msg msgs;
	unsigned int temp[2]; /* addr, val */

#if UPC_TEST_FLAG
	err("%s: val [0x%x], addr [0x%x]\n", __func__, val, addr);
#endif

	if (g_module_stub)
		return;

	if (g_upc_i2c_client == NULL || addr < g_upc_reg_limit[UPC_REG_MIN] ||
		addr > g_upc_reg_limit[UPC_REG_MAX]) {
		info("%s input is null!\n", __func__);
		return;
	}

	temp[0] = cpu_to_be32(addr);
	temp[1] = val;

	msgs.addr = g_upc_i2c_client->addr;
	msgs.flags = 0;
	msgs.len = (int)sizeof(temp);
	msgs.buf = (char *)&temp[0];
	ret = i2c_transfer(g_upc_i2c_client->adapter, &msgs, 1);
	if (ret < 0)
		err("%s transfer err: val [0x%x], addr [0x%x]\n", __func__, val, addr);
}

void hi6502_writel_u32(unsigned int val, unsigned int addr)
{
	/* wait hi6502 resume */
	mutex_lock(&upc_sr_lock);
	_hi6502_writel_u32(val, addr);
	mutex_unlock(&upc_sr_lock);
}

unsigned int hi6502_chip_type(void)
{
	unsigned int val = UNKNOWN_CHIP;

	if (g_module_stub)
		return UNKNOWN_CHIP;

	if (g_upc_chip_type != UNKNOWN_CHIP)
		return g_upc_chip_type;

	_hi6502_readl_u32(&val, g_upc_chip_type_addr);

	g_upc_chip_type = val;
	return g_upc_chip_type;
}

int hi6502_test_read(unsigned int addr)
{
	unsigned int temp = 0;

	hi6502_readl_u32(&temp, addr);
	err("%s: read_value is 0x%x\n", __func__, temp);
	return temp;
}

bool hi6502_is_suspend(void)
{
	return g_upc_sr_status;
}

bool hi6502_is_ready(void)
{
	if (g_upc_inited)
		return true;
	else
		return false;
}

#define USB_MISC_CTRL_SIZE	0x200
void __iomem *g_upc_usb_misc_ctrl = NULL;
static void __iomem *get_usb_misc_ctrl(void)
{
	struct device_node *np = NULL;
	void __iomem *usb_misc_ctrl = NULL;
	/*
	 * map USB MISC CTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,usb-misc-ctrl");
	if (!np) {
		err("get usb misc ctrl node failed!\n");
		return NULL;
	}

	usb_misc_ctrl = of_iomap(np, 0);
	if (usb_misc_ctrl == NULL) {
		err("iomap usb misc ctrl failed!\n");
		return NULL;
	}
	return usb_misc_ctrl;
}

static void __iomem *get_hsdt1_ioc_base(void)
{
	struct device_node *np = NULL;
	void __iomem *ioc_base = NULL;
	/*
	 * map HSDT1 IOC CTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,ioc-hsdt1-usb");
	if (!np) {
		err("get ioc-hsdt1-usb node failed!\n");
		return NULL;
	}

	ioc_base = of_iomap(np, 0);
	if (ioc_base == NULL) {
		err("iomap usb misc ctrl failed!\n");
		return NULL;
	}
	return ioc_base;
}

static void set_upc_ioc_func(void __iomem *ioc_base, bool not_cfg)
{
	unsigned int tmp;

	if (ioc_base == NULL)
		return;

	/* emu & fpga should not cfg hsdt1 ioc */
	if (not_cfg)
		return;

	tmp = readl(SOC_IOC_HSDT1_IOMG_024_ADDR(ioc_base));
	tmp = tmp | BIT(SOC_IOC_HSDT1_IOMG_024_iomg_024_START);
	writel(tmp, SOC_IOC_HSDT1_IOMG_024_ADDR(ioc_base));
}

#define USB_IOC_CLEAR_BIT(val)	(~((val) >> 16))
#define USB_IOC_SET_BIT(val)	((val) & 0x0000FFFF)
#define USB_IOC_REG_INTERVAL	0x4
void upc_set_usb_ioc(unsigned int snopsys_data, unsigned int snopsys_stp,
	unsigned int hisi_data, unsigned int hisi_stp)
{
	unsigned int phy_type;
	unsigned int io_data_force;
	unsigned int io_stp_force;
	unsigned int reg_temp;
	void __iomem *addr_temp = NULL;

	if (WARN_ON(g_upc_device == NULL))
		return;

	if (g_upc_device->module_stub) {
		info("%s module stub\n", __func__);
		return;
	}

	/*
	 * set ioc func, reg interval is 0x4
	 * ULPI_DATA0 ~ ULPI_DATA7、ULPI_DIR、ULPI_NXT、ULPI_STP
	 */
	for (addr_temp = g_upc_device->ioc_base + USB2_ULPI_CLK_IOC;
		addr_temp <= g_upc_device->ioc_base + USB2_ULPI_STP_IOC;
		addr_temp = addr_temp + USB_IOC_REG_INTERVAL)
		writel(IOC_HSDT1_USB_FUN, addr_temp);

	/* set ULPI_REF_CLK ioc func */
	if (!g_upc_device->ioc_func_cfg_stub)
		writel(IOC_HSDT1_USB_FUN, g_upc_device->ioc_base + USB2_ULPI_REF_CLK_IOC);

	phy_type = hi6502_chip_type();
	if (phy_type == SNOPSYS_USB_PHY_CHIP) {
		io_data_force = snopsys_data;
		io_stp_force = snopsys_stp;
	} else if (phy_type == HISI_USB_PHY_CHIP) {
		io_data_force = hisi_data;
		io_stp_force = hisi_stp;
	} else {
		info("%s: unknown usb phy chip\n", __func__);
		return;
	}

	/* ULPI_DATA0 ~ ULPI_DATA7 driving force cfg, reg interval is 0x4 */
	for (addr_temp = g_upc_device->ioc_base + USB2_ULPI_DATA0_PIN_CTRL;
		addr_temp <= g_upc_device->ioc_base + USB2_ULPI_DATA7_PIN_CTRL;
		addr_temp = addr_temp + USB_IOC_REG_INTERVAL) {
		reg_temp = readl(addr_temp);
		reg_temp = (reg_temp & USB_IOC_CLEAR_BIT(io_data_force)) |
			USB_IOC_SET_BIT(io_data_force);
		writel(reg_temp, addr_temp);
	}

	/* ULPI_STP driving force cfg */
	reg_temp = readl(g_upc_device->ioc_base + USB2_ULPI_STP_PIN_CTRL);
	reg_temp = (reg_temp & USB_IOC_CLEAR_BIT(io_stp_force)) |
		USB_IOC_SET_BIT(io_stp_force);
	writel(reg_temp, g_upc_device->ioc_base + USB2_ULPI_STP_PIN_CTRL);
}

/* only used by temporary ST USB PHY & Bal ES */
void usb_misc_ulpi_reset(void)
{
	void __iomem *base_addr = g_upc_usb_misc_ctrl;
	unsigned int tmp;

	if (base_addr == NULL) {
		err("%s failed!\n", __func__);
		return;
	}

	if (WARN_ON(g_upc_device == NULL))
		return;

	if (!g_upc_device->usb_misc_need_rst)
		return;

	/* chip reset, ulpi_reset low valid */
	tmp = readl(SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr));
	tmp = tmp & (~BIT(SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ulpi_reset_START));
	writel(tmp, SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr));

	info("%s [0x%lx] = 0x%x\n", __func__, SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr),
		readl(SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr)));
}

/* only used by temporary ST USB PHY & Bal ES */
void usb_misc_ulpi_unreset(void)
{
	void __iomem *base_addr = g_upc_usb_misc_ctrl;
	unsigned int tmp;

	if (base_addr == NULL) {
		err("%s failed!\n", __func__);
		return;
	}

	if (WARN_ON(g_upc_device == NULL))
		return;

	if (!g_upc_device->usb_misc_need_rst)
		return;

	/* chip unreset, ulpi_reset low valid */
	tmp = readl(SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr));
	tmp = tmp | BIT(SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ulpi_reset_START);
	writel(tmp, SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr));

	info("%s [0x%lx] = 0x%x\n", __func__, SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr),
		readl(SOC_USB_MISC_CTRL_sc_usb20phy_misc_ctrl6_ADDR(base_addr)));
}

static void fix_usb_transfer_disconnect(void)
{
	uint32_t tmp = 0;
	uint32_t phy_type = hi6502_chip_type();
	if (phy_type != HISI_USB_PHY_CHIP)
		return;
	/* set rx_active_cfg not pullup */
	_hi6502_readl_u32(&tmp, HISI_USB_PHYREG_SM_DBG);
	tmp = tmp | BIT(USBPHY_RX_ACTIVE_CFG_BIT);
	_hi6502_writel_u32(tmp, HISI_USB_PHYREG_SM_DBG);

	/* set HS SYNC detect value-8 */
	_hi6502_readl_u32(&tmp, HISI_USB_PHYREG_RX_SYNC_DET_CFG);
	tmp = (tmp & USBPHY_CLEAR_HS_M_VALUE) | USBPHY_HS_M_VALUE;
	_hi6502_writel_u32(tmp, HISI_USB_PHYREG_RX_SYNC_DET_CFG);
}

static void upc_ulpi_ioc_config(void)
{
	unsigned int i;
	uint32_t tmp = 0;
	uint32_t phy_type = hi6502_chip_type();
	/* ulpi ioc driving force config, only s28 need config */
	if (phy_type != HISI_USB_PHY_CHIP)
		return;

	if (g_upc_device->is_ulpi_ioc_config) {
		for (i = 0; i < IO_PARAM_NUM; i++) {
			_hi6502_readl_u32(&tmp,
				UPC_IOC_BASE + IOCG_032 + IOCG_REG_OFFSET * i);
			tmp = tmp & IOCG_DS_CFG_MASK;
			tmp = tmp | (g_upc_device->ulpi_iocg_val[i] << IOCG_DS_START);
			_hi6502_writel_u32(tmp,
				UPC_IOC_BASE + IOCG_032 + IOCG_REG_OFFSET * i);
		}
	}
}

static void usb_reg_restore(void)
{
	fix_usb_transfer_disconnect();
	upc_remove_pw_burr_enable(UPC_BURR_PW_OFF, UPC_BURR_RESUME_RESTORE);
	upc_ulpi_ioc_config();
}

static unsigned long hi6502_smc_set_reset(uint64_t reset)
{
	register uint64_t r0 __asm__("x0") = (uint64_t)FID_USB_SET_PHY_RESET;
	register uint64_t r1 __asm__("x1") = reset;

	__asm__ volatile(
			"smc #0"
			: /* Output registers, also used as inputs ('+' constraint). */
			"+r"(r0), "+r"(r1)
			);

	return r0;
}

static unsigned long hi6502_smc_get_reset(uint64_t *reset)
{
	register uint64_t r0 __asm__("x0") = (uint64_t)FID_USB_GET_PHY_RESET;
	register uint64_t r1 __asm__("x1") = 0;

	__asm__ volatile(
			"smc #0"
			: /* Output registers, also used as inputs ('+' constraint). */
			"+r"(r0), "+r"(r1)
			);

	if (r0 == 0)
		*reset = r1;
	return r0;
}

void upc_open(void)
{
	int ret;

	mutex_lock(&upc_open_lock);

	if (WARN_ON(g_upc_device == NULL))
		goto out;

	if (g_upc_device->module_stub) {
		info("%s module stub\n", __func__);
		goto out;
	}

	if (g_upc_enable_cnt == 0) {
		/* open usb phy chip out clk */
		ret = clk_prepare_enable(g_upc_device->gt_clk_upc_38p4);
		if (ret)
			err("clk_prepare_enable gt_clk_usb2phy_ref failed\n");

		/* set IO func to ulpi rst, default is GPIO func */
		set_upc_ioc_func(g_upc_device->ioc_base,
			g_upc_device->ioc_func_cfg_stub);

		/* unrst usb phy chip */
		if (g_upc_device->usb_misc_need_rst) {
			misc_ctrl_init();
			usb_misc_ulpi_unreset();
			info("usb misc ctrl init & ulpi unrst\n");
		} else {
			uint64_t reset_state = HI6502_STATE_RESET;

			if (hi6502_smc_set_reset(HI6502_DO_UNRESET))
				err("hi6502_smc_set_reset failed\n");
			if (hi6502_smc_get_reset(&reset_state))
				err("hi6502_smc_get_reset failed\n");

			info("PCTRL upc chip reset state %llu\n",
				 reset_state);
		}

		/* sel gpio clk to 38.4Mhz */
		_hi6502_writel_u32(HIMASK_DISABLE(BIT(g_upc_device->gpio_clk_sel[1])),
				   g_upc_device->gpio_clk_sel[0]);

		/* wait 5 * (1s / 32Khz) = 150us */
		udelay(150);

		usb_reg_restore();
	}

	/* fresh open cnt */
	g_upc_enable_cnt++;

out:
	info("%s cur upc enable cnt is %d\n", __func__, g_upc_enable_cnt);
	mutex_unlock(&upc_open_lock);
}

void upc_close(void)
{
	unsigned int temp = 0;

	mutex_lock(&upc_open_lock);

	if (WARN_ON(g_upc_device == NULL))
		goto out;

	if (g_upc_device->module_stub) {
		info("%s module stub\n", __func__);
		goto out;
	}

	if (WARN_ON(g_upc_enable_cnt == 0))
		goto out;

	/* fresh open cnt */
	if (--g_upc_enable_cnt > 0)
		goto out;

	/* set usb dir io pd invalid to reduce power consumption */
	_hi6502_readl_u32(&temp, UPC_IOC_BASE + UPC_IOC_IOCG_034);
	temp = temp & (~BIT(UPC_IOCG_PD_BIT));
	_hi6502_writel_u32(temp, UPC_IOC_BASE + UPC_IOC_IOCG_034);

	/* sel gpio clk to 32Khz */
	_hi6502_writel_u32(HIMASK_ENABLE(BIT(g_upc_device->gpio_clk_sel[1])),
		g_upc_device->gpio_clk_sel[0]);

	/* wait 5 * (1s / 32Khz) = 150us */
	udelay(150);

	/* rst usb phy chip */
	if (g_upc_device->usb_misc_need_rst) {
		usb_misc_ulpi_reset();
		misc_ctrl_exit();
		info("usb misc ctrl exit \n");
	} else {
		uint64_t reset_state = HI6502_STATE_RESET;

		if (hi6502_smc_set_reset(HI6502_DO_RESET))
			err("hi6502_smc_set_reset failed\n");
		if (hi6502_smc_get_reset(&reset_state))
			err("hi6502_smc_get_reset failed\n");
		info("PCTRL upc chip reset state %llu\n",
			 reset_state);
	}

	/* disable usb phy chip out clk */
	clk_disable_unprepare(g_upc_device->gt_clk_upc_38p4);

out:
	info("%s cur upc enable cnt is %d\n", __func__, g_upc_enable_cnt);
	mutex_unlock(&upc_open_lock);
}

void override_usb_trim(enum phy_mode mode)
{
	uint32_t phy_type, trim;
	uint32_t tmp = 0;

	if (!g_upc_device)
		return;

	if (mode == PHY_MODE_USB_HOST)
		trim = g_upc_device->usb_host_trim;
	else
		trim = g_upc_device->usb_trim;
	if (!trim)
		return;

	phy_type = hi6502_chip_type();
	if (phy_type != HISI_USB_PHY_CHIP)
		return;

	info("%s:set usb trim 0x%x\n", __func__, trim);
	_hi6502_readl_u32(&tmp, UPC_SCTRL_TRIM_CTRL);
	tmp &= ~(SC_RG_RPT_TRIM_MASK | SC_RG_RT_TRIM_MASK);
	tmp |= (SC_USBTRIM_OVRD_EN |
		(unsigned int)(trim << SC_RG_RPT_TRIM_SHIFT) |
		(unsigned int)(trim << SC_RG_RT_TRIM_SHIFT));
	_hi6502_writel_u32(tmp, UPC_SCTRL_TRIM_CTRL);
}

static void upc_get_ioc_config(struct usb_phy_chip_device *upc,
	struct device *dev, struct device_node *node)
{
	int ret;
	int i;
	const char *g_hi6502_iocg_name[IO_PARAM_NUM] = {
		"usb2_ulpi_clk",
		"usb2_ulpi_data0",
		"usb2_ulpi_data1",
		"usb2_ulpi_data2",
		"usb2_ulpi_data3",
		"usb2_ulpi_data4",
		"usb2_ulpi_data5",
		"usb2_ulpi_data6",
		"usb2_ulpi_data7",
		"usb2_ulpi_dir",
		"usb2_ulpi_nxt",
		"usb2_ulpi_stp"
	};

	upc->ioc_func_cfg_stub = of_property_read_bool(node, "ioc-func-cfg-stub");
	info("chip ioc func cfg stub %d\n", upc->ioc_func_cfg_stub);

	for (i = 0; i < IO_PARAM_NUM; i++) {
		ret = of_property_read_u32(node, g_hi6502_iocg_name[i],
			&(upc->ulpi_iocg_val[i]));
		if (ret) {
			info("%s has no tag %s\n",
				__func__, g_hi6502_iocg_name[i]);
			return;
		}
	}
	upc->is_ulpi_ioc_config = true;
}

static int upc_parse_dts_config(struct device_node *node, struct usb_phy_chip_device *upc)
{
	int ret;

	ret = of_property_read_u32(node, "usb_phy_chip_stub", &upc->module_stub);
	if (ret) {
		info("%s has no tag <usb_phy_chip_stub>\n", __func__);
		upc->module_stub = 1;
	}

	ret = of_property_read_u32(node, "usb_misc_need_rst", &upc->usb_misc_need_rst);
	if (ret) {
		err("%s has error tag <usb_misc_need_rst>\n", __func__);
		goto out;
	}

	ret = of_property_read_u32(node, "upc_i2c_bus", &upc->i2c_bus);
	if (ret) {
		err("%s has no tag <upc_i2c_bus>\n", __func__);
		goto out;
	}

	ret = of_property_read_u32(node, "upc_i2c_slaveid", &upc->i2c_slaveid);
	if (ret) {
		err("%s has no tag <upc_i2c_slaveid>\n", __func__);
		goto out;
	}

	ret = of_property_read_u32(node, "upc_chip_type_addr", &upc->upc_chip_type_addr);
	if (ret) {
		err("%s has no tag <upc_chip_type_addr>\n", __func__);
		goto out;
	}

	ret = of_property_read_u32_array(node, "upc_gpio_clk_sel", upc->gpio_clk_sel, UPC_OPS_NUM);
	if (ret) {
		err("%s has no tag <upc_gpio_clk_sel>\n", __func__);
		goto out;
	}

	ret = of_property_read_u32_array(node, "upc_reg_limit", g_upc_reg_limit, UPC_OPS_NUM);
	if (ret) {
		err("%s has no tag <upc_reg_limit>\n", __func__);
		goto out;
	}

	if (of_property_read_u32(node, "usb-trim", &upc->usb_trim)) {
		info("%s has no tag <usb-trim>\n", __func__);
		upc->usb_trim = 0;
	}

	if (of_property_read_u32(node, "usb-host-trim", &upc->usb_host_trim)) {
		info("%s has no tag <usb-host-trim>\n", __func__);
		upc->usb_host_trim = 0;
	}

out:
	return ret;
}

static int upc_get_dts_config(struct usb_phy_chip_device *upc,
	struct device *dev, struct device_node *node)
{
	int ret;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client = NULL;

	upc_get_ioc_config(upc, dev, node);

	ret = upc_parse_dts_config(node, upc);
	if (ret) {
		err("%s upc parse dts fail\n", __func__);
		goto out;
	}

	g_upc_i2c_board_info.addr = upc->i2c_slaveid;

	if (upc->module_stub) {
		err("%s: module_stub is open\n", __func__);
		g_module_stub = upc->module_stub;
		return 0;
	}

	adapter = i2c_get_adapter(upc->i2c_bus);
	if (adapter == NULL) {
		err("%s get i2c adapter err!\n", __func__);
		ret = -EPROBE_DEFER;
		goto out;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	client = i2c_new_client_device(adapter, &g_upc_i2c_board_info);
#else
	client = i2c_new_device(adapter, &g_upc_i2c_board_info);
#endif
	if (client == NULL) {
		err("%s get i2c client err!\n", __func__);
		ret = -EPROBE_DEFER;
		goto out;
	}

	upc->upc_i2c_adap = adapter;
	upc->upc_i2c_client = client;
	g_upc_i2c_client = client;
	g_upc_chip_type_addr = upc->upc_chip_type_addr;
out:
	return ret;
}

static int init_phy_chip_device(struct platform_device *pdev, struct usb_phy_chip_device *upc)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	void __iomem *chip_rst_base = NULL;
	int ret;

	upc->gt_clk_upc_38p4 = devm_clk_get(dev, "upc_38p4m");
	if (IS_ERR_OR_NULL(upc->gt_clk_upc_38p4)) {
		err("get upc clk error\n");
		return -EINVAL;
	}

	ret = upc_get_dts_config(upc, dev, node);
	if (ret) {
		err("%s get dts config error, please check dts\n", __func__);
		return ret;
	}

	chip_rst_base = of_iomap(node, 0);
	if (!chip_rst_base) {
		err("%s iomap error\n", __func__);
		return -EINVAL;
	}
	upc->rst_base = chip_rst_base;

	ret = clk_prepare_enable(upc->gt_clk_upc_38p4);
	if (ret) {
		err("clk_prepare_enable gt_clk_usb2phy_ref failed\n");
		goto fail_clk_enable;
	}

	/* set IO func to ulpi rst, default is GPIO func */
	upc->ioc_base = get_hsdt1_ioc_base();
	set_upc_ioc_func(upc->ioc_base, upc->ioc_func_cfg_stub);
	return 0;

fail_clk_enable:
	iounmap(upc->rst_base);

	return ret;
}


static int usb_phy_chip_probe(struct platform_device *pdev)
{
	struct usb_phy_chip_device *upc = NULL;
	int ret;

	if (!misc_ctrl_is_ready()) {
		err("misc ctrl is not ready\n");
		return -EPROBE_DEFER;
	}

	dbg("%s ++\n", __func__);
	if (!(pdev->dev.of_node)) {
		err("dts[%s] node not found\n", "hisilicon,usb_phy_chip");
		return -ENODEV;
	}

	upc = devm_kzalloc(&pdev->dev, sizeof(*upc), GFP_KERNEL);
	if (!upc) {
		err("no mem for usb phy chip device\n");
		return -ENOMEM;
	}

	ret = init_phy_chip_device(pdev, upc);
	if (ret)
		return ret;

	g_upc_device = upc;

	g_upc_usb_misc_ctrl = get_usb_misc_ctrl();
	if (upc->usb_misc_need_rst) {
		misc_ctrl_init();
		usb_misc_ulpi_unreset();
		info("usb misc ctrl init & ulpi unrst\n");
	}

	/* set module init flag as 1 */
	g_upc_inited = 1;

	platform_set_drvdata(pdev, upc);

	/* restore hi6502 reg config */
	fix_usb_transfer_disconnect();
	upc_ulpi_ioc_config();

	dbg("%s --\n", __func__);
	return 0;
}

static int usb_phy_chip_remove(struct platform_device *pdev)
{
	dbg("%s ++\n", __func__);
	platform_set_drvdata(pdev, NULL);
	g_upc_device = NULL;
	dbg("%s --\n", __func__);
	return 0;
}

static int usb_phy_chip_suspend(struct device *dev)
{
	info("%s: suspend +", __func__);
	mutex_lock(&upc_sr_lock);
	upc_close();
	upc_disable_vbus_power();
	g_upc_sr_status = true;
	info("%s: suspend -", __func__);
	return 0;
}

static int usb_phy_chip_resume(struct device *dev)
{
	info("%s: resume +", __func__);
	upc_open();
	g_upc_sr_status = false;
	mutex_unlock(&upc_sr_lock);
	info("%s: resume -", __func__);
	return 0;
}

static const struct of_device_id usb_phy_chip_of_match[] = {
	{ .compatible = "hisilicon,usb_phy_chip", },
	{},
};

MODULE_DEVICE_TABLE(of, hisi_ckm_of_match);

#ifdef CONFIG_PM
static const struct dev_pm_ops usb_phy_chip_pm_ops = {
	.suspend = usb_phy_chip_suspend,
	.resume = usb_phy_chip_resume,
};
#endif

static struct platform_driver usb_phy_chip_driver = {
	.probe = usb_phy_chip_probe,
	.remove = usb_phy_chip_remove,
	.driver = {
			   .name = "usb-phy-chip",
			   .owner = THIS_MODULE,
			   .of_match_table = of_match_ptr(usb_phy_chip_of_match),
			   .pm = &usb_phy_chip_pm_ops,
		},
};

static int __init hisi_usb_phy_chip_init(void)
{
	return platform_driver_register(&usb_phy_chip_driver);
}
subsys_initcall_sync(hisi_usb_phy_chip_init);

static void __exit hisi_usb_phy_chip_exit(void)
{
	platform_driver_unregister(&usb_phy_chip_driver);
}
module_exit(hisi_usb_phy_chip_exit);

MODULE_AUTHOR("HISILICON");
MODULE_DESCRIPTION("usb phy chip driver");
MODULE_LICENSE("GPL");
