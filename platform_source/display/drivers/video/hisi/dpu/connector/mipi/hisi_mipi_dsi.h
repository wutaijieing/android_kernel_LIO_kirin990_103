/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
*/
#ifndef HISI_MIPI_DSI_H
#define HISI_MIPI_DSI_H

#include "hisi_fb.h"
#include "hisi_disp_gadgets.h"
#include "hisi_connector_utils.h"

/* mipi dsi panel */
enum {
	DSI_VIDEO_MODE,
	DSI_CMD_MODE,
};

enum {
	DSI_1_1_VERSION = 0,
	DSI_1_2_VERSION,
};

enum {
	DSI_1_LANES = 0,
	DSI_2_LANES,
	DSI_3_LANES,
	DSI_4_LANES,
};

enum {
	DSI_LANE_NUMS_DEFAULT = 0,
	DSI_1_LANES_SUPPORT = BIT(0),
	DSI_2_LANES_SUPPORT = BIT(1),
	DSI_3_LANES_SUPPORT = BIT(2),
	DSI_4_LANES_SUPPORT = BIT(3),
};

enum {
	DSI_16BITS_1 = 0,
	DSI_16BITS_2,
	DSI_16BITS_3,
	DSI_18BITS_1,
	DSI_18BITS_2,
	DSI_24BITS_1,
	DSI_24BITS_2,
	DSI_24BITS_3,
	DSI_DSC24_COMPRESSED_DATA = 0xF,
};

enum {
	DSI_NON_BURST_SYNC_PULSES = 0,
	DSI_NON_BURST_SYNC_EVENTS,
	DSI_BURST_SYNC_PULSES_1,
	DSI_BURST_SYNC_PULSES_2,
};

enum {
	EN_DSI_TX_NORMAL_MODE = 0x0,
	EN_DSI_TX_LOW_PRIORITY_DELAY_MODE = 0x1,
	EN_DSI_TX_HIGH_PRIORITY_DELAY_MODE = 0x2,
	EN_DSI_TX_AUTO_MODE = 0xF,
};

enum MIPI_LP11_MODE {
	MIPI_NORMAL_LP11 = 0,
	MIPI_SHORT_LP11 = 1,
	MIPI_DISABLE_LP11 = 2,
};

enum V320_DPHY_VER {
	DPHY_VER_12 = 0,
	DPHY_VER_14,
	DPHY_VER_MAX
};

enum DSI_TE_TYPE {
	DSI0_TE0_TYPE = 0,  /* include video lcd */
	DSI1_TE0_TYPE = 1,
	DSI1_TE1_TYPE = 2,
};

enum DSI_INDEX {
	DSI0_INDEX = 0,
	DSI1_INDEX = 1,
};

#define V_FRONT_PORCH_MAX 1023

#define DSI_VIDEO_DST_FORMAT_RGB565 0
#define DSI_VIDEO_DST_FORMAT_RGB666 1
#define DSI_VIDEO_DST_FORMAT_RGB666_LOOSE 2
#define DSI_VIDEO_DST_FORMAT_RGB888 3

#define DSI_CMD_DST_FORMAT_RGB565 0
#define DSI_CMD_DST_FORMAT_RGB666 1
#define DSI_CMD_DST_FORMAT_RGB888 2

#define GEN_VID_LP_CMD BIT(24) /* vid lowpwr cmd write */
/* dcs read/write */
#define DTYPE_DCS_WRITE 0x05 /* short write, 0 parameter */
#define DTYPE_DCS_WRITE1 0x15 /* short write, 1 parameter */
#define DTYPE_DCS_WRITE2 0x07 /* short write, 2 parameter */
#define DTYPE_DCS_READ 0x06 /* read */
#define DTYPE_DCS_LWRITE 0x39 /* long write */
#define DTYPE_DSC_LWRITE 0x0A /* dsc dsi1.2 vase3x long write */

/* generic read/write */
#define DTYPE_GEN_WRITE 0x03 /* short write, 0 parameter */
#define DTYPE_GEN_WRITE1 0x13 /* short write, 1 parameter */
#define DTYPE_GEN_WRITE2 0x23 /* short write, 2 parameter */
#define DTYPE_GEN_LWRITE 0x29 /* long write */
#define DTYPE_GEN_READ 0x04 /* long read, 0 parameter */
#define DTYPE_GEN_READ1 0x14 /* long read, 1 parameter */
#define DTYPE_GEN_READ2 0x24 /* long read, 2 parameter */

#define DTYPE_TEAR_ON 0x35 /* set tear on */
#define DTYPE_MAX_PKTSIZE 0x37 /* set max packet size */
#define DTYPE_NULL_PKT 0x09 /* null packet, no data */
#define DTYPE_BLANK_PKT 0x19 /* blankiing packet, no data */

#define DTYPE_CM_ON 0x02 /* color mode off */
#define DTYPE_CM_OFF 0x12 /* color mode on */
#define DTYPE_PERIPHERAL_OFF 0x22
#define DTYPE_PERIPHERAL_ON 0x32

#define DSI_HDR_DTYPE(dtype) ((dtype) & 0x03f)
#define DSI_HDR_VC(vc) (((vc) & 0x03) << 6)
#define DSI_HDR_DATA1(data) (((data) & 0x0ff) << 8)
#define DSI_HDR_DATA2(data) (((data) & 0x0ff) << 16)
#define DSI_HDR_WC(wc) (((wc) & 0x0ffff) << 8)

