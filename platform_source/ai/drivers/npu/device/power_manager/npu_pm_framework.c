/*
 * npu_pm_framework.c
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

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "bbox/npu_dfx_black_box.h"
#include "npu_proc_ctx.h"
#include "npu_common.h"
#include "npu_log.h"
#include "npu_platform.h"
#include "npu_shm.h"
#include "npu_adapter.h"
#include "npu_autofs.h"
#include "npu_powercapping.h"
#include "npu_dpm.h"
#include "npu_message.h"
#include "npu_heart_beat.h"
#include "npu_io.h"
#include "npu_hwts_plat.h"
#include "npu_platform_exception.h"
#include "dts/npu_reg.h"
#include "npu_task_message.h"
#include "npu_manager.h"

#define npu_plat_power_status_vote(pm_flag) ((pm_flag) | (u32)npu_plat_get_npu_power_status())
#define npu_plat_power_status_unvote(pm_flag) ((~(pm_flag)) & (u32)npu_plat_get_npu_power_status())

static void npu_pm_idle_work(struct work_struct *npu_idle_work);
static void npu_pm_interframe_idle_work(
	struct work_struct *interframe_idle_work);
struct npu_pm_work_strategy *npu_pm_get_work_strategy(
	struct npu_power_manage *handle, u32 work_mode)
{
	struct npu_pm_work_strategy *strategy = NULL;

	strategy = (struct npu_pm_work_strategy *)(uintptr_t)(
		handle->strategy_table_addr +
		sizeof(struct npu_pm_work_strategy) * (work_mode));

	return strategy;
}

struct npu_pm_subip_action *npu_pm_get_subip_action(
	struct npu_power_manage *handle, u32 subip)
{
	struct npu_pm_subip_action *action = NULL;

	action = (struct npu_pm_subip_action *)(uintptr_t)(
		handle->action_table_addr +
		sizeof(struct npu_pm_subip_action) * (subip));

	return action;
}

static struct npu_pm_work_strategy *npu_pm_interframe_idle_get_work_strategy(
	struct npu_power_manage *handle, u32 work_mode)
{
	struct npu_pm_work_strategy *strategy = NULL;
	struct npu_interframe_idle_manager *interframe_idle_manager =
		&handle->interframe_idle_manager;

	strategy = (struct npu_pm_work_strategy *)(uintptr_t)(
		interframe_idle_manager->strategy_table_addr +
		sizeof(struct npu_pm_work_strategy) * (work_mode));

	return strategy;
}

static u32 npu_pm_interframe_idle_get_powerup_delta_subip(
	struct npu_power_manage *handle, u32 work_mode)
{
	struct npu_pm_work_strategy *strategy = NULL;
	u32 delta_subip;
	u32 cur_subip;

	strategy = npu_pm_interframe_idle_get_work_strategy(handle, work_mode);
	delta_subip = strategy->subip_set.data;
	cur_subip = handle->cur_subip_set;
	delta_subip &= (~cur_subip);

	return delta_subip;
}

static u32 npu_pm_interframe_idle_get_powerdown_delta_subip(
	struct npu_power_manage *handle, u32 work_mode)
{
	struct npu_pm_work_strategy *strategy = NULL;
	u32 mode_idx;
	u32 idle_subip;
	u32 delta_subip;
	u32 used_subip = 0;

	for (mode_idx = 0; mode_idx < NPU_WORKMODE_MAX; mode_idx++) {
		if (mode_idx == work_mode)
			continue;
		if (npu_bitmap_get(handle->work_mode, mode_idx) == 0x1) {
			strategy = npu_pm_interframe_idle_get_work_strategy(
				handle, mode_idx);
			used_subip |= strategy->subip_set.data;
		}
	}
	idle_subip = handle->interframe_idle_manager.idle_subip;
	delta_subip = idle_subip & (~used_subip);

	return delta_subip;
}

static u32 npu_pm_interframe_idle_get_delta_subip(
	struct npu_power_manage *handle, u32 work_mode, int pm_ops)
{
	u32 delta_subip;

	if (pm_ops == POWER_UP)
		delta_subip = npu_pm_interframe_idle_get_powerup_delta_subip(
			handle, work_mode);
	else
		delta_subip = npu_pm_interframe_idle_get_powerdown_delta_subip(
			handle, work_mode);

	return delta_subip;
}

void npu_close_idle_power_down(struct npu_dev_ctx *dev_ctx)
{
	int work_state;

	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");

	mutex_lock(&dev_ctx->pm.idle_mutex);
	work_state = atomic_cmpxchg(&dev_ctx->pm.idle_wq_processing, WORK_ADDED,
		WORK_CANCELING);
	if (work_state == WORK_ADDED) {
		cancel_delayed_work_sync(&dev_ctx->pm.idle_work);
		atomic_set(&dev_ctx->pm.idle_wq_processing, WORK_IDLE);
	} else if (work_state != WORK_IDLE) {
		npu_drv_warn("work state = %d\n", work_state);
	}
	mutex_unlock(&dev_ctx->pm.idle_mutex);
}

void npu_close_interframe_power_down(struct npu_dev_ctx *dev_ctx)
{
	int work_state;
	struct npu_interframe_idle_manager *idle_manager = NULL;

	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");

	idle_manager = &dev_ctx->pm.interframe_idle_manager;
	mutex_lock(&idle_manager->idle_mutex);
	work_state = atomic_cmpxchg(&idle_manager->wq_processing, WORK_ADDED, WORK_CANCELING);
	if (work_state == WORK_ADDED) {
		cancel_delayed_work_sync(&dev_ctx->pm.interframe_idle_manager.work);
		atomic_set(&idle_manager->wq_processing, WORK_IDLE);
	} else if (work_state != WORK_IDLE) {
		npu_drv_warn("work state = %d\n", work_state);
	}
	mutex_unlock(&idle_manager->idle_mutex);
}

void npu_pm_delete_idle_timer(struct npu_dev_ctx *dev_ctx)
{
	struct npu_platform_info *plat_info = dev_ctx->plat_info;

	cond_return_void(plat_info == NULL, "plat info is null\n");

	npu_close_idle_power_down(dev_ctx);

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN])
		npu_close_interframe_power_down(dev_ctx);
}

void npu_open_idle_power_down(struct npu_dev_ctx *dev_ctx)
{
	int work_state;

	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");

	mutex_lock(&dev_ctx->pm.idle_mutex);
	work_state = atomic_cmpxchg(&dev_ctx->pm.idle_wq_processing, WORK_IDLE,
		WORK_ADDING);
	if (work_state == WORK_IDLE) {
		schedule_delayed_work(&dev_ctx->pm.idle_work,
			msecs_to_jiffies(dev_ctx->pm.npu_idle_time_out));
		atomic_set(&dev_ctx->pm.idle_wq_processing, WORK_ADDED);
	} else if (work_state != WORK_ADDED) {
		npu_drv_warn("work state = %d\n", work_state);
	}
	mutex_unlock(&dev_ctx->pm.idle_mutex);
}

void npu_open_interframe_power_down(struct npu_dev_ctx *dev_ctx)
{
	int work_state;
	struct npu_interframe_idle_manager *idle_manager = NULL;

	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");

	idle_manager = &dev_ctx->pm.interframe_idle_manager;
	mutex_lock(&idle_manager->idle_mutex);
	work_state = atomic_cmpxchg(&idle_manager->wq_processing, WORK_IDLE, WORK_ADDING);
	if (work_state == WORK_IDLE) {
		INIT_DELAYED_WORK(&idle_manager->work, npu_pm_interframe_idle_work);
		schedule_delayed_work(&idle_manager->work,
			msecs_to_jiffies(
				dev_ctx->pm.interframe_idle_manager.idle_time_out));
		atomic_set(&idle_manager->wq_processing, WORK_ADDED);
	} else if (work_state != WORK_ADDED) {
		npu_drv_warn("work state = %d\n", work_state);
	}
	mutex_unlock(&idle_manager->idle_mutex);
}

void npu_pm_add_idle_timer(struct npu_dev_ctx *dev_ctx)
{
	struct npu_platform_info *plat_info = dev_ctx->plat_info;

	cond_return_void(plat_info == NULL, "plat info is null\n");

	if ((npu_bitmap_get(dev_ctx->pm.work_mode, NPU_NONSEC) == 0) ||
		(dev_ctx->power_stage != NPU_PM_UP)) {
		npu_drv_warn("Can not add idle timer, for work mode: %d, power stage: %d!\n",
			dev_ctx->pm.work_mode, dev_ctx->power_stage);
		return;
	}

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_AUTO_POWER_DOWN] == NPU_FEATURE_OFF) {
		npu_drv_warn("npu auto power down switch off\n");
		return;
	}

	npu_open_idle_power_down(dev_ctx);

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN] &&
		(atomic_read(&dev_ctx->pm.interframe_idle_manager.enable) == 1))
		npu_open_interframe_power_down(dev_ctx);
}

int npu_pm_powerdown_proc(struct npu_dev_ctx *dev_ctx,
	u32 work_mode, u32 delta_subip)
{
	int ret = 0;
	int fail_flag = false;
	u32 subip, subip_idx, curr_subip_state;
	struct npu_pm_subip_action *subip_action = NULL;
	enum npu_subip *powerdown_order = get_powerdown_order();

	curr_subip_state = dev_ctx->pm.cur_subip_set;
	npu_drv_info("delta_subip : 0x%x, curr_subip_state: 0x%x\n",
		delta_subip, curr_subip_state);

	for (subip_idx = 0; subip_idx < NPU_SUBIP_MAX; subip_idx++) {
		subip = powerdown_order[subip_idx];
		if (npu_bitmap_get(delta_subip, subip) &&
			(npu_bitmap_get(curr_subip_state, subip) == 1)) {
			subip_action = npu_pm_get_subip_action(&dev_ctx->pm, subip);
			ret = subip_action->power_down(work_mode, subip,
				(void *)dev_ctx);
			if (ret != 0) {
				fail_flag = true;
				npu_drv_err("subip power down fail : subip %u ret = %d\n",
					subip, ret);
			}
			curr_subip_state = npu_bitmap_clear(curr_subip_state, subip);
			dev_ctx->pm.cur_subip_set = curr_subip_state;
			npu_drv_info("subip_%u power down succ, state : 0x%x\n",
				subip, curr_subip_state);
		}
	}

	if (fail_flag == true)
		npu_rdr_exception_report(RDR_EXC_TYPE_NPU_POWERDOWN_FAIL);

	return ret;
}

int npu_pm_interframe_idle_powerdown(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;
	u32 delta_subip;

	delta_subip = npu_pm_interframe_idle_get_delta_subip(&dev_ctx->pm,
		work_mode, POWER_DOWN);
	npu_drv_warn("interframe idle power down enter, delta_subip= 0x%x\n", delta_subip);
	ret = npu_pm_powerdown_proc(dev_ctx, work_mode, delta_subip);

	return ret;
}

static void npu_pm_interframe_idle_work(
	struct work_struct *interframe_idle_work)
{
	struct npu_interframe_idle_manager *idle_manager = NULL;
	struct npu_dev_ctx *dev_ctx = NULL;
	int work_state;

	npu_drv_info("interframe idle timer work enter\n");
	cond_return_void(interframe_idle_work == NULL,
		"interframe_idle_work is null\n");
	idle_manager = container_of(interframe_idle_work,
		struct npu_interframe_idle_manager, work.work);
	dev_ctx = idle_manager->private_data;
	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");

	mutex_lock(&dev_ctx->npu_power_mutex);
	if (dev_ctx->pm.wm_cnt[NPU_NONSEC] != 0) {
		atomic_cmpxchg(&idle_manager->wq_processing, WORK_ADDED, WORK_IDLE);
		mutex_unlock(&dev_ctx->npu_power_mutex);
		return;
	}
	work_state = atomic_read(&idle_manager->wq_processing);
	if (work_state == WORK_ADDED) {
		(void)npu_pm_interframe_idle_powerdown(dev_ctx, NPU_NONSEC);
#ifdef CONFIG_NPU_AUTOFS
		if ((dev_ctx->plat_info->dts_info.feature_switch[NPU_FEATURE_NPU_AUTOFS] == 1) &&
			(npu_bitmap_get(dev_ctx->pm.work_mode, NPU_ISPNN_SEPARATED) == 0) &&
			(npu_bitmap_get(dev_ctx->pm.work_mode, NPU_ISPNN_SHARED) == 0) &&
			(dev_ctx->power_stage == NPU_PM_UP))
			npu_autofs_enable();
#endif
		atomic_set(&idle_manager->wq_processing, WORK_IDLE);
	} else {
		npu_drv_warn("work state = %d\n", work_state);
	}
	mutex_unlock(&dev_ctx->npu_power_mutex);
}

static int npu_pm_interframe_check_need_powerup(struct npu_dev_ctx *dev_ctx,
	u32 work_mode)
{
	u32 cur_subip;
	u32 workmode_subip;
	struct npu_power_manage *power_manage = &dev_ctx->pm;
	struct npu_pm_work_strategy *work_strategy = NULL;

	work_strategy = npu_pm_interframe_idle_get_work_strategy(power_manage,
		work_mode);
	workmode_subip = work_strategy->subip_set.data;
	cur_subip = power_manage->cur_subip_set;
	npu_drv_debug("cur_subip_set = 0x%x, subip_set = 0x%x\n", cur_subip,
		workmode_subip);
	if ((workmode_subip & cur_subip) != workmode_subip)
		return 1;
	else
		return 0;
}

int npu_pm_powerup_proc(struct npu_dev_ctx *dev_ctx,
	u32 work_mode, u32 delta_subip)
{
	int ret = 0;
	u32 subip, subip_idx, curr_subip_set;
	struct npu_pm_subip_action *subip_action = NULL;
	enum npu_subip *powerup_order = get_powerup_order();
#ifdef CONFIG_HUAWEI_DSM
	struct dsm_client *tmp_dsm_client = npu_get_dsm_client();
#endif

	curr_subip_set = dev_ctx->pm.cur_subip_set;
	npu_drv_info("delta_subip : 0x%x, curr_subip_state: 0x%x\n",
		delta_subip, curr_subip_set);
	for (subip_idx = 0; subip_idx < NPU_SUBIP_MAX; subip_idx++) {
		subip = powerup_order[subip_idx];
		if (npu_bitmap_get(delta_subip, subip) &&
			(npu_bitmap_get(curr_subip_set, subip) == 0)) {
			subip_action = npu_pm_get_subip_action(&dev_ctx->pm, subip);
			ret = subip_action->power_up(work_mode, subip,
				(void **)dev_ctx);
			cond_goto_error(ret != 0, POWER_UP_FAIL, ret, ret,
				"subip power up fail : subip %u ret = %d\n", subip, ret);
			curr_subip_set = npu_bitmap_set(curr_subip_set, subip);
			dev_ctx->pm.cur_subip_set = curr_subip_set;
			npu_drv_info("subip_%d power up succ, state : 0x%x\n",
				subip, dev_ctx->pm.cur_subip_set);
		}
	}

	return ret;
POWER_UP_FAIL:
	(void)npu_pm_powerdown_proc(dev_ctx, work_mode, delta_subip);
	if (ret != -NOSUPPORT)
		npu_rdr_exception_report(RDR_EXC_TYPE_NPU_POWERUP_FAIL);
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

static int npu_pm_interframe_idle_powerup(struct npu_dev_ctx *dev_ctx, u32 workmode)
{
	int ret;
	u32 delta_subip;

	delta_subip = npu_pm_interframe_idle_get_delta_subip(&dev_ctx->pm,
		workmode, POWER_UP);
	npu_drv_warn("interframe idle power up enter, delta_subip= 0x%x\n", delta_subip);
	ret = npu_pm_powerup_proc(dev_ctx, workmode, delta_subip);
	cond_return_error(ret != 0, ret, "power down fail: ret = %d\n", ret);

	npu_drv_info("interframe power up\n");

	return ret;
}

int npu_pm_check_and_interframe_powerup(struct npu_dev_ctx *dev_ctx, u32 workmode)
{
	int ret = 0;

	if (dev_ctx->plat_info == NULL ||
		dev_ctx->plat_info->dts_info.feature_switch[NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN] == 0)
		return ret;

	if (workmode == NPU_NONSEC) {
		if (npu_pm_interframe_check_need_powerup(dev_ctx, workmode)) {
#ifdef CONFIG_NPU_AUTOFS
			if ((dev_ctx->plat_info->dts_info.feature_switch[NPU_FEATURE_NPU_AUTOFS] == 1) &&
				(dev_ctx->power_stage == NPU_PM_UP))
				npu_autofs_disable();
#endif
			ret = npu_pm_interframe_idle_powerup(dev_ctx, workmode);
		}
	}

	return ret;
}

int npu_pm_dev_enter_wm(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;
	struct npu_pm_work_strategy *work_strategy = NULL;

	if (dev_ctx->pm.work_mode == 0)
		npu_rdr_exception_init();

	npu_drv_debug("dev work_mode_set = 0x%x, work_mode = %d\n",
		dev_ctx->pm.work_mode, work_mode);
	if (npu_bitmap_get(dev_ctx->pm.work_mode, work_mode) != 0) {
		dev_ctx->pm.wm_cnt[work_mode]++;
		return 0;
	}

	work_strategy = npu_pm_get_work_strategy(&dev_ctx->pm, work_mode);
	if (work_strategy->work_mode_set.data == UINT32_MAX ||
		(dev_ctx->pm.work_mode & work_strategy->work_mode_set.data) != 0) {
		npu_drv_err("work mode conflict, work_mode_set = 0x%x, work_mode = %d\n",
			dev_ctx->pm.work_mode, work_mode);
		return -1;
	}

	ret = npu_pm_vote(dev_ctx, work_mode);
	if (ret != 0)
		return ret;
	dev_ctx->pm.wm_cnt[work_mode]++;

	return ret;
}

int npu_pm_dev_exit_wm(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;

	if (dev_ctx->pm.wm_cnt[work_mode] == 0) {
		npu_drv_err("dev work_mode_cnt = %d\n", dev_ctx->pm.wm_cnt[work_mode]);
		return 0;
	}

	dev_ctx->pm.wm_cnt[work_mode]--;
	if (dev_ctx->pm.wm_cnt[work_mode] != 0) {
		npu_drv_debug("dev work_mode_cnt = %d work_mode = %d\n",
			dev_ctx->pm.wm_cnt[work_mode], work_mode);
		return 0;
	}

	if (npu_bitmap_get(dev_ctx->pm.work_mode, work_mode) == 0) {
		npu_drv_err("dev work_mode_set = 0x%x, work_mode = %d\n",
			dev_ctx->pm.work_mode, work_mode);
		return 0;
	}

	ret = npu_pm_unvote(dev_ctx, work_mode);

	return ret;
}

int npu_pm_dev_send_task_enter_wm(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;

	npu_pm_delete_idle_timer(dev_ctx);
	mutex_lock(&dev_ctx->npu_power_mutex);
	ret = npu_pm_dev_enter_wm(dev_ctx, work_mode);
	if (ret != 0) {
		mutex_unlock(&dev_ctx->npu_power_mutex);
		return ret;
	}
	ret = npu_pm_check_and_interframe_powerup(dev_ctx, work_mode);
	mutex_unlock(&dev_ctx->npu_power_mutex);
	return ret;
}

int npu_pm_dev_release_report_exit_wm(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	mutex_lock(&dev_ctx->npu_power_mutex);
	if (dev_ctx->pm.wm_cnt[NPU_NONSEC] == 0) {
		npu_drv_err("dev work_mode_cnt = %d work_mode = %d\n",
			dev_ctx->pm.wm_cnt[work_mode], work_mode);
		mutex_unlock(&dev_ctx->npu_power_mutex);
		return 0;
	}

	dev_ctx->pm.wm_cnt[work_mode]--;
	if (dev_ctx->pm.wm_cnt[NPU_NONSEC] == 0) {
		npu_pm_add_idle_timer(dev_ctx);
		npu_drv_debug("dev idle, work_mode_cnt = %d work_mode = %d\n",
			dev_ctx->pm.wm_cnt[work_mode], work_mode);
	}
	mutex_unlock(&dev_ctx->npu_power_mutex);

	return 0;
}

int npu_pm_proc_ioctl_enter_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;
	struct npu_platform_info *plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, -1, "get plat info fail\n");

	npu_drv_debug("proc work_mode_set = 0x%x, work_mode = %d\n",
		proc_ctx->wm_set, work_mode);
	if (work_mode == NPU_NONSEC) {
		npu_drv_err("proc work_mode_set = 0x%x, work_mode = %d\n",
		proc_ctx->wm_set, work_mode);
		return -1;
	}
	mutex_lock(&proc_ctx->wm_lock);
	if (npu_bitmap_get(proc_ctx->wm_set, work_mode) != 0) {
		npu_drv_warn("proc work_mode_set = 0x%x, work_mode = %d\n",
			proc_ctx->wm_set, work_mode);
		mutex_unlock(&proc_ctx->wm_lock);
		return 0;
	}

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_SEC_NONSEC_CONCURRENCY] == 0) {
		if ((work_mode == NPU_SEC) &&
			(dev_ctx->pm.wm_cnt[NPU_NONSEC] == 0) &&
			(npu_bitmap_get(dev_ctx->pm.work_mode, NPU_NONSEC) != 0)) {
			npu_pm_delete_idle_timer(dev_ctx);
			mutex_lock(&dev_ctx->npu_power_mutex);
			ret = npu_pm_unvote(dev_ctx, NPU_NONSEC);
			mutex_unlock(&dev_ctx->npu_power_mutex);
			if (ret != 0) {
				mutex_unlock(&proc_ctx->wm_lock);
				return ret;
			}
		}
	}

	mutex_lock(&dev_ctx->npu_power_mutex);
	ret = npu_pm_dev_enter_wm(dev_ctx, work_mode);
	mutex_unlock(&dev_ctx->npu_power_mutex);
	if (ret != 0) {
		mutex_unlock(&proc_ctx->wm_lock);
		return ret;
	}
	proc_ctx->wm_set = npu_bitmap_set(proc_ctx->wm_set, work_mode);
	mutex_unlock(&proc_ctx->wm_lock);

	return ret;
}

int npu_pm_proc_ioctl_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;

	npu_drv_debug("proc work_mode_set = 0x%x, work_mode = %d\n",
		proc_ctx->wm_set, work_mode);

	if (work_mode == NPU_NONSEC) {
		npu_drv_err("proc work_mode_set = 0x%x, work_mode = %d\n",
		proc_ctx->wm_set, work_mode);
		return -1;
	}

	mutex_lock(&proc_ctx->wm_lock);
	if (npu_bitmap_get(proc_ctx->wm_set, work_mode) == 0) {
		npu_drv_err("proc work_mode_set = 0x%x, work_mode = %d\n",
			proc_ctx->wm_set, work_mode);
		mutex_unlock(&proc_ctx->wm_lock);
		return 0;
	}

	mutex_lock(&dev_ctx->npu_power_mutex);
	ret = npu_pm_dev_exit_wm(dev_ctx, work_mode);
	mutex_unlock(&dev_ctx->npu_power_mutex);
	if (ret != 0) {
		mutex_unlock(&proc_ctx->wm_lock);
		return ret;
	}

	proc_ctx->wm_set = npu_bitmap_clear(proc_ctx->wm_set, work_mode);
	mutex_unlock(&proc_ctx->wm_lock);

	return ret;
}

int npu_pm_proc_send_task_enter_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;

	npu_drv_debug("proc work_mode_set = 0x%x, work_mode = %d\n",
		proc_ctx->wm_set, work_mode);
	mutex_lock(&proc_ctx->wm_lock);
	if (npu_bitmap_get(proc_ctx->wm_set, work_mode) == 0) {
		ret = npu_pm_dev_send_task_enter_wm(dev_ctx, work_mode);
		if (ret != 0) {
			mutex_unlock(&proc_ctx->wm_lock);
			return ret;
		}
		proc_ctx->wm_set = npu_bitmap_set(proc_ctx->wm_set, work_mode);
	}
	proc_ctx->wm_cnt++;
	mutex_unlock(&proc_ctx->wm_lock);
	return 0;
}

int npu_pm_proc_release_task_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode, u32 num)
{
	npu_drv_debug("proc work_mode_set = 0x%x, work_mode = %d\n",
		proc_ctx->wm_set, work_mode);

	mutex_lock(&proc_ctx->wm_lock);
	if (proc_ctx->wm_cnt < num) {
		npu_drv_err("proc work_mode_set = 0x%x, work_mode = %d, num = %d\n",
			proc_ctx->wm_set, work_mode, num);
		mutex_unlock(&proc_ctx->wm_lock);
		return -1;
	}

	proc_ctx->wm_cnt -= num;
	if ((proc_ctx->wm_cnt == 0)
		&& (npu_bitmap_get(proc_ctx->wm_set, work_mode) != 0)) {
		(void)npu_pm_dev_release_report_exit_wm(dev_ctx, work_mode);
		proc_ctx->wm_set = npu_bitmap_clear(proc_ctx->wm_set, work_mode);
	}
	mutex_unlock(&proc_ctx->wm_lock);

	return 0;
}

int npu_pm_proc_release_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx)
{
	u32 work_mode;
	npu_drv_info("proc work_mode_set = 0x%x\n", proc_ctx->wm_set);

	mutex_lock(&proc_ctx->wm_lock);
	proc_ctx->wm_cnt = 0;

	/* non secure normal exit */
	if ((npu_bitmap_get(dev_ctx->pm.work_mode, NPU_NONSEC) != 0) &&
		(dev_ctx->pm.wm_cnt[NPU_NONSEC] == 0) &&
		list_empty_careful(&dev_ctx->proc_ctx_list)) {
		npu_pm_delete_idle_timer(dev_ctx);
		mutex_lock(&dev_ctx->npu_power_mutex);
		(void)npu_pm_unvote(dev_ctx, NPU_NONSEC);
		mutex_unlock(&dev_ctx->npu_power_mutex);
	}

	down_write(&dev_ctx->pm.exception_lock);
	mutex_lock(&dev_ctx->npu_power_mutex);
	for (work_mode = 0; work_mode < NPU_WORKMODE_MAX; work_mode++) {
		if (npu_bitmap_get(proc_ctx->wm_set, work_mode) != 0)
			(void)npu_pm_dev_exit_wm(dev_ctx, work_mode);
	}
	if (dev_ctx->pm.wm_cnt[NPU_NONSEC] == 0 && dev_ctx->pm.npu_pm_vote == NPU_PM_VOTE_STATUS_PD)
		dev_ctx->pm.npu_exception_status = NPU_STATUS_NORMAL;
	mutex_unlock(&dev_ctx->npu_power_mutex);
	up_write(&dev_ctx->pm.exception_lock);

	proc_ctx->wm_set = 0;
	mutex_unlock(&proc_ctx->wm_lock);

	return 0;
}

