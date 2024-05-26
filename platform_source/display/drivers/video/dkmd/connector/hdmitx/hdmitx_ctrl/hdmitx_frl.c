/*
 * Copyright (c) CompanyNameMagicTag. 2019-2020. All rights reserved.
 * Description: hdmi driver frl module source file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/delay.h>

#include <securec.h>
#include "dkmd_log.h"

#include "hdmitx_ctrl.h"
#include "hdmitx_connector.h"
#include "hdmitx_ddc.h"
#include "hdmitx_frl_config.h"
#include "hdmitx_frl.h"

/* Link Training State */
#define LTS_L 0
#define LTS_1 1
#define LTS_2 2
#define LTS_3 3
#define LTS_4 4
#define LTS_P 5

#define LTS1_EDID_FAIL	 0
#define LTS1_PASS		  1
#define LTS2_PASS		  2
#define LTS2_TIMEOUT	   3
#define LTS3_TIMEOUT	   4
#define LTS3_REQ_NEW_RATE  5
#define LTS3_PASS		  6
#define LTS4_LOWER_RATE	7
#define LTS4_NO_LOWER_RATE 8
#define LTSP_RETRAIN	   9
#define LTSP_START		 10
#define LTSP_TIMEOUT	   11
#define LTSL			   12

static const char * const g_frl_msg[] = {
	"LTS1 edid check fail",
	"LTS1 PASS",
	"LTS2 PASS",
	"LTS2 timeout",
	"LTS3 timeout",
	"LTS3 req new rate",
	"LTS3 pass",
	"LTS4 lower rate",
	"LTS4 no lower rate(fail)",
	"LTSP re-train",
	"LTSP start",
	"LTSP timeout",
	"LTSL",
};

static void sink_set_frl_rate_ffe_levels(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 config_value;

	hal_frl_set_training_rate(&frl->hal, frl->scdc.frl_rate);

	/* write scdc 0x31[7:4] FFE_Levels,[3:0] FRL_Rate */
	config_value = ((frl->scdc.frl_rate & TRAIN_FRL_RATE_MASK) |
					((frl->config.src_ffe_levels << 4) & TRAIN_FFE_LEVELS_MASK)); /* ffe level must left shift 4bits. */
	ret = hdmitx_ddc_scdc_writeb(frl->ddc, SCDC_SINK_CFG2, config_value);
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		frl->scdc.frl_rate = 0;
		dpu_pr_err("[hdmitx%d]FRL_rate=%d,FFE_levels=%d, scdc fail\n",
			frl->id, frl->scdc.frl_rate, frl->config.src_ffe_levels);
	}

	frl->scdc.ffe_levels = frl->config.src_ffe_levels;
}

static void sink_get_frl_rate_ffe_levels(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 value;

	value = 0;
	ret = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_SINK_CFG2, &value);
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		frl->scdc.frl_rate = 0;
		dpu_pr_err("[hdmitx]read FRL_rate & FFE_levels fail\n");
	}

	/* scdc 0x31[7:4] FFE_Levels,[3:0] FRL_Rate */
	frl->scdc.frl_rate = value & TRAIN_FRL_RATE_MASK;
	frl->scdc.ffe_levels = (value & TRAIN_FFE_LEVELS_MASK) >> 4; /* right shift 4 bit */
}

static void sink_cleanup_frl_rate(struct hdmitx_frl *frl)
{
	s32 ret;
	/* write scdc 0x31[7:4] FFE_Levels(0),[3:0] FRL_Rate(0) */
	ret = hdmitx_ddc_scdc_writeb(frl->ddc, SCDC_SINK_CFG2, 0);
	frl->scdc.frl_rate = 0;
	frl->scdc.ffe_levels = 0;
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]FRL_rate=%d,FFE_levels=%d, scdc fail\n",
			frl->id, frl->scdc.frl_rate, frl->scdc.ffe_levels);
	}
}

