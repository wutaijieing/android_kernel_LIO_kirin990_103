/*
 * Copyright (c) 2016 Synopsys, Inc.
 *
 * Synopsys DP TX Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTerrUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * Copyright (c) 2017 Hisilicon Tech. Co., Ltd. Integrated into the Hisilicon display system.
 */

#include "controller/dp_core_interface.h"
#include "controller/dp_avgen_base.h"
#include "dp_link_training.h"
#include "dp_irq.h"
#include "dp_edid.h"
#include "dp_mst_topology.h"
#include "dp_dsc_algorithm.h"
#include "controller/hidptx/hidptx_reg.h"
#include "controller/hidptx/hidptx_dp_irq.h"
// TODO:
//#include "hdcp/hdcp_common.h"

/*lint -save -e* */
#define SAFE_MODE_TIMING_HACTIVE 640
#define SAFE_MODE_TIMING_PIXEL_CLOCK 2517  /* The pixel clock of 640 * 480 = 25175. The saving pixel clock need 1/10.*/
#define DPTX_CHECK_TIME_PERIOD 2000
#define PREFerrED_FPS_OF_VR_MODE 70

#ifdef CONFIG_DPU_FB_V501
#define VBLANKING_MAX 32768
#else
#define VBLANKING_MAX 255
#endif

uint32_t g_max_line_cnt = 656; // = (maxfps(noVrr) / minfps(inVrr)) * vtotal
uint32_t g_enable_adpative_sync = 0;

uint32_t g_min_interval = 12; // ms
uint32_t g_max_interval = 20; // ms
static uint32_t cnt_fresh = 0;
static uint32_t adaptive_interval = 16;
uint32_t g_out_adpative_sync = 0;

void test_adaptive_sync_val(uint32_t max_line_cnt, uint32_t enable_adpative_sync, \
							uint32_t min_interval, uint32_t max_interval)
{
	g_max_line_cnt = max_line_cnt;
	g_enable_adpative_sync = enable_adpative_sync;
	g_min_interval = min_interval;
	g_max_interval = max_interval;
	adaptive_interval = g_min_interval;
	disp_pr_info("[DP] test_adaptive_sync_val:%d,%d,%d,%d \n", \
				max_line_cnt, enable_adpative_sync, min_interval, max_interval);
}
EXPORT_SYMBOL_GPL(test_adaptive_sync_val);

void output_adaptive_sync_en(uint32_t out_adpative_sync)
{
	g_out_adpative_sync = out_adpative_sync;
}
EXPORT_SYMBOL_GPL(output_adaptive_sync_en);

static enum hrtimer_restart test_adpative_sync_handler(struct hrtimer *timer)
{
	struct dp_ctrl *dptx = NULL;
	uint32_t phyifctrl;
	disp_check_and_return((timer == NULL), HRTIMER_NORESTART, err, "[DP] timer is NULL!\n");

	dptx = container_of(timer, struct dp_ctrl, dptx_hrtimer);

	disp_check_and_return((dptx == NULL), HRTIMER_NORESTART, err, "[DP] dptx is NULL!\n");

	if ((!dptx->dptx_enable) || (!dptx->video_transfer_enable))
		return HRTIMER_NORESTART;

	phyifctrl = dptx_readl(dptx, 0x90C);
	phyifctrl |= BIT(1);
	dptx_writel(dptx, 0x90C, phyifctrl);
	cnt_fresh++;
	if (cnt_fresh % 500 == 0) {
		set_dp_irq_cnt_clear();
		adaptive_interval++;
		if (adaptive_interval >= g_max_interval)
			adaptive_interval = g_min_interval;
		disp_pr_info("[DP] Vsync interval = %u ms\n", adaptive_interval);
	}

	hrtimer_start(&dptx->dptx_hrtimer, ktime_set(0, adaptive_interval * 1000000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

void config_adaptive_sync(struct dp_ctrl *dptx, uint32_t max_line_cnt)
{
	int retval;
	uint32_t phyifctrl;
	uint8_t byte = 0;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, ADAPTIVE_SYNC_CAPABLITY, &byte);
	if (byte & ADAPTIVE_SYNC_SDP_SUPPORTED) {
		retval = dptx_read_dpcd(dptx, DP_DOWN_STREAM_PORT_COUNT, &byte);
		if (byte & DP_MSA_TIMING_PAR_IGNORED) {
			phyifctrl = dptx_readl(dptx, DPTX_EDP_CTRL0);
			phyifctrl |= DPTX_CFG_ADPTIVE_SYNC_SDP_TX_ENABLE;
			dptx_writel(dptx, DPTX_EDP_CTRL0, phyifctrl);
		}
	}

	retval = dptx_read_dpcd(dptx, DP_DOWNSPREAD_CTRL, &byte);
	byte |= DP_MSA_TIMING_PAR_IGNORE_EN;
	disp_pr_info("config_adaptive_sync byte = 0x%x \n", byte);
	retval = dptx_write_dpcd(dptx, DP_DOWNSPREAD_CTRL, byte);
	if (retval) {
		disp_pr_err("[DP] write DPCD error\n");
		return;
	}

	phyifctrl = dptx_readl(dptx, DPTX_LP_VBK_CTRL);
	phyifctrl &= ~GENMASK(21, 2);
	phyifctrl |= max_line_cnt << 2;
	phyifctrl |= 0x1;
	disp_pr_info("config_adaptive_sync phyifctrl : 0x%x max_line_cnt : 0x%x \n", phyifctrl, max_line_cnt);
	dptx_writel(dptx, DPTX_LP_VBK_CTRL, phyifctrl);


	set_dp_irq_cnt_clear();
}

static enum hrtimer_restart dptx_detect_hrtimer_fnc(struct hrtimer *timer)
{
	struct dp_ctrl *dptx = NULL;

	disp_check_and_return((timer == NULL), HRTIMER_NORESTART, err, "[DP] timer is NULL!\n");

	dptx = container_of(timer, struct dp_ctrl, dptx_hrtimer);

	disp_check_and_return((dptx == NULL), HRTIMER_NORESTART, err, "[DP] dptx is NULL!\n");

	if (dptx->dptx_check_wq != NULL)
		queue_work(dptx->dptx_check_wq, &(dptx->dptx_check_work));

	hrtimer_start(&dptx->dptx_hrtimer, ktime_set(DPTX_CHECK_TIME_PERIOD / 1000,
		(DPTX_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static bool dptx_check_vr_err_count(struct dp_ctrl *dptx)
{
	uint8_t vector, vector1;
	uint16_t lane0_err;
	uint16_t lane1_err;
	uint16_t lane2_err;
	uint16_t lane3_err;
	int retval;

	retval = 0;
	vector = vector1 = 0;
	lane0_err = lane1_err = lane2_err = lane3_err = 0;

	if ((dptx->video_transfer_enable) && (dptx->dptx_vr)) {
		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE0_L, &vector);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE0_H, &vector1);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}
		vector1 &= 0x7F;
		lane0_err = ((vector1 << 8) | vector);

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE1_L, &vector);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE1_H, &vector1);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}
		vector1 &= 0x7F;
		lane1_err = ((vector1 << 8) | vector);

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE2_L, &vector);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE2_H, &vector1);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}
		vector1 &= 0x7F;
		lane2_err = ((vector1 << 8) | vector);

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE3_L, &vector);
		if (retval) {
			disp_pr_err("Read DPCD error\n");
			return false;
		}

		retval = dptx_read_dpcd(dptx, DP_SYMBOL_ERROR_COUNT_LANE3_H, &vector1);
		if (retval) {
			disp_pr_err("[DP] Read DPCD error\n");
			return false;
		}
		vector1 &= 0x7F;
		lane3_err = ((vector1 << 8) | vector);

		if (lane0_err || lane1_err || lane2_err || lane3_err) {
			disp_pr_err("[DP] Lane x err count: (0x%x); (0x%x); (0x%x); (0x%x).\n",
			lane0_err, lane1_err, lane2_err, lane3_err);
			huawei_dp_imonitor_set_param_err_count(lane0_err, lane1_err, lane2_err, lane3_err);
			return false;
		}
	}

	return true;
}