int npu_pm_l2_swap_in(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret = 0;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct npu_vma_mmapping *npu_vma_map = NULL;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -1, "get plat info fail\n");
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_L2_BUFF] == 0)
		return 0;

	if ((!list_empty_careful(&proc_ctx->l2_vma_list)) &&
		(!npu_bitmap_get(dev_ctx->pm.work_mode, NPU_SEC)) &&
		(work_mode == NPU_INIT)) {
		list_for_each_safe(pos, n, &proc_ctx->l2_vma_list) {
			npu_vma_map = list_entry(pos, struct npu_vma_mmapping, list);
			ret = l2_mem_swapin(npu_vma_map->vma);
		}
	} else {
		npu_drv_warn("l2_vma_list is empty or work_mode:0x%x is sec,"
			" l2_mem_swapin is not necessary",
			dev_ctx->pm.work_mode);
	}

	return ret;
}

int npu_pm_l2_swap_out(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret = 0;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct npu_vma_mmapping *npu_vma_map = NULL;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -1, "get plat info fail\n");
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_L2_BUFF] == 0)
		return 0;

	if ((!list_empty_careful(&proc_ctx->l2_vma_list)) &&
		(!npu_bitmap_get(dev_ctx->pm.work_mode, NPU_SEC)) &&
		(work_mode == NPU_INIT)) {
		list_for_each_safe(pos, n, &proc_ctx->l2_vma_list) {
			npu_vma_map = list_entry(pos, struct npu_vma_mmapping, list);
			ret = l2_mem_swapout(npu_vma_map->vma, dev_ctx->devid);
		}
	} else {
		npu_drv_warn("l2_vma_list is empty or work_mode:0x%x is sec,"
			" l2_mem_swapout is not necessary",
			dev_ctx->pm.work_mode);
	}

	return ret;
}