static void sink_get_test_configuration(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 value;

	value = 0;
	/* read scdc addr offset 0x35 */
	ret = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_SOU_TEST_CFG, &value);
	/* scdc ddc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]ddc error!\n", frl->id);
	}

	frl->scdc.flt_no_timeout = !!(value & TRAIN_FLT_NO_TIMEOUT_MASK);
	frl->scdc.frl_max = !!(value & TRAIN_FRL_MAX_MASK);
	frl->scdc.dsc_frl_max = !!(value & TRAIN_DSC_FRL_MAX_MASK);
}

static void sink_get_flt_status(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 value;

	value = 0;
	/* read scdc 0x10 */
	ret = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_UPDATE_FLAG1, &value);
	/* scdc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]flt_start read scdc fail!\n", frl->id);
	}
	/* frl->scdc.flt_start */
	frl->scdc.flt_start = !!(value & TRAIN_FLT_START_MASK);
	/* frl->scdc.flt_update */
	frl->scdc.flt_update = !!(value & TRAIN_FLT_UPDATE_MASK);

	if (frl->scdc.flt_start && frl->scdc.flt_update)
		dpu_pr_err("[hdmitx%d]Sink conflict status:flt_start,flt_update=%d,%d.\n",
			frl->id, frl->scdc.flt_start, frl->scdc.flt_update);
}

static void sink_get_flt_ready(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 value;

	value = 0;
	/* get flt ready flag  offset 0x40 */
	ret = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_STATUS_FLAG1, &value);
	/* scdc ddc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]ddc error!\n", frl->id);
	}
	frl->scdc.flt_ready = !!(value & TRAIN_FLT_READY_MASK);
}

static void sink_get_scdc_version(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 value;

	value = 0;
	/* get flt ready flag  offset 0x40 */
	ret = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_SINK_VERSION, &value);
	/* scdc ddc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("ddc error!\n");
	}
	frl->scdc.sink_ver = value;
}

static void sink_get_lnx_ltp_req(struct hdmitx_frl *frl)
{
	struct frl_scdc *scdc = &frl->scdc;
	s32 ret1, ret2;
	u8 value, value1;

	value = 0;
	value1 = 0;
	ret1 = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_STATUS_FLAG2, &value);
	ret2 = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_STATUS_FLAG3, &value1);
	if ((ret1 < 0) || (ret2 < 0)) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]lnx_req scdc read fail,ret1=%d,ret2=%d\n", frl->id, ret1, ret2);
	}

	scdc->ltp_req[FRL_LANE_NUM_0] = value & TRAIN_LN0_LTP_REQ_MASK;
	scdc->ltp_req[FRL_LANE_NUM_1] = (value & TRAIN_LN1_LTP_REQ_MASK) >> 4; /* ln1_req must be right shift 4bit. */
	scdc->ltp_req[FRL_LANE_NUM_2] = value1 & TRAIN_LN2_LTP_REQ_MASK;
	scdc->ltp_req[FRL_LANE_NUM_3] = (value1 & TRAIN_LN3_LTP_REQ_MASK) >> 4; /* ln3_req must be right shift 4bit. */
	dpu_pr_info("[hdmitx%d]sink lnx_req[0-3]=0x%02x,0x%02x,0x%02x,0x%02x\n",
		frl->id,  scdc->ltp_req[FRL_LANE_NUM_0],  scdc->ltp_req[FRL_LANE_NUM_1],
		scdc->ltp_req[FRL_LANE_NUM_2],  scdc->ltp_req[FRL_LANE_NUM_3]);
}

