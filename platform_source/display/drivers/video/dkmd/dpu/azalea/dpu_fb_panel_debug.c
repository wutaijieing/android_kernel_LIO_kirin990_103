/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#include "dpu_fb_panel_debug.h"
#include "dpu_fb_adapt.h"

DBG_FUNC g_dpu_fb_panel_func;
static int g_parse_status = PARSE_HEAD;
static int g_cnt;
static uint32_t g_count;
struct mipi_phy_tab_info dpu_mpd_info;

static int init_panel_info(struct dpu_fb_data_type *dpufd,
	struct dpu_panel_info **pinfo)
{
	dpufd = dpufd_list[PRIMARY_PANEL_IDX];
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null\n");
		return DPU_FB_FAIL;
	}
	if (pinfo == NULL) {
		DPU_FB_ERR("pinfo is null\n");
		return DPU_FB_FAIL;
	}
	*pinfo = &dpufd->panel_info;
	if (*pinfo == NULL) {
		DPU_FB_ERR("*pinfo is null\n");
		return DPU_FB_FAIL;
	}
	return DPU_FB_OK;
}

static bool is_hex_valid_char(char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	if (ch >= 'a' && ch <= 'f')
		return true;
	if (ch >= 'A' && ch <= 'F')
		return true;
	return false;
}

static bool is_valid_char(char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	if (ch >= 'a' && ch <= 'z')
		return true;
	if (ch >= 'A' && ch <= 'Z')
		return true;
	return false;
}

static int str_to_del_invalid_ch(char *str)
{
	char *tmp = str;

	/* check param */
	if (!tmp)
		return -1;
	while (*str != '\0') {
		if (is_hex_valid_char(*str) || *str == ',' || *str == 'x' ||
			*str == 'X') {
			*tmp = *str;
			tmp++;
		}
		str++;
	}
	*tmp = '\0';
	return 0;
}

static int parse_u32_digit(char *in, unsigned int *out, int len)
{
	char *delim = ",";
	int i = 0;
	char *str1 = NULL;
	char *str2 = NULL;

	if (!in || !out) {
		DPU_FB_ERR("in or out is null\n");
		return DPU_FB_FAIL;
	}

	str_to_del_invalid_ch(in);
	str1 = in;
	do {
		str2 = strstr(str1, delim);
		if (i >= len) {
			DPU_FB_ERR("number is too much\n");
			return DPU_FB_FAIL;
		}
		if (str2 == NULL) {
			out[i++] = simple_strtoul(str1, NULL, 0);
			break;
		}
		*str2 = 0;
		out[i++] = simple_strtoul(str1, NULL, 0);
		str2++;
		str1 = str2;
	} while (str2 != NULL);
	return DPU_FB_OK;
}

static char hex_char_to_value(char *ch)
{
	switch (*ch) {
	case 'a' ... 'f':
		*ch = 10 + (*ch - 'a');
		break;
	case 'A' ... 'F':
		*ch = 10 + (*ch - 'A');
		break;
	case '0' ... '9':
		*ch = *ch - '0';
		break;
	default:
		break;
	}

	return *ch;
}

static bool dbg_panel_power_on(void)
{
	struct dpu_fb_data_type *dpufd = dpufd_list[PRIMARY_PANEL_IDX];
	bool panel_power_on = false;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null\n");
		return false;
	}
	down(&dpufd->blank_sem);
	panel_power_on = dpufd->panel_power_on;
	up(&dpufd->blank_sem);
	return panel_power_on;
}