static void dptx_err_count_check_wq_handler(struct work_struct *work)
{
	struct dp_ctrl *dptx = NULL;
	bool berr = false;

	dptx = container_of(work, struct dp_ctrl, dptx_check_work);

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	if ((!dptx->dptx_enable) || (!dptx->video_transfer_enable))
		return;

	berr = dptx_check_vr_err_count(dptx);

	/* dptx need check err count when video has been transmited on wallex.
	 * The first checking result should be discarded, and then, we need report
	 * the bad message when the err count has been detected by 3 times in
	 * a row. The time interval of detecting is 3 second.
	 */

	if ((!berr) && (dptx->detect_times >= 1))
		dptx->detect_times++;
	else if (berr)
		dptx->detect_times = 1;
	else
		dptx->detect_times++;

#if CONFIG_DP_ENABLE
	if (dptx->detect_times == 4) {
		huawei_dp_send_event(DP_LINK_STATE_BAD);
		dptx->detect_times = 1;
		disp_pr_info("\n [DP] err count upload!");
	}
#endif
}

static int dptx_init_detect_work(struct dp_ctrl *dptx)
{
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	disp_pr_info("[DP] Init Detect work\n");

	if (!dptx->dptx_detect_inited) {
		dptx->dptx_check_wq = create_singlethread_workqueue("dptx_check");
		if (dptx->dptx_check_wq == NULL) {
			disp_pr_err("[DP] create dptx_check_wq failed\n");
			return -1;
		}

		INIT_WORK(&dptx->dptx_check_work, dptx_err_count_check_wq_handler);

		/* hrtimer for detecting error count*/
		hrtimer_init(&dptx->dptx_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		if (g_out_adpative_sync) {
			dptx->dptx_hrtimer.function = test_adpative_sync_handler;
		} else {
			dptx->dptx_hrtimer.function = dptx_detect_hrtimer_fnc;
		}
		hrtimer_start(&dptx->dptx_hrtimer, ktime_set(DPTX_CHECK_TIME_PERIOD / 1000,
			(DPTX_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);

		dptx->dptx_detect_inited = true;
	}
	return 0;
}

static int dptx_cancel_detect_work(struct dp_ctrl *dptx)
{
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	disp_pr_info("[DP] Cancel Detect work\n");

	if (dptx->dptx_detect_inited) {
		if (dptx->dptx_check_wq != NULL) {
			destroy_workqueue(dptx->dptx_check_wq);
			dptx->dptx_check_wq = NULL;
		}

		hrtimer_cancel(&dptx->dptx_hrtimer);

		dptx->dptx_detect_inited = false;
	}

	return 0;
}

static int handle_test_link_training(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t lanes = 0;
	uint8_t rate = 0;
	struct video_params *vparams = NULL;
	struct dtd *mdtd = NULL;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, DP_TEST_LINK_RATE, &rate);
	if (retval)
		return retval;

	retval = dptx_bw_to_phy_rate(rate);
	if (retval < 0)
		return retval;

	rate = retval;

	retval = dptx_read_dpcd(dptx, DP_TEST_LANE_COUNT, &lanes);
	if (retval)
		return retval;

	disp_pr_info("[DP] %s: Strating link training rate=%d, lanes=%d\n",
		 __func__, rate, lanes);

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;

	if (dptx->dptx_video_ts_calculate)
		retval = dptx->dptx_video_ts_calculate(dptx, lanes, rate,
					 vparams->bpc, vparams->pix_enc,
					 mdtd->pixel_clock);
	if (retval)
		return retval;

	retval = dptx_link_training(dptx,
				    rate,
				    lanes);
	if (retval)
		disp_pr_err("[DP] Link training failed %d\n", retval);
	else
		disp_pr_info("[DP] Link training succeeded\n");

	return retval;
}

static int handle_test_get_video_mode(struct test_dtd tdtd, enum video_format_type *video_format, uint8_t *vmode)
{
	int i;

	disp_check_and_return((video_format == NULL), -EINVAL, err, "[DP] video_format is NULL!\n");
	disp_check_and_return((vmode == NULL), -EINVAL, err, "[DP] vmode is NULL!\n");

	*vmode = 0;
	for (i = 0; i < ARRAY_SIZE(test_timing); i++) {
		struct test_dtd tmp = test_timing[i];

		if (tdtd.h_total == tmp.h_total && tdtd.v_total == tmp.v_total && tdtd.h_start == tmp.h_start &&
			tdtd.v_start == tmp.v_start && tdtd.h_sync_width == tmp.h_sync_width
			&& tdtd.v_sync_width == tmp.v_sync_width &&
			tdtd.h_width == tmp.h_width && tdtd.v_width == tmp.v_width) {
			*video_format = tmp.video_format;
			*vmode = tmp.vmode;
			break;
		}
	}

	if (*video_format == DMT && *vmode == 82 && tdtd.refresh_rate == 120000) {
		*vmode = 63;
		*video_format = VCEA;
	}

	if (*vmode == 0) {
		disp_pr_info("[DP] Unknown video mode\n");
		return -EINVAL;
	}

	return 0;
}

static int handle_test_read_video_timing(struct dp_ctrl *dptx, struct test_dtd *tdtd)
{
	int retval;
	uint8_t bytes[19] = {0};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((tdtd == NULL), -EINVAL, err, "[DP] tdtd is NULL!\n");

	retval = dptx_read_bytes_from_dpcd(dptx, DP_TEST_H_TOTAL_MSB, bytes, sizeof(bytes));
	if (retval)
		return retval;

	tdtd->h_total = (bytes[0] << 8) | bytes[1];
	tdtd->v_total = (bytes[2] << 8) | bytes[3];
	tdtd->h_start = (bytes[4] << 8) | bytes[5];
	tdtd->v_start = (bytes[6] << 8) | bytes[7];
	tdtd->h_sync_width = ((bytes[8] & (~(1 << 7))) << 8) | bytes[9];
	tdtd->h_sync_pol = bytes[8]  >> 7;
	tdtd->v_sync_width = ((bytes[10] & (~(1 << 7))) << 8) | bytes[11];
	tdtd->v_sync_pol = bytes[10]  >> 7;
	tdtd->h_width = (bytes[12] << 8) | bytes[13];
	tdtd->v_width = (bytes[14] << 8) | bytes[15];
	tdtd->refresh_rate = bytes[18] * 1000;

	disp_pr_info("[DP] h_total = %d\n", tdtd->h_total);
	disp_pr_info("[DP] v_total = %d\n", tdtd->v_total);
	disp_pr_info("[DP] h_start = %d\n", tdtd->h_start);
	disp_pr_info("[DP] v_start = %d\n", tdtd->v_start);
	disp_pr_info("[DP] h_sync_width = %d\n", tdtd->h_sync_width);
	disp_pr_info("[DP] h_sync_pol = %d\n", tdtd->h_sync_pol);
	disp_pr_info("[DP] v_sync_width = %d\n", tdtd->v_sync_width);
	disp_pr_info("[DP] v_sync_pol = %d\n", tdtd->v_sync_pol);
	disp_pr_info("[DP] h_width = %d\n", tdtd->h_width);
	disp_pr_info("[DP] v_width = %d\n", tdtd->v_width);
	disp_pr_info("[DP] refresh_rate = %d\n", tdtd->refresh_rate);

	return 0;
}

static int handle_test_link_video_timming(struct dp_ctrl *dptx, int stream)
{
	int retval;
	enum video_format_type video_format;
	uint8_t vmode;
	struct video_params *vparams = NULL;
	struct dtd mdtd;
	struct test_dtd tdtd;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	memset(&tdtd, 0, sizeof(tdtd));
	retval = handle_test_read_video_timing(dptx, &tdtd);
	if (retval)
		return retval;

	video_format = DMT;
	retval = handle_test_get_video_mode(tdtd, &video_format, &vmode);
	if (retval)
		return retval;

	if (!convert_code_to_dtd(&mdtd, vmode, tdtd.refresh_rate, video_format)) {
		disp_pr_info("[DP] %s: Invalid video mode value %d\n", __func__, vmode);
		retval = -EINVAL;
		return retval;
	}

	vparams = &dptx->vparams;
	vparams->mdtd = mdtd;
	vparams->refresh_rate = tdtd.refresh_rate;
	if (dptx->dptx_video_ts_calculate)
		retval = dptx->dptx_video_ts_calculate(dptx, dptx->link.lanes,
					 dptx->link.rate, vparams->bpc,
					 vparams->pix_enc, mdtd.pixel_clock);
	if (retval)
		return retval;
	/* MMCM */
	if (dptx->dptx_resolution_switch)
		dptx->dptx_resolution_switch(dptx, HOT_PLUG_TEST);
	if (dptx->dptx_video_timing_change)
		dptx->dptx_video_timing_change(dptx, 0);

	return 0;
}

static int handle_test_get_audio_channel_count(struct dp_ctrl *dptx, uint8_t test_audio_ch_count)
{
	struct audio_params *aparams = NULL;
	uint8_t  audio_ch_count;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	aparams = &dptx->aparams;
	switch (test_audio_ch_count) {
	case DP_TEST_AUDIO_CHANNEL1:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL1\n");
		audio_ch_count = 1;
		break;
	case DP_TEST_AUDIO_CHANNEL2:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL2\n");
		audio_ch_count = 2;
		break;
	case DP_TEST_AUDIO_CHANNEL3:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL3\n");
		audio_ch_count = 3;
		break;
	case DP_TEST_AUDIO_CHANNEL4:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL4\n");
		audio_ch_count = 4;
		break;
	case DP_TEST_AUDIO_CHANNEL5:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL5\n");
		audio_ch_count = 5;
		break;
	case DP_TEST_AUDIO_CHANNEL6:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL6\n");
		audio_ch_count = 6;
		break;
	case DP_TEST_AUDIO_CHANNEL7:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL7\n");
		audio_ch_count = 7;
		break;
	case DP_TEST_AUDIO_CHANNEL8:
		disp_pr_info("[DP] DP_TEST_AUDIO_CHANNEL8\n");
		audio_ch_count = 8;
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_AUDIO_CHANNEL_COUNT\n");
		return -EINVAL;
	}
	disp_pr_info("[DP] test_audio_ch_count = %d\n", audio_ch_count);
	aparams->num_channels = audio_ch_count;

	return 0;
}

static int handle_test_get_audio_sample_params(struct dp_ctrl *dptx, uint8_t test_audio_smaple_range)
{
	struct audio_params *aparams = NULL;
	uint8_t  orig_sample_freq, sample_freq;
	uint32_t audio_clock_freq;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	aparams = &dptx->aparams;
	switch (test_audio_smaple_range) {
	case DP_TEST_AUDIO_SAMPLING_RATE_32:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_32\n");
		orig_sample_freq = 12;
		sample_freq = 3;
		audio_clock_freq = 320;
		break;
	case DP_TEST_AUDIO_SAMPLING_RATE_44_1:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_44_1\n");
		orig_sample_freq = 15;
		sample_freq = 0;
		audio_clock_freq = 441;
		break;
	case DP_TEST_AUDIO_SAMPLING_RATE_48:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_48\n");
		orig_sample_freq = 13;
		sample_freq = 2;
		audio_clock_freq = 480;
		break;
	case DP_TEST_AUDIO_SAMPLING_RATE_88_2:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_88_2\n");
		orig_sample_freq = 7;
		sample_freq = 8;
		audio_clock_freq = 882;
		break;
	case DP_TEST_AUDIO_SAMPLING_RATE_96:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_96\n");
		orig_sample_freq = 5;
		sample_freq = 10;
		audio_clock_freq = 960;
		break;
	case DP_TEST_AUDIO_SAMPLING_RATE_176_4:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_176_4\n");
		orig_sample_freq = 3;
		sample_freq = 12;
		audio_clock_freq = 1764;
		break;
	case DP_TEST_AUDIO_SAMPLING_RATE_192:
		disp_pr_info("[DP] DP_TEST_AUDIO_SAMPLING_RATE_192\n");
		orig_sample_freq = 1;
		sample_freq = 14;
		audio_clock_freq = 1920;
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_AUDIO_SAMPLING_RATE\n");
		return -EINVAL;
	}
	disp_pr_info("[DP] sample_freq = %d\n", sample_freq);
	disp_pr_info("[DP] orig_sample_freq = %d\n", orig_sample_freq);

	aparams->iec_samp_freq = sample_freq;
	aparams->iec_orig_samp_freq = orig_sample_freq;

	return 0;
}

static int handle_test_link_audio_pattern(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t test_audio_mode = 0;
	uint8_t test_audio_smaple_range;
	uint8_t test_audio_ch_count;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, DP_TEST_AUDIO_MODE, &test_audio_mode);
	if (retval)
		return retval;

	disp_pr_info("[DP] test_audio_mode= %d\n", test_audio_mode);

	test_audio_smaple_range = test_audio_mode &
		DP_TEST_AUDIO_SAMPLING_RATE_MASK;
	test_audio_ch_count = (test_audio_mode & DP_TEST_AUDIO_CH_COUNT_MASK)
		>> DP_TEST_AUDIO_CH_COUNT_SHIFT;

	retval = handle_test_get_audio_channel_count(dptx, test_audio_ch_count);
	if (retval)
		return retval;

	retval = handle_test_get_audio_sample_params(dptx, test_audio_smaple_range);
	if (retval)
		return retval;

	if (dptx->dptx_audio_num_ch_change)
		dptx->dptx_audio_num_ch_change(dptx);
	if (dptx->dptx_audio_infoframe_sdp_send)
		dptx->dptx_audio_infoframe_sdp_send(dptx);

	return retval;
}

static int calc_dynamic_range_map(uint8_t dynamic_range, uint8_t *dynamic_range_map)
{
	switch (dynamic_range) {
	case DP_TEST_DYNAMIC_RANGE_VESA:
		disp_pr_info("[DP] DP_TEST_DYNAMIC_RANGE_VESA\n");
		*dynamic_range_map = VESA;
		break;
	case DP_TEST_DYNAMIC_RANGE_CEA:
		disp_pr_info("[DP] DP_TEST_DYNAMIC_RANGE_CEA\n");
		*dynamic_range_map = CEA;
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_BIT_DEPTH\n");
		return -EINVAL;
	}

	return 0;
}

static int calc_ycbcr_coeff_map(uint8_t ycbcr_coeff, uint8_t *ycbcr_coeff_map)
{
	switch (ycbcr_coeff) {
	case DP_TEST_YCBCR_COEFF_ITU601:
		disp_pr_info("[DP] DP_TEST_YCBCR_COEFF_ITU601\n");
		*ycbcr_coeff_map = ITU601;
		break;
	case DP_TEST_YCBCR_COEFF_ITU709:
		disp_pr_info("[DP] DP_TEST_YCBCR_COEFF_ITU709:\n");
		*ycbcr_coeff_map = ITU709;
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_BIT_DEPTH\n");
		return -EINVAL;
	}

	return 0;
}

static int calc_color_format_map(uint8_t color_format, uint8_t *color_format_map)
{
	switch (color_format) {
	case DP_TEST_COLOR_FORMAT_RGB:
		disp_pr_info("[DP] DP_TEST_COLOR_FORMAT_RGB\n");
		*color_format_map = RGB;
		break;
	case DP_TEST_COLOR_FORMAT_YCBCR422:
		disp_pr_info("[DP] DP_TEST_COLOR_FORMAT_YCBCR422\n");
		*color_format_map = YCBCR422;
		break;
	case DP_TEST_COLOR_FORMAT_YCBCR444:
		disp_pr_info("[DP] DP_TEST_COLOR_FORMAT_YCBCR444\n");
		*color_format_map = YCBCR444;
		break;
	default:
		disp_pr_info("[DP] Invalid  DP_TEST_COLOR_FORMAT\n");
		return -EINVAL;
	}

	return 0;
}

static int calc_bpc_map(uint8_t bpc, uint8_t *bpc_map)
{
	switch (bpc) {
	case DP_TEST_BIT_DEPTH_6:
		*bpc_map = COLOR_DEPTH_6;
		disp_pr_info("[DP] TEST_BIT_DEPTH_6\n");
		break;
	case DP_TEST_BIT_DEPTH_8:
		*bpc_map = COLOR_DEPTH_8;
		disp_pr_info("[DP] TEST_BIT_DEPTH_8\n");
		break;
	case DP_TEST_BIT_DEPTH_10:
		*bpc_map = COLOR_DEPTH_10;
		disp_pr_info("[DP] TEST_BIT_DEPTH_10\n");
		break;
	case DP_TEST_BIT_DEPTH_12:
		*bpc_map = COLOR_DEPTH_12;
		disp_pr_info("[DP] TEST_BIT_DEPTH_12\n");
		break;
	case DP_TEST_BIT_DEPTH_16:
		*bpc_map = COLOR_DEPTH_16;
		disp_pr_info("[DP] TEST_BIT_DEPTH_16\n");
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_BIT_DEPTH\n");
		return -EINVAL;
	}

	return 0;
}

static int handle_test_link_video_pattern(struct dp_ctrl *dptx, int stream)
{
	int retval;
	uint8_t misc = 0;
	uint8_t pattern = 0;
	uint8_t bpc;
	uint8_t bpc_map = 0;
	uint8_t dynamic_range;
	uint8_t dynamic_range_map = 0;
	uint8_t color_format;
	uint8_t color_format_map = 0;
	uint8_t ycbcr_coeff;
	uint8_t ycbcr_coeff_map = 0;
	struct video_params *vparams = NULL;
	struct dtd *mdtd = NULL;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;
	retval = 0;

	retval = dptx_read_dpcd(dptx, DP_TEST_PATTERN, &pattern);
	if (retval)
		return retval;
	retval = dptx_read_dpcd(dptx, DP_TEST_MISC, &misc);
	if (retval)
		return retval;

	dynamic_range = (misc & DP_TEST_DYNAMIC_RANGE_MASK)
			>> DP_TEST_DYNAMIC_RANGE_SHIFT;
	retval = calc_dynamic_range_map(dynamic_range, &dynamic_range_map);
	if (retval)
		return retval;

	ycbcr_coeff = (misc & DP_TEST_YCBCR_COEFF_MASK)
			>> DP_TEST_YCBCR_COEFF_SHIFT;
	retval = calc_ycbcr_coeff_map(ycbcr_coeff, &ycbcr_coeff_map);
	if (retval)
		return retval;

	color_format = misc & DP_TEST_COLOR_FORMAT_MASK;
	retval = calc_color_format_map(color_format, &color_format_map);
	if (retval)
		return retval;

	bpc = (misc & DP_TEST_BIT_DEPTH_MASK)
		>> DP_TEST_BIT_DEPTH_SHIFT;
	retval = calc_bpc_map(bpc, &bpc_map);
	if (retval)
		return retval;

	vparams->dynamic_range = dynamic_range_map;
	disp_pr_info("[DP] Change video dynamic range to %d\n", dynamic_range_map);

	vparams->colorimetry = ycbcr_coeff_map;
	disp_pr_info("[DP] Change video colorimetry to %d\n", ycbcr_coeff_map);

	if (dptx->dptx_video_ts_calculate)
		retval = dptx->dptx_video_ts_calculate(
			dptx, dptx->link.lanes,
			dptx->link.rate,
			bpc_map,
			color_format_map,
			mdtd->pixel_clock);
	if (retval)
		return retval;

	vparams->pix_enc = color_format_map;
	disp_pr_info("[DP] Change pixel encoding to %d\n", color_format_map);

	vparams->bpc = bpc_map;

	if (dptx->dptx_video_bpc_change)
		dptx->dptx_video_bpc_change(dptx, stream);
	disp_pr_info("[DP] Change bits per component to %d\n", bpc_map);

	if (dptx->dptx_video_ts_change)
		dptx->dptx_video_ts_change(dptx, stream);

	switch (pattern) {
	case DP_TEST_PATTERN_NONE:
		disp_pr_info("[DP] TEST_PATTERN_NONE %d\n", pattern);
		break;
	case DP_TEST_PATTERN_COLOR_RAMPS:
		disp_pr_info("[DP] TEST_PATTERN_COLOR_RAMPS %d\n", pattern);
		vparams->pattern_mode = RAMP;
		disp_pr_info("[DP] Change video pattern to RAMP\n");
		break;
	case DP_TEST_PATTERN_BW_VERITCAL_LINES:
		disp_pr_info("[DP] TEST_PATTERN_BW_VERTICAL_LINES %d\n", pattern);
		break;
	case DP_TEST_PATTERN_COLOR_SQUARE:
		disp_pr_info("[DP] TEST_PATTERN_COLOR_SQUARE %d\n", pattern);
		vparams->pattern_mode = COLRAMP;
		disp_pr_info("[DP] Change video pattern to COLRAMP\n");
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_PATTERN %d\n", pattern);
		return -EINVAL;
	}

	retval = handle_test_link_video_timming(dptx, stream);
	if (retval)
		return retval;

	return 0;
}

static int handle_automated_test_request(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t test = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, DP_TEST_REQUEST, &test);
	if (retval)
		return retval;

	if (test & DP_TEST_LINK_TRAINING) {
		disp_pr_info("[DP] %s: DP_TEST_LINK_TRAINING\n", __func__);

		retval = dptx_write_dpcd(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		retval = handle_test_link_training(dptx);
		if (retval)
			return retval;
	}

	if (test & DP_TEST_LINK_VIDEO_PATTERN) {
		disp_pr_info("[DP] %s:DP_TEST_LINK_VIDEO_PATTERN\n", __func__);

		retval = dptx_write_dpcd(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		//dptx->hisifd->hpd_release_sub_fnc(dptx->hisifd->fbi);
		dp_send_cable_notification(dptx, HOT_PLUG_TEST_OUT);

		retval = handle_test_link_video_pattern(dptx, 0);
		if (retval)
			return retval;
	}

	if (test & DP_TEST_LINK_AUDIO_PATTERN) {
		disp_pr_info("[DP] %s:DP_TEST_LINK_AUDIO_PATTERN\n", __func__);

		retval = dptx_write_dpcd(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		retval = handle_test_link_audio_pattern(dptx);
		if (retval)
			return retval;
	}

	if (test & DP_TEST_LINK_EDID_READ) {
		/* Invalid, this should happen on HOTPLUG */
		disp_pr_info("[DP] %s:DP_TEST_LINK_EDID_READ\n", __func__);
		return -ENOTSUPP;
	}

	if (test & DP_TEST_LINK_PHY_TEST_PATTERN) {
		disp_pr_info("[DP] %s:DP_TEST_LINK_PHY_TEST_PATTERN\n", __func__);

		if (dptx->dptx_triger_media_transfer)
			dptx->dptx_triger_media_transfer(dptx, false);

		retval = dptx_write_dpcd(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		if (dptx->dptx_phy_set_ssc)
			dptx->dptx_phy_set_ssc(dptx, true);

		(void)dptx_link_adjust_drive_settings(dptx, NULL);   //Test only

		if (dptx->handle_test_link_phy_pattern)
			retval = dptx->handle_test_link_phy_pattern(dptx);
		if (retval)
			return retval;
	}

	return 0;
}

int handle_sink_request(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t vector = 0;
	uint8_t bytes[1] = {0};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_link_check_status(dptx);
	if (retval)
		return retval;

	retval = dptx_read_bytes_from_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes));
	if (retval)
		return retval;


	retval = dptx_read_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);
	if (retval)
		return retval;

	disp_pr_info("[DP] %s: IRQ_VECTOR: 0x%02x\n", __func__, vector);
	huawei_dp_imonitor_set_param(DP_PARAM_IRQ_VECTOR, &vector);

	/* handle sink interrupts */
	if (!vector)
		return 0;

	if ((vector | bytes[0]) & DP_REMOTE_CONTROL_COMMAND_PENDING)
		disp_pr_warn(
			  "[DP] %s: DP_REMOTE_CONTROL_COMMAND_PENDING: Not yet implemented",
			  __func__);

	if ((vector | bytes[0]) & DP_AUTOMATED_TEST_REQUEST) {
		disp_pr_info("[DP] %s: DP_AUTOMATED_TEST_REQUEST", __func__);
		retval = handle_automated_test_request(dptx);
		if (retval) {
			disp_pr_err("[DP] Automated test request failed\n");
			if (retval == -ENOTSUPP) {
				retval = dptx_write_dpcd(dptx, DP_TEST_RESPONSE, DP_TEST_NAK);
				if (retval)
					return retval;
			}
		}
	}

	if ((vector | bytes[0]) & DP_CP_IRQ) {
		disp_pr_warn("[DP] DP_CP_IRQ");
#ifdef CONFIG_DP_HDCP_ENABLE
		//hdcp_handle_cp_irq(dptx);
#endif
	}

	if ((vector | bytes[0]) & DP_MCCS_IRQ) {
		disp_pr_warn(
			  "[DP] %s: DP_MCCS_IRQ: Not yet implemented", __func__);
		retval = -ENOTSUPP;
	}

	if ((vector | bytes[0]) & DP_UP_REQ_MSG_RDY) {
		disp_pr_warn("[DP] DP_UP_REQ_MSG_RDY");
		retval = dptx_sideband_get_up_req(dptx);
		if (retval) {
			disp_pr_err("[DP]: Error reading UP REQ %d\n", retval);
			return retval;
		}
	}

	if ((vector | bytes[0]) & DP_SINK_SPECIFIC_IRQ) {
		disp_pr_warn("[DP] %s: DP_SINK_SPECIFIC_IRQ: Not yet implemented",
			  __func__);
		retval = -ENOTSUPP;
	}

	return retval;
}

int handle_hotunplug(struct dp_ctrl *dptx)
{
	uint32_t reg;

	disp_pr_info("[DP] +.\n");
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((!dptx->dptx_enable), -EINVAL, err, "[DP] dptx has already off!\n");

	dptx->video_transfer_enable = false;
	dptx->max_edid_timing_hactive = 0;
	dptx->dummy_dtds_present = false;

	dptx_cancel_detect_work(dptx);

#ifdef CONFIG_DP_HDCP_ENABLE
	//hdcp_dp_on(dptx, false);
#endif

	if (dptx->dptx_link_close_stream) {
		disp_pr_info("[DP] close stream.\n");
		dptx->dptx_link_close_stream(dptx);
	}

	release_edid_info(dptx);
	atomic_set(&dptx->sink_request, 0);
	atomic_set(&dptx->aux.event, 0);
	dptx->link.trained = false;
	dptx->dsc = false;
	dptx->fec = false;

	huawei_dp_imonitor_set_param(DP_PARAM_TIME_STOP, NULL);
	disp_pr_info("[DP] -.\n");
	reg = 0;
	return reg;
}

static int dptx_read_edid_block(struct dp_ctrl *dptx,
	unsigned int block)
{
	int retval;
	int retry = 0;

	uint8_t offset = block * EDID_BLOCK_LENGTH;
	uint8_t segment = block >> 1;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	disp_pr_info("[DP] block=%d.\n", block);
	if (segment != 0) {
		retval = dptx_write_bytes_to_i2c(dptx, 0x30, &segment, 1);
		if (retval) {
			disp_pr_err("[DP] failed to  dptx_write_bytes_to_i2c 1!\n");
			return retval;
		}
	}
	/* Skip if no E-DDC */
again:
	retval = dptx_write_bytes_to_i2c(dptx, 0x50, &offset, 1);
	if (retval) {
		disp_pr_err("[DP] failed to  dptx_write_bytes_to_i2c 2!\n");
		return retval;
	}

	retval = dptx_read_bytes_from_i2c(dptx, 0x50,
		&dptx->edid[block * EDID_BLOCK_LENGTH], EDID_BLOCK_LENGTH);
	if ((retval == -EINVAL) && !retry) {
		disp_pr_err("[DP] failed to  dptx_read_bytes_from_i2c 2!\n");
		retry = 1;
		goto again;
	}
	huawei_dp_imonitor_set_param(DP_PARAM_EDID + block, &(dptx->edid[block * DP_DSM_EDID_BLOCK_SIZE]));
	dptx_i2c_address_only(dptx, 0x50);

	return 0;
}

bool dptx_check_edid_header(struct dp_ctrl *dptx)
{
	int i;
	uint8_t *edid_t = NULL;

	disp_check_and_return((dptx == NULL), false, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx->edid == NULL), false, err, "[DP] dptx->edid is NULL!\n");

	edid_t = dptx->edid;
	for (i = 0; i < EDID_HEADER_END + 1; i++) {
		if (edid_t[i] != edid_v1_header[i]) {
			disp_pr_info("[DP] Invalide edid header\n");
			return false;
		}
	}

	return true;
}

