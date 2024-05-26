/*! \file nta5332_ndef.c
 * \brief  nta5332 ndef
 *
 * Driver for the nta5332
 * Copyright (c) 2011 Nxp Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include "nfc_kit.h"
#include "nfc_kit_ndef.h"
#include "securec.h"

#define MAX_LONDON_NFC_MMI_READ_NUM 64
#define MAX_LONDON_USER_DATA_AREA 888

static struct nfc_ndef_data *g_ndef_data = NULL;
static const char *nfc_at_info = NULL;    /* AT^GETDEVINFO=ALL指令获取到的NFC信息 */
static u8 g_is_read_at_nfc_info_flag;

static struct ndef_info g_ndef_info[] = {
#ifdef CONFIG_LCD_LONDON_PLATFORM
	{ "lcd_london", nfc_huashan_ndef_probe },
#endif
#ifdef CONFIG_HUASHAN_PLATFORM
	{ "huashan", nfc_huashan_ndef_probe },
#endif
};

static int nfc_ndef_param_get_by_dts(const struct device *dev, struct nfc_ndef_param *pdata)
{
	int ret;
	struct device_node *node = dev->of_node;
	struct device_node *child_node = NULL;

	ret = nfc_of_read_string(node, "ndef_type", &pdata->ndef_type);
	ret |= nfc_of_read_u8_array(node, "chip_type", &pdata->hd1.chip_type, sizeof(pdata->hd1.chip_type));
	ret |= nfc_of_read_u8_array(node, "func_and_flags", &pdata->hd2.func_and_flags, sizeof(pdata->hd2.func_and_flags));
	ret |= nfc_of_read_u8_array(node, "chip_type", &pdata->hd2.chip_type, sizeof(pdata->hd2.chip_type));
	ret |= nfc_of_read_u8_array(node, "device_info", pdata->hd2.device_info, sizeof(pdata->hd2.device_info));
	ret |= nfc_of_read_u8_array(node, "tag_format", pdata->hd2.tag_format, sizeof(pdata->hd2.tag_format));
	ret |= nfc_of_read_u8_array(node, "device_type_id", pdata->hd2.dev_type_id_v, sizeof(pdata->hd2.dev_type_id_v));

	child_node = of_get_next_child(node, NULL);
	if (child_node == NULL) {
		NFC_ERR("%s: get dts nfc child nod failed\n", __func__);
		return ret;
	}

	if (nfc_of_read_string(child_node, "compatible", &nfc_at_info) == NFC_OK)
		g_is_read_at_nfc_info_flag = 1;

	return ret;
}

static int nfc_ndef_buf_decode(const char *buf, size_t count, u8 *pdata, size_t pdata_sz)
{
	u32 i;
	u32 j;
	u32 cnt = 0x0;

	if ((buf == NULL) || (count <= 0) || (pdata == NULL)) {
		NFC_ERR("%s: buf is NULL or count %zu <= 0\n", __func__, count);
		return NFC_FAIL;
	}
	if (buf[count - 1] == '\n')
		count--; /* 通过echo写设备属性节点时, 字符串末尾会自动添加换行符, 这里需要去掉 */

	for (j = 0x0; j < pdata_sz; j++)
		pdata[j] = 0x0;

	/* 输入字符串有效字符有'0'-'9', 'a'-'f', 'A'-'F', 每两个字符表示1B数据;
	   例如"09aF"是正确的, " 0z  xF"是错误的. */
	j = 0x0;
	for (i = 0x0; i < count; i++) {
		if (j >= pdata_sz) {
			NFC_ERR("%s: buf %s too long\n", __func__, buf);
			return NFC_FAIL;
		}
		if (buf[i] >= '0' && buf[i] <= '9') {
			pdata[j] = (pdata[j] * 16) + (buf[i] - '0');
	    } else if (buf[i] >= 'a' && buf[i] <= 'f') {
			pdata[j] = (pdata[j] * 16) + (buf[i] - 'a' + 10);
		} else if (buf[i] >= 'A' && buf[i] <= 'F') {
			pdata[j] = (pdata[j] * 16) + (buf[i] - 'A' + 10);
		} else {
			NFC_ERR("%s: buf %s contain invalid character\n", __func__, buf);
			return NFC_FAIL;
		}
		cnt++;
		if (cnt % 2 == 0)
			j++;
	}

	if (cnt != j * 2) {
		NFC_ERR("%s: buf %s format error\n", __func__, buf);
		return NFC_FAIL;
	}

	return NFC_OK;
}