static void sink_clear_flt_start(struct hdmitx_frl *frl)
{
	s32 ret;

	/* write scdc 0x10 source clears FLT_start by writing 1 */
	ret = hdmitx_ddc_scdc_writeb(frl->ddc, SCDC_UPDATE_FLAG1, TRAIN_FLT_START_MASK);
	/* scdc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]ddc error!\n", frl->id);
		return;
	}
	frl->scdc.flt_start = false;
}

static void sink_clear_flt_update(struct hdmitx_frl *frl)
{
	s32 ret;

	/* write scdc 0x10 source clears FLT_update by writing 1 */
	ret = hdmitx_ddc_scdc_writeb(frl->ddc, SCDC_UPDATE_FLAG1, TRAIN_FLT_UPDATE_MASK);
	/* scdc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]crear FLT_update scdc fail!\n", frl->id);
		return;
	}
	frl->scdc.flt_update = false;
}

static void sink_get_flt_update(struct hdmitx_frl *frl)
{
	s32 ret;
	u8 value;

	value = 0;
	/* read scdc 0x10 */
	ret = hdmitx_ddc_scdc_readb(frl->ddc, SCDC_UPDATE_FLAG1, &value);
	/* scdc ddc fail */
	if (ret < 0) {
		frl->stat.event = TRAIN_EVENT_DDC_ERR;
		dpu_pr_err("[hdmitx%d]flt update read scdc fail!\n", frl->id);
	}
	frl->scdc.flt_update = !!(value & TRAIN_FLT_UPDATE_MASK);
}

static void timer_callback(struct timer_list *t)
{
	struct hdmitx_frl *frl = container_of(t, struct hdmitx_frl, timer);

	if (frl == NULL) {
		dpu_pr_err("null!\n");
		return;
	}

	if (frl->stat.frl_state == LTS_2)
		frl->stat.ready_timeout = true;

	if (frl->stat.frl_state == LTS_3)
		frl->stat.tflt_timeout = true;

	if (frl->stat.frl_state == LTS_P)
		frl->stat.ltsp_timeout = true;
}

static void frl_log_msg(u32 msg_macro, bool is_fail)
{
	if (msg_macro >= hdmitx_array_size(g_frl_msg))
		return;

	if (is_fail)
		dpu_pr_err("%s", g_frl_msg[msg_macro]);
	else
		dpu_pr_info("%s", g_frl_msg[msg_macro]);
}

static void frl_set_ln_total(struct hdmitx_frl *frl, u8 frl_rate)
{
	if (frl_rate == FRL_RATE_3G3L || frl_rate == FRL_RATE_6G3L)
		frl->stat.work_3lane = true;
	else
		frl->stat.work_3lane = false;

	dpu_pr_info("[hdmitx%d]work_3lane:%d\n", frl->id, frl->stat.work_3lane);
}

static bool frl_is_change_rate(const struct hdmitx_frl *frl)
{
	bool ret = false;

	if (frl->stat.work_3lane) {
		ret = (frl->scdc.ltp_req[FRL_LANE_NUM_0] == 0xf &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_1] == 0xf &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_2] == 0xf);
	} else {
		ret = (frl->scdc.ltp_req[FRL_LANE_NUM_0] == 0xf &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_1] == 0xf &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_2] == 0xf &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_3] == 0xf);
	}

	return ret;
}

static void frl_set_lanex_training_pattern(struct hdmitx_frl *frl, u32 index)
{
	struct frl_scdc *scdc = &frl->scdc;

	if (frl->stat.work_3lane && index == FRL_LANE_NUM_3) {
		hal_frl_set_training_pattern(&frl->hal, index, 0);
		hal_frl_set_training_ffe(&frl->hal, index, 0);
		return;
	}

	if (scdc->ltp_req[index] >= 1 && scdc->ltp_req[index] <= 8) { /* ltp is in the 1~8 range. */
		/* ltp req is 3 and flt_no_timeoutis 0, ignore. */
		if (scdc->ltp_req[index] == 3 && !scdc->flt_no_timeout)
			dpu_pr_info("[hdmitx%d]ln[%d] req3 and no timeout, don't set pattern.\n", frl->id, index);
		else
			hal_frl_set_training_pattern(&frl->hal, index, scdc->ltp_req[index]);
	} else if (scdc->ltp_req[index] == 0xe) { /* ltp req is 0xe. */
		frl->stat.ffe_levels[index] = (frl->stat.ffe_levels[index] + 1) % 4; /* lanex ffe level can't over 3(4 - 1). */
		hal_frl_set_training_ffe(&frl->hal, index, frl->stat.ffe_levels[index]);
	}
}

