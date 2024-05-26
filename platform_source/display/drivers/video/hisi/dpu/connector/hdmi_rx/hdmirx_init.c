/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "hdmirx_init.h"
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>

#include "hisi_connector_utils.h"
#include "hdmirx_struct.h"
#include "hdcp/hdmirx_hdcp.h"
#include "hdcp/hdmirx_hdcp_tee.h"
#include "control/hdmirx_ctrl.h"
#include "phy/hdmirx_phy.h"
#include "link/hdmirx_link_hpd.h"
#include "link/hdmirx_link_edid.h"
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "hdmirx_common.h"
#include "control/hdmirx_ctrl_irq.h"
#include "video/hdmirx_video.h"
#include "packet/hdmirx_packet.h"
#include "link/hdmirx_link_frl.h"
#include "video/hdmirx_video_dsc.h"

static struct hdmirx_chr_dev_st *g_dpu_hdmirx_dev;
static struct task_struct *g_hdmirx_main_task = NULL;
static bool g_start_display_hdmi_ready = false;
static bool g_start_display_dss_ready = false;
static bool g_start_display = false;
static bool g_hotplug_flag = false;

struct hdmirx_ctrl_st *hdmirx_get_dev_ctrl(void)
{
	if (g_dpu_hdmirx_dev == NULL) {
		disp_pr_info("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return NULL;
	}

	return &(g_dpu_hdmirx_dev->hdmirx_ctrl);
}

uint32_t hdmirx_get_layer_fmt(void)
{
	struct hdmirx_ctrl_st *hdmirx = hdmirx_get_dev_ctrl();

	return hdmirx_ctrl_get_layer_fmt(hdmirx);
}

void hdmirx_yuv422_set(uint32_t enable)
{
	struct hdmirx_ctrl_st *hdmirx = hdmirx_get_dev_ctrl();

	hdmirx_ctrl_yuv422_set(hdmirx, enable);
}
EXPORT_SYMBOL_GPL(hdmirx_yuv422_set);

void hdmirx_display_dss_ready(void)
{
	if (!g_start_display_dss_ready)
		disp_pr_info("start display dss ready\n");

	g_start_display_dss_ready = true;
}

void hdmirx_display_hdmi_ready(void)
{
	if (!g_start_display_hdmi_ready)
		disp_pr_info("start display hdmi ready\n");

	g_start_display_hdmi_ready = true;
}

bool hdmirx_display_is_ready(struct hdmirx_ctrl_st *hdmirx)
{
	static int cnt = 0;

	cnt++;
	if (cnt >= 100) {
		disp_pr_info("start display status %d %d %d\n",
			g_start_display_dss_ready, g_start_display_hdmi_ready, hdmirx_ctrl_get_sys_mute(hdmirx));
		cnt = 0;
	}

	return (g_start_display_dss_ready &&
			g_start_display_hdmi_ready);
}

static int hdmirx_hsdt1_dprx_reset(struct hdmirx_ctrl_st *hdmirx)
{
	// reset HDMI
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x80, 1, 1, 15);

	disp_pr_info("[hdmirx] dprx reset success\n");

	return 0;
}

static int hdmirx_hsdt1_dprx_unreset(struct hdmirx_ctrl_st *hdmirx)
{
	// unreset HDMI APB rst¡¢ por reset¡¢ 24 clk rst¡¢ djtag rst
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x84, 1, 1, 15);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x84, 1, 1, 16);

	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x84, 1, 1, 17);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x84, 1, 1, 18);

	disp_pr_info("[hdmirx] dprx unreset success\n");

	return 0;
}

