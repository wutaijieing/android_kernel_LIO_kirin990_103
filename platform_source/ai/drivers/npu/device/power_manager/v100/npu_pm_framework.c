/*
 * npu_pm_framework.c
 *
 * about npu pm
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
#include "npu_pm_framework.h"

#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/timer.h>

#include "npu_log.h"
#include "npu_platform.h"
#include "npu_shm.h"
#include "npu_dpm.h"
#include "bbox/npu_dfx_black_box.h"
#include "npu_heart_beat.h"
#include "npu_adapter.h"
#include "npu_manager.h"
#include "npu_stream.h"

enum npu_refcnt_updata {
	OPS_ADD = 0,
	OPS_SUB = 1,
	OPS_BUTT
};

static int npu_pm_check_workmode(u32 curr_work_mode, u32 workmode)
{
	if (workmode >= NPU_ISPNN_SEPARATED)
		return -1;
	if (npu_bitmap_get(curr_work_mode, NPU_NONSEC) && (workmode == NPU_SEC))
		return -1;
	if (npu_bitmap_get(curr_work_mode, NPU_SEC) && (workmode == NPU_NONSEC))
		return -1;

	if (npu_bitmap_get(curr_work_mode, workmode) != 0)
		return 1;

	return 0;
}

int npu_pm_enter_workmode(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 workmode)
{
	int ret;
	u32 curr_work_mode;
#ifdef CONFIG_HUAWEI_DSM
	struct dsm_client *tmp_dsm_client = npu_get_dsm_client();
#endif

	mutex_lock(&dev_ctx->npu_power_mutex);
	curr_work_mode = dev_ctx->pm.work_mode;
	npu_drv_info("curr_work_mode_set = 0x%x secure_mode = 0x%x \n",
		curr_work_mode, workmode);

	ret = npu_pm_check_workmode(curr_work_mode, workmode);
	if (ret != 0) {
		mutex_unlock(&dev_ctx->npu_power_mutex);
		cond_return_error(ret < 0, ret, "not support workmode, curr_work_mode_set = 0x%x work_mode = 0x%x\n",
			curr_work_mode, workmode);
		npu_drv_info("workmode has already set, workmode %u\n", workmode);
		return 0;
	}

	dev_ctx->pm.work_mode = (1 << workmode);
	ret = npu_powerup(proc_ctx, dev_ctx, workmode);
	if (ret) {
		dev_ctx->pm.work_mode = curr_work_mode;
		mutex_unlock(&dev_ctx->npu_power_mutex);
		npu_drv_err("npu powerup failed\n");

#ifdef CONFIG_HUAWEI_DSM
		if (tmp_dsm_client != NULL && !dsm_client_ocuppy(tmp_dsm_client)) {
			dsm_client_record(tmp_dsm_client, "npu power up failed\n");
			dsm_client_notify(tmp_dsm_client, DSM_AI_KERN_POWER_UP_ERR_NO);
			npu_drv_err("[I/DSM] %s dmd report\n",
				tmp_dsm_client->client_name);
		}
#endif
		return ret;
	}

	if (workmode == NPU_SEC) {
		mutex_unlock(&dev_ctx->npu_power_mutex);
		npu_drv_warn("secure or ispnn npu power up,no need send mbx to tscpu,return directly\n");
		return 0;
	}
	mutex_unlock(&dev_ctx->npu_power_mutex);

	mutex_lock(&proc_ctx->stream_mutex);
	mutex_lock(&dev_ctx->npu_power_mutex);
	if (dev_ctx->power_stage == NPU_PM_UP) {
		ret = npu_proc_send_alloc_stream_mailbox(proc_ctx);
		if (ret) {
			mutex_unlock(&dev_ctx->npu_power_mutex);
			mutex_unlock(&proc_ctx->stream_mutex);
			npu_drv_err("npu send stream mailbox failed\n");
			return ret;
		}
	} else {
		npu_drv_warn("alloc stream no need to inform ts, power_stage %d, work_mode = %d\n",
			dev_ctx->power_stage, dev_ctx->pm.work_mode);
	}
	mutex_unlock(&dev_ctx->npu_power_mutex);
	mutex_unlock(&proc_ctx->stream_mutex);

	npu_rdr_exception_init();

	npu_drv_warn("npu dev %u powerup successfully!\n", dev_ctx->devid);
	return ret;
}

int npu_pm_exit_workmode(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 workmode)
{
	int ret;
	u32 curr_work_mode;

	mutex_lock(&dev_ctx->npu_power_mutex);
	curr_work_mode = dev_ctx->pm.work_mode;
	npu_drv_warn("cur_secure_mode = 0x%x, workmode = 0x%x\n",
		curr_work_mode, workmode);
	ret = npu_pm_check_workmode(curr_work_mode, workmode);
	if (ret <= 0) {
		mutex_unlock(&dev_ctx->npu_power_mutex);
		npu_drv_err("not support workmode, curr_work_mode_set = 0x%x work_mode = 0x%x\n",
			curr_work_mode, workmode);
		return ret;
	}

	ret = npu_powerdown(proc_ctx, dev_ctx);
	if (ret != 0) {
		mutex_unlock(&dev_ctx->npu_power_mutex);
		npu_drv_err("npu powerdown failed\n");
		return ret;
	}

	mutex_unlock(&dev_ctx->npu_power_mutex);

	return ret;
}

int npu_powerup(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;
	unsigned long flags;
	struct npu_platform_info *plat_info = NULL;

	npu_drv_warn("enter\n");

	plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, -ENODEV, "get plat_ops failed\n");
	cond_return_error(
		atomic_cmpxchg(&dev_ctx->power_access, 1, 0) == 0, 0,
		"npu dev %d has already powerup!\n", dev_ctx->devid);

	ret = plat_info->adapter.pm_ops.npu_power_up(dev_ctx, work_mode);
	if (ret != 0) {
		atomic_inc(&dev_ctx->power_access);
		npu_drv_err("plat_ops npu_power_up failed\n");
		/* bbox : npu power up falied */
		npu_rdr_exception_report(RDR_EXC_TYPE_NPU_POWERUP_FAIL);
		return ret;
	}

	npu_proc_clear_sqcq_info(proc_ctx);

	// update inuse
	dev_ctx->inuse.devid = dev_ctx->devid;
	spin_lock_irqsave(&dev_ctx->ts_spinlock, flags);
	dev_ctx->inuse.ai_core_num = plat_info->spec.aicore_max;
	dev_ctx->inuse.ai_core_error_bitmap = 0;
	spin_unlock_irqrestore(&dev_ctx->ts_spinlock, flags);

	dev_ctx->ts_work_status = (u32)(work_mode != NPU_SEC ? NPU_TS_WORK :
			NPU_TS_SEC_WORK);
	npu_drv_warn("npu dev %u hardware powerup successfully!\n", dev_ctx->devid);

	if (work_mode != NPU_SEC) {
		 /* bbox heart beat init in non_secure mode */
		ret = npu_heart_beat_init(dev_ctx);
		if (ret != 0)
			npu_drv_err("npu_heart_beat_init failed, ret = %d\n", ret);

		ret = npu_sync_ts_time();
		if (ret != 0)
			npu_drv_warn("npu_sync_ts_time fail. ret = %d\n", ret);
#if defined (CONFIG_DPM_HWMON) && defined (CONFIG_NPU_DPM_ENABLED)
		npu_dpm_init();
#endif
	}

	atomic_dec(&dev_ctx->power_success);
	__pm_stay_awake(dev_ctx->wakeup);
	return 0;
}