static void frl_set_training_pattern(struct hdmitx_frl *frl)
{
	u32 i;

	for (i = 0; i < FRL_LANE_NUM_MAX; i++)
		frl_set_lanex_training_pattern(frl, i);
}

static bool frl_is_train_pass(const struct hdmitx_frl *frl)
{
	bool ret = false;

	if (frl->stat.work_3lane) {
		ret = (frl->scdc.ltp_req[FRL_LANE_NUM_0] == 0x0 &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_1] == 0x0 &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_2] == 0x0);
	} else {
		ret = (frl->scdc.ltp_req[FRL_LANE_NUM_0] == 0x0 &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_1] == 0x0 &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_2] == 0x0 &&
			   frl->scdc.ltp_req[FRL_LANE_NUM_3] == 0x0);
	}
	return ret;
}

static bool frl_edid_check(const struct hdmitx_frl *frl)
{
	if (!frl->config.scdc_present) {
		dpu_pr_info("[hdmitx%d]scdc present=%d\n", frl->id, frl->config.scdc_present);
		return false;
	}
	if (!frl->scdc.sink_ver) {
		dpu_pr_warn("[hdmitx%d]scdc version=%d\n", frl->id, frl->scdc.sink_ver);
		return false;
	}
	if (!frl->config.frl_max_rate) {
		dpu_pr_warn("[hdmitx%d]frl_max_rate=%d\n", frl->id, frl->config.frl_max_rate);
		return false;
	}
	dpu_pr_info("[hdmitx%d]ffe=%d,max_rate=%d,min_rate=%d\n", frl->id, frl->config.src_ffe_levels,
		frl->config.frl_max_rate, frl->config.frl_min_rate);
	return true;
}

static void frl_training_clear_ffe(struct hdmitx_frl *frl)
{
	hal_frl_set_training_ffe(&frl->hal, 0, 0); /* Clear lane0 ffe level. */
	hal_frl_set_training_ffe(&frl->hal, 1, 0); /* Clear lane1 ffe level. */
	hal_frl_set_training_ffe(&frl->hal, 2, 0); /* Clear lane2 ffe level. */
	hal_frl_set_training_ffe(&frl->hal, 3, 0); /* Clear lane3 ffe level. */
	frl->stat.ffe_levels[FRL_LANE_NUM_0] = 0;
	frl->stat.ffe_levels[FRL_LANE_NUM_1] = 0;
	frl->stat.ffe_levels[FRL_LANE_NUM_2] = 0;
	frl->stat.ffe_levels[FRL_LANE_NUM_3] = 0;
}

static void frl_training_prepare(const struct hdmitx_frl *frl)
{
	hal_frl_set_training_pattern(&frl->hal, 0, 0); /* set lane0' pattern init value to 0. */
	hal_frl_set_training_pattern(&frl->hal, 1, 0); /* set lane1' pattern init value to 0. */
	hal_frl_set_training_pattern(&frl->hal, 2, 0); /* set lane2' pattern init value to 0. */
	hal_frl_set_training_pattern(&frl->hal, 3, 0); /* set lane3' pattern init value to 0. */
	hal_frl_set_source_data(&frl->hal, FRL_TRAINING_PATTERN);

	//  to fix : hal_ctrl_set_rc really need?
}

static void frl_stat_reset(struct hdmitx_frl *frl)
{
	struct frl_stat *stat = &frl->stat;

	stat->ready_timeout = false;
	stat->tflt_timeout = false;
	stat->ltsp_timeout = false;
	stat->ltsp_poll = false;
	stat->phy_output = false;
	stat->video_transifer = false;
	stat->frl_start = false;
	stat->ffe_levels[FRL_LANE_NUM_0] = 0;
	stat->ffe_levels[FRL_LANE_NUM_1] = 0;
	stat->ffe_levels[FRL_LANE_NUM_2] = 0;
	stat->ffe_levels[FRL_LANE_NUM_3] = 0;
	stat->work_3lane = false;
	stat->frl_state = LTS_L;
	stat->event = TRAIN_EVENT_DISABLE;
}

