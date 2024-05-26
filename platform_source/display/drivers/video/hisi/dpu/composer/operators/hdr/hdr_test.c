#include "dpu_hihdr.h"
#include "dpu_mitm.h"
#include "hdr_test.h"
#include "hihdr_gtm_lut_test.h"
#include "hisi_disp_debug.h"
#include <securec.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>

/***** test	********/
unsigned hisi_lumacolor_bypass = 0;
module_param_named(debug_lumacolor_bypass, hisi_lumacolor_bypass, int, 0644);
MODULE_PARM_DESC(debug_lumacolor_bypass, "hisi_lumacolor_bypass");

/***** test	********/
unsigned chroma_ratio2lut_value = 0;
module_param_named(debug_chromaRatio2Lut_value, chroma_ratio2lut_value, int, 0644);
MODULE_PARM_DESC(debug_chromaRatio2Lut_value, "chroma_ratio2lut_value");

/***** test ********/
unsigned chroma_ratio2lut_last_value = 0;
module_param_named(debug_chromaRatio2Lut_last_value, chroma_ratio2lut_last_value, int, 0644);
MODULE_PARM_DESC(debug_chromaRatio2Lut_last_value, "chroma_ratio2lut_last_value");

/***** hdr type ********/
unsigned hisi_set_hdrtype = 0;
module_param_named(debug_set_hdrtype, hisi_set_hdrtype, int, 0644);
MODULE_PARM_DESC(debug_set_hdrtype, "hisi_set_hdrtype");

/***** degamma ********/
unsigned hisi_degamma = 0;
module_param_named(debug_degamma, hisi_degamma, int, 0644);
MODULE_PARM_DESC(debug_degamma, "hisi_degamma");

/***** gamma ********/
unsigned hisi_gamma = 0;
module_param_named(debug_gamma, hisi_gamma, int, 0644);
MODULE_PARM_DESC(debug_gamma, "hisi_gamma");

/***** gamut ********/
unsigned hisi_gamut = 0;
module_param_named(debug_gamut, hisi_gamut, int, 0644);
MODULE_PARM_DESC(debug_gamut, "hisi_gamut");

/***** gamut ********/
unsigned hisi_gamut_coef00 = 0;
module_param_named(debug_gamut_coef00, hisi_gamut_coef00, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef00, "hisi_gamut_coef00");

/***** gamut ********/
unsigned hisi_gamut_coef01 = 0;
module_param_named(debug_gamut_coef01, hisi_gamut_coef01, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef01, "hisi_gamut_coef01");

/***** gamut ********/
unsigned hisi_gamut_coef02 = 0;
module_param_named(debug_gamut_coef02, hisi_gamut_coef02, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef02, "hisi_gamut_coef02");
/***** gamut ********/
unsigned hisi_gamut_coef10 = 0;
module_param_named(debug_gamut_coef10, hisi_gamut_coef10, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef10, "hisi_gamut_coef10");

/***** gamut ********/
unsigned hisi_gamut_coef11 = 0;
module_param_named(debug_gamut_coef11, hisi_gamut_coef11, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef11, "hisi_gamut_coef11");

/***** gamut ********/
unsigned hisi_gamut_coef12 = 0;
module_param_named(debug_gamut_coef12, hisi_gamut_coef12, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef12, "hisi_gamut_coef12");

/***** gamut ********/
unsigned hisi_gamut_coef20 = 0;
module_param_named(debug_gamut_coef20, hisi_gamut_coef20, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef20, "hisi_gamut_coef20");

/***** gamut ********/
unsigned hisi_gamut_coef21 = 0;
module_param_named(debug_gamut_coef21, hisi_gamut_coef21, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef21, "hisi_gamut_coef21");

/***** gamut ********/
unsigned hisi_gamut_coef22 = 0;
module_param_named(debug_gamut_coef22, hisi_gamut_coef22, int, 0644);
MODULE_PARM_DESC(debug_gamut_coef22, "hisi_gamut_coef22");

struct dpu_wcg_info g_wcg_info[2];

