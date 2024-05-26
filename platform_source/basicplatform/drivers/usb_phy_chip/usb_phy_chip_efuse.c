/*
 * hisi_usb_phy_chip_efuse.c
 *
 * Hisilicon Hi6502 efuse driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#if defined(HISI_UPC_EFUSE_SUPPORT)
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kern_levels.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/platform_drivers/usb_phy_chip.h>
#include <platform_include/basicplatform/linux/hisi_usb_phy_driver.h>

#define HIMASK_ENABLE(val)		((val) << 16 | (val))
#define HIMASK_DISABLE(val)		((val) << 16)
#define UPC_SCTRL_POR			0x16000
#define UPC_EFUSE_BASE			0x14000
#define UPC_EFUSE_CLK_RST_BASE		(UPC_SCTRL_POR + 0x0)
#define UPC_EFUSE_CLK_BIT		8
#define UPC_EFUSE_RST_BIT		9
#define UPC_EFUSE_MAX_SIZE		256
#define UPC_EFUSE_BITS_PER_GROUP	32
#define UPC_EFUSE_BYTES_PER_GROUP	4
#define UPC_EFUSEC_GROUP_MAX_COUNT    (UPC_EFUSE_MAX_SIZE / \
						UPC_EFUSE_BITS_PER_GROUP)
#define UPC_DIEID_START_BIT		96
#define UPC_DIEID_BIT_WIDTH		160
#define UPC_DIEID_BYTE_LENGTH		20

#define UPC_GET_GROUP_START(start_bit)	((start_bit) / UPC_EFUSE_BITS_PER_GROUP)
#define UPC_GET_GROUP_NUM(width_bit)	(((width_bit) + UPC_EFUSE_BITS_PER_GROUP - 1) / \
						UPC_EFUSE_BITS_PER_GROUP)
#define UPC_EFUSE_BITS_MASK(num)	((1u << (num)) - 1)
#define UPC_EFUSE_CFG_BASE		((UPC_EFUSE_BASE) + (0x000))
#define UPC_EFUSEC_APB_SEL		BIT(3)
#define UPC_EFUSE_APB_SEL_SNSP_BASE	((UPC_EFUSE_BASE) + (0x064))
#define UPC_EFUSE_APB_SEL_SNSP		BIT(0)
#define UPC_EFUSE_STATUS_BASE		((UPC_EFUSE_BASE) + (0x004))
#define UPC_EFUSE_PW_STAT		BIT(4)
#define UPC_EFUSE_PW_EN			BIT(5)
#define UPC_EFUSE_GROUP_BASE		((UPC_EFUSE_BASE) + (0x008))
#define UPC_EFUSE_RD			BIT(2)
#define UPC_EFUSE_RD_STAT		BIT(1)
#define UPC_EFUSE_DATA_BASE		((UPC_EFUSE_BASE) + (0x018))
#define UPC_EFUSE_PRE_PG_EN		BIT(1)
#define UPC_EFUSE_PRE_PG_STAT		BIT(2)
#define UPC_EFUSE_PG_VALUE_BASE		((UPC_EFUSE_BASE) + (0x00C))
#define UPC_EFUSE_PG_EN			BIT(0)
#define UPC_EFUSE_WR_STAT		BIT(0)
#define UPC_EFUSE_COUNT_RD_BASE		((UPC_EFUSE_BASE) + (0x024))
#define UPC_EFUSE_1P8_EN		BIT(16)
#define UPC_EFUSE_READ_TIMEOUT		10
#define UPC_EFUSE_WRITE_TIMEOUT		500
#define UPC_EFUSE_DIEID_START_BIT	96
#define UPC_EFUSE_DIEID_BYTE_LENGTH	20
#define UPC_EFUSE_DIEID_BIT_WIDTH	160
#define UPC_EFUSE_EXIT_PD_TIMEOUT	1500
#define UPC_DIEID_GROUP_COUNT		5
#define UPC_GROUP_STR_WIDTH		8
#define UPC_DIEID_ARRAY_LENGTH		80
#define OK				0
#define ERROR				(-1)

static DEFINE_MUTEX(upc_efuse_ops_lock);

enum {
	UPC_EFUSE_WR_OPS,
	UPC_EFUSE_RD_OPS
};

struct upc_efuse_trans {
	u32 start_bit;
	u32 bit_cnt;
	u32 *src;
	u32 src_grp_num;
	u32 *dst;
	u32 dst_grp_num;
};

static void upc_efuse_wr_en(u32 value, u32 addr)
{
	u32 tmp = 0;

	hi6502_readl_u32(&tmp, addr);
	tmp = tmp | value;
	hi6502_writel_u32(tmp, addr);
}

static void upc_efuse_wr_dis(u32 value, u32 addr)
{
	u32 tmp = 0;

	hi6502_readl_u32(&tmp, addr);
	tmp = tmp & (~value);
	hi6502_writel_u32(tmp, addr);
}

static s32 upc_efuse_check_grp(u32 grp_cnt, u32 start_grp)
{
	if (grp_cnt == 0 || grp_cnt > UPC_EFUSEC_GROUP_MAX_COUNT ||
		start_grp >= UPC_EFUSEC_GROUP_MAX_COUNT ||
		start_grp + grp_cnt > UPC_EFUSEC_GROUP_MAX_COUNT) {
		pr_err("upc_efuse err Size:%u, Offset:%u\n",
			grp_cnt, start_grp);
		return ERROR;
	}
	return OK;
}

int u32_to_str(unsigned int data, char *str_buf, unsigned int buf_length,
	const unsigned int str_width, const unsigned int radix)
{
	unsigned int str_pos;
	unsigned int temp;
	/* parameter check */
	if (str_buf == NULL || str_width >= buf_length) {
		pr_err("%s:str_buf invalid!\n", __func__);
		return -1;
	}
	if (radix == 0) {
		pr_err("%s:radix is 0!\n", __func__);
		return -1;
	}
	if (str_width == 0) {
		pr_err("%s:str_width is 0!\n", __func__);
		return -1;
	}

	str_pos = str_width - 1;
	while (str_pos < str_width) {
		temp = data % radix;
		if (temp < 10) /* Remainder less than 10, add '0' */
			str_buf[str_pos] = (char)temp + '0';
		else /* Remainder bigger than 10, add 'a' */
			str_buf[str_pos] = (char)(temp - 10) + 'a';

		data /= radix;
		str_pos--;
	}
	return 0;
}

