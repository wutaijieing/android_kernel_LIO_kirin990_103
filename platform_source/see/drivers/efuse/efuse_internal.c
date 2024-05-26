/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: efuse source file to provide drivers
 * Create: 2022/01/18
 */

#include <platform_include/see/efuse_driver.h>
#include "efuse_internal.h"
#include <linux/kernel.h>          /* pr_err */
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/arm-smccc.h>       /* arm_smccc_smc */
#else
#include <asm/compiler.h>
#endif

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
static uint32_t smc_switch_to_atf(struct smc_arg_wrap *arg)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct arm_smccc_res res;
	arm_smccc_smc(arg->x0, arg->x1, arg->x2, arg->x3,
			arg->x4, 0, 0, 0, &res);
	arg->x1 = res.a1;
	arg->x2 = res.a2;
	arg->x3 = res.a3;
	return (uint32_t)res.a0;
#else
	register u64 smc_fid asm("x0") = arg->x0;
	register u64 arg1 asm("x1") = arg->x1;
	register u64 arg2 asm("x2") = arg->x2;
	register u64 arg3 asm("x3") = arg->x3;
	register u64 arg4 asm("x4") = arg->x4;

	asm volatile(
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		__asmeq("%4", "x4")
		"smc    #0\n"
		: "+r" (smc_fid), "+r" (arg1)
		: "r" (arg2), "r" (arg3), "r"(arg4));
	arg->x1 = arg1;
	arg->x2 = arg2;
	arg->x3 = arg3;
	return (uint32_t)smc_fid;
#endif
}

/*
 * check params
 *
 * @param
 * trans              pointer of struct efuse_trans_t
 *
 * @return
 * EFUSE_OK           params ok
 * other              invalid params
 */
static uint32_t efuse_check_param(struct efuse_trans_t *trans)
{
	uint32_t tmp_len;
	uint32_t buf_size;
	uint32_t start_bit;
	uint32_t bit_cnt;

	if (trans == NULL || trans->buf == NULL) {
		pr_err ("[%s]: error, null pointer\n", __func__);
		return EFUSE_INVALID_PARAM_ERR;
	}

	buf_size = trans->buf_size;
	start_bit = trans->start_bit;
	bit_cnt = trans->bit_cnt;
	if (start_bit >= EFUSE_MAX_SIZE || bit_cnt > EFUSE_MAX_SIZE ||
	    bit_cnt == 0 || (start_bit > EFUSE_MAX_SIZE - bit_cnt)) {
		pr_err ("[%s]: error, invalid params, start_bit=%u, \
		       bit_cnt=%u\n", __func__, start_bit, bit_cnt);
		return EFUSE_INVALID_PARAM_ERR;
	}

	tmp_len = div_round_up(bit_cnt, EFUSE_BITS_PER_GROUP);
	if (buf_size < tmp_len) {
		pr_err ("[%s]: error, buf_size=%u\n", __func__, buf_size);
		return EFUSE_INVALID_PARAM_ERR;
	}

	return EFUSE_OK;
}

/*
 * combine the start_bit and bit_cnt params into one uint32_t because
 *     the params number of smc may be not enough.
 *
 * @param
 * start_bit          efuse start bit
 * bit_cnt            efuse bit cnt
 *
 * @return
 * combine result
 */
static uint32_t efuse_combine_bit_param(uint32_t start_bit, uint32_t bit_cnt)
{
	uint32_t result;

	result = (start_bit << EFUSE_BITCNT_ALLOC_BITS) | bit_cnt;
	return result;
}

/*
 * bottom layer function of read efuse value
 * only support not more than 32 bit efuse for each operation now
 *
 * @param
 * trans              pointer to struct efuse_trans_t
 *
 * @return
 * EFUSE_OK           operation succeed
 * other              error code
 */
static uint32_t efuse_do_read_value(struct efuse_trans_t *trans)
{
	uint32_t ret;
	struct smc_arg_wrap smc_args = { 0 };

	ret = efuse_check_param(trans);
	if (ret != EFUSE_OK || trans->bit_cnt > EFUSE_BITS_PER_GROUP) {
		pr_err ("[%s]: error, invalid params\n", __func__);
		return EFUSE_INVALID_PARAM_ERR;
	}

	smc_args.x0 = (uintptr_t)FID_BL31_KERNEL_READ_EFUSE;
	smc_args.x1 = (uintptr_t)trans->item_vid;
	smc_args.x2 = (uintptr_t)efuse_combine_bit_param(trans->start_bit, trans->bit_cnt);
	ret = smc_switch_to_atf(&smc_args);
	if (ret != EFUSE_OK) {
		pr_err("[%s]: error, smc failed, smc_id=0x%08X, ret=0x%08X, item_vid=0x%08X\n", \
		      __func__, smc_args.x0, ret, trans->item_vid);
		return EFUSE_SMC_PROC_ERR;
	}

	*(trans->buf) = smc_args.x1;
	return EFUSE_OK;
}

/*
 * get efuse item information by item_vid.
 *
 * @param
 * trans              pointer to struct efuse_trans_t
 *
 * @return
 * EFUSE_OK           operation succeed
 * other              error code
 */
