/*
 * lcd_kit_debug.c
 *
 * lcdkit debug function for lcdkit head file
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
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

#include "lcd_kit_dbg.h"
#include <linux/string.h>
#include "lcd_kit_common.h"
#include <base.h>

#define VALUE_MAX 3

static int lcd_kit_dbg_cmd(char *par);
static int lcd_kit_dbg_cmdstate(char *par);

struct lcd_kit_dbg_func item_func[] = {
	/* send mipi cmds for debugging, both support tx and rx */
	{ "PanelDbgCommand", lcd_kit_dbg_cmd },
	{ "PanelDbgCommandState", lcd_kit_dbg_cmdstate },
};

struct lcd_kit_dbg_cmds lcd_kit_cmd_list[] = {
	{ LCD_KIT_DBG_LEVEL_SET, "set_debug_level" },
	{ LCD_KIT_DBG_PARAM_CONFIG, "set_param_config" },
};

struct lcd_kit_debug lcd_kit_dbg;
/* show usage or print last read result */
static char lcd_kit_debug_buf[LCD_KIT_DBG_BUFF_MAX];

static struct lcd_kit_dbg_ops *g_dbg_ops;

int lcd_kit_debug_register(struct lcd_kit_dbg_ops *ops)
{
	if (g_dbg_ops) {
		LCD_KIT_ERR("g_dbg_ops has already been registered!\n");
		return LCD_KIT_FAIL;
	}
	g_dbg_ops = ops;
	LCD_KIT_INFO("g_dbg_ops register success!\n");
	return LCD_KIT_OK;
}