static int nfc_ndef_buf_encode(char *buf, size_t *count, const u8 *pdata, size_t pdata_sz)
{
	int ret;
	u32 i;

	if (buf == NULL) {
		NFC_ERR("%s: buf is NULL\n", __func__);
		return NFC_FAIL;
	}
	/* 输出字符串有效字符有'0'-'9', 'a'-'f', 每两个字符表示1B数据;
	   例如"09af"表示2B数据, 09和af. */
	*count = 0;
	for (i = 0; i < pdata_sz; i++) {
		ret = snprintf_s(buf + *count, PAGE_SIZE - *count, PAGE_SIZE - *count - 1, "%02x", pdata[i]);
		if (ret < 0) {
			NFC_ERR("%s: snprintf_s failed\n", __func__);
			return NFC_FAIL;
		}
		*count += ret;
	}

	if (*count < PAGE_SIZE)
		buf[*count++] = '\n';

	return NFC_OK;
}

ssize_t nfc_ndef_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret, j;
	size_t count = 256;
	struct nfc_kit_data *pdata = NULL;
	char tmp[256];

	pdata = get_nfc_kit_data();
	if (pdata == NULL || pdata->chip_data == NULL || pdata->chip_data->ops == NULL) {
		NFC_INFO("%s: test fail\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops->ndef_show == NULL) {
		NFC_INFO("%s: test fail\n", __func__);
		return 0;
	}

	if (g_ndef_data == NULL || g_ndef_data->msg.buf == NULL) {
		NFC_INFO("%s: test fail\n", __func__);
		return 0;
	}

	ret = pdata->chip_data->ops->ndef_show(tmp, &count);
	for (j = 0; j < count; j += 8)
		NFC_DBG("%02x %02x %02x %02x %02x %02x %02x %02x\n",
			tmp[j], tmp[j + 1], tmp[j + 2], tmp[j + 3], tmp[j + 4], tmp[j + 5], tmp[j + 6], tmp[j + 7]);

	ret = memcmp(tmp, g_ndef_data->msg.buf, count);
	if (ret) {
		NFC_INFO("%s: test fail\n", __func__);
		ret = memcpy_s(buf, PAGE_SIZE, "test fail", sizeof("test fail"));
		if (ret != EOK) {
			return 0;
		}
	} else {
		NFC_INFO("%s: test pass\n", __func__);
		ret = memcpy_s(buf, PAGE_SIZE, "test pass", sizeof("test pass"));
		if (ret != EOK) {
			return 0;
		}
	}

	return (ssize_t)strlen(buf);
}