static int dbg_dss_power_off_and_on(int val)
{
#define BACKLIGHT_DELAY 100
	struct dpu_fb_data_type *dpufd = dpufd_list[PRIMARY_PANEL_IDX];
	bool panel_power_on = dbg_panel_power_on();
	uint32_t bl_level_cur = 0;
	int ret;

	void_unused(val);

	/* lcd panel off */
	if (panel_power_on) {
		bl_level_cur = dpufd->bl_level;
		dpufb_set_backlight(dpufd, 0, false);
		ret = dpu_fb_blank_sub(FB_BLANK_POWERDOWN, dpufd->fbi);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, blank_mode %d failed!\n",
				dpufd->index, FB_BLANK_POWERDOWN);
			return DPU_FB_FAIL;
		}
	}

	/* lcd panel on */
	if (panel_power_on) {
		ret = dpu_fb_blank_sub(FB_BLANK_UNBLANK, dpufd->fbi);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, blank_mode %d failed!\n",
				dpufd->index, FB_BLANK_UNBLANK);
			return DPU_FB_FAIL;
		}

		dpu_fb_frame_refresh(dpufd, "dynamic_test");
		msleep(BACKLIGHT_DELAY);
		bl_level_cur = bl_level_cur ? bl_level_cur : dpufd->bl_level;
		dpufb_set_backlight(dpufd, bl_level_cur, false);
	}
	DPU_FB_INFO("dss power off and on success\n");
	return DPU_FB_OK;
}

static int mipi_phy_tx_parse_par(char *par, uint32_t* addr, uint32_t* value)
{
	char *p = par;
	dpu_check_and_return(!p, -EINVAL, ERR, "p is NULL\n");

	while ((*p != '\0') && (*p != ':'))
		p++;

	if (*p == ':') {
		*p = '\0';
		*addr = (uint32_t)simple_strtoul(par, NULL, 0);
		*value = (uint32_t)simple_strtoul((p + 1), NULL, 0);
		return DPU_FB_OK;
	}

	return DPU_FB_FAIL;
}

static int dpu_dbg_mipi_phy_tx_config(unsigned char *item, char *par)
{

	int ret;
	uint32_t addr;
	uint32_t value;

	void_unused(item);

	ret = mipi_phy_tx_parse_par(par, &addr, &value);
	if (ret != DPU_FB_OK) {
		DPU_FB_ERR("mipi_phy_tx_parse_par data %s fail\n", par);
		return ret;
	}
	DPU_FB_INFO("mipi tx config addr:value = 0x%X:0x%X", addr, value);

	if (dpu_mpd_info.config_count < DPU_MIPI_PHY_MAX_CFG_NUM) {
		dpu_mpd_info.mipi_phy_tab[dpu_mpd_info.config_count].addr = addr;
		dpu_mpd_info.mipi_phy_tab[dpu_mpd_info.config_count].value = value;
		dpu_mpd_info.config_count++;
	} else {
		DPU_FB_ERR("Exceeded the maximum(%u) value", DPU_MIPI_PHY_MAX_CFG_NUM);
		return DPU_FB_FAIL;
	}

	return DPU_FB_OK;
}

static int dpu_fb_dbg_dss_power_off_and_on(unsigned char *item, char *par)
{
	int value;
	int ret;

	void_unused(item);

	value = hex_char_to_value(par);
	DPU_FB_INFO("trigger dss power off and on\n");
	ret = dbg_dss_power_off_and_on(value);
	if (ret != DPU_FB_OK)
		DPU_FB_ERR("dss_power_off_and_on fail\n");

	return ret;
}

static int dpu_fb_dbg_set_panel_dbg_u8(unsigned char *item, char *par)
{
	uint32_t i;
	int value;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(dpufd, &pinfo);
	if (ret != DPU_FB_OK) {
		DPU_FB_ERR("init_panel_info fail\n");
		return ret;
	}

	struct dpu_fb_panel_dbg_u8 dpu_fb_panel_info_itm_u8[] = {
		{ "MipiLaneNums", &(pinfo->mipi.lane_nums) },
		{ "DirtyRegionSupport", &(pinfo->dirty_region_updt_support) },
		{ "MipiDsiUptSupport", &(pinfo->dsi_bit_clk_upt_support) },
		{ "DsiTimingSupport", &(pinfo->mipi.dsi_timing_support) },
		{ "MipiNonContinueEnable", &(pinfo->mipi.non_continue_en) },
	};

	value = hex_char_to_value(par);
	DPU_FB_INFO("value = %d\n", value);

	for (i = 0; i < ARRAY_SIZE(dpu_fb_panel_info_itm_u8); i++) {
		if (!strncmp(item, dpu_fb_panel_info_itm_u8[i].name, strlen(item))) {
			DPU_FB_INFO("found %s\n", item);
			*(dpu_fb_panel_info_itm_u8[i].addr) = value;
			return DPU_FB_OK;
		}
	}
	return DPU_FB_FAIL;
}

