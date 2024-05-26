/*
 * used for soundwire bandwidth self-adaptive arrangement
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#include "soundwire_bandwidth_adp.h"

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/of.h>

#include "audio_log.h"
#include "soundwire_type.h"
#include "soundwire_master_reg.h"
#include "soundwire_slave_reg.h"
#include "soundwire_utils.h"

#define SDW_DATA_LANE_NUM_MAX 2
#define SDW_BITS_PER_CLK 2
#define SDW_BANK_NUM 2
#define SDW_DP_REG_BASE_OFFSET 0x100
#define SDW_H_CTRL_VALUE 0x1f
#define SDW_BLOCK_CTRL2_VALUE 0x0
#define SDW_BLOCK_CTRL3_VALUE 0x1
#define SDW_FRAME_CTRL_VALUE 0x1
#define SDW_FRAME_ROWS 48
#define SDW_FRAME_COLS 4
#define SDW_WAIT_CNT 40
#define SDW_MAX_FREE_BLKS_NUM 48

#define LOG_TAG "sdw_bw_adp"

enum soundwire_reg_type {
	REG_TYPE_MST,
	REG_TYPE_SLV,
	REG_TYPE_MAX
};

enum soundwire_clk_div {
	CLK_DIV_UNKNOWN = -1,
	CLK_DIV_2,
	CLK_DIV_4,
	CLK_DIV_8,
	CLK_DIV_16,
	CLK_DIV_MAX
};

enum soundwire_dp_reg_type {
	DP_REG_INTERRUPT_STAT,
	DP_REG_PORT_CTRL,
	DP_REG_BLOCK_CTRL1,
	DP_REG_BLOCK_CTRL2,
	DP_REG_BLOCK_CTRL3,
	DP_REG_PREPARE_STATUS,
	DP_REG_PREPARE_CTRL,
	DP_REG_CHANNEL_EN,
	DP_REG_SAMPLE_CTRL1,
	DP_REG_SAMPLE_CTRL2,
	DP_REG_OFFSET_CTRL1,
	DP_REG_OFFSET_CTRL2,
	DP_REG_H_CTRL,
	DP_REG_LANE_CTRL,
	DP_REG_MAX
};

struct soundwire_fifo_reg {
	uint32_t reg_offset;
	uint32_t bit_offset;
};

struct soundwire_data_blk {
	int offset;
	int size;
};

struct soundwire_transport_frame {
	unsigned char lane;
	unsigned char h_start;                    /* left edge of sub frame */
	unsigned char h_stop;                     /* right edge of sub frame */
	unsigned int sample_interval;
	unsigned int transport_bit_slots;         /* bits of slot that used for transport */

	struct soundwire_data_blk free_blks[SDW_MAX_FREE_BLKS_NUM];  /* maximum sample rate(384k) / minimum sample rate(8k) */
	unsigned int free_blks_num;
};

struct soundwire_bandwidth_arrangement {
	struct soundwire_priv *priv;
	struct soundwire_payload payloads[DP_NUM];
	unsigned int payloads_num;
	struct soundwire_reg curr_reg;
	struct soundwire_reg prev_reg;
	int data_lane_num;
};

