/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: implement the external interface for BootDetector kernel stage
 * Author: hucangjun
 * Create: 2020-12-25
 */

#include <linux/time.h>
#include <linux/securec.h>
#include <hwbootfail/chipsets/common/bootfail_common.h>
#include <hwbootfail/chipsets/bootfail_honghu.h>

#define BF_KERNEL_DETAIL_INFO "Bootfail in kernel stage"

static void get_boot_fail_time(unsigned long *rtc_time,
	unsigned int *boot_time)
{
	struct timeval tv;

	if (rtc_time == NULL || boot_time == NULL)
		return;

	do_gettimeofday(&tv);
	*rtc_time = tv.tv_sec;
	*boot_time = 0;
}

int process_bootfail(enum bootfail_errno bootfail_errno,
	enum suggest_recovery_method suggested_method,
	const char *detail_info)
{
	struct bootfail_proc_param param;
	unsigned int len;

	print_err("%s: %d\n", __func__, __LINE__);
	(void)memset_s(&param, sizeof(param), 0, sizeof(param));
	param.magic = BF_SW_MAGIC_NUM;
	param.binfo.bootfail_errno = bootfail_errno;
	param.binfo.stage = KERNEL_STAGE;
	param.binfo.suggest_recovery_method = suggested_method;
	if (suggested_method != METHOD_DO_NOTHING) {
		param.binfo.post_action = PA_REBOOT;
		print_info("%s: ready do action reboot\n", __func__);
	}

	get_boot_fail_time((unsigned long *)&param.binfo.rtc_time,
		(unsigned int *)&param.binfo.bootup_time);
	if (detail_info != NULL) {
		len = min((sizeof(param.detail_info) - 1), strlen(detail_info));
		if (strncpy_s(param.detail_info, sizeof(param.detail_info),
			detail_info, len) != EOK)
			print_err("strncpy_s detail_info failed\n");
	} else {
		if (strncpy_s(param.detail_info, sizeof(param.detail_info),
			BF_KERNEL_DETAIL_INFO, strlen(BF_KERNEL_DETAIL_INFO)) != EOK)
			print_err("strncpy_s default detail_info failed\n");
	}

	param.binfo.is_updated = 0;
	param.binfo.is_rooted = 0;
	(void)boot_fail_error(&param);

	return 0;
}