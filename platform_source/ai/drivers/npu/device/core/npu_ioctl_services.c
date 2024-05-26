/*
 * npu_ioctl_service.c
 *
 * about npu ioctl service
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include "npu_ioctl_services.h"

#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/swap.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <linux/mman.h>
#include <securec.h>
#include <linux/platform_drivers/mm_svm.h>

#include "npu_common.h"
#include "npu_calc_channel.h"
#include "npu_calc_cq.h"
#include "npu_stream.h"
#include "npu_shm.h"
#include "npu_log.h"
#include "npu_mailbox.h"
#include "npu_event.h"
#include "npu_hwts_event.h"
#include "npu_model.h"
#include "npu_task.h"
#include "bbox/npu_dfx_black_box.h"
#include "npu_heart_beat.h"
#include "npu_adapter.h"
#include "npu_calc_sq.h"
#include "npu_comm_sqe_fmt.h"
#include "npu_sink_sqe_fmt.h"
#include "npu_pool.h"
#include "npu_iova.h"
#include "npu_svm.h"
#include "npu_doorbell.h"
#include "npu_message.h"
#include "npu_pm_framework.h"
#include "npu_rt_task.h"
#include "npu_sink_task_verify.h"
#ifdef CONFIG_NPU_SWTS
#include "schedule_interface.h"
#endif

int npu_ioctl_alloc_stream(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	npu_stream_alloc_ioctl_info_t *para =
		(npu_stream_alloc_ioctl_info_t *)((uintptr_t)arg);
	npu_stream_alloc_ioctl_info_t para_1 = {0};
	struct npu_stream_info *stream_info = NULL;

	npu_drv_debug("enter\n");
	mutex_lock(&proc_ctx->stream_mutex);

	if (copy_from_user_safe(&para_1, (void __user *)(uintptr_t)arg,
		sizeof(npu_stream_alloc_ioctl_info_t))) {
		npu_drv_err("copy from user safe error\n");
		mutex_unlock(&proc_ctx->stream_mutex);
		return -EFAULT;
	}

	ret = npu_proc_alloc_stream(proc_ctx, &stream_info, para_1.strategy,
		para_1.priority);
	if (ret != 0) {
		npu_drv_err("npu alloc stream failed\n");
		mutex_unlock(&proc_ctx->stream_mutex);
		return -ENOKEY;
	}

	if (copy_to_user_safe((void __user *)(&(para->stream_id)),
		&stream_info->id, sizeof(int))) {
		npu_drv_err("copy to user safe stream_id = %d error\n",
			stream_info->id);
		ret = npu_proc_free_stream(proc_ctx, stream_info->id);
		if (ret != 0)
			npu_drv_err("npu_ioctl_free_stream_id = %d error\n",
				stream_info->id);
		mutex_unlock(&proc_ctx->stream_mutex);
		return -EFAULT;
	}
	bitmap_set(proc_ctx->stream_bitmap, stream_info->id, 1);
	mutex_unlock(&proc_ctx->stream_mutex);

	npu_drv_debug("end\n");
	return 0;
}

int npu_ioctl_alloc_event(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	u32 event_id = 0;
	u32 max_event_id = 0;
	int ret;
	u16 strategy;
	npu_event_alloc_ioctl_info_t para = {0};
	npu_event_alloc_ioctl_info_t *para_1 =
		(npu_event_alloc_ioctl_info_t *)((uintptr_t)arg);
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -EINVAL, "get plat info fail\n");

	mutex_lock(&proc_ctx->event_mutex);
	if (copy_from_user_safe(&para, (void __user *)(uintptr_t)arg,
		sizeof(npu_event_alloc_ioctl_info_t))) {
		npu_drv_err("copy from user safe error\n");
		mutex_unlock(&proc_ctx->event_mutex);
		return -1;
	}

	if (para.strategy >= EVENT_STRATEGY_TS) {
		npu_drv_err("proc alloc event failed, invalid input strategy: %u\n",
			para.strategy);
		mutex_unlock(&proc_ctx->event_mutex);
		return -EINVAL;
	} else if (para.strategy == EVENT_STRATEGY_SINK &&
		plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1) {
		strategy = EVENT_STRATEGY_HWTS;
		max_event_id = NPU_MAX_HWTS_EVENT_ID;
	} else {
		strategy = EVENT_STRATEGY_TS;
		max_event_id = NPU_MAX_EVENT_ID;
	}

	ret = npu_proc_alloc_event(proc_ctx, &event_id, strategy);
	if (ret != 0) {
		npu_drv_err("proc alloc event failed, event id: %u\n", event_id);
		mutex_unlock(&proc_ctx->event_mutex);
		return -1;
	}

	if (copy_to_user_safe((void __user *)(&(para_1->event_id)), &event_id,
		sizeof(int))) {
		npu_drv_err("copy to user safe event_id = %u error\n", event_id);
		if (event_id != max_event_id) {
			ret = npu_proc_free_event(proc_ctx, event_id, strategy);
			if (ret != 0) {
				npu_drv_err("proc free event id failed, event id: %u\n",
					event_id);
				mutex_unlock(&proc_ctx->event_mutex);
				return -1;
			}
			mutex_unlock(&proc_ctx->event_mutex);
			return -1;
		}
	}

	if (strategy == EVENT_STRATEGY_HWTS)
		bitmap_set(proc_ctx->hwts_event_bitmap, event_id, 1);
	else
		bitmap_set(proc_ctx->event_bitmap, event_id, 1);

	mutex_unlock(&proc_ctx->event_mutex);
	return 0;
}

int npu_ioctl_alloc_model(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	u32 model_id = 0;
	int ret;

	mutex_lock(&proc_ctx->model_mutex);
	ret = npu_proc_alloc_model(proc_ctx, &model_id);
	if (ret != 0) {
		npu_drv_err("proc alloc model failed, model id: %d\n", model_id);
		mutex_unlock(&proc_ctx->model_mutex);
		return -EFAULT;
	}

	if (copy_to_user_safe((void *)(uintptr_t)arg, &model_id, sizeof(int))) {
		npu_drv_err("copy to user safe model_id = %d error\n", model_id);
		if (model_id != NPU_MAX_MODEL_ID) {
			ret = npu_proc_free_model(proc_ctx, model_id);
			if (ret != 0) {
				npu_drv_err("proc free model id failed, model id: %d\n",
					model_id);
				mutex_unlock(&proc_ctx->model_mutex);
				return -EFAULT;
			}
			mutex_unlock(&proc_ctx->model_mutex);
			return -EFAULT;
		}
	}

	bitmap_set(proc_ctx->model_bitmap, model_id, 1);
	mutex_unlock(&proc_ctx->model_mutex);
	return 0;
}

int npu_ioctl_alloc_task(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	u32 task_id = 0;
	int ret;

	mutex_lock(&proc_ctx->task_mutex);
	ret = npu_proc_alloc_task(proc_ctx, &task_id);
	if (ret != 0) {
		npu_drv_err("proc alloc task failed, task id: %d\n", task_id);
		mutex_unlock(&proc_ctx->task_mutex);
		return -EFAULT;
	}

	if (copy_to_user_safe((void *)(uintptr_t)arg, &task_id, sizeof(int))) {
		npu_drv_err("copy to user safe task_id = %d error\n", task_id);
		if (task_id != NPU_MAX_SINK_TASK_ID) {
			ret = npu_proc_free_task(proc_ctx, task_id);
			if (ret != 0) {
				npu_drv_err("proc free task id failed, task id: %d\n",
					task_id);
				mutex_unlock(&proc_ctx->task_mutex);
				return -EFAULT;
			}
			mutex_unlock(&proc_ctx->task_mutex);
			return -EFAULT;
		}
	}

	bitmap_set(proc_ctx->task_bitmap, task_id, 1);
	mutex_unlock(&proc_ctx->task_mutex);
	return 0;
}

int npu_ioctl_free_stream(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	struct npu_stream_free_ioctl_info para = {0};

	if (copy_from_user_safe(&para, (void *)(uintptr_t)arg, sizeof(para))) {
		npu_drv_err("copy from user safe error\n");
		return -EFAULT;
	}

	ret = npu_proc_check_stream_id(proc_ctx, para.stream_id);
	cond_return_error(ret != 0, -EFAULT, "check para fail\n");

	mutex_lock(&proc_ctx->stream_mutex);
	ret = npu_proc_free_stream(proc_ctx, para.stream_id);
	mutex_unlock(&proc_ctx->stream_mutex);
	cond_return_error(ret != 0, -EINVAL,
		"npu ioctl free stream_id = %d error\n", para.stream_id);

	return ret;
}

int npu_ioctl_free_event(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret;
	u16 strategy;
	npu_event_free_ioctl_info_t para = {0};
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -EINVAL, "get plat info fail\n");

	if (copy_from_user_safe(&para, (void *)(uintptr_t)arg, sizeof(para))) {
		npu_drv_err("copy from user safe error\n");
		return -EFAULT;
	}

	if (para.strategy >= EVENT_STRATEGY_TS) {
		npu_drv_err("strategy %u\n", para.strategy);
		return -EINVAL;
	} else if (para.strategy == EVENT_STRATEGY_SINK &&
		plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1) {
		ret = npu_proc_check_hwts_event_id(proc_ctx, para.event_id);
		cond_return_error(ret != 0, -EFAULT, "check para fail\n");
		strategy = EVENT_STRATEGY_HWTS;
	} else {
		ret = npu_proc_check_event_id(proc_ctx, para.event_id);
		cond_return_error(ret != 0, -EFAULT, "check para fail\n");
		strategy = EVENT_STRATEGY_TS;
	}

	mutex_lock(&proc_ctx->event_mutex);
	ret = npu_proc_free_event(proc_ctx, para.event_id, strategy);
	mutex_unlock(&proc_ctx->event_mutex);
	cond_return_error(ret != 0, -EINVAL,
		"free event id = %d error\n", para.event_id);

	return ret;
}

int npu_ioctl_free_model(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret;
	int model_id = 0;

	if (copy_from_user_safe(&model_id, (void *)(uintptr_t)arg, sizeof(int))) {
		npu_drv_err("copy from user safe error\n");
		return -EFAULT;
	}

	ret = npu_proc_check_model_id(proc_ctx, model_id);
	cond_return_error(ret != 0, -EFAULT, "check para fail\n");

	mutex_lock(&proc_ctx->model_mutex);
	ret = npu_proc_free_model(proc_ctx, model_id);
	mutex_unlock(&proc_ctx->model_mutex);
	cond_return_error(ret != 0, -EINVAL,
		"free model id = %d error\n", model_id);

	return ret;
}

int npu_ioctl_free_task(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret;
	int task_id = 0;

	if (copy_from_user_safe(&task_id, (void *)(uintptr_t)arg, sizeof(int))) {
		npu_drv_err("copy from user safe error\n");
		return -EFAULT;
	}

	ret = npu_proc_check_task_id(proc_ctx, task_id);
	cond_return_error(ret != 0, -EFAULT, "check para fail\n");

	mutex_lock(&proc_ctx->task_mutex);
	ret = npu_proc_free_task(proc_ctx, task_id);
	mutex_unlock(&proc_ctx->task_mutex);
	cond_return_error(ret != 0, -EINVAL,
		"free task id = %d error\n", task_id);

	return ret;
}

static int npu_ioctl_get_chip_info(u64 arg)
{
	int ret;
	struct npu_chip_info info = {0};
	struct npu_platform_info *plat_info = NULL;
	struct npu_mem_desc *l2_desc = NULL;

	npu_drv_debug("arg = 0x%llx\n", arg);

	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get plat_info failed\n");
		return -EFAULT;
	}

	l2_desc = &plat_info->dts_info.reg_desc[NPU_REG_L2BUF_BASE];
	if (l2_desc == NULL) {
		npu_drv_err("npu_plat_get_reg_desc failed\n");
		return -EFAULT;
	}
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_L2_BUFF] == 1) {
		info.l2_size = (uint32)l2_desc->len + 1; // becasue of dts will minus 1
	} else {
		info.l2_size = 0;
	}

	ret = copy_to_user_safe((void __user *)(uintptr_t)arg, &info, sizeof(info));
	if (ret != 0) {
		npu_drv_err("fail to copy chip_info params to user space,ret = %d\n",
			ret);
		return -EINVAL;
	}

	return ret;
}

static int npu_ioctl_get_svm_ssid(struct npu_dev_ctx *dev_ctx, u64 arg)
{
	int ret;
	struct process_info info = {0};
	u16 ssid = 0;
	uint64_t ttbr;
	uint64_t tcr;

	ret = copy_from_user_safe(&info, (void __user *)(uintptr_t)arg,
		sizeof(info));
	if (ret != 0) {
		npu_drv_err("fail to copy process_info params, ret = %d\n",
			ret);
		return -EINVAL;
	}

	ret = npu_get_ssid_bypid(dev_ctx->devid, current->tgid,
		info.vpid, &ssid, &ttbr, &tcr);
	if (ret != 0) {
		npu_drv_err("fail to get ssid, ret = %d\n", ret);
		return ret;
	}

	info.pasid = ssid;

	npu_drv_debug("pid=%d get ssid 0x%x\n", current->pid, info.pasid);

	ret = copy_to_user_safe((void __user *)(uintptr_t)arg, &info, sizeof(info));
	if (ret != 0) {
		npu_drv_err("fail to copy process info params to user space,"
			"ret = %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

int npu_ioctl_enter_workwode(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	uint32_t workmode;
	npu_work_mode_info_t work_mode_info = {0};
	struct npu_dev_ctx *dev_ctx = NULL;

	dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(dev_ctx == NULL, -1, "invalid dev ctx\n");

	ret = copy_from_user_safe(&work_mode_info,
		(void __user *)(uintptr_t)arg, sizeof(npu_work_mode_info_t));
	if (ret != 0) {
		npu_drv_err("fail to copy sec_mode_info params, ret = %d\n", ret);
		return -EINVAL;
	}
	workmode = work_mode_info.work_mode;
	cond_return_error(workmode >= NPU_WORKMODE_MAX, -EINVAL,
		"invalid work_mode = %u\n", workmode);

	dev_ctx->pm.work_mode_flags = work_mode_info.flags;

	npu_drv_debug("work mode %u flags 0x%x\n", workmode, work_mode_info.flags);
	ret = npu_pm_proc_ioctl_enter_wm(proc_ctx, dev_ctx, workmode);
	if (ret != 0) {
		npu_drv_err("fail to enter workwode: %u, ret = %d\n", workmode, ret);
		return -EINVAL;
	}

	return ret;
}

int npu_ioctl_exit_workwode(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	uint32_t workmode;
	struct npu_power_down_info power_down_info = {{0}};
	struct npu_dev_ctx *dev_ctx = NULL;

	dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(dev_ctx == NULL, -1, "invalid dev ctx\n");

	ret = copy_from_user_safe(&power_down_info,
		(void __user *)(uintptr_t)arg, sizeof(power_down_info));
	if (ret != 0) {
		npu_drv_err("fail to copy sec_mode_info params, ret = %d\n", ret);
		return -EINVAL;
	}

	workmode = power_down_info.secure_info.work_mode;
	npu_drv_debug("workmode = %d\n", workmode);
	cond_return_error(workmode >= NPU_WORKMODE_MAX, -EINVAL,
		"invalid workmode = %u\n", workmode);

	dev_ctx->pm.work_mode_flags = 0; /* clear the flag */

	ret = npu_pm_proc_ioctl_exit_wm(proc_ctx, dev_ctx, workmode);
	if (ret != 0) {
		npu_drv_err("fail to exit workwode: %d, ret = %d\n", workmode, ret);
		return ret;
	}
	return ret;
}

