/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description:
 */
#include "npu_svm.h"

#include <linux/err.h>
#include <linux/iommu.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm_iommu.h>
#include <linux/pid.h>
#include <linux/platform_drivers/mm_svm.h>
#include <linux/sched/task.h>
#include <securec.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
#include <linux/sched/mm.h>
#endif

#include "npu_log.h"
#include "npu_adapter.h"
#include "mm_smmu.h"

struct t_svm_pid_info {
	// the pid of the process that opened the manager device
	pid_t manager_pid;
	pid_t svm_pid;
	struct mm_svm *npu_svm;
};
#define DRVDEV_APP_MAX_NUM 20
struct t_svm_manager {
	struct mutex lock;
	uint32_t item_num;
	struct t_svm_pid_info svm_pid_info[DRVDEV_APP_MAX_NUM];
};

int npu_svm_manager_init(uint32_t devid)
{
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct t_svm_manager *svm_manager = NULL;

	if (devid >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id\n");
		return -1;
	}

	cur_dev_ctx = get_dev_ctx_by_id(devid);
	if (cur_dev_ctx == NULL) {
		npu_drv_err("cur_dev_ctx is null\n");
		return -1;
	}

	svm_manager = kmalloc(sizeof(struct t_svm_manager), GFP_KERNEL);
	if (svm_manager == NULL) {
		npu_drv_err("no sys mem to alloc svm manager\n");
		return -ENOMEM;
	}
	(void)memset_s(svm_manager, sizeof(struct t_svm_manager), 0x0,
		sizeof(struct t_svm_manager));
	cur_dev_ctx->svm_manager = (void *)svm_manager;
	mutex_init(&svm_manager->lock);
	return 0;
}

static int npu_get_item_bypid(struct t_svm_manager *svm_manager,
	pid_t svm_pid)
{
	// if find the item, return its index,
	// otherwise return the first empty item.
	int i;
	int j = -1;

	for (i = 0; i < DRVDEV_APP_MAX_NUM; i++) {
		if (svm_manager->svm_pid_info[i].svm_pid == svm_pid)
			break;
		if (svm_manager->svm_pid_info[i].manager_pid == 0 && j < 0)
			j = i;
	}
	return i < DRVDEV_APP_MAX_NUM ? i : j;
}

static int npu_check_item_bypid(struct t_svm_manager *svm_manager,
	pid_t svm_pid)
{
	int i;

	for (i = 0; i < DRVDEV_APP_MAX_NUM; i++) {
		if (svm_manager->svm_pid_info[i].svm_pid == svm_pid)
			return i;
	}
	return -1;
}

static int npu_get_item(struct t_svm_manager *svm_manager, pid_t manager_pid, pid_t svm_pid)
{
	int item;
	item = npu_get_item_bypid(svm_manager, svm_pid);
	if (unlikely(item < 0)) {
		// table is full
		npu_drv_err("svm manager table is full\n");
		return -1;
	}
	if (unlikely(svm_manager->svm_pid_info[item].manager_pid == 0)) {
		npu_drv_err("the item is not existed, manager_pid=%d, svm_pid=%d\n", manager_pid, svm_pid);
		return -1;
	}

	if (unlikely(svm_manager->svm_pid_info[item].manager_pid != manager_pid)) {
		npu_drv_err("the item has inserted by other process=%d, manager_pid=%d, svm_pid=%d\n",
			svm_manager->svm_pid_info[item].manager_pid, manager_pid, svm_pid);
		return -1;
	}

	npu_drv_info("mnt_pid=%d svm_pid=%d", svm_manager->svm_pid_info[item].manager_pid,
		svm_manager->svm_pid_info[item].svm_pid);

	return item;
}

static struct task_struct *npu_get_task_struct(pid_t svm_pid)
{
	struct pid *pid_struct = NULL;
	struct task_struct *task = NULL;

	pid_struct = find_get_pid(svm_pid);
	if (pid_struct == NULL) {
		npu_drv_err("find get pid failed\n");
		return NULL;
	}
	put_pid(pid_struct);
	task = get_pid_task(pid_struct, PIDTYPE_PID);
	if (task != NULL)
		put_task_struct(task);

	return task;
}

