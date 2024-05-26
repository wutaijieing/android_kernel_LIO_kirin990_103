/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Hisi fbex for "lock/unlock screen" source file
 * Create : 2020/01/02
 */

#include "fbex_dev.h"
#include "fbex_driver.h"

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/fscrypt_common.h>
#include <platform_include/see/fbe_ctrl.h>

enum g_fbex_stat {
	FBEX_IDLE = 0,
	FBEX_UFS_BUSY = 1,
	FBEX_TA_BUSY = 2,
	FBEX_BREAK = 3,
};

struct ufs_info {
	u32 user;
	u32 file;
};

static enum g_fbex_stat g_curr_stat;
/* define mutex for fbex_ca_lock_screen */
static DEFINE_MUTEX(g_fbex_comp_lock);
static struct completion g_fbex_completion;
static struct ufs_info g_ufs_info[FBEX_MAX_UFS_SLOT];
static u32 g_ufs_status;
static u32 g_ufs_lock;

static u32 fbex_get_slot(u32 user, u32 file)
{
	u32 slot;

	for (slot = 0; slot < FBEX_MAX_UFS_SLOT; slot++)
		if (g_ufs_info[slot].user == user &&
			g_ufs_info[slot].file == file)
			break;
	return slot;
}

void fbex_init_slot(u32 user, u32 file, u8 slot)
{
	if (slot >= FBEX_MAX_UFS_SLOT) {
		pr_err("%s, user 0x%x, slot 0x%x\n", __func__, user, slot);
		return;
	}

	file &= FBEX_FILE_MASK;
	g_ufs_info[slot].user = user;
	g_ufs_info[slot].file = file;
	g_ufs_status |= (0x1u << slot);
}

void fbex_deinit_slot(u32 user, u32 file)
{
	u32 slot = fbex_get_slot(user, file);
	if (slot >= FBEX_MAX_UFS_SLOT) {
		pr_err("%s, user 0x%x not install\n", __func__, user);
		return;
	}
	g_ufs_status &= ~(0x1u << slot);
}

bool fbex_slot_clean(u32 slot)
{
	/*
	 * if slot id is >= FBEX_MAX_UFS_SLOT
	 * return "true" means the slot is locked, do not use it
	 */
	if (unlikely(slot >= FBEX_MAX_UFS_SLOT))
		return true;

	return g_ufs_lock & (0x1u << slot);
}

void fbex_set_status(u32 user, bool lock)
{
	u32 slot = fbex_get_slot(user, FILE_ECE);
	if (slot >= FBEX_MAX_UFS_SLOT) {
		pr_err("%s, user 0x%x not install\n", __func__, user);
		return;
	}
	if (!(g_ufs_status & (0x1u << slot))) {
		pr_err("%s, ufs slot 0x%x is not configed\n", __func__, slot);
		return;
	}

	if (lock) /* set bit to 1, lock the slot */
		g_ufs_lock |= (0x1u << slot);
	else /* clear bit to 0, unlock the slot */
		g_ufs_lock &= ~(0x1u << slot);
	pr_info("%s , user 0x%x, status 0x%x\n", __func__, user, g_ufs_lock);
}

void fbe3_flushcache_done(void)
{
	mutex_lock(&g_fbex_comp_lock);
	if (g_curr_stat == FBEX_UFS_BUSY) {
		complete(&g_fbex_completion);
		g_curr_stat = FBEX_TA_BUSY;
	}
	else
		pr_err("screen had been unlocked 0x%x\n", g_curr_stat);
	mutex_unlock(&g_fbex_comp_lock);
}

/*
 * Function: file_encry_lock_screen
 * Parameters:
 *    user: input, user id
 *    file: unused
 *    iv: unused
 *    iv_len: unused
 * Description:
 *    This is called when lock screen.
 *    Function should be called 1 time for one user
 */
u32 file_encry_lock_screen(u32 user, u32 file _unused, u8 *iv _unused,
			   u32 iv_len _unused)
{
	u32 ret = 0;

	/* Clear SECE status in TA for lock request */
	ret = fbex_ca_lock_screen(user, FILE_SECE);

	/* Release completion first and then go with ret check and ECE handle */
	pr_info("%s, release ioctl first\n", __func__);
	fbex_work_finish(ret);
	if (ret != 0) {
		pr_err("%s, lock key error, %d\n", __func__, ret);
		return ret;
	}
	mutex_lock(&g_fbex_comp_lock);
	if (g_curr_stat != FBEX_IDLE) {
		pr_err("%s: locking, ignore this request.\n", __func__);
		mutex_unlock(&g_fbex_comp_lock);
		return 0;
	}
	g_curr_stat = FBEX_UFS_BUSY;

	fbe3_lock_in(user);

	g_curr_stat = FBEX_IDLE;
	mutex_unlock(&g_fbex_comp_lock);
	pr_info("%s: lock process finish.\n", __func__);
	return 0;
}

/*
 * Function: file_encry_unlock_screen
 * Parameters:
 *    user: input, user id
 *    file: input, file type(ECE/SECE)
 *    iv: output for ECE, iv buffer
 *    iv_len: input, iv length
 * Description:
 *    This is called when unlock screen. When file == ECE, iv will be update.
 *    Function should be called 2 times for one user(ECE/SECE)
 */
u32 file_encry_unlock_screen(u32 user, u32 file, u8 *iv, u32 iv_len)
{
	u32 ret = 0;

	pr_info("%s, unlock in\n", __func__);
	mutex_lock(&g_fbex_comp_lock);
	if (g_curr_stat == FBEX_UFS_BUSY) {
		g_curr_stat = FBEX_BREAK;
		pr_err("%s, locking, break the process\n", __func__);
	}
	ret = fbe3_unlock_in(user, file, iv, iv_len);
	mutex_unlock(&g_fbex_comp_lock);
	return ret;
}

void file_encry_init_sece(void)
{
	init_completion(&g_fbex_completion);
}