static void frl_scdc_init_from_sink(struct hdmitx_frl *frl)
{
	if (frl == NULL) {
		dpu_pr_err("null ptr!\n");
		return;
	}

	if (frl->config.scdc_present == false)
		return;

	sink_get_scdc_version(frl);
	sink_get_flt_ready(frl);
	sink_get_frl_rate_ffe_levels(frl);
	sink_get_test_configuration(frl);
	sink_get_lnx_ltp_req(frl);
	sink_get_flt_update(frl);
	sink_get_flt_status(frl);
}

static void frl_scdc_reset(struct hdmitx_frl *frl)
{
	struct frl_scdc *scdc = &frl->scdc;

	scdc->frl_rate = 0;
	scdc->ffe_levels = 0;
	scdc->flt_no_timeout = false;
	scdc->frl_max = 0;
	scdc->dsc_frl_max = 0;
	scdc->sink_ver = 0;
	scdc->flt_update = false;
	scdc->flt_start = false;
	scdc->flt_ready = false;
	scdc->ltp_req[FRL_LANE_NUM_0] = 0;
	scdc->ltp_req[FRL_LANE_NUM_1] = 0;
	scdc->ltp_req[FRL_LANE_NUM_2] = 0;
	scdc->ltp_req[FRL_LANE_NUM_3] = 0;
}

static void frl_config_init(struct hdmitx_ctrl *hdmitx)
{
	struct frl_config *config = &hdmitx->frl->config;

	/* You shold make sure these config keep same with boot param */
	config->max_rate_proir = false;
	config->ready_timeout = TRAIN_READY_TIMEOUT;
	config->tflt_margin = TRAIN_TFLT_MARGIN;
	config->update_flag_magin = TRAIN_UPDATE_TIMEOUT_MAGIN;
	config->ltsp_poll_interval = TRAIN_LTSP_POLL_INTERVAL;
	config->frl_max_rate = FRL_RATE_12G4L;
	config->frl_min_rate = FRL_RATE_3G3L;
	config->src_ffe_levels = 3; /* max ffe level is 3. */
	config->scdc_present = false;

	config->scdc_present = drv_hdmitx_connector_is_scdc_present(hdmitx);
}

static void frl_training_state_machine_reset(struct hdmitx_frl *frl)
{
	frl_stat_reset(frl);
	frl_scdc_reset(frl);
	frl->stat.event = TRAIN_EVENT_SUCCESS;
	frl->stat.frl_state = LTS_1;
	cancel_delayed_work_sync(&frl->dl_work);
}

static void frl_training_state_machine_stop(struct hdmitx_frl *frl)
{
	frl_stat_reset(frl);
	frl_scdc_reset(frl);
	frl->stat.event = TRAIN_EVENT_DISABLE;
	frl->stat.frl_state = LTS_L;
}

static void frl_config_crg_phy(const struct hdmitx_frl *frl)
{
	/* to fix: if need?

	   frl_config_phy_param
	   hdmi_crg_set
	   frl_hdmi_video_config;
	*/

	/* Deepcolor need disable when dsc enable in the hdmi2.1 spec. */
	// to fix: drv_hdmitx_controller_disable_deepcolor_for_dsc

	//  to fix: need?
	// drv_hdmitx_controller_soft_reset
	hal_frl_config_16to18_table(&frl->hal);
}

static void frl_training_lts1_process(struct hdmitx_frl *frl)
{
	dpu_pr_err("training start\n");
	sink_get_scdc_version(frl);
	if (!frl_edid_check(frl)) {
		frl->stat.event = TRAIN_EVENT_SINK_NO_SCDC;
		frl->stat.frl_state = LTS_L;
		frl_log_msg(LTS1_EDID_FAIL, true);
	} else {
		frl_log_msg(LTS1_PASS, false);
		frl->stat.frl_state = LTS_2;
		mod_timer(&frl->timer, msecs_to_timer(frl->config.ready_timeout));
	}
}