int npu_pm_powerdown_pre_proc(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;
	struct npu_proc_ctx *proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;
	npu_work_mode_info_t work_mode_info = {0};

	list_for_each_entry_safe(proc_ctx, next_proc_ctx, &dev_ctx->proc_ctx_list,
		dev_ctx_list) {
		ret = npu_pm_l2_swap_out(proc_ctx, dev_ctx, work_mode);
		if (ret != 0)
			npu_drv_err("l2 swap out fail, ret = %d\n", ret);
	}

	/* hwts aicore pool switch back */
	work_mode_info.work_mode = work_mode;
	work_mode_info.flags = dev_ctx->pm.work_mode_flags;
	ret = npu_plat_switch_hwts_aicore_pool(dev_ctx, &work_mode_info,
		POWER_OFF);
	if (ret != 0)
		npu_drv_err("hwts return aicore to pool fail, ret = %d\n", ret);

	/* True pm dfx disable when total workmode is 0 except for sec bit. */
	if (npu_bitmap_clear(npu_bitmap_clear(dev_ctx->pm.work_mode, work_mode), NPU_SEC) == 0) {
#ifndef CONFIG_NPU_SWTS
		/* bbox heart beat exit */
		npu_heart_beat_exit(dev_ctx);
#endif

#if defined CONFIG_DPM_HWMON && defined CONFIG_NPU_DPM_ENABLED
		npu_dpm_exit();
#endif

#ifdef CONFIG_NPU_PCR_ENABLED
		npu_powercapping_disable();
#endif
	}

	return 0;
}