static void upc_efuse_power_on(void)
{
	u32 upc_type = hi6502_chip_type();

	/* open efuse clk */
	hi6502_writel_u32(HIMASK_ENABLE(BIT(UPC_EFUSE_CLK_BIT)),
		UPC_EFUSE_CLK_RST_BASE);
	/* unreset efuse */
	hi6502_writel_u32(HIMASK_ENABLE(BIT(UPC_EFUSE_RST_BIT)),
		UPC_EFUSE_CLK_RST_BASE);

	/* open efuse power 1p8_en */
	if (upc_type == HISI_USB_PHY_CHIP)
		upc_efuse_wr_en(UPC_EFUSE_1P8_EN, UPC_EFUSE_COUNT_RD_BASE);
}

static void upc_efuse_power_down(void)
{
	u32 upc_type = hi6502_chip_type();
	/* close efuse power 1p8_en */
	if (upc_type == HISI_USB_PHY_CHIP)
		upc_efuse_wr_dis(UPC_EFUSE_1P8_EN, UPC_EFUSE_COUNT_RD_BASE);

	/* reset efuse */
	hi6502_writel_u32(HIMASK_DISABLE(BIT(UPC_EFUSE_RST_BIT)),
		UPC_EFUSE_CLK_RST_BASE);

	/* close efuse clk */
	hi6502_writel_u32(HIMASK_DISABLE(BIT(UPC_EFUSE_CLK_BIT)),
		UPC_EFUSE_CLK_RST_BASE);
}

