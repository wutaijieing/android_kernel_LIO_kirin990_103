
#include "igs_tof_dev.h"
#include <linux/completion.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <securec.h>
#include "bbox_diaginfo_id_def.h"
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include "mm_lb.h"
#include "common/common.h"
#include "inputhub_api/inputhub_api.h"
#include "shmem/shmem.h"

#define SWING_TOF_IOCTL_WAIT_TMOUT (5000) // ms
#define SWING_TOF_DATA_CACHE_COUNT (32)

#define SWING_TOF_DEV_MAX_UPLOAD_LEN (0x1000)

struct swing_tof_read_data {
	unsigned int recv_len;
	void *recv_data;
};

struct swing_tof_priv {
	struct device *dev;
	struct completion tof_wait;
	struct completion read_wait;
	struct mutex read_mutex;
	struct mutex tof_mutex;
	struct kfifo read_kfifo;
	int open_cnt;
	int sh_recover_flag;
	struct swing_tof_msg msg;
};

static struct swing_tof_priv g_swing_tof_priv = {0};

static int swing_tof_wait_completion(struct completion *wait, u32 timeout)
{
	if (timeout != 0) {
		if (!wait_for_completion_interruptible_timeout(wait,
				msecs_to_jiffies(timeout))) {
			pr_warn("%s:wait timeout\n", __func__);
			return -ETIMEOUT;
		}
	} else {
		if (wait_for_completion_interruptible(wait)) {
			pr_warn("%s:wait interrupted.\n", __func__);
			return -EFAULT;
		}
	}

	return 0;
}

static int swing_tof_dev_ioctl_timeout_handler(void)
{
	pr_info("%s ,release read wait\n", __func__);
	complete(&g_swing_tof_priv.read_wait);
	return 0;
}