static int dptx_get_first_edid_block(struct dp_ctrl *dptx, unsigned int *ext_block_nums)
{
	int retval = 0;
	unsigned int ext_blocks = 0;
	int edid_try_count = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx->edid == NULL), -EINVAL, err, "[DP] dptx->edid is NULL!\n");
	disp_check_and_return((ext_block_nums == NULL), -EINVAL, err, "[DP] ext_block_nums is NULL!\n");

edid_retry:
	memset(dptx->edid, 0, DPTX_DEFAULT_EDID_BUFLEN);
	retval = dptx_read_edid_block(dptx, 0);
	/* will try to read edid block again when ready edid block failed */
	if (retval) {
		if (edid_try_count <= dptx->edid_try_count) {
			disp_pr_info("[DP] Read edid block failed, try %d times\n", edid_try_count);
			mdelay(dptx->edid_try_delay);
			edid_try_count += 1;
			goto edid_retry;
		} else {
			disp_pr_err("[DP] failed to dptx_read_edid_block!\n");
			return -EINVAL;
		}
	}

	if (dptx->edid[126] > 10)
		/* Workaround for QD equipment */
		/* investigate corruptions of EDID blocks */
		ext_blocks = 2;
	else
		ext_blocks = dptx->edid[126];

	if ((ext_blocks > MAX_EXT_BLOCKS) || !dptx_check_edid_header(dptx)) {
		edid_try_count += 1;
		if (edid_try_count <= dptx->edid_try_count) {
			mdelay(dptx->edid_try_delay);
			disp_pr_info("[DP] Read edid data is not correct, try %d times\n", edid_try_count);
			goto edid_retry;
		} else {
			if (ext_blocks > MAX_EXT_BLOCKS)
				ext_blocks = MAX_EXT_BLOCKS;
		}
	}

	*ext_block_nums = ext_blocks;

	return 0;
}