static s32 __upc_efuse_set_apb_sel(void)
{
	u32 upc_type = hi6502_chip_type();;

	if (upc_type == UNKNOWN_CHIP) {
		pr_err("%s err chip type\n", __func__);
		return ERROR;
	}

	if (upc_type == HISI_USB_PHY_CHIP) {
		upc_efuse_wr_en(UPC_EFUSEC_APB_SEL, UPC_EFUSE_CFG_BASE);
	} else { /* snosphy chip */
		upc_efuse_wr_en(UPC_EFUSE_APB_SEL_SNSP,
			UPC_EFUSE_APB_SEL_SNSP_BASE);
	}
	return OK;
}

static s32 __upc_efuse_exit_power_down(void)
{
	u32 pd_status = 0;
	u32 i;
	u32 timeout = 50;

	hi6502_readl_u32(&pd_status, UPC_EFUSE_STATUS_BASE);
	pd_status = pd_status & UPC_EFUSE_PW_STAT;
	if (pd_status == 0) {
		pr_info("%s UPC_EFUSE pd status is nomal.\n", __func__);
		return OK;
	}
	/* cfg upc efuse pw up, wait: 50 * 100us = 5ms */
	for (i = 0; i < timeout; i++) {
		upc_efuse_wr_dis(UPC_EFUSE_PW_EN, UPC_EFUSE_CFG_BASE);
		udelay(100);
		hi6502_readl_u32(&pd_status, UPC_EFUSE_STATUS_BASE);
		pd_status = pd_status & UPC_EFUSE_PW_STAT;
		if (pd_status == 0) {
			pr_info("%s Exit power down mode OK.\n", __func__);
			return OK;
		}
	}
	pr_info("Exit power down mode ERROR.\n");
	return ERROR;
}

static s32 __upc_efuse_write_groups(const u32 *buf, u32 buf_len,
	const u32 grp_cnt, const u32 start_grp)
{
	u32 grp_idx, i;
	u32 wr_status = 0;
	const u32 *cur_val = buf;
	u32 cur_val_len = 0;
	u32 timeout = 50;

	for (grp_idx = 0; grp_idx < grp_cnt; grp_idx++, cur_val++, cur_val_len++) {
		if (cur_val_len >= buf_len) {
			pr_err("error,buf_len override in %s\n", __func__);
			return ERROR;
		}
		/* set the efuse group number */
		hi6502_writel_u32((start_grp + grp_idx), UPC_EFUSE_GROUP_BASE);

		/* set the buf be written to efuse device */
		hi6502_writel_u32(*cur_val, UPC_EFUSE_PG_VALUE_BASE);

		/* enable writing */
		upc_efuse_wr_en(UPC_EFUSE_PG_EN, UPC_EFUSE_CFG_BASE);

		udelay(UPC_EFUSE_WRITE_TIMEOUT);

		/* waiting for reading finish, wait: 50 * 100us = 5ms */
		for (i = 0; i < timeout; i++) {
			hi6502_readl_u32(&wr_status, UPC_EFUSE_STATUS_BASE);
			wr_status = wr_status & UPC_EFUSE_WR_STAT;
			if (wr_status)
				break; /* wr status is ok, read data */
			udelay(100);
		}
		if (i == timeout) {
			pr_err("error,read efuse data timeout in %s\n", __func__);
			return ERROR;
		}
	}
	return OK;
}

