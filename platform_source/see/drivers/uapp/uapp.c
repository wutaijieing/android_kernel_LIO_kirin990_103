/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: uapp source file
 * Create: 2022/04/20
 */

#include <platform_include/see/uapp.h>
#include <platform_include/see/efuse_driver.h>
#include "uapp_internal.h"
#include <linux/kernel.h>          /* pr_err */
#include <linux/version.h>
#include <linux/arm-smccc.h>       /* arm_smccc_smc */
#include <securec.h>               /* memset_s */

struct smc_arg_wrap {
	uintptr_t x0;
	uintptr_t x1;
	uintptr_t x2;
	uintptr_t x3;
	uintptr_t x4;
};

/*
 * adapt local smc function
 */
static u32 smc_switch_to_atf(struct smc_arg_wrap *arg)
{
	struct arm_smccc_res res;
	arm_smccc_smc(arg->x0, arg->x1, arg->x2, arg->x3,
			arg->x4, 0, 0, 0, &res);
	arg->x1 = res.a1;
	arg->x2 = res.a2;
	arg->x3 = res.a3;
	return (u32)res.a0;
}

/*
 * get uapp enable state
 *
 * @param
 * state              pointer to save state
 *                    [0-DISABLE] [1-ENABLE] [other-INVALID]
 * @return
 * UAPP_OK            operation succeed
 * other              error code
 */
u32 uapp_get_enable_state(u32 *state)
{
	u32 ret;
	u32 val = 0;
	struct efuse_desc_t desc = { 0 };

	if (!state) {
		pr_err ("[%s]: error, state NULL\n", __func__);
		return UAPP_NULL_PTR_ERR;
	}

	desc.buf = &val;
	desc.buf_size = sizeof(val) / sizeof(u32);
	desc.item_vid = EFUSE_KERNEL_UAPP_ENABLE;
	ret = efuse_read_value_t(&desc);
	if (ret != EFUSE_OK) {
		pr_err("[%s]: error, read efuse ret=0x%08X", __func__, ret);
		return UAPP_READ_EFUSE_ERR;
	}

	*state = (val == 0 ? UAPP_DISABLE : UAPP_ENABLE);
	return UAPP_OK;
}

/*
 * set uapp enable state
 *
 * @param
 * state              the state to set
 *                    [0-DISABLE] [1-ENABLE] [other-INVALID]
 * @return
 * UAPP_OK            operation succeed
 * other              error code
 */
u32 uapp_set_enable_state(u32 state)
{
	u32 ret;
	struct smc_arg_wrap smc_args = { 0 };

	if (state >= UAPP_ENABLE_STATE_MAX) {
		pr_err("[%s]: error, invalid state=0x%08X", __func__, state);
		return UAPP_INVALID_PARAM_ERR;
	}

	smc_args.x0 = (uintptr_t)FID_UAPP_SET_ENABLE_STATE;
	smc_args.x1 = (uintptr_t)state;
	ret = smc_switch_to_atf(&smc_args);
	if (ret != UAPP_OK) {
		pr_err("[%s]: error, smc failed, smc_id=0x%08X, ret=0x%08X", \
		      __func__, smc_args.x0, ret);
		return UAPP_SMC_PROC_ERR;
	}

	return UAPP_OK;
}

/*
 * valid bindfile authkey by key_idx
 *
 * @param
 * key_idx            the key_idx to valid
 *
 * @return
 * UAPP_OK            operation succeed
 * other              error code
 */
u32 uapp_valid_bindfile_pubkey(u32 key_idx)
{
	u32 ret;
	u32 val = 0;
	struct efuse_desc_t desc = { 0 };

	if (key_idx >= UAPP_BINDFILE_PUBKEY_INDEX_MAX) {
		pr_err("[%s]: error, invalid key_idx=0x%08X", __func__, key_idx);
		return UAPP_INVALID_PARAM_ERR;
	}

	desc.buf = &val;
	desc.buf_size = sizeof(val) / sizeof(u32);
	desc.item_vid = EFUSE_KERNEL_UAPP_KEYREVOKE;
	ret = efuse_read_value_t(&desc);
	if (ret != EFUSE_OK) {
		pr_err("[%s]: error, read efuse ret=0x%08X", __func__, ret);
		return UAPP_READ_EFUSE_ERR;
	}

	if ((val & (1u << key_idx)) != 0) {
		pr_err("[%s]: error, uapp key_idx=0x%08X is revoked\n", __func__, key_idx);
		return UAPP_AUTHKEY_IS_REVOKED_ERR;
	}

	return UAPP_OK;
}

/*
 * uapp revoke certain bindfile authkey by key_idx
 *
 * @param
 * key_idx            the key_idx to revoke
 *
 * @return
 * UAPP_OK            operation succeed
 * other              error code
 */
u32 uapp_revoke_bindfile_pubkey(u32 key_idx)
{
	u32 ret;
	struct smc_arg_wrap smc_args = { 0 };

	if (key_idx >= UAPP_BINDFILE_PUBKEY_INDEX_MAX) {
		pr_err("[%s]: error, invalid key_idx=0x%08X", __func__, key_idx);
		return UAPP_INVALID_PARAM_ERR;
	}

	smc_args.x0 = (uintptr_t)FID_UAPP_REVOKE_BINDFILE_PUBKEY;
	smc_args.x1 = (uintptr_t)key_idx;
	ret = smc_switch_to_atf(&smc_args);
	if (ret != UAPP_OK) {
		pr_err("[%s]: error, smc failed, smc_id=0x%08X, ret=0x%08X", \
		      __func__, smc_args.x0, ret);
		return UAPP_SMC_PROC_ERR;
	}

	return UAPP_OK;
}

#define SHA256_BYTES   0x20
u32 uapp_get_rotpk_hash(struct rotpk_hash *rotpk_hash)
{
	u32 ret;
	errno_t err;
	uint32_t read_size = 0;
	struct efuse_desc_t desc = { 0 };
	uint32_t buf[SHA256_BYTES / sizeof(uint32_t)] = { 0 };

	if (!rotpk_hash) {
		pr_err("[%s]: error, rotpk_hash NULL\n", __func__);
		return UAPP_NULL_PTR_ERR;
	}

	desc.buf = buf;
	desc.buf_size = ARRAY_SIZE(buf);
	desc.item_vid = EFUSE_KERNEL_ROTPK_HASH0;
	ret = efuse_read_value_t(&desc);
	if (ret != EFUSE_OK) {
		pr_err("[%s]: error 0x%x, read rotph hash0", __func__, ret);
		return UAPP_READ_EFUSE_ERR;
	}

	/* add the read size of first part */
	read_size += desc.buf_size;

	desc.buf = buf + read_size;
	desc.buf_size = ARRAY_SIZE(buf) - read_size;
	desc.item_vid = EFUSE_KERNEL_ROTPK_HASH1;
	ret = efuse_read_value_t(&desc);
	if (ret != EFUSE_OK) {
		pr_err("[%s]: error 0x%x, read rotph hash1", __func__, ret);
		return UAPP_READ_EFUSE_ERR;
	}

	/* add the read size of second part */
	read_size += desc.buf_size;
	err = memcpy_s(rotpk_hash->val, sizeof(rotpk_hash->val), \
			buf, read_size * sizeof(uint32_t));
	if (err != EOK) {
		pr_err("[%s]: error 0x%x, cp rotph hash", __func__, err);
		return UAPP_READ_EFUSE_ERR;
	}

	/* get the total size of read data */
	rotpk_hash->bytes = read_size * sizeof(uint32_t);
	return UAPP_OK;
}