int npu_svm_bind(struct npu_dev_ctx *cur_dev_ctx, pid_t manager_pid, pid_t svm_pid)
{
	int ret = 0;
	int item = 0;
	struct t_svm_manager *svm_manager = NULL;
	struct task_struct *task = NULL;

	if (manager_pid <= 0 || svm_pid <= 0) {
		npu_drv_err("illegal pid\n");
		return -1;
	}

	if (cur_dev_ctx == NULL) {
		npu_drv_err("illegal cur_dev_ctx\n");
		return -1;
	}

	svm_manager = (struct t_svm_manager *)cur_dev_ctx->svm_manager;
	if (svm_manager == NULL) {
		npu_drv_err("illegal svm_manager\n");
		return -1;
	}

	mutex_lock(&svm_manager->lock);
	item = npu_get_item(svm_manager, manager_pid, svm_pid);
	if (item == -1) {
		npu_drv_err("get item failed\n");
		mutex_unlock(&svm_manager->lock);
		return -1;
	}
	if (svm_manager->svm_pid_info[item].npu_svm == NULL) {
		task = npu_get_task_struct(svm_pid);
		if (task == NULL) {
			npu_drv_err("get pid task failed\n");
			mutex_unlock(&svm_manager->lock);
			return -1;
		}
		ret = npu_plat_svm_bind(cur_dev_ctx, task, (void **)(&svm_manager->svm_pid_info[item].npu_svm));
	}

	mutex_unlock(&svm_manager->lock);
	return ret;
}

// for current and app process
// 0. item should exist 1.[bind svm] 2.get ssid
int npu_get_ssid_bypid(uint32_t devid, pid_t manager_pid, pid_t svm_pid, uint16_t *ssid, uint64_t *ttbr, uint64_t *tcr)
{
	int ret = 0;
	int item = 0;
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct t_svm_manager *svm_manager = NULL;
	struct task_struct *task = NULL;

	if (devid >= NPU_DEV_NUM || manager_pid <= 0 || svm_pid <= 0 || ssid == NULL || ttbr == NULL || tcr == NULL) {
		npu_drv_err("illegal input para\n");
		return -1;
	}

	cur_dev_ctx = get_dev_ctx_by_id(devid);
	if (cur_dev_ctx == NULL) {
		npu_drv_err("cur_dev_ctx is null\n");
		return -1;
	}
	svm_manager = (struct t_svm_manager *)cur_dev_ctx->svm_manager;
	if (svm_manager == NULL) {
		npu_drv_err("illegal svm_manager\n");
		return -1;
	}

	mutex_lock(&svm_manager->lock);
	item = npu_get_item(svm_manager, manager_pid, svm_pid);
	if (item == -1) {
		npu_drv_err("get item failed\n");
		goto Complete;
	}
	if (svm_manager->svm_pid_info[item].npu_svm == NULL) {
		task = npu_get_task_struct(svm_pid);
		if (task == NULL) {
			npu_drv_err("get pid task failed\n");
			goto Complete;
		}
		ret = npu_plat_svm_bind(cur_dev_ctx, task, (void **)(&svm_manager->svm_pid_info[item].npu_svm));
		if (ret != 0) {
			npu_drv_err("fail to npu_plat_svm_bind, ret = %d\n", ret);
			goto Complete;
		}
	}

	ret = mm_svm_get_ssid(svm_manager->svm_pid_info[item].npu_svm, ssid, ttbr, tcr);
	if (ret != 0) {
		npu_drv_err("fail to get ssid, ret = %d\n", ret);
		goto Complete;
	}

Complete:
	mutex_unlock(&svm_manager->lock);
	return ret;
}