int npu_ioctl_set_limit(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret;
	u32 value;
	struct npu_limit_time_info limit_time_info = {0, 0};
	struct npu_dev_ctx *dev_ctx = NULL;

	dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(dev_ctx == NULL, -1, "invalid dev ctx\n");

	ret = copy_from_user_safe(&limit_time_info,
		(void __user *)(uintptr_t)arg, sizeof(limit_time_info));
	if (ret != 0) {
		npu_drv_err("fail to copy sec_mode_info params, ret = %d\n", ret);
		return -EINVAL;
	}

	value = limit_time_info.time_out;
	if (limit_time_info.type == NPU_LOW_POWER_TIMEOUT) {
		cond_return_error(value < NPU_IDLE_TIME_OUT_MIN_VALUE,
			-EINVAL, "value :%u is too small\n", value);
		dev_ctx->pm.npu_idle_time_out = value;
	} else if (limit_time_info.type == NPU_STREAM_SYNC_TIMEOUT) {
		cond_return_error(value < NPU_TASK_TIME_OUT_MIN_VALUE,
			-EINVAL, "value :%u is too small\n", value);
		cond_return_error(value > NPU_TASK_TIME_OUT_MAX_VALUE,
			-EINVAL, "value :%u is too big\n", value);
		dev_ctx->pm.npu_task_time_out = value;
	} else {
		ret = -1;
		npu_drv_err("limit type wrong!, type = %u\n", limit_time_info.type);
	}

	return ret;
}