int npu_pm_powerdown_post_proc(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	u32 goal_work_mode_set =
		npu_bitmap_clear(dev_ctx->pm.work_mode, work_mode);
	if (goal_work_mode_set == 0) {
		__pm_relax(dev_ctx->wakeup);
		dev_ctx->power_stage = NPU_PM_DOWN;
		atomic_set(&dev_ctx->power_access, 1);
		npu_recycle_rubbish_proc(dev_ctx);
	}

	return 0;
}

int npu_pm_powerup_pre_proc(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	unused(work_mode);
	npu_drv_warn("no need to load firmware, for npu firmware is loaded in fastboot");

	atomic_set(&dev_ctx->power_access, 0);

	return 0;
}

int npu_pm_powerup_post_proc(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret;
	unsigned long flags;
	struct npu_platform_info *plat_info = NULL;
	struct npu_proc_ctx *proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;
	npu_work_mode_info_t work_mode_info = {0};

	plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, -1, "get plat infofailed\n");
	dev_ctx->inuse.devid = dev_ctx->devid;
	spin_lock_irqsave(&dev_ctx->ts_spinlock, flags);
	dev_ctx->inuse.ai_core_num = plat_info->spec.aicore_max;
	dev_ctx->inuse.ai_core_error_bitmap = 0;
	spin_unlock_irqrestore(&dev_ctx->ts_spinlock, flags);
	dev_ctx->power_stage = NPU_PM_UP;

	npu_drv_warn("npu dev %u hardware powerup successfully!\n",
		dev_ctx->devid);

	if ((work_mode != NPU_SEC) && (npu_bitmap_clear(dev_ctx->pm.work_mode, NPU_SEC) == 0)) {
#ifndef CONFIG_NPU_SWTS
		dev_ctx->heart_beat.hwts_exception_callback = npu_exception_report_proc;
		/* bbox heart beat init in non_secure mode */
		ret = npu_heart_beat_init(dev_ctx);
		if (ret != 0)
			npu_drv_err("npu heart beat init failed, ret = %d\n", ret);

		ret = npu_sync_ts_time();
		if (ret != 0)
			npu_drv_warn("npu sync ts time fail. ret = %d\n", ret);
#endif

#ifdef CONFIG_NPU_PCR_ENABLED
		npu_powercapping_enable();
#endif

#if defined CONFIG_DPM_HWMON && defined CONFIG_NPU_DPM_ENABLED
		npu_dpm_init();
#endif
		/* set hwts log&profiling gobal config when hwts init ok */
		hwts_profiling_init(dev_ctx->devid);
	}

	if (dev_ctx->pm.work_mode == 0)
		__pm_stay_awake(dev_ctx->wakeup);

	/* hwts aicore pool switch */
	work_mode_info.work_mode = work_mode;
	work_mode_info.flags = dev_ctx->pm.work_mode_flags;
	ret = npu_plat_switch_hwts_aicore_pool(dev_ctx, &work_mode_info, POWER_ON);
	cond_return_error(ret != 0, ret,
		"hwts pull aicore from pool fail, ret = %d\n", ret);

	if (work_mode == NPU_NONSEC) {
		// lock power mutex, no need dev_ctx->proc_ctx_lock
#ifndef CONFIG_NPU_SWTS
		list_for_each_entry_safe(proc_ctx, next_proc_ctx, &dev_ctx->proc_ctx_list,
			dev_ctx_list) {
			ret = npu_proc_send_alloc_stream_mailbox(proc_ctx);
			cond_return_error(ret != 0, ret,
					"npu send stream mailbox failed\n");
		}
#endif
	} else {
		npu_drv_warn("secure or ispnn npu power up,no need send mbx to tscpu,return directly\n");
	}

	list_for_each_entry_safe(proc_ctx, next_proc_ctx, &dev_ctx->proc_ctx_list,
		dev_ctx_list) {
		ret = npu_pm_l2_swap_in(proc_ctx, dev_ctx, work_mode);
		if (ret != 0)
			npu_drv_err("l2 swap in fail, ret = %d\n", ret);
	}

	return 0;
}

