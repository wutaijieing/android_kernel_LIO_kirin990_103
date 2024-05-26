/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description:  dev drvier to communicate with sensorhub di app
 * Author:       Huawei
 * Create:       2020-07-07
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <securec.h>
#include "inputhub_api/inputhub_api.h"
#include "common/common.h"

#ifdef __LLT_UT__
#define STATIC
#else
#define STATIC static
#endif

#define di_log_info(msg...) pr_info("[I/IOMCU_DI]" msg)
#define di_log_err(msg...) pr_err("[E/IOMCU_DI]" msg)
#define di_log_warn(msg...) pr_warn("[W/IOMCU_DI]" msg)

#define DI_DRV_NAME "hisilicon,iomcu_di"

#define CMD_UDI_STATUS_NOTIFY   0x1F
#define DI_READ_CACHE_CNT       5
#define DI_IOCTL_WAIT_TMOUT     5000 /* ms */
#define DI_RESET_NOTIFY         0xFFFF
#define DI_IOCTL_ALLOC_MAX_SIZE     (5 * 1024 * 1024)

/* ioctl cmd define */
#define DI_IO                           0xB4
#define DI_IO_IOCTL_CONFIG              _IOW(DI_IO, 0xD1, short)
#define DI_IO_IOCTL_GET_PHYS            _IOW(DI_IO, 0xD2, short)
#define DI_IO_IOCTL_FREE_PHYS           _IOW(DI_IO, 0xD3, short)

struct udi_shmem_node {
	struct list_head head_list;
	int udi_index;
	unsigned int size;
	void *cpu_addr;
	dma_addr_t dma_handle;
};

struct udi_user_info {
	int udi_index;
	unsigned int size;
	unsigned long long *pa;
};

static int g_udi_index = 0;
static LIST_HEAD(g_udi_shemem_list);

struct di_read_data {
	u32 recv_len;
	void *p_recv;
};

#pragma pack(1)
/* ap -> sensorhub */
struct udi_config {
	u8 cmd;
	u8 mode;
	u16 recv1;
	u32 buff_addr;
	u32 buff_size;
	u32 resv2;
};

/* sensorhub -> ap */
struct udi_notify_sh {
	struct pkt_header hd;
	u8 data[0];
};

#pragma pack()
struct di_priv {
	struct device *self;
	struct completion di_wait;
	struct completion read_wait;
	struct mutex read_mutex; /* Used to protect read ops */
	struct mutex di_mutex; /* Used to protect ops on ioctl & open */
	struct kfifo read_kfifo;
	int ref_cnt;
	int sh_recover_flag;
};

STATIC struct di_priv g_di_priv = { 0 };

STATIC void di_dev_wait_init(struct completion *p_wait)
{
	if (p_wait == NULL) {
		di_log_err("di_dev_wait_init: wait NULL\n");
		return;
	}

	init_completion(p_wait);
}

STATIC int di_dev_wait_completion(struct completion *p_wait, unsigned int tm_out)
{
	if (p_wait == NULL) {
		di_log_err("di_dev_wait_completion: wait NULL\n");
		return -EFAULT;
	}

	di_log_info("di_dev_wait_completion: waitting\n");
	if (tm_out != 0) {
		if (wait_for_completion_interruptible_timeout(
		    p_wait, msecs_to_jiffies(tm_out)) == 0) {
			di_log_warn("di_dev_wait_completion: wait timeout\n");
			return -ETIMEOUT;
		}
	} else {
		if (wait_for_completion_interruptible(p_wait) != 0) {
			di_log_warn("di_dev_wait_completion: wait interrupted.\n");
			return -EFAULT;
		}
	}

	return 0;
}

STATIC void di_dev_complete(struct completion *p_wait)
{
	if (p_wait == NULL) {
		di_log_err("di_dev_complete: wait NULL\n");
		return;
	}

	complete(p_wait);
}