static int swing_tof_dev_ioctl_internal(int sub_cmd, unsigned long arg)
{
	struct swing_tof_msg tof_msg;
	int ret;

	if (arg == 0) {
		pr_err("%s arg NULL\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&tof_msg, (void *)(uintptr_t)arg,
			sizeof(tof_msg))) {
		pr_err("%s copy from user error\n", __func__);
		return -EFAULT;
	}

	if (send_cmd_from_kernel(TAG_SWING_TOF, CMD_CMN_CONFIG_REQ, sub_cmd,
			(char *)&tof_msg, sizeof(struct swing_tof_msg))) {
		pr_err("%s send msg error\n", __func__);
		return -EFAULT;
	}

	ret = swing_tof_wait_completion(&g_swing_tof_priv.tof_wait,
				SWING_TOF_IOCTL_WAIT_TMOUT);
	if (ret) {
		pr_err("%s  fail\n", __func__);
		return ret;
	}

	ret = memcpy_s(&tof_msg, sizeof(struct swing_tof_msg),
			&g_swing_tof_priv.msg, sizeof(struct swing_tof_msg));
	if (ret) {
		pr_err("%s memcpy fail\n", __func__);
		return ret;
	}
	if (copy_to_user((void *)((uintptr_t)arg), &tof_msg,
			sizeof(tof_msg))) {
		pr_err("%s copy to user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static int swing_tof_get_config_sub_cmd(unsigned int cmd)
{
	int sub_cmd;

	switch (cmd) {
	case SWING_TOF_MATCH_ID:
		sub_cmd = SUB_CMD_SWING_TOF_MACTH_ID;
		break;
	case SWING_TOF_ACQUIRE:
		sub_cmd = SUB_CMD_SWING_TOF_ACQUIRE;
		break;
	case SWING_TOF_SET_POWER_ON_OFF_RESULT:
		sub_cmd = SUB_CMD_SWING_TOF_SET_POWER_ON_OFF_RESULT;
		break;
	default:
		sub_cmd = -1;
		break;
	}

	return sub_cmd;
}

static long swing_tof_dev_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int ret;
	int sub_cmd;

	pr_info("%s cmd: [0x%x]\n", __func__, cmd);

	if (g_swing_tof_priv.sh_recover_flag == 1) {
		pr_info("%s sensorhub in recover mode\n", __func__);
		return -EBUSY;
	}

	sub_cmd = swing_tof_get_config_sub_cmd(cmd);
	if (sub_cmd == -1)
		return -EFAULT;

	mutex_lock(&g_swing_tof_priv.tof_mutex);
	reinit_completion(&g_swing_tof_priv.tof_wait);
	ret = swing_tof_dev_ioctl_internal(sub_cmd, arg);
	mutex_unlock(&g_swing_tof_priv.tof_mutex);

	if (ret == -ETIMEOUT)
		swing_tof_dev_ioctl_timeout_handler();

	return ret;
}

static int swing_tof_dev_open(struct inode *inode, struct file *file)
{
	int ret;

	pr_info("enter %s\n", __func__);
	mutex_lock(&g_swing_tof_priv.tof_mutex);

	if (g_swing_tof_priv.open_cnt != 0) {
		pr_warn("%s duplicate open\n", __func__);
		mutex_unlock(&g_swing_tof_priv.tof_mutex);
		return -EFAULT;
	}

	if (g_swing_tof_priv.open_cnt == 0) {
		ret = send_cmd_from_kernel(TAG_SWING_TOF, CMD_CMN_OPEN_REQ,
				0, NULL, (size_t)0);
		if (ret != 0)
			pr_err("%s: send cmd failed\n", __func__);
	}
	file->private_data = &g_swing_tof_priv;
	g_swing_tof_priv.open_cnt++;
	g_swing_tof_priv.sh_recover_flag = 0;

	mutex_unlock(&g_swing_tof_priv.tof_mutex);

	return 0;
}

static int swing_tof_dev_flush(struct file *file, fl_owner_t id)
{
	int ret;
	pr_info("enter %s\n", __func__);

	mutex_lock(&g_swing_tof_priv.tof_mutex);

	if (g_swing_tof_priv.open_cnt == 0) {
		pr_err("%s: swing tof cnt is 0\n", __func__);
		mutex_unlock(&g_swing_tof_priv.tof_mutex);
		return -EFAULT;
	}

	g_swing_tof_priv.open_cnt--;

	if (g_swing_tof_priv.open_cnt == 0) {
		ret = send_cmd_from_kernel(TAG_SWING_TOF, CMD_CMN_CLOSE_REQ,
				0, NULL, (size_t)0);
		if (ret != 0)
			pr_err("%s: send cmd failed\n", __func__);
	}
	mutex_unlock(&g_swing_tof_priv.tof_mutex);
	complete(&g_swing_tof_priv.read_wait);

	return 0;
}

static ssize_t swing_tof_dev_read(struct file *file, char __user *buf,
					size_t count, loff_t *pos)
{
	struct swing_tof_read_data read_data = {0};
	u32 error;
	u32 length;
	u32 ret;
	error = (u32)swing_tof_wait_completion(&g_swing_tof_priv.read_wait, 0);
	if (error != 0) {
		error = 0;
		goto READ_ERR;
	}
	mutex_lock(&g_swing_tof_priv.read_mutex);
	if (kfifo_len(&g_swing_tof_priv.read_kfifo) <
			sizeof(struct swing_tof_read_data)) {
		pr_err("%s: read data failed\n", __func__);
		mutex_unlock(&g_swing_tof_priv.read_mutex);
		goto READ_ERR;
	}
	ret = kfifo_out(&g_swing_tof_priv.read_kfifo,
				(unsigned char *)&read_data,
				sizeof(struct swing_tof_read_data));
	if (ret == 0) {
		pr_err("%s: kfifo out failed\n", __func__);
		mutex_unlock(&g_swing_tof_priv.read_mutex);
		goto READ_ERR;
	}
	if (count < read_data.recv_len) {
		length = count;
		pr_err("%s user buffer is too small\n", __func__);
	} else
		length = read_data.recv_len;
	pr_info("%s copy len[0x%x], count[0x%x]\n", __func__,
			read_data.recv_len, (u32)count);
	error = length;
	if ((buf == NULL) || (read_data.recv_data == NULL)){
		pr_err("%s: addr NULL\n", __func__);
		mutex_unlock(&g_swing_tof_priv.read_mutex);
		goto READ_ERR;
	}
	if (copy_to_user(buf, read_data.recv_data, length)) {
		pr_err("%s failed to copy to user\n", __func__);
		error = 0;
	}
	mutex_unlock(&g_swing_tof_priv.read_mutex);
READ_ERR:
	if (read_data.recv_data != NULL) {
		kfree(read_data.recv_data);
		read_data.recv_data = NULL;
		read_data.recv_len = 0;
	}
	return error;
}

static ssize_t swing_tof_dev_write(struct file *file, const char __user *data,
						size_t len, loff_t *ppos)
{
	return 0;
}

static const struct file_operations swing_tof_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.unlocked_ioctl = swing_tof_dev_ioctl,
	.open = swing_tof_dev_open,
	.flush = swing_tof_dev_flush,
	.read = swing_tof_dev_read,
	.write = swing_tof_dev_write,
};

static struct miscdevice swing_tof_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "swing_tof_dev",
	.fops = &swing_tof_dev_fops,
};

