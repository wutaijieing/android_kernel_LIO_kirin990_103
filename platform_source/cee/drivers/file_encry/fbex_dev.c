/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Hisi fbex for "ioctrl cmd" source file
 * Create : 2020/01/02
 */

#include "fbex_dev.h"
#include "fbex_driver.h"
#include "fbex_debug.h"

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/miscdevice.h>
#include <platform_include/see/fbe_ctrl.h>

struct fbex_data {
	u32 ret;
	u32 cmd;
	struct fbex_req req;
};

/* mutex for hisi_fbex_ioctrl_cmd */
static DEFINE_MUTEX(g_fbex_lock);
static struct completion g_fbex_ta_comp;

/* global fbex work queue needed */
static struct fbex_data g_fbex_data;
static struct work_struct g_fbex_work;
struct workqueue_struct *g_fbex_response_wq;

void fbex_work_finish(u32 ret)
{
	g_fbex_data.ret = ret;
	complete(&g_fbex_ta_comp);
}

static void hisi_fbex_start_fn(struct work_struct *work)
{
	u32 ret;
	u32 cmd;
	u32 user = g_fbex_data.req.user;
	u32 file = g_fbex_data.req.file;
	u32 len = g_fbex_data.req.len;
	u8 *iv = g_fbex_data.req.key;

	static const file_encry_cb dispatch[] = { FILE_ENCRY_LIST };

	pr_info("%s, cmd 0x%x, user 0x%x, file 0x%x, len 0x%x, flag 0x%x\n",
		__func__, g_fbex_data.cmd, user, file, len, g_fbex_data.req.flag);
	g_fbex_data.ret = FBE3_ERROR_CMD_START;
	if (g_fbex_data.cmd >= (u32)ARRAY_SIZE(dispatch)) {
		pr_err("%s, unsupported cmd id, 0x%x\n", __func__,
		       g_fbex_data.cmd);
		ret = FBE3_ERROR_CMD_UNSUPPORT;
		goto out;
	}
	cmd = g_fbex_data.cmd & SEC_FILE_ENCRY_CMD_ID_MASK;
	if (!dispatch[cmd]) {
		pr_err("%s, unregistered call back\n", __func__);
		ret = FBE3_ERROR_CMD_INVALID;
		goto out;
	}
	/*
	 * NOTE, using the "uint32 file" for:
	 *    1. file type bit[0..7]
	 *    2. cmd flag bit[8..15]
	 *    3. you can extend it yourself
	 * STEPS:
	 *    1. clear high bit[7..31]
	 *    2. amend flag to bit[7..15]
	 */
	file &= FBEX_FILE_MASK;
	file |= (g_fbex_data.req.flag << FBEX_FILE_LEN);
	ret = dispatch[cmd](user, file, iv, len);
	/* if cmd == lock screen, ignore the ret and do not release comp again */
	if (cmd == SEC_FILE_ENCRY_CMD_ID_LOCK_SCREEN)
		return;
out:
	fbex_work_finish(ret);
}

const static struct cmd_table g_cmd_table[] = {
	{ HISI_FBEX_ADD_IV, SEC_FILE_ENCRY_CMD_ID_VOLD_ADD_IV },
	{ HISI_FBEX_DEL_IV, SEC_FILE_ENCRY_CMD_ID_VOLD_DELETE_IV },
	{ HISI_FBEX_LOCK_SCREEN, SEC_FILE_ENCRY_CMD_ID_LOCK_SCREEN },
	{ HISI_FBEX_UNLOCK_SCREEN, SEC_FILE_ENCRY_CMD_ID_UNLOCK_SCREEN },
	{ HISI_FBEX_USER_LOGOUT, SEC_FILE_ENCRY_CMD_ID_USER_LOGOUT },
	{ HISI_FBEX_STATUS_REPORT, SEC_FILE_ENCRY_CMD_ID_STATUS_REPORT },
};

static u32 fbex_ioccmd_to_id(u32 cmd)
{
	unsigned int i;
	u32 count = sizeof(g_cmd_table) / sizeof(g_cmd_table[0]);

	for (i = 0; i < count; i++)
		if (g_cmd_table[i].cmd == cmd)
			return g_cmd_table[i].id;

	return SEC_FILE_ENCRY_CMD_ID_MAX_SUPPORT;
}

static u32 hisi_fbex_copy_out(u32 cmd, struct fbex_req *from, void __user *to)
{
	u32 ret;

	if (cmd != SEC_FILE_ENCRY_CMD_ID_VOLD_ADD_IV &&
	    cmd != SEC_FILE_ENCRY_CMD_ID_UNLOCK_SCREEN)
		return 0;

	if (!to) {
		pr_err("%s, user buff is NULL\n", __func__);
		return FBE3_ERROR_BUFFER_NULL;
	}
	ret = copy_to_user(to, from, sizeof(g_fbex_data.req));
	if (ret) {
		pr_err("%s, copy to user fail\n", __func__);
		return FBE3_ERROR_COPY_FAIL;
	}
	return 0;
}