static inline void npu_pm_set_power_on(u32 work_mode)
{
	uint32 pm_flag;

	pm_flag = (work_mode == NPU_SEC) ? DRV_NPU_POWER_ON_SEC_FLAG : DRV_NPU_POWER_ON_FLAG;

	npu_plat_set_npu_power_status(npu_plat_power_status_vote(pm_flag));
}

static inline void npu_pm_set_power_off(u32 work_mode)
{
	uint32 pm_flag;

	pm_flag = (work_mode == NPU_SEC) ? DRV_NPU_POWER_ON_SEC_FLAG : DRV_NPU_POWER_ON_FLAG;

	npu_plat_set_npu_power_status(npu_plat_power_status_unvote(pm_flag));
}

int npu_pm_vote(struct npu_dev_ctx *dev_ctx, u32 workmode)
{
	int ret;
	u32 goal_work_mode_set;
	u32 delta_subip;

	goal_work_mode_set = npu_bitmap_set(dev_ctx->pm.work_mode, workmode);
	if (dev_ctx->pm.work_mode == goal_work_mode_set)
		return 0;

	npu_drv_warn("enter powerup, workmode = %u\n", workmode);

	ret = npu_pm_powerup_pre_proc(dev_ctx, workmode);
	cond_return_error(ret != 0, ret,
		"power up pre_porc fail: ret = %d\n", ret);

	delta_subip = (u32)npu_pm_get_delta_subip_set(&dev_ctx->pm, workmode, POWER_UP);
	ret = npu_pm_powerup_proc(dev_ctx, workmode, delta_subip);
	cond_goto_error(ret != 0, fail, ret, ret,
		"power up fail: ret = %d\n", ret);

	ret = npu_pm_powerup_post_proc(dev_ctx, workmode);
	dev_ctx->pm.work_mode = goal_work_mode_set;
	cond_goto_error(ret != 0, post_fail, ret, ret,
		"power up post_porc fail: ret = %d\n", ret);

	npu_pm_set_power_on(workmode);
	npu_drv_warn("powerup succ, workmode_set = 0x%x, delta_subip = 0x%x\n",
		goal_work_mode_set, delta_subip);

	return ret;

post_fail:
	(void)npu_pm_unvote(dev_ctx, workmode);
fail:
	if (dev_ctx->pm.work_mode == 0) {
		dev_ctx->power_stage = NPU_PM_DOWN;
		atomic_set(&dev_ctx->power_access, 1);
	}
	return ret;
}