static s32 upc_efuse_read_groups(u32 *buf, u32 buf_len,
	const u32 grp_cnt, const u32 start_grp)
{
	s32 ret, i;
	u32 grp_idx;
	u32 rd_status = 0;
	u32 *cur_val = NULL;
	u32 cur_val_len = 0;
	u32 timeout = 50;

	ret = upc_efuse_check_grp(grp_cnt, start_grp);
	if (ret != OK) {
		pr_err("error,invalid group in %s\n", __func__);
		return ERROR;
	}

	upc_efuse_power_on();

	/* select apb bus operate efusec */
	ret = __upc_efuse_set_apb_sel();
	if (ret == ERROR)
		goto efuse_pd;

	/* exit power_down status */
	ret = __upc_efuse_exit_power_down();
	if (ret == ERROR)
		goto exit_pd_fail;

	cur_val = buf;
	for (grp_idx = 0; grp_idx < grp_cnt; grp_idx++, cur_val++, cur_val_len++) {
		if (cur_val_len >= buf_len) {
			pr_err("error,buf len override in %s\n", __func__);
			ret = ERROR;
			goto exit_pd_fail;
		}
		/* set reading group */
		hi6502_writel_u32((start_grp + grp_idx), UPC_EFUSE_GROUP_BASE);

		/* enable read */
		upc_efuse_wr_en(UPC_EFUSE_RD, UPC_EFUSE_CFG_BASE);

		udelay(UPC_EFUSE_READ_TIMEOUT);

		/* waiting for reading finish, wait: 50 * 100us = 5ms */
		for (i = 0; i < timeout; i++) {
			hi6502_readl_u32(&rd_status, UPC_EFUSE_STATUS_BASE);
			rd_status = rd_status & UPC_EFUSE_RD_STAT;
			if (rd_status) {
				/* rd status is ok, read data */
				hi6502_readl_u32(cur_val, UPC_EFUSE_DATA_BASE);
				break;
			}
			udelay(100);
		}
		if (i == timeout) {
			pr_err("error,read efuse data timeout in %s\n", __func__);
			ret = ERROR;
			goto exit_pd_fail;
		}
	}

exit_pd_fail:
	upc_efuse_wr_en(UPC_EFUSE_PW_EN, UPC_EFUSE_CFG_BASE);
efuse_pd:
	upc_efuse_power_down();
	return ret;
}

static s32 upc_efuse_writel_groups(const u32 *buf, u32 buf_len,
	const u32 grp_cnt, const u32 start_grp)
{
	s32 ret;
	u32 pg_stat = 0;
	u32 i;
	u32 timeout = 50;

	ret = upc_efuse_check_grp(grp_cnt, start_grp);
	if (ret != OK) {
		pr_err("error,invalid group in %s\n", __func__);
		return ERROR;
	}

	upc_efuse_power_on();

	/* select apb bus operate efusec */
	ret = __upc_efuse_set_apb_sel();
	if (ret == ERROR)
		goto efuse_pd;

	/* exit power_down status */
	ret = __upc_efuse_exit_power_down();
	if (ret == ERROR)
		goto exit_pd_fail;

	/* efuse pre_pg enable */
	upc_efuse_wr_en(UPC_EFUSE_PRE_PG_EN, UPC_EFUSE_CFG_BASE);

	/* waiting for reading finish, wait: 50 * 100us = 5ms */
	for (i = 0; i < timeout; i++) {
		hi6502_readl_u32(&pg_stat, UPC_EFUSE_STATUS_BASE);
		pg_stat = pg_stat & UPC_EFUSE_PRE_PG_STAT;
		if (pg_stat != 0)
			break;
		udelay(100);
	}
	if (i == timeout) {
		pr_err("error,write efuse state timeout in %s\n", __func__);
		ret = ERROR;
		goto exit_cfg_fail;
	}

	ret = __upc_efuse_write_groups(buf, buf_len, grp_cnt, start_grp);

exit_cfg_fail:
	/* disable pre-wiriting */
	upc_efuse_wr_dis(UPC_EFUSE_PRE_PG_EN, UPC_EFUSE_CFG_BASE);

exit_pd_fail:
	upc_efuse_wr_en(UPC_EFUSE_PW_EN, UPC_EFUSE_CFG_BASE);
efuse_pd:
	upc_efuse_power_down();
	return ret;
}