STATIC int di_send_data_to_mcu(void *buf, int len)
{
	struct write_info winfo = {0};
	struct read_info rd = {0};
	int ret;

	winfo.tag = TAG_UDI;
	winfo.cmd = CMD_CMN_CONFIG_REQ;
	winfo.wr_buf = buf;
	winfo.wr_len = len;
	ret = write_customize_cmd(&winfo, &rd, true);
	if (ret != 0)
		di_log_err("%s: di_send_data_to_mcu fail, ret=%d\n", __func__, ret);

	return ret;
}

STATIC int di_ioctl_udi_config(unsigned long arg)
{
	struct udi_config di = {0};
	int ret;
	void *p_arg = (void *)((uintptr_t)arg);

	if (p_arg == NULL) {
		di_log_err("%s: arg NULL.\n", __func__);
		return -ENODATA;
	}

	if (copy_from_user((void *)&di, p_arg, sizeof(struct udi_config))) {
		di_log_err("%s: copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = di_send_data_to_mcu((void *)&di, sizeof(struct udi_config));
	return ret;
}

STATIC int di_dev_ioctl_timeout_handler(void)
{
	struct di_read_data read_data = {0};
	int ret = 0;
	unsigned int len;

	mutex_lock(&g_di_priv.read_mutex);
	if (kfifo_avail(&g_di_priv.read_kfifo) < sizeof(struct di_read_data)) {
		di_log_err("%s: read_kfifo is full, drop upload data.\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	read_data.recv_len = sizeof(u32);
	read_data.p_recv = kzalloc(sizeof(u32), GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		di_log_err("Failed to alloc memory for sensorhub reset message...\n");
		ret = -ENOMEM;
		goto err;
	}

	*(u32 *)read_data.p_recv = DI_RESET_NOTIFY;
	len = kfifo_in(&g_di_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct di_read_data));
	if (len == 0) {
		di_log_err("%s: kfifo_in failed\n", __func__);
		ret = -ENOSPC;
		goto err;
	}

	mutex_unlock(&g_di_priv.read_mutex);
	di_dev_complete(&g_di_priv.read_wait);
	di_log_info("%s\n", __func__);

	return 0;
err:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

	mutex_unlock(&g_di_priv.read_mutex);

	return ret;
}

STATIC int di_ioctl_dma_alloc(unsigned long arg)
{
	struct udi_user_info user_info = { 0 };
	struct udi_shmem_node *shmem_node = NULL;
	void *p_arg = (void *)((uintptr_t)arg);

	if (p_arg == NULL) {
		di_log_err("%s: arg NULL...\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&user_info, p_arg, sizeof(struct udi_user_info))) {
		di_log_err("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	shmem_node = (struct udi_shmem_node *)kzalloc(sizeof(struct udi_shmem_node), GFP_ATOMIC);
	if (shmem_node == NULL) {
		di_log_err("%s: kzalloc failed\n", __func__);
		return -EFAULT;
	}

	shmem_node->udi_index = g_udi_index++;
	shmem_node->size = user_info.size;
	if (shmem_node->size > DI_IOCTL_ALLOC_MAX_SIZE) {
		di_log_err("%s: alloc size is %d\n", __func__, shmem_node->size);
		goto dma_alloc_fail;
	}
	shmem_node->cpu_addr = dma_alloc_coherent(g_di_priv.self, shmem_node->size, &shmem_node->dma_handle, GFP_KERNEL);
	if (shmem_node->cpu_addr == NULL) {
		di_log_err("%s: dma_alloc_coherent failed\n", __func__);
		goto dma_alloc_fail;
	}

	list_add_tail(&shmem_node->head_list, &g_udi_shemem_list);
	user_info.udi_index = shmem_node->udi_index;
	user_info.pa = (unsigned long long *)(uintptr_t)(shmem_node->dma_handle);
	if (copy_to_user(p_arg, &user_info, sizeof(struct udi_user_info))) {
		di_log_err("%s: copy_to_user failed\n", __func__);
		goto copy_to_user_fail;
	}
	di_log_info("%s: success udi_index = %d, pa = 0x%llx, size = %u\n",
		__func__, user_info.udi_index,(unsigned long long)(uintptr_t)user_info.pa, user_info.size);
	return 0;

copy_to_user_fail:
	list_del(&shmem_node->head_list);
	dma_free_coherent(g_di_priv.self, shmem_node->size, shmem_node->cpu_addr, shmem_node->dma_handle);
dma_alloc_fail:
	kfree(shmem_node);
	return -EFAULT;
}

STATIC int di_ioctl_dma_free(unsigned long arg)
{
	struct udi_user_info user_info = { 0 };
	struct udi_shmem_node *shmem_node = NULL;
	struct udi_shmem_node *tmp_n = NULL;
	void *p_arg = (void *)((uintptr_t)arg);

	if (p_arg == NULL) {
		di_log_err("%s: arg NULL...\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&user_info, p_arg, sizeof(struct udi_user_info))) {
		di_log_err("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	list_for_each_entry_safe(shmem_node, tmp_n, &g_udi_shemem_list, head_list) {
		if ((unsigned long long *)(uintptr_t)shmem_node->dma_handle == user_info.pa) {
			dma_free_coherent(g_di_priv.self, shmem_node->size, shmem_node->cpu_addr, shmem_node->dma_handle);
			di_log_info("%s: udi_index %d, pa = 0x%llx\n",
				__func__, shmem_node->udi_index, shmem_node->dma_handle);
			list_del(&shmem_node->head_list);
			kfree(shmem_node);
			user_info.udi_index = -1;
			user_info.pa = NULL;
			break;
		}
	}

	if (user_info.pa != NULL) {
		di_log_err("%s: failed, pa = 0x%llx not found in shemem_list\n", __func__,(unsigned long long)(uintptr_t)user_info.pa);
		return -EFAULT;
	}

	if (copy_to_user(p_arg, &user_info, sizeof(struct udi_user_info))) {
		di_log_err("%s: copy_to_user failed\n", __func__);
		return -EFAULT;
	}
	return 0;
}

STATIC long di_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;

	if (file == NULL) {
		di_log_err("%s: invalid param\n", __func__);
		return -EINVAL;
	}

	di_log_info("%s: cmd: [0x%x]\n", __func__, cmd);
	mutex_lock(&g_di_priv.di_mutex);
	if (g_di_priv.sh_recover_flag == 1) {
		mutex_unlock(&g_di_priv.di_mutex);
		di_log_info("%s: sensorhub in recover mode\n", __func__);
		return -EBUSY;
	}

	reinit_completion(&g_di_priv.di_wait);
	switch (cmd) {
	case DI_IO_IOCTL_CONFIG:
		ret = di_ioctl_udi_config(arg);
		break;
	case DI_IO_IOCTL_GET_PHYS:
		ret = di_ioctl_dma_alloc(arg);
		break;
	case DI_IO_IOCTL_FREE_PHYS:
		ret = di_ioctl_dma_free(arg);
		break;
	default:
		mutex_unlock(&g_di_priv.di_mutex);
		di_log_err("%s: unknown cmd : %d\n", __func__, cmd);
		return -EINVAL;
	}

	mutex_unlock(&g_di_priv.di_mutex);
	if (ret == -ETIMEOUT) {
		if (di_dev_ioctl_timeout_handler())
			di_log_err("%s: di_dev_ioctl_timeout_handler err\n", __func__);
	}

	return ret;
}

STATIC int di_status_notify(const struct pkt_header *head)
{
	const struct udi_notify_sh *notify = NULL;
	struct di_read_data read_data = {0};
	int ret;
	unsigned int len;

	if (head == NULL) {
		di_log_err("%s: input head is null\n", __func__);
		return -EINVAL;
	}
	notify = (struct udi_notify_sh *)head;
	di_log_info("%s: tag[%d], cmd[%d], length[%d]\n", __func__, head->tag, head->cmd, head->length);
	mutex_lock(&g_di_priv.read_mutex);
	if (kfifo_avail(&g_di_priv.read_kfifo) < sizeof(struct di_read_data)) {
		di_log_err("%s: read_kfifo is full, drop upload data.\n", __func__);
		ret = -ENOMEM;
		goto err_reply_req;
	}

	read_data.recv_len = head->length;
	read_data.p_recv = kzalloc(read_data.recv_len, GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		di_log_err("Failed to alloc memory to save data...\n");
		ret = -ENOMEM;
		goto err_reply_req;
	}

	ret = memcpy_s(read_data.p_recv, read_data.recv_len, notify->data, head->length);
	if (ret != 0) {
		di_log_err("%s: Copy IOMCU return buffer failed, length = %d, errno = %d\n", __func__,
			   read_data.recv_len, ret);
		goto err_free_recv;
	}

	len = kfifo_in(&g_di_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct di_read_data));
	if (len == 0) {
		di_log_err("%s: kfifo_in failed.\n", __func__);
		goto err_free_recv;
	}

	mutex_unlock(&g_di_priv.read_mutex);
	di_dev_complete(&g_di_priv.read_wait);
	return ret;

err_free_recv:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

err_reply_req:
	mutex_unlock(&g_di_priv.read_mutex);
	return ret;
}

STATIC void di_sensorhub_reset_handler(void)
{
	struct di_read_data read_data = {0};
	unsigned int len;

	if (g_di_priv.ref_cnt == 0) {
		di_log_info("%s: device not open\n", __func__);
		return;
	}

	mutex_lock(&g_di_priv.read_mutex);
	if (kfifo_avail(&g_di_priv.read_kfifo) < sizeof(struct di_read_data)) {
		di_log_err("%s: read_kfifo is full, drop upload data.\n", __func__);
		goto err;
	}

	read_data.recv_len = sizeof(u32);
	read_data.p_recv = kzalloc(sizeof(u32), GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		di_log_err("%s: Failed to alloc memory for sensorhub reset message...\n", __func__);
		goto err;
	}

	*(u32 *)read_data.p_recv = DI_RESET_NOTIFY;
	len = kfifo_in(&g_di_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct di_read_data));
	if (len == 0) {
		di_log_err("%s: kfifo_in failed\n", __func__);
		goto err;
	}

	mutex_unlock(&g_di_priv.read_mutex);
	g_di_priv.sh_recover_flag = 1;
	di_dev_complete(&g_di_priv.read_wait);
	di_dev_complete(&g_di_priv.di_wait);
	di_log_info("%s\n", __func__);
	return;

err:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

	mutex_unlock(&g_di_priv.read_mutex);
}

STATIC int di_sensorhub_reset_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	switch (action) {
	case IOM3_RECOVERY_IDLE:
		di_sensorhub_reset_handler();
		break;
	default:
		break;
	}

	return 0;
}

STATIC ssize_t di_dev_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	struct di_read_data read_data = {0};
	u32 error = 0;
	u32 length;
	u32 ret = 0;

	if (file == NULL || buf == NULL || count == 0) {
		di_log_err("%s: para err\n", __func__);
		return -EINVAL;
	}

	error = (unsigned int)(di_dev_wait_completion(&g_di_priv.read_wait, 0));
	if (error != 0) {
		error = 0;
		di_log_err("%s: wait_event_interruptible failed.\n", __func__);
		goto err;
	}

	mutex_lock(&g_di_priv.read_mutex);
	if (kfifo_len(&g_di_priv.read_kfifo) < sizeof(struct di_read_data)) {
		di_log_err("%s: read data failed.\n", __func__);
		mutex_unlock(&g_di_priv.read_mutex);
		goto err;
	}

	ret = kfifo_out(&g_di_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct di_read_data));
	if (ret == 0) {
		di_log_err("%s: kfifo out failed.\n", __func__);
		mutex_unlock(&g_di_priv.read_mutex);
		goto err;
	}

	if (read_data.p_recv == NULL) {
		mutex_unlock(&g_di_priv.read_mutex);
		di_log_err("%s: kfifo out is NULL.\n", __func__);
		error = 0;
		goto err;
	}

	if (count < read_data.recv_len) {
		length = count;
		di_log_err("%s: user buffer is too small\n", __func__);
	} else {
		length = read_data.recv_len;
	}

	error = length;
	if (copy_to_user(buf, read_data.p_recv, length)) {
		di_log_err("%s: failed to copy to user\n", __func__);
		error = 0;
	}
	mutex_unlock(&g_di_priv.read_mutex);
	di_log_info("%s: recv_len=%u, count=%u\n", __func__, read_data.recv_len, (u32)count);

err:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

	return error;
}

STATIC ssize_t di_dev_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	di_log_info("%s: NULL\n", __func__);
	return len;
}

STATIC void di_clear_read_kfifo(void)
{
	struct di_read_data read_data = {0};
	unsigned int ret;

	mutex_lock(&g_di_priv.read_mutex);
	while (kfifo_len(&g_di_priv.read_kfifo) >= sizeof(struct di_read_data)) {
		ret = kfifo_out(&g_di_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct di_read_data));
		if (ret == 0) {
			mutex_unlock(&g_di_priv.read_mutex);
			di_log_info("%s: kfifo empty\n", __func__);
			return;
		}
		if (read_data.p_recv != NULL)
			kfree(read_data.p_recv);
	}
	mutex_unlock(&g_di_priv.read_mutex);
}

