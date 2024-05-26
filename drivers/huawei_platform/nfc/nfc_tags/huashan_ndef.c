/*! \file huashan_ndef.c
 * \brief  huashan ndef
 *
 * Driver for the huashan
 * Copyright (c) 2011 Nxp Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include "nfc_kit.h"
#include "nfc_kit_ndef.h"

typedef enum {
	TF_MAC,
	TF_SNCODE,
	TF_TAGUID,
	TF_UDID,
	TF_SAFETOKEN,
	TF_DEVTYPEID,
	TF_NUM
} nfc_tag_format_t;

static u8 huashan_ndef_extras_set(u8* ndef, u32 ndef_len, u32 *offset, u16 tag_format)
{
	u32 i = *offset;
	int j;
	u32 k;
	u8 tag_fmt_to_extras[TF_NUM] = {0x03, 0x14, 0x85, 0x10, 0x02, 0x15};
	u8 len = 0;

	if (tag_format & (0x1 << TF_TAGUID)) {
		ndef[i++] = tag_fmt_to_extras[TF_TAGUID];
		len++;
	}
	for (j = TF_NUM - 1; j >= 0; j--) {
		k = (u32)j;
		if ((k == TF_TAGUID) || ((tag_format & (0x1 << k)) == 0x0)) {
			continue;
		}
		ndef[i++] = tag_fmt_to_extras[j];
		len++;
	}
	*offset = i;
	return len;
}

static u8 huashan_ndef_tlv_set(u8* ndef, u32 ndef_len, u32 *offset, u8 type, u8 len, u8 *value)
{
	u32 i = *offset;
	u32 j;
	u8 cnt;

	/* 防止越界 */
	if (i + len + 2 > ndef_len) {
		NFC_ERR("%s:total_l = %u, type = %u, len = %u error\n", __func__, i + len + 2, type, len);
		return 0;
	}
	ndef[i++] = type;
	ndef[i++] = len;
	for (j = 0; j < len; j++)
		ndef[i++] = value[j];
	cnt = (u8)(i - *offset);
	*offset = i;

	return cnt;
}

static u8 huashan_ndef_tags_set(u8* ndef, u32 ndef_len, u32 *offset, u16 tag_format, struct ndef_hidot20_param *hd2)
{
	u32 i = *offset;
	int j;
	u32 k;
	u8 len;
	u8 tag_fmt_to_t[TF_NUM]  = {0x03, 0x14, 0x85, 0x10, 0x02, 0x15};
	u8 tag_fmt_to_l[TF_NUM]  = {
		sizeof(hd2->mac_v), hd2->sn_code_l, 0x0, sizeof(hd2->udid_v),
		sizeof(hd2->safe_token_v), sizeof(hd2->dev_type_id_v)
	};
	u8 *tag_fmt_to_v[TF_NUM] = {
		hd2->mac_v, hd2->sn_code_v, NULL, hd2->udid_v,
		hd2->safe_token_v, hd2->dev_type_id_v
	};

	for (j = TF_NUM - 1; j > 0; j--) {
		k = (u32)j;
		if ((k == TF_TAGUID) || ((tag_format & (0x1 << k)) == 0x0)) {
			continue;
		}
		len = huashan_ndef_tlv_set(ndef, ndef_len, &i,
			tag_fmt_to_t[j], tag_fmt_to_l[j], tag_fmt_to_v[j]);
		if (len == 0x0) {
			return 0x0;
		}
	}

	len = (u8)(i - *offset);
	*offset = i;

	return len;
}