static int dptx_read_edid(struct dp_ctrl *dptx)
{
	int i;
	int retval = 0;
	unsigned int ext_blocks = 0;
	uint8_t *first_edid_block = NULL;
	unsigned int edid_buf_size = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx->edid == NULL), -EINVAL, err, "[DP] dptx->edid is NULL!\n");

	retval = dptx_get_first_edid_block(dptx, &ext_blocks);
	if (retval)
		return retval;

	first_edid_block = kmalloc(EDID_BLOCK_LENGTH, GFP_KERNEL);
	if (first_edid_block == NULL) {
		disp_pr_err("[DP] Allocate buffer error\n");
		return -EINVAL;
	}
	memset(first_edid_block, 0, EDID_BLOCK_LENGTH);
	memcpy(first_edid_block, dptx->edid, EDID_BLOCK_LENGTH);
	kfree(dptx->edid);
	dptx->edid = NULL;

	dptx->edid = kzalloc(EDID_BLOCK_LENGTH * ext_blocks + EDID_BLOCK_LENGTH, GFP_KERNEL);
	if (dptx->edid == NULL) {
		disp_pr_err("[DP] Allocate edid buffer error!\n");
		retval = -EINVAL;
		goto fail;
	}

	memcpy(dptx->edid, first_edid_block, EDID_BLOCK_LENGTH);
	for (i = 1; i <= ext_blocks; i++) {
		retval = dptx_read_edid_block(dptx, i);
		if (retval)
			goto fail;
	}

	edid_buf_size = EDID_BLOCK_LENGTH * ext_blocks + EDID_BLOCK_LENGTH;
	retval = edid_buf_size;