static void hdr_gtm_param_normal_config(struct dpu_hdr_info *hdr_param)
{
	hdr_param->flag = FLAG_BASIC;
	hdr_param->gtm_info.isinslf = 1;
	hdr_param->gtm_info.input_min_e = 6;
	hdr_param->gtm_info.output_min_e = 2884;
	hdr_param->gtm_info.ratiolutinclip = 4096;
	hdr_param->gtm_info.ratiolutoutclip = 1024;

	hdr_param->gtm_info.lumaoutbit0 = 3;
	hdr_param->gtm_info.lumaoutbit1 = 8;

	hdr_param->gtm_info.chromaoutbit1 = 21;
	hdr_param->gtm_info.chromaoutbit2 = 2;

	hdr_param->gtm_info.chromaratiolutstep0 = 2;
	hdr_param->gtm_info.chromaratiolutstep1 = 4;

	hdr_param->gtm_info.lumaratiolutstep0 = 1;
	hdr_param->gtm_info.lumaratiolutstep1 = 4;

	hdr_param->gtm_info.chromalutthres = 64;
	hdr_param->gtm_info.lumalutthres = 128;
	hdr_param->gtm_info.chromaratioclip = 2048;

	if (hisi_set_hdrtype & BIT(0)) { // CUVA
		hdr_param->gtm_info.isinslf = 0;
	} else if (hisi_set_hdrtype & BIT(1)) { // NONE
		hdr_param->gtm_info.isinslf = 0;
	}

	memcpy_s(hdr_param->gtm_info.y_transform, sizeof(hdr_param->gtm_info.y_transform),
		y_transform, sizeof(y_transform));
	memcpy_s(hdr_param->gtm_info.pq2slf_step, sizeof(hdr_param->gtm_info.pq2slf_step),
		step_slf, sizeof(step_slf));
	memcpy_s(hdr_param->gtm_info.pq2slf_pos, sizeof(hdr_param->gtm_info.pq2slf_pos),
		pos_slf, sizeof(pos_slf));
	memcpy_s(hdr_param->gtm_info.pq2slf_num, sizeof(hdr_param->gtm_info.pq2slf_num),
		num_slf, sizeof(num_slf));
	memcpy_s(hdr_param->gtm_info.lut_pq2slf, sizeof(hdr_param->gtm_info.lut_pq2slf),
		pq2slflut, sizeof(pq2slflut));
	memcpy_s(hdr_param->gtm_info.lut_lumalut0, sizeof(hdr_param->gtm_info.lut_lumalut0),
		lum_ratio_lut0, sizeof(lum_ratio_lut0));
	memcpy_s(hdr_param->gtm_info.lut_chromalut0, sizeof(hdr_param->gtm_info.lut_chromalut0),
		chroma_ratio_lut0, sizeof(chroma_ratio_lut0));
	memcpy_s(hdr_param->gtm_info.lut_luma, sizeof(hdr_param->gtm_info.lut_luma),
		lum_ratio_lut, sizeof(lum_ratio_lut));
	memcpy_s(hdr_param->gtm_info.lut_chroma, sizeof(hdr_param->gtm_info.lut_chroma),
		chroma_ratio_lut, sizeof(chroma_ratio_lut));
	memcpy_s(hdr_param->gtm_info.lut_chroma0, sizeof(hdr_param->gtm_info.lut_chroma0),
		chroma_ratio0_lut, sizeof(chroma_ratio0_lut));
	memcpy_s(hdr_param->gtm_info.lut_chroma1, sizeof(hdr_param->gtm_info.lut_chroma1),
		chroma_ratio1_lut, sizeof(chroma_ratio1_lut));
	memcpy_s(hdr_param->gtm_info.lut_chroma2, sizeof(hdr_param->gtm_info.lut_chroma2),
		chroma_ratio2_lut, sizeof(chroma_ratio2_lut));
}

