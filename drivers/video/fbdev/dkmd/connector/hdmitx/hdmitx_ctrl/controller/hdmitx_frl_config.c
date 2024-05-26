/*
 * Copyright (c) CompanyNameMagicTag. 2019-2020. All rights reserved.
 * Description: hdmi hal frl module source file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include <securec.h>
#include "dkmd_log.h"

#include "hdmitx_ctrl.h"
#include "hdmitx_frl_config.h"

#include "hdmitx_link_training_reg.h"
#include "hdmitx_frl_reg.h"

#define reg_ffe_n(ln)		(REG_SOURCE_FFE_UP0 + 0x4 * ((ln) / 2))
#define ffe_mask_n(ln)	   (0xf << (0x4 * ((ln) % 2)))
#define ffe_value_n(ln, val) (((val) & 0xf) << (0x4 * ((ln) % 2)))

#define reg_pattern_n(ln)		(REG_FRL_TRN_LTP0 + 0x4 * ((ln) / 2))
#define pattern_mask_n(ln)	   (0xf << (0x4 * ((ln) % 2)))
#define pattern_value_n(ln, val) (((val) & 0xf) << (0x4 * ((ln) % 2)))

struct frl_16to18_data {
	u32 ram_addr;
	u32 wr_data;
	u8 sel_7b8b_9b10b;
};

#define FRL_9B10B_TABLE_SIZE 512
#define FRL_7B8B_TABLE_SIZE  128
/* frl 9b10b coding table */
static const u32 g_table_9b_10b[FRL_9B10B_TABLE_SIZE] = {
	0x34b2d2, 0x8b1d32, 0x52ab52, 0xc18f92, 0x62a752, 0xa19792, 0x61a792, 0xe087d2,
	0x32b352, 0x919b92, 0x51ab92, 0xd08bd2, 0x31b392, 0xb093d2, 0x722372, 0xd10bb2,
	0x8d1cb2, 0x899d92, 0x49ad92, 0xc88dd2, 0x29b592, 0xa895d2, 0x64a6d2, 0xe87a10,
	0x19b992, 0x9899d2, 0x58a9d2, 0xd87610, 0x38b1d2, 0xb86e10, 0x785e10, 0xd20b72,
	0x8c9cd2, 0x859e92, 0x45ae92, 0xc40ef4, 0x25b692, 0xa416f4, 0x6426f4, 0xe47910,
	0x15ba92, 0x941af4, 0x542af4, 0xd47510, 0x3432f4, 0xb46d10, 0x745d10, 0xbfd00,
	0x2e3472, 0x8c1cf4, 0x4c2cf4, 0xcc7310, 0x2c34f4, 0xac6b10, 0x6c5b10, 0x13fb00,
	0x1c38f4, 0x9c6710, 0x5c5710, 0x23f700, 0x3c4f10, 0x43ef00, 0x83df00, 0x70a3d2,
	0x4e2c72, 0x961a72, 0x562a72, 0xc20f74, 0x363272, 0xa21774, 0x622774, 0xe27890,
	0x532b32, 0x921b74, 0x522b74, 0xd27490, 0x323374, 0xb26c90, 0x725c90, 0xdfc80,
	0xca0d72, 0x8a1d74, 0x4a2d74, 0xca7290, 0x2a3574, 0xaa6a90, 0x6a5a90, 0xea3a80,
	0x1a3974, 0x9a6690, 0x5a5690, 0xda3680, 0x3a4e90, 0xba2e80, 0x7a1e80, 0x6a2572,
	0xc60e72, 0x861e74, 0x462e74, 0xc67190, 0x263674, 0xa66990, 0x665990, 0xe63980,
	0x163a74, 0x966590, 0x565590, 0xd63580, 0x364d90, 0xb62d80, 0x761d80, 0xa61672,
	0xe3c74,  0x8e6390, 0x4e5390, 0xce3380, 0x2e4b90, 0xae2b80, 0x6e1b80, 0x11fb82,
	0x1e4790, 0x9e2780, 0x5e1780, 0x21f782, 0x3e0f80, 0x41ef82, 0x81df82, 0x9a1972,
	0x4d2cb2, 0x871e32, 0x472e32, 0xc10fb4, 0x273632, 0xa117b4, 0x6127b4, 0xe17850,
	0x173a32, 0x911bb4, 0x512bb4, 0xd17450, 0x3133b4, 0xb16c50, 0x715c50, 0xefc40,
	0xc90db2, 0x891db4, 0x492db4, 0xc97250, 0x2935b4, 0xa96a50, 0x695a50, 0xe93a40,
	0x1939b4, 0x996650, 0x595650, 0xd93640, 0x394e50, 0xb92e40, 0x791e40, 0x6925b2,
	0xc50eb2, 0x851eb4, 0x452eb4, 0xc57150, 0x2536b4, 0xa56950, 0x655950, 0xe53940,
	0x153ab4, 0x956550, 0x555550, 0xd53540, 0x354d50, 0xb52d40, 0x751d40, 0xa516b2,
	0xd3cb4,  0x8d6350, 0x4d5350, 0xcd3340, 0x2d4b50, 0xad2b40, 0x6d1b40, 0x12fb42,
	0x1d4750, 0x9d2740, 0x5d1740, 0x22f742, 0x3d0f40, 0x42ef42, 0x82df42, 0x9919b2,
	0xc30f32, 0x831f34, 0x432f34, 0xc370d0, 0x233734, 0xa368d0, 0x6358d0, 0xe338c0,
	0x133b34, 0x9364d0, 0x5354d0, 0xd334c0, 0x334cd0, 0xb32cc0, 0x731cc0, 0xa31732,
	0xb3d34,  0x8b62d0, 0x4b52d0, 0xcb32c0, 0x2b4ad0, 0xab2ac0, 0x6b1ac0, 0x14fac2,
	0x1b46d0, 0x9b26c0, 0x5b16c0, 0x24f6c2, 0x3b0ec0, 0x44eec2, 0x84dec2, 0x931b32,
	0x2b3532, 0x8761d0, 0x4751d0, 0xc731c0, 0x2749d0, 0xa729c0, 0x6719c0, 0x18f9c2,
	0x1745d0, 0x9725c0, 0x5715c0, 0x28f5c2, 0x370dc0, 0x48edc2, 0x88ddc2, 0x333332,
	0x1cb8d2, 0x8f23c0, 0x4f13c0, 0x30f3c2, 0x2f0bc0, 0x50ebc2, 0x90dbc2, 0x10fbc4,
	0x1f07c0, 0x60e7c2, 0xa0d7c2, 0x20f7c4, 0xc0cfc2, 0x40efc4, 0x80dfc4, 0xa915b2,
	0x2ab552, 0x869e52, 0x46ae52, 0xc08fd4, 0x26b652, 0xa097d4, 0x60a7d4, 0xe0f830,
	0x16ba52, 0x909bd4, 0x50abd4, 0xd0f430, 0x30b3d4, 0xb0ec30, 0x70dc30, 0xf7c20,
	0x8a9d52, 0x889dd4, 0x48add4, 0xc8f230, 0x28b5d4, 0xa8ea30, 0x68da30, 0xe8ba20,
	0x18b9d4, 0x98e630, 0x58d630, 0xd8b620, 0x38ce30, 0xb8ae20, 0x789e20, 0x68a5d2,
	0xc48ed2, 0x849ed4, 0x44aed4, 0xc4f130, 0x24b6d4, 0xa4e930, 0x64d930, 0xe4b920,
	0x14bad4, 0x94e530, 0x54d530, 0xd4b520, 0x34cd30, 0xb4ad20, 0x749d20, 0xa496d2,
	0xcbcd4,  0x8ce330, 0x4cd330, 0xccb320, 0x2ccb30, 0xacab20, 0x6c9b20, 0x137b22,
	0x1cc730, 0x9ca720, 0x5c9720, 0x237722, 0x3c8f20, 0x436f22, 0x835f22, 0x949ad2,
	0xc28f52, 0x829f54, 0x42af54, 0xc2f0b0, 0x22b754, 0xa2e8b0, 0x62d8b0, 0xe2b8a0,
	0x12bb54, 0x92e4b0, 0x52d4b0, 0xd2b4a0, 0x32ccb0, 0xb2aca0, 0x729ca0, 0xe20772,
	0xabd54,  0x8ae2b0, 0x4ad2b0, 0xcab2a0, 0x2acab0, 0xaaaaa0, 0x6a9aa0, 0x157aa2,
	0x1ac6b0, 0x9aa6a0, 0x5a96a0, 0x2576a2, 0x3a8ea0, 0x456ea2, 0x855ea2, 0x929b52,
	0xa29752, 0x86e1b0, 0x46d1b0, 0xc6b1a0, 0x26c9b0, 0xa6a9a0, 0x6699a0, 0x1979a2,
	0x16c5b0, 0x96a5a0, 0x5695a0, 0x2975a2, 0x368da0, 0x496da2, 0x895da2, 0x97da4,
	0x1e3872, 0x8ea3a0, 0x4e93a0, 0x3173a2, 0x2e8ba0, 0x516ba2, 0x915ba2, 0x117ba4,
	0x1e87a0, 0x6167a2, 0xa157a2, 0x2177a4, 0xc14fa2, 0x416fa4, 0x815fa4, 0xaa1572,
	0x54aad2, 0x819f94, 0x41af94, 0xc1f070, 0x21b794, 0xa1e870, 0x61d870, 0xe1b860,
	0x11bb94, 0x91e470, 0x51d470, 0xd1b460, 0x31cc70, 0xb1ac60, 0x719c60, 0xe107b2,
	0x9bd94,  0x89e270, 0x49d270, 0xc9b260, 0x29ca70, 0xa9aa60, 0x699a60, 0x167a62,
	0x19c670, 0x99a660, 0x599660, 0x267662, 0x398e60, 0x466e62, 0x865e62, 0x3931b2,
	0x951ab2, 0x85e170, 0x45d170, 0xc5b160, 0x25c970, 0xa5a960, 0x659960, 0x1a7962,
	0x15c570, 0x95a560, 0x559560, 0x2a7562, 0x358d60, 0x4a6d62, 0x8a5d62, 0xa7d64,
	0x1d38b2, 0x8da360, 0x4d9360, 0x327362, 0x2d8b60, 0x526b62, 0x925b62, 0x127b64,
	0x1d8760, 0x626762, 0xa25762, 0x227764, 0xc24f62, 0x426f64, 0x825f64, 0x7123b2,
	0x1ab952, 0x839f12, 0x43af12, 0xc3b0e0, 0x23b712, 0xa3a8e0, 0x6398e0, 0x1c78e2,
	0x13bb12, 0x93a4e0, 0x5394e0, 0x2c74e2, 0x338ce0, 0x4c6ce2, 0x8c5ce2, 0xc7ce4,
	0x1b3932, 0x8ba2e0, 0x4b92e0, 0x3472e2, 0x2b8ae0, 0x546ae2, 0x945ae2, 0x147ae4,
	0x1b86e0, 0x6466e2, 0xa456e2, 0x2476e4, 0xc44ee2, 0x446ee4, 0x845ee4, 0xb21372,
	0x8e1c72, 0x87a1e0, 0x4791e0, 0x3871e2, 0x2789e0, 0x5869e2, 0x9859e2, 0x1879e4,
	0x1785e0, 0x6865e2, 0xa855e2, 0x2875e4, 0xc84de2, 0x486de4, 0x885de4, 0x3532b2,
	0xfbc10,  0x7063e2, 0xb053e2, 0x3073e4, 0xd04be2, 0x506be4, 0x905be4, 0x4aad52,
	0xe047e2, 0x6067e4, 0xa057e4, 0x4cacd2, 0x3a3172, 0x2cb4d2, 0x2d34b2, 0x5a2972,
};

