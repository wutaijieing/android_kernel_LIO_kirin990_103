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
#include "rx_reg.h"
#include "rx_core.h"
#include "rx_irq.h"
#include "dprx.h"
#include "../device/hisi_disp.h"


#undef pr_fmt
#define pr_fmt(fmt)  "[DPRX] " fmt

/* for test */
uint32_t g_dprx_log_level = DPU_LOG_LVL_DEBUG;
void set_dprx_debug_level(uint32_t log_level)
{
	g_dprx_log_level = log_level;
}
EXPORT_SYMBOL_GPL(set_dprx_debug_level);

struct platform_device *g_dprx_pdev[DPRX_CTRL_NUMS_MAX];


int dprx_off(struct dprx_ctrl *dprx)
{
	int ret = 0;

	dpu_check_and_return((dprx == NULL), -EINVAL, err, "dprx is NULL!\n");
	dpu_pr_info("+++\n");

	mutex_lock(&dprx->dprx_mutex);
	if (!dprx->power_on) {
		dpu_pr_info("[DPRX] dprx has already off\n");
		mutex_unlock(&dprx->dprx_mutex);
		return 0;
	}

	/* ctrl config */
	dprx_core_off(dprx);

	/* reset */
	dprx_dis_reset(dprx, false);

	if (dprx->dprx_sdp_wq != NULL) {
		destroy_workqueue(dprx->dprx_sdp_wq);
		dprx->dprx_sdp_wq = NULL;
	}

	dprx->power_on = false;
	dprx->v_params.video_format_valid = false;
	dprx->msa.msa_valid = false;

	dprx->video_input_params_ready = false;

	mutex_unlock(&dprx->dprx_mutex);

	dpu_pr_info("---\n");

	return ret;
}

int dprx_on(struct dprx_ctrl *dprx)
{
	int ret = 0;

	dpu_check_and_return((dprx == NULL), -EINVAL, err, "dprx is NULL!\n");
	dpu_pr_info("+++\n");

	mutex_lock(&dprx->dprx_mutex);
	if (dprx->power_on) {
		dpu_pr_info("[DPRX] dprx has already on\n");
		mutex_unlock(&dprx->dprx_mutex);
		return 0;
	}

	/* disreset */
	dprx_dis_reset(dprx, true);

	/* ctrl config */
	dprx_core_on(dprx);

	/* Check here: In dprx doc, enable clk after ctrl reset */
	dprx->dprx_sdp_wq = create_singlethread_workqueue("dprx_sdp_wq");
	if (dprx->dprx_sdp_wq == NULL) {
		dpu_pr_err("[DP] create dprx->dprx_sdp_wq failed\n");
		mutex_unlock(&dprx->dprx_mutex);
		return -1;
	}

	INIT_WORK(&dprx->dprx_sdp_work, dprx_sdp_wq_handler);

	dprx->power_on = true;

	mutex_unlock(&dprx->dprx_mutex);

	dpu_pr_info("---\n");

	return ret;
}

int disp_dprx_hpd_trigger(uint8_t rx_id, uint8_t is_plug_in, uint8_t rx_lane_num)
{
	struct dpu_dprx_device *dprx_data = NULL;
	struct dprx_ctrl *dprx = NULL;
	int ret = 0;

	dpu_check_and_return((rx_id >= DPRX_CTRL_NUMS_MAX), -EINVAL, err, "[DPRX] rx_id >= DPRX_CTRL_NUMS_MAX!\n");
	dpu_check_and_return((g_dprx_pdev[rx_id] == NULL), -EINVAL, err, "[DPRX] g_dprx_pdev is NULL!\n");

	dprx_data = platform_get_drvdata(g_dprx_pdev[rx_id]);
	dpu_check_and_return((dprx_data == NULL), -EINVAL, err, "[DP] dprx_data is NULL!\n");

	dprx = &(dprx_data->dprx);
	dpu_pr_info("[DPRX] dprx%u +\n", dprx->id);

	if (is_plug_in)
		dprx_on(dprx);
	else
		dprx_off(dprx);

	dpu_pr_info("[DPRX] dprx%u -\n", dprx->id);
	return ret;
}
EXPORT_SYMBOL_GPL(disp_dprx_hpd_trigger);