static void hdr_gtm_param_h_l_luma_bypass(struct dpu_hdr_info *hdr_param)
{
	memset_s(hdr_param->gtm_info.lut_lumalut0, sizeof(hdr_param->gtm_info.lut_lumalut0),
		g_lum_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_lumalut0));
	memset_s(hdr_param->gtm_info.lut_luma, sizeof(hdr_param->gtm_info.lut_luma),
		lum_ratio_lut_bypass, sizeof(hdr_param->gtm_info.lut_luma));
}

static void hdr_gtm_param_chroma_bypass(struct dpu_hdr_info *hdr_param)
{
	memset_s(hdr_param->gtm_info.lut_chromalut0, sizeof(hdr_param->gtm_info.lut_chromalut0),
		chroma_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_chromalut0));
	memset_s(hdr_param->gtm_info.lut_chroma, sizeof(hdr_param->gtm_info.lut_chroma),
		chroma_ratio_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma));
	memset_s(hdr_param->gtm_info.lut_chroma0, sizeof(hdr_param->gtm_info.lut_chroma0),
		chroma_ratio0_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma0));
	memset_s(hdr_param->gtm_info.lut_chroma1, sizeof(hdr_param->gtm_info.lut_chroma1),
		chroma_ratio1_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma1));
	memset_s(hdr_param->gtm_info.lut_chroma2, sizeof(hdr_param->gtm_info.lut_chroma2),
		chroma_ratio2_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma2));
}

static void hdr_gtm_param_h_l_luma_chroma_bypass(struct dpu_hdr_info *hdr_param)
{
	hdr_gtm_param_h_l_luma_bypass(hdr_param);
	memset_s(hdr_param->gtm_info.lut_chromalut0, sizeof(hdr_param->gtm_info.lut_chromalut0),
		chroma_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_chromalut0));
	memset_s(hdr_param->gtm_info.lut_chroma, sizeof(hdr_param->gtm_info.lut_chroma),
		chroma_ratio_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma));
}

static void hdr_gtm_param_chroma_r_bypass(struct dpu_hdr_info *hdr_param)
{
	memset_s(hdr_param->gtm_info.lut_chroma0, sizeof(hdr_param->gtm_info.lut_chroma0),
		chroma_ratio0_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma0));
}

static void hdr_gtm_param_chroma_g_bypass(struct dpu_hdr_info *hdr_param)
{
	memset_s(hdr_param->gtm_info.lut_chroma1, sizeof(hdr_param->gtm_info.lut_chroma1),
		chroma_ratio1_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma1));
}

static void hdr_gtm_param_chroma_b_bypass(struct dpu_hdr_info *hdr_param)
{
	memset_s(hdr_param->gtm_info.lut_chroma2, sizeof(hdr_param->gtm_info.lut_chroma2),
		chroma_ratio2_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma2));
}

static void hdr_gtm_param_bit_bypass(struct dpu_hdr_info *hdr_param)
{
	hdr_param->gtm_info.input_min_e = 0;
	hdr_param->gtm_info.output_min_e = 0;
	hdr_param->gtm_info.chromaoutbit2 = 0;
}

static void hdr_gtm_param_lumacolor_config(struct dpu_hdr_info *hdr_param)
{
	if (hisi_lumacolor_bypass & BIT(0)) { // lumacolor bypass
		hdr_gtm_param_h_l_luma_chroma_bypass(hdr_param);
		hdr_gtm_param_chroma_r_bypass(hdr_param);
		hdr_gtm_param_chroma_g_bypass(hdr_param);
		hdr_gtm_param_chroma_b_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(1)) { // lumacolor bypass b
		hdr_gtm_param_h_l_luma_chroma_bypass(hdr_param);
		hdr_gtm_param_chroma_b_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(2)) { // lumacolor bypass bmin
		hdr_gtm_param_h_l_luma_bypass(hdr_param);
		memset_s(hdr_param->gtm_info.lut_chroma2, sizeof(hdr_param->gtm_info.lut_chroma2),
			0, sizeof(hdr_param->gtm_info.lut_chroma2));
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(3)) { // lumacolor bypass g
		hdr_gtm_param_h_l_luma_chroma_bypass(hdr_param);
		hdr_gtm_param_chroma_g_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(4)) { // lumacolor bypass gmin
		hdr_gtm_param_h_l_luma_bypass(hdr_param);
		memset_s(hdr_param->gtm_info.lut_chroma1, sizeof(hdr_param->gtm_info.lut_chroma1),
			0, sizeof(hdr_param->gtm_info.lut_chroma1));
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(5)) { // lumacolor bypass r
		hdr_gtm_param_h_l_luma_chroma_bypass(hdr_param);
		hdr_gtm_param_chroma_r_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(6)) { // lumacolor bypass rmin
		hdr_gtm_param_h_l_luma_bypass(hdr_param);
		memset_s(hdr_param->gtm_info.lut_chroma0, sizeof(hdr_param->gtm_info.lut_chroma0),
			0, sizeof(hdr_param->gtm_info.lut_chroma0));
		hdr_gtm_param_bit_bypass(hdr_param);
	}
}