int lcd_kit_debug_unregister(struct lcd_kit_dbg_ops *ops)
{
	if (g_dbg_ops == ops) {
		g_dbg_ops = NULL;
		LCD_KIT_INFO("g_dbg_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_dbg_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

bool lcd_kit_is_valid_char(char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	if (ch >= 'a' && ch <= 'f')
		return true;
	if (ch >= 'A' && ch <= 'F')
		return true;
	return false;
}

bool is_valid_char(char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	if (ch >= 'a' && ch <= 'z')
		return true;
	if (ch >= 'A' && ch <= 'Z')
		return true;
	return false;
}

struct lcd_kit_dbg_ops *lcd_kit_get_debug_ops(void)
{
	return g_dbg_ops;
}

static char lcd_kit_hex_char_to_value(char ch)
{
	switch (ch) {
	case 'a' ... 'f':
		ch = 10 + (ch - 'a');
		break;
	case 'A' ... 'F':
		ch = 10 + (ch - 'A');
		break;
	case '0' ... '9':
		ch = ch - '0';
		break;
	}

	return ch;
}

void lcd_kit_dump_buf(const char *buf, int cnt)
{
	int i;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return;
	}
	for (i = 0; i < cnt; i++)
		LCD_KIT_DEBUG("buf[%d] = 0x%02x\n", i, buf[i]);
}

void lcd_kit_dump_buf_32(const u32 *buf, int cnt)
{
	int i = 0;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return;
	}
	for (i = 0; i < cnt; i++)
		LCD_KIT_DEBUG("buf[%d] = 0x%02x\n", i, buf[i]);

}

void lcd_kit_dump_cmds_desc(struct lcd_kit_dsi_cmd_desc *desc)
{
	if (!desc) {
		LCD_KIT_INFO("NULL point!\n");
		return;
	}
	LCD_KIT_DEBUG("dtype      = 0x%02x\n", desc->dtype);
	LCD_KIT_DEBUG("last       = 0x%02x\n", desc->last);
	LCD_KIT_DEBUG("vc         = 0x%02x\n", desc->vc);
	LCD_KIT_DEBUG("ack        = 0x%02x\n", desc->ack);
	LCD_KIT_DEBUG("wait       = 0x%02x\n", desc->wait);
	LCD_KIT_DEBUG("waittype   = 0x%02x\n", desc->waittype);
	LCD_KIT_DEBUG("dlen       = 0x%02x\n", desc->dlen);

	lcd_kit_dump_buf(desc->payload, (int)(desc->dlen));
}

void lcd_kit_dump_cmds(struct lcd_kit_dsi_panel_cmds *cmds)
{
	int i;

	if (!cmds) {
		LCD_KIT_INFO("NULL point!\n");
		return;
	}
	LCD_KIT_DEBUG("blen       = 0x%02x\n", cmds->blen);
	LCD_KIT_DEBUG("cmd_cnt    = 0x%02x\n", cmds->cmd_cnt);
	LCD_KIT_DEBUG("link_state = 0x%02x\n", cmds->link_state);
	LCD_KIT_DEBUG("flags      = 0x%02x\n", cmds->flags);
	for (i = 0; i < cmds->cmd_cnt; i++)
		lcd_kit_dump_cmds_desc(&cmds->cmds[i]);
}

/* convert string to lower case */
/* return: 0 - success, negative - fail */
static int lcd_kit_str_to_lower(char *str)
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
static int lcd_kit_str_start_with(const char *str, const char *sub)
{
	/* check param */
	if (!str || !sub)
		return -EINVAL;
	return ((strncmp(str, sub, strlen(sub)) == 0) ? 0 : -1);
}

int lcd_kit_str_to_del_invalid_ch(char *str)
{
	char *tmp = str;

	/* check param */
	if (!tmp)
		return -1;
	while (*str != '\0') {
		if (lcd_kit_is_valid_char(*str) || *str == ',' || *str == 'x' ||
			*str == 'X') {
			*tmp = *str;
			tmp++;
		}
		str++;
	}
	*tmp = '\0';
	return 0;
}

int lcd_kit_str_to_del_ch(char *str, char ch)
{
	char *tmp = str;

	/* check param */
	if (!tmp)
		return -1;
	while (*str != '\0') {
		if (*str != ch) {
			*tmp = *str;
			tmp++;
		}
		str++;
	}
	*tmp = '\0';
	return 0;
}

/* parse config xml */
int lcd_kit_parse_u8_digit(char *in, char *out, int max)
{
	unsigned char ch = '\0';
	unsigned char last_char = 'Z';
	unsigned char last_ch = 'Z';
	int j = 0;
	int i = 0;
	int len;

	if (!in || !out) {
		LCD_KIT_ERR("in or out is null\n");
		return LCD_KIT_FAIL;
	}
	len = strlen(in);
	LCD_KIT_INFO("LEN = %d\n", len);
	while (len--) {
		ch = in[i++];
		if (last_ch == '0' && ((ch == 'x') || (ch == 'X'))) {
			j--;
			last_char = 'Z';
			continue;
		}
		last_ch = ch;
		if (!lcd_kit_is_valid_char(ch)) {
			last_char = 'Z';
			continue;
		}
		if (last_char != 'Z') {
			/*
			 * two char value is possible like F0,
			 * so make it a single char
			 */
			--j;
			if (j >= max) {
				LCD_KIT_ERR("number is too much\n");
				return LCD_KIT_FAIL;
			}
			out[j] = (out[j] * LCD_KIT_HEX_BASE) +
				lcd_kit_hex_char_to_value(ch);
			last_char = 'Z';
		} else {
			if (j >= max) {
				LCD_KIT_ERR("number is too much\n");
				return LCD_KIT_FAIL;
			}
			out[j] = lcd_kit_hex_char_to_value(ch);
			last_char = out[j];
		}
		j++;
	}
	return j;
}

int lcd_kit_parse_u32_digit(char *in, unsigned int *out, int len)
{
	char *delim = ",";
	int i = 0;
	char *str1 = NULL;
	char *str2 = NULL;

	if (!in || !out) {
		LCD_KIT_ERR("in or out is null\n");
		return LCD_KIT_FAIL;
	}

	lcd_kit_str_to_del_invalid_ch(in);
	str1 = in;
	do {
		str2 = strstr(str1, delim);
		if (i >= len) {
			LCD_KIT_ERR("number is too much\n");
			return LCD_KIT_FAIL;
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
	return i;
}

int lcd_kit_dbg_parse_array(char *in, unsigned int *array,
	struct lcd_kit_arrays_data *out, int len)
{
	char *delim = "\n";
	int count = 0;
	char *str1 = NULL;
	char *str2 = NULL;
	unsigned int *temp = NULL;
	struct lcd_kit_array_data *tmp = NULL;

	if (!in || !array || !out) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	temp = array;
	str1 = in;
	tmp = out->arry_data;
	if (!temp || !str1 || !tmp) {
		LCD_KIT_ERR("temp or str1 or tmp is null\n");
		return LCD_KIT_FAIL;
	}
	do {
		str2 = strstr(str1, delim);
		if (str2 == NULL) {
			lcd_kit_parse_u32_digit(str1, temp, len);
			tmp->buf = temp;
			tmp++;
			temp += len;
			count++;
			break;
		}
		*str2 = 0;
		lcd_kit_parse_u32_digit(str1, temp, len);
		tmp->buf = temp;
		tmp++;
		temp += len;
		count++;
		str2++;
		str1 = str2;
	} while (str2 != NULL);
	out->cnt = count;
	LCD_KIT_INFO("out->cnt = %d\n", out->cnt);
	return count;
}

int lcd_kit_dbg_parse_cmd(struct lcd_kit_dsi_panel_cmds *pcmds, char *buf,
	int length)
{
	int blen;
	int len;
	char *bp = NULL;
	struct lcd_kit_dsi_ctrl_hdr *dchdr = NULL;
	struct lcd_kit_dsi_cmd_desc *newcmds = NULL;
	int i;
	int cnt = 0;

	if (!pcmds || !buf) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	/* scan dcs commands */
	bp = buf;
	blen = length;
	len = blen;
	while (len > sizeof(*dchdr)) {
		dchdr = (struct lcd_kit_dsi_ctrl_hdr *)bp;
		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		if (dchdr->dlen > len) {
			LCD_KIT_ERR("dtsi cmd=%x error, len=%d, cnt=%d\n",
				dchdr->dtype, dchdr->dlen, cnt);
			return LCD_KIT_FAIL;
		}
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}
	if (len != 0) {
		LCD_KIT_ERR("dcs_cmd=%x len=%d error!\n", buf[0], blen);
		return LCD_KIT_FAIL;
	}
	newcmds = kzalloc(cnt * sizeof(*newcmds), GFP_KERNEL);
	if (newcmds == NULL) {
		LCD_KIT_ERR("kzalloc fail\n");
		return LCD_KIT_FAIL;
	}
	if (pcmds->cmds != NULL)
		kfree(pcmds->cmds);
	pcmds->cmds = newcmds;
	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;
	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct lcd_kit_dsi_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dtype = dchdr->dtype;
		pcmds->cmds[i].last = dchdr->last;
		pcmds->cmds[i].vc = dchdr->vc;
		pcmds->cmds[i].ack = dchdr->ack;
		pcmds->cmds[i].wait = dchdr->wait;
		pcmds->cmds[i].waittype = dchdr->waittype;
		pcmds->cmds[i].dlen = dchdr->dlen;
		pcmds->cmds[i].payload = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}
	lcd_kit_dump_cmds(pcmds);
	return 0;
}

void lcd_kit_dbg_free(void)
{
	kfree(lcd_kit_dbg.dbg_esd_cmds);
	kfree(lcd_kit_dbg.dbg_on_cmds);
	kfree(lcd_kit_dbg.dbg_off_cmds);
	kfree(lcd_kit_dbg.dbg_effect_on_cmds);
	kfree(lcd_kit_dbg.dbg_cabc_off_cmds);
	kfree(lcd_kit_dbg.dbg_cabc_ui_cmds);
	kfree(lcd_kit_dbg.dbg_cabc_still_cmds);
	kfree(lcd_kit_dbg.dbg_cabc_moving_cmds);
	kfree(lcd_kit_dbg.dbg_rgbw_mode1_cmds);
	kfree(lcd_kit_dbg.dbg_rgbw_mode2_cmds);
	kfree(lcd_kit_dbg.dbg_rgbw_mode3_cmds);
	kfree(lcd_kit_dbg.dbg_rgbw_mode4_cmds);
	kfree(lcd_kit_dbg.dbg_rgbw_backlight_cmds);
	kfree(lcd_kit_dbg.dbg_rgbw_pixel_gainlimit_cmds);
	kfree(lcd_kit_dbg.dbg_dirty_region_cmds);
	kfree(lcd_kit_dbg.dbg_barcode_2d_cmds);
	kfree(lcd_kit_dbg.dbg_brightness_color_cmds);
	kfree(lcd_kit_dbg.dbg_power_on_array);
}

static struct lcd_kit_dsi_panel_cmds dbgcmds;
static int lcd_kit_dbg_cmd(char *par)
{
#define LCD_DDIC_INFO_LEN 200
#define PRI_LINE_LEN 8
	struct lcd_kit_dbg_ops *dbg_ops = NULL;
	uint8_t readbuf[LCD_DDIC_INFO_LEN] = {0};
	int len, i;

	dbgcmds.buf = NULL;
	dbgcmds.blen = 0;
	dbgcmds.cmds = NULL;
	dbgcmds.cmd_cnt = 0;
	dbgcmds.flags = 0;
	dbg_ops = lcd_kit_get_debug_ops();
	if (!dbg_ops) {
		LCD_KIT_ERR("dbg_ops is null!\n");
		return LCD_KIT_FAIL;
	}
	if (dbg_ops->panel_power_on) {
		if (!dbg_ops->panel_power_on()) {
			LCD_KIT_ERR("panel power off!\n");
			return LCD_KIT_FAIL;
		}
	} else {
		LCD_KIT_ERR("panel_power_on is null!\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg.dbg_cmds = kzalloc(LCD_KIT_CONFIG_TABLE_MAX_NUM, 0);
	if (!lcd_kit_dbg.dbg_cmds) {
		LCD_KIT_ERR("kzalloc fail\n");
		return LCD_KIT_FAIL;
	}
	len = lcd_kit_parse_u8_digit(par, lcd_kit_dbg.dbg_cmds,
		LCD_KIT_CONFIG_TABLE_MAX_NUM);
	if (len > 0)
		lcd_kit_dbg_parse_cmd(&dbgcmds, lcd_kit_dbg.dbg_cmds, len);
	if (dbg_ops->dbg_mipi_rx) {
		dbg_ops->dbg_mipi_rx(readbuf, LCD_DDIC_INFO_LEN, &dbgcmds);
		readbuf[LCD_DDIC_INFO_LEN - 1] = '\0';
		LCD_KIT_INFO("dbg-cmd read string:%s\n", readbuf);
		LCD_KIT_INFO("corresponding hex data:\n");
		for (i = 0; i < LCD_DDIC_INFO_LEN; i++) {
			LCD_KIT_INFO("0x%x", readbuf[i]);
			if ((i + 1) % PRI_LINE_LEN == 0)
				LCD_KIT_INFO("\n");
		}
		LCD_KIT_INFO("dbg_mipi_rx done\n");
		kfree(lcd_kit_dbg.dbg_cmds);
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("dbg_mipi_rx is NULL!\n");
	kfree(lcd_kit_dbg.dbg_cmds);
	return LCD_KIT_FAIL;
}

static int lcd_kit_dbg_cmdstate(char *par)
{
	if (!par) {
		LCD_KIT_ERR("par is null\n");
		return LCD_KIT_FAIL;
	}
	dbgcmds.link_state = lcd_kit_hex_char_to_value(*par);
	LCD_KIT_INFO("dbgcmds link_state = %d\n", dbgcmds.link_state);
	return LCD_KIT_OK;
}

static void lcd_kit_dbg_set_dbg_level(int level)
{
	lcd_kit_msg_level = level;
}

int *lcd_kit_dbg_find_item(unsigned char *item)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(item_func); i++) {
		if (!strncmp(item, item_func[i].name, strlen(item))) {
			LCD_KIT_INFO("found %s\n", item);
			return (int *)item_func[i].func;
		}
	}
	LCD_KIT_ERR("not found %s\n", item);
	return NULL;
}

DBG_FUNC func;
static int parse_status = PARSE_HEAD;
static int cnt;
static int count;

static int right_angle_bra_pro(unsigned char *item_name,
	unsigned char *tmp_name, unsigned char *data)
{
	int ret;

	if (!item_name || !tmp_name || !data) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	if (parse_status == PARSE_HEAD) {
		func = (DBG_FUNC)lcd_kit_dbg_find_item(item_name);
		cnt = 0;
		parse_status = RECEIVE_DATA;
	} else if (parse_status == PARSE_FINAL) {
		if (strncmp(item_name, tmp_name, strlen(item_name))) {
			LCD_KIT_ERR("item head match final\n");
			return LCD_KIT_FAIL;
		}
		if (func) {
			LCD_KIT_INFO("data:%s\n", data);
			ret = func(data);
			if (ret)
				LCD_KIT_ERR("func execute err:%d\n", ret);
		}
		/* parse new item start */
		parse_status = PARSE_HEAD;
		count = 0;
		cnt = 0;
		memset(data, 0, LCD_KIT_CONFIG_TABLE_MAX_NUM);
		memset(item_name, 0, LCD_KIT_ITEM_NAME_MAX);
		memset(tmp_name, 0, LCD_KIT_ITEM_NAME_MAX);
	}
	return LCD_KIT_OK;
}

static int parse_ch(unsigned char *ch, unsigned char *data,
	unsigned char *item_name, unsigned char *tmp_name)
{
	int ret;

	switch (*ch) {
	case '<':
		if (parse_status == PARSE_HEAD)
			parse_status = PARSE_HEAD;
		return LCD_KIT_OK;
	case '>':
		ret = right_angle_bra_pro(item_name, tmp_name, data);
		if (ret < 0) {
			LCD_KIT_ERR("right_angle_bra_pro error\n");
			return LCD_KIT_FAIL;
		}
		return LCD_KIT_OK;
	case '/':
		if (parse_status == RECEIVE_DATA)
			parse_status = PARSE_FINAL;
		return LCD_KIT_OK;
	default:
		if (parse_status == PARSE_HEAD && is_valid_char(*ch)) {
			if (cnt >= LCD_KIT_ITEM_NAME_MAX) {
				LCD_KIT_ERR("item is too long\n");
				return LCD_KIT_FAIL;
			}
			item_name[cnt++] = *ch;
			return LCD_KIT_OK;
		}
		if (parse_status == PARSE_FINAL && is_valid_char(*ch)) {
			if (cnt >= LCD_KIT_ITEM_NAME_MAX) {
				LCD_KIT_ERR("item is too long\n");
				return LCD_KIT_FAIL;
			}
			tmp_name[cnt++] = *ch;
			return LCD_KIT_OK;
		}
		if (parse_status == RECEIVE_DATA) {
			if (count >= LCD_KIT_CONFIG_TABLE_MAX_NUM) {
				LCD_KIT_ERR("data is too long\n");
				return LCD_KIT_FAIL;
			}
			data[count++] = *ch;
			return LCD_KIT_OK;
		}
	}
	return LCD_KIT_OK;
}

static int parse_fd(const int *fd, unsigned char *data,
	unsigned char *item_name, unsigned char *tmp_name)
{
	unsigned char ch = '\0';
	int ret;
	int cur_seek = 0;

	if (!fd || !data || !item_name || !tmp_name) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	while (1) {
		if ((unsigned int)ksys_read(*fd, &ch, 1) != 1) {
			LCD_KIT_INFO("it's end of file\n");
			break;
		}
		cur_seek++;
		ret = ksys_lseek(*fd, (off_t)cur_seek, 0);
		if (ret < 0)
			LCD_KIT_ERR("sys_lseek error!\n");
		ret = parse_ch(&ch, data, item_name, tmp_name);
		if (ret < 0) {
			LCD_KIT_ERR("parse_ch error!\n");
			return LCD_KIT_FAIL;
		}
		continue;
	}
	return LCD_KIT_OK;
}

int lcd_kit_dbg_parse_config(void)
{
	unsigned char *item_name = NULL;
	unsigned char *tmp_name = NULL;
	unsigned char *data = NULL;
	int fd;
	mm_segment_t fs;
	int ret;

	fs = get_fs(); /* save previous value */
	set_fs(KERNEL_DS); /* use kernel limit */
	fd = ksys_open((const char __force *) LCD_KIT_PARAM_FILE_PATH, O_RDONLY, 0);
	if (fd < 0) {
		LCD_KIT_ERR("%s file doesn't exsit\n", LCD_KIT_PARAM_FILE_PATH);
		set_fs(fs);
		return FALSE;
	}
	LCD_KIT_INFO("Config file %s opened\n", LCD_KIT_PARAM_FILE_PATH);
	ret = ksys_lseek(fd, (off_t)0, 0);
	if (ret < 0)
		LCD_KIT_ERR("sys_lseek error!\n");
	data = kzalloc(LCD_KIT_CONFIG_TABLE_MAX_NUM, 0);
	if (!data) {
		LCD_KIT_ERR("data kzalloc fail\n");
		return LCD_KIT_FAIL;
	}
	item_name = kzalloc(LCD_KIT_ITEM_NAME_MAX, 0);
	if (!item_name) {
		kfree(data);
		LCD_KIT_ERR("item_name kzalloc fail\n");
		return LCD_KIT_FAIL;
	}
	tmp_name = kzalloc(LCD_KIT_ITEM_NAME_MAX, 0);
	if (!tmp_name) {
		kfree(data);
		kfree(item_name);
		LCD_KIT_ERR("tmp_name kzalloc fail\n");
		return LCD_KIT_FAIL;
	}
	ret = parse_fd(&fd, data, item_name, tmp_name);
	if (ret < 0) {
		LCD_KIT_INFO("parse fail\n");
		ksys_close(fd);
		set_fs(fs);
		kfree(data);
		kfree(item_name);
		kfree(tmp_name);
		return LCD_KIT_FAIL;
	}
	LCD_KIT_INFO("parse success\n");
	ksys_close(fd);
	set_fs(fs);
	kfree(data);
	kfree(item_name);
	kfree(tmp_name);
	return LCD_KIT_OK;
}

/* open function */
static int lcd_kit_dbg_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

/* release function */
static int lcd_kit_dbg_release(struct inode *inode, struct file *file)
{
	return 0;
}

/* read function */
static ssize_t lcd_kit_dbg_read(struct file *file,  char __user *buff,
	size_t count, loff_t *ppos)
{
	int len;
	int ret_len;
	char *cur = NULL;
	int buf_len = sizeof(lcd_kit_debug_buf);

	cur = lcd_kit_debug_buf;
	if (*ppos)
		return 0;
	/* show usage */
	len = snprintf(cur, buf_len, "Usage:\n");
	buf_len -= len;
	cur += len;

	len = snprintf(cur, buf_len, "\teg. echo set_debug_level:4 > /sys/kernel/debug/lcd-dbg/lcd_kit_dbg to open set debug level\n");
	buf_len -= len;
	cur += len;

	len = snprintf(cur, buf_len, "\teg. echo set_param_config:1 > /sys/kernel/debug/lcd-dbg/lcd_kit_dbg to set parameter\n");
	buf_len -= len;
	cur += len;

	ret_len = sizeof(lcd_kit_debug_buf) - buf_len;

	// error happened!
	if (ret_len < 0)
		return 0;

	/* copy to user */
	if (copy_to_user(buff, lcd_kit_debug_buf, ret_len))
		return -EFAULT;

	*ppos += ret_len; // increase offset
	return ret_len;
}

/* write function */
static ssize_t lcd_kit_dbg_write(struct file *file, const char __user *buff,
	size_t count, loff_t *ppos)
{
#define BUF_LEN 256
	char *cur = NULL;
	int ret = 0;
	int cmd_type = -1;
	int cnt = 0;
	int i;
	int val;
	char lcd_debug_buf[BUF_LEN];

	cur = lcd_debug_buf;
	if (count > (BUF_LEN - 1))
		return count;
	if (copy_from_user(lcd_debug_buf, buff, count))
		return -EFAULT;
	lcd_debug_buf[count] = 0;
	/* convert to lower case */
	if (lcd_kit_str_to_lower(cur) != 0)
		goto err_handle;
	LCD_KIT_DEBUG("cur=%s, count=%d!\n", cur, (int)count);
	/* get cmd type */
	for (i = 0; i < ARRAY_SIZE(lcd_kit_cmd_list); i++) {
		if (!lcd_kit_str_start_with(cur, lcd_kit_cmd_list[i].pstr)) {
			cmd_type = lcd_kit_cmd_list[i].type;
			cur += strlen(lcd_kit_cmd_list[i].pstr);
			break;
		}
		LCD_KIT_DEBUG("lcd_kit_cmd_list[%d].pstr=%s\n", i,
			lcd_kit_cmd_list[i].pstr);
	}
	if (i >= ARRAY_SIZE(lcd_kit_cmd_list)) {
		LCD_KIT_ERR("cmd type not find!\n");  // not support
		goto err_handle;
	}
	switch (cmd_type) {
	case LCD_KIT_DBG_LEVEL_SET:
		cnt = sscanf(cur, ":%d", &val);
		if (cnt != 1) {
			LCD_KIT_ERR("get param fail!\n");
			goto err_handle;
		}
		lcd_kit_dbg_set_dbg_level(val);
		break;
	case LCD_KIT_DBG_PARAM_CONFIG:
		cnt = sscanf(cur, ":%d", &val);
		if (cnt != 1) {
			LCD_KIT_ERR("get param fail!\n");
			goto err_handle;
		}
		lcd_kit_dbg_free();
		if (val == 1) {
			ret = lcd_kit_dbg_parse_config();
			if (!ret)
				LCD_KIT_INFO("parse parameter succ!\n");
		}
		break;
	default:
		LCD_KIT_ERR("cmd type not support!\n"); // not support
		ret = LCD_KIT_FAIL;
		break;
	}
	/* finish */
	if (ret) {
		LCD_KIT_ERR("fail\n");
		goto err_handle;
	} else {
		return count;
	}

err_handle:
	return -EFAULT;
}

static const struct file_operations lcd_kit_debug_fops = {
	.open = lcd_kit_dbg_open,
	.release = lcd_kit_dbg_release,
	.read = lcd_kit_dbg_read,
	.write = lcd_kit_dbg_write,
};

/* init lcd debugfs interface */
int lcd_kit_debugfs_init(void)
{
	static char already_init;  // internal flag
	struct dentry *dent = NULL;
	struct dentry *file = NULL;

	/* judge if already init */
	LCD_KIT_ERR("enter lcd_kit_debugfs_init");
	if (already_init) {
		LCD_KIT_ERR("(%d): already init\n", __LINE__);
		return LCD_KIT_OK;
	}
	/* create dir */
	dent = debugfs_create_dir("lcd-dbg", NULL);
	if (IS_ERR_OR_NULL(dent)) {
		LCD_KIT_ERR("(%d):create_dir fail, error %ld\n", __LINE__,
			PTR_ERR(dent));
		dent = NULL;
		goto err_create_dir;
	}
	/* create reg_dbg_mipi node */
	file = debugfs_create_file("lcd_kit_dbg", 0644, dent, 0,
		&lcd_kit_debug_fops);
	if (IS_ERR_OR_NULL(file)) {
		LCD_KIT_ERR("(%d):create_file: lcd_kit_dbg fail\n", __LINE__);
		goto err_create_mipi;
	}
	already_init = 1;  // set flag
	return LCD_KIT_OK;
err_create_mipi:
		debugfs_remove_recursive(dent);
err_create_dir:
	return LCD_KIT_FAIL;
}