ssize_t nfc_ndef_safe_token_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct ndef_hidot20_param *hd2 = NULL;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_decode(buf, count, hd2->safe_token_v, sizeof(hd2->safe_token_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_safe_token_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ndef_hidot20_param *hd2 = NULL;
	size_t count = 0x0;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_encode(buf, &count, hd2->safe_token_v, sizeof(hd2->safe_token_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_mac_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct ndef_hidot20_param *hd2 = NULL;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_decode(buf, count, hd2->mac_v, sizeof(hd2->mac_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_mac_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ndef_hidot20_param *hd2 = NULL;
	size_t count = 0x0;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_encode(buf, &count, hd2->mac_v, sizeof(hd2->mac_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_passwd_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct ndef_hidot20_param *hd2 = NULL;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_decode(buf, count, hd2->passwd_v, sizeof(hd2->passwd_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_passwd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ndef_hidot20_param *hd2 = NULL;
	size_t count = 0x0;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_encode(buf, &count, hd2->passwd_v, sizeof(hd2->passwd_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_udid_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct ndef_hidot20_param *hd2 = NULL;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_decode(buf, count, hd2->udid_v, sizeof(hd2->udid_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_udid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ndef_hidot20_param *hd2 = NULL;
	size_t count = 0x0;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;
	ret = nfc_ndef_buf_encode(buf, &count, hd2->udid_v, sizeof(hd2->udid_v));
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_sn_code_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct ndef_hidot20_param *hd2 = NULL;
	int ret;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;

	/* 对sn code, buf的前两个字符，是sn code长度，其后数据才是sn code */
	if (count < 2) {
		NFC_ERR("%s: buf too short, count = %zu\n", __func__, count);
		return (ssize_t)count;
	}
	ret = nfc_ndef_buf_decode(buf, 2, &hd2->sn_code_l, sizeof(hd2->sn_code_l));
	if (ret != NFC_OK) {
		NFC_ERR("%s: 1 fail, ret=%d\n", __func__, ret);
		return (ssize_t)count;
	}
	if (hd2->sn_code_l > HIDOT20_SN_CODE_LEN) {
		NFC_ERR("%s: sn_code_l %u too long\n", __func__, hd2->sn_code_l);
		return (ssize_t)count;
	}
	if (count < 2 * (1 + hd2->sn_code_l)) {
		NFC_ERR("%s: buf too short, count = %zu\n", __func__, count);
		return (ssize_t)count;
	}

	ret = nfc_ndef_buf_decode(buf + 2, count - 2, hd2->sn_code_v, HIDOT20_SN_CODE_LEN);
	if (ret != NFC_OK)
		NFC_ERR("%s: 2 fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_sn_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ndef_hidot20_param *hd2 = NULL;
	int ret;
	u8  tmp[HIDOT20_SN_CODE_LEN + 1];
	u32 i;
	size_t count = 0x0;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return (ssize_t)count;
	}
	hd2 = &g_ndef_data->param.hd2;

	/* 对sn code, buf的前两个字符，是sn code长度，其后数据才是sn code */
	if (hd2->sn_code_l > HIDOT20_SN_CODE_LEN) {
		NFC_ERR("%s: sn_code_l %u too long\n", __func__, hd2->sn_code_l);
		return (ssize_t)count;
	}
	tmp[0] = hd2->sn_code_l;
	for (i = 0x0; i < hd2->sn_code_l; i++)
		tmp[i + 1] = hd2->sn_code_v[i];

	ret = nfc_ndef_buf_encode(buf, &count, tmp, hd2->sn_code_l + 1);
	if (ret != NFC_OK)
		NFC_ERR("%s: fail, ret=%d\n", __func__, ret);

	return (ssize_t)count;
}

static void nfc_ndef_buf_free(struct nfc_ndef *msg)
{
	if (msg->buf != NULL) {
		memset_s(msg->buf, msg->buf_len, 0x0, msg->buf_len);
		kfree(msg->buf);
		msg->buf = NULL;
		msg->buf_len = 0x0;
	}
	return;
}

ssize_t nfc_ndef_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	struct nfc_ndef_ops *ops = NULL;
	struct nfc_ndef *msg = NULL;
	struct nfc_kit_data *pdata = NULL;

	if ((g_ndef_data == NULL) || (g_ndef_data->ops == NULL)) {
		NFC_ERR("%s: g_ndef_data or ops is NULL\n", __func__);
		return (ssize_t)count;
	}

	ops = g_ndef_data->ops;
	if (ops->nfc_ndef_create == NULL) {
		NFC_ERR("%s: nfc_ndef_create is NULL\n", __func__);
		return (ssize_t)count;
	}

	pdata = get_nfc_kit_data();
	if (pdata == NULL || pdata->chip_data == NULL || pdata->chip_data->ops == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return (ssize_t)count;
	}
	if (pdata->chip_data->ops->ndef_store == NULL) {
		NFC_ERR("%s: ndef_store is NULL\n", __func__);
		return (ssize_t)count;
	}

	mutex_lock(&g_ndef_data->mutex);
	msg = &g_ndef_data->msg;
	ret = ops->nfc_ndef_create(&g_ndef_data->param, msg);
	if (ret == NFC_OK)
		ret = pdata->chip_data->ops->ndef_store(msg->buf, msg->buf_len);

	mutex_unlock(&g_ndef_data->mutex);
	if (ret != NFC_OK)
		NFC_ERR("%s: ndef config fail, ret = %d\n", __func__, ret);

	return (ssize_t)count;
}

ssize_t nfc_ndef_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	size_t count = 0;
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data or ops is NULL\n", __func__);
		return (ssize_t)count;
	}

	if (pdata == NULL || pdata->chip_data == NULL) {
		return (ssize_t)count;
	}

	if (pdata->chip_data->ops == NULL || pdata->chip_data->ops->ndef_show == NULL) {
		return (ssize_t)count;
	}

	mutex_lock(&g_ndef_data->mutex);
	ret = pdata->chip_data->ops->ndef_show(g_ndef_data->msg.buf, (size_t *)&g_ndef_data->msg.buf_len);

	// NFC_ERR("%s: ndef_show, ret = %d\n", __func__, ret); ret=0
	nfc_ndef_buf_encode(buf, &count, g_ndef_data->msg.buf, g_ndef_data->msg.buf_len);
	mutex_unlock(&g_ndef_data->mutex);
	return (ssize_t)count;
}

ssize_t nfc_ndef_ldn_mmi_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data or ops is NULL\n", __func__);
		return 0;
	}

	if (pdata == NULL || pdata->chip_data == NULL) {
		NFC_ERR("%s: pdata or chip_data is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops == NULL) {
		NFC_ERR("%s: pdata->chip_data->ops is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops->ndef_store == NULL) {
		NFC_ERR("%s: ndef_store is NULL\n", __func__);
		return 0;
	}

	mutex_lock(&g_ndef_data->mutex);
	ret = pdata->chip_data->ops->ndef_store(buf, count);
	mutex_unlock(&g_ndef_data->mutex);

	if (ret != NFC_OK) {
		NFC_ERR("%s: ndef_store fail, ret = %d\n", __func__, ret);
		return 0;
	}

	return (ssize_t)count;
}

ssize_t nfc_ndef_ldn_mmi_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	size_t count = 0;
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return 0;
	}

	if (pdata == NULL || pdata->chip_data == NULL) {
		NFC_ERR("%s: pdata or chip_data is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops == NULL) {
		NFC_ERR("%s: pdata->chip_data->ops is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops->ndef_show == NULL) {
		NFC_ERR("%s: ndef_show is NULL\n", __func__);
		return 0;
	}

	mutex_lock(&g_ndef_data->mutex);
	count = MAX_LONDON_NFC_MMI_READ_NUM;
	ret = pdata->chip_data->ops->ndef_show(g_ndef_data->msg.buf, &count);
	if (ret != NFC_OK) {
		NFC_ERR("%s: ndef_show failed\n", __func__);
		mutex_unlock(&g_ndef_data->mutex);
		return 0;
	}

	count = 0;
	// NFC_ERR("%s: ndef_show, ret = %d\n", __func__, ret); ret=0
	ret = nfc_ndef_buf_encode(buf, &count, g_ndef_data->msg.buf, MAX_LONDON_NFC_MMI_READ_NUM);
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc_ndef_buf_encode failed\n", __func__);
		mutex_unlock(&g_ndef_data->mutex);
		return 0;
	}
	mutex_unlock(&g_ndef_data->mutex);

	return (ssize_t)count;
}

ssize_t nfc_ndef_direct_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;
	u8 tmp[MAX_LONDON_USER_DATA_AREA + 1];

	pdata = get_nfc_kit_data();

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data or ops is NULL\n", __func__);
		return 0;
	}

	if (pdata == NULL || pdata->chip_data == NULL) {
		NFC_ERR("%s: pdata or chip_data is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops == NULL) {
		NFC_ERR("%s: pdata->chip_data->ops is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops->ndef_store == NULL) {
		NFC_ERR("%s: ndef_store is NULL\n", __func__);
		return 0;
	}

	ret = nfc_ndef_buf_decode(buf, count, tmp, MAX_LONDON_USER_DATA_AREA);
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc_ndef_buf_decode failed\n", __func__);
		return 0;
	}

	mutex_lock(&g_ndef_data->mutex);
	ret = pdata->chip_data->ops->ndef_store(tmp, count);
	mutex_unlock(&g_ndef_data->mutex);

	if (ret != NFC_OK) {
		NFC_ERR("%s: ndef_store fail, ret = %d\n", __func__, ret);
		return 0;
	}

	return (ssize_t)count;
}

ssize_t nfc_ndef_direct_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	size_t count = 0;
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return 0;
	}

	if (pdata == NULL || pdata->chip_data == NULL) {
		NFC_ERR("%s: pdata or chip_data is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops == NULL) {
		NFC_ERR("%s: pdata->chip_data->ops is NULL\n", __func__);
		return 0;
	}

	if (pdata->chip_data->ops->ndef_show == NULL) {
		NFC_ERR("%s: ndef_show is NULL\n", __func__);
		return 0;
	}

	mutex_lock(&g_ndef_data->mutex);
	ret = pdata->chip_data->ops->ndef_show(g_ndef_data->msg.buf, (size_t *)&g_ndef_data->msg.buf_len);
	if (ret != NFC_OK) {
		NFC_ERR("%s: ndef_show failed\n", __func__);
		mutex_unlock(&g_ndef_data->mutex);
		return 0;
	}

	ret = nfc_ndef_buf_encode(buf, &count, g_ndef_data->msg.buf, g_ndef_data->msg.buf_len);
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc_ndef_buf_encode failed\n", __func__);
		mutex_unlock(&g_ndef_data->mutex);
		return 0;
	}
	mutex_unlock(&g_ndef_data->mutex);

	return (ssize_t)count;
}

ssize_t nfc_ndef_ldn_at_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;

	if (g_is_read_at_nfc_info_flag == 0) {
		NFC_ERR("%s: parse nfc dts at info failed\n", __func__);
		return 0;
	}
	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%s,i2c", nfc_at_info);
	if (ret < 0) {
		NFC_ERR("%s: snprintf_s failed\n", __func__);
		return 0;
	}
	return (ssize_t)ret;
}

int nfc_ndef_ops_register(struct nfc_ndef_ops *ndef_ops)
{
	if (ndef_ops == NULL) {
		NFC_ERR("%s: ndef_ops is NULL\n", __func__);
		return NFC_FAIL;
	}

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return NFC_FAIL;
	}

	mutex_lock(&g_ndef_data->mutex);
	g_ndef_data->ops = ndef_ops;
	mutex_unlock(&g_ndef_data->mutex);
	return NFC_OK;
}

static int nfc_ndef_probe(struct nfc_ndef_data *pdata)
{
	u32 i;

	if (g_ndef_data == NULL) {
		NFC_ERR("%s: g_ndef_data is NULL\n", __func__);
		return NFC_FAIL;
	}

	for (i = 0; i < sizeof(g_ndef_info) / sizeof(struct ndef_info); i++) {
		if (strcmp(g_ndef_data->param.ndef_type, g_ndef_info[i].ndef_type) == 0) {
			break;
		}
	}

	if (i >= sizeof(g_ndef_info) / sizeof(struct ndef_info)) {
		NFC_ERR("%s: can not find [%s] in g_ndef_info\n", __func__, g_ndef_data->param.ndef_type);
		return NFC_FAIL;
	}

	if (g_ndef_info[i].nfc_ndef_probe == NULL) {
		NFC_ERR("%s: nfc_ndef_probe is NULL\n", __func__);
		return NFC_FAIL;
	}
	return g_ndef_info[i].nfc_ndef_probe(pdata);
}

void nfc_kit_ndef_free(void)
{
	if (g_ndef_data != NULL) {
		nfc_ndef_buf_free(&g_ndef_data->msg);
		kfree(g_ndef_data);
		g_ndef_data = NULL;
	}
}

int nfc_kit_ndef_init(const struct device *dev)
{
	int ret;
	struct nfc_ndef_data *pdata = NULL;

	NFC_MAIN("%s: nfc kit ndef probe start\n", __func__);
	pdata = (struct nfc_ndef_data *)kzalloc(sizeof(struct nfc_ndef_data), GFP_KERNEL);
	if (pdata == NULL) {
		NFC_ERR("%s: kzalloc failed\n", __func__);
		return -ENOMEM;
	}
	mutex_init(&pdata->mutex);
	g_ndef_data = pdata;

#ifdef CONFIG_OF
	ret = nfc_ndef_param_get_by_dts(dev, &g_ndef_data->param);
	if (ret != NFC_OK) {
		NFC_ERR("%s: parse dts error, ret = %d\n", __func__, ret);
		nfc_kit_ndef_free();
		return ret;
	}
	NFC_INFO("%s: dts parse success\n", __func__);
#else
	/* 如果平台不支持设备树，通过编译错误进行提醒 */
#error : NFC-i2c kit ndef only support device tree ndef
#endif

	/* 根据ndef类型执行对应的probe */
	ret = nfc_ndef_probe(pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc_ndef_probe failed\n", __func__);
		nfc_kit_ndef_free();
	}

	NFC_MAIN("%s: success\n", __func__);
	return NFC_OK;
}