int npu_ioctl_enable_feature(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	uint32_t feature_id;

	ret = copy_from_user_safe(&feature_id,
		(void __user *)(uintptr_t)arg, sizeof(uint32_t));
	if (ret != 0) {
		npu_drv_err("copy_from_user_safe error\n");
		return -EINVAL;
	}
	ret = npu_feature_enable(proc_ctx, feature_id, 1);

	return ret;
}

int npu_ioctl_disable_feature(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	uint32_t feature_id;

	ret = copy_from_user_safe(&feature_id,
		(void __user *)(uintptr_t)arg, sizeof(uint32_t));
	if (ret != 0) {
		npu_drv_err("copy_from_user_safe error\n");
		return -EINVAL;
	}

	ret = npu_feature_enable(proc_ctx, feature_id, 0);
	return ret;
}

#ifdef CONFIG_NPU_HWTS
static int npu_verify_model_task_info(npu_model_desc_t *model_desc, u8 idx)
{
	if ((model_desc->stream_id[idx] <
		(NPU_MAX_NON_SINK_STREAM_ID + NPU_MAX_LONG_HWTS_SQ_NUM)) &&
		(model_desc->stream_tasks[idx] > NPU_MAX_LONG_HWTS_SQ_DEPTH ||
		model_desc->stream_tasks[idx] == 0)) {
		npu_drv_err("user sink long stream_tasks[%u]= %u invalid\n",
			idx, model_desc->stream_tasks[idx]);
		return -1;
	}
	if ((model_desc->stream_id[idx] >=
		(NPU_MAX_NON_SINK_STREAM_ID + NPU_MAX_LONG_HWTS_SQ_NUM)) &&
		(model_desc->stream_tasks[idx] > NPU_MAX_HWTS_SQ_DEPTH ||
		model_desc->stream_tasks[idx] == 0)) {
		npu_drv_err("user sink stream_tasks[%u]= %u invalid\n",
			idx, model_desc->stream_tasks[idx]);
		return -1;
	}
	return 0;
}

