#include <linux/types.h>
#include "hisi_dpp_hiace_test.h"
#include "hisi_disp_config.h"
#include "hisi_disp_gadgets.h"
#include "hisi_disp_dpp.h"
#include "hisi_operator_tool.h"

static uint32_t hiace_lut_read[36][32] = {0};
static uint32_t g_sel_gamma_ab_shadow_hdr_lut;
#define LOG_LUM_SIZE 32
#define LUMA_GAMMA_SIZE 21
#define DPE_DETAIL_WEIGHT (0x77500)
#define LOG_LUM (0x77600)
#define LUMA_GAMMA (0x77700)
#define DETAIL_WEIGHT_TABLE_UPDATED (1 << 0)
#define LOGLUM_EOTF_TABLE_UPDATED (1 << 1)
#define LUMA_GAMA_TABLE_UPDATED (1 << 2)

uint32_t g_histogram[CE_SIZE_HIST];

static uint32_t orginal_detail_weight[HIACE_DETAIL_WEIGHT_TABLE_LEN] = {
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80
};

static int orginal_loglum_eotf[HIACE_LOGLUM_EOTF_TABLE_LEN] = {
	-4096, -4096, -4096, -4096, -4096, -3942, -3719, -3518,
	-3347, -3201, -3074, -2963, -2863, -2546, -2309, -2118,
	-1958, -1819, -1695, -1585, -1484, -1391, -1078, -826,
	-613, -426, -259, 32, 288, 512, 718, 910,
	1089, 1259, 1421, 1576, 1726, 1872, 2014, 2153,
	2289, 2424, 2556, 2687, 2817, 2947, 3075, 3204,
	3332, 3461, 3590, 3720, 3851, 3983, 4050, 4083,
	4087, 4091, 4095, 4095, 4095, 4095, 4095
};

static int orginal_luma_eotf_gama[HIACE_LUMA_GAMA_TABLE_LEN] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 45, 65, 83, 99, 116, 133,
	151, 169, 188, 208, 229, 252, 275, 300,
	326, 354, 384, 415, 448, 483, 520, 560,
	602, 646, 693, 743, 796, 853, 913, 976,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023
};

static const uint32_t user_detail_weight[33] = {
	0, 0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160,
	176, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192
};

static const int user_loglum_eotf[63] = {
	-4096, -4096, -4096, -4096, -4096, -3942, -3719, -3518, -3347, -3201, -3074,
	-2963, -2863, -2546, -2309, -2118, -1958, -1819, -1695, -1585, -1484, -1391,
	-1078, -826, -613, -426, -259, 32, 288, 512, 718, 910, 1089,
	1259, 1421, 1576,
	1726, 1872, 2014, 2153, 2289, 2424, 2556, 2687, 2817, 2947, 3075,
	3204, 3332,
	3461, 3590, 3720, 3851, 3983, 4050, 4083, 4087, 4091, 4095, 4095,
	4095, 4095,
	4095
};

static const int user_luma_eotf_gama[63] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44,
	63, 81, 97, 114, 131, 148, 166, 185, 205, 226, 248,
	272, 296, 323, 350, 380, 411, 444, 479, 517, 556, 598,
	643, 690, 741, 794, 851,
	912, 976, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
	1023, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
	1023
};
static const uint32_t hiace_lut1[36][32] = {
#include "hiace_test_parameters/Europe_24fps_1920x1080_out_721_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut2[36][32] = {
#include "hiace_test_parameters/Europe_24fps_1920x1080_out_1294_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut3[36][32] = {
#include "hiace_test_parameters/Europe_24fps_1920x1080_out_1608_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut4[36][32] = {
#include "hiace_test_parameters/Europe_35_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut5[36][32] = {
#include "hiace_test_parameters/Exodus_UHD_HDR_Exodus_draft_12_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut6[36][32] = {
#include "hiace_test_parameters/Exodus_UHD_HDR_Exodus_draft_24fps_1920x1080_out_457_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut7[36][32] = {
#include "hiace_test_parameters/Exodus_UHD_HDR_Exodus_draft_24fps_1920x1080_out_901_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut8[36][32] = {
#include "hiace_test_parameters/HDR_liangchaoduibi_1067_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut9[36][32] = {
#include "hiace_test_parameters/HDR_niguangyingxiang_62_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut10[36][32] = {
#include "hiace_test_parameters/HDR_qupingduibi_60fps_1920x1080_out_1038_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut11[36][32] = {
#include "hiace_test_parameters/liangzidianDemo_60fps_5107_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut12[36][32] = {
#include "hiace_test_parameters/liangzidianDemo_60fps_5761_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut13[36][32] = {
#include "hiace_test_parameters/Life_of_Pi_draft_Ultra_HD_HDR_11_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut14[36][32] = {
#include "hiace_test_parameters/Life_of_Pi_draft_Ultra_HD_HDR_222_1BIT_input_lut.txt"
};