static int hdmirx_hsdt1_crg_set(struct hdmirx_ctrl_st *hdmirx)
{
	unsigned int value;
	hdmirx_hsdt1_dprx_unreset(hdmirx);

	disp_pr_info("[hdmirx] hsdt1 crg set\n");

	// APB clock
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x10, 1, 1, 2);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x54, 1, 1, 22);

	// HDMI Media Clk, AON Clk 24Mhz, link, ref djtag
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x30, 1, 1, 23);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x30, 1, 1, 24);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x30, 1, 1, 26);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x30, 1, 1, 28);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x30, 1, 1, 29);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x30, 1, 1, 30);

	// PLL5 525MHz clock for media.
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 2, 2, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 1, 6, 2);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 0x36, 12, 8);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 0x7, 3, 20);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 0x3, 3, 23);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x624, 0xb00000, 25, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x648, 0x8c000004, 32, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x64c, 0x00b40000, 32, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x650, 0x20100fa8, 32, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x654, 0x2404ff20, 32, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x658, 0x0004013f, 32, 0);
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x65c, 0x00004046, 32, 0);

	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 1, 1, 0);
	mdelay(5);
	value = inp32(hdmirx->hdmirx_hsdt1_crg_base + 0x620);
	if (!(value & (1 << 26))) {
		disp_pr_err("[hdmirx] hsdt1 crg set fail\n");
		return -1;
	}
	set_reg(hdmirx->hdmirx_hsdt1_crg_base + 0x620, 0, 1, 1);

	disp_pr_info("[hdmirx] hsdt1 crg set success\n");

	return 0;
}

static int hdmirx_ddc_i2c_slaver_set(struct hdmirx_ctrl_st *hdmirx)
{
	set_reg(hdmirx->hdmirx_ioc_base + 0x68, 1, 32, 0);
	set_reg(hdmirx->hdmirx_ioc_base + 0x6C, 1, 32, 0);
	set_reg(hdmirx->hdmirx_ioc_base + 0x868, 0x28, 32, 0);
	set_reg(hdmirx->hdmirx_ioc_base + 0x86C, 0x28, 32, 0);

	disp_pr_info("[hdmirx] hdmirx_ddc_i2c_slaver_set success\n");

	return 0;
}

static int hdmirx_edid_init(struct hdmirx_ctrl_st *hdmirx)
{
	return hdmirx_link_edid_init(hdmirx);
}

static void hdmirx_fpga_soft_reboot(struct hdmirx_ctrl_st *hdmirx)
{
	if (!g_hotplug_flag) {
		disp_pr_info("[hdmirx]only fpga hdmirx hotplug need soft reboot\n");
		return;
	}

	// FPGA soft reset
	set_reg(hdmirx->hdmirx_pwd_base + 0x0020, 1, 1, 0);
	udelay(100);
	set_reg(hdmirx->hdmirx_pwd_base + 0x0020, 0, 1, 0);

	disp_pr_info("[hdmirx]hdmirx fpga soft reboot\n");
}

static int hdmirx_main_task_thread(void *p)
{
	struct hdmirx_ctrl_st *hdmirx = (struct hdmirx_ctrl_st *)p;
	uint32_t phy_status = 0;
	bool phy_init = false;
	uint32_t cnt = 0;

	if (hdmirx == NULL) {
		disp_pr_err("[hdmirx]hdmirx null\n");
		return -1;
	}

	while (!kthread_should_stop()) {
		msleep(100);

		// for asic,hpd shall be after phy init
		if (!phy_init) {
			mdelay(4000);
			disp_pr_info("[hdmirx]init phy\n");
			hdmirx_phy_init(hdmirx);
			phy_init = true;
		}

		if (phy_status != 0x3f8) {
			cnt++;
			phy_status = inp32(hdmirx->hdmirx_sysctrl_base + 0x084);
			if (phy_status == 0x3f8) {
				hdmirx_fpga_soft_reboot(hdmirx);

				disp_pr_info("[hdmirx]phy init success. status 0x%x, irq mask\n", phy_status);
				hdmirx_video_detect_enable(hdmirx);
				hdmirx_packet_depack_enable(hdmirx);
				hdmirx_video_dsc_mask(hdmirx);
			} else if (cnt >= 10) {
				cnt = 0;
				disp_pr_info("[hdmirx]phy init fail. %d, phy_status 0x%x, is not 3f8\n", phy_init, phy_status);
			}
		}

		if ((!g_start_display) && hdmirx_display_is_ready(hdmirx)) {
			hdmirx_ctrl_source_select_hdmi(hdmirx);
			mdelay(200);
			hdmirx_video_start_display(hdmirx);
			g_start_display = true;
		}
	}

	return 0;
}

