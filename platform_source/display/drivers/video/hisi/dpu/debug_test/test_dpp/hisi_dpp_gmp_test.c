#include <linux/moduleparam.h>
#include "hisi_dpp_gmp_test.h"
#include "hisi_disp_config.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"
unsigned hisi_fb_gmp_lut_high4 = 0xf;
module_param_named(debug_gmp_lut_high4, hisi_fb_gmp_lut_high4, int, 0644);
MODULE_PARM_DESC(debug_gmp_lut_high4, "hisi_fb_gmp_lut_high4");

unsigned hisi_fb_gmp_lut_low32 = 0xffffffff;
module_param_named(debug_gmp_lut_low32, hisi_fb_gmp_lut_low32, int, 0644);
MODULE_PARM_DESC(debug_gmp_lut_low32, "hisi_fb_gmp_lut_low32");

static u32 gmp_lut_table_low32bit_yellow[729] = {
	#include "gmp_test_params/low32bit_yellow.txt"
};

static u32 gmp_lut_table_high4bit_yellow[729] = {
	#include "gmp_test_params/high4bit_yellow.txt"
};

static u32 gmp_lut_table_low32bit_blue[729] = {
	#include "gmp_test_params/low32bit_blue.txt"
};

static u32 gmp_lut_table_high4bit_blue[729] = {
	#include "gmp_test_params/high4bit_blue.txt"
};

static u32 gmp_lut_table_low32bit_normal[729] = {
	#include "gmp_test_params/low32bit_normal.txt"
};

static u32 gmp_lut_table_high4bit_normal[729] = {
	#include "gmp_test_params/high4bit_normal.txt"
};

static u32 gmp_lut_low[] = {
	#include "gmp_test_params/low.txt"
};

static u32 gmp_lut_high[] = {
	#include "gmp_test_params/high.txt"
};

static u32 gmp_lut_low_srgb[] = {
	#include "gmp_test_params/low_srgb.txt"
};

static u32 gmp_lut_high_srgb[] = {
	#include "gmp_test_params/high_srgb.txt"
};

static u32 gmp_lut_low_p3[] = {
	#include "gmp_test_params/low_p3.txt"
};

static u32 gmp_lut_high_p3[] = {
	#include "gmp_test_params/high_p3.txt"
};

static u32 gmp_lut_low_srgb_p3[] = {
	#include "gmp_test_params/low_srgb_p3.txt"
};

static u32 gmp_lut_high_srgb_p3[] = {
	#include "gmp_test_params/high_srgb_p3.txt"
};

static u32 gmp_lut_low_720_480[] = {
	#include "gmp_test_params/low_720_480.txt"
};

static u32 gmp_lut_high_720_480[] = {
	#include "gmp_test_params/high_720_480.txt"
};

static u32 gmp_lut_low_1080_1920[] = {
	#include "gmp_test_params/low_1080_1920.txt"
};

static u32 gmp_lut_high_1080_1920[] = {
	#include "gmp_test_params/high_1080_1920.txt"
};

static u32 gmp_lut_low_1920_1080[] = {
	#include "gmp_test_params/low_1920_1080.txt"
};
static u32 gmp_lut_high_1920_1080[] = {
	#include "gmp_test_params/high_1920_1080.txt"
};
static u32 gmp_lut_low_2008_1504[] = {
	#include "gmp_test_params/low_2008_1504.txt"
};
static u32 gmp_lut_high_2008_1504[] = {
	#include "gmp_test_params/high_2008_1504.txt"
};

static u32 gmp_lut_low_3264_2448[] = {
	#include "gmp_test_params/low_3264_2448.txt"
};

static u32 gmp_lut_high_3264_2448[] = {
	#include "gmp_test_params/high_3264_2448.txt"
};

static u32 gmp_lut_low_3840_2160[] = {
	#include "gmp_test_params/low_3840_2160.txt"
};

static u32 gmp_lut_high_3840_2160[] = {
	#include "gmp_test_params/high_3840_2160.txt"
};

static u32 gmp_lut_low_4096_3840[] = {
	#include "gmp_test_params/low_4096_3840.txt"
};

static u32 gmp_lut_high_4096_3840[] = {
	#include "gmp_test_params/high_4096_3840.txt"
};

static u32 gmp_lut_high_4096_4096[] = {
	#include "gmp_test_params/high_4096_4096.txt"
};

static u32 gmp_lut_low_4096_4096[] = {
	#include "gmp_test_params/low_4096_4096.txt"
};

static u32 gmp_lut_high_ui[] = {
	#include "gmp_test_params/high_ui.txt"
};
static u32 gmp_lut_low_ui[] = {
	#include "gmp_test_params/low_ui.txt"
};
static u32 gmp_lut_low_ui_enhance[] = {
	#include "gmp_test_params/low_ui_enhance.txt"
};

static u32 gmp_lut_high_ui_enhance[] = {
	#include "gmp_test_params/high_ui_enhance.txt"
};