int npu_pm_unvote(struct npu_dev_ctx *dev_ctx, u32 workmode)
{
	int ret;
	u32 goal_work_mode_set;
	u32 delta_subip;
	struct npu_platform_info *plat_info = dev_ctx->plat_info;

	cond_return_error(plat_info == NULL, -1, "plat_info is null\n");

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_AUTO_POWER_DOWN] == NPU_FEATURE_OFF) {
		npu_drv_warn("npu auto power down switch off\n");
		return 0;
	}

	goal_work_mode_set = npu_bitmap_clear(dev_ctx->pm.work_mode, workmode);
	if (dev_ctx->pm.work_mode == goal_work_mode_set)
		return 0;

	npu_drv_warn("enter powerdown, workmode = %u\n", workmode);
	npu_pm_set_power_off(workmode);

	ret = npu_pm_powerdown_pre_proc(dev_ctx, workmode);
	if (ret != 0)
		npu_drv_err("power down pre_porc fail: ret = %d\n", ret);

	delta_subip = (u32)npu_pm_get_delta_subip_set(&dev_ctx->pm, workmode, POWER_DOWN);
	ret = npu_pm_powerdown_proc(dev_ctx, workmode, delta_subip);
	if (ret != 0)
		npu_drv_err("power down fail: ret = %d\n", ret);

	ret = npu_pm_powerdown_post_proc(dev_ctx, workmode);
	if (ret != 0)
		npu_drv_err("power down post_proc fail : ret = %d\n", ret);

	dev_ctx->pm.work_mode = goal_work_mode_set;
	npu_drv_warn("powerdown succ, workmode_set = 0x%x, delta_subip = 0x%x\n",
		goal_work_mode_set, delta_subip);

	return ret;
}

