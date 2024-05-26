/*
 * Copyright (c) CompanyNameMagicTag. 2019-2020. All rights reserved.
 * Description: hdmi driver frl trainning header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_FRL_H__
#define __HDMITX_FRL_H__

#include <linux/timer.h>

#include "hdmitx_frl_config.h"

/* TRAIN_time in ms */
#define TRAIN_TFLT                    300
#define TRAIN_READY_TIMEOUT           200
#define TRAIN_TFLT_MARGIN             100
#define TRAIN_UPDATE_TIMEOUT_MAGIN    100
#define TRAIN_LTSP_POLL_INTERVAL      1000

#define msecs_to_timer(x) ((jiffies) + (msecs_to_jiffies(x)))

/* Link Training Event */
#define TRAIN_EVENT_DISABLE       0
#define TRAIN_EVENT_SUCCESS       1
#define TRAIN_EVENT_SINK_NO_SCDC  2
#define TRAIN_EVENT_READY_TIMEOUT 3
#define TRAIN_EVENT_TFLT_TIMEOUT  4
#define TRAIN_EVENT_LAST_RATE     5
#define TRAIN_EVENT_LTSP_TIMEOUT  6
#define TRAIN_EVENT_DDC_ERR       7

/* see SPEC  Table 10-18: SCDCS - Update Flags */
#define TRAIN_FLT_START_MASK  (0x1 << 4)
#define TRAIN_FLT_UPDATE_MASK (0x1 << 5)

/* see SPEC  Table 10-21: SCDCS - Sink Configuration */
#define TRAIN_FFE_LEVELS_MASK (0xf << 4)
#define TRAIN_FRL_RATE_MASK   (0xf << 0)

/* see SPEC  Table 10-22: SCDCS - Source Test Configuration */
#define TRAIN_FLT_NO_TIMEOUT_MASK (0x1 << 5)
#define TRAIN_DSC_FRL_MAX_MASK    (0x1 << 6)
#define TRAIN_FRL_MAX_MASK        (0x1 << 7)

/* see SPEC  Table 10-23: SCDCS - Status Flags */
#define TRAIN_FLT_READY_MASK   (0x1 << 6)
#define TRAIN_LN3_LTP_REQ_MASK (0xf << 4)
#define TRAIN_LN2_LTP_REQ_MASK (0xf << 0)
#define TRAIN_LN1_LTP_REQ_MASK (0xf << 4)
#define TRAIN_LN0_LTP_REQ_MASK (0xf << 0)

#define TIMER_DEFAULT_2MS 0xBB80

enum frl_lane_num {
	FRL_LANE_NUM_0,
	FRL_LANE_NUM_1,
	FRL_LANE_NUM_2,
	FRL_LANE_NUM_3,
	FRL_LANE_NUM_MAX
};

struct frl_config {
	bool max_rate_proir;    /* max_frl_rate traning prior */
	u32 ready_timeout;      /* FLT_Ready timeout in LTS 2,default 200ms */
	u32 tflt_margin;        /* tFLT(200ms) margin in LTS 3£¬default 100ms */
	u32 update_flag_magin;  /* FLT_update & FRL_Start timeout margin in LTS P £¬default 100ms */
	u32 ltsp_poll_interval; /* FLT_update polling interval time in LTS P£¬default 150ms */

	/*
	 * frl max rate,the lesser of the Max_FRL_Rate from the Sink¡¯s HF-VSDB and
	 * the maximum FRL_Rate the source support
	 */
	u32 frl_max_rate;
	u32 frl_min_rate;       /* frl min rate for the transmit mode (vic & color depth & color format) */
	u32 dsc_frl_min_rate;   /* dsc frl min rate for the transmit mode (vic & color depth & color format) */
	bool scdc_present;      /* SCDC_Present in HF-VSDB */
	u8 src_ffe_levels;      /* ffe_levels */
};

struct frl_scdc {
	u8 frl_rate; /* FRL_Rate. */
	/*
	 * FFE_Levels.The Source shall set this field to indicate the maximum TxFFE level
	 * supported for the current FRL Rate.Values greater than 3 are reserved.
	 */
	u8 ffe_levels;
	bool flt_no_timeout; /* FLT_no_timeout */
	bool frl_max; /* FRL_max. */
	bool dsc_frl_max; /* DSC_FRL_max. */
	u32 sink_ver;        /* sink scdc version. */
	bool flt_update; /* FLT_update. */
	bool flt_start; /* FLT_start.Link Training is successful and the Sink is ready to receive video */
	bool flt_ready; /* FLT_ready.The Sink shall set (=1) FLT_ready when the Sink is ready for Link Training */
	u8 ltp_req[FRL_LANE_NUM_MAX]; /* Link Training Pattern requested by the Sink for Lane0~3 */
};

struct frl_stat {
	bool ready_timeout;   /* true-LTS 2 FLT_ready timeout occur;false- timeout didn't occur */
	bool tflt_timeout;    /* true-LTS 3 tFLT timeout occur;false- timeout didn't occur */
	bool ltsp_timeout;    /* true-LTS P polling FRL_Update;false-disable */
	bool ltsp_poll;       /* true-LTS P polling FRL_Update;false-disable */
	bool phy_output;      /* true-phy output signal;false-disable */
	bool video_transifer; /* true-transmiting active normal video etc(WorkEn);false-disable */
	/*
	 * true-FRL transmission start, including Gap Characters etc;false-disalbe
	 * FRL transmission,select pattern channel.
	 */
	bool frl_start;
	/* FFE level in each lane.[0] for ln0;[1] for ln1;[2] for ln2;[3] for ln3. */
	u8 ffe_levels[FRL_LANE_NUM_MAX];
	bool work_3lane;      /* if true,in 3 lane mode;if false,in 4 lane mode */
	u32 frl_state;        /* see Link Training State */
	u32 event;            /* see Link Training Event */
};

struct hdmitx_frl {
	u32 id;
	bool force_out;
	/* use for tFLT\flt_update\flt_ready timeout etc. */
	struct timer_list timer;
	/* use for LTS_P polling FRL_update flag */
	struct delayed_work dl_work;

	struct hdmitx_ddc *ddc;
	struct frl_config config;
	struct frl_scdc scdc;
	struct frl_stat stat;
	struct frl_reg hal;
};

s32 hdmitx_frl_start(struct hdmitx_ctrl *hdmitx);

s32 hdmitx_frl_init(struct hdmitx_ctrl *hdmitx);
s32 drv_hdmitx_frl_set_config(struct hdmitx_frl *frl, const struct frl_config *config);
s32 drv_hdmitx_frl_get_config(const struct hdmitx_frl *frl, struct frl_config *config);
s32 drv_hdmitx_frl_start(struct hdmitx_frl *frl);
void drv_hdmitx_frl_stop(struct hdmitx_frl *frl);
void drv_hdmitx_frl_clr_rate(struct hdmitx_frl *frl);
s32 drv_hdmitx_frl_get_scdc(const struct hdmitx_frl *frl, struct frl_scdc *scdc);
s32 drv_hdmitx_frl_get_stat(struct hdmitx_frl *frl, struct frl_stat *stat);
void drv_hdmitx_frl_set_worken(struct hdmitx_frl *frl, bool enable, bool fast_mode);
s32 drv_hdmitx_frl_debug(struct hdmitx_frl *frl_info, enum debug_ext_cmd_list cmd, struct hdmitx_debug_msg msg);
void drv_hdmitx_frl_deinit(struct hdmitx_frl *frl);
#endif /* __DRV_HDMITX_FRL_H__ */

