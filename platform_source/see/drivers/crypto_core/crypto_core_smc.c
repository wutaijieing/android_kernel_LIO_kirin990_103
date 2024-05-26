/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Drivers for Crypto Core smc process.
 * Create: 2019/11/15
 */
#include "crypto_core_smc.h"
#include <linux/arm-smccc.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <securec.h>
#include "crypto_core_internal.h"
#include "crypto_core_errno.h"
#include "crypto_core_upgrade.h"
#define MSPC_ATF_MSG_ACK_ERROR       0xdeadbeef

static struct mutex g_smc_mutex;
static struct completion g_smc_completion;
static bool g_smc_cmd_run = false;

static noinline int32_t mspc_send_atf_smc(uint64_t _function_id, uint64_t _arg0,
					  uint64_t _arg1, uint64_t _arg2)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct arm_smccc_res res = {0};
	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
		      0, 0, 0, 0, &res);
	return (int32_t)res.a0;
#else
	register uint64_t function_id asm("x0") = _function_id;
	register uint64_t arg0 asm("x1") = _arg0;
	register uint64_t arg1 asm("x2") = _arg1;
	register uint64_t arg2 asm("x3") = _arg2;

	asm volatile(
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc    #0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int32_t)function_id;
#endif
}

int32_t mspc_send_smc_no_ack(uint64_t fid, uint64_t smc_cmd,
			     const char *msg, uint32_t msg_len)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	uint64_t buff_size = SIZE_4K;
	int32_t ret;
	struct device *mspc_device = NULL;

	if (msg_len > buff_size) {
		pr_err("%s: msg_len is invalid!\n", __func__);
		ret = MSPC_INVALID_PARAMS_ERROR;
		mspc_record_errno(ret);
		return ret;
	}

	mspc_device = mspc_get_device();
	if (!mspc_device) {
		pr_err("%s:get devices failed!\n", __func__);
		ret = MSPC_CMA_DEVICE_INIT_ERROR;
		mspc_record_errno(ret);
		return ret;
	}

	buff_virt = (char *)dma_alloc_coherent(mspc_device, buff_size,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		ret = MSPC_NO_RESOURCES_ERROR;
		mspc_record_errno(ret);
		return ret;
	}
	(void)memset_s(buff_virt, buff_size, 0, buff_size);

	/* if need to send data, copy it to CMA buffer */
	if (msg) {
		if (memcpy_s(buff_virt, buff_size, msg, msg_len) != EOK) {
			pr_err("%s(): memcpy_s failed\n", __func__);
			ret = MSPC_SECUREC_ERROR;
			mspc_record_errno(ret);
			goto exit;
		}
	}

	ret = mspc_send_atf_smc(fid, smc_cmd, (u64)buff_phy, buff_size);
exit:
	dma_free_coherent(mspc_device, buff_size, buff_virt, buff_phy);
	return ret;
}

int32_t mspc_send_smc_process(struct mspc_atf_msg_header *p_msg_header,
			      uint64_t phy_addr, uint64_t size,
			      uint32_t timeout, uint64_t smc_cmd)
{
	int32_t ret;
	uint64_t local_jiffies;
	uint64_t func_id = MSPC_FN_MAIN_SERVICE_CMD;

	if (!p_msg_header) {
		pr_err("%s: msg_header is NULL\n", __func__);
		return MSPC_INVALID_PARAMS_ERROR;
	}
	if (smc_cmd == MSPC_SMC_CHANNEL_AUTOTEST)
		func_id = MSPC_FN_CHANNEL_TEST_CMD;

	mutex_lock(&g_smc_mutex);
	g_smc_cmd_run = true;
	ret = mspc_send_atf_smc(func_id, (uint64_t)smc_cmd, phy_addr, size);
	if (ret != MSPC_OK) {
		pr_err("%s: send smc to ATF failed, ret=%d\n", __func__, ret);
		ret = MSPC_FIRST_SMC_CMD_ERROR;
		goto exit;
	}

	local_jiffies = msecs_to_jiffies(timeout);
	if (smc_cmd == MSPC_SMC_CHANNEL_AUTOTEST)
		local_jiffies = (uint64_t)MAX_SCHEDULE_TIMEOUT;

	if (wait_for_completion_timeout(&g_smc_completion, local_jiffies) == 0) {
		pr_err("%s:wait seamphore timeout!\n", __func__);
		ret = MSPC_SMC_CMD_TIMEOUT_ERROR;
		goto exit;
	}

	if (p_msg_header->cmd != smc_cmd ||
	    p_msg_header->ack != MSPC_ATF_ACK_OK) {
		ret = MSPC_SMC_CMD_PROCESS_ERROR;
		pr_err("mspc:cmd=%llu %u, ack=%x, smc process error\n",
		       smc_cmd, p_msg_header->cmd, p_msg_header->ack);
	} else {
		ret = MSPC_OK;
		pr_err("%s: smc process successful!\n", __func__);
	}

exit:
	g_smc_cmd_run = false;
	mutex_unlock(&g_smc_mutex);
	mspc_record_errno(ret);
	return ret;
}

void mspc_set_atf_msg_header(struct mspc_atf_msg_header *header,
			     uint32_t cmd_type)
{
	if (!header) {
		pr_err("%s:NULL pointer! cmd:%x\n", __func__, cmd_type);
		return;
	}

	header->ack = MSPC_ATF_ACK_ERROR;
	header->cmd = cmd_type;
	header->result_size = 0;
	header->result_addr = 0;
}

void general_see_active(void)
{
	if (g_smc_cmd_run)
		complete(&g_smc_completion);
	else
		pr_err("%s:Receive an unexpected IPI from ATF", __func__);
}
EXPORT_SYMBOL(general_see_active);

void mspc_init_smc(void)
{
	mutex_init(&g_smc_mutex);
	init_completion(&g_smc_completion);
}