static void upc_efuse_move_align(struct upc_efuse_trans *desc)
{
	u32 i;
	u32 bit_cnt = desc->bit_cnt;

	for (i = 0; i < desc->src_grp_num; i++) {
		desc->dst[i] = desc->src[i];
		if (bit_cnt < UPC_EFUSE_BITS_PER_GROUP)
			desc->dst[i] &= UPC_EFUSE_BITS_MASK(bit_cnt);
		else
			bit_cnt -= UPC_EFUSE_BITS_PER_GROUP;
	}
}

static void upc_efuse_mem_move_rd(struct upc_efuse_trans *desc)
{
	u32 i;
	u32 len_front, len_behind;
	u32 wild_bit;
	u32 grp_high_part, grp_low_part;
	u32 bit_cnt = desc->bit_cnt;
	u32 *src = desc->src;
	u32 *dst = desc->dst;

	wild_bit = desc->start_bit % UPC_EFUSE_BITS_PER_GROUP;
	if (wild_bit == 0) {
		upc_efuse_move_align(desc);
		return;
	} else if (wild_bit + bit_cnt <= UPC_EFUSE_BITS_PER_GROUP) {
		*dst = (*src >> wild_bit) & UPC_EFUSE_BITS_MASK(bit_cnt);
		return;
	}

	for (i = 0; i < desc->dst_grp_num; i++, src++, dst++) {
		len_front = wild_bit;
		len_behind = UPC_EFUSE_BITS_PER_GROUP - wild_bit;

		if (bit_cnt < len_behind)
			len_behind = bit_cnt;

		grp_low_part = *src >> len_front;
		grp_low_part &= UPC_EFUSE_BITS_MASK(len_behind);
		*dst = grp_low_part;
		bit_cnt -= len_behind;

		if ((i + 1) < desc->src_grp_num && bit_cnt < len_front)
			len_front = bit_cnt;

		if ((i + 1) < desc->src_grp_num) {
			grp_high_part = *(src + 1);
			grp_high_part &= UPC_EFUSE_BITS_MASK(len_front);
			grp_high_part <<= len_behind;
			*dst |= grp_high_part;

			bit_cnt -= len_front;
		}
	}
}

static void upc_efuse_mem_move_write(struct upc_efuse_trans *desc)
{
	u32 i;
	u32 len_front, len_behind;
	u32 wild_bit;
	u32 grp_high_part, grp_low_part;
	u32 bit_cnt = desc->bit_cnt;
	u32 *src = desc->src;
	u32 *dst = desc->dst;

	for (i = 0; i < desc->dst_grp_num; i++)
		dst[i] = 0;

	wild_bit = desc->start_bit % UPC_EFUSE_BITS_PER_GROUP;
	if (wild_bit == 0) {
		upc_efuse_move_align(desc);
		return;
	} else if (wild_bit + bit_cnt <= UPC_EFUSE_BITS_PER_GROUP) {
		*dst = (*src & UPC_EFUSE_BITS_MASK(bit_cnt)) << wild_bit;
		return;
	}

	for (i = 0; i < desc->src_grp_num; i++, src++, dst++) {
		len_front = wild_bit;
		len_behind = UPC_EFUSE_BITS_PER_GROUP - wild_bit;
		if (bit_cnt < len_behind)
			len_behind = bit_cnt;

		grp_high_part = *src & UPC_EFUSE_BITS_MASK(len_behind);
		grp_high_part <<= len_front;
		*dst |= grp_high_part;
		bit_cnt -= len_behind;

		if ((i + 1) < desc->dst_grp_num && bit_cnt < len_front)
			len_front = bit_cnt;

		if ((i + 1) < desc->dst_grp_num) {
			grp_low_part = *src >> len_behind;
			grp_low_part &= UPC_EFUSE_BITS_MASK(len_front);
			*(dst + 1) = grp_low_part;
			bit_cnt -= len_front;
		}
	}
}