uint32_t efuse_get_item_info(struct efuse_trans_t *trans)
{
	uint32_t ret;
	struct smc_arg_wrap smc_args = { 0 };

	smc_args.x0 = (uintptr_t)FID_BL31_KERNEL_GET_EFUSE_ITEM_INFO;
	smc_args.x1 = (uintptr_t)trans->item_vid;
	ret = smc_switch_to_atf(&smc_args);
	if (ret != EFUSE_OK) {
		pr_err("[%s]: error, smc failed, smc_id=0x%08X, ret=0x%08X, item_vid=0x%08X\n", \
		      __func__, smc_args.x0, ret, trans->item_vid);
		return EFUSE_SMC_PROC_ERR;
	}

	trans->bit_cnt = smc_args.x1;
	return EFUSE_OK;
}

/*
 * Inner layer function of read efuse value.
 *
 * In kernel, the caller doesn't need to know specific efuse layout, especially
 *     the real start bit and bit cnt. We only provide the item_vid to support the
 *     operation of the whole efuse item.
 *
 * However, there is no share memory between kernel and bl31, we cannot operate the
 *     whole efuse item when the item includes more than 1 group due to the length
 *     limition of smc parameters.
 *
 * Hence, we need to divide the item into several parts so that each part could
 *     be operated with a uint32_t. Therefore
 * 1.we need to know the real bit cnt by item_vid first,
 * 2.use the start_bit and bit_cnt parameters to mark different parts.
 *
 * The start_bit here is a concept of offset in the corresponding efuse item and will
 *     be mapped to the real start_bit in atf. See below:
 *
 *   kernel:            +<--------+ bit_cnt +------->+
 *                      |                            |
 *                      |       start_bit=32     start_bit=0
 *                      |            |               |
 *                      +<-cur_cnt-->+<---cur_cnt--->+               +
 *                      |            |               |
 *                      v            v               v
 *   +---------------+-------+-------+---------------+
 *   |               |               |               |
 *   +---------------+---------------+---------------+
 *   95             64              32               0
 *                           +
 *                           |
 *                           v
 *
 *   atf:         +<--------+ bit_cnt +------->+
 *                |                            |
 *                |        start_bit       start_bit
 *                |            |               |
 *                +<-cur_cnt-->+<---cur_cnt--->+                    +
 *                |            |               |
 *                v            v               v
 *   +------------+--+---------+-----+---------+-----+
 *   |               |               |               |
 *   +---------------+---------------+---------------+
 *   95             64              32               0
 *
 *   @param desc      pointer to struct efuse_desc_t
 *
 *   @return
 *   EFUSE_OK         operation succeed
 *   other            error code
 */
uint32_t efuse_inner_read_value(struct efuse_desc_t *desc)
{
	uint32_t efuse_val;
	uint32_t start_bit;
	uint32_t bit_cnt;
	uint32_t cur_cnt;
	uint32_t grp_cnt;
	uint32_t ret;
	struct efuse_trans_t trans = { 0 };

	if (desc == NULL) {
		pr_err ("[%s]: error, desc is null\n", __func__);
		return EFUSE_INVALID_PARAM_ERR;
	}

	/* 1. get the real bit_cnt of efuse item */
	trans.item_vid = desc->item_vid;
	ret = efuse_get_item_info(&trans);
	if (ret != EFUSE_OK) {
		pr_err ("[%s]: error, get efuse item info, ret=0x%08X\n", __func__, ret);
		return ret;
	}

	/* 2. check the input param to make sure buf is big enough */
	trans.buf = desc->buf;
	trans.buf_size = desc->buf_size;
	ret = efuse_check_param(&trans);
	if (ret != EFUSE_OK) {
		pr_err ("[%s]: error, invalid params\n", __func__);
		return ret;
	}

	/* 3. set trans paras to operate a u32 each time */
	trans.buf = &efuse_val;
	trans.buf_size = sizeof(efuse_val) / sizeof(uint32_t);

	/* 4. iteratively read each cur_cnt */
	bit_cnt = trans.bit_cnt;
	start_bit = 0;
	grp_cnt = 0;
	while (bit_cnt > 0) {
		if (bit_cnt > EFUSE_BITS_PER_GROUP) {
			cur_cnt = EFUSE_BITS_PER_GROUP;
		} else {
			cur_cnt = bit_cnt;
		}

		trans.start_bit = start_bit;
		trans.bit_cnt = cur_cnt;
		ret = efuse_do_read_value(&trans);
		if (ret != EFUSE_OK) {
			(void)memset_s(desc->buf, desc->buf_size * sizeof(uint32_t), 0, \
				       desc->buf_size * sizeof(uint32_t));
			pr_err ("[%s]: error, read efuse start_bit=%u, cur_cnt=%u\n", \
				__func__, start_bit, cur_cnt);
			return ret;
		}

		desc->buf[grp_cnt++] = efuse_val;
		start_bit += cur_cnt;
		bit_cnt -= cur_cnt;
	}

	/* return the real buf_size to the caller */
	desc->buf_size = grp_cnt;
	return EFUSE_OK;
}