static u32 hisi_fbex_cmd_dispatch(u32 cmd, void __user *argp)
{
	u32 ret;
	u32 id;

	if (!argp) {
		pr_err("%s, invalid input NULL.\n", __func__);
		file_encry_record_error(0, 0, 0, FBE3_ERROR_INIT_BUFFER);
		return FBE3_ERROR_INIT_BUFFER;
	}
	ret = copy_from_user(&g_fbex_data.req, argp, sizeof(g_fbex_data.req));
	if (ret) {
		pr_err("%s, copy from user fail\n", __func__);
		file_encry_record_error(0, 0, 0, FBE3_ERROR_INIT_INFO);
		return FBE3_ERROR_INIT_INFO;
	}
	id = fbex_ioccmd_to_id(cmd);
	g_fbex_data.cmd = id;
	pr_info("%s, cmd 0x%x, user 0x%x, file 0x%x, len 0x%x, flag 0x%x\n",
		__func__, g_fbex_data.cmd, g_fbex_data.req.user,
		g_fbex_data.req.file, g_fbex_data.req.len,
		g_fbex_data.req.flag);

	queue_work(g_fbex_response_wq, &g_fbex_work);
	if (!wait_for_completion_timeout(&g_fbex_ta_comp,
					 msecs_to_jiffies(FBEX_KEY_TIMEOUT))) {
		pr_err("%s: timeout!\n", __func__);
		file_encry_record_error(g_fbex_data.cmd, g_fbex_data.req.user,
					g_fbex_data.req.file,
					FBE3_ERROR_CKEY_TIMEER_OUT);
		return FBE3_ERROR_CKEY_TIMEER_OUT;
	}
	if (g_fbex_data.ret != 0) {
		pr_err("%s, error, ret = 0x%x\n", __func__, g_fbex_data.ret);
		file_encry_record_error(g_fbex_data.cmd, g_fbex_data.req.user,
					g_fbex_data.req.file, g_fbex_data.ret);
		return g_fbex_data.ret;
	}
	/* check the key index for adding key cmd */
	if (id == SEC_FILE_ENCRY_CMD_ID_VOLD_ADD_IV) {
		u8 index = g_fbex_data.req.key[KEY_LEN - 1];
		if (index >= FBEX_MAX_UFS_SLOT) {
			pr_err("%s,index err 0x%x, cmd 0x%x, user 0x%x, file 0x%x, len 0x%x\n",
				__func__, index, g_fbex_data.cmd, g_fbex_data.req.user,
				g_fbex_data.req.file, g_fbex_data.req.len);
			file_encry_record_error(g_fbex_data.cmd,
				g_fbex_data.req.user,
				g_fbex_data.req.file,
				FBE3_ERROR_INVALID_IV);
			return FBE3_ERROR_INVALID_IV;
		}
	}

	ret = hisi_fbex_copy_out(id, &g_fbex_data.req, argp);
	if (ret != 0) {
		file_encry_record_error(g_fbex_data.cmd, g_fbex_data.req.user,
					g_fbex_data.req.file, ret);
		return ret;
	}
	(void)memset(&g_fbex_data.req, 0, sizeof(g_fbex_data.req));
	/* return result value from FBE3 service */
	return g_fbex_data.ret;
}

static long fbex_cmd_unlocked_ioctl(struct file *file, unsigned int cmd,
				    unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)(uintptr_t)arg;

	mutex_lock(&g_fbex_lock);
	pr_info("%s, fbex ioctrl enter\n", __func__);
	ret = hisi_fbex_cmd_dispatch(cmd, argp);
	pr_info("%s, fbex ioctrl exit\n", __func__);
	mutex_unlock(&g_fbex_lock);

	return (long)ret;
}

static ssize_t fbex_read(struct file *file, char __user *buf,
			 size_t size, loff_t *data)
{
	char msp_offline[] = "false";
	char msp_online[] = "true";
	if (!buf) {
		pr_err("%s, input buf is NULL\n", __func__);
		return FBE3_ERROR_BUFFER_NULL;
	}
	if (fbex_is_msp_online())
		return simple_read_from_buffer(buf, size, data, msp_online,
					       strlen(msp_online));
	return simple_read_from_buffer(buf, size, data, msp_offline,
				       strlen(msp_offline));
}

static int fbex_open(struct inode *inode, struct file *filep)
{
	return 0;
}

static const struct file_operations g_fbex_cmd_fops = {
	.owner = THIS_MODULE,
	.read = fbex_read,
	.unlocked_ioctl = fbex_cmd_unlocked_ioctl,
	.open = fbex_open,
};

static struct miscdevice g_fbex_dev_cmd = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fbex_cmd",
	.fops = &g_fbex_cmd_fops,
	.mode = 0600, /* permission */
};

void __exit hisi_fbex_module_exit(void)
{
	if (g_fbex_dev_cmd.list.prev)
		misc_deregister(&g_fbex_dev_cmd);
	fbex_cleanup_debugfs();
}

int __init hisi_fbex_module_init(void)
{
	int ret;

	/* init global completion */
	init_completion(&g_fbex_ta_comp);
	file_encry_init_sece();
	ret = misc_register(&g_fbex_dev_cmd);
	if (ret != 0) {
		pr_err("failed to register fbex cmd\n");
		goto out;
	}

	ret = fbex_init_debugfs();
	if (ret != 0) {
		pr_err("failed to register fbex debug\n");
		goto out;
	}

	ret = fbex_init_driver();
	if (ret != 0) {
		pr_err("failed to register fbex driver\n");
		goto out;
	}

	g_fbex_response_wq = create_singlethread_workqueue("fbex_work");
	if (!g_fbex_response_wq) {
		pr_err("Create fbex work failed\n");
		ret = -1;
		goto out;
	}
	INIT_WORK(&g_fbex_work, hisi_fbex_start_fn);
	pr_info("%s success\n", __func__);
	return 0;
out:
	hisi_fbex_module_exit();
	pr_err("%s fail, %d\n", __func__, ret);
	return ret;
}

module_init(hisi_fbex_module_init);
module_exit(hisi_fbex_module_exit);

MODULE_DESCRIPTION("hisi fbex driver");
MODULE_ALIAS("hisi fbex driver");
MODULE_LICENSE("GPL");