int npu_powerdown(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx)
{
	int ret;
	struct npu_platform_info *plat_info = NULL;
	u32 work_mode = 0;

	npu_drv_warn("enter\n");
	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get plat_ops failed\n");
		return -1;
	}

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_AUTO_POWER_DOWN] == NPU_FEATURE_OFF) {
		npu_drv_info("npu auto power down switch off\n");
		dev_ctx->pm.work_mode = 0;
		return 0;
	}

	if (atomic_cmpxchg(&dev_ctx->power_success, 0, 1) == 1) {
		npu_drv_warn("npu dev %d has already powerdown!\n", dev_ctx->devid);
		dev_ctx->pm.work_mode = 0;
		return 0;
	}

	if (!npu_bitmap_get(dev_ctx->pm.work_mode, NPU_SEC)) {
		/* bbox heart beat exit */
		npu_heart_beat_exit(dev_ctx);
#if defined (CONFIG_DPM_HWMON) && defined (CONFIG_NPU_DPM_ENABLED)
		npu_dpm_exit();
#endif
	}

	if (npu_bitmap_get(dev_ctx->pm.work_mode, NPU_SEC))
		work_mode = NPU_SEC;

	ret = plat_info->adapter.pm_ops.npu_power_down(dev_ctx->devid,
		work_mode, &dev_ctx->power_stage);
	if (ret != 0) {
		npu_drv_err("plat_ops npu_power_down failed\n");
		/* bbox : npu power down falied */
		npu_rdr_exception_report(RDR_EXC_TYPE_NPU_POWERDOWN_FAIL);
	} else {
		npu_drv_warn("npu dev %d powerdown success!\n", dev_ctx->devid);
	}
	npu_proc_clear_sqcq_info(proc_ctx);
	npu_recycle_rubbish_stream(dev_ctx);

	dev_ctx->ts_work_status = NPU_TS_DOWN;
	dev_ctx->pm.work_mode = 0;
	atomic_inc(&dev_ctx->power_access);
	__pm_relax(dev_ctx->wakeup);

	return ret;
}