static int npu_load_hwts_sqe(struct npu_proc_ctx *proc_ctx,
	npu_model_desc_t *model_desc,
	int stream_idx,
	npu_model_desc_info_t *model_desc_info,
	struct npu_entity_info *hwts_sq_sub_info)
{
	void *stream_buf_addr = NULL;
	int ret, sqe_num;
	u32 stream_len;

	cond_return_error(model_desc == NULL, -EINVAL, "model_desc is invalid\n");
	cond_return_error(stream_idx < 0 || stream_idx >= NPU_MODEL_STREAM_NUM,
		-EINVAL, "stream_idx(%d) is invalid\n", stream_idx);
	cond_return_error(model_desc_info == NULL, -EINVAL,
		"model_desc_info is invalid\n");
	cond_return_error(hwts_sq_sub_info == NULL, -EINVAL,
		"hwts_sq_sub_info is invalid\n");
	// copy tasks of one stream to temp buff
	stream_len = model_desc->stream_tasks[stream_idx] * NPU_RT_TASK_SIZE;
	stream_buf_addr = vmalloc(stream_len);
	cond_return_error(stream_buf_addr == NULL, -ENOMEM,
		"vmalloc stream_buf memory size= %u failed\n", stream_len);

	ret = copy_from_user_safe(stream_buf_addr,
		(void __user *)(uintptr_t)model_desc->stream_addr[stream_idx],
		stream_len);
	if (ret != 0) {
		vfree(stream_buf_addr);
		npu_drv_err("fail to copy stream_buf, ret= %d\n", ret);
		return -EINVAL;
	}