static void hdr_gtm_param_h_l_luma_config(struct dpu_hdr_info *hdr_param)
{
	if (hisi_lumacolor_bypass & BIT(7)) { // high bright bypass
		memset_s(hdr_param->gtm_info.lut_luma, sizeof(hdr_param->gtm_info.lut_luma),
			lum_ratio_lut_bypass, sizeof(hdr_param->gtm_info.lut_luma));
		hdr_gtm_param_chroma_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(8)) { // high bright min
		memset_s(hdr_param->gtm_info.lut_luma, sizeof(hdr_param->gtm_info.lut_luma),
			0, sizeof(hdr_param->gtm_info.lut_luma));
		hdr_gtm_param_chroma_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(9)) { // low bright bypass
		memset_s(hdr_param->gtm_info.lut_lumalut0, sizeof(hdr_param->gtm_info.lut_lumalut0),
			g_lum_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_lumalut0));
		hdr_gtm_param_chroma_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(10)) { // low bright min
		memset_s(hdr_param->gtm_info.lut_lumalut0, sizeof(hdr_param->gtm_info.lut_lumalut0),
			0, sizeof(hdr_param->gtm_info.lut_lumalut0));
		hdr_gtm_param_chroma_bypass(hdr_param);
		hdr_gtm_param_bit_bypass(hdr_param);
	}
}

