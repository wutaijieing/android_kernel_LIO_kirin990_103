#include <linux/moduleparam.h>
#include "hisi_dpp_xcc_test.h"
#include "hisi_disp_config.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"

unsigned hisi_fb_xcc_00 = 0x0;
module_param_named(debug_xcc_00, hisi_fb_xcc_00, int, 0644);
MODULE_PARM_DESC(debug_xcc_00, "hisi_fb_xcc_00");

unsigned hisi_fb_xcc_01 = 0x8000;
module_param_named(debug_xcc_01, hisi_fb_xcc_01, int, 0644);
MODULE_PARM_DESC(debug_xcc_01, "hisi_fb_xcc_01");

unsigned hisi_fb_xcc_02 = 0x00;
module_param_named(debug_xcc_02, hisi_fb_xcc_02, int, 0644);
MODULE_PARM_DESC(debug_xcc_02, "hisi_fb_xcc_02");

unsigned hisi_fb_xcc_03 = 11;
module_param_named(debug_xcc_03, hisi_fb_xcc_03, int, 0644);
MODULE_PARM_DESC(debug_xcc_03, "hisi_fb_xcc_03");

unsigned hisi_fb_xcc_10 = 0x0;
module_param_named(debug_xcc_10, hisi_fb_xcc_10, int, 0644);
MODULE_PARM_DESC(debug_xcc_10, "hisi_fb_xcc_10");

unsigned hisi_fb_xcc_11 = 0x0;
module_param_named(debug_xcc_11, hisi_fb_xcc_11, int, 0644);
MODULE_PARM_DESC(debug_xcc_11, "hisi_fb_xcc_11");

unsigned hisi_fb_xcc_12 = 0x4000;
module_param_named(debug_xcc_12, hisi_fb_xcc_12, int, 0644);
MODULE_PARM_DESC(debug_xcc_12, "hisi_fb_xcc_12");

unsigned hisi_fb_xcc_13 = 11;
module_param_named(debug_xcc_13, hisi_fb_xcc_13, int, 0644);
MODULE_PARM_DESC(debug_xcc_13, "hisi_fb_xcc_13");

unsigned hisi_fb_xcc_20 = 0x0;
module_param_named(debug_xcc_20, hisi_fb_xcc_20, int, 0644);
MODULE_PARM_DESC(debug_xcc_20, "hisi_fb_xcc_20");

unsigned hisi_fb_xcc_21 = 0x0;
module_param_named(debug_xcc_21, hisi_fb_xcc_21, int, 0644);
MODULE_PARM_DESC(debug_xcc_21, "hisi_fb_xcc_21");

unsigned hisi_fb_xcc_22 = 0x0;
module_param_named(debug_xcc_22, hisi_fb_xcc_22, int, 0644);
MODULE_PARM_DESC(debug_xcc_22, "hisi_fb_xcc_22");

unsigned hisi_fb_xcc_23 = 0x8000;
module_param_named(debug_xcc_23, hisi_fb_xcc_23, int, 0644);
MODULE_PARM_DESC(debug_xcc_23, "hisi_fb_xcc_23");

unsigned hisi_xcc_roi_mode = 0;
module_param_named(debug_xcc_roi_mode, hisi_xcc_roi_mode, int, 0644);
MODULE_PARM_DESC(debug_xcc_roi_mode, "hisi_xcc_roi_mode");
static uint32_t xcc_table[12] = {
	0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000,
};

void dpp_effect_xcc_test(char __iomem *xcc_base) {
	int xcc_roi_start_x = 0;
	int xcc_roi_start_y = 0;
	int xcc_roi_start_end_x = 1000;
	int xcc_roi_start_end_y = 1000;
	int cnt;
	disp_pr_err("[effect] xcc_base %p, hisi_fb_display_effect_test 0x%x",
		xcc_base, hisi_fb_display_effect_test);

	disp_pr_err("[effect] hisi_fb_xcc_01 0x%x", hisi_fb_xcc_01);

	if (hisi_fb_display_effect_test & BIT(30)) {
		for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
			dpu_set_reg(xcc_base + XCC_COEF_00 + 0 * 4, hisi_fb_xcc_00, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 1 * 4, hisi_fb_xcc_01, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 2 * 4, hisi_fb_xcc_02, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 3 * 4, hisi_fb_xcc_03, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 4 * 4, hisi_fb_xcc_10, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 5 * 4, hisi_fb_xcc_11, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 6 * 4, hisi_fb_xcc_12, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 7 * 4, hisi_fb_xcc_13, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 8 * 4, hisi_fb_xcc_20, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 9 * 4, hisi_fb_xcc_21, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 10 * 4, hisi_fb_xcc_22, 17, 0);
			dpu_set_reg(xcc_base + XCC_COEF_00 + 11 * 4, hisi_fb_xcc_23, 17, 0);
		}
	} else {
		for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
			dpu_set_reg(xcc_base + XCC_COEF_00 + cnt * 4, xcc_table[cnt], 17, 0);
		}
	}
}