static const unsigned int dp1_reg_offset[REG_TYPE_MAX][SDW_BANK_NUM][DP_REG_MAX] =
{
	{
		{
			MST_DP1_INTSTAT_OFFSET,             MST_DP1_PORTCTRL_OFFSET,
			MST_DP1_BLOCKCTRL1_OFFSET,          MST_DP1_BLOCKCTRL2_BANK0_OFFSET,
			MST_DP1_BLOCKCTRL3_BANK0_OFFSET,    MST_DP1_PREPARESTATUS_OFFSET,
			MST_DP1_PREPARECTRL_OFFSET,         MST_DP1_CHANNELEN_BANK0_OFFSET,
			MST_DP1_SAMPLECTRL1_BANK0_OFFSET,   MST_DP1_SAMPLECTRL2_BANK0_OFFSET,
			MST_DP1_OFFSETCTRL1_BANK0_OFFSET,   MST_DP1_OFFSETCTRL2_BANK0_OFFSET,
			MST_DP1_HCTRL_BANK0_OFFSET,         MST_DP1_LANECTRL_BANK0_OFFSET
		},
		{
			MST_DP1_INTSTAT_OFFSET,             MST_DP1_PORTCTRL_OFFSET,
			MST_DP1_BLOCKCTRL1_OFFSET,          MST_DP1_BLOCKCTRL2_BANK1_OFFSET,
			MST_DP1_BLOCKCTRL3_BANK1_OFFSET,    MST_DP1_PREPARESTATUS_OFFSET,
			MST_DP1_PREPARECTRL_OFFSET,         MST_DP1_CHANNELEN_BANK1_OFFSET,
			MST_DP1_SAMPLECTRL1_BANK1_OFFSET,   MST_DP1_SAMPLECTRL2_BANK1_OFFSET,
			MST_DP1_OFFSETCTRL1_BANK1_OFFSET,   MST_DP1_OFFSETCTRL2_BANK1_OFFSET,
			MST_DP1_HCTRL_BANK1_OFFSET,         MST_DP1_LANECTRL_BANK1_OFFSET
		}
	},
	{
		{
			SLV_DP1_INTSTAT_OFFSET,             SLV_DP1_PORTCTRL_OFFSET,
			SLV_DP1_BLOCKCTRL1_OFFSET,          SLV_DP1_BLOCKCTRL2_BANK0_OFFSET,
			SLV_DP1_BLOCKCTRL3_BANK0_OFFSET,    SLV_DP1_PREPARESTATUS_OFFSET,
			SLV_DP1_PREPARECTRL_OFFSET,         SLV_DP1_CHANNELEN_BANK0_OFFSET,
			SLV_DP1_SAMPLECTRL1_BANK0_OFFSET,   SLV_DP1_SAMPLECTRL2_BANK0_OFFSET,
			SLV_DP1_OFFSETCTRL1_BANK0_OFFSET,   SLV_DP1_OFFSETCTRL2_BANK0_OFFSET,
			SLV_DP1_HCTRL_BANK0_OFFSET,         SLV_DP1_LANECTRL_BANK0_OFFSET
		},
		{
			SLV_DP1_INTSTAT_OFFSET,             SLV_DP1_PORTCTRL_OFFSET,
			SLV_DP1_BLOCKCTRL1_OFFSET,          SLV_DP1_BLOCKCTRL2_BANK1_OFFSET,
			SLV_DP1_BLOCKCTRL3_BANK1_OFFSET,    SLV_DP1_PREPARESTATUS_OFFSET,
			SLV_DP1_PREPARECTRL_OFFSET,         SLV_DP1_CHANNELEN_BANK1_OFFSET,
			SLV_DP1_SAMPLECTRL1_BANK1_OFFSET,   SLV_DP1_SAMPLECTRL2_BANK1_OFFSET,
			SLV_DP1_OFFSETCTRL1_BANK1_OFFSET,   SLV_DP1_OFFSETCTRL2_BANK1_OFFSET,
			SLV_DP1_HCTRL_BANK1_OFFSET,         SLV_DP1_LANECTRL_BANK1_OFFSET
		}
	}
};

struct soundwire_fifo_reg mst_fifo_regs[DP_NUM] = {
	{ 0x0, 0 },
	{ 0x8d, 1 << 0 },
	{ 0x8d, 1 << 1 },
	{ 0x8d, 1 << 2 },
	{ 0x8d, 1 << 3 },
	{ 0x8d, 1 << 4 },
	{ 0x8d, 1 << 5 },
	{ 0x8d, 1 << 6 },
	{ 0x8f, 1 << 0 },
	{ 0x8f, 1 << 1 },
	{ 0x8e, 1 << 2 },
	{ 0x8e, 1 << 3 },
	{ 0x8e, 1 << 4 },
	{ 0x8e, 1 << 5 },
	{ 0x8e, 1 << 6 },
};

struct soundwire_fifo_reg slv_fifo_regs[DP_NUM] = {
	{ 0x0, 0 },
	{ 0x89, 1 << 0 },
	{ 0x89, 1 << 1 },
	{ 0x89, 1 << 2 },
	{ 0x89, 1 << 3 },
	{ 0x89, 1 << 4 },
	{ 0x89, 1 << 5 },
	{ 0x89, 1 << 6 },
	{ 0x8A, 1 << 0 },
	{ 0x8A, 1 << 1 },
	{ 0x8A, 1 << 2 },
	{ 0x8A, 1 << 3 },
	{ 0x8A, 1 << 4 },
	{ 0x8A, 1 << 5 },
	{ 0x8A, 1 << 6 },
};


static const unsigned int g_sdw_clk[CLK_DIV_MAX] = {
	12288000, 6144000, 3072000, 1536000
};

static struct soundwire_bandwidth_arrangement *g_arrangement = NULL;

/* kzalloc is a static inline function, wrapped for llt mocker */
void *sdw_kzalloc(size_t size, gfp_t flags)
{
	return kzalloc(size, flags);
}

static unsigned int sdw_get_dp_addr_offset(enum soundwire_reg_type reg_type,
	enum soundwire_dp_reg_type dp_type, int dp_id, int bank)
{
	unsigned int offset = 0;

	if (dp_id >= DP_NUM || dp_id <= 0 || bank > 2 || bank < 0)
		return 0;

	offset = dp1_reg_offset[reg_type][bank][dp_type];
	offset += (dp_id - 1) * SDW_DP_REG_BASE_OFFSET;

	return offset;
}

static unsigned char sdw_get_port_flow_mode(struct soundwire_payload *payload)
{
	/* 1 means push mode, 2 means pull mode, select push mode permanently */

	return payload->special_sample_rate ? 1 : 0;
}

static unsigned char sdw_get_flow_ctrl_bits(struct soundwire_payload *payload)
{
	/* special sample rate(44.1/88.2/...), must use tx/rx/full port flow control mode,
	 *  which use 2bits at the lowest numbered channel */

	return payload->special_sample_rate ? 2 : 0;
}

static inline unsigned int sdw_get_transport_bit_slots(unsigned int sample_interval, unsigned int cols_num)
{
	return (sample_interval / SDW_FRAME_COLS) * cols_num;
}