void hdmirx_create_main_task(struct hdmirx_ctrl_st *hdmirx)
{
	if (g_hdmirx_main_task == NULL) {
		g_hdmirx_main_task = kthread_create(hdmirx_main_task_thread, hdmirx, "hdmirx_main_task");
		if (IS_ERR(g_hdmirx_main_task)) {
			disp_pr_err("Unable to start kernel hdmirx_main_task\n");
			g_hdmirx_main_task = NULL;
			return;
		}
	}
	wake_up_process(g_hdmirx_main_task);
}

void hdmirx_destroy_main_task(struct hdmirx_ctrl_st *hdmirx)
{
	if (g_hdmirx_main_task) {
		kthread_stop(g_hdmirx_main_task);
		g_hdmirx_main_task = NULL;

		disp_pr_info("stop main task thread succ\n");
	}
}

/* for hotplug out */
int hdmirx_off(struct hdmirx_ctrl_st *hdmirx)
{
	disp_check_and_return((hdmirx == NULL), -EINVAL, err, "[hdmirx] hdmirx is NULL\n");

	disp_pr_info("[hdmirx]hdmi rx off enter\n");

	hdmirx_destroy_main_task(hdmirx);

	hdmirx_ctrl_irq_disable(hdmirx);

	// phy reset
	set_reg(hdmirx->hdmirx_sysctrl_base + 0x084, 0x3ff, 32, 0);
	disp_pr_info("[hdmirx]reset phy 0x3ff\n");

	// set true: hpd down
	hdmirx_link_hpd_set(hdmirx, false);

	// ctrl reset
	hdmirx_hsdt1_dprx_reset(hdmirx);

	disp_pr_info("[hdmirx] hdmi rx off exit\n");

	return 0;
}

/* for hotplug in */
int hdmirx_on(struct hdmirx_ctrl_st *hdmirx)
{
	disp_check_and_return((hdmirx == NULL), -EINVAL, err, "[hdmirx] hdmirx is NULL\n");

	disp_pr_info("[hdmirx]hdmi rx on enter\n");

	// clock set
	hdmirx_hsdt1_crg_set(hdmirx);

	hdmirx_ddc_i2c_slaver_set(hdmirx);

	hdmirx_edid_init(hdmirx);

	hdmirx_hdcp_init(hdmirx);

	hdmirx_ctrl_init(hdmirx);

	hdmirx_ctrl_irq_enable(hdmirx);

	disp_pr_info("[hdmirx] hdmi rx on success. hpd need cmd\n");

	return 0;
}

void hdmirx_power5v_on(struct hdmirx_ctrl_st *hdmirx_ctrl)
{
	disp_pr_info("[hdmirx]power 5v on\n");

	hdmirx_on(hdmirx_ctrl);

	// set true: hpd up
	hdmirx_link_hpd_set(hdmirx_ctrl, true);

	hdmirx_create_main_task(hdmirx_ctrl);
}

