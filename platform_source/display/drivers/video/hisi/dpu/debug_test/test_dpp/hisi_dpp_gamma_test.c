#include <linux/moduleparam.h>
#include "hisi_dpp_gamma_test.h"
#include "hisi_disp_config.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"

unsigned hisi_fb_gamma_lut_r = 11;
module_param_named(debug_gamma_lut_r, hisi_fb_gamma_lut_r, int, 0644);
MODULE_PARM_DESC(debug_gamma_lut_r, "hisi_fb_gamma_lut_r");

unsigned hisi_fb_gamma_lut_g = 11;
module_param_named(debug_gamma_lut_g, hisi_fb_gamma_lut_g, int, 0644);
MODULE_PARM_DESC(debug_gamma_lut_g, "hisi_fb_gamma_lut_g");

unsigned hisi_fb_gamma_lut_b = 11;
module_param_named(debug_gamma_lut_b, hisi_fb_gamma_lut_b, int, 0644);
MODULE_PARM_DESC(debug_gamma_lut_b, "hisi_fb_gamma_lut_b");

unsigned hisi_fb_degamma_lut_r = 11;
module_param_named(debug_degamma_lut_r, hisi_fb_degamma_lut_r, int, 0644);
MODULE_PARM_DESC(debug_degamma_lut_r, "hisi_fb_degamma_lut_r");

unsigned hisi_fb_degamma_lut_g = 11;
module_param_named(debug_degamma_lut_g, hisi_fb_degamma_lut_g, int, 0644);
MODULE_PARM_DESC(debug_degamma_lut_g, "hisi_fb_degamma_lut_g");

unsigned hisi_fb_degamma_lut_b = 11;
module_param_named(debug_degamma_lut_b, hisi_fb_degamma_lut_b, int, 0644);
MODULE_PARM_DESC(debug_degamma_lut_b, "hisi_fb_degamma_lut_b");

unsigned hisi_fb_display_effect_test_color = 0x7;
module_param_named(debug_display_effect_test_color,
hisi_fb_display_effect_test_color, int, 0644);
MODULE_PARM_DESC(debug_display_effect_test_color,
"hisi_fb_display_effect_test_color");

static u32 gama_nolinear_lut[] = {
	#include "gmp_test_params/gama_nolinear_lut.txt"
};

static u32 degamma_nolinear_lut[] = {
	#include "gmp_test_params/degamma_nolinear_lut.txt"
};

static u32 linear_lut[] = {
	#include "gmp_test_params/gama_linear_lut.txt"
};

static void set_normal_luts(char __iomem *addr, u32 *lut_value)
{
	int cnt;
	for (cnt = 0; cnt < GAMMA_LUT_LEN; cnt = cnt + 2) {
			if (hisi_fb_display_effect_test_color & BIT(0)) {
				dpu_set_reg(addr + (U_R_COEF + cnt * 2),
					lut_value[cnt], 13, 0);
				if (cnt != GAMMA_LUT_LEN - 1)
					dpu_set_reg(addr + (U_R_COEF + cnt * 2),
					lut_value[cnt + 1], 13, 16);
			}
			if (hisi_fb_display_effect_test_color & BIT(1)) {
				dpu_set_reg(addr + (U_G_COEF + cnt * 2),
					lut_value[cnt], 13, 0);
				if (cnt != GAMMA_LUT_LEN - 1)
					dpu_set_reg(addr + (U_G_COEF + cnt * 2),
						lut_value[cnt + 1], 13, 16);
			}
			if (hisi_fb_display_effect_test_color & BIT(2)) {
				dpu_set_reg(addr + (U_B_COEF + cnt * 2),
					lut_value[cnt], 13, 0);
				if (cnt != GAMMA_LUT_LEN - 1)
					dpu_set_reg(addr + (U_B_COEF + cnt * 2),
						lut_value[cnt + 1], 13, 16);
			}
		}
}

static void set_user_luts(char __iomem *addr, uint32_t flag)
{
	int cnt = 0;
	int r = 0;
	int g = 0;
	int b = 0;
	if (flag == 0) {
		r = hisi_fb_degamma_lut_r;
		g = hisi_fb_degamma_lut_g;
		b = hisi_fb_degamma_lut_b;
	} else {
		r = hisi_fb_gamma_lut_r;
		g = hisi_fb_gamma_lut_g;
		b = hisi_fb_gamma_lut_b;
	}
	for (cnt = 0; cnt < IGM_LUT_LEN; cnt = cnt + 2) {
			if (hisi_fb_display_effect_test_color & BIT(0)) {
				dpu_set_reg(addr + (U_R_COEF + cnt * 2),
					r, 13, 0);

				if (cnt != IGM_LUT_LEN - 1)
					dpu_set_reg(addr + (U_R_COEF + cnt * 2),
						r, 13, 16);
			}

			if (hisi_fb_display_effect_test_color & BIT(1)) {
				dpu_set_reg(addr + (U_G_COEF + cnt * 2),
					g, 13, 0);

				if (cnt != IGM_LUT_LEN - 1)
					dpu_set_reg(addr + (U_G_COEF + cnt * 2),
						g, 13, 16);
			}

			if (hisi_fb_display_effect_test_color & BIT(2)) {
				dpu_set_reg(addr + (U_B_COEF + cnt * 2),
					b, 13, 0);

				if (cnt != IGM_LUT_LEN - 1)
					dpu_set_reg(addr + (U_B_COEF + cnt * 2),
						b, 13, 16);
			}
		}
}

void dpp_effect_degamma_test(char __iomem *dpu_base)
{
	int cnt;
	char __iomem *degamma_lut_base = dpu_base + DPU_DPP_CH0_DEGAMMA_LUT_OFFSET;
	degamma_lut_base = dpu_base + DPU_DPP_CH0_DEGAMMA_LUT_OFFSET;

	if (hisi_fb_display_effect_test & BIT(3)) { // linear lut
		set_normal_luts(degamma_lut_base, linear_lut);
	} else if (hisi_fb_display_effect_test & BIT(4)) { // no-linear lut
		set_normal_luts(degamma_lut_base, degamma_nolinear_lut);
	} else if (hisi_fb_display_effect_test & BIT(5)) { // user lut
		set_user_luts(degamma_lut_base, 0);
	}
}

void dpp_effect_gamma_test(char __iomem *dpu_base)
{
	char __iomem *gamma0_lut_base = NULL;
	char __iomem *gamma1_lut_base = NULL;
	gamma0_lut_base = dpu_base + DPU_DPP_CH0_GAMA0_LUT_OFFSET;
	gamma1_lut_base = dpu_base + DPU_DPP_CH0_GAMA1_LUT_OFFSET;

	if (hisi_fb_display_effect_test & BIT(0)) { // linear lut
		set_normal_luts(gamma0_lut_base, linear_lut);
		set_normal_luts(gamma1_lut_base, linear_lut);
	} else if (hisi_fb_display_effect_test & BIT(1)) { // no-linear lut
		set_normal_luts(gamma0_lut_base, gama_nolinear_lut);
		set_normal_luts(gamma1_lut_base, gama_nolinear_lut);
	} else if (hisi_fb_display_effect_test & BIT(2)) { // user lut
		set_user_luts(gamma0_lut_base, 1);
		set_user_luts(gamma1_lut_base, 1);
	} else if (hisi_fb_display_effect_test & BIT(3)) { // no-linear linear-lut
		set_normal_luts(gamma0_lut_base, linear_lut);
		set_normal_luts(gamma1_lut_base, gama_nolinear_lut);
	}
}