static void dprx_connect(rx_context_t rx_ctx, notify_dsscontext_func event_notify)
{
	struct dprx_ctrl *dprx = rx_ctx;

	if (!dprx) {
		dpu_pr_err("[DTE] invalid rx context\r\n");
		return;
	}

	dpu_pr_info("[DTE] dprx_connect %d\n", dprx->id);

	dprx->notify_dsscontext = event_notify;

	rx_edid_init(&dprx->edid, dprx->edid.valid_block_num, dprx->intf_info.timing_code);

	disp_dprx_hpd_trigger(dprx->id, 1, 4);
}

static void dprx_disconnect(rx_context_t rx_ctx)
{
	struct dprx_ctrl *dprx = rx_ctx;

	if (!dprx) {
		dpu_pr_err("[DTE] invalid rx context\r\n");
		return;
	}

	dpu_pr_info("[DTE] dprx_disconnect %d\n", dprx->id);
	disp_dprx_hpd_trigger(dprx->id, 0, 4);
}

static void dprx_get_information(rx_context_t rx_ctx, dss_intfinfo_t *info)
{
	struct dprx_ctrl *dprx = rx_ctx;

	if (!dprx) {
		dpu_pr_err("[DTE] invalid rx context\r\n");
		return;
	}

	dpu_pr_info("[DTE] dprx_get_information %d\n", dprx->id);

	*info = dprx->intf_info;
}

static void dprx_enable_stream(rx_context_t rx_ctx, bool on)
{
	struct dprx_ctrl *dprx = rx_ctx;

	if (!dprx) {
		dpu_pr_err("[DTE] invalid rx context\r\n");
		return;
	}

	dpu_pr_info("[DTE] dprx%d, %s stream\n", dprx->id, on ? "enable" : "disable");

	if (on) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_VIDEO_CTRL, 2);
		mdelay(20);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_VIDEO_CTRL, 1);
	} else {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_VIDEO_CTRL, 0);
	}
}

static void dprx_regisger_inputsource(struct dprx_ctrl *dprx)
{
	dss_inputsource_t source_data = {
		.rx_ctx = (rx_context_t)dprx,
		.enable_inputsource = dprx_connect,
		.disable_inputsource = dprx_disconnect,
		.get_information = dprx_get_information,
		.enable_pipeline = dprx_enable_stream,
	};

	dss_register_inputsource(dprx->id + INPUT_SOURCE_DPRX0, &source_data);
}

// THIS FUNC would be called from IRQ Context
static void dprx_event_notify(struct dprx_ctrl *dprx, uint32_t local_event_id)
{
	uint32_t event_id = 0;
	if (!dprx->notify_dsscontext) {
		dpu_pr_info("[DTE] none event notify registered\n");
		return;
	}

	if (local_event_id & DPRX_IRQ_ATC_TRAIN_LOST) {
		event_id = INPUTSOURCE_DISCONNECTED;
	} else if (local_event_id & (DPRX_IRQ_FORMAT_CHANGE | DPRX_IRQ_TIMING_CHANGE)) {
		event_id = INPUTSOURCE_CONNECTED;
	}

	dpu_pr_info("[DTE] dprx%d notify event_id %d\n", dprx->id, event_id);
	dprx->notify_dsscontext(dprx->id + INPUT_SOURCE_DPRX0, event_id);
}

static int dprx_irq_setup(struct dprx_ctrl *dprx)
{
	int ret;

	dpu_check_and_return((dprx == NULL || dprx->irq == 0), -EINVAL, err, "dprx is NULL!\n");

	ret = devm_request_threaded_irq(&dprx->device->pdev->dev,
		dprx->irq, dprx->dprx_irq_handler, dprx->dprx_threaded_irq_handler,
		IRQF_SHARED | IRQ_LEVEL, "dpu_dprx", (void *)dprx->device);
	if (ret) {
		dpu_pr_err("[DP] Request for irq %u failed!\n", dprx->irq);
		return -EINVAL;
	}
	disable_irq(dprx->irq);

	return 0;
}