int npu_pm_exception_powerdown(struct npu_dev_ctx *dev_ctx)
{
	int ret = 0;
	int workmode_idx;
	u32 goal_work_mode_set;
	struct npu_platform_info *plat_info = dev_ctx->plat_info;

	cond_return_error(plat_info == NULL, -1, "plat_info is null\n");

	npu_drv_warn("exception powerdown enter");
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_AUTO_POWER_DOWN] == NPU_FEATURE_OFF) {
		npu_drv_warn("npu auto power down switch off\n");
		return 0;
	}

	npu_pm_delete_idle_timer(dev_ctx);

	mutex_lock(&dev_ctx->npu_power_mutex);
	goal_work_mode_set = dev_ctx->pm.work_mode;
	for (workmode_idx = 0; workmode_idx < NPU_WORKMODE_MAX; workmode_idx++) {
		if (workmode_idx == NPU_SEC)
			continue;
		if (npu_bitmap_get(dev_ctx->pm.work_mode, workmode_idx) != 0) {
			goal_work_mode_set = npu_bitmap_clear(dev_ctx->pm.work_mode, workmode_idx);
			ret = npu_pm_unvote(dev_ctx, workmode_idx);
			cond_goto_error(ret != 0, fail, ret, ret,
				"power down fail: ret = %d, workmode = %d\n",
				ret, workmode_idx);
		}
	}

	if (dev_ctx->pm.work_mode == 0)
		dev_ctx->power_stage = NPU_PM_DOWN;

	npu_drv_warn("power down succ, work_mode_set = 0x%x\n",
		dev_ctx->pm.work_mode);
	mutex_unlock(&dev_ctx->npu_power_mutex);
	return ret;
fail:
	dev_ctx->pm.work_mode = goal_work_mode_set;
	if (dev_ctx->pm.work_mode == 0) {
		dev_ctx->power_stage = NPU_PM_DOWN;
		atomic_set(&dev_ctx->power_access, 1);
	}
	mutex_unlock(&dev_ctx->npu_power_mutex);
	return ret;
}