/* frl 7b8b coding table */
static const u32 g_table_7b_8b[FRL_7B8B_TABLE_SIZE] = {
	0x946b2, 0x8c732, 0x4cb32, 0xc8372, 0x2cd32, 0xa8572, 0x68972, 0xe1e10,
	0x926d2, 0x98672, 0x58a72, 0xd1d10, 0x38c72, 0xb1b10, 0x71710, 0xff00,
	0x8a752, 0x88774, 0x48b74, 0xc9c90, 0x28d74, 0xa9a90, 0x69690, 0xe8e80,
	0x18e74, 0x99990, 0x59590, 0xd8d80, 0x39390, 0xb8b80, 0x78780, 0x16e92,
	0x54ab2, 0x847b4, 0x44bb4, 0xc5c50, 0x24db4, 0xa5a50, 0x65650, 0xe4e40,
	0x14eb4, 0x95950, 0x55550, 0xd4d40, 0x35350, 0xb4b40, 0x74740, 0x1ae52,
	0xc43b2, 0x8d8d0, 0x4d4d0, 0xcccc0, 0x2d2d0, 0xacac0, 0x6c6c0, 0x13ec2,
	0x1d1d0, 0x9c9c0, 0x5c5c0, 0x23dc2, 0x3c3c0, 0x43bc2, 0x837c2, 0x34cb2,
	0x52ad2, 0x827d4, 0x42bd4, 0xc3c30, 0x22dd4, 0xa3a30, 0x63630, 0xe2e20,
	0x12ed4, 0x93930, 0x53530, 0xd2d20, 0x33330, 0xb2b20, 0x72720, 0x1ce32,
	0xc23d2, 0x8b8b0, 0x4b4b0, 0xcaca0, 0x2b2b0, 0xaaaa0, 0x6a6a0, 0x15ea2,
	0x1b1b0, 0x9a9a0, 0x5a5a0, 0x25da2, 0x3a3a0, 0x45ba2, 0x857a2, 0x629d2,
	0x26d92, 0x87870, 0x47470, 0xc6c60, 0x27270, 0xa6a60, 0x66660, 0x19e62,
	0x17170, 0x96960, 0x56560, 0x29d62, 0x36360, 0x49b62, 0x89762, 0x32cd2,
	0x4ab52, 0x8e8e0, 0x4e4e0, 0x31ce2, 0x2e2e0, 0x51ae2, 0x916e2, 0x2ad52,
	0x1e1e0, 0x619e2, 0xa15e2, 0x86792, 0xc13e2, 0xa45b2, 0x649b2, 0xa25d2,
};