static void dprx_link_layer_init(struct dprx_ctrl *dprx)
{
}

static void dprx_ctrl_layer_init(struct dprx_ctrl *dprx)
{
	dprx->local_event_notify = dprx_event_notify;
	dprx->dprx_irq_handler = dprx_irq_handler;
	dprx->dprx_threaded_irq_handler = dprx_threaded_irq_handler;
}

static void dprx_init_timing(dss_intfinfo_t *intf_info, uint16_t timing_code)
{
	intf_info->timing_code = timing_code;
	intf_info->format = 6;

	if (timing_code == TIMING_1080P) {
		intf_info->xres = 1920;
		intf_info->yres = 1080;
		intf_info->framerate = 30;
	} else {
		intf_info->xres = 720;
		intf_info->yres = 480;
		intf_info->framerate = 60;
	}
}

static int dprx_device_init(struct dprx_ctrl *dprx)
{
	int ret;

	dpu_check_and_return((dprx == NULL), -EINVAL, err, "dprx is NULL!\n");

	dprx->notify_dsscontext = NULL;
	dprx_init_timing(&dprx->intf_info, TIMING_1080P);

	rx_edid_init(&dprx->edid, dprx->edid.valid_block_num, dprx->intf_info.timing_code);
	rx_sdp_init(&dprx->pps_table);

	dprx_link_layer_init(dprx);
	dprx_ctrl_layer_init(dprx);

	ret = dprx_irq_setup(dprx);
	if (ret) {
		dpu_pr_err("[DPRX] dprx set irq fail\n");
		return ret;
	}

	dprx->power_on = false;
	dprx->video_on = false;

	return 0;
}

static int dprx_read_dtsi(struct dpu_dprx_device *dprx_data)
{
	int ret;
	uint8_t i;
	char __iomem *rx_base_addrs[DPRX_BASE_ADDR_MAX] = {NULL};

	/**
	 * add clk/base_addr/irq etc.
	 * */

	ret = of_property_read_u32(dprx_data->pdev->dev.of_node, "channel_id", &(dprx_data->dprx.id));
	if (ret) {
		dpu_pr_err("[DPRX] read id fail\n");
		return -EINVAL;
	}
	if (dprx_data->dprx.id >= DPRX_CTRL_NUMS_MAX) {
		dpu_pr_err("[DPRX] invalid id\n");
		return -EINVAL;
	}
	dpu_pr_info("[DPRX] dprx id: %u.\n", dprx_data->dprx.id);

	ret = of_property_read_u32(dprx_data->pdev->dev.of_node, "mode", &(dprx_data->dprx.mode));
	if (ret) {
		dpu_pr_err("[DPRX] read mode fail\n");
		return -EINVAL;
	}
	dpu_pr_info("[DPRX] dprx mode: %u.\n", dprx_data->dprx.mode);

	ret = of_property_read_u8(dprx_data->pdev->dev.of_node, "edid_block_num", &(dprx_data->dprx.edid.valid_block_num));
	if (ret) {
		dpu_pr_err("[DPRX] read edid_block_num fail\n");
		return -EINVAL;
	}
	if (dprx_data->dprx.edid.valid_block_num > MAX_EDID_BLOCK_NUM) {
		dpu_pr_err("[DPRX] error edid_block_num\n");
		return -EINVAL;
	}
	dprx_data->dprx.edid.valid_block_num = 3; /* for test */
	dpu_pr_info("[DPRX] edid_block_num: %u\n", dprx_data->dprx.edid.valid_block_num);

	dprx_data->dprx.irq = irq_of_parse_and_map(dprx_data->pdev->dev.of_node, 0);
	if (dprx_data->dprx.irq == 0)
		dpu_pr_err("get irq no fail\n");
	dpu_pr_info("[DPRX] irq_num: %u\n", dprx_data->dprx.irq);

	for (i = 0; i < DPRX_BASE_ADDR_MAX; i++) {
		rx_base_addrs[i] = of_iomap(dprx_data->pdev->dev.of_node, i);
		if (!rx_base_addrs[i]) {
			dpu_pr_err("get rx base addr %u fail", i);
			return -EINVAL;
		}
	}

	dprx_data->dprx.hsdt1_crg_base = rx_base_addrs[DPRX_HSDT1_CRG_BASE];
	dprx_data->dprx.hsdt1_sys_ctrl_base = rx_base_addrs[DPRX_HSDT1_SYS_CTRL_BASE];
	dprx_data->dprx.base = rx_base_addrs[DPRX_CTRL_BASE];
	dprx_data->dprx.edid_base = rx_base_addrs[DPRX_EDID_BASE];
	dpu_pr_info("[DPRX] hsdt_crg_base: %p\n", dprx_data->dprx.hsdt1_crg_base);
	dpu_pr_info("[DPRX] hsdt1_sys_ctrl_base: %p\n", dprx_data->dprx.hsdt1_sys_ctrl_base);
	dpu_pr_info("[DPRX] dprx_base: %p\n", dprx_data->dprx.base);
	dpu_pr_info("[DPRX] edid_base: %p\n", dprx_data->dprx.edid_base);

	dprx_data->dprx.fpga_flag = true; /* for test, add dts item for fpga flag */

	return 0;
}