static void npu_pm_idle_work(struct work_struct *npu_idle_work)
{
	int ret;
	struct npu_power_manage *power_manager = NULL;
	struct npu_dev_ctx *dev_ctx = NULL;
	u32 goal_work_mode_set;
	int work_state;

	npu_drv_warn("idle timer work enter\n");
	cond_return_void(npu_idle_work == NULL, "idle_work is null\n");
	power_manager = container_of(npu_idle_work, struct npu_power_manage,
		idle_work.work);
	cond_return_void(power_manager == NULL, "power_manager is null\n");
	dev_ctx = (struct npu_dev_ctx *)power_manager->private_data[0];
	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");

	mutex_lock(&dev_ctx->npu_power_mutex);
	if (dev_ctx->pm.wm_cnt[NPU_NONSEC] != 0) {
		atomic_cmpxchg(&power_manager->idle_wq_processing, WORK_ADDED,
			WORK_IDLE);
		mutex_unlock(&dev_ctx->npu_power_mutex);
		return;
	}

	work_state = atomic_read(&power_manager->idle_wq_processing);
	if (work_state == WORK_ADDED) {
		if (npu_bitmap_get(dev_ctx->pm.work_mode, NPU_NONSEC) != 0) {
			goal_work_mode_set = npu_bitmap_clear(dev_ctx->pm.work_mode, NPU_NONSEC);
			ret = npu_pm_unvote(dev_ctx, NPU_NONSEC);
			cond_goto_error(ret != 0, fail, ret, ret,
				"fail in power down : ret = %d\n", ret);
			npu_drv_warn("npu nonsec exit succ, workmode_set = 0x%x\n",
				goal_work_mode_set);
		} else {
			npu_drv_warn("npu nonsec already exit, workmode_set = 0x%x\n",
				dev_ctx->pm.work_mode);
		}

		if (dev_ctx->pm.work_mode == 0)
			dev_ctx->power_stage = NPU_PM_DOWN;

		atomic_set(&power_manager->idle_wq_processing, WORK_IDLE);
	} else {
		npu_drv_warn("work state = %d\n", work_state);
	}

	mutex_unlock(&dev_ctx->npu_power_mutex);

	return;
fail:
	dev_ctx->pm.work_mode = goal_work_mode_set;
	if (dev_ctx->pm.work_mode == 0)
		dev_ctx->power_stage = NPU_PM_DOWN;
	mutex_unlock(&dev_ctx->npu_power_mutex);
	atomic_set(&power_manager->idle_wq_processing, WORK_IDLE);
}

int npu_pm_resource_init(struct npu_dev_ctx *dev_ctx)
{
	int ret;
	u32 work_mode;
	struct npu_platform_info *plat_info = NULL;
	struct npu_power_manage *power_manager = &dev_ctx->pm;
	struct npu_interframe_idle_manager *idle_manager = NULL;

	plat_info = dev_ctx->plat_info;
	cond_return_error(plat_info == NULL, -1, "plat_info is null\n");

	ret = npu_pm_init(dev_ctx);
	cond_return_error(ret != 0, -1, "npu power manager init failed\n");

	for (work_mode = 0; work_mode < NPU_WORKMODE_MAX; work_mode++)
		power_manager->wm_cnt[work_mode] = 0;

	power_manager->private_data[0] = dev_ctx;
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN]) {
		idle_manager = &dev_ctx->pm.interframe_idle_manager;
		idle_manager->private_data = dev_ctx;
	}

	return 0;
}

void npu_pm_delay_work_init(struct npu_dev_ctx *dev_ctx)
{
	struct npu_power_manage *power_manager = &dev_ctx->pm;
	struct npu_interframe_idle_manager *idle_manager = NULL;
	struct npu_platform_info *plat_info = NULL;

	plat_info = dev_ctx->plat_info;
	cond_return_void(plat_info == NULL, "plat_info is null\n");

	atomic_set(&power_manager->idle_wq_processing, WORK_IDLE);
	INIT_DELAYED_WORK(&power_manager->idle_work, npu_pm_idle_work);

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN]) {
		idle_manager = &dev_ctx->pm.interframe_idle_manager;
		idle_manager->idle_time_out =
			NPU_INTERFRAME_IDLE_TIME_OUT_DEFAULT_VALUE;
		atomic_set(&idle_manager->wq_processing, WORK_IDLE);
		INIT_DELAYED_WORK(&idle_manager->work, npu_pm_interframe_idle_work);
	}
}

void npu_pm_adapt_init(struct npu_dev_ctx *dev_ctx)
{
	npu_pm_delay_work_init(dev_ctx);
	npu_pm_resource_init(dev_ctx);
}

int npu_pm_get_delta_subip_set(struct npu_power_manage *handle, u32 work_mode,
	int pm_ops)
{
	u32 mode_idx;
	u32 delta_subip_set;
	u32 old_subip_set;
	u32 new_subip_set = 0;
	struct npu_pm_work_strategy *work_strategy = NULL;

	work_strategy = npu_pm_get_work_strategy(handle, work_mode);
	delta_subip_set = work_strategy->subip_set.data;
	old_subip_set = handle->cur_subip_set;

	if (pm_ops == POWER_UP) {
		delta_subip_set &= (~old_subip_set);
	} else {
		for (mode_idx = 0; mode_idx < NPU_WORKMODE_MAX; mode_idx++) {
			if (npu_bitmap_get(handle->work_mode, mode_idx) &&
				(mode_idx != work_mode)) {
				work_strategy = npu_pm_get_work_strategy(handle, mode_idx);
				new_subip_set |= work_strategy->subip_set.data;
			}
		}
		delta_subip_set = (old_subip_set & (~new_subip_set));
	}

	return delta_subip_set;
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
		npu_drv_err("invalid core num %d\n", core_num);
		return -EINVAL;
	}

	if (plat_info->adapter.res_ops.npu_ctrl_core == NULL) {
		npu_drv_err("do not support ctrl core num %d\n", core_num);
		return -EINVAL;
	}

	dev_ctx = get_dev_ctx_by_id(dev_id);
	if (dev_ctx == NULL) {
		npu_drv_err("get device %d ctx fail\n", dev_id);
		return -EINVAL;
	}

	mutex_lock(&dev_ctx->npu_power_mutex);
	ret = plat_info->adapter.res_ops.npu_ctrl_core(dev_ctx, core_num);
	mutex_unlock(&dev_ctx->npu_power_mutex);

	if (ret != 0)
		npu_drv_err("ctrl device %d core num %d fail ret %d\n",
			dev_id, core_num, ret);
	else
		npu_drv_warn("ctrl device %d core num %d success\n", dev_id, core_num);

	return ret;
}

int npu_interframe_enable(struct npu_proc_ctx *proc_ctx, uint32_t enable)
{
	struct npu_dev_ctx *dev_ctx = NULL;

	dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(dev_ctx == NULL, -1, "invalid dev ctx\n");

	atomic_set(&dev_ctx->pm.interframe_idle_manager.enable, enable);
	npu_drv_debug("interframe enable = %u",
		atomic_read(&dev_ctx->pm.interframe_idle_manager.enable));

	return 0;
}