#define DSI_PLD_DATA1(data) ((data) & 0x0ff)
#define DSI_PLD_DATA2(data) (((data) & 0x0ff) << 8)
#define DSI_PLD_DATA3(data) (((data) & 0x0ff) << 16)
#define DSI_PLD_DATA4(data) (((data) & 0x0ff) << 24)

#define MIN_T3_PREPARE_PARAM 450
#define MIN_T3_LPX_PARAM 500
#define MIN_T3_PREBEGIN_PARAM 21
#define MIN_T3_POST_PARAM 77
#define MIN_T3_PREBEGIN_PHY_TIMING 0x21
#define MIN_T3_POST_PHY_TIMING 0xf

#define T3_PREPARE_PARAM 650
#define T3_LPX_PARAM 600
#define T3_PREBEGIN_PARAM 350
#define T3_POST_PARAM 224
#define T3_PREBEGIN_PHY_TIMING 0x3e
#define T3_POST_PHY_TIMING 0x1e

#define MIPI_DSI_BIT_CLK_STR1 "00001"
#define MIPI_DSI_BIT_CLK_STR2 "00010"
#define MIPI_DSI_BIT_CLK_STR3 "00100"
#define MIPI_DSI_BIT_CLK_STR4 "01000"
#define MIPI_DSI_BIT_CLK_STR5 "10000"

#define PLL_PRE_DIV_ADDR 0x00010048
#define PLL_POS_DIV_ADDR 0x00010049
#define PLL_FBK_DIV_ADDR 0x0001004A
#define PLL_UPT_CTRL_ADDR 0x0001004B

#define PHY_LOCK_STANDARD_STATUS 0x00000001
#define PLL_FBK_DIV_MAX_VALUE 0xFF
#define SHIFT_4BIT 4
#define SHIFT_8BIT 8

struct dsi_cmd_desc {
	uint32_t dtype;
	uint32_t vc;
	uint32_t wait;
	uint32_t waittype;
	uint32_t dlen;
	char *payload;
};

struct mipi_dsi_read_compare_data {
	uint32_t *read_value;
	uint32_t *expected_value;
	uint32_t *read_mask;
	char **reg_name;
	int log_on;
	struct dsi_cmd_desc *cmds;
	int cnt;
};

#define DEFAULT_MAX_TX_ESC_CLK (10 * 1000000UL)

#define DEFAULT_MIPI_CLK_RATE (192 * 100000L)
#define DEFAULT_PCLK_DSI_RATE (120 * 1000000L)

#define MAX_CMD_QUEUE_LOW_PRIORITY_SIZE 256
#define MAX_CMD_QUEUE_HIGH_PRIORITY_SIZE 128
#define CMD_AUTO_MODE_THRESHOLD (2 / 3)

struct dsi_delayed_cmd_queue {
	struct semaphore work_queue_sem;
	spinlock_t CmdSend_lock;
	struct dsi_cmd_desc CmdQueue_LowPriority[MAX_CMD_QUEUE_LOW_PRIORITY_SIZE];
	uint32_t CmdQueue_LowPriority_rd;
	uint32_t CmdQueue_LowPriority_wr;
	bool isCmdQueue_LowPriority_full;
	bool isCmdQueue_LowPriority_working;
	spinlock_t CmdQueue_LowPriority_lock;

	struct dsi_cmd_desc CmdQueue_HighPriority[MAX_CMD_QUEUE_HIGH_PRIORITY_SIZE];
	uint32_t CmdQueue_HighPriority_rd;
	uint32_t CmdQueue_HighPriority_wr;
	bool isCmdQueue_HighPriority_full;
	bool isCmdQueue_HighPriority_working;
	spinlock_t CmdQueue_HighPriority_lock;

	s64 timestamp_frame_start;
	s64 oneframe_time;
};

enum mipi_dsi_phy_mode {
	DPHY_MODE = 0,
	CPHY_MODE,
};

struct mipi_dsi_timing {
	uint32_t hsa;
	uint32_t hbp;
	uint32_t dpi_hsize;
	uint32_t width;
	uint32_t hline_time;

	uint32_t vsa;
	uint32_t vbp;
	uint32_t vactive_line;
	uint32_t vfp;
};

/*
 * FUNCTIONS PROTOTYPES
 */

int dpu_mipi_dsi_cmds_tx(struct dsi_cmd_desc *cmds, int cnt, char __iomem *dsi_base);
int dpu_mipi_dsi_cmds_rx(uint32_t *out, struct dsi_cmd_desc *cmds, int cnt, char __iomem *dsi_base);

void dpu_mipi_dsi_dphy_on_fpga(struct hisi_connector_device *connector_dev);
void dpu_mipi_dsi_dphy_off_fpga(struct hisi_connector_device *connector_dev);
void mipi_dsi_peri_dis_reset(struct hisi_connector_device *connector_dev);
void mipi_dsi_peri_reset(struct hisi_connector_device *connector_dev);
void mipi_transfer_lock_init(void);
/*
 * connector_info: different for master and slave
 */
void mipi_dsi_set_lp_mode(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info);
void mipi_dsi_set_hs_mode(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info);

void mipi_dsi_off_sub1(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info);
void mipi_dsi_off_sub2(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info);

#endif  /* HISI_MIPI_DSI_H */