void dpu_hdr_gtm_param_config(struct dpu_hdr_info *hdr_param) {
	disp_pr_info(" ++++ ");

	hdr_gtm_param_normal_config(hdr_param);
	hdr_gtm_param_lumacolor_config(hdr_param);
	hdr_gtm_param_h_l_luma_config(hdr_param);

	if (hisi_lumacolor_bypass & BIT(11)) { // pq2slf max
		memset_s(hdr_param->gtm_info.lut_pq2slf, sizeof(hdr_param->gtm_info.lut_pq2slf),
			g_pq2slf_lut_max, sizeof(hdr_param->gtm_info.lut_pq2slf));
	} else if (hisi_lumacolor_bypass & BIT(12)) { // pq2slf min
		memset_s(hdr_param->gtm_info.lut_pq2slf, sizeof(hdr_param->gtm_info.lut_pq2slf),
			0, sizeof(hdr_param->gtm_info.lut_pq2slf));
	} else if (hisi_lumacolor_bypass & BIT(13)) { // pq2slf linear
		memcpy_s(hdr_param->gtm_info.lut_pq2slf, sizeof(hdr_param->gtm_info.lut_pq2slf),
			g_pq2slf_lut_liner, sizeof(g_pq2slf_lut_liner));
	} else if (hisi_lumacolor_bypass & BIT(14)) { // low chrome min
		memset_s(hdr_param->gtm_info.lut_lumalut0, sizeof(hdr_param->gtm_info.lut_lumalut0),
			g_lum_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_lumalut0));
		memset_s(hdr_param->gtm_info.lut_chromalut0, sizeof(hdr_param->gtm_info.lut_chromalut0),
			0, sizeof(hdr_param->gtm_info.lut_chromalut0));
		memset_s(hdr_param->gtm_info.lut_chroma, sizeof(hdr_param->gtm_info.lut_chroma),
			chroma_ratio_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma));
		memset_s(hdr_param->gtm_info.lut_chroma0, sizeof(hdr_param->gtm_info.lut_chroma0),
			chroma_ratio0_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma0));
		memset_s(hdr_param->gtm_info.lut_chroma1, sizeof(hdr_param->gtm_info.lut_chroma1),
			chroma_ratio1_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma1));
		memset_s(hdr_param->gtm_info.lut_chroma2, sizeof(hdr_param->gtm_info.lut_chroma2),
			chroma_ratio2_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma2));
		hdr_gtm_param_bit_bypass(hdr_param);
	} else if (hisi_lumacolor_bypass & BIT(15)) { // high chrome min
		memset_s(hdr_param->gtm_info.lut_lumalut0, sizeof(hdr_param->gtm_info.lut_lumalut0),
			g_lum_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_lumalut0));
		memset_s(hdr_param->gtm_info.lut_chromalut0, sizeof(hdr_param->gtm_info.lut_chromalut0),
			chroma_ratio_lut0_bypass, sizeof(hdr_param->gtm_info.lut_chromalut0));
		memset_s(hdr_param->gtm_info.lut_chroma, sizeof(hdr_param->gtm_info.lut_chroma),
			0, sizeof(hdr_param->gtm_info.lut_chroma));
		memset_s(hdr_param->gtm_info.lut_chroma0, sizeof(hdr_param->gtm_info.lut_chroma0),
			chroma_ratio0_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma0));
		memset_s(hdr_param->gtm_info.lut_chroma1, sizeof(hdr_param->gtm_info.lut_chroma1),
			chroma_ratio1_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma1));
		memset_s(hdr_param->gtm_info.lut_chroma2, sizeof(hdr_param->gtm_info.lut_chroma2),
			chroma_ratio2_lut_bypass, sizeof(hdr_param->gtm_info.lut_chroma2));
		hdr_gtm_param_bit_bypass(hdr_param);
	}
	disp_pr_info(" ---- ");
};

/* sRGB to P3 */
uint32_t g_itm_degamma_step_map[8] = {5, 5, 5, 4, 3, 3, 3, 0};
uint32_t g_itm_degamma_num_map[8] = {1, 10, 19, 28, 37, 55, 62, 63};
uint32_t g_itm_degamma_pos_map[8] = {32, 320, 608, 752, 824, 968, 1022, 1023};
uint32_t g_itm_degamma_lut_map[64] = {
		0, 158, 337, 595, 940, 1380, 1923, 2572, 3334, 4213, 5215,
		6343, 7603, 8997, 10530, 12205, 14027, 15998, 18121, 20401, 21600, 22839,
		24119, 25440, 26802, 28206, 29651, 31139, 32670, 33451, 34243, 35047, 35860,
		36685, 37521, 38368, 39226, 40095, 40975, 41866, 42768, 43682, 44607, 45543,
		46491, 47450, 48420, 49402, 50396, 51401, 52418, 53446, 54486, 55538, 56601,
		57676, 58763, 59862, 60973, 62095, 63230, 64377, 65535, 65535};
uint32_t g_itm_degamma_lut_map_min[64] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
uint32_t g_itm_degamma_lut_map_max[64] = {
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};
uint32_t g_itm_degamma_lut_map_max_fff[64] = { // temp test
		0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
		0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
		0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
		0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
		0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
		0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
		0xfff, 0xfff, 0xfff, 0xfff,
};

uint32_t g_itm_gamma_step_map[8] = {7, 7, 8, 9, 10, 11, 13, 0};
uint32_t g_itm_gamma_num_map[8] = {1, 32, 42, 47, 52, 57, 62, 63};
uint32_t g_itm_gamma_pos_map[8] = {128, 4096, 6656, 9216, 14336, 24576, 65534, 65535};
uint32_t g_itm_gamma_lut_map[64] = {
		0, 25, 50, 70, 86, 100, 113, 124, 134, 144, 153, 161, 169,
		177, 184, 191, 198, 205, 211, 217, 223, 229, 234, 240, 245, 250,
		255, 260, 265, 270, 274, 279, 283, 292, 301, 309, 317, 324, 332,
		339, 346, 353, 360, 373, 385, 397, 409, 420, 442, 462, 481, 499,
		517, 549, 580, 609, 636, 661, 753, 831, 901, 965, 1023, 1023};