fail:
	if (first_edid_block != NULL) {
		kfree(first_edid_block);
		first_edid_block = NULL;
	}

	return retval;
}

/*
 * convert timing info to dtd format
 */
static void dptx_convert_timing_info(struct timing_info *src_timing_info, struct dtd *dst_dtd)
{
	disp_check_and_no_retval((src_timing_info == NULL), err, "[DP] src_timing_info is NULL!\n");
	disp_check_and_no_retval((dst_dtd == NULL), err, "[DP] dst_dtd is NULL!\n");

	dst_dtd->pixel_repetition_input = 0;
	dst_dtd->pixel_clock = src_timing_info->pixelClock;

	dst_dtd->h_active = src_timing_info->hActivePixels;
	dst_dtd->h_blanking = src_timing_info->hBlanking;
	dst_dtd->h_sync_offset = src_timing_info->hSyncOffset;
	dst_dtd->h_sync_pulse_width = src_timing_info->hSyncPulseWidth;

	dst_dtd->h_image_size = src_timing_info->hSize;

	dst_dtd->v_active = src_timing_info->vActivePixels;
	dst_dtd->v_blanking = src_timing_info->vBlanking;
	dst_dtd->v_sync_offset = src_timing_info->vSyncOffset;
	dst_dtd->v_sync_pulse_width = src_timing_info->vSyncPulseWidth;

	dst_dtd->v_image_size = src_timing_info->vSize;

	dst_dtd->interlaced = src_timing_info->interlaced;

	dst_dtd->v_sync_polarity = src_timing_info->vSyncPolarity;
	dst_dtd->h_sync_polarity = src_timing_info->hSyncPolarity;

	dst_dtd->pixel_clock *= 10;
	dst_dtd->v_active = (dst_dtd->interlaced == 1) ? (dst_dtd->v_active / 2) : dst_dtd->v_active;
}