static const uint32_t hiace_lut15[36][32] = {
#include "hiace_test_parameters/Life_of_Pi_draft_Ultra_HD_HDR_857_1BIT_input_lut.txt"
};

static const uint32_t sdr_lut1[36][32] = {
#include "hiace_test_parameters/black_dress_girl26_input_lut.txt"
};

static const uint32_t sdr_lut2[36][32] = {
#include "hiace_test_parameters/Black_tea_board_input_lut.txt"
};

static const uint32_t sdr_lut3[36][32] = {
#include "hiace_test_parameters/Camera1001_input_lut.txt"
};

static const uint32_t sdr_lut4[36][32] = {
#include "hiace_test_parameters/constrast_original_016_input_lut.txt"
};

static const uint32_t sdr_lut5[36][32] = {
#include "hiace_test_parameters/constrast_original_028_input_lut.txt"
};

static const uint32_t sdr_lut6[36][32] = {
#include "hiace_test_parameters/Lobster_input_lut.txt"
};

static const uint32_t sdr_lut7[36][32] = {
#include "hiace_test_parameters/LovelyGirl_1_input_lut.txt"
};

static const uint32_t sdr_lut8[36][32] = {
#include "hiace_test_parameters/LovelyGirl_5_input_lut.txt"
};

static const uint32_t sdr_lut9[36][32] = {
#include "hiace_test_parameters/Play_piano_1920x1080_input_lut.txt"
};

static const uint32_t sdr_lut10[36][32] = {
#include "hiace_test_parameters/still_life_0855_input_lut.txt"
};

static const uint32_t sdr_lut11[36][32] = {
#include "hiace_test_parameters/Table_food_1_input_lut.txt"
};

static const uint32_t sdr_lut12[36][32] = {
#include "hiace_test_parameters/White_Clothes_girl_1_input_lut.txt"
};
static const uint32_t sdr_lut13[36][32] = {
#include "hiace_test_parameters/white_shirt_290_input_lut.txt"
};

static const uint32_t folding_screen_lut1[36][32] = {
#include "hiace_test_parameters/folding_screen_test1_input_lut.txt"
};

static const uint32_t folding_screen_lut2[36][32] = {
#include "hiace_test_parameters/folding_screen_test2_input_lut.txt"
};

static const uint32_t local_highlight_hdr_lut1[36][32] = {
#include "hiace_test_parameters/local_highlight_1920x1080_test1_input_lut.txt"
};

static const uint32_t local_highlight_hdr_lut2[36][32] = {
#include "hiace_test_parameters/local_highlight_1920x1080_test2_input_lut.txt"
};

static const uint32_t local_highlight_sdr_lut1[36][32] = {
#include "hiace_test_parameters/local_highlight_sdr_test1_input_lut.txt"
};

static const uint32_t local_refresh_lut1[36][32] = {
#include "hiace_test_parameters/local_refresh_test001_input_lut.txt"
};

static const uint32_t lre_sat_lut1[36][32] = {
#include "hiace_test_parameters/lre_sat_test1_input_lut.txt"
};

static const uint32_t lre_sat_lut2[36][32] = {
#include "hiace_test_parameters/lre_sat_test2_input_lut.txt"
};

static const uint32_t skin_count_lut2[36][32] = {
#include "hiace_test_parameters/skin_count_test1_input_lut.txt"
};
unsigned hisi_fb_hiace_lut = 1;
module_param_named(debug_hiace_lut, hisi_fb_hiace_lut, int, 0644);
MODULE_PARM_DESC(debug_hiace_lut, "hisi_fb_hiace_lut");