// app process:called by CreateProcess(Runtime)
// AicpServer:called by open manager device
// 1. item  should not exist
// 2. insert a item, and set flag
int npu_insert_item_bypid(uint32_t devid, pid_t manager_pid, pid_t svm_pid)
{
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct t_svm_manager *svm_manager = NULL;
	int i;
	int ret = 0;
	struct pid *pid_struct = NULL;
	struct task_struct *task = NULL;

	if (manager_pid <= 0 || svm_pid <= 0) {
		npu_drv_err("illegal pid\n");
		return -1;
	}

	if (devid >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id\n");
		return -1;
	}

	cur_dev_ctx = get_dev_ctx_by_id(devid);
	if (cur_dev_ctx == NULL) {
		npu_drv_err("cur_dev_ctx is null\n");
		return -1;
	}
	svm_manager = (struct t_svm_manager *)cur_dev_ctx->svm_manager;

	mutex_lock(&svm_manager->lock);
	i = npu_get_item_bypid(svm_manager, svm_pid);
	if (i < 0) {
		// table is full
		npu_drv_err("svm manager table is full\n");
		ret = -1;
		goto Complete;
	}
	if (svm_manager->svm_pid_info[i].manager_pid == 0) {
		// create a new item
		pid_struct = find_get_pid(svm_pid);
		if (pid_struct == NULL) {
			npu_drv_err("find get pid failed\n");
			ret = -1;
			goto Complete;
		}
		task = get_pid_task(pid_struct, PIDTYPE_PID);
		if (task == NULL) {
			npu_drv_err("get pid task failed\n");
			ret = -1;
			goto Complete;
		}
		ret = mm_svm_flag_set(task, 1);
		if (ret != 0) {
			npu_drv_err("mm svm flag set failed\n");
			goto Complete;
		}
		svm_manager->svm_pid_info[i].manager_pid = manager_pid;
		svm_manager->svm_pid_info[i].svm_pid = svm_pid;
		svm_manager->item_num++;

		npu_drv_warn("insert a new item, manager_pid=%d, svm_pid=%d, item_num=%u",
			manager_pid, svm_pid, svm_manager->item_num);
	} else {
		npu_drv_err("the pid is existed, exe_p_pid=%d, manager_pid=%d, svm_pid=%d\n",
			svm_manager->svm_pid_info[i].manager_pid, manager_pid, svm_pid);
		ret = -1;
		goto Complete;
	}

Complete:
	if (task != NULL)
		put_task_struct(task);
	if (pid_struct != NULL)
		put_pid(pid_struct);
	mutex_unlock(&svm_manager->lock);
	return ret;
}

// for app process
// 1. item should exist
// 2. unbind + unsetflag + delete item
int npu_release_item_bypid(uint32_t devid, pid_t manager_pid, pid_t svm_pid)
{
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct t_svm_manager *svm_manager = NULL;
	struct pid *pid_struct = NULL;
	struct task_struct *task = NULL;
	int ret = 0;
	int i;

	if (manager_pid <= 0 || svm_pid <= 0) {
		npu_drv_err("illegal manager_pid=%d, svm_pid=%d\n",
			manager_pid, svm_pid);
		return -1;
	}

	if (devid >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id\n");
		return -1;
	}

	cur_dev_ctx = get_dev_ctx_by_id(devid);
	if (cur_dev_ctx == NULL) {
		npu_drv_err("cur_dev_ctx is null\n");
		return -1;
	}
	svm_manager = (struct t_svm_manager *)cur_dev_ctx->svm_manager;

	mutex_lock(&svm_manager->lock);
	i = npu_check_item_bypid(svm_manager, svm_pid);
	if (i < 0 || svm_manager->svm_pid_info[i].manager_pid != manager_pid) {
		npu_drv_err("pid is invalid, the manager_pid=%d, svm_pid=%d \n",
			manager_pid, svm_pid);
		goto Complete;
	}
	// unbind the svm
	if (svm_manager->svm_pid_info[i].npu_svm) {
		mm_svm_unbind_task(svm_manager->svm_pid_info[i].npu_svm);
		svm_manager->svm_pid_info[i].npu_svm = NULL;
	}

	// delete the item
	svm_manager->svm_pid_info[i].manager_pid = 0;
	svm_manager->svm_pid_info[i].svm_pid = 0;
	svm_manager->item_num--;
	npu_drv_warn("delete a new item, manager_pid=%d, svm_pid=%d, item_num=%u",
		manager_pid, svm_pid, svm_manager->item_num);

	// unsetflag, ai client exit first, then call api rtProcessDestory, so pid is invalid
	pid_struct = find_get_pid(svm_pid);
	if (pid_struct == NULL) {
		npu_drv_warn("pid_struct is null, pid = %d", svm_pid);
		goto Complete;
	}
	task = get_pid_task(pid_struct, PIDTYPE_PID);
	if (task == NULL) {
		npu_drv_warn("pid task is null, pid = %d", svm_pid);
		goto Pid_out;
	}
	mm_svm_flag_set(task, 0);
	put_task_struct(task);

Pid_out:
	put_pid(pid_struct);
Complete:
	mutex_unlock(&svm_manager->lock);
	return ret;
}