static int dptx_calc_fps(struct timing_info *dptx_timing_node, uint32_t *fps)
{
	uint32_t pixels;

	disp_check_and_return((dptx_timing_node == NULL), -EINVAL, err, "[DP] dptx_timing_node is NULL!\n");
	disp_check_and_return((fps == NULL), -EINVAL, err, "[DP] fps is NULL!\n");

	pixels = ((dptx_timing_node->hActivePixels + dptx_timing_node->hBlanking) *
		(dptx_timing_node->vActivePixels + dptx_timing_node->vBlanking));

	if (pixels == 0) {
		disp_pr_info("[DP] pixels cannot be zero\n");
		return -EINVAL;
	}

	*fps = (uint32_t)(dptx_timing_node->pixelClock * 10000 / pixels);

	return 0;
}

static bool dptx_need_update_timing(struct timing_info *dptx_timing_node,
	struct timing_info *per_timing_info, uint32_t fps, uint32_t fps_default)
{
	disp_check_and_return((dptx_timing_node == NULL), false, err, "[DP] dptx_timing_node is NULL!\n");

	if ((per_timing_info == NULL) ||
		((dptx_timing_node->hActivePixels * dptx_timing_node->vActivePixels * fps) >
		(per_timing_info->hActivePixels * per_timing_info->vActivePixels * fps_default)))
		return true;

	return false;
}

static bool dptx_need_update_timing_vr(struct dp_ctrl *dptx, struct timing_info *dptx_timing_node,
	struct timing_info *per_timing_info_vr, uint32_t fps)
{
	disp_check_and_return((dptx == NULL), false, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx_timing_node == NULL), false, err, "[DP] dptx_timing_node is NULL!\n");

	if ((dptx->dptx_vr == true) && (fps == PREFerrED_FPS_OF_VR_MODE) && ((per_timing_info_vr == NULL) ||
		((dptx_timing_node->hActivePixels * dptx_timing_node->vActivePixels) >
		(per_timing_info_vr->hActivePixels * per_timing_info_vr->vActivePixels))))
		return true;

	return false;
}


int dptx_choose_edid_timing(struct dp_ctrl *dptx, bool *bsafemode)
{
	struct timing_info *per_timing_info = NULL;
	struct timing_info *per_timing_info_vr = NULL;
	struct timing_info *dptx_timing_node = NULL, *_node_ = NULL;
	struct dtd *mdtd = NULL;
	uint32_t fps = 0;
	uint32_t fps_default = 0;
	int retval;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((bsafemode == NULL), -EINVAL, err, "[DP] bsafemode is NULL!\n");

	mdtd = &dptx->vparams.mdtd;

	if (dptx->edid_info.Video.dptx_timing_list == NULL) {
		*bsafemode = true;
		return 0;
	}

	list_for_each_entry_safe(dptx_timing_node, _node_, dptx->edid_info.Video.dptx_timing_list, list_node) {
		dptx_convert_timing_info(dptx_timing_node, mdtd);
		if ((dptx_timing_node->interlaced != 1) &&
			(dptx_timing_node->vBlanking <= VBLANKING_MAX) &&
			(dptx->dptx_video_ts_calculate && !dptx->dptx_video_ts_calculate(dptx, dptx->link.lanes,
				dptx->link.rate, dptx->vparams.bpc,
				dptx->vparams.pix_enc, (dptx_timing_node->pixelClock * 10)))) {

			retval = dptx_calc_fps(dptx_timing_node, &fps);
			if (retval) {
				disp_pr_info("[DP] fps calc error, skip this timing node\n");
				continue;
			}

			if (dptx_need_update_timing(dptx_timing_node, per_timing_info, fps, fps_default)) {
				per_timing_info = dptx_timing_node;
				fps_default = fps;
			}

			/* choose vr timing */
			if (dptx_need_update_timing_vr(dptx, dptx_timing_node, per_timing_info_vr, fps))
				per_timing_info_vr = dptx_timing_node;
		}
	}

	if ((dptx->dptx_vr == true) && (per_timing_info_vr != NULL))
		per_timing_info = per_timing_info_vr;

	if ((per_timing_info == NULL) || (per_timing_info->hActivePixels == SAFE_MODE_TIMING_HACTIVE)) {
		*bsafemode = true;
		return 0;
	}

	dptx_convert_timing_info(per_timing_info, mdtd);

	dptx->max_edid_timing_hactive = per_timing_info->hActivePixels;

	disp_pr_info(
		"[DP] The choosed DTD: pixel_clock is %llu, interlaced is %d, h_active is %d, v_active is %d\n",
		mdtd->pixel_clock, mdtd->interlaced, mdtd->h_active, mdtd->v_active);
	disp_pr_debug("[DP] DTD pixel_clock: %llu interlaced: %d\n",
		 mdtd->pixel_clock, mdtd->interlaced);
	disp_pr_debug("[DP] h_active: %d h_blanking: %d h_sync_offset: %d\n",
		 mdtd->h_active, mdtd->h_blanking, mdtd->h_sync_offset);
	disp_pr_debug("[DP] h_sync_pulse_width: %d h_image_size: %d h_sync_polarity: %d\n",
		 mdtd->h_sync_pulse_width, mdtd->h_image_size,
		 mdtd->h_sync_polarity);
	disp_pr_debug("[DP] v_active: %d v_blanking: %d v_sync_offset: %d\n",
		 mdtd->v_active, mdtd->v_blanking, mdtd->v_sync_offset);
	disp_pr_debug("[DP] v_sync_pulse_width: %d v_image_size: %d v_sync_polarity: %d\n",
		 mdtd->v_sync_pulse_width, mdtd->v_image_size,
		 mdtd->v_sync_polarity);

	huawei_dp_imonitor_set_param(DP_PARAM_MAX_WIDTH,   &(mdtd->h_active));
	huawei_dp_imonitor_set_param(DP_PARAM_MAX_HIGH,    &(mdtd->v_active));
	huawei_dp_imonitor_set_param(DP_PARAM_PIXEL_CLOCK, &(mdtd->pixel_clock));
	*bsafemode = false;

	return 0;
}