unsigned hisi_fb_hiace_lut_times = 1;
module_param_named(debug_hiace_lut_times, hisi_fb_hiace_lut_times, int, 0644);
MODULE_PARM_DESC(debug_hiace_lut_times, "hisi_fb_hiace_lut_times");

unsigned hisi_fb_read_hiace_lut = 30;
module_param_named(debug_read_hiace_lut, hisi_fb_read_hiace_lut, int, 0644);
MODULE_PARM_DESC(debug_read_hiace_lut, "hisi_fb_read_hiace_lut");

unsigned hisi_fb_hiace_histogram_protect = 1;
module_param_named(debug_hiace_histogram_protect,
hisi_fb_hiace_histogram_protect, int, 0644);
MODULE_PARM_DESC(debug_hiace_histogram_protect,
"hisi_fb_hiace_histogram_protect");

unsigned hisi_fb_read_histogram = 1;
module_param_named(debug_read_histogram, hisi_fb_read_histogram, int, 0644);
MODULE_PARM_DESC(debug_read_histogram, "hisi_fb_read_histogram");

unsigned hisi_fb_read_histogram_times = 0;
module_param_named(debug_read_histogram_times, hisi_fb_read_histogram_times,
int, 0644);
MODULE_PARM_DESC(debug_read_histogram_times, "hisi_fb_read_histogram_times");

unsigned hisi_fb_hiace_hdr_lut = 0;
module_param_named(debug_hiace_hdr_lut, hisi_fb_hiace_hdr_lut, int, 0644);
MODULE_PARM_DESC(debug_hiace_hdr_lut, "hisi_fb_hiace_hdr_lut");

unsigned hisi_fb_hiace_nr_flg = 1;
module_param_named(debug_hisi_fb_hiace_nr_flg, hisi_fb_hiace_nr_flg, int, 0644);
MODULE_PARM_DESC(debug_hisi_fb_hiace_nr_flg, "hisi_fb_hiace_nr_flg");

unsigned hisi_fb_hiace_hdr_lut_times = 0;
module_param_named(debug_hiace_hdr_lut_times, hisi_fb_hiace_hdr_lut_times, int,
0644);
MODULE_PARM_DESC(debug_hiace_hdr_lut_times, "hisi_fb_hiace_hdr_lut_times");

unsigned hisi_fb_hiace_lut_cmdlist = 0;
module_param_named(debug_hiace_lut_cmdlist, hisi_fb_hiace_lut_cmdlist, int,
0644);
MODULE_PARM_DESC(debug_hiace_lut_cmdlist, "hisi_fb_hiace_lut_cmdlist");

static void set_loglum_eotf(char __iomem *dpu_base, const int luts_value[])
{
	uint32_t i = 0;
	uint32_t temp = 0;
			for (i = 0; i < (LOG_LUM_EOTF_LUT_SIZE - 1); i++) {
				temp = ((luts_value[i * 2]) |
								(luts_value[i * 2 + 1] << 16));
				outp32(dpu_base + DPP_CH0_HI_ACE_LOG_LUM + i * 4, temp);
			}
			outp32(dpu_base + DPP_CH0_HI_ACE_LOG_LUM + i * 4,
						 (luts_value[LOG_LUM_EOTF_LUT_SIZE - 1]));
}

static void set_detail_weight_lut(char __iomem *dpu_base, const int luts_value[])
{
	uint32_t i = 0;
	uint32_t temp = 0;
			for (i = 0; i < (DETAIL_WEIGHT_SIZE - 1); i++) {
				temp = ((luts_value[i * 4]) |
								(luts_value[i * 4 + 1] << 8) |
								(luts_value[i * 4 + 2] << 16) |
								(luts_value[i * 4 + 3] << 24));
				outp32(dpu_base + DPP_CH0_HI_ACE_DETAIL_WEIGHT_OFFSET + i * 4, temp);
			}
			outp32(dpu_base + DPP_CH0_HI_ACE_DETAIL_WEIGHT_OFFSET + i * 4,
						 (luts_value[HIACE_DETAIL_WEIGHT_TABLE_LEN - 1]));
}