static int sdw_get_bank(bool selected)
{
	MST_BANK_SW_MONITOR_UNION mst_bank_monitor_stat;

	mst_bank_monitor_stat.value = mst_read_reg(g_arrangement->priv, MST_BANK_SW_MONITOR_OFFSET);

	if (mst_bank_monitor_stat.reg.frame_bank0_sel == 1)
		return selected ? 0 : 1;
	else if (mst_bank_monitor_stat.reg.frame_bank1_sel == 1)
		return selected ? 1 : 0;

	return -1;
}

static inline int sdw_get_selected_bank()
{
	return sdw_get_bank(true);
}

static inline int sdw_get_unselected_bank()
{
	return sdw_get_bank(false);
}

static void sdw_clear_fifo(uint32_t dp_id)
{
	mst_write_reg(g_arrangement->priv, 0xff & (~mst_fifo_regs[dp_id].bit_offset), mst_fifo_regs[dp_id].reg_offset);
	slv_write_reg(g_arrangement->priv, 0xff & (~slv_fifo_regs[dp_id].bit_offset), slv_fifo_regs[dp_id].reg_offset);
	mst_write_reg(g_arrangement->priv, 0xff, mst_fifo_regs[dp_id].reg_offset);
	slv_write_reg(g_arrangement->priv, 0xff, slv_fifo_regs[dp_id].reg_offset);
}

static void write_clk_div_ctrl_reg(unsigned char clk)
{
	mst_write_reg(g_arrangement->priv, clk, MST_CLKDIVCTRL_OFFSET);
	slv_write_reg(g_arrangement->priv, clk, SLV_CLKDIVCTRL_OFFSET);
}

static void write_port_ctrl_reg(unsigned char port_flow_mode, int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, port_flow_mode,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_PORT_CTRL, dp_id, bank));
	slv_write_reg(g_arrangement->priv, port_flow_mode,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_PORT_CTRL, dp_id, bank));
}

static void write_block_ctrl_reg(unsigned char word_length, unsigned char block_packing_mode,
	int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, word_length - 1,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_BLOCK_CTRL1, dp_id, bank));
	slv_write_reg(g_arrangement->priv, word_length - 1,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_BLOCK_CTRL1, dp_id, bank));

	mst_write_reg(g_arrangement->priv, block_packing_mode,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_BLOCK_CTRL3, dp_id, bank));
	slv_write_reg(g_arrangement->priv, block_packing_mode,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_BLOCK_CTRL3, dp_id, bank));
}

static void write_sample_ctrl_reg(unsigned short sample_interval, int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, (sample_interval - 1) & 0xff,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_SAMPLE_CTRL1, dp_id, bank));
	slv_write_reg(g_arrangement->priv, (sample_interval - 1) & 0xff,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_SAMPLE_CTRL1, dp_id, bank));

	mst_write_reg(g_arrangement->priv, (sample_interval - 1) >> 8,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_SAMPLE_CTRL2, dp_id, bank));
	slv_write_reg(g_arrangement->priv, (sample_interval - 1) >> 8,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_SAMPLE_CTRL2, dp_id, bank));
}

static void write_offset_ctrl_reg(int block_offset, int sub_block_offset,
	int dp_id, int bank)
{
	/* block per channel mode, block offset maybe overflow */
	mst_write_reg(g_arrangement->priv, block_offset,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_OFFSET_CTRL1, dp_id, bank));
	slv_write_reg(g_arrangement->priv, block_offset,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_OFFSET_CTRL1, dp_id, bank));

	mst_write_reg(g_arrangement->priv, sub_block_offset,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_OFFSET_CTRL2, dp_id, bank));
	slv_write_reg(g_arrangement->priv, sub_block_offset,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_OFFSET_CTRL2, dp_id, bank));
}

static void write_h_ctrl_reg(unsigned char h_ctrl, int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, h_ctrl,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_H_CTRL, dp_id, bank));
	slv_write_reg(g_arrangement->priv, h_ctrl,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_H_CTRL, dp_id, bank));
}

static void write_lane_ctrl_reg(unsigned char lane_ctrl, int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, lane_ctrl,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_LANE_CTRL, dp_id, bank));
	slv_write_reg(g_arrangement->priv, lane_ctrl,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_LANE_CTRL, dp_id, bank));
}

static void wait_for_channel_prepared(enum soundwire_reg_type reg_type, int dp_id, int bank)
{
	unsigned int cnt = SDW_WAIT_CNT;
	unsigned int offset;
	unsigned char ret = 0;

	while (cnt) {
		offset = sdw_get_dp_addr_offset(reg_type, DP_REG_PREPARE_STATUS, dp_id, bank);
		ret = (reg_type == REG_TYPE_MST) ?
			mst_read_reg(g_arrangement->priv, offset) : slv_read_reg(g_arrangement->priv, offset);
		if (ret == 0)
			return;
		usleep_range(50, 60);
		cnt--;
	}

	AUDIO_LOGE("mst dp channel %d prepare failed", dp_id);
}