void npu_task_cnt_update(struct npu_dev_ctx *dev_ctx, u32 cnt, u32 flag)
{
	int ref_cnt;

	if (flag == OPS_ADD) {
		atomic_add(cnt, &dev_ctx->pm.task_ref_cnt);
		npu_drv_debug("add , task ref cnt = %d, cnt = %d\n",
			atomic_read(&dev_ctx->pm.task_ref_cnt), cnt);
	} else {
		ref_cnt = atomic_sub_return(cnt, &dev_ctx->pm.task_ref_cnt);
		npu_drv_debug("sub , task ref cnt = %d, cnt = %d\n",
			atomic_read(&dev_ctx->pm.task_ref_cnt), cnt);
		if (ref_cnt == 0) {
			npu_pm_add_idle_timer(dev_ctx);
			npu_drv_debug("idle timer add, ref_cnt = %d\n", cnt);
		}
	}
}

int npu_pm_proc_send_task_enter_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	unused(work_mode);
	npu_task_cnt_update(dev_ctx, 1, OPS_ADD);
	return npu_pm_enter_workmode(proc_ctx, dev_ctx, NPU_NONSEC);
}

int npu_pm_proc_release_task_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode, u32 num)
{
	unused(proc_ctx);
	unused(work_mode);
	npu_task_cnt_update(dev_ctx, num, OPS_SUB);
	return 0;
}

int npu_pm_proc_ioctl_enter_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	return npu_pm_enter_workmode(proc_ctx, dev_ctx, work_mode);
}

int npu_pm_proc_ioctl_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	return npu_pm_exit_workmode(proc_ctx, dev_ctx, work_mode);
}

int npu_pm_proc_release_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx)
{
	int ret;

	mutex_lock(&dev_ctx->npu_power_mutex);
	ret = npu_powerdown(proc_ctx, dev_ctx);
	mutex_unlock(&dev_ctx->npu_power_mutex);

	return ret;
}

int npu_ctrl_core(u32 dev_id, u32 core_num)
{
	int ret;
	struct npu_platform_info *plat_info = NULL;
	struct npu_dev_ctx *dev_ctx = NULL;

	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get plat_ops failed\n");
		return -EINVAL;
	}

	if (dev_id > NPU_DEV_NUM) {
		npu_drv_err("invalid id\n");
		return -EINVAL;
	}

	if ((core_num == 0) || (core_num > plat_info->spec.aicore_max)) {
		npu_drv_err("invalid core num %u\n", core_num);
		return -EINVAL;
	}

	if (NULL == plat_info->adapter.res_ops.npu_ctrl_core) {
		npu_drv_err("do not support ctrl core num %u\n", core_num);
		return -EINVAL;
	}

	dev_ctx = get_dev_ctx_by_id(dev_id);
	if (dev_ctx == NULL) {
		npu_drv_err("get device ctx fail\n");
		return -EINVAL;
	}

	mutex_lock(&dev_ctx->npu_power_mutex);
	ret = plat_info->adapter.res_ops.npu_ctrl_core(dev_ctx, core_num);
	mutex_unlock(&dev_ctx->npu_power_mutex);

	if (ret != 0)
		npu_drv_err("ctrl device core num %u fail ret %d\n",
			core_num, ret);
	else
		npu_drv_warn("ctrl device core num %u success\n", core_num);

	return ret;
}

int npu_pm_reboot(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx)
{
	unused(proc_ctx);
	unused(dev_ctx);
	return 0;
}

void npu_pm_resource_init(struct npu_dev_ctx *dev_ctx)
{
	unused(dev_ctx);
}

void npu_pm_delete_idle_timer(struct npu_dev_ctx *dev_ctx)
{
	unused(dev_ctx);
}

void npu_pm_add_idle_timer(struct npu_dev_ctx *dev_ctx)
{
	unused(dev_ctx);
}

int npu_interframe_enable(struct npu_proc_ctx *proc_ctx, uint32_t enable)
{
	unused(proc_ctx);
	unused(enable);
	return 0;
}

void npu_pm_adapt_init(struct npu_dev_ctx *dev_ctx)
{
	unused(dev_ctx);
}

int npu_pm_exception_powerdown(struct npu_dev_ctx *dev_ctx)
{
	unused(dev_ctx);
	return 0;
}