static void set_luma_gama(char __iomem *dpu_base, const int luts_value[])
{
	uint32_t i = 0;
	uint32_t temp = 0;
	for (i = 0; i < LUMA_GAMA_LUT_SIZE; i++) {
		temp = (luts_value[i * 3]) |
				(luts_value[i * 3 + 1] << 10) |
				(luts_value[i * 3 + 2] << 20);
		outp32(dpu_base + DPP_CH0_HI_ACE_LUMA_GAMMA + i * 4, temp);
	}
}

static void dpp_hiace_hdr_test(char __iomem *dpu_base)
{
	char __iomem *hiace_base = NULL;
	uint32_t gamma_ab_shadow = 0;
	uint32_t gamma_ab_work = 0;
	uint32_t temp = 0;
	uint32_t i = 0;
	uint32_t sel_gamma_ab_shadow = 0;
	uint32_t g_table_update = 0;
	hiace_base = dpu_base + HI_ACE_OFFSET;
	sel_gamma_ab_shadow = inp32(hiace_base + HI_ACE_GAMMA_AB_SHADOW_OFFSET) & 0xe;
	disp_pr_info("hiace_base %p, hisi_fb_hiace_hdr_lut_times %d, "
							 "hisi_fb_hiace_hdr_lut	%x",
							 hiace_base, hisi_fb_hiace_hdr_lut_times, hisi_fb_hiace_hdr_lut);

	if (0 == hisi_fb_hiace_hdr_lut_times)
		return;

	gamma_ab_shadow = inp32(hiace_base + HI_ACE_GAMMA_AB_SHADOW_OFFSET) & 0x8;
	gamma_ab_work = inp32(hiace_base + HI_ACE_GAMMA_AB_WORK_OFFSET) & 0x8;
	disp_pr_info("[hiace] test 000 set hiace lut! "
		"gamma_ab_shadow=0x%x,gamma_ab_work=0x%x \n",
		gamma_ab_shadow, gamma_ab_work);

	if (gamma_ab_shadow == gamma_ab_work) {
		if (hisi_fb_hiace_hdr_lut == 0)
			set_detail_weight_lut(dpu_base, orginal_detail_weight);
		else
			set_detail_weight_lut(dpu_base, user_detail_weight);

		g_table_update &= ~DETAIL_WEIGHT_TABLE_UPDATED;
		sel_gamma_ab_shadow ^= 0x8;
	}

	gamma_ab_shadow = inp32(hiace_base + HI_ACE_GAMMA_AB_SHADOW_OFFSET) & 0x4;
	gamma_ab_work = inp32(hiace_base + HI_ACE_GAMMA_AB_WORK_OFFSET) & 0x4;
	disp_pr_info("[hiace] test 001 set hiace lut! "
		"gamma_ab_shadow=0x%x,gamma_ab_work=0x%x \n",
		gamma_ab_shadow, gamma_ab_work);
	if (gamma_ab_shadow == gamma_ab_work) {
		if (hisi_fb_hiace_hdr_lut == 0)
			set_loglum_eotf(dpu_base, orginal_loglum_eotf);
		else
			set_loglum_eotf(dpu_base, user_loglum_eotf);
		g_table_update &= ~LOGLUM_EOTF_TABLE_UPDATED;
		sel_gamma_ab_shadow ^= 0x4;
	}

		gamma_ab_shadow = inp32(hiace_base + HI_ACE_GAMMA_AB_SHADOW_OFFSET) & 0x2;
		gamma_ab_work = inp32(hiace_base + HI_ACE_GAMMA_AB_WORK_OFFSET) & 0x2;
		disp_pr_info("[hiace] test 002 set hiace lut! "
			"gamma_ab_shadow=0x%x,gamma_ab_work=0x%x \n",
			gamma_ab_shadow, gamma_ab_work);
	if (gamma_ab_shadow == gamma_ab_work) {
		if (hisi_fb_hiace_hdr_lut == 0)
			set_luma_gama(dpu_base, orginal_luma_eotf_gama);
		else
			set_luma_gama(dpu_base, user_luma_eotf_gama);
		g_table_update &= ~LUMA_GAMA_TABLE_UPDATED;
		sel_gamma_ab_shadow ^= 0x2;
	}

	hisi_fb_hiace_hdr_lut_times--;
	g_sel_gamma_ab_shadow_hdr_lut = sel_gamma_ab_shadow;
}