static void write_prepare_ctrl_reg(unsigned char channel_mask, int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, channel_mask,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_PREPARE_CTRL, dp_id, bank));
	wait_for_channel_prepared(REG_TYPE_MST, dp_id, bank);

	slv_write_reg(g_arrangement->priv, channel_mask,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_PREPARE_CTRL, dp_id, bank));
	wait_for_channel_prepared(REG_TYPE_SLV, dp_id, bank);
}

static void write_channelen_reg(unsigned char channel_mask, int dp_id, int bank)
{
	mst_write_reg(g_arrangement->priv, channel_mask,
		sdw_get_dp_addr_offset(REG_TYPE_MST, DP_REG_CHANNEL_EN, dp_id, bank));
	slv_write_reg(g_arrangement->priv, channel_mask,
		sdw_get_dp_addr_offset(REG_TYPE_SLV, DP_REG_CHANNEL_EN, dp_id, bank));
}

static void write_dp_reg(struct soundwire_dp *dp, int dp_num)
{
	int i;
	int bank = sdw_get_unselected_bank();

	for (i = 0; i < dp_num; i++) {
		if (!dp[i].enable)
			continue;

		if (!g_arrangement->prev_reg.dp[i].enable)
			sdw_clear_fifo(i); /* first use, need to clear fifo */

		write_port_ctrl_reg(dp[i].port_flow_mode, i, bank);
		write_block_ctrl_reg(dp[i].word_length, dp[i].block_packing_mode, i, bank);
		write_sample_ctrl_reg(dp[i].sample_interval, i, bank);
		write_offset_ctrl_reg(dp[i].block_offset, dp[i].sub_block_offset, i, bank);
		write_h_ctrl_reg(dp[i].h_ctrl, i, bank);
		write_lane_ctrl_reg(dp[i].lane_ctrl, i, bank);
		write_prepare_ctrl_reg(dp[i].channel_mask, i, bank);
		write_channelen_reg(dp[i].channel_mask, i, bank);
	}
}

static void write_frame_ctrl_reg(void)
{
	int bank = sdw_get_unselected_bank();

	unsigned int mst_frame_ctrl_offset = (bank == 0 ? MST_SCP_FRAMECTRL_BANK0_OFFSET :
		MST_SCP_FRAMECTRL_BANK1_OFFSET);

	unsigned int slv_frame_ctrl_offset = (bank == 0 ? SLV_SCP_FRAMECTRL_BANK0_OFFSET :
		SLV_SCP_FRAMECTRL_BANK1_OFFSET);

	mst_write_reg(g_arrangement->priv, SDW_FRAME_CTRL_VALUE, mst_frame_ctrl_offset);
	slv_write_reg(g_arrangement->priv, SDW_FRAME_CTRL_VALUE, slv_frame_ctrl_offset);
}

static bool sdw_check_payloads_valid(struct soundwire_payload *payloads, int payloads_num)
{
	int i;

	if (payloads == NULL || payloads_num == 0)
		return false;

	for (i = 0; i < payloads_num; i++) {
		if (payloads[i].dp_id > DP_NUM) {
			AUDIO_LOGE("invalid dp id %d", payloads[i].dp_id);
			return false;
		}
	}

	return true;
}

static int payload_dp_cmp(const void* a, const void* b)
{
	return ((struct soundwire_payload*)a)->dp_id - ((struct soundwire_payload*)b)->dp_id;
}

static bool sdw_check_payloads_conflict(struct soundwire_payload *payloads, int payloads_num)
{
	int i;
	unsigned int j;

	sort(payloads, payloads_num, sizeof(struct soundwire_payload), payload_dp_cmp, NULL);

	for (i = 0; i < payloads_num; i++) {
		if (i < (payloads_num - 1) && payloads[i].dp_id == payloads[i + 1].dp_id)
			return false;

		for (j = 0; j < g_arrangement->payloads_num; j++) {
			if (payloads[i].dp_id == g_arrangement->payloads[j].dp_id &&
				payloads[i].priority <= g_arrangement->payloads[j].priority) {
				AUDIO_LOGE("conflict dp id %d", payloads[i].dp_id);
				return false;
			}
		}
	}

	return true;
}

static enum soundwire_clk_div sdw_calc_required_clk_div(struct soundwire_payload *payloads,
	int payloads_num)
{
	int i;
	unsigned int tmp;
	unsigned int required_bandwidth = 0;

	for (i = 0; i < payloads_num; i++)
		required_bandwidth +=
			payloads[i].sample_rate * payloads[i].channel_num * payloads[i].bits_per_sample;

	for (i = CLK_DIV_2; i > CLK_DIV_UNKNOWN; i--) {
		tmp = g_sdw_clk[i] * 2 * 2 * (SDW_FRAME_COLS - 1) / SDW_FRAME_COLS;
		if (tmp >= required_bandwidth)
			return i;
	}

	AUDIO_LOGE("required bandwidth(%u) is more than sdw supported", required_bandwidth);
	return CLK_DIV_UNKNOWN;
}

static int payload_sample_rate_cmp(const void* a, const void* b)
{
	return ((struct soundwire_payload*)b)->sample_rate - ((struct soundwire_payload*)a)->sample_rate;
}