static void frl_hw_set_pfifo_threshold(const struct frl_reg *frl, u32 up, u32 down)
{
	hdmi_clrset(frl->frl_base, REG_PFIFO_LINE_THRESHOLD,
				REG_PFIFO_UP_THRESHOLD_M, reg_pfifo_up_threshold(up));
	hdmi_clrset(frl->frl_base, REG_PFIFO_LINE_THRESHOLD,
				REG_PFIFO_DOWN_THRESHOLD_M, reg_pfifo_down_threshold(down));
}

static void frl_hw_set_training_pattern(const struct frl_reg *frl, u8 lnx, u8 pattern)
{
	u32 offset, mask, value;
	u64 temp;

	temp = reg_pattern_n(lnx);
	offset = (u32)temp;
	temp = pattern_mask_n(lnx);
	mask = (u32)temp;
	temp = pattern_value_n(lnx, pattern);
	value = (u32)temp;
	hdmi_clrset(frl->frl_training_base, offset, mask, value);
}

static void frl_hw_set_training_ffe(const struct frl_reg *frl, u8 lnx, u8 level)
{
	u32 offset, mask, value;
	u64 temp;

	temp = reg_ffe_n(lnx);
	offset = (u32)temp;
	temp = ffe_mask_n(lnx);
	mask = (u32)temp;
	temp = ffe_value_n(lnx, level);
	value = (u32)temp;
	hdmi_clrset(frl->frl_training_base, offset, mask, value);
}

