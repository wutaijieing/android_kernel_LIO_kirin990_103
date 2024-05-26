/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
 *  Description: eFuse driver test code
 *  Create : 2017/9/15
 */

#include <linux/err.h>
#include <platform_include/see/efuse_driver.h>
#include <linux/kernel.h>
#include <linux/arm-smccc.h>
#include <securec.h>
#include "socid.h"

/* enable and disable debug mode */
#define EFUSE_FN_ENABLE_DEBUG             0xCA000048
#define EFUSE_FN_DISABLE_DEBUG            0xCA000049

uint32_t efuse_read_value_tc(uint32_t item_vid, uint32_t buf_size)
{
	uint32_t ret;
	uint32_t i;
	uint32_t buf[128] = { 0 };
	struct efuse_desc_t desc = { 0 };

	if (buf_size > 128) {
		pr_err("over buf_size\n");
		return ERROR;
	}
	desc.buf = buf;
	desc.buf_size = buf_size;
	desc.item_vid = item_vid;
	ret = efuse_read_value_t(&desc);
	for (i = 0; i < buf_size; i++)
		pr_err("0x%x\n", buf[i]);

	return ret;
}

s32 efuse_test_read_chipid(void)
{
	/* chipid has 2-groups efuse, so it's a 8-bytes value */
	u32 test_buf_chipid[2] = { 0 };

	if (get_efuse_chipid_value((u8 *)test_buf_chipid, 8,
				   EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("get_efuse_chipid_value failed in %s\n", __func__);
		return ERROR;
	}
	pr_err("chipid_0=0x%x, chipid_1=0x%x\n", test_buf_chipid[0],
	       test_buf_chipid[1]);
	return OK;
}

s32 efuse_test_read_partial_pass_info(void)
{
	/* chipid has 2-groups efuse, so it's a 8-bytes value */
	u32 lite_pass_info[2] = { 0 };

	if (get_partial_pass_info((u8 *)lite_pass_info, 3,
				  EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("get_partial_pass_info failed in %s\n", __func__);
		return ERROR;
	}
	pr_err("info0=0x%x, info1=0x%x\n",
	       lite_pass_info[0], lite_pass_info[1]);
	return OK;
}

s32 efuse_test_read_avs_value(u32 size)
{
	u32 test_buf_avs = 0;

	if (get_efuse_avs_value((u8 *)&test_buf_avs, size,
				EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("get_efuse_avs_value failed in %s\n", __func__);
		return ERROR;
	}
	pr_err("avs value is 0x%x\n", test_buf_avs);

	return OK;
}

s32 efuse_test_read_deskew_value(void)
{
	u32 test_buf_deskew = 0;

	if (get_efuse_deskew_value((u8 *)&test_buf_deskew, 1,
				   EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("get_efuse_deskew_value failed in %s\n", __func__);
		return ERROR;
	}
	pr_err("deskew value is 0x%x\n", test_buf_deskew);

	return OK;
}

s32 efuse_test_write_kce(void)
{
	/* modem huk has 4-groups efuse, so it's a 16-bytes value */
	s32 test_buf_kce[4] = { 0x100, 0x40, 0x40, 0x40 };

	if (set_efuse_kce_value((u8 *)test_buf_kce, 4 * sizeof(s32),
				EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("set_efuse_kce_value failed in %s\n", __func__);
		return ERROR;
	}
	pr_err("kce_0=0x%x, kce_1=0x%x, kce_2=0x%x, kce_3=0x%x\n",
	       test_buf_kce[0], test_buf_kce[1], test_buf_kce[2],
	       test_buf_kce[3]);
	return OK;
}

s32 efuse_update_nvcnt_tc(void)
{
	s32 ret;
	uint32_t nv = 0;

	ret = efuse_update_nvcnt((u8 *)(&nv), sizeof(nv));
	pr_err("[%s]: nvcnt = %d\n", __func__, nv);
	return nv;
}

#ifdef CONFIG_GENERAL_SEE
s32 efuse_test_write_read_hisee(void)
{
	/* hisee has 2-groups efuse, so it's a 8-bytes value */
	u8 test_buf_hisee[8] = {
		0x78, 0x56, 0x34, 0x12,
		0xaa, 0xbb, 0xee, 0xff
	};

	if (set_efuse_hisee_value(test_buf_hisee, 8,
				  EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("set_efuse_hisee_value failed in %s\n", __func__);
		return ERROR;
	}

	if (get_efuse_hisee_value(test_buf_hisee, 8,
				  EFUSE_TIMEOUT_SECOND) != 0) {
		pr_err("get_efuse_hisee_value failed in %s\n", __func__);
		return ERROR;
	}
	return OK;
}
#endif

s32 efuse_read_tc(u32 func_id)
{
	s32 ret;
	u32 read_buf[EFUSE_BUFFER_MAX_NUM] = {0};
	u32 i;

	/* read */
	ret = efuse_read_value(read_buf, EFUSE_BUFFER_MAX_NUM, func_id);
	if (ret != OK) {
		pr_err("read_efuse_value failed in %s\n", __func__);
		return ERROR;
	}
	for (i = 0; i < EFUSE_BUFFER_MAX_NUM; i++)
		pr_err("read_efuse_value is 0x%x\n", read_buf[i]);
	return OK;
}

s32 efuse_write_tc(u32 func_id)
{
	s32 ret;
	u32 write_buf[EFUSE_BUFFER_MAX_NUM] = {0};
	u32 i;

	/* write, set efuse group 0xFFFFFFFF */
	for (i = 0; i < EFUSE_BUFFER_MAX_NUM; i++)
		write_buf[i] = 0xFFFFFFFF;
	ret = efuse_write_value(write_buf, EFUSE_BUFFER_MAX_NUM, func_id);
	if (ret != OK) {
		pr_err("write_efuse_value failed in %s\n", __func__);
		return ERROR;
	}
	return OK;
}

s32 efuse_set_debug_tc(u32 debug_mode)
{
	s32 ret;
	u32 write_buf = 0;
	u32 buf_size = 1;
	u32 func_id;

	/* 0 for debug_mode off */
	/* 1 for debug_mode on */
	if (debug_mode == 0) {
		write_buf = 0x0;
		func_id = EFUSE_FN_DISABLE_DEBUG;
	} else if (debug_mode == 1) {
		write_buf = 0x1;
		func_id = EFUSE_FN_ENABLE_DEBUG;
	} else {
		pr_err("unsupport debug mode!%s\n", __func__);
		return ERROR;
	}
	ret = efuse_write_value(&write_buf, buf_size, func_id);
	if (ret != OK) {
		pr_err("write_efuse_value failed in %s\n", __func__);
		return ERROR;
	}
	return OK;
}

s32 efuse_forge_smc_test(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res;
	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
			0, 0, 0, 0, &res);
	return (int)res.a0;
}

void get_socid_value_test(void)
{
	s32 ret;
	u32 i, offset;
	u8 socid[EFUSE_SOCID_LENGTH_BYTES] = {0};
	u8 socid_str[EFUSE_SOCID_LENGTH_BYTES * 2 + 1] = {0};

	ret = get_socid(socid, sizeof(socid));
	if (ret != 0) {
		pr_err("error, get_socid\n");
		return;
	}
	for (offset = 0, i = 0; i < sizeof(socid); i++, offset += 2) {
		ret = snprintf_s(
			socid_str + offset, sizeof(socid_str) - offset,
			2, "%02x", (u32)socid[i]);
		if (ret < 0) {
			pr_err("error,snprintf_s socid[%u]\n", i);
			return;
		}
	}
	pr_err("socid is %s\n", socid_str);
}