static void set_luts(char __iomem *hiace_base, const uint32_t luts_value[36][32])
{
	int xpartition = 6;
	int j = 0, k = 0, i = 0;
	uint32_t lut_value = 0;
	for (i = 0; i < (6 * xpartition * 8); i++) {
		j = i % 8;
		k = i / 8;

		lut_value =
			luts_value[k][4 * j] |
			((luts_value[k][4 * j + 1] - luts_value[k][4 * j]) << 10) |
			((luts_value[k][4 * j + 2] - luts_value[k][4 * j + 1]) << 17) |
			((luts_value[k][4 * j + 3] - luts_value[k][4 * j + 2]) << 24);
			dpu_set_reg(hiace_base + DPE_GAMMA_VH_W, lut_value, 32,
			0);
	}
}

static void read_lut_value(char __iomem *hiace_base)
{
	int i = 0, j = 0, k = 0;
	uint32_t lut_value = 0;
	for (i = 0; i < 36 * 11; i++) {
		j = i % 11;
		k = i / 11;
		lut_value = inp32(hiace_base + HI_ACE_GAMMA_VH_R_OFFSET);
		hiace_lut_read[k][3 * j] = lut_value & 0x3FF;
		hiace_lut_read[k][3 * j + 1] = (lut_value & 0xFFC00) >> 10;
		if ((3 * j + 2) <= 31)
			hiace_lut_read[k][3 * j + 2] = (lut_value & 0x3FF00000) >> 20;
		disp_pr_info("[%d]:%x\n", i, lut_value);
	}
}
static void hisi_test_read_hiace_lut(char __iomem *dpu_base) {
	char __iomem *hiace_base = NULL;
	int gamma_ab_shadow;
	int gamma_ab_work;

	int i = 0, j = 0, k = 0;
	disp_pr_info("hisi_fb_read_hiace_lut[%d]", hisi_fb_read_hiace_lut);

	if (50 > hisi_fb_read_hiace_lut)
		return;

	hiace_base = dpu_base + HI_ACE_OFFSET;
	gamma_ab_shadow = inp32(hiace_base + HI_ACE_GAMMA_AB_SHADOW_OFFSET);
	gamma_ab_work = inp32(hiace_base + HI_ACE_GAMMA_AB_WORK_OFFSET);
	disp_pr_info("[LIJUAN HIACE] gamma_ab_shadow=0x%x ,gamma_ab_work=0x%x\n",
		gamma_ab_shadow, gamma_ab_work);
	if (gamma_ab_shadow == gamma_ab_work) {
		dpu_set_reg(hiace_base + HI_ACE_GAMMA_R_OFFSET, 1, 1, 31);
		dpu_set_reg(hiace_base + HI_ACE_GAMMA_EN_HV_R_OFFSET, 0, 6, 4);
		read_lut_value(hiace_base);
		dpu_set_reg(hiace_base + HI_ACE_GAMMA_R_OFFSET, 0, 1, 31);
	}
	hisi_fb_read_hiace_lut--;
	return;
}

static void switch_sdr_luts(char __iomem *hiace_base, int cur_hiace_lut)
{
	if (cur_hiace_lut == 21) {
		set_luts(hiace_base, sdr_lut1);
	} else if (cur_hiace_lut == 22) {
		set_luts(hiace_base, sdr_lut2);
	} else if (cur_hiace_lut == 23) {
		set_luts(hiace_base, sdr_lut3);
	} else if (cur_hiace_lut == 24) {
		set_luts(hiace_base, sdr_lut4);
	} else if (cur_hiace_lut == 25) {
		set_luts(hiace_base, sdr_lut5);
	} else if (cur_hiace_lut == 26) {
		set_luts(hiace_base, sdr_lut6);
	} else if (cur_hiace_lut == 27) {
		set_luts(hiace_base, sdr_lut7);
	} else if (cur_hiace_lut == 28) {
		set_luts(hiace_base, sdr_lut8);
	} else if (cur_hiace_lut == 29) {
		set_luts(hiace_base, sdr_lut9);
	} else if (cur_hiace_lut == 30) {
		set_luts(hiace_base, sdr_lut10);
	} else if (cur_hiace_lut == 31) {
		set_luts(hiace_base, sdr_lut11);
	} else if (cur_hiace_lut == 32) {
		set_luts(hiace_base, sdr_lut12);
	} else if (cur_hiace_lut == 33) {
		set_luts(hiace_base, sdr_lut13);
	}
}