static int dpu_fb_dbg_set_panel_dbg_u32(unsigned char *item, char *par)
{
	uint32_t i;
	uint32_t value = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(dpufd, &pinfo);
	if (ret != DPU_FB_OK) {
		DPU_FB_ERR("init_panel_info fail\n");
		return ret;
	}

	struct dpu_fb_panel_dbg_u32 dpu_fb_panel_info_itm_u32[] = {
		{ "MipiDsiBitClk", &(pinfo->mipi.dsi_bit_clk_upt) },
		{ "CmdType", &(pinfo->type) },
		{ "VsynCtrType", &(pinfo->vsync_ctrl_type) },
		{ "HBackPorch", &(pinfo->ldi.h_back_porch) },
		{ "HFrontPorch", &(pinfo->ldi.h_front_porch) },
		{ "HPulseWidth", &(pinfo->ldi.h_pulse_width) },
		{ "VBackPorch", &(pinfo->ldi.v_back_porch) },
		{ "VFrontPorch", &(pinfo->ldi.v_front_porch) },
		{ "VPulseWidth", &(pinfo->ldi.v_pulse_width) },
		{ "MipiHsa", &(pinfo->mipi.hsa) },
		{ "MipiHbp", &(pinfo->mipi.hbp) },
		{ "MipiDpiHsize", &(pinfo->mipi.dpi_hsize) },
		{ "MipiHlineTime", &(pinfo->mipi.hline_time) },
		{ "MipiVsa", &(pinfo->mipi.vsa) },
		{ "MipiVbp", &(pinfo->mipi.vbp) },
		{ "MipiVfp", &(pinfo->mipi.vfp) },
		{ "MipiBurstMode", &(pinfo->mipi.burst_mode) },
		{ "MipiDsiBitClkValA", &(pinfo->mipi.dsi_bit_clk_val1) },
		{ "MipiDsiBitClkValB", &(pinfo->mipi.dsi_bit_clk_val2) },
		{ "MipiDsiBitClkValC", &(pinfo->mipi.dsi_bit_clk_val3) },
		{ "MipiDsiBitClkValD", &(pinfo->mipi.dsi_bit_clk_val4) },
		{ "MipiDsiBitClkValE", &(pinfo->mipi.dsi_bit_clk_val5) },
	};

	ret = parse_u32_digit(par, &value, 1);
	if (ret != DPU_FB_OK) {
		DPU_FB_ERR("parse u32 digit fail\n");
		return ret;
	}

	DPU_FB_INFO("value = %d\n", value);

	for (i = 0; i < ARRAY_SIZE(dpu_fb_panel_info_itm_u32); i++) {
		if (!strncmp(item, dpu_fb_panel_info_itm_u32[i].name, strlen(item))) {
			DPU_FB_INFO("found %s\n", item);
			*(dpu_fb_panel_info_itm_u32[i].addr) = value;
			return DPU_FB_OK;
		}
	}
	return DPU_FB_FAIL;
}