	// verify hwts_sqe
	ret = npu_verify_hwts_sqe(proc_ctx, stream_buf_addr, stream_len);
	if (ret != 0) {
		vfree(stream_buf_addr);
		npu_drv_err("npu_verify_hwts_sqe fail, length= %u, ret= %d\n",
			stream_len, ret);
		return -1;
	}

	// save hwts_sqe
	npu_drv_debug("hwts_sq_vir_addr:0x%lx, stream_buf_addr:%pK, stream_id:%u, stream_len:%u\n",
		hwts_sq_sub_info->vir_addr, stream_buf_addr,
		model_desc->stream_id[stream_idx], stream_len);
	sqe_num = npu_format_hwts_sqe((void *)(
		uintptr_t)hwts_sq_sub_info->vir_addr,
		stream_buf_addr, stream_len, model_desc_info);

	vfree(stream_buf_addr);
	return sqe_num;
}

static int npu_load_model(struct npu_proc_ctx *proc_ctx,
	npu_model_desc_t *model_desc)
{
	int ret, sqe_num;
	u16 idx;
	u8 devid = proc_ctx->devid;
	struct npu_stream_info *stream_info = NULL;
	struct npu_hwts_sq_info *hwts_sq_info = NULL;
	npu_model_desc_info_t *model_desc_info = NULL;
	struct npu_entity_info *hwts_sq_sub_info = NULL;

	cond_return_error(devid >= NPU_DEV_NUM, -1, "invalid device");

	model_desc_info = npu_calc_model_desc_info(devid, model_desc->model_id);
	cond_return_error(model_desc_info == NULL, -1,
		"model_desc_info is NULL\n");
	ret = memset_s(model_desc_info, sizeof(npu_model_desc_info_t), 0,
		sizeof(npu_model_desc_info_t));
	cond_return_error(ret != 0, -1,
		"memset_s model_desc_info fail, ret = %d\n", ret);

	model_desc_info->model_id = model_desc->model_id;
	model_desc_info->stream_cnt = model_desc->stream_cnt;
	model_desc_info->compute_type = (uint16_t)(model_desc->compute_type);
	model_desc_info->aicore_stack_base = model_desc->aicore_stack_base;
	model_desc_info->aiv_csw_buff_addr = model_desc->aiv_csw_buff_addr;
	npu_drv_debug("model_id:%d stream_cnt:%d compute_type:%d aiv_csw_buff_addr:0x%pK",
		model_desc_info->model_id, model_desc_info->stream_cnt,
		model_desc_info->compute_type, model_desc_info->aiv_csw_buff_addr);

	for (idx = 0; idx < model_desc->stream_cnt; idx++) {
		stream_info = npu_calc_stream_info(devid, model_desc->stream_id[idx]);
		cond_return_error(stream_info == NULL, -1,
			"stream_id= %d stream_info is null\n", model_desc->stream_id[idx]);

		hwts_sq_info = npu_calc_hwts_sq_info(devid, stream_info->sq_index);
		cond_return_error(hwts_sq_info == NULL, -1,
			"stream= %d, sq_index= %u, hwts_sq_info is null\n",
			stream_info->id, stream_info->sq_index);

		hwts_sq_sub_info =
			(struct npu_entity_info *)(uintptr_t)hwts_sq_info->hwts_sq_sub;
		cond_return_error(hwts_sq_sub_info == NULL, -1,
			"stream_id= %d sq_sub is null\n", stream_info->id);

		sqe_num = npu_load_hwts_sqe(proc_ctx, model_desc,
			idx, model_desc_info, hwts_sq_sub_info);
		if (sqe_num <= 0) {
			npu_drv_err("formate hwts_sq failed, stream:%d\n",
				stream_info->id);
			return -1;
		}

		model_desc_info->stream_id[idx] = model_desc->stream_id[idx];
		hwts_sq_info->head = 0;
		hwts_sq_info->tail = (u32)sqe_num;
	}

	return 0;
}
#else
static int npu_verify_model_task_info(npu_model_desc_t *model_desc, u8 idx)
{
	if (model_desc->stream_tasks[idx] > NPU_MAX_SINK_TASK_ID) {
		npu_drv_err("user sink long stream_tasks[%u]= %u invalid\n",
			idx, model_desc->stream_tasks[idx]);
		return -1;
	}
	return 0;
}