uint32_t g_itm_gamma_lut_map_min[64] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
uint32_t g_itm_gamma_lut_map_max[64] = {
		0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff,
		0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff,
		0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff,
		0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff,
		0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff,
		0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff,
};

uint32_t g_itm_gamut_coef[9] = {842, 182, 0, 34, 990, 0, 18, 74, 932};
uint32_t g_itm_gamut_coef_zero[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

void dpu_mitm_param_config(struct wcg_mitm_info *mitm_param) {
	int i;
	disp_pr_info(" ++++ ");

	memset_s(mitm_param, sizeof(struct wcg_mitm_info), 0, sizeof(struct wcg_mitm_info));

	mitm_param->itm_slf_gamut_coef00 = g_itm_degamma_step_map[0];

	// step map
	mitm_param->itm_slf_degamma_step1 =
			(g_itm_degamma_step_map[3] << 24) | (g_itm_degamma_step_map[2] << 16) |
			(g_itm_degamma_step_map[1] << 8) | g_itm_degamma_step_map[0];
	mitm_param->itm_slf_degamma_step2 =
			(g_itm_degamma_step_map[7] << 24) | (g_itm_degamma_step_map[6] << 16) |
			(g_itm_degamma_step_map[5] << 8) | g_itm_degamma_step_map[4];

	// num map
	mitm_param->itm_slf_degamma_num1 =
			(g_itm_degamma_num_map[3] << 24) | (g_itm_degamma_num_map[2] << 16) |
			(g_itm_degamma_num_map[1] << 8) | g_itm_degamma_num_map[0];
	mitm_param->itm_slf_degamma_num2 =
			(g_itm_degamma_num_map[7] << 24) | (g_itm_degamma_num_map[6] << 16) |
			(g_itm_degamma_num_map[5] << 8) | g_itm_degamma_num_map[4];

	// pos map
	mitm_param->itm_slf_degamma_pos1 =
			(g_itm_degamma_pos_map[1] << 16) | g_itm_degamma_pos_map[0];
	mitm_param->itm_slf_degamma_pos2 =
			(g_itm_degamma_pos_map[3] << 16) | g_itm_degamma_pos_map[2];
	mitm_param->itm_slf_degamma_pos3 =
			(g_itm_degamma_pos_map[5] << 16) | g_itm_degamma_pos_map[4];
	mitm_param->itm_slf_degamma_pos4 =
			(g_itm_degamma_pos_map[7] << 16) | g_itm_degamma_pos_map[6];

	// degamma lut
	if (hisi_degamma == 0) {
		memcpy_s(mitm_param->itm_degamma_lut, sizeof(mitm_param->itm_degamma_lut),
			g_itm_degamma_lut_map, sizeof(g_itm_degamma_lut_map));
	} else if (hisi_degamma == 1) { // min
		memcpy_s(mitm_param->itm_degamma_lut, sizeof(mitm_param->itm_degamma_lut),
			g_itm_degamma_lut_map_min, sizeof(g_itm_degamma_lut_map_min));
	} else if (hisi_degamma == 2) { // max
		memcpy_s(mitm_param->itm_degamma_lut, sizeof(mitm_param->itm_degamma_lut),
			g_itm_degamma_lut_map_max, sizeof(g_itm_degamma_lut_map_max));
	}

	// step map
	mitm_param->itm_slf_gamma_step1 =
			(g_itm_gamma_step_map[3] << 24) | (g_itm_gamma_step_map[2] << 16) |
			(g_itm_gamma_step_map[1] << 8) | g_itm_gamma_step_map[0];
	mitm_param->itm_slf_gamma_step2 =
			(g_itm_gamma_step_map[7] << 24) | (g_itm_gamma_step_map[6] << 16) |
			(g_itm_gamma_step_map[5] << 8) | g_itm_gamma_step_map[4];

	// num map
	mitm_param->itm_slf_gamma_num1 =
			(g_itm_gamma_num_map[3] << 24) | (g_itm_gamma_num_map[2] << 16) |
			(g_itm_gamma_num_map[1] << 8) | g_itm_gamma_num_map[0];
	mitm_param->itm_slf_gamma_num2 =
			(g_itm_gamma_num_map[7] << 24) | (g_itm_gamma_num_map[6] << 16) |
			(g_itm_gamma_num_map[5] << 8) | g_itm_gamma_num_map[4];

	// pos map
	mitm_param->itm_slf_gamma_pos1 = g_itm_gamma_pos_map[0];
	mitm_param->itm_slf_gamma_pos2 = g_itm_gamma_pos_map[1];
	mitm_param->itm_slf_gamma_pos3 = g_itm_gamma_pos_map[2];
	mitm_param->itm_slf_gamma_pos4 = g_itm_gamma_pos_map[3];
	mitm_param->itm_slf_gamma_pos5 = g_itm_gamma_pos_map[4];
	mitm_param->itm_slf_gamma_pos6 = g_itm_gamma_pos_map[5];
	mitm_param->itm_slf_gamma_pos7 = g_itm_gamma_pos_map[6];
	mitm_param->itm_slf_gamma_pos8 = g_itm_gamma_pos_map[7];

	/* gamma lut */
	if (hisi_gamma == 0) {
		memcpy_s(mitm_param->itm_gamma_lut, sizeof(mitm_param->itm_gamma_lut),
			g_itm_gamma_lut_map, sizeof(g_itm_gamma_lut_map));
	} else if (hisi_gamma == 1) {
		memcpy_s(mitm_param->itm_gamma_lut, sizeof(mitm_param->itm_gamma_lut),
			g_itm_gamma_lut_map_min, sizeof(g_itm_gamma_lut_map_min));
	} else if (hisi_gamma == 2) {
		memcpy_s(mitm_param->itm_gamma_lut, sizeof(mitm_param->itm_gamma_lut),
			g_itm_gamma_lut_map_max, sizeof(g_itm_gamma_lut_map_max));
	}

	mitm_param->itm_slf_gamut_coef00 = g_itm_gamut_coef[0]; // R
	mitm_param->itm_slf_gamut_coef01 = g_itm_gamut_coef[1];
	mitm_param->itm_slf_gamut_coef02 = g_itm_gamut_coef[2];
	mitm_param->itm_slf_gamut_coef10 = g_itm_gamut_coef[3];
	mitm_param->itm_slf_gamut_coef11 = g_itm_gamut_coef[4]; // G
	mitm_param->itm_slf_gamut_coef12 = g_itm_gamut_coef[5];
	mitm_param->itm_slf_gamut_coef20 = g_itm_gamut_coef[6];
	mitm_param->itm_slf_gamut_coef21 = g_itm_gamut_coef[7];
	mitm_param->itm_slf_gamut_coef22 = g_itm_gamut_coef[8]; // B

	if (hisi_gamut == 0) {
	} else if (hisi_gamut == 1) {
		mitm_param->itm_slf_gamut_coef00 = hisi_gamut_coef00;
		mitm_param->itm_slf_gamut_coef01 = hisi_gamut_coef01;
		mitm_param->itm_slf_gamut_coef02 = hisi_gamut_coef02;
		mitm_param->itm_slf_gamut_coef10 = hisi_gamut_coef10;
		mitm_param->itm_slf_gamut_coef11 = hisi_gamut_coef11; // G
		mitm_param->itm_slf_gamut_coef12 = hisi_gamut_coef12;
		mitm_param->itm_slf_gamut_coef20 = hisi_gamut_coef20;
		mitm_param->itm_slf_gamut_coef21 = hisi_gamut_coef21;
		mitm_param->itm_slf_gamut_coef22 = hisi_gamut_coef22; // B
	}

	mitm_param->itm_slf_gamut_scale = 10;
	mitm_param->itm_slf_gamut_clip_max = 65535;

	disp_pr_info(" ---- ");
}
