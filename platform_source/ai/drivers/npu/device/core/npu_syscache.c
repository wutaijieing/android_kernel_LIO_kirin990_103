/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: syscache api
 * Create: 2021-07-06
 */

#include <linux/types.h>
#include "npu_common.h"
#include "npu_proc_ctx.h"
#include "npu_adapter.h"
#include "npu_syscache.h"

int npu_ioctl_attach_syscache(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret = 0;
	struct npu_attach_sc msg = {0};
	unused(proc_ctx);

	ret = copy_from_user_safe(&msg, (void __user *)(uintptr_t)arg, sizeof(msg));
	cond_return_error(ret != 0, -EINVAL,
		"fail to copy npu sc attach params, ret = %d\n", ret);

	/* syscahce attach interface with offset */
	ret = npu_plat_attach_sc(msg.fd, msg.offset, (size_t)msg.size);
	cond_return_error(ret != 0, -EINVAL,
		"fail to devdrV_plat_attach_sc, ret = %d\n", ret);

	return ret;
}
int npu_ioctl_set_sc_prio(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret = 0;
	u32 prio = 0;
	unused(proc_ctx);

	ret = copy_from_user_safe(&prio, (void __user *)(uintptr_t)arg, sizeof(prio));
	cond_return_error(ret != 0, -EINVAL,
		"fail to copy npu sc prio params, ret = %d\n", ret);

	ret = npu_plat_set_sc_prio(prio);
	cond_return_error(ret != 0, -EINVAL,
		"fail to npu_plat_set_sc_prio, ret = %d\n", ret);

	return ret;
}

int npu_ioctl_switch_sc(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret = 0;
	u32 switch_sc = 0;
	unused(proc_ctx);

	ret = copy_from_user_safe(&switch_sc,
		(void __user *)(uintptr_t)arg, sizeof(switch_sc));
	cond_return_error(ret != 0, -EINVAL,
		"fail to copy npu sc enable tag params, ret = %d\n", ret);

	ret = npu_plat_switch_sc(switch_sc);
	cond_return_error(ret != 0, -EINVAL,
		"fail to npu_plat_switch_sc, ret = %d\n", ret);

	return ret;
}