static void frl_hw_set_training_rate(const struct frl_reg *frl, u32 rate)
{
	hdmi_clrset(frl->frl_training_base, REG_FRL_TRN_RATE, REG_FRL_TRN_RATE_M, reg_frl_trn_rate_f(rate));
}

static void frl_hw_config_data_source(const struct frl_reg *frl, enum frl_data_source source)
{
	u32 work_en = 0;
	u32 chan_sel = 0;

	switch (source) {
	case FRL_GAP_PACKET:
		work_en = 0;
		chan_sel = 1;
		break;
	case FRL_VIDEO:
		work_en = 1;
		chan_sel = 1;
		break;
	case FRL_TRAINING_PATTERN:
		chan_sel = 0;
		work_en = 0;
		break;
	default:
		break;
	}

	/**
	 * Maybe it need to fixed depends on the hw behavior, in some cases,
	 * the two register should be config in reverse order.
	 */
	hdmi_clrset(frl->frl_training_base, REG_FRL_CHAN_SEL, REG_FRL_CHAN_SEL_M,
				reg_frl_chan_sel_f(chan_sel));
	hdmi_clrset(frl->frl_base, REG_WORK_EN, REG_WORK_EN_M, reg_work_en_f(work_en));
}

static void frl_hw_init(const struct frl_reg *frl)
{
	/* select CPU frl training mode */
	hdmi_clrset(frl->frl_training_base, REG_FRL_TRN_MODE, REG_FRL_TRN_MODE_M, reg_frl_trn_mode_f(0));
}