static int sdw_merge_and_sort_payloads(struct soundwire_payload *payloads, int payloads_num,
	struct soundwire_payload **out, int *out_num)
{
	unsigned int i;
	int j, total_num;
	bool conflict = false;

	total_num = payloads_num + g_arrangement->payloads_num;
	*out = (struct soundwire_payload *)sdw_kzalloc(sizeof(struct soundwire_payload) * total_num, GFP_KERNEL);
	if (*out == NULL) {
		AUDIO_LOGE("out of memory");
		return -EFAULT;
	}

	memcpy((*out), payloads, payloads_num * sizeof(struct soundwire_payload));
	*out_num = payloads_num;

	for (i = 0; i < g_arrangement->payloads_num; ++i) {
		conflict = false;
		for (j = 0; j < payloads_num; j++) {
			if (g_arrangement->payloads[i].dp_id == payloads[j].dp_id &&
				payloads[j].priority > g_arrangement->payloads[i].priority) {
				conflict = true;
				break;
			}
		}
		if (!conflict) {
			memcpy((*out) + *out_num, g_arrangement->payloads + i, sizeof(struct soundwire_payload));
			*out_num = *out_num + 1;
		}
	}

	sort(*out, *out_num, sizeof(struct soundwire_payload), payload_sample_rate_cmp, NULL);
	return 0;
}

static void sdw_remove_payloads(struct soundwire_payload *payloads, int payloads_num)
{
	int i, j, k;

	for (i = 0; i < payloads_num; i++) {
		for (j = 0; j < g_arrangement->payloads_num; j++) {
			if (payloads[i].dp_id == g_arrangement->payloads[j].dp_id &&
				payloads[i].priority >= g_arrangement->payloads[j].priority) {
				for (k = j; k < g_arrangement->payloads_num - 1; k++)
					g_arrangement->payloads[k] = g_arrangement->payloads[k+1];

				memset(&g_arrangement->curr_reg.dp[payloads[i].dp_id], 0, sizeof(struct soundwire_dp));
				memset(&g_arrangement->prev_reg.dp[payloads[i].dp_id], 0, sizeof(struct soundwire_dp));
				write_channelen_reg(g_arrangement->curr_reg.dp[payloads[i].dp_id].channel_mask,
					payloads[i].dp_id, sdw_get_selected_bank());
				write_channelen_reg(g_arrangement->curr_reg.dp[payloads[i].dp_id].channel_mask,
					payloads[i].dp_id, sdw_get_unselected_bank());

				g_arrangement->payloads_num--;
				break;
			}
		}
	}
}

static void sdw_remap_sample_rate(struct soundwire_payload *payloads,
	int payloads_num)
{
	int i;

	for (i = 0; i < payloads_num; i++) {
		if (payloads[i].special_sample_rate) {
			continue;
		}

		/* 11025 is minimum value of special sample rate */
		if (payloads[i].sample_rate % 11025 != 0) {
			payloads[i].special_sample_rate = false;
			continue;
		}

		payloads[i].special_sample_rate = true;

		/* convert special sample rate to 12000's multiple */
		payloads[i].sample_rate = 12000 * (payloads[i].sample_rate / 11025);
	}
}

static void sdw_print_enabled_dp_config(struct soundwire_dp *dp_reg, int dp_num)
{
	int i = 0;

	for (; i < dp_num; i++) {
		if (!dp_reg[i].enable)
			continue;

		AUDIO_LOGI("dp%d config:", i);
		AUDIO_LOGI("sample_interval = %d, word_length = %d, channel_num = %d, "
			"channel_mask = 0x%x, block_packing_mode = %d",
			dp_reg[i].sample_interval,
			dp_reg[i].word_length,
			dp_reg[i].channel_num,
			dp_reg[i].channel_mask,
			dp_reg[i].block_packing_mode);
		AUDIO_LOGI("block_offset = %d, sub_block_offset = %d, port_flow_mode = %d, "
			"h_ctrl = %x, lane_ctrl = %d",
			dp_reg[i].block_offset,
			dp_reg[i].sub_block_offset,
			dp_reg[i].port_flow_mode,
			dp_reg[i].h_ctrl,
			dp_reg[i].lane_ctrl);
	}
}

static void sdw_config_bandwidth()
{
	if (g_arrangement != NULL) {
		intercore_try_lock(g_arrangement->priv);
		write_clk_div_ctrl_reg(g_arrangement->curr_reg.clk_div);
		write_dp_reg(g_arrangement->curr_reg.dp, DP_NUM);

		/* switch to unselected bank */
		write_frame_ctrl_reg();

		/* commit register config */
		g_arrangement->prev_reg = g_arrangement->curr_reg;

		/* write same configs for another bank, hifi will use these configs */
		write_dp_reg(g_arrangement->curr_reg.dp, DP_NUM);
		intercore_unlock(g_arrangement->priv);
	}
}

static void sdw_commit_layout(struct soundwire_payload *payloads, int payloads_num)
{
	if (payloads_num > DP_NUM || payloads_num < 0) {
		AUDIO_LOGE("payloads num error: %d", payloads_num);
		return;
	}

	g_arrangement->payloads_num = payloads_num;

	memcpy(g_arrangement->payloads, payloads, sizeof(struct soundwire_payload) * payloads_num);

	sdw_config_bandwidth();
}

