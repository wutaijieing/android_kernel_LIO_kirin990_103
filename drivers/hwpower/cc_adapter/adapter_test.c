// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_test.c
 *
 * test of adapter driver
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
 *
 */

#include <chipset_common/hwpower/adapter/adapter_test.h>
#include <chipset_common/hwpower/adapter/adapter_detect.h>
#include <chipset_common/hwpower/charger/charger_common_interface.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <huawei_platform/power/direct_charger/direct_charger.h>

#define HWLOG_TAG adapter_test
HWLOG_REGIST();

static struct adapter_test_attr g_adapter_test_attr_tbl[] = {
	{ AT_TYPE_LVC, "SCP", AT_DETECT_FAIL },
	{ AT_TYPE_FCP, "FCP", AT_DETECT_FAIL },
	{ AT_TYPE_PD, "PD", AT_DETECT_FAIL },
	{ AT_TYPE_SC, "SC", AT_DETECT_FAIL },
};

void adapter_test_clear_result(void)
{
	int i;
	struct adapter_test_attr *tbl = g_adapter_test_attr_tbl;
	int size = ARRAY_SIZE(g_adapter_test_attr_tbl);

	hwlog_info("clear_result\n");

	for (i = 0; i < size; i++)
		tbl[i].result = AT_DETECT_FAIL;
}

void adapter_test_set_result(unsigned int type, unsigned int result)
{
	int i;
	struct adapter_test_attr *tbl = g_adapter_test_attr_tbl;
	int size = ARRAY_SIZE(g_adapter_test_attr_tbl);

	for (i = 0; i < size; i++) {
		if (tbl[i].type != type)
			continue;

		tbl[i].result = result;
		hwlog_info("set_result, type=%u result=%u\n", type, result);
		return;
	}

	if (i == size)
		hwlog_err("type invalid, type=%u result=%u\n", type, result);
}

#ifdef CONFIG_DIRECT_CHARGER
int adapter_test_get_result(char *buf, unsigned int buf_len)
{
	int i;
	char tmp_buf[AT_BUF_LEN] = { 0 };
	int tmp_size;
	int wr_size = 0;
	int rd_size = 0;
	int unused;
	struct adapter_test_attr *tbl = g_adapter_test_attr_tbl;
	int size = ARRAY_SIZE(g_adapter_test_attr_tbl);

	if (!buf)
		return -EINVAL;

	for (i = 0; i < size; i++) {
		if (!(direct_charge_get_local_mode() & SC_MODE) &&
			(tbl[i].type == AT_TYPE_SC))
			continue;

		if (i != size - 1)
			wr_size += snprintf(tmp_buf, AT_BUF_LEN, "%s:%d,",
				tbl[i].type_name, tbl[i].result);
		else
			wr_size += snprintf(tmp_buf, AT_BUF_LEN, "%s:%d\n",
				tbl[i].type_name, tbl[i].result);

		unused = buf_len - strlen(buf);
		tmp_size = strlen(tmp_buf);
		if (unused > tmp_size)
			strncat(buf, tmp_buf, tmp_size);
		else
			strncat(buf, tmp_buf, unused);
		memset(tmp_buf, 0, AT_BUF_LEN);

		rd_size += (strlen(tbl[i].type_name) + AT_SUFFIX_LEN);
		if (wr_size != rd_size) {
			wr_size = -EINVAL;
			break;
		}
	}
	buf[strlen(buf) - 1] = '\n';
	hwlog_info("wr_size=%d, rd_size=%d, buf=%s\n", wr_size, rd_size, buf);

	return wr_size;
}
#else
int adapter_test_get_result(char *buf, unsigned int buf_len)
{
	if (!buf)
		return -EINVAL;

	return 0;
}
#endif /* CONFIG_DIRECT_CHARGER */

int adapter_test_check_protocol_type(unsigned int prot_type)
{
	adapter_detect_init_protocol_type();

	if (!adapter_detect_check_protocol_type(prot_type)) {
		hwlog_err("prot_type %u is invalid\n", prot_type);
		return -EINVAL;
	}

	return 0;
}

int adapter_test_check_support_mode(unsigned int prot_type)
{
	int ret;
	int adp_mode = ADAPTER_SUPPORT_UNDEFINED;

	ret = adapter_get_support_mode(prot_type, &adp_mode);
	hwlog_info("prot_type=%u adp_mode=%x ret=%d\n", prot_type, adp_mode, ret);

	if (ret || (adp_mode == ADAPTER_SUPPORT_UNDEFINED))
		return -EINVAL;

	return 0;
}

int adapter_test_match_adp_type(unsigned int prot_type)
{
	int result;
	unsigned int chg_type = charge_get_charger_type();

	/* judge whether support specified protocol */
	if (adapter_test_check_protocol_type(prot_type)) {
		result = AT_RESULT_NOT_SUPPORT;
		goto end;
	}

	/* to avoid the usb port cutoff when CTS test */
	if ((chg_type == CHARGER_TYPE_USB) || (chg_type == CHARGER_TYPE_BC_USB)) {
		result = AT_RESULT_FAIL;
		goto end;
	}

	/* judge whether support specified adapter mode */
	if (adapter_test_check_support_mode(prot_type))
		result = AT_RESULT_FAIL;
	else
		result = AT_RESULT_SUCC;

end:
	hwlog_info("prot_type=%u chg_type=%u result=%d\n",
		prot_type, chg_type, result);
	return result;
}