STATIC int di_dev_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	if (inode == NULL || file == NULL) {
		di_log_err("%s: invalid param\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_di_priv.di_mutex);
	if (g_di_priv.ref_cnt != 0) {
		di_log_warn("%s: duplicate open.\n", __func__);
		mutex_unlock(&g_di_priv.di_mutex);
		return -EFAULT;
	}

	if (g_di_priv.ref_cnt == 0)
		ret = send_cmd_from_kernel(TAG_UDI, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);

	file->private_data = &g_di_priv;
	if (ret == 0)
		g_di_priv.ref_cnt++;

	g_di_priv.sh_recover_flag = 0;
	mutex_unlock(&g_di_priv.di_mutex);
	di_clear_read_kfifo();
	di_log_info("%s\n", __func__);

	return ret;
}

STATIC int di_dev_release(struct inode *inode, struct file *file)
{
	struct read_info rd = { 0 };
	struct udi_shmem_node *shmem_node = NULL;
	struct udi_shmem_node *tmp_n = NULL;

	if (inode == NULL || file == NULL) {
		di_log_err("%s: invalid param\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_di_priv.di_mutex);
	if (g_di_priv.ref_cnt == 0) {
		di_log_err("%s: ref cnt is 0.\n", __func__);
		mutex_unlock(&g_di_priv.di_mutex);
		return -EFAULT;
	}

	g_di_priv.ref_cnt--;
	if (g_di_priv.ref_cnt == 0) {
		(void)memset_s((void *)&rd, sizeof(struct read_info), 0, sizeof(struct read_info));
		send_cmd_from_kernel_response(TAG_UDI, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0, &rd);
		di_log_info("%s: got close resp\n", __func__);

		list_for_each_entry_safe(shmem_node, tmp_n, &g_udi_shemem_list, head_list) {
			dma_free_coherent(g_di_priv.self, shmem_node->size, shmem_node->cpu_addr, shmem_node->dma_handle);
				di_log_info("%s: udi_index %d free\n", __func__, shmem_node->udi_index);
			list_del(&shmem_node->head_list);
			kfree(shmem_node);
		}
		g_udi_index = 0;
		INIT_LIST_HEAD(&g_udi_shemem_list);
	}
	mutex_unlock(&g_di_priv.di_mutex);
	di_clear_read_kfifo();


	di_log_info("%s\n", __func__);

	return 0;
}

static int di_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct udi_shmem_node *shmem_node = NULL;
	int udi_index = (int)(vma->vm_pgoff);

	list_for_each_entry(shmem_node, &g_udi_shemem_list, head_list) {
		if (shmem_node->udi_index == udi_index) {
			vma->vm_pgoff = 0;
			dma_mmap_coherent(g_di_priv.self, vma, shmem_node->cpu_addr, shmem_node->dma_handle, shmem_node->size);
			di_log_info("%s: udi_index %d addr 0x%llx mmap done\n", __func__, shmem_node->udi_index, vma->vm_start);
			return 0;
		}
	}

	di_log_err("%s: udi_index: %d not found\n", __func__, udi_index);
	return -EFAULT;
}

STATIC const struct file_operations di_dev_fops = {
	.owner             = THIS_MODULE,
	.llseek            = no_llseek,
	.unlocked_ioctl    = di_dev_ioctl,
#ifdef CONFIG_COMPAT
	compat_ioctl       = di_dev_ioctl,
#endif
	.open              = di_dev_open,
	.release           = di_dev_release,
	.read              = di_dev_read,
	.write             = di_dev_write,
	.mmap              = di_dev_mmap,
};

STATIC struct miscdevice di_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "iomcu_di",
	.fops =     &di_dev_fops,
};

STATIC struct notifier_block di_reboot_notify = {
	.notifier_call = di_sensorhub_reset_notifier,
	.priority = -1,
};

STATIC int __init di_dev_init(struct platform_device *pdev)
{
	int ret = 0;
	int len;

	if (get_contexthub_dts_status()) {
		di_log_err("sensorhub disabled....\n");
		return -ENODEV;
	}

	ret = misc_register(&di_miscdev);
	if (ret != 0) {
		di_log_err("%s: cannot register miscdev err=%d\n", __func__, ret);
		return ret;
	}

	mutex_init(&g_di_priv.read_mutex);
	mutex_init(&g_di_priv.di_mutex);
	di_dev_wait_init(&g_di_priv.di_wait);
	di_dev_wait_init(&g_di_priv.read_wait);

	ret = register_mcu_event_notifier(TAG_UDI, CMD_UDI_STATUS_NOTIFY, di_status_notify);
	if (ret != 0) {
		di_log_err("%s: register CMD_UDI_STATUS_NOTIFY notifier failed. [%d]\n", __func__, ret);
		goto err1;
	}

	ret = register_iom3_recovery_notifier(&di_reboot_notify);
	if (ret < 0) {
		di_log_err("%s: register_iom3_recovery_notifier fail\n", __func__);
		goto err2;
	}

	len = kfifo_alloc(&g_di_priv.read_kfifo, sizeof(struct di_read_data) * DI_READ_CACHE_CNT, GFP_KERNEL);
	if (len != 0) {
		di_log_err("%s: kfifo alloc failed.\n", __func__);
		ret = -ENOMEM;
		goto err2;
	}
	g_di_priv.ref_cnt = 0;
	g_di_priv.self = &(pdev->dev);

	return 0;

err2:
	unregister_mcu_event_notifier(TAG_UDI, CMD_UDI_STATUS_NOTIFY, di_status_notify);
err1:
	misc_deregister(&di_miscdev);
	di_log_err("%s: init failed....\n", __func__);

	return ret;
}

STATIC void __exit di_dev_exit(void)
{
	di_log_info("%s\n", __func__);
	kfifo_free(&g_di_priv.read_kfifo);
	unregister_mcu_event_notifier(TAG_UDI, CMD_UDI_STATUS_NOTIFY, di_status_notify);
	misc_deregister(&di_miscdev);
}

STATIC int di_probe(struct platform_device *pdev)
{
	if (pdev == NULL) {
		di_log_err("%s: pdev is NULL\n", __func__);
		return -EINVAL;
	}
	di_log_info("%s\n", __func__);

	return di_dev_init(pdev);
}

STATIC int __exit di_remove(struct platform_device *pdev)
{
	if (pdev == NULL) {
		di_log_err("%s: pdev is NULL\n", __func__);
		return -EINVAL;
	}
	di_log_info("%s\n", __func__);
	di_dev_exit();

	return 0;
}

STATIC const struct of_device_id di_match_table[] = {
	{ .compatible = DI_DRV_NAME, },
	{},
};

STATIC struct platform_driver di_platdev = {
	.driver = {
		.name = "iomcu_di",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(di_match_table),
	},
	.probe  = di_probe,
	.remove = di_remove,
};

STATIC int __init di_main_init(void)
{
	return platform_driver_register(&di_platdev);
}

STATIC void __exit di_main_exit(void)
{
	platform_driver_unregister(&di_platdev);
}

late_initcall_sync(di_main_init);
module_exit(di_main_exit);