static int swing_tof_upload(struct swing_tof_msg *msg, int length)
{
	struct swing_tof_read_data read_data;
	int ret;
	unsigned int count = 0;

	read_data.recv_data = NULL;
	read_data.recv_len = 0;
	mutex_lock(&g_swing_tof_priv.read_mutex);

	if (kfifo_avail(&g_swing_tof_priv.read_kfifo) <
			sizeof(struct swing_tof_read_data)) {
		pr_err("%s read kfifo is full, drop event data\n", __func__);
		goto RET_ERR;
	}

	if (length > SWING_TOF_DEV_MAX_UPLOAD_LEN) {
		pr_err("%s event msg length err\n", __func__);
		goto RET_ERR;
	}

	read_data.recv_len = (unsigned int)length;
	read_data.recv_data = kzalloc(length, GFP_ATOMIC);
	if (read_data.recv_data == NULL) {
		pr_err("%s failed to alloc memory for event\n", __func__);
		goto RET_ERR;
	}

	ret = memcpy_s(read_data.recv_data, read_data.recv_len, msg, length);
	if (ret) {
		pr_err("%s memecpy failed\n", __func__);
		goto RET_ERR;
	}

	count = kfifo_in(&g_swing_tof_priv.read_kfifo,
				(unsigned char *)&read_data,
				sizeof(struct swing_tof_read_data));
	if (count == 0) {
		pr_err("%s kfifo in failed\n", __func__);
		goto RET_ERR;
	}

	mutex_unlock(&g_swing_tof_priv.read_mutex);

	return 0;

RET_ERR:
	ret = -EFAULT;
	if (read_data.recv_data != NULL)
		kfree(read_data.recv_data);
	mutex_unlock(&g_swing_tof_priv.read_mutex);
	return ret;
}

static int swing_tof_get_resp(const struct pkt_header *head)
{
	struct pkt_subcmd_resp *resp = NULL;
	struct swing_tof_msg *msg = NULL;
	int ret = 0;
	resp = (struct pkt_subcmd_resp *)head;
	if (resp == NULL) {
		pr_err("%s resp is null\n", __func__);
		return -EFAULT;
	}
	if (resp->hd.tag != TAG_SWING_TOF) {
		pr_err("%s: invalid tag [0x%x]\n", __func__, resp->hd.tag);
		return -EFAULT;
	}
	msg = (struct swing_tof_msg *)(resp + 1);
	pr_info("%s subcmd: [0x%x]\n", __func__, resp->subcmd);
	switch (resp->subcmd) {
	case SUB_CMD_SWING_TOF_MACTH_ID:
		msg->upload_type = MATCH_ID;
		ret = memcpy_s(&g_swing_tof_priv.msg,
					sizeof(struct swing_tof_msg),
					msg, sizeof(struct swing_tof_msg));
		if (ret != 0) {
			pr_err("%s memcpy fail\n", __func__);
			return ret;
		}
		complete(&g_swing_tof_priv.tof_wait);
		break;
	case SUB_CMD_SWING_TOF_ACQUIRE:
		msg->upload_type = ACUIQRE;
		ret = memcpy_s(&g_swing_tof_priv.msg,
					sizeof(struct swing_tof_msg),
					msg, sizeof(struct swing_tof_msg));
		if (ret != 0) {
			pr_err("%s memcpy fail\n", __func__);
			return ret;
		}
		complete(&g_swing_tof_priv.tof_wait);
		break;
	case SUB_CMD_SWING_TOF_SET_POWER_ON_OFF_RESULT:
		msg->upload_type = SET_POWER_ON_OFF_RESULT;
		ret = memcpy_s(&g_swing_tof_priv.msg,
					sizeof(struct swing_tof_msg),
					msg, sizeof(struct swing_tof_msg));
		if (ret != 0) {
			pr_err("%s memcpy fail\n", __func__);
			return ret;
		}
		complete(&g_swing_tof_priv.tof_wait);
		break;
	case SUB_CMD_SWING_TOF_REQ_POWER_ON_OFF:
		msg->upload_type = REQ_POWER_ON_OFF;
		ret = swing_tof_upload(msg, sizeof(struct swing_tof_msg));
		if (ret != 0) {
			pr_err("%s swint tof upload failed\n", __func__);
			return ret;
		}
		complete(&g_swing_tof_priv.read_wait);
		break;
	default:
		break;
	}
	return ret;
}