static int nfc_huashan_ndef_create(struct nfc_ndef_param *param, struct nfc_ndef *msg)
{
	u32 i = 0;
	u32 j = 0;
	const u8 *app = "app/hwonehop";
	struct ndef_hidot10_param *hd1 = NULL;
	struct ndef_hidot20_param *hd2 = NULL;
	u16 tag_format;
	u8 x1;
	u8 x2;
	u8 x3;
	u8 x4;
	u8 x5;
	u8 x1_idx;
	u8 x2_idx;
	u8 x3_idx;
	u8 x4_idx;
	u8 *ndef = NULL;
	u32 len;

	if (param == NULL) {
		NFC_ERR("%s:param is NULL\n", __func__);
		return NFC_FAIL;
	}

	if (msg->buf == NULL) {
		NFC_ERR("%s:msg->buf is NULL\n", __func__);
		return NFC_FAIL;
	}

	hd1 = &param->hd1;
	hd2 = &param->hd2;
	ndef = msg->buf;
	len = msg->buf_len;

	/* NDEF Header */                   /*    内容          长度    Value    */
	ndef[i++] = 0x03;                   /*    NDEF Msg       1      0x 03    */
	x1_idx = i;
	ndef[i++] = 0x00;                   /*    length         1      0x X1    */
	ndef[i++] = 0xD2;                   /*    NDEF头字节      1     0x D2    */
	ndef[i++] = strlen(app);            /*    类型长度        1     0x 0C    */
	x2_idx = i;
	ndef[i++] = 0x00;                   /*    负载长度        1     0x X2    */
	for (j = 0; j < strlen(app); j++)
		ndef[i++] = app[j];             /*    类型数值        12    61 70 70 2F 68 77 6F 6E 65 68 6F 70    */

	/* HiDot1.0 Header */
	ndef[i++] = 0x10;                    /*    HiDot协议版本    1    0x 10    */
	ndef[i++] = 0x09;                    /*    Func&Flags      1    0x 09    */
	ndef[i++] = hd1->chip_type;          /*    ChipType        1    根据标签实际内容决定    */
	ndef[i++] = 0x00;                    /*    ExtraFlags      1    0x 00    */
	ndef[i++] = 0x00;                    /*    RFU             1    0x 00    */
	ndef[i++] = 0x30;                    /*    DeviceInfo      4    0x 30 32 45 01    */
	ndef[i++] = 0x32;
	ndef[i++] = 0x45;
	ndef[i++] = 0x01;

	/* HiDot2.0 Header */
	ndef[i++] = 0x01;                    /*    T               1    0x 01    */
	x3_idx = i;
	ndef[i++] = 0x00;                    /*    L               1    0x X3    */
	ndef[i++] = 0x20;                    /*    HiDot协议版本    1    0x 20    */
	ndef[i++] = hd2->func_and_flags;     /*    Func&Flags      1    根据标签实际内容决定    */
	ndef[i++] = hd2->chip_type;          /*    ChipType        1    根据标签实际内容决定    */
	ndef[i++] = 0x40;                    /*    ExtraFlags      1    0x 40    */
	ndef[i++] = 0x00;                    /*    RFU             1    0x 00    */
	for (j = 0; j < sizeof(hd2->device_info); j++)
		ndef[i++] = hd2->device_info[j]; /*    DeviceInfo      5    根据标签实际内容决定    */

	ndef[i++] = 0x81;                    /*    T               1    0x 81    */
	x4_idx = i;
	ndef[i++] = 0x00;                    /*    L               1    0x X4 + 04    */
	ndef[i++] = hd2->tag_format[0];      /*    TagFormat       2    根据实际内容填入    */
	ndef[i++] = hd2->tag_format[1];
	ndef[i++] = 0x00;                    /*    业务执行         2    0x 00 00     */
	ndef[i++] = 0x00;

	tag_format = ((u16)hd2->tag_format[0] << 8) | (u16)hd2->tag_format[1];
	                                     /*    Extras            X4    根据实际内容填入    */
	x4 = huashan_ndef_extras_set(ndef, len, &i, tag_format);
	/* HiDot2.0 payload (optional) */
	                                     /*    T               1    0x 15    */
	                                     /*    L               1    0x 03    */
	                                     /*    V               3     根据标签实际内容决定    */
	                                     /*    T               1    0x 02    */
	                                     /*    L               1    0x 20    */
	                                     /*    V               32    根据标签实际内容决定    */
	                                     /*    T               1    0x 03    */
	                                     /*    L               1    0x 06    */
	                                     /*    V               6    根据标签实际内容决定    */
	                                     /*    T               1    0x 0C    */
	                                     /*    L               1    0x 20    */
	                                     /*    V               32    根据标签实际内容决定    */
	                                     /*    T               1    0x 10    */
	                                     /*    L               1    0x 20    */
	                                     /*    V               32    根据标签实际内容决定    */
	                                     /*    T               1    0x 14    */
	                                     /*    L               1    0x Y1    */
	                                     /*    V               Y1    根据标签实际内容决定    */
	x5 = huashan_ndef_tags_set(ndef, len, &i, tag_format, hd2);
	if ((x5 == 0) && ((tag_format & 0xfffe) > 0)) {
		return NFC_FAIL;
	}

	/* 计算长度 */
	x3 = 16 + x4 + x5;
	x2 = x3 + 11;
	x1 = x2 + strlen(app) + 3;
	ndef[x4_idx] = x4 + 0x04;
	ndef[x3_idx] = x3;
	ndef[x2_idx] = x2;
	ndef[x1_idx] = x1;

	if (i < len)
		ndef[i] = 0x0;

	for (j = 0; j < i; j += 8)
		NFC_DBG("%02x %02x %02x %02x %02x %02x %02x %02x\n",
			ndef[j], ndef[j + 1], ndef[j + 2], ndef[j + 3], ndef[j + 4], ndef[j + 5], ndef[j + 6], ndef[j + 7]);

	return NFC_OK;
}

struct nfc_ndef_ops nfc_huashan_ndef_ops = {
	.nfc_ndef_create = nfc_huashan_ndef_create,
};

int nfc_huashan_ndef_probe(struct nfc_ndef_data *pdata)
{
	int ret;
	const u32 ndef_len = 256; /* VW MONITOR标签最大256字节 */

	if (pdata == NULL) {
		NFC_ERR("%s: ndef data is NULL, error\n", __func__);
		return NFC_FAIL;
	}
	NFC_INFO("%s: called\n", __func__);

	pdata->msg.buf = (u8*)kzalloc(ndef_len, GFP_KERNEL);
	if (pdata->msg.buf == NULL) {
		NFC_ERR("%s:alloc mem for nfc_ndef failed\n", __func__);
		return NFC_FAIL;
	}
	pdata->msg.buf_len = ndef_len;

	/* 注册平台层回调接口 */
	ret = nfc_ndef_ops_register(&nfc_huashan_ndef_ops);
	if (ret) {
		NFC_ERR("%s:ndef ops register failed, ret = %d\n", __func__, ret);
		return ret;
	}

	NFC_INFO("%s: finished\n", __func__);
	return ret;
}