static int dprx_probe(struct platform_device *pdev)
{
	struct dpu_dprx_device *dprx_data = NULL;

	int ret;

	dpu_check_and_return((pdev == NULL), -EINVAL, err, "pdev is NULL!\n");
	dpu_pr_info("[DPRX] dprx probe ++++++\n");

	dprx_data = devm_kzalloc(&pdev->dev, sizeof(*dprx_data), GFP_KERNEL);
	if (!dprx_data) {
		dpu_pr_err("alloc dprx fail\n");
		return -ENOMEM;
	}
	dprx_data->pdev = pdev;
	dprx_data->dprx.device = dprx_data;

	ret = dprx_read_dtsi(dprx_data);
	if (ret) {
		dpu_pr_err("[DPRX] read dtsi fail\n");
		return -EINVAL;
	}

	ret = dprx_device_init(&(dprx_data->dprx));
	if (ret) {
		dpu_pr_err("[DPRX] init dprx fail\n");
		return -EINVAL;
	}

	dprx_regisger_inputsource(&dprx_data->dprx);

	g_dprx_pdev[dprx_data->dprx.id] = dprx_data->pdev;
	platform_set_drvdata(pdev, dprx_data);

	dpu_pr_info("[DPRX] dprx probe ------\n");
	return 0;
}

static int dprx_remove(struct platform_device *pdev)
{
	struct dpu_dprx_device *dprx_data = NULL;

	dpu_check_and_return((pdev == NULL), 0, err, "pdev is NULL!\n");

	dprx_data = platform_get_drvdata(pdev);
	if (!dprx_data)
		return 0;

	dpu_pr_info("[DPRX] remove dprx%u++++++\n", dprx_data->dprx.id);

	g_dprx_pdev[dprx_data->dprx.id] = NULL;

	dpu_pr_info("[DPRX] remove dprx%u------\n", dprx_data->dprx.id);
	return 0;
}

uint32_t dprx_ctrl_get_layer_fmt(struct dprx_ctrl *dprx)
{
	uint32_t bit_width;
	uint32_t color_space;
	uint32_t format;

	bit_width = dprx->v_params.bpc;
	color_space = dprx_get_color_space(dprx);

	format = dprx_ctrl_dp2layer_fmt(color_space, bit_width);
	dpu_pr_info("bpp :%d, format value:%d\n", bit_width, format);

	return format;
}
uint32_t g_rx_id = 0;
void set_rxid_getfmt(uint32_t rx_id)
{
	g_rx_id = rx_id;
}
EXPORT_SYMBOL_GPL(set_rxid_getfmt);