void dptx_link_params_reset(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	dptx->cr_fail = false; /* harutk ---ntr */
	dptx->mst = false;
	dptx->ssc_en = false;
	dptx->efm_en = true;
	dptx->fec = false;
	dptx->streams = 1;
	if (g_enable_adpative_sync)
		dptx->dptx_vr = true;
	dptx->dsc = false;

	dptx_video_params_reset(&dptx->vparams);
	dptx_audio_params_reset(&dptx->aparams);

#ifdef CONFIG_DP_GRAPHIC_DEBUG_TOOL
	if (g_dp_debug_mode_enable)
		dp_graphic_debug_set_debug_params(dptx);
#endif
}

uint32_t g_caps_long_byte_read = 0;
void set_caps_long_byte_read(uint32_t caps_long_byte_read)
{
	g_caps_long_byte_read = caps_long_byte_read;
}
EXPORT_SYMBOL_GPL(set_caps_long_byte_read);

static int dptx_get_device_caps(struct dp_ctrl *dptx)
{
	uint8_t rev = 0;
	int retval;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, DP_DPCD_REV, &rev);
	if (retval) {
		/*
		 * Abort bringup
		 * Reset core and try again
		 * Abort all aux, and other work, reset the core
		 */
		disp_pr_err("[DP] failed to dptx_read_dpcd DP_DPCD_REV, retval=%d.\n", retval);
		return retval;
	}
	disp_pr_debug("[DP] Revision %x.%x .\n", (rev & 0xf0) >> 4, rev & 0xf);

	memset(dptx->rx_caps, 0, DPTX_RECEIVER_CAP_SIZE);
	if (g_caps_long_byte_read) {
		retval = dptx_read_bytes_from_dpcd(dptx, DP_DPCD_REV,
			dptx->rx_caps, DPTX_RECEIVER_CAP_SIZE);
		if (retval) {
			disp_pr_err("[DP] failed to dptx_read_bytes_from_dpcd DP_DPCD_REV, retval=%d.\n", retval);
			return retval;
		}
		huawei_dp_imonitor_set_param(DP_PARAM_DPCD_RX_CAPS, dptx->rx_caps);
	} else {
		for (uint32_t i = 0; i < DPTX_RECEIVER_CAP_SIZE; i++) {
			retval = dptx_read_dpcd(dptx, DP_DPCD_REV + i, &dptx->rx_caps[i]);
			if (retval) {
				disp_pr_err("[DP] failed to dptx_read_bytes_from_dpcd DP_DPCD_REV, i=%d.\n", i);
				return retval;
			}
		}
	}

#if 0
	if (dptx->rx_caps[DP_TRAINING_AUX_RD_INTERVAL] &
		DP_EXTENDED_RECEIVER_CAPABILITY_FIELD_PRESENT) {
		retval = dptx_read_bytes_from_dpcd(dptx, DP_DPCD_REV_EXT,
			dptx->rx_caps, DPTX_RECEIVER_CAP_SIZE);
		if (retval) {
			disp_pr_err("[DP] DP_EXTENDED_RECEIVER_CAPABILITY_FIELD_PRESENT failed, retval=%d.\n", retval);
			return retval;
		}
	}
#endif
	return 0;
}

uint32_t g_force_dsc = 0;
void force_dsc_enable(uint32_t enable)
{
	g_force_dsc = enable;
}
EXPORT_SYMBOL_GPL(force_dsc_enable);

static int dptx_link_get_device_caps(struct dp_ctrl *dptx)
{
	int retval;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	/* set sink device power state to D0 */
	retval = dptx_write_dpcd(dptx, DP_SET_POWER, DP_SET_POWER_D0);
	disp_pr_err("[DP] test %x", DP_SET_POWER);
	if (retval) {
		disp_pr_err("[DP] failed to dptx_write_dpcd DP_SET_POWER, DP_SET_POWER_D0 %d", retval);
		return retval;
	}
	mdelay(1);

	/* get rx_caps */
	retval = dptx_get_device_caps(dptx);
	if (retval) {
		disp_pr_err("[DP] Check device capability failed.\n");
		return retval;
	}

	/* get mst capability, info in rx_caps */
	retval = dptx_mst_initial(dptx);
	if (retval) {
		disp_pr_err("[DP] Failed to get mst info %d", retval);
		return retval;
	}

	retval = dptx_dsc_initial(dptx);
	if (retval) {
		disp_pr_err("[DP] Failed to get dsc info %d", retval);
		return retval;
	}
	if (g_force_dsc) {
		dptx->dsc = true;
		dptx->fec = true;
	}

	return 0;
}

static int dptx_get_test_request(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t blocks = 0;
	uint8_t test = 0;
	uint8_t vector = 0;
	uint8_t checksum = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);
	if (retval) {
		disp_pr_err("[DP] failed to  dptx_read_dpcd DP_DEVICE_SERVICE_IRQ_VECTOR, retval=%d", retval);
		return retval;
	}

	if (vector & DP_AUTOMATED_TEST_REQUEST) {
		disp_pr_info("[DP] DP_AUTOMATED_TEST_REQUEST");
		retval = dptx_read_dpcd(dptx, DP_TEST_REQUEST, &test);
		if (retval) {
			disp_pr_err("[DP] failed to dptx_read_dpcd DP_TEST_REQUEST, retval=%d\n", retval);
			return retval;
		}

		if (test & DP_TEST_LINK_EDID_READ) {
			blocks = dptx->edid[126];
			checksum = dptx->edid[127 + EDID_BLOCK_LENGTH * blocks];

			retval = dptx_write_dpcd(dptx, DP_TEST_EDID_CHECKSUM, checksum);
			if (retval) {
				disp_pr_err(
					"[DP] failed to dptx_write_dpcd DP_TEST_EDID_CHECKSUM, retval=%d\n", retval);
				return retval;
			}

			retval = dptx_write_dpcd(dptx, DP_TEST_RESPONSE, DP_TEST_EDID_CHECKSUM_WRITE);
			if (retval) {
				disp_pr_err("[DP] failed to dptx_write_dpcd DP_TEST_RESPONSE, retval=%d\n", retval);
				return retval;
			}
		}
	}

	return 0;
}

static int dptx_video_config(struct dp_ctrl *dptx, int stream)
{
	struct video_params *vparams = NULL;
	struct dtd *mdtd = NULL;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;

	if (!convert_code_to_dtd(mdtd, vparams->mode,
			   vparams->refresh_rate, vparams->video_format))
		return -EINVAL;

	if (dptx->dptx_video_core_config)
		dptx->dptx_video_core_config(dptx, stream);
	return 0;
}

/**
 * default link params and controller reset
 * reset mst/fec/dec control params here
 */
static int dptx_link_reset(struct dp_ctrl *dptx)
{
	int retval;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	dptx_link_params_reset(dptx);

	retval = dptx_video_config(dptx, 0);
	if (retval) {
		disp_pr_err("[DP] video config fail\n");
		return retval;
	}

	if (dptx->dptx_link_core_reset)
		dptx->dptx_link_core_reset(dptx);

	return 0;
}

/**
 * get sink device's edid, now just for sst
 * include mst mode which is now implemented in dptx_get_topology
 */