struct dpu_fb_panel_dbg_func dpu_fb_panel_item_func[] = {
	{ "DirtyRegionSupport", dpu_fb_dbg_set_panel_dbg_u8 },
	{ "CmdType", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "VsynCtrType", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "HBackPorch", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "HFrontPorch", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "HPulseWidth", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "VBackPorch", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "VFrontPorch", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "VPulseWidth", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "DsiTimingSupport", dpu_fb_dbg_set_panel_dbg_u8 },
	{ "MipiHsa", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiHbp", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDpiHsize", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiHlineTime", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiVsa", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiVbp", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiVfp", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiBurstMode", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDsiBitClk", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiLaneNums", dpu_fb_dbg_set_panel_dbg_u8 },
	{ "MipiDsiBitClkValA", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDsiBitClkValB", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDsiBitClkValC", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDsiBitClkValD", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDsiBitClkValE", dpu_fb_dbg_set_panel_dbg_u32 },
	{ "MipiDsiUptSupport", dpu_fb_dbg_set_panel_dbg_u8 },
	{ "MipiNonContinueEnable", dpu_fb_dbg_set_panel_dbg_u8 },
	{ "DssPowerOffAndOn", dpu_fb_dbg_dss_power_off_and_on },
	{ "MipiPhyTxCfg", dpu_dbg_mipi_phy_tx_config },
};

struct dpu_fb_panel_dbg_cmds dpu_fb_panel_cmd_list[] = {
	{ DPU_FB_PANEL_DBG_PARAM_CONFIG, "set_param_config" },
};

/* show usage or print last read result */
static char dpu_fb_panel_debug_buf[DPU_FB_PANEL_DBG_BUFF_MAX];

/* open function */
static int dpu_fb_panel_dbg_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

/* release function */
static int dpu_fb_panel_dbg_release(struct inode *inode, struct file *file)
{
	return 0;
}

/* read function */
static ssize_t dpu_fb_panel_dbg_read(struct file *file,  char __user *buff,
	size_t count, loff_t *ppos)
{
	int len;
	int ret_len;
	char *cur = NULL;
	int buf_len = sizeof(dpu_fb_panel_debug_buf);

	if (!file || !buff)
		return 0;

	cur = dpu_fb_panel_debug_buf;
	if (*ppos)
		return 0;
	/* show usage */
	len = snprintf(cur, buf_len, "Usage:\n");
	buf_len -= len;
	cur += len;

	len = snprintf(cur, buf_len,
		"\teg. echo set_param_config:1 > /sys/kernel/debug/panel_dbg/fb_panel_dbg to set parameter\n");
	buf_len -= len;
	cur += len;

	ret_len = sizeof(dpu_fb_panel_debug_buf) - buf_len;

	/* error happened */
	if (ret_len < 0)
		return 0;

	/* copy to user */
	if (copy_to_user(buff, dpu_fb_panel_debug_buf, ret_len))
		return -EFAULT;

	*ppos += ret_len; // increase offset
	return ret_len;
}

/* convert string to lower case */
/* return: 0 - success, negative - fail */
static int dpu_fb_str_to_lower(char *str)
{
	char *tmp = str;

	/* check param */
	if (!tmp)
		return -1;
	while (*tmp != '\0') {
		*tmp = tolower(*tmp);
		tmp++;
	}
	return 0;
}

/* check if string start with sub string */
/* return: 0 - success, negative - fail */
static int dpu_fb_str_start_with(const char *str, const char *sub)
{
	/* check param */
	if (!str || !sub)
		return -EINVAL;
	return ((strncmp(str, sub, strlen(sub)) == 0) ? 0 : -1);
}

int *dpu_fb_panel_dbg_find_item(unsigned char *item)
{
	uint32_t i = 0;

	for (i = 0; i < ARRAY_SIZE(dpu_fb_panel_item_func); i++) {
		if (!strncmp(item, dpu_fb_panel_item_func[i].name, strlen(item))) {
			DPU_FB_INFO("found %s\n", item);
			return (int *)dpu_fb_panel_item_func[i].func;
		}
	}
	DPU_FB_ERR("not found %s\n", item);
	return NULL;
}

/*
 * Func Name : right_angle_bra_pro
 * Description : get data from xml
 * Input : item_name - data label name
 *         tmp_name - temporary data label name
 *         data - variable
 * Return : DPU_FB_OK - read success
 *        : DPU_FB_FAIL - read fail or input parameters have null
 */
static int right_angle_bra_pro(unsigned char *item_name,
	unsigned char *tmp_name, unsigned char *data)
{
	int ret;

	if (!item_name || !tmp_name || !data) {
		DPU_FB_ERR("null pointer\n");
		return DPU_FB_FAIL;
	}
	if (g_parse_status == PARSE_HEAD) {
		g_dpu_fb_panel_func = (DBG_FUNC)dpu_fb_panel_dbg_find_item(item_name);
		g_cnt = 0;
		g_parse_status = RECEIVE_DATA;
	} else if (g_parse_status == PARSE_FINAL) {
		if (strncmp(item_name, tmp_name, strlen(item_name))) {
			DPU_FB_ERR("item head match final\n");
			return DPU_FB_FAIL;
		}
		if (g_dpu_fb_panel_func) {
			DPU_FB_INFO("data:%s\n", data);
			ret = g_dpu_fb_panel_func(item_name, data);
			if (ret)
				DPU_FB_ERR("func execute err:%d\n", ret);
		}
		/* parse new item start */
		g_parse_status = PARSE_HEAD;
		g_count = 0;
		g_cnt = 0;
		memset(data, 0, DPU_FB_PANEL_CONFIG_TABLE_MAX_NUM);
		memset(item_name, 0, DPU_FB_PANEL_ITEM_NAME_MAX);
		memset(tmp_name, 0, DPU_FB_PANEL_ITEM_NAME_MAX);
	}
	return DPU_FB_OK;
}

static int parse_ch(unsigned char *ch, unsigned char *data,
	unsigned char *item_name, unsigned char *tmp_name)
{
	int ret;

	switch (*ch) {
	case '<':
		if (g_parse_status == PARSE_HEAD)
			g_parse_status = PARSE_HEAD;
		return DPU_FB_OK;
	case '>':
		ret = right_angle_bra_pro(item_name, tmp_name, data);
		if (ret < 0) {
			DPU_FB_ERR("right_angle_bra_pro error\n");
			return DPU_FB_FAIL;
		}
		return DPU_FB_OK;
	case '/':
		if (g_parse_status == RECEIVE_DATA)
			g_parse_status = PARSE_FINAL;
		return DPU_FB_OK;
	default:
		if ((g_parse_status == PARSE_HEAD) && is_valid_char(*ch)) {
			if (g_cnt >= DPU_FB_PANEL_ITEM_NAME_MAX) {
				DPU_FB_ERR("item is too long\n");
				return DPU_FB_FAIL;
			}
			item_name[g_cnt++] = *ch;
			return DPU_FB_OK;
		}
		if ((g_parse_status == PARSE_FINAL) && is_valid_char(*ch)) {
			if (g_cnt >= DPU_FB_PANEL_ITEM_NAME_MAX) {
				DPU_FB_ERR("item is too long\n");
				return DPU_FB_FAIL;
			}
			tmp_name[g_cnt++] = *ch;
			return DPU_FB_OK;
		}
		if (g_parse_status == RECEIVE_DATA) {
			if (g_count >= DPU_FB_PANEL_CONFIG_TABLE_MAX_NUM) {
				DPU_FB_ERR("data is too long\n");
				return DPU_FB_FAIL;
			}
			data[g_count++] = *ch;
			return DPU_FB_OK;
		}
	}
	return DPU_FB_OK;
}

static int parse_fd(const int *fd, unsigned char *data,
	unsigned char *item_name, unsigned char *tmp_name)
{
	unsigned char ch = '\0';
	int ret;
	int cur_seek = 0;

	if (!fd || !data || !item_name || !tmp_name) {
		DPU_FB_ERR("null pointer\n");
		return DPU_FB_FAIL;
	}
	while (1) {
		if ((unsigned int)sys_read(*fd, &ch, 1) != 1) {
			DPU_FB_INFO("it's end of file\n");
			break;
		}
		cur_seek++;
		ret = sys_lseek(*fd, (off_t)cur_seek, 0);
		if (ret < 0)
			DPU_FB_ERR("sys_lseek error!\n");
		ret = parse_ch(&ch, data, item_name, tmp_name);
		if (ret < 0) {
			DPU_FB_ERR("parse_ch error!\n");
			return DPU_FB_FAIL;
		}
		continue;
	}
	return DPU_FB_OK;
}

int dpu_fb_panel_dbg_parse_config(void)
{
	unsigned char *item_name = NULL;
	unsigned char *tmp_name = NULL;
	unsigned char *data = NULL;
	int fd;
	mm_segment_t fs;
	int ret;

	fs = get_fs(); /* save previous value */
	set_fs(KERNEL_DS); /*lint !e501*/
	fd = sys_open((const char __force *) DPU_FB_PANEL_PARAM_FILE_PATH, O_RDONLY, 0);
	if (fd < 0) {
		DPU_FB_ERR("%s file doesn't exsit\n", DPU_FB_PANEL_PARAM_FILE_PATH);
		set_fs(fs);
		return DPU_FB_FAIL;
	}
	DPU_FB_INFO("Config file %s opened\n", DPU_FB_PANEL_PARAM_FILE_PATH);
	ret = sys_lseek(fd, (off_t)0, 0);
	if (ret < 0)
		DPU_FB_ERR("sys_lseek error!\n");
	data = kzalloc(DPU_FB_PANEL_CONFIG_TABLE_MAX_NUM, 0);
	if (!data) {
		sys_close(fd);
		set_fs(fs);
		DPU_FB_ERR("data kzalloc fail\n");
		return DPU_FB_FAIL;
	}
	item_name = kzalloc(DPU_FB_PANEL_ITEM_NAME_MAX, 0);
	if (!item_name) {
		kfree(data);
		data = NULL;
		sys_close(fd);
		set_fs(fs);
		DPU_FB_ERR("item_name kzalloc fail\n");
		return DPU_FB_FAIL;
	}
	tmp_name = kzalloc(DPU_FB_PANEL_ITEM_NAME_MAX, 0);
	if (!tmp_name) {
		kfree(data);
		kfree(item_name);
		data = NULL;
		item_name = NULL;
		sys_close(fd);
		set_fs(fs);
		DPU_FB_ERR("tmp_name kzalloc fail\n");
		return DPU_FB_FAIL;
	}
	ret = parse_fd(&fd, data, item_name, tmp_name);
	if (ret < 0) {
		kfree(data);
		kfree(item_name);
		kfree(tmp_name);
		data = NULL;
		item_name = NULL;
		tmp_name = NULL;
		sys_close(fd);
		set_fs(fs);
		DPU_FB_INFO("parse success\n");
		return DPU_FB_OK;
	}
	DPU_FB_INFO("parse fail\n");
	sys_close(fd);
	set_fs(fs);
	kfree(data);
	kfree(item_name);
	kfree(tmp_name);
	data = NULL;
	item_name = NULL;
	tmp_name = NULL;
	return DPU_FB_FAIL;
}

/* write function */
static ssize_t dpu_fb_panel_dbg_write(struct file *file, const char __user *buff,
	size_t count, loff_t *ppos)
{
#define BUF_LEN 256
	char *cur = NULL;
	int ret = 0;
	int cmd_type = -1;
	int cnt = 0;
	uint32_t i;
	int val;
	char lcd_debug_buf[BUF_LEN];

	if (!file || !buff)
		return -EFAULT;

	cur = lcd_debug_buf;
	if (count > (BUF_LEN - 1))
		return count;
	if (copy_from_user(lcd_debug_buf, buff, count))
		return -EFAULT;
	lcd_debug_buf[count] = 0;
	/* convert to lower case */
	if (dpu_fb_str_to_lower(cur) != 0)
		goto err_handle;
	DPU_FB_INFO("cur = %s, count = %d!\n", cur, (int)count);
	/* get cmd type */
	for (i = 0; i < ARRAY_SIZE(dpu_fb_panel_cmd_list); i++) {
		if (!dpu_fb_str_start_with(cur, dpu_fb_panel_cmd_list[i].pstr)) {
			cmd_type = dpu_fb_panel_cmd_list[i].type;
			cur += strlen(dpu_fb_panel_cmd_list[i].pstr);
			break;
		}
		DPU_FB_INFO("dpu_fb_panel_cmd_list[%d].pstr=%s\n", i,
			dpu_fb_panel_cmd_list[i].pstr);
	}
	if (i >= ARRAY_SIZE(dpu_fb_panel_cmd_list)) {
		DPU_FB_ERR("cmd type not find!\n"); // not support
		goto err_handle;
	}
	switch (cmd_type) {
	case DPU_FB_PANEL_DBG_PARAM_CONFIG:
		cnt = sscanf(cur, ":%d", &val);
		if (cnt != 1) {
			DPU_FB_ERR("get param fail!\n");
			goto err_handle;
		}
		if (val == 1) {
			ret = dpu_fb_panel_dbg_parse_config();
			if (!ret)
				DPU_FB_INFO("parse parameter succ!\n");
		}
		break;
	default:
		DPU_FB_ERR("cmd type not support!\n"); // not support
		ret = DPU_FB_FAIL;
		break;
	}
	/* finish */
	if (ret) {
		DPU_FB_INFO("fail\n");
		goto err_handle;
	} else {
		return count;
	}

err_handle:
	return -EFAULT;
}

static const struct file_operations dpu_fb_panel_debug_fops = {
	.open = dpu_fb_panel_dbg_open,
	.release = dpu_fb_panel_dbg_release,
	.read = dpu_fb_panel_dbg_read,
	.write = dpu_fb_panel_dbg_write,
};

/* init panel debugfs interface */
int dpu_fb_panel_debugfs_init(void)
{
	static char already_init;
	struct dentry *dent = NULL;
	struct dentry *file = NULL;

	/* init mipi dsi */
	memset (&dpu_mpd_info, 0x00, sizeof(struct mipi_phy_tab_info));

	/* judge if already init */
	if (already_init) {
		DPU_FB_INFO("already init\n");
		return DPU_FB_OK;
	}
	/* create dir */
	dent = debugfs_create_dir("panel_dbg", NULL);
	if (IS_ERR_OR_NULL(dent)) {
		DPU_FB_ERR("create_dir panel_dbg fail, error %ld\n", PTR_ERR(dent));
		goto err_create_dir;
	}
	/* create reg_dbg_mipi node */
	file = debugfs_create_file("fb_panel_dbg", 0644, dent, 0,
		&dpu_fb_panel_debug_fops);
	if (IS_ERR_OR_NULL(file)) {
		DPU_FB_ERR("create_file: fb_panel_dbg fail\n");
		goto err_create_mipi;
	}
	already_init = 1;
	return DPU_FB_OK;
err_create_mipi:
	debugfs_remove_recursive(dent);
err_create_dir:
	return DPU_FB_FAIL;
}

void dpu_mipi_phy_debug_config(void)
{
	int index;
	int count = dpu_mpd_info.config_count;
	struct dpu_fb_data_type *dpufd = dpufd_list[PRIMARY_PANEL_IDX];
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null\n");
		return;
	}

	if ((count == 0) || (g_debug_mipi_phy_config == 0))
		return;

	DPU_FB_INFO("dpu mipi phy debug config count %u", count);

	for (index = 0; index < count; index++)
	{
		DPU_FB_INFO("set reg phy addr 0x%X value 0x%X",
			dpu_mpd_info.mipi_phy_tab[index].addr,
			dpu_mpd_info.mipi_phy_tab[index].value);

		mipi_config_phy_test_code(dpufd->mipi_dsi0_base,
			MIPI_PHY_TEST_START_OFFSET + dpu_mpd_info.mipi_phy_tab[index].addr,
			dpu_mpd_info.mipi_phy_tab[index].value);
	}

	memset (&dpu_mpd_info, 0x00, sizeof(struct mipi_phy_tab_info));
	g_debug_mipi_phy_config = 0;
}

#pragma GCC diagnostic pop