static int npu_load_model(struct npu_proc_ctx *proc_ctx,
	npu_model_desc_t *model_desc)
{
	int ret;
	int stream_idx;
	void *stream_buf_addr = NULL;
	int stream_len;
	u8 devid = proc_ctx->devid;
	u16 first_task_id = MAX_UINT16_NUM;

	npu_drv_debug("stream_cnt %u\n", model_desc->stream_cnt);
	cond_return_error(devid >= NPU_DEV_NUM, -1, "invalid device");

	for (stream_idx = model_desc->stream_cnt - 1; stream_idx >= 0; stream_idx--) {
		stream_len = model_desc->stream_tasks[stream_idx] * NPU_RT_TASK_SIZE;
		stream_buf_addr = vmalloc(stream_len);
		cond_return_error(stream_buf_addr == NULL, -ENOMEM,
			"vmalloc stream_buf memory size= %d failed\n", stream_len);

		ret = copy_from_user_safe(stream_buf_addr,
			(void __user *)(uintptr_t)model_desc->stream_addr[stream_idx],
			stream_len);
		if (ret != 0) {
			vfree(stream_buf_addr);
			npu_drv_err("fail to copy stream_buf, ret= %d\n", ret);
			return -EINVAL;
		}

		// verify tasks
		ret = npu_verify_rt_tasks(proc_ctx, stream_buf_addr, stream_len);
		if (ret != 0) {
			vfree(stream_buf_addr);
			npu_drv_err("npu_verify_hwts_sqe fail, length= %d, ret= %d\n",
				stream_len, ret);
			return -1;
		}

		npu_drv_debug("stream_cnt %u, stream_idx = %d\n", model_desc->stream_cnt, stream_idx);
		ret = npu_format_sink_sqe(model_desc, stream_buf_addr, &first_task_id, devid, stream_idx);
		if (ret != 0) {
			vfree(stream_buf_addr);
			npu_drv_err("npu_verify_hwts_sqe fail, length= %d, ret= %d\n",
				stream_len, ret);
			return -1;
		}

		vfree(stream_buf_addr);
		stream_buf_addr = NULL;
	}

	return 0;
}
#endif