static void switch_v3_luts(char __iomem *hiace_base, int cur_hiace_lut)
{
	if (cur_hiace_lut == 41) {
		set_luts(hiace_base, folding_screen_lut1);
	} else if (cur_hiace_lut == 42) {
		set_luts(hiace_base, folding_screen_lut2);
	} else if (cur_hiace_lut == 43) {
		set_luts(hiace_base, local_highlight_hdr_lut1);
	} else if (cur_hiace_lut == 44) {
		set_luts(hiace_base, local_highlight_hdr_lut2);
	} else if (cur_hiace_lut == 45) {
		set_luts(hiace_base, local_highlight_sdr_lut1);
	} else if (cur_hiace_lut == 46) {
		set_luts(hiace_base, local_refresh_lut1);
	} else if (cur_hiace_lut == 47) {
		set_luts(hiace_base, lre_sat_lut1);
	} else if (cur_hiace_lut == 48) {
		set_luts(hiace_base, lre_sat_lut2);
	} else if (cur_hiace_lut == 49) {
		set_luts(hiace_base, skin_count_lut2);
	}
}
static void switch_normal_lut(char __iomem *hiace_base, int cur_hiace_lut)
{
	if (cur_hiace_lut == 10 || cur_hiace_lut == 110) {
		set_luts(hiace_base, hiace_lut10);
	} else if (cur_hiace_lut == 11 || cur_hiace_lut == 111) {
		set_luts(hiace_base, hiace_lut11);
	} else if (cur_hiace_lut == 12 || cur_hiace_lut == 112) {
		set_luts(hiace_base, hiace_lut12);
	} else if (cur_hiace_lut == 13 || cur_hiace_lut == 113) {
		set_luts(hiace_base, hiace_lut13);
	} else if (cur_hiace_lut == 14 || cur_hiace_lut == 114) {
		set_luts(hiace_base, hiace_lut14);
	} else if (cur_hiace_lut == 15 || cur_hiace_lut == 115) {
		set_luts(hiace_base, hiace_lut15);
	}
}
static void switch_normal_luts(char __iomem *hiace_base, int cur_hiace_lut)
{
	disp_pr_info("[effect] hiace cur_hiace_lut =	%d, hisi_fb_hiace_lut %d\n",
		cur_hiace_lut, hisi_fb_hiace_lut);
	if (cur_hiace_lut == 1 || cur_hiace_lut == 101) {
		set_luts(hiace_base, hiace_lut1);
	} else if (cur_hiace_lut == 2 || cur_hiace_lut == 102) {
		set_luts(hiace_base, hiace_lut2);
	} else if (cur_hiace_lut == 3 || cur_hiace_lut == 103) {
		set_luts(hiace_base, hiace_lut3);
	} else if (cur_hiace_lut == 4 || cur_hiace_lut == 104) {
		set_luts(hiace_base, hiace_lut4);
	} else if (cur_hiace_lut == 5 || cur_hiace_lut == 105) {
		set_luts(hiace_base, hiace_lut5);
	} else if (cur_hiace_lut == 6 || cur_hiace_lut == 106) {
		set_luts(hiace_base, hiace_lut6);
	} else if (cur_hiace_lut == 7 || cur_hiace_lut == 107) {
		set_luts(hiace_base, hiace_lut7);
	} else if (cur_hiace_lut == 8 || cur_hiace_lut == 108) {
		set_luts(hiace_base, hiace_lut8);
	} else if (cur_hiace_lut == 9 || cur_hiace_lut == 109) {
		set_luts(hiace_base, hiace_lut9);
	}
	switch_normal_lut(hiace_base, cur_hiace_lut);
}