static void frl_hw_enable_ram(const struct frl_reg *frl, bool enable)
{
	hdmi_clrset(frl->frl_base, REG_LM_SRC_EN, REG_RAM_CONFIG_EN_M, reg_ram_config_en((u8)enable));
}

static void frl_hw_write_16to18_data(const struct frl_reg *frl, const struct frl_16to18_data *data)
{
	u32 value = 0;

	hdmi_writel(frl->frl_base, REG_LM_COMMAND_EN, reg_command_en(0));
	udelay(1);

	/* step1: reg 0x320c, coding table write data */
	hdmi_writel(frl->frl_base, REG_LM_IN_AC0_WDATA, data->wr_data);
	udelay(1);

	/*
	 * step2: reg 0x3200,  bit[31] = 1,9b RAM; bit[31] = 0,7B RAM;
	 * bit[24:16] = RAM addr;bit[15:8] = channel number,read used;bit[7:4]=0xa
	 */
	value |= reg_command(0);
	value |= reg_in_ac0_number(0);
	value |= reg_protect_number(0xa);
	value |= reg_in_ac0_addr((data->sel_7b8b_9b10b << 15) | data->ram_addr); /* shift left 15 */
	hdmi_writel(frl->frl_base, REG_LM_IN_AC0_CMD, value);
	udelay(1);

	/* step3: reg 0x321c=1 enable command */
	hdmi_writel(frl->frl_base, REG_LM_COMMAND_EN, reg_command_en(1));
	udelay(1);

	/* step4: reg 0x321c=0 clear command */
	hdmi_writel(frl->frl_base, REG_LM_COMMAND_EN, reg_command_en(0));
	hdmi_clrset(frl->frl_base, REG_LM_IN_AC0_CMD, REG_PROTECT_NUMBER_M,
				reg_protect_number(0x5));
	udelay(1);
}

void hal_frl_init(const struct frl_reg *frl)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	frl_hw_init(frl);
	frl_hw_set_pfifo_threshold(frl, 0x2ee, 0x2ee); /* set default pfifo up&down threshold is 0x2ee. */
}

s32 hal_frl_enter(struct frl_reg *frl)
{
	return 0;
}

void hal_frl_config_16to18_table(const struct frl_reg *frl)
{
	u32 i;
	struct frl_16to18_data config_data;

	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	/* step1:0x3210=0x1 */
	frl_hw_enable_ram(frl, true);

	/* step2:7B8B coding table write operation */
	config_data.sel_7b8b_9b10b = 0;
	for (i = 0; i < ARRAY_SIZE(g_table_7b_8b); i++) {
		config_data.ram_addr = i;
		config_data.wr_data = g_table_7b_8b[i];
		frl_hw_write_16to18_data(frl, &config_data);
	}

	/* step3:9B10B coding table write operation */
	config_data.sel_7b8b_9b10b = 1;
	for (i = 0; i < ARRAY_SIZE(g_table_9b_10b); i++) {
		config_data.ram_addr = i;
		config_data.wr_data = g_table_9b_10b[i];
		frl_hw_write_16to18_data(frl, &config_data);
	}

	/*
	 * step4:when Soure write operation is done,
	 * Source clear operation enable register 0x3210
	 */
	udelay(1);
	frl_hw_enable_ram(frl, false);
}