static int swing_tof_sensorhub_reset_handler(void)
{
	struct swing_tof_msg msg;
	int ret;

	ret = memset_s(&msg, sizeof(struct swing_tof_msg),
				0, sizeof(struct swing_tof_msg));
	if (ret != 0) {
		pr_err("%s, memset fail\n", __func__);
		return ret;
	}
	msg.upload_type = SWING_TOF_RECOVERY;
	ret = swing_tof_upload(&msg, sizeof(struct swing_tof_msg));
	complete(&g_swing_tof_priv.read_wait);
	complete(&g_swing_tof_priv.tof_wait);

	return ret;
}

static int swing_tof_sensorhub_reset_notifier(struct notifier_block *nb,
					unsigned long action, void *data)
{
	int ret = 0;

	switch (action) {
	case IOM3_RECOVERY_IDLE:
		ret = swing_tof_sensorhub_reset_handler();
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block swing_tof_reboot_notify = {
	.notifier_call = swing_tof_sensorhub_reset_notifier,
	.priority = -1,
};

static int swing_tof_dev_init(struct platform_device *pdev)
{
	int ret;

	if (is_sensorhub_disabled()) {
		pr_err("sensorhub disabled\n");
		return -EFAULT;
	}

	if (g_config_on_ddr->igs_hardware_bypass != 0) {
		pr_err("swing hardware bypass!\n");
		return -ENODEV;
	}

	ret = misc_register(&swing_tof_miscdev);
	if (ret != 0) {
		pr_err("%s cannot register miscdev, err = %d\n", __func__, ret);
		return ret;
	}

	mutex_init(&g_swing_tof_priv.read_mutex);
	mutex_init(&g_swing_tof_priv.tof_mutex);

	init_completion(&g_swing_tof_priv.tof_wait);
	init_completion(&g_swing_tof_priv.read_wait);

	ret = register_mcu_event_notifier(TAG_SWING_TOF,
				CMD_CMN_CONFIG_RESP, swing_tof_get_resp);
	if (ret != 0) {
		pr_err("[%s]: register config resp err. [%d]\n", __func__, ret);
		goto RET_ERR1;
	}

	ret = register_iom3_recovery_notifier(&swing_tof_reboot_notify);
	if (ret) {
		pr_err("%s register iom3 recovery notifier failed\n", __func__);
		goto RET_ERR2;
	}

	ret = kfifo_alloc(&g_swing_tof_priv.read_kfifo,
		sizeof(struct swing_tof_read_data) *
		SWING_TOF_DATA_CACHE_COUNT,
		GFP_KERNEL);
	if (ret) {
		pr_err("%s kfifo alloc failed\n", __func__);
		goto RET_ERR2;
	}

	g_swing_tof_priv.dev = &(pdev->dev);
	pr_info("%s swing tof init success\n", __func__);

	return 0;

RET_ERR2:
	unregister_mcu_event_notifier(TAG_SWING_TOF,
		CMD_CMN_CONFIG_RESP,
		swing_tof_get_resp);
RET_ERR1:
	misc_deregister(&swing_tof_miscdev);

	return ret;
}

static int swing_tof_probe(struct platform_device *pdev)
{
	if (pdev == NULL)
		return -EFAULT;

	return swing_tof_dev_init(pdev);
}

static void swing_tof_dev_exit(void)
{
	kfifo_free(&g_swing_tof_priv.read_kfifo);
	unregister_mcu_event_notifier(TAG_SWING_TOF,
			CMD_CMN_CONFIG_RESP, swing_tof_get_resp);
	misc_deregister(&swing_tof_miscdev);
}

static int swing_tof_remove(struct platform_device *pdev)
{
	swing_tof_dev_exit();
	return 0;
}

#define SWING_TOF_DRV_NAME "hisilicon,swing-tof-dev"
static const struct of_device_id swing_tof_match_table[] = {
	{
		.compatible = SWING_TOF_DRV_NAME,
	},
	{},
};

static struct platform_driver swing_tof_platdev = {
	.driver = {
		.name = "swing_tof_dev",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(swing_tof_match_table),
	},
	.probe = swing_tof_probe,
	.remove = swing_tof_remove,
};

static int __init swing_tof_init(void)
{
	return platform_driver_register(&swing_tof_platdev);
}

static void __exit swing_tof_exit(void)
{
	platform_driver_unregister(&swing_tof_platdev);
}

late_initcall_sync(swing_tof_init);
module_exit(swing_tof_exit);