void dpp_hiace_test(char __iomem *dpu_base)
{
	char __iomem *hiace_base = NULL;
	uint32_t gamma_ab_shadow;
	int gamma_ab_work = 0;
	int cur_hiace_lut = 0;

	cur_hiace_lut = hisi_fb_hiace_lut;
	hiace_base = dpu_base + HI_ACE_OFFSET;

	gamma_ab_shadow = inp32(hiace_base + DPE_GAMMA_AB_SHADOW) & 0x1;
	if (hisi_fb_hiace_lut_times) {
		gamma_ab_work = inp32(hiace_base + DPE_GAMMA_AB_WORK) & 0x1;
		disp_pr_info("[hiace] start set hiace lut! "
								 "gamma_ab_shadow=%d,gamma_ab_work=%d,g_sel_gamma_ab_shadow_"
								 "hdr_lut=%d\n",
								 gamma_ab_shadow, gamma_ab_work, g_sel_gamma_ab_shadow_hdr_lut);
		if (gamma_ab_shadow == gamma_ab_work) {
			dpu_set_reg(hiace_base + DPE_GAMMA_WRITE_EN, 1, 1, 31);
			switch_normal_luts(hiace_base, cur_hiace_lut);
			switch_sdr_luts(hiace_base, cur_hiace_lut);
			switch_v3_luts(hiace_base, cur_hiace_lut);
			hisi_test_read_hiace_lut(dpu_base);
			dpu_set_reg(hiace_base + DPE_GAMMA_WRITE_EN, 0, 1, 31);
			gamma_ab_shadow ^= 1;
		}
	}
	dpp_hiace_hdr_test(hiace_base);
	dpu_set_reg(hiace_base + DPE_GAMMA_AB_SHADOW,
		(gamma_ab_shadow & 0x1) | (g_sel_gamma_ab_shadow_hdr_lut & 0xe),
		 4, 0);
}

static int effect_hiace_get_lhist_band(const char __iomem *hiace_base) {
	uint32_t lhist_en;
	int lhist_quant;
	int lhist_band;

	lhist_en = inp32(DPU_HIACE_LHIST_EN_ADDR(hiace_base));

	lhist_quant = (lhist_en >> 10) & 0x1;

	if (lhist_quant == 0)
		lhist_band = 16;
	else
		lhist_band = 8;

	disp_pr_info("lhist_band = %d", lhist_band);

	return lhist_band;
}

static void read_local_hist(char __iomem *hiace_base, int lhist_band)
{
	uint32_t *local_hist_ptr = NULL;
	int local_sat;
	int i;
	int xpartition = 6;

	local_hist_ptr = &g_histogram[HIACE_GHIST_RANK * 2];
	dpu_set_reg(DPU_HIACE_LHIST_EN_ADDR(hiace_base), 0x1, 1, 31);

	local_sat = (int)inp32(DPU_HIACE_LOCAL_VALID_ADDR(hiace_base));
	disp_pr_info("[hiace] befor read local valid = %d\n", local_sat);

	for (i = 0; i < (6 * xpartition * lhist_band); i++) {
		local_hist_ptr[i] = inp32(DPU_HIACE_LOCAL_HIST_ADDR(hiace_base));
		disp_pr_info("[hiace] local_hist_ptr[%d] = 0x%x \n", i, local_hist_ptr[i]);
	}

	dpu_set_reg(DPU_HIACE_LHIST_EN_ADDR(hiace_base), 0x0, 1, 31);
	local_sat = (int)inp32(DPU_HIACE_LOCAL_VALID_ADDR(hiace_base));
	disp_pr_info("[hiace]local valid = %d\n", local_sat);
}

static void read_fna(char __iomem *hiace_base)
{
	uint32_t *fna_data_ptr = NULL;
	int fna_valid;
	int i;
	int xpartition = 6;
	fna_valid = (int)inp32(DPU_HIACE_FNA_VALID_ADDR(hiace_base));
	if (fna_valid == 1) {
		fna_data_ptr = &g_histogram[HIACE_GHIST_RANK * 2 +
			YBLOCKNUM * XBLOCKNUM * HIACE_LHIST_RANK];

		dpu_set_reg(DPU_HIACE_FNA_ADDR_ADDR(hiace_base), 1, 1, 31);

		for (i = 0; i < (6 * xpartition); i++) {
			fna_data_ptr[i] = inp32(DPU_HIACE_FNA_DATA_ADDR(hiace_base));
			disp_pr_info("[hiace] fna_data_ptr[%d] 0x%x \n", i, fna_data_ptr[i]);
		}

		dpu_set_reg(DPU_HIACE_FNA_ADDR_ADDR(hiace_base), 0, 1, 31);
	}
}