int sdw_get_lane_num(struct soundwire_priv *priv)
{
	int lane_num = SDW_DATA_LANE_NUM_MAX;

	if (!of_property_read_u32(priv->dev->of_node, "data_lane_num", &lane_num))
		return lane_num > SDW_DATA_LANE_NUM_MAX ? SDW_DATA_LANE_NUM_MAX : lane_num;

	return lane_num;
}

static int sdw_find_blk(struct soundwire_transport_frame *trans_frame,
	int required_bit_slots)
{
	int i;

	for (i = 0; i < trans_frame->free_blks_num; i++) {
		if (trans_frame->free_blks[i].size >= required_bit_slots)
			return i;
	}

	return -1;
}

static void sdw_expand_blk(struct soundwire_transport_frame *trans_frame,
	unsigned int sample_interval)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int multiple;

	if (trans_frame->sample_interval == 0) {
		trans_frame->sample_interval = sample_interval;
		trans_frame->transport_bit_slots =
			sdw_get_transport_bit_slots(sample_interval, trans_frame->h_stop - trans_frame->h_start + 1);
		trans_frame->free_blks[0].size = trans_frame->transport_bit_slots;
		trans_frame->free_blks[0].offset = 0;
		trans_frame->free_blks_num = 1;
		return;
	}

	multiple = sample_interval / trans_frame->sample_interval;
	if (multiple <= 1)
		return;

	for (i = 1; i < multiple; i++) {
		for (j = 0; j < trans_frame->free_blks_num; j++) {
			unsigned int idx = trans_frame->free_blks_num * i + j;
			trans_frame->free_blks[idx].offset =
				trans_frame->free_blks[j].offset + trans_frame->transport_bit_slots * i;
			trans_frame->free_blks[idx].size = trans_frame->free_blks[j].size;
		}
	}

	trans_frame->free_blks_num *= multiple;
	trans_frame->sample_interval = sample_interval;
	trans_frame->transport_bit_slots =
		sdw_get_transport_bit_slots(sample_interval, trans_frame->h_stop - trans_frame->h_start + 1);
}

static void sdw_apply_blk(struct soundwire_transport_frame *trans_frame,
	int free_blk_idx, unsigned char dp_id, int required_bits)
{
	trans_frame->free_blks[free_blk_idx].offset += required_bits;
	trans_frame->free_blks[free_blk_idx].size -= required_bits;
}

static void sdw_save_dp_configs(struct soundwire_payload *payload,
	struct soundwire_transport_frame *trans_frame, struct soundwire_reg *reg)
{
	reg->dp[payload->dp_id].enable = true;
	reg->dp[payload->dp_id].sample_interval = trans_frame->sample_interval;
	reg->dp[payload->dp_id].word_length = payload->bits_per_sample;
	reg->dp[payload->dp_id].channel_num = payload->channel_num;
	reg->dp[payload->dp_id].channel_mask = payload->channel_mask;
	reg->dp[payload->dp_id].port_flow_mode = sdw_get_port_flow_mode(payload);
	reg->dp[payload->dp_id].h_ctrl = (trans_frame->h_start << 4) + trans_frame->h_stop;
	reg->dp[payload->dp_id].lane_ctrl = trans_frame->lane;
}

static int sdw_arrange_by_per_channel_mode(struct soundwire_payload *payload,
	struct soundwire_transport_frame *trans_frame, struct soundwire_reg *reg)
{
	int block_offset = 0;
	int sub_block_offset = 0;
	int low_ch_idx = 0;
	int low_ch_required_bits = payload->bits_per_sample + sdw_get_flow_ctrl_bits(payload);
	int high_ch_required_bits = payload->bits_per_sample;
	struct soundwire_transport_frame tmp_trans_frame = *trans_frame;

	low_ch_idx = sdw_find_blk(&tmp_trans_frame, low_ch_required_bits);
	if ((low_ch_idx < 0) || (low_ch_idx >= SDW_MAX_FREE_BLKS_NUM))
		return -EFAULT;

	block_offset = tmp_trans_frame.free_blks[low_ch_idx].offset;
	sdw_apply_blk(&tmp_trans_frame, low_ch_idx, payload->dp_id, low_ch_required_bits);

	if (payload->channel_num > 1) {
		int high_ch_idx = sdw_find_blk(&tmp_trans_frame, high_ch_required_bits);
		if ((high_ch_idx < 0) || (high_ch_idx >= SDW_MAX_FREE_BLKS_NUM))
			return -EFAULT;

		sub_block_offset = tmp_trans_frame.free_blks[high_ch_idx].offset - block_offset - low_ch_required_bits;
		sdw_apply_blk(&tmp_trans_frame, high_ch_idx, payload->dp_id, high_ch_required_bits);
	}

	/* the maximum valid value of block offset reg is 255 */
	if (block_offset > 255 || sub_block_offset > 255) {
		AUDIO_LOGW("block offset overflow, offset1 %d, offset2 %d", block_offset, sub_block_offset);
		return -EFAULT;
	}