static int npu_verify_model_desc(struct npu_proc_ctx *proc_ctx,
	npu_model_desc_t *model_desc)
{
	u8 idx;
	int ret;
	cond_return_error(model_desc == NULL, -1, "model_desc is invalid\n");

	ret = npu_proc_check_model_id(proc_ctx, model_desc->model_id);
	if (ret != 0) {
		npu_drv_err("user model_id= %u invalid\n", model_desc->model_id);
		return -1;
	}

	if (model_desc->stream_cnt > NPU_MODEL_STREAM_NUM ||
		model_desc->stream_cnt == 0) {
		npu_drv_err("user stream_cnt= %u invalid\n", model_desc->stream_cnt);
		return -1;
	}

	if (model_desc->compute_type >= NPU_COMPUTE_TYPE_MAX) {
		npu_drv_err("user compute_type= %u invalid\n", model_desc->compute_type);
		return -1;
	}

	for (idx = 0; idx < model_desc->stream_cnt; idx++) {
		if (model_desc->stream_id[idx] < NPU_MAX_NON_SINK_STREAM_ID ||
			npu_proc_check_stream_id(proc_ctx, model_desc->stream_id[idx]) != 0) {
			npu_drv_err("user sink stream_id[%u]= %u invalid\n",
				idx, model_desc->stream_id[idx]);
			return -1;
		}

		if (model_desc->stream_addr[idx] == NULL) {
			npu_drv_err("user sink stream_addr[%u] is NULL invalid\n", idx);
			return -1;
		}

		ret = npu_verify_model_task_info(model_desc, idx);
		cond_return_error(ret != 0, -1, "model_desc is invalid\n");
	}
	return 0;
}

int npu_ioctl_load_model(struct npu_proc_ctx *proc_ctx, u64 arg)
{
	int ret;
	npu_model_desc_t model_desc = {0};

	npu_drv_debug("enter\n");
	ret = copy_from_user_safe(&model_desc, (void __user *)(uintptr_t)arg,
		sizeof(npu_model_desc_t));
	cond_return_error(ret != 0, -EINVAL, "fail to copy model_desc, ret= %d\n",
		ret);

	npu_drv_debug("model_id= %u, stream_cnt= %u\n",
		model_desc.model_id, model_desc.stream_cnt);

	ret = npu_verify_model_desc(proc_ctx, &model_desc);
	cond_return_error(ret != 0, -1,
		"npu_verify_model_desc fail, ret= %d\n", ret);

	ret = npu_load_model(proc_ctx, &model_desc);
	cond_return_error(ret != 0, ret, "fail to load model, ret= %d\n",
		ret);

	npu_drv_debug("end\n");
	return 0;
}

int npu_check_ioctl_custom_para(struct npu_proc_ctx *proc_ctx,
	unsigned long arg,
	npu_custom_para_t *custom_para, struct npu_dev_ctx **dev_ctx)
{
	int ret;

	ret = copy_from_user_safe(custom_para, (void __user *)(uintptr_t)arg,
		sizeof(npu_custom_para_t));
	if (ret != 0) {
		npu_drv_err("copy_from_user_safe failed, ret = %d\n", ret);
		return -EINVAL;
	}

	if (custom_para->arg == 0) {
		npu_drv_err("invalid arg\n");
		return -EINVAL;
	}

	*dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	if ((*dev_ctx) == NULL) {
		npu_drv_err("npu proc ioctl custom %d of npu process %d is null\n",
			proc_ctx->devid, proc_ctx->pid);
		return -1;
	}

	return ret;
}

int npu_ioctl_send_request(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	npu_rt_task_t task = {0};
	int ret = 0;

	ret = copy_from_user_safe(&task, (void __user *)(uintptr_t)arg,
		sizeof(npu_rt_task_t));
	cond_return_error(ret != 0, -EINVAL,
		"fail to copy task, ret= %d\n", ret);
#ifndef CONFIG_NPU_SWTS
	return npu_send_request(proc_ctx, &task);
#else
	return task_sched_put_request(proc_ctx,  &task);
#endif
}

int npu_ioctl_receive_response(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	struct npu_receive_response_info report_info = {0};
	int ret = 0;

	npu_drv_debug("npu ioctl receive report enter\n");
	ret = copy_from_user_safe(&report_info, (void __user *)(uintptr_t)arg,
		sizeof(struct npu_receive_response_info));
	if (ret != 0) {
		npu_drv_err("fail to copy comm_report, ret= %d\n", ret);
		return -EINVAL;
	}
#ifndef CONFIG_NPU_SWTS
	ret = npu_receive_response(proc_ctx, &report_info);
#else
	ret = task_sched_get_response(proc_ctx, &report_info);
#endif
	if (ret != 0)
		npu_drv_err("fail to receive_response ret= %d\n", ret);

	if (copy_to_user_safe((void *)(uintptr_t)arg, &report_info,
		sizeof(report_info))) {
		npu_drv_err("ioctl receive response copy_to_user_safe error\n");
		return -EINVAL;
	}

	npu_drv_debug("npu ioctl receive report exit\n");
	return ret;
}