static u32 gmp_lut_low_gallery_normal[] = {
	#include "gmp_test_params/low_gallery_normal.txt"
};

static u32 gmp_lut_high_gallery_normal[] = {
	#include "gmp_test_params/high_gallery_normal.txt"
};

static u32 gmp_lut_low_gallery_enhance[] = {
	#include "gmp_test_params/low_gallery_enhance.txt"
};

static u32 gmp_lut_high_gallery_enhance[] = {
	#include "gmp_test_params/high_gallery_enhance.txt"
};

static u32 gmp_lut_low_gallery_red[] = {
	#include "gmp_test_params/low_gallery_red.txt"
};

static u32 gmp_lut_high_gallery_red[] = {
	#include "gmp_test_params/high_gallery_red.txt"
};

static u32 gmp_lut_low_gallery_blue[] = {
	#include "gmp_test_params/low_gallery_blue.txt"
};

static u32 gmp_lut_high_gallery_blue[] = {
	#include "gmp_test_params/high_gallery_blue.txt"
};

static u32 gmp_lut_low_gallery_green[] = {
	#include "gmp_test_params/low_gallery_green.txt"
};

static u32 gmp_lut_high_gallery_green[] = {
	#include "gmp_test_params/high_gallery_green.txt"
};

static void set_normal_luts(char __iomem *addr, uint32_t low[], uint32_t high[])
{
	int i = 0;
	for (i = 0; i < GMP_COFE_CNT; i++) {
		dpu_set_reg(addr + i * 2 * 4, low[i], 32, 0);
		dpu_set_reg(addr + i * 2 * 4 + 4, high[i], 4, 0);
	}
}

static void set_user_luts(char __iomem *addr)
{
	int i = 0;
	for (i = 0; i < GMP_COFE_CNT; i++) {
		dpu_set_reg(addr + i * 2 * 4, hisi_fb_gmp_lut_low32, 32, 0);
		dpu_set_reg(addr + i * 2 * 4 + 4, hisi_fb_gmp_lut_high4, 4, 0);
	}
}

static void switch_resolution_luts(char __iomem *gmp_lut_base)
{
	if (hisi_fb_display_effect_test & BIT(10)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_720_480, gmp_lut_high_720_480);
	} else if (hisi_fb_display_effect_test & BIT(11)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_1080_1920, gmp_lut_high_1080_1920);
	} else if (hisi_fb_display_effect_test & BIT(12)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_3840_2160, gmp_lut_high_3840_2160);
	} else if (hisi_fb_display_effect_test & BIT(13)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_4096_3840, gmp_lut_high_4096_3840);
	} else if (hisi_fb_display_effect_test & BIT(14)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_3264_2448, gmp_lut_high_3264_2448);
	} else if (hisi_fb_display_effect_test & BIT(15)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_2008_1504, gmp_lut_high_2008_1504);
	} else if (hisi_fb_display_effect_test & BIT(16)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_1920_1080, gmp_lut_high_1920_1080);
	} else if (hisi_fb_display_effect_test & BIT(17)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_4096_4096, gmp_lut_high_4096_4096);
	}
}

void dpp_effect_gmp_test(char __iomem *dpu_base)
{
	char __iomem *gmp_lut_base = dpu_base + DPU_DPP_CH0_GMP_LUT_OFFSET;
	switch_resolution_luts(gmp_lut_base);
	if (hisi_fb_display_effect_test & BIT(6)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low, gmp_lut_high);
	} else if (hisi_fb_display_effect_test & BIT(7)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_srgb, gmp_lut_high_srgb);
	} else if (hisi_fb_display_effect_test & BIT(8)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_p3, gmp_lut_high_p3);
	} else if (hisi_fb_display_effect_test & BIT(9)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_srgb_p3, gmp_lut_high_srgb_p3);
	} else if (hisi_fb_display_effect_test & BIT(18)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_ui, gmp_lut_high_ui);
	} else if (hisi_fb_display_effect_test & BIT(19)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_ui_enhance, gmp_lut_high_ui_enhance);
	} else if (hisi_fb_display_effect_test & BIT(20)) {
		set_user_luts(gmp_lut_base);
	} else if (hisi_fb_display_effect_test & BIT(21)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_gallery_normal, gmp_lut_high_gallery_normal);
	} else if (hisi_fb_display_effect_test & BIT(22)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_gallery_enhance, gmp_lut_high_gallery_enhance);
	} else if (hisi_fb_display_effect_test & BIT(23)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_gallery_red, gmp_lut_high_gallery_red);
	} else if (hisi_fb_display_effect_test & BIT(24)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_gallery_blue, gmp_lut_high_gallery_blue);
	} else if (hisi_fb_display_effect_test & BIT(25)) {
		set_normal_luts(gmp_lut_base, gmp_lut_low_gallery_green, gmp_lut_high_gallery_green);
	}
}