static s32 upc_efuse_calc_grp_num(u32 start_bit, u32 bit_cnt)
{
	s32 grp_num = 1;
	u32 wild_bit = start_bit % UPC_EFUSE_BITS_PER_GROUP;

	while ((wild_bit + bit_cnt) > UPC_EFUSE_BITS_PER_GROUP) {
		grp_num++;
		bit_cnt -= (UPC_EFUSE_BITS_PER_GROUP - wild_bit);
		wild_bit = 0;
	}

	return grp_num;
}

static s32 upc_efuse_check_bit_range(u32 start_bit, u32 bit_cnt)
{
	if (start_bit >= UPC_EFUSE_MAX_SIZE || bit_cnt > UPC_EFUSE_MAX_SIZE ||
	    bit_cnt == 0 || (start_bit + bit_cnt > UPC_EFUSE_MAX_SIZE)) {
		pr_err("UPC_EFUSE over length,%s, start_bit=0x%x,cnt=0x%x\n",
			    __func__, start_bit, bit_cnt);
		return ERROR;
	}
	return OK;
}

s32 get_upc_efuse_value(u32 start_bit, u32 *buf, u32 bit_cnt)
{
	s32 ret;
	u32 start_grp;
	u32 src_grp_num, dst_grp_num;
	u32 buf_bytes;
	u32 temp_buf[UPC_EFUSEC_GROUP_MAX_COUNT] = {0};
	struct upc_efuse_trans trans_desc = {0};

	ret = upc_efuse_check_bit_range(start_bit, bit_cnt);
	if (!buf || ret != OK) {
		pr_err("error,invalid paras,in %s\n", __func__);
		return ERROR;
	}

	mutex_lock(&upc_efuse_ops_lock);

	src_grp_num = upc_efuse_calc_grp_num(start_bit, bit_cnt);
	start_grp = UPC_GET_GROUP_START(start_bit);
	dst_grp_num = UPC_GET_GROUP_NUM(bit_cnt);
	buf_bytes = (u32)(dst_grp_num * (u32)UPC_EFUSE_BYTES_PER_GROUP);
	memset((void *)buf, 0, buf_bytes);

	ret = upc_efuse_read_groups(temp_buf, UPC_EFUSEC_GROUP_MAX_COUNT,
		src_grp_num, start_grp);
	if (ret == OK) {
		trans_desc.start_bit = start_bit;
		trans_desc.bit_cnt = bit_cnt;
		trans_desc.src = temp_buf;
		trans_desc.src_grp_num = src_grp_num;
		trans_desc.dst = buf;
		trans_desc.dst_grp_num = dst_grp_num;
		upc_efuse_mem_move_rd(&trans_desc);
	}

	mutex_unlock(&upc_efuse_ops_lock);
	return ret;
}

s32 set_upc_efuse_value(u32 start_bit, u32 *buf, u32 bit_cnt)
{
	u32 start_grp;
	u32 src_grp_num, dst_grp_num;
	s32 ret;
	u32 temp_buf[UPC_EFUSEC_GROUP_MAX_COUNT] = {0};
	struct upc_efuse_trans trans_desc = {0};

	ret = upc_efuse_check_bit_range(start_bit, bit_cnt);
	if (!buf || ret != OK) {
		pr_err("error,invalid paras,in %s\n", __func__);
		return ERROR;
	}

	mutex_lock(&upc_efuse_ops_lock);

	dst_grp_num = upc_efuse_calc_grp_num(start_bit, bit_cnt);
	start_grp = UPC_GET_GROUP_START(start_bit);
	src_grp_num = UPC_GET_GROUP_NUM(bit_cnt);

	trans_desc.start_bit = start_bit;
	trans_desc.bit_cnt = bit_cnt;
	trans_desc.src = buf;
	trans_desc.src_grp_num = src_grp_num;
	trans_desc.dst = temp_buf;
	trans_desc.dst_grp_num = dst_grp_num;
	upc_efuse_mem_move_write(&trans_desc);

	ret = upc_efuse_writel_groups(temp_buf, UPC_EFUSEC_GROUP_MAX_COUNT,
		dst_grp_num, start_grp);

	mutex_unlock(&upc_efuse_ops_lock);
	return ret;
}