// for current and app process
// if pid == 0, do device clear
// if called by powerdown(flag=0):unbind
// if called by close device(flag=1):
// unbind + (unsetflag + delete item)(exept current->tgid)
int npu_clear_pid_ssid_table(uint32_t devid, pid_t pid, int flag)
{
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct t_svm_manager *svm_manager = NULL;
	struct pid *pid_struct = NULL;
	struct task_struct *task = NULL;
	pid_t svm_pid;
	int i;

	if (devid >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id\n");
		return -1;
	}

	cur_dev_ctx = get_dev_ctx_by_id(devid);
	if (cur_dev_ctx == NULL) {
		npu_drv_err("cur_dev_ctx is null\n");
		return -1;
	}
	svm_manager = (struct t_svm_manager *)cur_dev_ctx->svm_manager;

	mutex_lock(&svm_manager->lock);
	for (i = 0; i < DRVDEV_APP_MAX_NUM; i++) {
		svm_pid = svm_manager->svm_pid_info[i].svm_pid;
		if ((svm_manager->svm_pid_info[i].manager_pid != pid) && (pid != 0))
			continue;
		// 1. unbind
		if (svm_manager->svm_pid_info[i].npu_svm != NULL) {
			mm_svm_unbind_task(svm_manager->svm_pid_info[i].npu_svm);
			npu_drv_warn("unbind item, mnt_pid=%d svm_pid=%d",
				svm_manager->svm_pid_info[i].manager_pid, svm_pid);
			svm_manager->svm_pid_info[i].npu_svm = NULL;
		}

		if (flag && svm_pid != pid) {
			svm_manager->svm_pid_info[i].manager_pid = 0;
			svm_manager->svm_pid_info[i].svm_pid = 0;
			svm_manager->item_num--;

			// 2. unset flag + delete item
			pid_struct = find_get_pid(svm_pid);
			if (pid_struct == NULL) {
				npu_drv_warn("find get pid failed\n");
				continue;
			}
			task = get_pid_task(pid_struct, PIDTYPE_PID);
			if (task == NULL) {
				put_pid(pid_struct);
				npu_drv_warn("get pid task failed\n");
				continue;
			}
			mm_svm_flag_set(task, 0);
			put_task_struct(task);
			put_pid(pid_struct);
		}
	}
	mutex_unlock(&svm_manager->lock);
	npu_drv_warn("clear table when %s", flag == 0 ? "powerdown" :
		"close npu device");
	return 0;
}

int npu_svm_manager_destroy(uint32_t devid)
{
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct t_svm_manager *svm_manager = NULL;

	if (devid >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id\n");
		return -1;
	}

	cur_dev_ctx = get_dev_ctx_by_id(devid);
	if (cur_dev_ctx == NULL) {
		npu_drv_err("cur_dev_ctx is null\n");
		return -1;
	}

	npu_clear_pid_ssid_table(devid, 0, 1);

	svm_manager = (struct t_svm_manager *)cur_dev_ctx->svm_manager;
	mutex_destroy(&svm_manager->lock);
	kfree(svm_manager);
	cur_dev_ctx->svm_manager = NULL;
	return 0;
}

int npu_get_ttbr(u64 *ttbr, u64 *tcr, pid_t process_id)
{
	unsigned long asid;
	struct mm_struct *mm = NULL;
	struct pid *pid_struct = NULL;
	struct task_struct *task = NULL;

	if ((ttbr == NULL) || (tcr == NULL)) {
		npu_drv_err("ttbr or tcr is null\n");
		return -EINVAL;
	}

	pid_struct = find_get_pid(process_id);
	if (pid_struct == NULL) {
		npu_drv_err("pid_struct is null\n");
		return -ESRCH;
	}

	task = get_pid_task(pid_struct, PIDTYPE_PID);
	if (task == NULL) {
		put_pid(pid_struct);
		npu_drv_err("get pid task failed\n");
		return -ESRCH;
	}

	mm = get_task_mm(task);
	if (mm == NULL) {
		put_task_struct(task);
		put_pid(pid_struct);
		npu_drv_err("get mm is null\n");
		return -ESRCH;
	}

	asid = ASID(mm);
	*ttbr = virt_to_phys(mm->pgd) | (asid << 48);
	*tcr  = read_sysreg(tcr_el1);
	npu_drv_debug("pgdaddr:0x:%pK, context:0x%pK, pa:0x%pK\n",
		mm->pgd, (u64 *)(mm->pgd), (void *)(uintptr_t)virt_to_phys(mm->pgd));

	mmput(mm);
	npu_drv_debug("asid:%lu ,ttbr:0x%pK, tcr:0x%pK\n", asid,
		(void *)(uintptr_t)*ttbr, (void *)(uintptr_t)*tcr);

	put_task_struct(task);
	put_pid(pid_struct);
	return 0;
}