uint32_t dprx_get_layer_fmt(uint32_t src_fmt)
{
	struct dpu_dprx_device *dprx_data = NULL;
	struct dprx_ctrl *dprx = NULL;

	if ((g_rx_id >= DPRX_CTRL_NUMS_MAX) || (g_dprx_pdev[g_rx_id] == NULL)) {
		dpu_pr_info("return src fmt\n");
		return src_fmt;
	}

	dprx_data = platform_get_drvdata(g_dprx_pdev[g_rx_id]);
	if (dprx_data == NULL) {
		dpu_pr_info("return src fmt\n");
		return src_fmt;
	}
	dprx = &(dprx_data->dprx);
	if ((dprx == NULL) || (dprx->power_on == false) || (dprx->v_params.video_format_valid == false)) {
		dpu_pr_info("return src fmt\n");
		return src_fmt;
	}
	return dprx_ctrl_get_layer_fmt(dprx);
}
EXPORT_SYMBOL_GPL(dprx_get_layer_fmt);

static const struct of_device_id device_match_table[] = {
	{
		.compatible = DTS_NAME_DPRX0,
		.data = NULL,
	},
	{
		.compatible = DTS_NAME_DPRX1,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, device_match_table);

static struct platform_driver dprx_platform_driver = {
	.probe  = dprx_probe,
	.remove = dprx_remove,
	.driver = {
		.name  = DEV_NAME_DPRX,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(device_match_table),
	}
};

static int __init dprx_register(void)
{
	int ret;
	dpu_pr_info("[DPRX] dprx registing");

	ret = platform_driver_register(&dprx_platform_driver);
	if (ret) {
		dpu_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

static void __exit dprx_unregister(void)
{
	dpu_pr_info("[DPRX] dprx unregisting");
	platform_driver_unregister(&dprx_platform_driver);
}

uint32_t dprx_select_timing(uint8_t rx_id, uint32_t timing_code)
{
	struct dpu_dprx_device *dprx_data = NULL;
	struct dprx_ctrl *dprx;

	dpu_check_and_return((rx_id >= DPRX_CTRL_NUMS_MAX), -EINVAL, err, "invalid rx_id!\n");
	dpu_check_and_return((g_dprx_pdev[rx_id] == NULL), -EINVAL, err, "dprx is NULL!\n");
	dpu_check_and_return(timing_code >= TIMING_MAX, -EINVAL, err, "invalid timingcode\n");

	dprx_data = platform_get_drvdata(g_dprx_pdev[rx_id]);
	dprx = &dprx_data->dprx;

	dprx_init_timing(&dprx->intf_info, timing_code);
	rx_edid_init(&dprx->edid, dprx->edid.valid_block_num, timing_code);

	dpu_pr_info("[DTE] set dprx%d timing code to %d\n", rx_id, timing_code);

	return 0;
}
EXPORT_SYMBOL_GPL(dprx_select_timing);

int dprx_dump_edid(uint8_t rx_id)
{
	struct dpu_dprx_device *dprx_data = NULL;

	dpu_check_and_return((rx_id >= DPRX_CTRL_NUMS_MAX), -EINVAL, err, "invalid rx_id!\n");
	dpu_check_and_return((g_dprx_pdev[rx_id] == NULL), -EINVAL, err, "dprx is NULL!\n");

	dprx_data = platform_get_drvdata(g_dprx_pdev[rx_id]);
	if (!dprx_data)
		return -1;

	rx_edid_dump(&dprx_data->dprx.edid);

	return 0;
}
EXPORT_SYMBOL_GPL(dprx_dump_edid);

uint32_t dprx_enable_videostream(uint8_t rx_id, bool flag)
{
	struct dpu_dprx_device *dprx_data = NULL;

	dpu_check_and_return((rx_id >= DPRX_CTRL_NUMS_MAX), -EINVAL, err, "invalid rx_id!\n");
	dpu_check_and_return((g_dprx_pdev[rx_id] == NULL), -EINVAL, err, "dprx is NULL!\n");

	dprx_data = platform_get_drvdata(g_dprx_pdev[rx_id]);
	if (!dprx_data)
		return -1;

	dprx_enable_stream(&dprx_data->dprx, flag);

	return 0;
}
EXPORT_SYMBOL_GPL(dprx_enable_videostream);

module_init(dprx_register);
module_exit(dprx_unregister);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Dprx Function Driver");
MODULE_LICENSE("GPL");
