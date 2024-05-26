// ****************************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    orb_common.h
// Description:
//
// Version      1.0
// Date         2019-05-21
// ****************************************************************

#ifndef _ORBCOMMON_CS_H
#define _ORBCOMMON_CS_H

#define ORB_LAYER_MAX          8
#define ORB_GRIDSTAT_RANGE      748
#define ORB_SCORE_TH_RANGE      94
#define ORB_KPT_NUM_RANGE       187
#define ORB_PATTERN_RANGE      256
#define MATCHER_KPT_MAX          4096
#define CPMPARE_OUT_REG_NUM        4096
#define MC_OUT_REG_NUM        3200

#define FRAME_NUM_MAX            8

#define ORB_OUT_Y_SIZE          (992*744)
// (80*1600+0x8000) 80B(Freak) or 64B(Brief)
#define ORB_OUT_DESC_SIZE       (80*MATCHER_KPT_MAX + 0x8000)
#define ORB_OUT_SCORE_MAP_SIZE  ORB_OUT_Y_SIZE
#define ORB_OUT_GRID_STAT_SIZE  1496
#define ORB_OUT_SCORE_HIST_SIZE 252

#define RDR_OUT_DESC_SIZE        (96*MATCHER_KPT_MAX + 0x8000)
#define CMP_IN_INDEX_NUM         RDR_OUT_DESC_SIZE
#define CMP_OUT_RESULT_SIZE      (4*CMP_IN_INDEX_NUM)

enum cmdlst_frame_prio_e {
	CMD_PRIO_LOW  = 0,
	CMD_PRIO_HIGH = 1,
};

enum orcm_cascade_en_e {
	CASCADE_DISABLE  = 0,
	CASCADE_ENABLE = 1,
};

struct orb_stream_t {
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int buf;
	enum pix_format_e format;
};

#endif