	sdw_save_dp_configs(payload, &tmp_trans_frame, reg);
	reg->dp[payload->dp_id].block_packing_mode = 1;
	reg->dp[payload->dp_id].block_offset = block_offset;
	reg->dp[payload->dp_id].sub_block_offset = sub_block_offset;
	*trans_frame = tmp_trans_frame;

	return 0;
}

static int sdw_arrange_by_per_port_mode(struct soundwire_payload *payload,
	struct soundwire_transport_frame *trans_frame, struct soundwire_reg *reg)
{
	int block_offset = 0;
	int required_bit_slots = payload->bits_per_sample * payload->channel_num +
		sdw_get_flow_ctrl_bits(payload);

	int idx = sdw_find_blk(trans_frame, required_bit_slots);
	if (idx < 0)
		return -EFAULT;

	block_offset = trans_frame->free_blks[idx].offset;
	sdw_apply_blk(trans_frame, idx, payload->dp_id, required_bit_slots);

	sdw_save_dp_configs(payload, trans_frame, reg);
	reg->dp[payload->dp_id].block_packing_mode = 0;
	reg->dp[payload->dp_id].block_offset = block_offset;

	return 0;
}

static int sdw_arrange_payload(struct soundwire_payload *payload,
	struct soundwire_transport_frame *trans_frame, struct soundwire_reg *reg)
{
	unsigned int sample_interval;

	if (payload->sample_rate == 0)
		return -EFAULT;

	sample_interval = g_sdw_clk[reg->clk_div] * SDW_BITS_PER_CLK / payload->sample_rate;
	if (trans_frame->sample_interval != 0 && (sample_interval % trans_frame->sample_interval != 0))
		return -EFAULT;

	sdw_expand_blk(trans_frame, sample_interval);

	if (sdw_arrange_by_per_channel_mode(payload, trans_frame, reg) == 0)
		return 0;

	return sdw_arrange_by_per_port_mode(payload, trans_frame, reg);
}

static int sdw_dfs_payloads_arrangement(struct soundwire_payload *payloads, int payloads_num,
	struct soundwire_transport_frame *trans_frames[], int trans_frames_num, struct soundwire_reg reg)
{
	int i;

	if (payloads_num == 0) {
		g_arrangement->curr_reg = reg;
		return 0;
	}

	for (i = 0; i < trans_frames_num; i++) {
		struct soundwire_transport_frame ori_trans_frame = *trans_frames[i];
		struct soundwire_reg ori_reg = reg;

		if (sdw_arrange_payload(&payloads[0], trans_frames[i], &reg) != 0)
			goto resume;

		if (sdw_dfs_payloads_arrangement(payloads + 1, payloads_num - 1,
			trans_frames, trans_frames_num, reg) == 0)
			return 0;

		resume:
		*trans_frames[i] = ori_trans_frame;
		reg = ori_reg;
	}

	return -EFAULT;
}

static int sdw_create_transport_frame(unsigned char lane, unsigned char h_start, unsigned char h_stop,
	struct soundwire_transport_frame **out)
{
	struct soundwire_transport_frame *tmp =
		(struct soundwire_transport_frame *)sdw_kzalloc(sizeof(struct soundwire_transport_frame), GFP_KERNEL);
	if (tmp == NULL) {
		AUDIO_LOGE("out of memory");
		return -ENOMEM;
	}

	tmp->lane = lane;
	tmp->h_start = h_start;
	tmp->h_stop = h_stop;
	tmp->sample_interval = 0;
	tmp->transport_bit_slots = 0;
	tmp->free_blks_num = 0;

	*out = tmp;
	return 0;
}

static void sdw_release_transport_frame(struct soundwire_transport_frame *trans_frames[],
	int trans_frames_num)
{
	int i;

	for (i = 0; i < trans_frames_num; i++) {
		kfree(trans_frames[i]);
		trans_frames[i] = NULL;
	}
}

static int sdw_search_multi_lane_with_one_sub_frame(struct soundwire_payload *payloads,
	int payloads_num, unsigned char clk_div)
{
	int i;
	int result = 0;
	int trans_frame_num = 0;
	struct soundwire_reg reg = {0};
	struct soundwire_transport_frame *trans_frame[SDW_DATA_LANE_NUM_MAX] = {NULL};

	reg.clk_div = clk_div;
	for (i = 0; i < g_arrangement->data_lane_num; i++) {
		trans_frame_num++;
		if (sdw_create_transport_frame(i, 1, SDW_FRAME_COLS - 1, &trans_frame[i]) != 0)
			return -EFAULT;
	}

	result = sdw_dfs_payloads_arrangement(payloads, payloads_num, trans_frame, trans_frame_num, reg);
	sdw_release_transport_frame(trans_frame, trans_frame_num);

	return result;
}