static void frl_training_lts2_process(struct hdmitx_frl *frl)
{
	bool max_flag = false;

	sink_get_flt_ready(frl);
	if (frl->scdc.flt_ready) {
		sink_get_test_configuration(frl);
		max_flag = !!(frl->config.max_rate_proir || frl->scdc.dsc_frl_max || frl->scdc.frl_max);
		frl->scdc.frl_rate = max_flag ? frl->config.frl_max_rate : frl->config.frl_min_rate;
		frl->scdc.ffe_levels = 0;
		sink_set_frl_rate_ffe_levels(frl);
		frl_training_prepare(frl);
		frl_training_clear_ffe(frl);
		/* To here,driver strratgy should confirm oe is down */
		frl_config_crg_phy(frl);
		// frl_config_phy_oe(frl, true)
		frl_set_ln_total(frl, frl->scdc.frl_rate);
		mod_timer(&frl->timer, msecs_to_timer(frl->config.tflt_margin + TRAIN_TFLT));
		dpu_pr_info("[hdmitx%d]LTS2 rate=%u,start LTS3 timer\n", frl->id, frl->scdc.frl_rate);
		frl_log_msg(LTS2_PASS, false);
		frl->stat.frl_state = LTS_3;
	} else if (frl->stat.ready_timeout) { /* timeout in timer */
		frl->stat.event = TRAIN_EVENT_READY_TIMEOUT;
		frl->stat.frl_state = LTS_L;
		frl_log_msg(LTS2_TIMEOUT, true);
	} else {
		msleep(1); /* need sleep 1ms. */
	}
}

static void frl_training_lts3_process(struct hdmitx_frl *frl)
{
	sink_get_test_configuration(frl);
	if (frl->stat.tflt_timeout && !frl->scdc.flt_no_timeout) {
		frl_log_msg(LTS3_TIMEOUT, true);
		/* timeout in timer */
		frl->stat.event = TRAIN_EVENT_TFLT_TIMEOUT;
		frl->stat.frl_state = LTS_L;
	}
	sink_get_flt_update(frl);
	if (frl->scdc.flt_update) {
		sink_get_lnx_ltp_req(frl);
		frl_set_training_pattern(frl);
		if (frl_is_change_rate(frl)) {
			frl->stat.frl_state = LTS_4;
			frl_log_msg(LTS3_REQ_NEW_RATE, false);
		} else if (frl_is_train_pass(frl)) {
			mod_timer(&frl->timer, msecs_to_timer(frl->config.update_flag_magin + TRAIN_TFLT));
			frl->stat.frl_state = LTS_P;
			hal_frl_set_source_data(&frl->hal, FRL_GAP_PACKET);
			frl_log_msg(LTS3_PASS, false);
		}
		sink_clear_flt_update(frl);
	} else {
		msleep(1); /* need sleep 1ms. */
	}
}

static void frl_training_lts4_process(struct hdmitx_frl *frl)
{
	frl_training_prepare(frl);
	frl_training_clear_ffe(frl);
	if (frl->scdc.frl_rate > frl->config.dsc_frl_min_rate) {
		mod_timer(&frl->timer, msecs_to_timer(frl->config.tflt_margin + TRAIN_TFLT));
		frl->scdc.frl_rate -= 1;
		// frl_config_phy_oe(frl, false)
		frl_config_crg_phy(frl);
		// frl_config_phy_oe(frl, true)
		sink_set_frl_rate_ffe_levels(frl);
		dpu_pr_info("[hdmitx%d]LTS4 change rate=%u\n", frl->id, frl->scdc.frl_rate);
		frl_set_ln_total(frl, frl->scdc.frl_rate);
		frl_log_msg(LTS4_LOWER_RATE, false);
		frl->stat.frl_state = LTS_3;
	} else {
		frl_log_msg(LTS4_NO_LOWER_RATE, true);
		frl->stat.event = TRAIN_EVENT_LAST_RATE;
		frl->stat.frl_state = LTS_L;
	}
}