void hdmirx_hotplug_in(void)
{
	disp_pr_info("[hdmirx]hdmi insert\n");

	if (g_dpu_hdmirx_dev == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_on(&(g_dpu_hdmirx_dev->hdmirx_ctrl));
}
EXPORT_SYMBOL_GPL(hdmirx_hotplug_in);

void hdmirx_power5v_off(struct hdmirx_ctrl_st *hdmirx_ctrl)
{
	disp_pr_info("[hdmirx]power 5v off\n");

	hdmirx_video_stop_display(hdmirx_ctrl);

	hdmirx_off(hdmirx_ctrl);

	disp_pr_info("[hdmirx]hdmi dss not ready\n");
	g_start_display_hdmi_ready = false;
	g_start_display_dss_ready = false;
	g_start_display = false;

	g_hotplug_flag = true;
}

void hdmirx_hotplug_out(void)
{
	disp_pr_info("[hdmirx]hdmi out\n");

	if (g_dpu_hdmirx_dev == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_power5v_off(&(g_dpu_hdmirx_dev->hdmirx_ctrl));
}
EXPORT_SYMBOL_GPL(hdmirx_hotplug_out);

void hdmirx_cmd_hpd_set(void)
{
	struct hdmirx_ctrl_st *hdmirx;
	disp_pr_info("[hdmirx]hpd set\n");

	hdmirx = hdmirx_get_dev_ctrl();
	if (hdmirx == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_link_hpd_set(hdmirx, true);
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_hpd_set);

void hdmirx_cmd_phy_init(void)
{
	struct hdmirx_ctrl_st *hdmirx;
	disp_pr_info("[hdmirx]phy init\n");

	hdmirx = hdmirx_get_dev_ctrl();
	if (hdmirx == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_create_main_task(hdmirx);
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_phy_init);

void hdmirx_cmd_frl_init(void)
{
	struct hdmirx_ctrl_st *hdmirx;
	disp_pr_info("[hdmirx]frl init\n");

	hdmirx = hdmirx_get_dev_ctrl();
	if (hdmirx == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_link_frl_init(hdmirx);
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_frl_init);

void hdmirx_start_hdcp_init(void)
{
	if (g_dpu_hdmirx_dev == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}
	hdmirx_start_hdcp(&(g_dpu_hdmirx_dev->hdmirx_ctrl));
}
EXPORT_SYMBOL_GPL(hdmirx_start_hdcp_init);

void hdmirx_cmd_hpd_flt(void)
{
	struct hdmirx_ctrl_st *hdmirx;
	disp_pr_info("[hdmirx]hpd set\n");

	hdmirx = hdmirx_get_dev_ctrl();
	if (hdmirx == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_link_hpd_set(hdmirx, true);
	hdmirx_link_frl_ready(hdmirx);
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_hpd_flt);

void hdmirx_cmd_flt_ready(void)
{
	struct hdmirx_ctrl_st *hdmirx;
	disp_pr_info("[hdmirx]flt_ready\n");

	hdmirx = hdmirx_get_dev_ctrl();
	if (hdmirx == NULL) {
		disp_pr_err("[hdmirx]g_dpu_hdmirx_dev NULL\n");
		return;
	}

	hdmirx_link_frl_ready(hdmirx);
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_flt_ready);

static bool is_valid_irq = false;
static irqreturn_t hdmirx_hpd_gpio_irq(int irq, void *ptr)
{
	disp_pr_info("[hdmirx] hpd gpio irq\n");
	is_valid_irq = !is_valid_irq;
	return IRQ_WAKE_THREAD;
}

static irqreturn_t hdmirx_hpd_gpio_irq_thread(int irq, void *ptr)
{
	struct hdmirx_chr_dev_st *hdmirx_chr_dev = ptr;
	struct hdmirx_ctrl_st *hdmirx_ctrl = NULL;
	int gpio_high= 0;
	uint32_t is_plug = 0;

	if (hdmirx_chr_dev == NULL) {
		disp_pr_err("[hdmirx]parameter invalid, ptr null\n");
		return 0;
	}
	hdmirx_ctrl = &(hdmirx_chr_dev->hdmirx_ctrl);

	gpio_high = gpio_get_value_cansleep(hdmirx_ctrl->hpd_gpio);

	is_plug = inp32(hdmirx_chr_dev->hdmirx_ctrl.hi_gpio14_base + 0xdc) & 0x4;

	disp_pr_info("[hdmirx] gpio_high %d, is_plug %d\n", gpio_high, is_plug);

	mdelay(5);
	if (!is_valid_irq) {
		disp_pr_info("[hdmirx] ignore invalid irq\n");
		return 0;
	}
	is_valid_irq = false;

	if (is_plug == 0) {
		mdelay(100); // ignore short irq
		is_plug = inp32(hdmirx_chr_dev->hdmirx_ctrl.hi_gpio14_base + 0xdc) & 0x4;
		gpio_high = gpio_get_value_cansleep(hdmirx_ctrl->hpd_gpio);
		disp_pr_info("[hdmirx]mdelay gpio_high %d, is_plug %d\n", gpio_high, is_plug);
		if (is_plug != 0) {
			disp_pr_info("[hdmirx] ignore the irq  gpio_high=%d, is_plug %d\n", gpio_high, is_plug);
			return 0;
		}

		// FPGA not call hdmirx_power5v_off;
		disp_pr_info("[hdmirx]hdmi hotplug out\n");
	} else {
		// FPGA not call hdmirx_power5v_on;
		disp_pr_info("[hdmirx]hdmi hotplug in\n");
	}

	return 0;
}

static int hdmirx_init_hpd_gpio(struct hdmirx_chr_dev_st *hdmirx_chr_dev)
{
	int ret;
	int irq_num;
	int temp;

	// for FPGA
	set_reg(hdmirx_chr_dev->hdmirx_ctrl.hi_gpio14_base + 0x10, 0x001F0004, 32, 0);
	temp = inp32(hdmirx_chr_dev->hdmirx_ctrl.hi_gpio14_base + 0x10);
	disp_pr_info("gpio config is 0x%x\n", temp); // 0000_0004
	set_reg(hdmirx_chr_dev->hdmirx_ctrl.hi_gpio14_base + 0x88, 0, 1, 2);
	disp_pr_info("hi_gpio14_base 0x88 config\n");

	hdmirx_chr_dev->hdmirx_ctrl.hpd_gpio = 114; // 14 * 8 + 2 = 114; 14 * 32 + 2 = 450
	disp_pr_info("hpd_gpio 114\n");

	ret = devm_gpio_request(&(hdmirx_chr_dev->pdev->dev), hdmirx_chr_dev->hdmirx_ctrl.hpd_gpio, "gpio_hpd");
	if (ret) {
		disp_pr_err("[hdmirx] Fail[%d] request gpio:%d\n", ret, hdmirx_chr_dev->hdmirx_ctrl.hpd_gpio);
		return ret;
	}

	ret = gpio_direction_input(hdmirx_chr_dev->hdmirx_ctrl.hpd_gpio);
	if (ret < 0) {
		disp_pr_err("[hdmirx] Failed to set gpio_down direction\n");
		return ret;
	}

	irq_num = gpio_to_irq(hdmirx_chr_dev->hdmirx_ctrl.hpd_gpio);
	if (irq_num < 0) {
		disp_pr_err("[hdmirx] Failed to get dp_hpd_gpio irq\n");
		ret = -EINVAL;
		return ret;
	}

	ret = devm_request_threaded_irq(&(hdmirx_chr_dev->pdev->dev),
		irq_num, hdmirx_hpd_gpio_irq, hdmirx_hpd_gpio_irq_thread,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"hpd_gpio", (void *)hdmirx_chr_dev);
	if (ret) {
		disp_pr_err("[hdmirx] Failed to request press interupt handler\n");
		return ret;
	}
	hdmirx_chr_dev->hdmirx_ctrl.hpd_irq = irq_num;
	disp_pr_info("[hdmirx] hpd_irq %d\n", irq_num);

	return 0;
}

static int hdmirx_config_base_addr_init(struct device_node *np, struct hdmirx_ctrl_st *hdmirx_ctrl)
{
	char __iomem *hdmirx_base_addrs[HDMIRX_BASE_ADDR_MAX] = {NULL};
	int i = 0;

	disp_pr_info("+++\n");

	for (i = 0; i < HDMIRX_BASE_ADDR_MAX; i++) {
		hdmirx_base_addrs[i] = of_iomap(np, i);
		if (!hdmirx_base_addrs[i]) {
			pr_err("get hdmi rx base addr %u fail\n", i);
			return -EINVAL;
		}
	}
	hdmirx_ctrl->hdmirx_aon_base = hdmirx_base_addrs[HDMIRX_AON_BASE_ID];
	hdmirx_ctrl->hdmirx_pwd_base = hdmirx_base_addrs[HDMIRX_PWD_BASE_ID];
	hdmirx_ctrl->hdmirx_sysctrl_base = hdmirx_base_addrs[HDMIRX_SYSCTRL_BASE_ID];
	hdmirx_ctrl->hdmirx_hsdt1_crg_base = hdmirx_base_addrs[HDMIRX_HSDT1_CRG_BASE_ID];
	hdmirx_ctrl->hsdt1_sysctrl_base = hdmirx_base_addrs[HDMIRX_HSDT1_SYSCTRL_BASE_ID];
	hdmirx_ctrl->hdmirx_ioc_base = hdmirx_base_addrs[HDMIRX_IOC_BASE_ID];
	hdmirx_ctrl->hi_gpio14_base = hdmirx_base_addrs[HDMIRX_HI_GPIO14_BASE_ID];

	disp_pr_info("---\n");

	return 0;
}

static int hdmirx_drv_init_hdmi_config(struct hdmirx_chr_dev_st *hdmirx_dev)
{
	struct device_node *np = hdmirx_dev->pdev->dev.of_node;
	struct hdmirx_ctrl_st *hdmirx_ctrl = &(hdmirx_dev->hdmirx_ctrl);

	disp_pr_info("+++\n");

	hdmirx_ctrl_irq_config_init(np, hdmirx_ctrl);

	hdmirx_config_base_addr_init(np, hdmirx_ctrl);

	disp_pr_info("---\n");

	return 0;
}

static int hdmirx_device_init(struct hdmirx_chr_dev_st *hdmirx_dev)
{
	struct hdmirx_ctrl_st *hdmirx = &(hdmirx_dev->hdmirx_ctrl);

	disp_pr_info("+++\n");

	hdmirx_drv_init_hdmi_config(hdmirx_dev);

	hdmirx_hdcp_tee_init();

	hdmirx_ctrl_irq_setup(hdmirx);

	disp_pr_info("---\n");

	return 0;
}

void hdmirx_drv_init_component(struct hisi_connector_device *hdmirx_dev)
{
}

static int hdmirx_drv_open(struct inode *inode, struct file *filp)
{
	disp_pr_info("+++\n");

	disp_pr_info("---\n");
	return 0;
}

static ssize_t hdmirx_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t hdmirx_debug_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute hdmirx_attrs[] = {
	__ATTR(hdmirx_debug, S_IRUGO | S_IWUSR, hdmirx_debug_show, hdmirx_debug_store),

	/* TODO: other attrs */
};

static int hdmirx_drv_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long hdmirx_drv_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations hdmirx_fops = {
	.owner = THIS_MODULE,
	.open = hdmirx_drv_open, // hisi_offline_open,
	.release = hdmirx_drv_release, // hisi_offline_release,
	.unlocked_ioctl = hdmirx_drv_ioctl, // hisi_offline_adaptor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =  hdmirx_drv_ioctl, // hisi_offline_adaptor_ioctl,
#endif
};

// hisi_drv_offline_create_chrdev
static void hdmirx_drv_create_chrdev(struct hdmirx_chr_dev_st *hdmirx_dev)
{
	/* init chrdev info */
	hdmirx_dev->chrdev.name = HISI_HDMIRX_CHR_NAME;
	hdmirx_dev->chrdev.id = 1;
	hdmirx_dev->chrdev.chrdev_fops = &hdmirx_fops;

	hisi_disp_create_chrdev(&hdmirx_dev->chrdev, hdmirx_dev);
	hisi_disp_create_attrs(hdmirx_dev->chrdev.dte_cdevice, hdmirx_attrs, ARRAY_SIZE(hdmirx_attrs));
}

static int hdmirx_drv_probe(struct platform_device *pdev)
{
	struct hdmirx_chr_dev_st *hdmirx_dev = NULL;

	disp_pr_info("+++\n");

	if (pdev == NULL) {
		disp_pr_err("[hdmirx] pdev null\n");
		return -1;
	}

	hdmirx_dev = devm_kzalloc(&pdev->dev, sizeof(*hdmirx_dev), GFP_KERNEL);
	if (!hdmirx_dev) {
		disp_pr_err("[hdmirx] devm_kzalloc fail\n");
		return -1;
	}
	hdmirx_dev->pdev = pdev;

	hdmirx_device_init(hdmirx_dev);

	/* create chrdev */
	hdmirx_drv_create_chrdev(hdmirx_dev);
	g_dpu_hdmirx_dev = hdmirx_dev;

	hdmirx_init_hpd_gpio(hdmirx_dev);

	disp_pr_info("---\n");
	return 0;
}

static int hdmirx_drv_remove(struct platform_device *pdev)
{
	disp_pr_info("+++\n");

	disp_pr_info("---\n");
	return 0;
}

static const struct of_device_id device_match_table[] = {
	{
		.compatible = DTS_NAME_HDMIRX0,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, device_match_table);

static struct platform_driver g_dpu_hdmirx_driver = {
	.probe = hdmirx_drv_probe,
	.remove = hdmirx_drv_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_CONNECTOR_HDMIRX_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(device_match_table),
	},
};

static int __init hdmirx_module_init(void)
{
	int ret;

	ret = platform_driver_register(&g_dpu_hdmirx_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d\n", ret);
		return ret;
	}

	return ret;
}

module_init(hdmirx_module_init);

MODULE_DESCRIPTION("Hisilicon HDMI RX Driver");
MODULE_LICENSE("GPL v2");