int dptx_link_get_device_edid(struct dp_ctrl *dptx, bool *bsafemode, uint32_t *edid_info_size)
{
	int retval;
	char *monitor_name_info = NULL;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((bsafemode == NULL), -EINVAL, err, "[DP] bsafemode is NULL!\n");
	disp_check_and_return((edid_info_size == NULL), -EINVAL, err, "[DP] edid_info_size is NULL!\n");

	if (dptx->mst) {
		retval = dptx_get_topology(dptx);
		if (retval)
			return retval;
	}

	if (!dptx->mst) {
		retval = dptx_read_edid(dptx);
		if (retval < EDID_BLOCK_LENGTH) {
			disp_pr_err("[DP] failed to  dptx_read_edid, retval=%d\n", retval);
			huawei_dp_imonitor_set_param(DP_PARAM_READ_EDID_FAILED, &retval);
			*edid_info_size = 0;
			*bsafemode = true;
		} else {
			*edid_info_size = retval;
		}

		retval = parse_edid(dptx, *edid_info_size);
		if (retval) {
			disp_pr_err("[DP] EDID Parser fail, display safe mode\n");
			*bsafemode = true;
		}

		if (dptx->edid_info.Video.dp_monitor_descriptor != NULL) {
			monitor_name_info = dptx->edid_info.Video.dp_monitor_descriptor;
			if (!(strncmp("HUAWEIAV02", monitor_name_info, strlen("HUAWEIAV02"))) ||
				!(strncmp("HUAWEIAV03", monitor_name_info, strlen("HUAWEIAV03")))) {
				dptx->dptx_vr = true;
				disp_pr_info("[DP] The display is VR\n");
			}
			huawei_dp_set_dptx_vr_status(dptx->dptx_vr);
		}
	}

	/*
	 * The TEST_EDID_READ is asserted on HOTPLUG. Check for it and
	 * handle it here.
	 */
	retval = dptx_get_test_request(dptx);
	if (retval) {
		disp_pr_err("[DP] Check test request failed\n");
		return retval;
	}

	return 0;
}

static int dptx_link_choose_timing(struct dp_ctrl *dptx, bool bsafe_mode, int edid_info_size)
{
	struct video_params *vparams = NULL;
	struct dtd mdtd;
	int retval = 0;
	uint8_t rev = 0;
	uint8_t code;
	int i;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	vparams = &dptx->vparams;

	if (!bsafe_mode)
		dptx_choose_edid_timing(dptx, &bsafe_mode);

	if ((bsafe_mode) || (g_fpga_flag)) {
		huawei_dp_imonitor_set_param(DP_PARAM_SAFE_MODE, &bsafe_mode);
		if (edid_info_size) {
			code = 1; /* resolution: 640*480 */
			vparams->video_format = VCEA;
			huawei_dp_imonitor_set_param_resolution(&code, &(vparams->video_format));
			/* If edid is parsed error, DP transfer 640*480 firstly!.*/
			(void)convert_code_to_dtd(&mdtd, code, vparams->refresh_rate, vparams->video_format);
		} else {
			vparams->video_format = VCEA;
			/* If edid can't be got, DP transfer 1024*768 firstly!*/
			(void)convert_code_to_dtd(&mdtd, 16, vparams->refresh_rate, vparams->video_format);

			retval = dptx_read_dpcd(dptx, DP_DOWNSTREAMPORT_PRESENT, &rev);
			if (retval) {
				disp_pr_err(
					"[DP] failed to dptx_read_dpcd DP_DOWNSTREAMPORT_PRESENT, retval=%d\n",
					retval);
				return retval;
			}

			if (((rev & DP_DWN_STRM_PORT_TYPE_MASK) >> 1) != 0x01) {
				dptx->edid_info.Audio.basicAudio = 0x1;
				disp_pr_info(
					"[DP] If DFP port don't belong to analog(VGA/DVI-I), update audio capabilty\n");
				huawei_dp_imonitor_set_param(DP_PARAM_BASIC_AUDIO, &(dptx->edid_info.Audio.basicAudio));
			}
		}

		if (g_fpga_flag) {
			vparams->video_format = VCEA;
			/* Fpga only display 720*480. */
			(void)convert_code_to_dtd(&mdtd, g_video_code, vparams->refresh_rate, vparams->video_format);
			if (g_pixel_clock)
				mdtd.pixel_clock = g_pixel_clock;
			mdtd.h_blanking = g_hblank;
		}
		memcpy(&(dptx->vparams.mdtd), &mdtd, sizeof(mdtd));
	}

	if (dptx->dptx_video_ts_calculate)
		retval = dptx->dptx_video_ts_calculate(dptx, dptx->link.lanes,
			dptx->link.rate, vparams->bpc, vparams->pix_enc, vparams->mdtd.pixel_clock);

	if (dptx->mst) {
		if ((vparams->mdtd.h_active > FHD_TIMING_H_ACTIVE) || (vparams->mdtd.v_active > FHD_TIMING_V_ACTIVE)) {
			vparams->video_format = VCEA;
			for (i = 0; i < dptx->streams; i++) {
				retval = dptx_video_mode_change(dptx, 16, i);
				if (retval)
					return retval;
			}
		}
	} else {
		if (retval) {
			disp_pr_info("[DP] Can't change to the preferred video mode: frequency = %llu\n",
				vparams->mdtd.pixel_clock);
		} else {
			//vparams->mdtd = mdtd;
			disp_pr_debug("[DP] pixel_frequency=%llu\n", vparams->mdtd.pixel_clock);
		}
	}

	return 0;
}

uint32_t bypass_edid = 0;
void set_bypass_edid(uint32_t enable)
{
	bypass_edid = enable;
}
EXPORT_SYMBOL_GPL(set_bypass_edid);

int handle_hotplug(struct dp_ctrl *dptx)
{
	int retval;
	uint32_t edid_info_size = 0;
	struct video_params *vparams = NULL;
	bool bsafe_mode = false;

	disp_pr_info("[DP] +\n");

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((!dptx->dptx_enable), -EINVAL, err, "[DP] dptx has already off!\n");

	huawei_dp_imonitor_set_param(DP_PARAM_TIME_START, NULL);

	vparams = &dptx->vparams;

	/**
	 * default link params and controller reset
	 * reset mst/fec/dec control params here
	 */
	retval = dptx_link_reset(dptx);
	if (retval)
		return retval;

	/* get rx_caps */
	retval = dptx_link_get_device_caps(dptx);
	if (retval)
		return retval;

	if (bypass_edid == 0) {
		/* get edid */
		retval = dptx_link_get_device_edid(dptx, &bsafe_mode, &edid_info_size);
		if (retval)
			return retval;
	}

	/* No other IRQ should be set on hotplug */
	retval = dptx_link_training(dptx, dptx->max_rate, dptx->max_lanes);

	if (retval) {
		disp_pr_err("[DP] failed to  dptx_link_training, retval=%d\n", retval);
		huawei_dp_imonitor_set_param(DP_PARAM_LINK_TRAINING_FAILED, &retval);
		return retval;
	}
	msleep(1);

	/* choose timing */
	retval = dptx_link_choose_timing(dptx, bsafe_mode, edid_info_size);
	if (retval)
		return retval;

#ifdef CONFIG_DP_HDCP_ENABLE
	//hdcp_dp_on(dptx, true);
#endif
	disp_pr_info("handle_hotplug g_enable_adpative_sync = %d \n", g_enable_adpative_sync);
	if (g_enable_adpative_sync)
		config_adaptive_sync(dptx, g_max_line_cnt);
	/* config dss and dp core */
	if (dptx->dptx_link_timing_config)
		retval = dptx->dptx_link_timing_config(dptx);
	if (retval)
		return retval;

	if (dptx->dptx_vr)
		dptx_init_detect_work(dptx);

	dptx->current_link_rate = dptx->link.rate;
	dptx->current_link_lanes = dptx->link.lanes;

#if CONFIG_DP_ENABLE
	/* for factory test */
	if (huawei_dp_factory_mode_is_enable()) {
		if (!huawei_dp_factory_is_4k_60fps(dptx->max_rate, dptx->max_lanes,
			dptx->vparams.mdtd.h_active, dptx->vparams.mdtd.v_active, dptx->vparams.m_fps)) {
			disp_pr_err("[DP] can't hotplug when combinations is invalid in factory mode!\n");
			if (dptx->edid_info.Audio.basicAudio == 0x1)
				switch_set_state(&dptx->dp_switch, 0);
			return -ECONNREFUSED;
		}
	}
#endif

	disp_pr_info("[DP] -\n");

	return 0;
}
/*lint -restore*/