static int npu_proc_ioctl_custom(struct npu_proc_ctx *proc_ctx,
	const npu_custom_para_t *custom_para, struct npu_dev_ctx *dev_ctx)
{
	int ret;

	switch (custom_para->cmd) {
	case NPU_IOC_GET_SVM_SSID:
		ret = npu_ioctl_get_svm_ssid(dev_ctx, custom_para->arg);
		break;
	case NPU_IOC_GET_CHIP_INFO:
		ret = npu_ioctl_get_chip_info(custom_para->arg);
		break;
	case NPU_IOC_LOAD_MODEL_BUFF: // for sink stream(v200)
		ret = npu_ioctl_load_model(proc_ctx, custom_para->arg);
		break;
	default:
		npu_drv_err("invalid custom cmd 0x%x\n", custom_para->cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int npu_ioctl_custom(struct npu_proc_ctx *proc_ctx, unsigned long arg)
{
	int ret;
	npu_custom_para_t custom_para = {0};
	struct npu_dev_ctx *dev_ctx = NULL;

	ret = npu_check_ioctl_custom_para(proc_ctx, arg, &custom_para, &dev_ctx);
	if (ret != 0) {
		npu_drv_err("npu check ioctl custom para failed, ret = %d\n", ret);
		return -EINVAL;
	}

	ret = npu_proc_ioctl_custom(proc_ctx, &custom_para, dev_ctx);
	if (ret != 0) {
		npu_drv_err("npu proc ioctl custom failed, ret = %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

/* new add for TS timeout function */
int npu_ioctl_get_ts_timeout(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	uint64_t exception_code = 0;
	int ret = 0;
	unused(proc_ctx);

	npu_drv_debug("enter\n");
	if (copy_from_user_safe(&exception_code, (void *)(uintptr_t)arg,
		sizeof(uint64_t))) {
		npu_drv_err("copy_from_user_safe error\n");
		return -EFAULT;
	}

	if (exception_code < (uint64_t)MODID_NPU_START || exception_code >
		(uint64_t)MODID_NPU_EXC_END) {
		npu_drv_err("expection code %llu out of npu range\n", exception_code);
		return -1;
	}

	/* receive TS exception */
	npu_rdr_exception_report(exception_code);

	return ret;
}

/* ION memory map */
int npu_ioctl_svm_bind_pid(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	pid_t pid;

	ret = copy_from_user_safe(&pid,
		(void __user *)(uintptr_t)arg, sizeof(pid));
	cond_return_error(ret != 0, -EINVAL, "fail to copy svm params, ret = %d\n",
		ret);

	ret = npu_insert_item_bypid(proc_ctx->devid, current->tgid, pid);
	cond_return_error(ret != 0, -EINVAL,
		"npu insert item bypid fail, ret = %d\n", ret);

	return ret;
}

/* ION memory map */
int npu_ioctl_svm_unbind_pid(struct npu_proc_ctx *proc_ctx,
	unsigned long arg)
{
	int ret;
	pid_t pid;

	ret = copy_from_user_safe(&pid,
		(void __user *)(uintptr_t)arg, sizeof(pid));
	cond_return_error(ret != 0, -EINVAL, "fail to copy svm params, ret = %d\n",
		ret);

	ret = npu_release_item_bypid(proc_ctx->devid, current->tgid, pid);
	cond_return_error(ret != 0, -EINVAL,
		"npu release item bypid fail, ret = %d\n", ret);

	return ret;
}

long npu_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct npu_platform_info *plat_info = npu_plat_get_info();
	struct npu_proc_ctx *proc_ctx = NULL;
	int ret;

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail\n");
		return -EINVAL;
	}

	proc_ctx = (struct npu_proc_ctx *)filep->private_data;
	if (proc_ctx == NULL || arg == 0) {
		npu_drv_err("invalid parameter,arg = 0x%lx,cmd = %d\n",
			arg, cmd);
		return -EINVAL;
	}

	if ((plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1) &&
		proc_ctx->pid != current->tgid) {
		npu_drv_err("can't make ioctl in child process!");
		return -EBUSY;
	}

	ret = npu_proc_npu_ioctl_call(proc_ctx, cmd, arg);
	if (ret != 0) {
		npu_drv_err("process failed,arg = %d\n", cmd);
		return -EINVAL;
	}

	return ret;
}