#define HEX_RADIX	16
static void upc_analysis_dieid_format(unsigned int *ori_dieid, unsigned int ori_length,
	char *dieid_buf, unsigned int dieid_length)
{
	unsigned int index;
	int ret;

	ret = snprintf(dieid_buf, dieid_length, "\r\nHi6502:0x");
	if (ret < 0)
		pr_err("%s snprintf hi6502 err format\n", __func__);

	dieid_buf += strlen("\r\nHi6502:0x");

	for (index = 0; index < ori_length; index++) {
		if (u32_to_str(ori_dieid[(ori_length - index) - 1],
			dieid_buf, dieid_length,
			UPC_GROUP_STR_WIDTH, HEX_RADIX) != OK) {
			pr_err("%s u32_to_str failed!\n", __func__);
			return;
		}
		dieid_buf += UPC_GROUP_STR_WIDTH;
	}

	strncat(dieid_buf, "\r\n", strlen("\r\n"));
	dieid_buf += strlen("\r\n");
}

int hisi_usb_phy_chip_get_dieid(char *buf, unsigned int buf_len)
{
	int ret;
	unsigned int upc_dieid[UPC_DIEID_GROUP_COUNT] = {0};

	if (buf == NULL) {
		pr_err("[%s]: dieid buf is null\n", __func__);
		return ERROR;
	}

	if (buf_len < UPC_DIEID_ARRAY_LENGTH) {
		pr_err("[%s]: dieid length is too short\n", __func__);
		return ERROR;
	}
	ret = get_upc_efuse_value(UPC_EFUSE_DIEID_START_BIT, upc_dieid,
			      (unsigned int)UPC_EFUSE_DIEID_BIT_WIDTH);
	if (ret != OK) {
		pr_err("[%s] : get_efuse_value err, errcode=%d\n",
			    __func__, ret);
		return ERROR;
	}
	upc_analysis_dieid_format(upc_dieid, UPC_DIEID_GROUP_COUNT,
		buf, UPC_DIEID_ARRAY_LENGTH);

	return OK;
}

int get_upc_dieid_test(void)
{
	char dieid_buf[UPC_DIEID_ARRAY_LENGTH] = {0};
	int ret;

	ret = hisi_usb_phy_chip_get_dieid(dieid_buf, UPC_DIEID_ARRAY_LENGTH);
	if (ret == OK)
		pr_err("usb_phy_chip DIEID is: %s\n", dieid_buf);
	else
		pr_err("usb_phy_chip DIEID read failed\n");
	return ret;
}

void get_upc_efuse_test(unsigned int start_bit, unsigned int bit_cnt)
{
	u32 buf = 0;

	(void)get_upc_efuse_value(start_bit, &buf, bit_cnt);
	pr_err("%s : value is 0x%x\n", __func__, buf);
}

void set_upc_efuse_test(unsigned int start_bit, u32 buf, unsigned int bit_cnt)
{
	(void)set_upc_efuse_value(start_bit, &buf, bit_cnt);
	pr_err("%s : value is 0x%x\n", __func__, buf);
}
#else
#include <linux/types.h>
s32 get_upc_efuse_value(u32 start_bit, u32 *buf, u32 bit_cnt)
{
	return -1;
}

s32 set_upc_efuse_value(u32 start_bit, u32 *buf, u32 bit_cnt)
{
	return -1;
}
int hisi_usb_phy_chip_get_dieid(char *buf, unsigned int buf_len)
{
	return -1;
}
#endif /* HISI_UPC_EFUSE_SUPPORT */