static int sdw_search_one_lane_with_two_sub_frame(struct soundwire_payload *payloads,
	int payloads_num, unsigned char clk_div)
{
	int i;
	int result = 0;
	int trans_frame_num = 0;
	struct soundwire_reg reg = {0};
	struct soundwire_transport_frame *trans_frame[2] = {NULL};

	reg.clk_div = clk_div;
	trans_frame_num = 2;

	for (i = 1; i < (SDW_FRAME_COLS / 2); i++) {
		sdw_create_transport_frame(0, 1, i, &trans_frame[0]);
		sdw_create_transport_frame(0, i + 1, SDW_FRAME_COLS - 1, &trans_frame[1]);
		result = sdw_dfs_payloads_arrangement(payloads, payloads_num, trans_frame, trans_frame_num, reg);
		sdw_release_transport_frame(trans_frame, trans_frame_num);
		if (result == 0)
			break;
	}

	return result;
}

static int sdw_search_best_arrangement(struct soundwire_payload *payloads, int payloads_num)
{
	enum soundwire_clk_div clk_div;
	enum soundwire_clk_div required_clk_div = CLK_DIV_16;

	required_clk_div = sdw_calc_required_clk_div(payloads, payloads_num);
	if (required_clk_div == CLK_DIV_UNKNOWN) {
		goto search_fail;
	}

	for (clk_div = required_clk_div; clk_div > CLK_DIV_UNKNOWN; clk_div--) {
		if (sdw_search_multi_lane_with_one_sub_frame(payloads, payloads_num, clk_div) == 0)
			goto search_succ;
	}

	for (clk_div = required_clk_div; clk_div > CLK_DIV_UNKNOWN; clk_div--) {
		if (sdw_search_one_lane_with_two_sub_frame(payloads, payloads_num, clk_div) == 0)
			goto search_succ;
	}

search_fail:
	AUDIO_LOGE("arrange failed\n");
	return -EFAULT;

search_succ:
	AUDIO_LOGI("arrange succ, clk: %u", g_sdw_clk[clk_div]);
	sdw_print_enabled_dp_config(g_arrangement->curr_reg.dp, DP_NUM);
	sdw_commit_layout(payloads, payloads_num);

	return 0;
}


int soundwire_bandwidth_adp_init(struct soundwire_priv *priv)
{
	if (priv == NULL || priv->dev == NULL)
		return -EINVAL;

	if (g_arrangement != NULL) {
		AUDIO_LOGE("already inited");
		return 0;
	}

	g_arrangement = (struct soundwire_bandwidth_arrangement *)devm_kzalloc(
		priv->dev, sizeof(struct soundwire_bandwidth_arrangement), GFP_KERNEL);
	if (g_arrangement == NULL) {
		AUDIO_LOGE("not enough memory for bandwidth layout");
		return -ENOMEM;
	}

	g_arrangement->data_lane_num = sdw_get_lane_num(priv);
	g_arrangement->priv = priv;

	return 0;
}

int soundwire_bandwidth_adp_add(struct soundwire_payload *payloads, int payloads_num)
{
	int ret = 0;

	int total_payloads_num = 0;
	struct soundwire_payload *total_payloads = NULL;

	if (g_arrangement == NULL) {
		AUDIO_LOGE("not inited");
		return -EFAULT;
	}

	if (!sdw_check_payloads_valid(payloads, payloads_num)) {
		AUDIO_LOGE("invalid payloads");
		return -EFAULT;
	}

	if (!sdw_check_payloads_conflict(payloads, payloads_num)) {
		AUDIO_LOGE("conflict payloads");
		return -EFAULT;
	}

	if (sdw_merge_and_sort_payloads(payloads, payloads_num, &total_payloads, &total_payloads_num) != 0) {
		AUDIO_LOGE("merge and sort payloads failed");
		return -EFAULT;
	}

	sdw_remap_sample_rate(total_payloads, total_payloads_num);

	ret = sdw_search_best_arrangement(total_payloads, total_payloads_num);

	kfree(total_payloads);
	return ret;
}

int soundwire_bandwidth_adp_del(struct soundwire_payload *payloads, int payloads_num)
{
	int ret = 0;

	if (g_arrangement == NULL) {
		AUDIO_LOGE("not inited");
		return -EFAULT;
	}

	if (g_arrangement->payloads_num == 0) {
		AUDIO_LOGW("no payloads can be delete");
		return -EFAULT;
	}

	if (!sdw_check_payloads_valid(payloads, payloads_num)) {
		AUDIO_LOGE("invalid payloads");
		return -EFAULT;
	}

	intercore_try_lock(g_arrangement->priv);
	sdw_remove_payloads(payloads, payloads_num);
	intercore_unlock(g_arrangement->priv);

	if (g_arrangement->payloads_num != 0) {
		ret = sdw_search_best_arrangement(g_arrangement->payloads, g_arrangement->payloads_num);
	} else {
		/* switch bank */
		intercore_try_lock(g_arrangement->priv);
		write_clk_div_ctrl_reg(CLK_DIV_4);
		write_frame_ctrl_reg();
		intercore_unlock(g_arrangement->priv);
	}

	return ret;
}

void soundwire_bandwidth_adp_deinit()
{
	if (g_arrangement != NULL) {
		devm_kfree(g_arrangement->priv->dev, g_arrangement);
		g_arrangement = NULL;
	}
}

bool soundwire_bandwidth_adp_is_active(void)
{
	return g_arrangement != NULL && g_arrangement->payloads_num != 0;
}

void soundwire_bandwidth_recover(void)
{
	sdw_config_bandwidth();
}