void hal_frl_set_training_pattern(const struct frl_reg *frl, u8 lnx, u8 pattern)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	frl_hw_set_training_pattern(frl, lnx, pattern);
}

void hal_frl_set_training_ffe(const struct frl_reg *frl, u8 lnx, u8 ffe)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	frl_hw_set_training_ffe(frl, lnx, ffe);
}

u8 hal_frl_get_training_ffe(const struct frl_reg *frl, u8 lnx)
{
	u8 ffe;

	if (frl == NULL || frl->frl_training_base == NULL) {
		dpu_pr_err("null pointer.\n");
		return 0;
	}

	if (lnx == 0) { /* lane 0 */
		ffe = hdmi_read_bits(frl->frl_training_base, REG_SOURCE_FFE_UP0, REG_SOURCE_LANE0_FFE_UP_M);
	} else if (lnx == 1) { /* lane 1 */
		ffe = hdmi_read_bits(frl->frl_training_base, REG_SOURCE_FFE_UP0, REG_SOURCE_LANE1_FFE_UP_M);
	} else if (lnx == 2) { /* lane 2 */
		ffe = hdmi_read_bits(frl->frl_training_base, REG_SOURCE_FFE_UP1, REG_SOURCE_LANE2_FFE_UP_M);
	} else if (lnx == 3) { /* lane 3 */
		ffe = hdmi_read_bits(frl->frl_training_base, REG_SOURCE_FFE_UP1, REG_SOURCE_LANE3_FFE_UP_M);
	} else {
		ffe = 0;
		dpu_pr_err("unknown lnx=%d.\n", lnx);
	}

	return ffe;
}

void hal_frl_set_training_rate(const struct frl_reg *frl, u8 rate)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	frl_hw_set_training_rate(frl, rate);
}

u8 hal_frl_get_training_rate(const struct frl_reg *frl)
{
	u8 rate;

	if (frl == NULL || frl->frl_training_base == NULL) {
		dpu_pr_err("null pointer.\n");
		return 0;
	}

	rate = hdmi_read_bits(frl->frl_training_base, REG_FRL_TRN_RATE, REG_FRL_TRN_RATE_M);

	return rate;
}

void hal_frl_set_source_data(const struct frl_reg *frl, enum frl_data_source source)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	frl_hw_config_data_source(frl, source);
}

enum frl_data_source hal_frl_get_source_data(const struct frl_reg *frl)
{
	u32 work_en;
	u32 chan_sel;
	enum frl_data_source source;

	if (frl == NULL || frl->frl_training_base == NULL || frl->frl_base == NULL) {
		dpu_pr_err("null pointer.\n");
		return FRL_TRAINING_PATTERN;
	}

	chan_sel = hdmi_read_bits(frl->frl_training_base, REG_FRL_CHAN_SEL, REG_FRL_CHAN_SEL_M);
	work_en = hdmi_read_bits(frl->frl_base, REG_WORK_EN, REG_WORK_EN_M);
	if ((work_en != 0) && (chan_sel != 0)) {
		source = FRL_VIDEO;
	} else if ((work_en == 0) && (chan_sel != 0)) {
		source = FRL_GAP_PACKET;
	} else {
		source = FRL_TRAINING_PATTERN;
	}

	return source;
}

void hal_frl_set_pfifo_threshold(const struct frl_reg *frl, u32 up, u32 down)
{
	if (frl == NULL) {
		dpu_pr_err("null pointer.\n");
		return;
	}

	frl_hw_set_pfifo_threshold(frl, up, down);
}

void hal_frl_deinit(struct frl_reg *frl)
{
}