static void frl_training_ltsp_process(struct hdmitx_frl *frl, bool *machine_continue)
{
	sink_get_flt_status(frl);
	if (frl->scdc.flt_update) {
		frl_training_prepare(frl);
		mod_timer(&frl->timer, msecs_to_timer(frl->config.tflt_margin + TRAIN_TFLT));
		frl->stat.frl_state = LTS_3;
		sink_clear_flt_update(frl);
		frl_log_msg(LTSP_RETRAIN, false);
	} else if (frl->scdc.flt_start) {
		/*
		 * frl_worken_set(frl,true,false) should be called
		 * by hdmi_driver module.
		 */
		*machine_continue = false;
		// drv_hdmitx_controller_frl_work_en(frl->controller)
		//  to fix: if dsc enable
		drv_hdmitx_frl_set_worken(frl, true, false);
		frl_log_msg(LTSP_START, false);
		frl->stat.event = TRAIN_EVENT_SUCCESS;
		dpu_pr_err("training success!\n");
	} else if (frl->stat.ltsp_timeout) {
		frl->stat.event = TRAIN_EVENT_LTSP_TIMEOUT;
		frl->stat.frl_state = LTS_L;
		frl_log_msg(LTSP_TIMEOUT, true);
	} else {
		msleep(1); /* need sleep 1ms. */
	}
}

static void frl_training_ltsl_process(struct hdmitx_frl *frl, bool *machine_continue)
{
	frl_log_msg(LTSL, true);
	sink_cleanup_frl_rate(frl);
	frl_training_prepare(frl);
	*machine_continue = false;
	// frl_event_notify(frl)
}

static bool frl_training_state_machine_step(struct hdmitx_frl *frl)
{
	bool continue_machine = true;

	switch (frl->stat.frl_state) {
	case LTS_1:
		frl_training_lts1_process(frl);
		break;
	case LTS_2:
		frl_training_lts2_process(frl);
		break;
	case LTS_3:
		frl_training_lts3_process(frl);
		break;
	case LTS_4:
		frl_training_lts4_process(frl);
		break;
	case LTS_P:
		frl_training_ltsp_process(frl, &continue_machine);
		break;

	case LTS_L:
	default:
		frl_training_ltsl_process(frl, &continue_machine);
		break;
	}

	if (frl->stat.event == TRAIN_EVENT_DDC_ERR) {
		frl->stat.frl_state = LTS_L;
		continue_machine = false;
	}

	if (!continue_machine)
		del_timer(&frl->timer);

	return continue_machine;
}

static void work_queue_callback(struct work_struct *work)
{
	bool continue_mach = true;
	struct hdmitx_frl *frl = container_of(work, struct hdmitx_frl, dl_work.work);

	if (frl == NULL) {
		dpu_pr_err("null poniter!\n");
		return;
	}

	if (frl->stat.frl_state == LTS_P) {
		sink_get_flt_update(frl);
		if (frl->stat.event == TRAIN_EVENT_DDC_ERR)
			dpu_pr_warn("ddc error\n");

		if (!frl->scdc.flt_update) {
			frl->stat.ltsp_poll = true;
			schedule_delayed_work(&frl->dl_work, frl->config.ltsp_poll_interval);
		} else {
			dpu_pr_info("[hdmitx%d]flt_update=%d,re-train LTS3\n", frl->id, frl->scdc.flt_update);
			mod_timer(&frl->timer, frl->config.tflt_margin + TRAIN_TFLT);
			frl->stat.frl_state = LTS_3;
			frl->stat.ltsp_poll = false;
			frl_training_prepare(frl);
			sink_clear_flt_update(frl);
			do
				continue_mach = frl_training_state_machine_step(frl);
			while (continue_mach);
		}
	}
}

