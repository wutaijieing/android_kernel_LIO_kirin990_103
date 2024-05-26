/*
 * 350_507_12p6.c
 *
 * 350_507_12p6 lcd driver
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#include <securec.h>

#define _350_BARCODE_2D_LEN  23


static int _350_507_panel_sncode(struct ts_kit_ops *ts_ops)
{
	int ret;
	struct hisi_panel_info *pinfo = NULL;
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if (!hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	pinfo = &(hisifd->panel_info);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}

	if (common_info && common_info->sn_code.support) {
		ret = ts_ops->get_sn_code(pinfo->sn_code, _350_BARCODE_2D_LEN);
		if (ret) {
			LCD_KIT_ERR("get sn code failed on boot\n");
			return ret;
		}
		pinfo->sn_code_length = LCD_KIT_SN_CODE_LENGTH;
	}
	return LCD_KIT_OK;
}

static int _350_507_ddic_oem_info(char *oem_data,
	struct hisi_fb_data_type *hisifd, struct ts_kit_ops *ts_ops)
{
	int ret;
	char read_value[_350_BARCODE_2D_LEN + 1] = {0};

	if (disp_info->oeminfo.barcode_2d.support) {
		oem_data[0] = BARCODE_2D_TYPE;
		oem_data[1] = BARCODE_BLOCK_NUM;
		ret = ts_ops->get_sn_code(read_value, _350_BARCODE_2D_LEN);
		if (ret) {
			LCD_KIT_ERR("get sn code failed");
			return ret;
		}
		ret = strncat_s(oem_data, OEM_INFO_SIZE_MAX, read_value,
			OEM_INFO_SIZE_MAX - strlen(read_value) - 1);
		if (ret) {
			LCD_KIT_ERR("strncat failed\n");
			return ret;
		}
	}
	return LCD_KIT_OK;
}

static int _350_507_2d_code(char *oem_data,
	struct hisi_fb_data_type *hisifd)
{
	struct ts_kit_ops *ts_ops = NULL;

	ts_ops = ts_kit_get_ops();
	if (!ts_ops || !ts_ops->get_sn_code) {
		LCD_KIT_ERR("get_sn_code is null point\n");
		return LCD_KIT_FAIL;
	}
	if (!oem_data && !hisifd)
		return _350_507_panel_sncode(ts_ops);
	if (oem_data && hisifd)
		return _350_507_ddic_oem_info(oem_data, hisifd, ts_ops);
	LCD_KIT_ERR("invalid para\n");
	return LCD_KIT_FAIL;
}

static struct lcd_kit_panel_ops _350_507_panel_ops = {
	.lcd_get_2d_barcode = _350_507_2d_code,
};

int _350_507_probe(void)
{
	int ret;

	ret = lcd_kit_panel_ops_register(&_350_507_panel_ops);
	if (ret) {
		LCD_KIT_ERR("register lcd_kit_panel_ops failed\n");
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}