static void read_global_hist(char __iomem *hiace_base)
{
	uint32_t *global_hist_ptr = NULL;
	int i;
	global_hist_ptr = &g_histogram[0];
	for (i = 0; i < 32; i++) {
		global_hist_ptr[i] = inp32(hiace_base + DPU_GLOBAL_HIST_LUT_ADDR + i * 4);
		disp_pr_info("[hiace] global_hist_ptr[%d] = 0x%x \n", i,
			global_hist_ptr[i]);
	}
}

static void read_sat_global_hist(char __iomem *hiace_base)
{
	uint32_t *sat_global_hist_ptr = NULL;
	int i;
	sat_global_hist_ptr = &g_histogram[HIACE_GHIST_RANK];

	for (i = 0; i < 32; i++) {
		sat_global_hist_ptr[i] =
			inp32(hiace_base + DPU_SAT_GLOBAL_HIST_LUT_ADDR + i * 4);
		disp_pr_info("[hiace] sat_global_hist_ptr[%d] = 0x%x \n", i,
			sat_global_hist_ptr[i]);
	}
}

static void read_skin_cnt(char __iomem *dpu_base)
{
	uint32_t *skin_count_ptr = NULL;
	int i;
	uint32_t skin_count_val;
	skin_count_ptr = &g_histogram[CE_SIZE_HIST - HIACE_SKIN_COUNT_RANK - 1];

	for (i = 0; i < 18; i++) {
		skin_count_val =
		inp32(dpu_base + HIACE_SKIN_COUNT + 0x4 * i);
		skin_count_ptr[2 * i] = skin_count_val & 0xFFFF;
		skin_count_ptr[2 * i + 1] = (skin_count_val >> 16) & 0xFFFF;
	}
}

void hiace_end_handle_func(struct work_struct *work)
{
	char __iomem *hiace_base = NULL;
	char __iomem *dpu_base = NULL;
	struct hisi_dpp_data *dpp = NULL;
	int lhist_band;
	int sum_sat;
	int global_hist_ab_shadow;
	int global_hist_ab_work;

	dpp = container_of(work, struct hisi_dpp_data, hiace_end_work);
	if (dpp == NULL) {
		disp_pr_info("[effect] dpp is NULL\n");
		return;
	}
	dpu_base = dpp->dpu_base;
	hiace_base = dpu_base + HI_ACE_OFFSET;
	lhist_band = effect_hiace_get_lhist_band(hiace_base);
	disp_pr_info("[hiace] after safe guard, lhist_band = %d, dpu_base %p\n",
		lhist_band, dpu_base);

	dpu_set_reg(DPU_DPP_DBG_HIACE_ADDR(dpu_base + DPU_DPP_CH0_OFFSET), 0x0, 2, 3);

	sum_sat = (int)inp32(DPU_HIACE_SUM_SATURATION_ADDR(hiace_base));

	read_local_hist(hiace_base, lhist_band);
	read_fna(hiace_base);

	if (hisi_fb_hiace_histogram_protect == 1)
		// protect: permit refresh local_hist and fna
		dpu_set_reg(DPU_DPP_DBG_HIACE_ADDR(dpu_base + DPU_DPP_CH0_OFFSET), 0x3, 2,
			3);

	global_hist_ab_shadow =
		inp32(DPU_HIACE_GLOBAL_HIST_AB_SHADOW_ADDR(hiace_base));
	global_hist_ab_work =
		inp32(DPU_HIACE_GLOBAL_HIST_AB_WORK_ADDR(hiace_base));
	disp_pr_info("[hiace] global_hist_ab_shadow %d, global_hist_ab_work %d \n",
		global_hist_ab_shadow, global_hist_ab_work);

	if (global_hist_ab_shadow == global_hist_ab_work) {
		read_global_hist(hiace_base);
		read_sat_global_hist(hiace_base);
		read_skin_cnt(dpu_base);
		dpu_set_reg(DPU_HIACE_GLOBAL_HIST_AB_SHADOW_ADDR(hiace_base),
		global_hist_ab_shadow ^ 1, 1, 0);
	}

	g_histogram[CE_SIZE_HIST - 1] = sum_sat;
}