s32 hdmitx_frl_init(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_frl *frl = NULL;

	frl = kmalloc(sizeof(struct hdmitx_frl), GFP_KERNEL);
	if (frl == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return -1;
	}

	if (memset_s(frl, sizeof(*frl), 0, sizeof(*frl)) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		goto err;
	}

	frl->id = 0;
	frl->hal.id = 0;
	frl->force_out = false;

	frl->ddc = hdmitx->ddc;
	frl->hal.frl_base = hdmitx->base;
	frl->hal.frl_training_base = hdmitx->training_base;

	timer_setup(&frl->timer, timer_callback, 0);
	INIT_DELAYED_WORK(&frl->dl_work, work_queue_callback);

	/* step1: config init */
	frl_config_init(hdmitx);

	/* step2: scdc init */
	frl_scdc_reset(frl); /* sw */
	if (frl->config.scdc_present)
		frl_scdc_init_from_sink(frl); /* hw */

	/* step3: stat init */
	frl_stat_reset(frl); /* sw */
	// frl_init_from_boot(frl)  /* hw */

	/* step4: hardware init */
	if (frl->stat.video_transifer == 0 || frl->stat.phy_output == 0)
		hal_frl_init(&frl->hal); /* hw */

	hdmitx->frl = frl;
	return 0;
err:
	kfree(frl);
	frl = NULL;
	return -1;
}

void drv_hdmitx_frl_set_worken(struct hdmitx_frl *frl, bool enable, bool fast_mode)
{
	if (frl == NULL)
		return;

	if (enable) {
		hal_frl_set_source_data(&frl->hal, FRL_VIDEO);
		dpu_pr_info("[hdmitx%d]true!\n", frl->id);
	} else {
		hal_frl_set_source_data(&frl->hal, FRL_GAP_PACKET);
		dpu_pr_info("[hdmitx%d]false!\n", frl->id);
	}
	if (enable && (!fast_mode)) {
		sink_clear_flt_start(frl);
		frl->stat.ltsp_poll = true;
		dpu_pr_info("[hdmitx%d]poll update!\n", frl->id);
		schedule_delayed_work(&frl->dl_work, msecs_to_jiffies(frl->config.ltsp_poll_interval));
	}
}

s32 drv_hdmitx_frl_set_config(struct hdmitx_frl *frl, const struct frl_config *config)
{
	if (frl == NULL || config == NULL) {
		dpu_pr_err("null pointer\n");
		return -1;
	}

	/* check */
	frl->config = *config;
	return 0;
}

s32 drv_hdmitx_frl_get_config(const struct hdmitx_frl *frl, struct frl_config *config)
{
	if (frl == NULL || config == NULL) {
		dpu_pr_err("null pointer\n");
		return -1;
	}

	*config = frl->config;
	return 0;
}

s32 drv_hdmitx_frl_get_scdc(const struct hdmitx_frl *frl, struct frl_scdc *scdc)
{
	if (frl == NULL || scdc == NULL) {
		dpu_pr_err("null pointer\n");
		return -1;
	}

	*scdc = frl->scdc;
	return 0;
}

s32 hdmitx_frl_start(struct hdmitx_ctrl *hdmitx)
{
	bool continue_mach = false;
	struct hdmitx_frl *frl = NULL;

	if (hdmitx == NULL || hdmitx->frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return -1;
	}
	frl = hdmitx->frl;

	frl_training_state_machine_reset(frl);

	/* to fix : if (frl->force_out) */

	do
		continue_mach = frl_training_state_machine_step(frl);
	while (continue_mach);

	if (frl->stat.event == TRAIN_EVENT_SUCCESS)
		frl->stat.frl_start = true;

	return frl->stat.event;
}

void drv_hdmitx_frl_stop(struct hdmitx_frl *frl)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	hal_frl_set_source_data(&frl->hal, FRL_TRAINING_PATTERN);
	hal_frl_set_training_rate(&frl->hal, 0);
	frl_training_clear_ffe(frl);
	frl_training_state_machine_stop(frl);

	dpu_pr_info("[hdmitx%d]stop FRL!\n", frl->id);
}

void drv_hdmitx_frl_clr_rate(struct hdmitx_frl *frl)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	sink_cleanup_frl_rate(frl);
	sink_clear_flt_update(frl);
}

void drv_hdmitx_frl_deinit(struct hdmitx_frl *frl)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	del_timer(&frl->timer);
	cancel_delayed_work_sync(&frl->dl_work);
	// delayedwork_destroy
	hal_frl_deinit(&frl->hal);
	kfree(frl);
}

