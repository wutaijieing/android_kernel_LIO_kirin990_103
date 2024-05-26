/** @file
 * Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#include <securec.h>

#include "adaptor/hisi_dss.h"
#include "adaptor/hisi_fb_adaptor.h"
#include "hisi_fb_pm.h"
#include "hisi_disp_debug.h"
#include "hisi_drv_kthread.h"

#include "hisi_dss_context.h"

int32_t dss_setup_pipeline(uint32_t inputsource, uint32_t displaysink);
int32_t dss_reset_system(void);

#define MAX_MONITOR_MSG 128
// 2s
#define CHECK_TIME_PERIOD 20

typedef int (*monitor_proc_func)(void *data, uint32_t msg_id);
typedef struct dss_monitor_context {
	wait_queue_head_t kthread_wq;
	struct task_struct *task;

	raw_spinlock_t lock;

	bool video_on;
	bool rx_on;
	bool tx_on;
	bool context_ready;

	monitor_proc_func msg_handler;

	uint32_t msg_queue[MAX_MONITOR_MSG];
	uint32_t msg_cnt;
	uint32_t rd_ptr;
	uint32_t wr_ptr;

	void *cookie;
} dss_monitor_context_t;

typedef struct dss_fpga_context {
	bool to_restart_rx;
	uint32_t pipeline_state;

	bool dpu_state;
	bool dsd_state;

	uint32_t cur_inputsource;
	uint32_t cur_displaysink;

	struct platform_device *save_primary_pdev;
	struct platform_device *save_external_pdev;

	dss_inputsource_t inputsources[INPUT_SOURCE_MAX];
	dss_displaysink_t displaysinks[DISPLAY_SINK_MAX];

	dss_monitor_context_t monitor_ctx;

	struct hrtimer hrtimer;
} dss_fpga_context_t;

static dss_fpga_context_t g_fpga_context;
static bool g_auto_reconnect;

static void dss_monitor_queue(dss_monitor_context_t *monitor_ctx, uint32_t msg_id)
{
	bool full_flag = false;

	raw_spin_lock(&monitor_ctx->lock);
	if (monitor_ctx->msg_cnt < MAX_MONITOR_MSG) {
		monitor_ctx->msg_queue[monitor_ctx->wr_ptr] = msg_id;

		monitor_ctx->wr_ptr++;
		monitor_ctx->msg_cnt++;
		monitor_ctx->wr_ptr %= MAX_MONITOR_MSG;
	} else {
		full_flag = true;
	}
	raw_spin_unlock(&monitor_ctx->lock);

	if (full_flag)
		disp_pr_err("monitor queue is full\n");

	wake_up(&monitor_ctx->kthread_wq);
}

static bool dss_monitor_dequeue(dss_monitor_context_t *monitor_ctx, uint32_t *msg_id)
{
	bool ret = false;

	raw_spin_lock(&monitor_ctx->lock);
	if (monitor_ctx->msg_cnt != 0) {
		ret = true;
		*msg_id = monitor_ctx->msg_queue[monitor_ctx->rd_ptr];

		monitor_ctx->rd_ptr++;
		monitor_ctx->msg_cnt--;
		monitor_ctx->rd_ptr %= MAX_MONITOR_MSG;
	}
	raw_spin_unlock(&monitor_ctx->lock);

	return ret;
}

static void dss_reset_monitor(dss_monitor_context_t *monitor_ctx)
{
	disp_pr_info("reset monitor msg queue");

	raw_spin_lock(&monitor_ctx->lock);

	monitor_ctx->msg_cnt = 0;
	monitor_ctx->rd_ptr = 0;
	monitor_ctx->wr_ptr = 0;

	monitor_ctx->video_on = false;
	monitor_ctx->tx_on = false;
	monitor_ctx->rx_on = false;
	monitor_ctx->context_ready = false;

	raw_spin_unlock(&monitor_ctx->lock);
}

static int dss_monitor_entry(void *data)
{
	dss_monitor_context_t *monitor_ctx = (dss_monitor_context_t *)data;
	uint32_t msg_id;

	while (1) {
		wait_event(monitor_ctx->kthread_wq, monitor_ctx->msg_cnt || kthread_should_stop());

		if (kthread_should_stop())
			break;

		while (dss_monitor_dequeue(monitor_ctx, &msg_id))
			monitor_ctx->msg_handler(monitor_ctx->cookie, msg_id);
	}

	return 0;
}

static void dss_create_monitor(dss_monitor_context_t *monitor_ctx, monitor_proc_func handler, void *cookie)
{
	disp_pr_info("create pipeline monitor\n");

	init_waitqueue_head(&monitor_ctx->kthread_wq);
	raw_spin_lock_init(&monitor_ctx->lock);

	monitor_ctx->msg_handler = handler;
	monitor_ctx->cookie = cookie;

	dss_reset_monitor(monitor_ctx);

	monitor_ctx->task = kthread_create(dss_monitor_entry, monitor_ctx, "dss_pipeline_monitor");
	if (!monitor_ctx->task) {
		disp_pr_err("failed to create dss pipeline monitor\n");
		return;
	}
	wake_up_process(monitor_ctx->task);
}

static void dss_destroy_monitor(dss_monitor_context_t *monitor_ctx)
{
	if (monitor_ctx->task) {
		disp_pr_info("stop dss pipeline monitor\n");
		kthread_stop(monitor_ctx->task);
	}
}

static inline void dss_rect_init(dss_rect_t *rect, int32_t x, int32_t y, int32_t w, int32_t h)
{
	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = h;
	return;
}

static bool dss_is_valid_inputsource(uint32_t inputsource)
{
	return inputsource < INPUT_SOURCE_MAX && inputsource > INPUT_SOURCE_NONE;
}

static bool dss_is_valid_displaysink(uint32_t displaysink)
{
	return displaysink < DISPLAY_SINK_MAX && displaysink > DISPLAY_SINK_NONE;
}

static bool dss_is_dp_displaysink(uint32_t displaysink)
{
	return displaysink == DISPLAY_SINK_DPTX0;
}

static bool dss_is_mipi_displaysink(uint32_t displaysink)
{
	return displaysink == DISPLAY_SINK_DSI0;
}

static bool dss_is_external_inputsource(uint32_t inputsource)
{
	return inputsource == INPUT_SOURCE_DPRX0 || inputsource == INPUT_SOURCE_DPRX1 || inputsource == INPUT_SOURCE_HDMIIN;
}

static void dss_notify_dprxevent(uint32_t inputsource, uint32_t msg_id)
{
	disp_pr_info("notify dprx event : inputsource %d, msg %d\n", inputsource, msg_id);

	dss_monitor_queue(&g_fpga_context.monitor_ctx, msg_id);
}

static void dss_notify_dptxevent(uint32_t displaysink, uint32_t msg_id)
{
	disp_pr_info("notify dptx event : displaysink %d, msg %d\n", displaysink, msg_id);

	dss_monitor_queue(&g_fpga_context.monitor_ctx, msg_id);
}

void dss_register_displaysink(uint32_t displaysink, dss_displaysink_t *intf)
{
	disp_pr_info(" enter ++++\n");

	if (!intf || !dss_is_valid_inputsource(displaysink)) {
		disp_pr_err("failed to register displaysink\n");
		return;
	}

	g_fpga_context.displaysinks[displaysink] = *intf;

	disp_pr_info(" exit ----\n");
}
EXPORT_SYMBOL_GPL(dss_register_displaysink);

void dss_register_inputsource(uint32_t inputsource, dss_inputsource_t *intf)
{
	disp_pr_info(" enter ++++\n");

	if (!intf || !dss_is_valid_inputsource(inputsource)) {
		disp_pr_err("failed to register inputsource\n");
		return;
	}

	g_fpga_context.inputsources[inputsource] = *intf;
	disp_pr_info(" exit ----\n");
}
EXPORT_SYMBOL_GPL(dss_register_inputsource);

static void _dss_enable_inputsource(dss_fpga_context_t *fpga_ctx, uint32_t inputsource)
{
	enable_inputsource_func enable_inputsource = NULL;
	rx_context_t rx_ctx;
	disp_pr_info(" enter ++++\n");

	rx_ctx = fpga_ctx->inputsources[inputsource].rx_ctx;
	enable_inputsource = fpga_ctx->inputsources[inputsource].enable_inputsource;
	if (!enable_inputsource) {
		disp_pr_err("none enable func for inputsource %d\n", inputsource);
		return;
	}

	enable_inputsource(rx_ctx, dss_notify_dprxevent);

	disp_pr_info(" exit ----\n");
}

static void dss_enable_inputsource(uint32_t inputsource)
{
	if (inputsource == g_fpga_context.cur_inputsource) {
		disp_pr_warn("inputsource %d already enabled\n", inputsource);
		return;
	}

	_dss_enable_inputsource(&g_fpga_context, inputsource);
	g_fpga_context.cur_inputsource = inputsource;
}

static void dss_disable_inputsource(uint32_t inputsource)
{
	disable_inputsource_func disable_inputsource = NULL;
	rx_context_t rx_ctx;

	disp_pr_info(" enter ++++\n");

	if (g_fpga_context.cur_inputsource != inputsource) {
		disp_pr_err("current inputsource is %d, inputsource requested to be disabled %d\n",
			g_fpga_context.cur_inputsource, inputsource);
		return;
	}

	rx_ctx = g_fpga_context.inputsources[inputsource].rx_ctx;
	disable_inputsource = g_fpga_context.inputsources[inputsource].disable_inputsource;
	if (!disable_inputsource) {
		disp_pr_err("none disable func for inputsource %d\n", inputsource);
		return;
	}

	disable_inputsource(rx_ctx);
	g_fpga_context.cur_inputsource = INPUT_SOURCE_NONE;

	disp_pr_info(" exit ----\n");
}

static void dss_get_inputsource_info(uint32_t inputsource, dss_intfinfo_t *info)
{
	get_rxinfo_func get_information = NULL;
	rx_context_t rx_ctx;

	disp_pr_info(" enter ++++\n");

	rx_ctx = g_fpga_context.inputsources[inputsource].rx_ctx;
	get_information = g_fpga_context.inputsources[inputsource].get_information;
	if (!get_information) {
		disp_pr_err("none getinfor func for inputsource %d\n", inputsource);
		return;
	}

	get_information(rx_ctx, info);

	disp_pr_info(" exit ----\n");
}

static void dss_enable_videostream(uint32_t inputsource, bool on)
{
	enable_pipeline_func enable_pipeline = NULL;
	rx_context_t rx_ctx;

	disp_pr_info(" enter ++++\n");

	rx_ctx = g_fpga_context.inputsources[inputsource].rx_ctx;
	enable_pipeline = g_fpga_context.inputsources[inputsource].enable_pipeline;
	if (!enable_pipeline) {
		disp_pr_err("none enable pipeline func for inputsource %d\n", inputsource);
		return;
	}

	enable_pipeline(rx_ctx, on);

	disp_pr_info(" exit ----\n");
}

static void dss_enable_displaysink(uint32_t displaysink, uint32_t inputsouce)
{
	enable_displaysink_func enable_displaysink = NULL;
	tx_context_t tx_ctx;

	disp_pr_info(" enter ++++\n");

	if (displaysink == g_fpga_context.cur_displaysink) {
		disp_pr_warn("displaysink %d already enabled\n", displaysink);
		return;
	}

	tx_ctx = g_fpga_context.displaysinks[displaysink].tx_ctx;
	enable_displaysink = g_fpga_context.displaysinks[displaysink].enable_displaysink;
	if (!enable_displaysink) {
		disp_pr_err("none enable display sink func for displaysink %d\n", displaysink);
		return;
	}

	enable_displaysink(tx_ctx, inputsouce, dss_notify_dptxevent);
	g_fpga_context.cur_displaysink = displaysink;

	disp_pr_info(" exit ----\n");
}

static void dss_disable_displaysink(uint32_t displaysink)
{
	disable_displaysink_func disable_displaysink = NULL;
	tx_context_t tx_ctx;

	disp_pr_info(" enter ++++\n");

	if (g_fpga_context.cur_displaysink != displaysink) {
		disp_pr_err("current displaysink is %d, displaysink requested to be disabled %d\n",
			g_fpga_context.cur_displaysink, displaysink);
		return;
	}

	tx_ctx = g_fpga_context.displaysinks[displaysink].tx_ctx;
	disable_displaysink = g_fpga_context.displaysinks[displaysink].disable_displaysink;
	if (!disable_displaysink) {
		disp_pr_err("none disable display sink func for displaysink %d\n", displaysink);
		return;
	}

	disable_displaysink(tx_ctx);
	g_fpga_context.cur_displaysink = DISPLAY_SINK_NONE;

	disp_pr_info(" exit ----\n");
}

static void dss_get_displaysink_info(uint32_t displaysink, dss_intfinfo_t *info)
{
	get_txinfo_func get_information = NULL;
	tx_context_t tx_ctx;

	disp_pr_info(" enter ++++\n");

	tx_ctx = g_fpga_context.displaysinks[displaysink].tx_ctx;
	get_information = g_fpga_context.displaysinks[displaysink].get_information;
	if (!get_information) {
		disp_pr_err("none getinfor func for displaysink %d\n", displaysink);
		return;
	}

	get_information(tx_ctx, info);

	disp_pr_info(" exit ----\n");
}

void dss_save_pdev(uint32_t ov_id, struct platform_device *pdev)
{
	disp_pr_info("save pdev for id ov%d\n", ov_id);

	if (ov_id == ONLINE_OVERLAY_ID_PRIMARY)
		g_fpga_context.save_primary_pdev = pdev;

	if (ov_id == ONLINE_OVERLAY_ID_EXTERNAL1)
		g_fpga_context.save_external_pdev = pdev;
}

bool dss_is_system_on(void)
{
	return g_fpga_context.dpu_state == STATE_ON;
}

static void _dss_power_up(uint32_t displaysink)
{
	struct hisi_fb_info *hfb_info = NULL;
	struct platform_device *pdev = NULL;

	disp_pr_info(" enter ++++\n");

	if (dss_is_dp_displaysink(displaysink)) {
		pdev = g_fpga_context.save_external_pdev;
	} else {
		pdev = g_fpga_context.save_primary_pdev;
	}

	if (!pdev) {
		disp_pr_err("pdev hadn't been saved for %d yet\n", displaysink);
		return;
	}

	g_fpga_context.dpu_state = STATE_ON;
	hfb_info = platform_get_drvdata(pdev);
	hisi_fb_pm_force_blank(FB_BLANK_UNBLANK, hfb_info, true);
	disp_pr_info(" exit ----\n");
}

static void _dss_power_down(void)
{
	struct hisi_fb_info *hfb_info = NULL;

	disp_pr_info(" enter ++++\n");

	if (!g_fpga_context.save_primary_pdev) {
		disp_pr_err("primary pdev hadn't been saved yet\n");
		return;
	}

	g_fpga_context.dpu_state = STATE_OFF;
	hfb_info = platform_get_drvdata(g_fpga_context.save_primary_pdev);
	hisi_fb_pm_force_blank(FB_BLANK_POWERDOWN, hfb_info, true);
	disp_pr_info(" exit ----\n");
}

static int dss_setup_dummy_sourcelayers(dpu_source_layers_t *sourcelayers, dss_intfinfo_t *inputsource_info,
	dss_intfinfo_t *displaysink_info)
{
	struct dss_overlay *overlay_frame;
	struct dss_overlay_block *ov_blocks;
	struct dss_layer *layer;
	struct dss_block_info *block_info;
	struct dss_img *image;
	uint32_t size;

	disp_pr_info(" enter ++++\n");

	overlay_frame = &sourcelayers->ov_frame;
	memset_s(overlay_frame, sizeof(*overlay_frame), 0, sizeof(*overlay_frame));

	overlay_frame->wb_layer_nums = 0;
	overlay_frame->wb_enable = 0;
	overlay_frame->ov_block_nums = 1;
	overlay_frame->ov_block_infos_ptr = NULL;
	overlay_frame->ovl_idx = ONLINE_OVERLAY_ID_EXTERNAL1;
	overlay_frame->release_fence = -1;

	size = overlay_frame->ov_block_nums * sizeof(*ov_blocks);
	ov_blocks = kzalloc(size, GFP_ATOMIC);
	if (!ov_blocks) {
		disp_pr_err("ov_h_block_infos is null");
		return -1;
	}

	sourcelayers->ov_blocks = ov_blocks;

	ov_blocks->layer_nums = 1;
	dss_rect_init(&ov_blocks->ov_block_rect, 0, 0, 0, 0);

	layer = &ov_blocks->layer_infos[0];
	layer->transform = 0;
	dss_rect_init(&layer->src_rect, 0, 0, inputsource_info->xres, inputsource_info->yres);
	dss_rect_init(&layer->dst_rect, 0, 0, displaysink_info->xres, displaysink_info->yres);
	layer->blending = 0;
	layer->glb_alpha = 255;
	layer->color = 0xff000000;
	layer->layer_idx = 0;
	layer->need_cap = 0;
	layer->acquire_fence = -1;
	set_dss_layer_src_type(0, LAYER_SRC_TYPE_DPRX);

	image = &layer->img;
	image->format = inputsource_info->format;
	image->width = inputsource_info->xres;
	image->height = inputsource_info->yres;

	// FIXME get it from format
	image->bpp = 4; // size of inputsource format
	image->stride = 4 * inputsource_info->xres;

	image->buf_size = inputsource_info->xres * inputsource_info->yres * image->bpp;
	image->shared_fd = 0;

	disp_pr_info(" exit ----\n");

	return 0;
}

static void dss_free_dummy_sourcelayers(dpu_source_layers_t *sourcelayers)
{
	disp_pr_info(" enter ++++\n");

	kfree(sourcelayers->ov_blocks);
	sourcelayers->ov_blocks = NULL;

	disp_pr_info(" exit ----\n");
}

static void dss_present_sourcelayers(dpu_source_layers_t *sourcelayers)
{
	struct hisi_fb_info *hfb_info = NULL;

	disp_pr_info(" enter ++++\n");

	if (!g_fpga_context.save_external_pdev) {
		disp_pr_err("external pdev hadn't been saved yet\n");
		return;
	}

	hfb_info = platform_get_drvdata(g_fpga_context.save_external_pdev);
	if (hisi_fb_open(hfb_info->fbi_info, 0)) {
		disp_pr_err("failed to open devices\n");
		return;
	}

	hisi_fb_adaptor_online_play(hfb_info, sourcelayers);

	hisi_fb_release(hfb_info->fbi_info, 0);

	disp_pr_info(" exit ----\n");
}

void _dss_setup_pipeline(uint32_t inputsource, uint32_t displaysink)
{
	struct dpu_source_layers sourcelayers;
	struct dpu_dispintf_info inputsource_info;
	struct dpu_dispintf_info displaysink_info;

	_dss_power_up(displaysink);

	dss_enable_inputsource(inputsource);

	// FIXME: DPRX & DPTX PHY coupled in PHY reset, this is a work-around
	dss_enable_displaysink(displaysink, inputsource);

	if (dss_is_external_inputsource(inputsource)) {
		dss_get_displaysink_info(displaysink, &displaysink_info);
		dss_get_inputsource_info(inputsource, &inputsource_info);
		if (dss_setup_dummy_sourcelayers(&sourcelayers, &inputsource_info, &displaysink_info)) {
			disp_pr_err("failed to setup sourcelayers\n");
			return;
		}

		dss_present_sourcelayers(&sourcelayers);
		dss_free_dummy_sourcelayers(&sourcelayers);

		dss_monitor_queue(&g_fpga_context.monitor_ctx, CONTEXT_READY);
	}
}

int32_t dss_setup_pipeline(uint32_t inputsource, uint32_t displaysink)
{
	disp_pr_info(" enter ++++\n");

	if (!dss_is_valid_inputsource(inputsource)) {
		disp_pr_err("invalid inputsource: %d\n", inputsource);
		return -1;
	}

	if (!dss_is_valid_displaysink(displaysink)) {
		disp_pr_err("invalid displaysink: %d\n", displaysink);
		return -1;
	}

	if (g_fpga_context.dpu_state == STATE_ON) {
		disp_pr_info("pipeline[%d,%d] is on, turn it off first\n",
			g_fpga_context.cur_inputsource, g_fpga_context.cur_displaysink);
		return -1;
	}

	_dss_setup_pipeline(inputsource, displaysink);

	disp_pr_info(" exit ----\n");

	return 0;
}
EXPORT_SYMBOL_GPL(dss_setup_pipeline);

static void _dss_reset_system(void)
{
	dss_disable_inputsource(g_fpga_context.cur_inputsource);
	dss_disable_displaysink(g_fpga_context.cur_displaysink);
	_dss_power_down();
}

int32_t dss_reset_system(void)
{
	disp_pr_info(" enter ++++\n");

	if (g_fpga_context.dpu_state == STATE_OFF) {
		disp_pr_info("dss already off\n");
		return -1;
	}

	_dss_reset_system();

	disp_pr_info(" exit ----\n");
	return 0;
}
EXPORT_SYMBOL_GPL(dss_reset_system);

int32_t dss_power_up(uint32_t displaysink)
{
	disp_pr_info(" enter ++++\n");

	if (!dss_is_valid_displaysink(displaysink)) {
		disp_pr_err("invalid displaysink: %d\n", displaysink);
		return -1;
	}

	if (g_fpga_context.dpu_state == STATE_ON) {
		disp_pr_warn("already powred up\n");
		return -1;
	}

	_dss_power_up(displaysink);

	disp_pr_info(" exit ----\n");
	return 0;
}
EXPORT_SYMBOL_GPL(dss_power_up);

int32_t dss_power_down(void)
{
	disp_pr_info(" enter ++++\n");

	if (g_fpga_context.dpu_state == STATE_OFF) {
		disp_pr_warn("already powered off\n");
		return -1;
	}

	_dss_power_down();

	disp_pr_info(" exit ----\n");
	return 0;
}
EXPORT_SYMBOL_GPL(dss_power_down);

uint32_t dss_set_autoreconnect(uint32_t flag)
{
	g_auto_reconnect = (flag != 0);

	disp_pr_info("set auto-reconnet %d\n", g_auto_reconnect);
	return 0;
}
EXPORT_SYMBOL_GPL(dss_set_autoreconnect);

static void dss_recover_pipeline(dss_fpga_context_t *fpga_ctx)
{
	uint32_t save_source;
	uint32_t save_sink;
	disp_pr_info("reset pipeline\n");
	save_source = fpga_ctx->cur_inputsource;
	save_sink = fpga_ctx->cur_displaysink;

	dss_reset_monitor(&fpga_ctx->monitor_ctx);

	dss_reset_system();

	dss_setup_pipeline(save_source, save_sink);
}

static int dss_monitor_proc(void *cookie, uint32_t msg_id)
{
	dss_fpga_context_t *fpga_ctx = (dss_fpga_context_t *)cookie;
	dss_monitor_context_t *monitor_ctx = &fpga_ctx->monitor_ctx;

	disp_pr_info("++++\n");
	disp_pr_info("received msg : %d\n", msg_id);

	if (!g_auto_reconnect) {
		disp_pr_info("no need to reconenct pipeline\n");
		return 0;
	}

	switch (msg_id) {
	case INPUTSOURCE_CONNECTED:
		disp_pr_info("[event] rx connected\n");
		monitor_ctx->rx_on = true;
		if (!monitor_ctx->video_on && monitor_ctx->tx_on && monitor_ctx->context_ready) {
			dss_enable_videostream(fpga_ctx->cur_inputsource, true);
			monitor_ctx->video_on = true;
		}
		break;
	case INPUTSOURCE_DISCONNECTED:
		disp_pr_info("[event] rx disconnected\n");
		monitor_ctx->rx_on = false;
		if (monitor_ctx->video_on)
			dss_recover_pipeline(fpga_ctx);
		break;
	case DISPLAYSINK_CONNECTED:
		disp_pr_info("[event] tx connected\n");
		monitor_ctx->tx_on = true;
		if (!monitor_ctx->video_on && monitor_ctx->rx_on && monitor_ctx->context_ready) {
			dss_enable_videostream(fpga_ctx->cur_inputsource, true);
			monitor_ctx->video_on = true;
		}
		break;
	case DISPLAYSINK_DISCONNECTED:
		monitor_ctx->tx_on = false;
		disp_pr_info("[event] tx disconnected\n");
		if (monitor_ctx->video_on)
			dss_recover_pipeline(fpga_ctx);
		break;
	case DISPLAYSINK_VIDEO_ACTIVATED:
		disp_pr_info("[event] video stream activated\n");
		break;
	case CONTEXT_READY:
		disp_pr_info("[event] context is ready\n");
		monitor_ctx->context_ready = true;
		if (!monitor_ctx->video_on && monitor_ctx->rx_on && monitor_ctx->tx_on) {
			dss_enable_videostream(fpga_ctx->cur_inputsource, true);
			monitor_ctx->video_on = true;
		}
		break;
	}

	disp_pr_info("----\n");

	return 0;
}

static enum hrtimer_restart dss_monitor_hrtimer_proc(struct hrtimer *timer)
{
	disp_pr_info("[info dump] rx[%s], tx[%s], video[%s]\n", g_fpga_context.monitor_ctx.rx_on ? "on" : "off",
			g_fpga_context.monitor_ctx.tx_on ? "on" : "off", g_fpga_context.monitor_ctx.video_on ? "on" : "off");

	hrtimer_start(&g_fpga_context.hrtimer, ktime_set(CHECK_TIME_PERIOD, 0), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

void dss_init_fpgacontext(void)
{
	disp_pr_info("init fpga context\n");

	g_auto_reconnect = false;

	g_fpga_context.dpu_state = STATE_ON;
	g_fpga_context.dsd_state = STATE_OFF;

	g_fpga_context.cur_displaysink = DISPLAY_SINK_DSI0;
	g_fpga_context.cur_inputsource = INPUT_SOURCE_DDR;

	g_fpga_context.save_primary_pdev = NULL;
	g_fpga_context.save_external_pdev = NULL;

	hrtimer_init(&g_fpga_context.hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_fpga_context.hrtimer.function = dss_monitor_hrtimer_proc;
	hrtimer_start(&g_fpga_context.hrtimer, ktime_set(CHECK_TIME_PERIOD, 0), HRTIMER_MODE_REL);

	dss_create_monitor(&g_fpga_context.monitor_ctx, dss_monitor_proc, &g_fpga_context);
}

void dss_deinit_fpgacontext(void)
{
	disp_pr_info("deinit fpga context\n");

	hrtimer_cancel(&g_fpga_context.hrtimer);

	dss_destroy_monitor(&g_fpga_context.monitor_ctx);
}
