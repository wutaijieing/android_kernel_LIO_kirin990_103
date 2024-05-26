/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: Contexthub /dev/sensorhub mock driver.Test only, not enable in USER build.
 * Create: 2020-09-18
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>
#include <linux/completion.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <securec.h>
#include <net/genetlink.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "inputhub_api/inputhub_api.h"
#include "inputhub_wrapper/inputhub_wrapper.h"
#include "shmem/shmem.h"
#include "common/common.h"


#define BLE_GENL_NAME                   "TASKBLE"
#define TASKBLE_GENL_VERSION            0x01
enum {
	BLE_GENL_ATTR_UNSPEC,
	BLE_GENL_ATTR_EVENT,
	__BLE_GENL_ATTR_MAX,
};
#define BLE_GENL_ATTR_MAX (__BLE_GENL_ATTR_MAX - 1)
#define BLE_MAX_BLE_CONFIG_SIZE 500

#define BLE_IOCTL_SET_CONFIG         (0x454C0000 + 0x71)
#define BLE_IOCTL_GET_MESSAGE        (0x454C0000 + 0x72)
#define BLE_IOCTL_GET_CAPABILITY     (0x454C0000 + 0x73)
#define BLE_IOCTL_GET_DEVICE_LIST    (0x454C0000 + 0x74)
#define BLE_IOCTL_OPEN_BT            (0x454C0000 + 0x75)
#define BLE_IOCTL_CLOSE_BT           (0x454C0000 + 0x76)
#define BLE_IOCTL_COMMON_SETPID      (0x464C0000 + 0xFFF4) //0x464C0000 for fusd
#define BLE_IOMCU_SHMEM_TAG          (0xABAB)

enum {
	BLE_SHMEM_CMD_ADV_CONFIG = 0x1,
};

enum {
	BLE_IOMCU_RESET,
	BLE_AP_WAKE,
	BLE_RESET_MAX,
};

enum {
	COMM_GENL_CMD_UNSPEC = 0,
	BLE_GENL_CMD_IOMCU_RESET,
	BLE_GENL_CMD_GET_DEVICE,
	BLE_GENL_CMD_GET_MESSAGE,
	BLE_GENL_CMD_GET_WAKE_MESSAGE,
	BLE_GENL_CMD_MAX,
};

enum {
	SUB_CMD_BLE_GET_DEVICE_LIST_REQ,
	SUB_CMD_BLE_GET_DEVICE_LIST_REPORT_REQ,
	SUB_CMD_BLE_GET_ABILITY_REQ,
	SUB_CMD_BLE_GET_ABILITY_REPORT_REQ,
	SUB_CMD_BLE_GET_BROADCAST_REQ,
	SUB_CMD_BLE_GET_BROADCAST_REPORT_REQ,
	SUB_CMD_BLE_SET_BLE_ADV_CONFIG,
	SUB_CMD_BLE_WEAKUP_AP,
};

/* ap -> sensorhub */
struct hal_config_t {
	unsigned int  length;
	void *buf;
} __packed;

struct ble_shmem_hdr {
	unsigned short int tag; /*0xCACA*/
	unsigned short int cmd;
	unsigned int transid;
	unsigned int data_len;
	char data[0];
} __packed;

struct ble_device_t {
	struct mutex lock;
	struct mutex recovery_lock;
	int ref_cnt;
	int sh_recover_flag;
	int netlinkid;
};

struct ble_device_t  g_ble_dev;
static unsigned int g_ble_shmem_transid;


/*lint -e785 */
static struct genl_family ble_genl_family = {//GENL_ID_GENERATE,BLE_GENL_NAME,TASKBLE_GENL_VERSION,BLE_GENL_ATTR_MAX
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	.id = GENL_ID_GENERATE,
#endif
	.name = BLE_GENL_NAME,
	.version = TASKBLE_GENL_VERSION,
	.maxattr = BLE_GENL_ATTR_MAX,
};
/*lint +e785 */

static int ble_generate_netlink_packet(const char *buf,
	unsigned int count, unsigned char cmd_type)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	void *msg_header = NULL;
	char *data = NULL;
	int result;
	static unsigned int ble_event_seqnum;

	skb = genlmsg_new((size_t)count, GFP_ATOMIC);
	if (skb == NULL)
		return -ENOMEM;

	msg_header = genlmsg_put(skb, 0, ble_event_seqnum++,
	&ble_genl_family, 0, cmd_type);
	if (msg_header == NULL) {
		nlmsg_free(skb);
		return -ENOMEM;
	}

	data = nla_reserve_nohdr(skb, (int)count);
	if (data == NULL) {
		nlmsg_free(skb);
		return -EINVAL;
	}

	if (buf != NULL && count) {
		result = memcpy_s(data, (size_t)count, buf, (size_t)count);
		if (result != EOK) {
			nlmsg_free(skb);
			pr_err("[%s]memset_s fail[%d].\n", __func__, result);
			return result;
		}
	}

	nlh = (struct nlmsghdr *)((unsigned char *)msg_header - GENL_HDRLEN - NLMSG_HDRLEN);
	nlh->nlmsg_len = count + GENL_HDRLEN + NLMSG_HDRLEN;

	pr_info("[%s]: cmd %d, len %d\n", __func__,  cmd_type, nlh->nlmsg_len);

	result = genlmsg_unicast(&init_net, skb, g_ble_dev.netlinkid);
	if (result != 0)
		pr_err("[%s]: ble Failed to send netlink event:%d.\n", __func__, result);

	return result;
}

static void  ble_service_recovery(void)
{
	unsigned int response = BLE_IOMCU_RESET;

	pr_info("[%s]: notify HAL sensorhub recovery.\n", __func__);
	mutex_lock(&g_ble_dev.recovery_lock);
	ble_generate_netlink_packet((char *)&response, (unsigned int)sizeof(unsigned int), BLE_GENL_CMD_IOMCU_RESET);
	g_ble_dev.sh_recover_flag = 1;
	mutex_unlock(&g_ble_dev.recovery_lock);
}

static int ble_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	int ret = 0;

	switch (action) {
	case IOM3_RECOVERY_3RD_DOING:
		pr_info("[%s]: sensorhub recovery is doing.\n", __func__);
		ble_service_recovery();
		break;
	case IOM3_RECOVERY_IDLE:
		pr_info("[%s]: sensorhub recovery is done.\n", __func__);
		mutex_lock(&g_ble_dev.recovery_lock);
		g_ble_dev.sh_recover_flag = 0;
		mutex_unlock(&g_ble_dev.recovery_lock);
		break;
	default:
		pr_err("[%s]: register_iom3_recovery_notifier err.\n", __func__);
		break;
	}
	return ret;
}

/*lint -e785*/
static struct notifier_block sensor_reboot_notify = {
	.notifier_call = ble_notifier,
	.priority = -1,
};
/*lint +e785*/

static int get_data_from_mcu(const struct pkt_header *head)
{
	unsigned int len;
	unsigned int response = BLE_AP_WAKE;
	char *data = NULL;
	int cmd;
	int status;

	if (head == NULL) {
		pr_err("[%s]: head is err.\n", __func__);
		return -EINVAL;
	}

	if (head->length < sizeof(unsigned int)) {
		pr_err("[%s]: length is too short.\n", __func__);
		return -EINVAL;
	}
	len = head->length - sizeof(unsigned int);
	data = (char *)((pkt_subcmd_req_t *)head + 1);

	cmd = ((pkt_subcmd_req_t *)head)->subcmd;

	pr_info("[%s]: cmd %d, len %d.\n", __func__, head->cmd, len);

	mutex_lock(&g_ble_dev.lock);
	pr_info("[%s]: subcmd %d.\n", __func__, ((pkt_subcmd_req_t *)head)->subcmd);
	switch (cmd) {
	case SUB_CMD_BLE_GET_DEVICE_LIST_REPORT_REQ:
		pr_info("[%s]: get device_list.\n", __func__);
		status = ble_generate_netlink_packet(data, len, BLE_GENL_CMD_GET_DEVICE);
		break;
	case SUB_CMD_BLE_GET_BROADCAST_REPORT_REQ:
		pr_info("[%s]: get broadcast.\n", __func__);
		status = ble_generate_netlink_packet(data, len, BLE_GENL_CMD_GET_MESSAGE);
		break;
	case SUB_CMD_BLE_WEAKUP_AP:
		pr_info("[%s]: sensorhub wake up AP.\n", __func__);
		status = ble_generate_netlink_packet((char *)&response, (unsigned int)sizeof(unsigned int), BLE_GENL_CMD_GET_WAKE_MESSAGE);
		break;
	default:
		pr_err("[%s]: uncorrect subcmd 0x%x.\n", __func__, cmd);
		mutex_unlock(&g_ble_dev.lock);
		return -EINVAL;
	}
	pr_info("[%s]:status:%d.\n", __func__, status);
	mutex_unlock(&g_ble_dev.lock);
	return 0;
}

static int ble_shmem_send_async(unsigned short int cmd, const void __user *buf,
		unsigned int length)
{
	struct ble_shmem_hdr *iomcu_data = NULL;
	unsigned int iomcu_len;
	int ret;

	if (buf == NULL || length == 0) {
		pr_err("[%s]: user buf is null or length:%u fail.\n",
		__func__, length);
		return -ENOMEM;
	}

	if (length > shmem_get_capacity()) {
		pr_err("[%s]: length:%u too large, capacity:%u.\n",
		__func__, length, shmem_get_capacity());
		return -ENOMEM;
	}

	iomcu_len = sizeof(struct ble_shmem_hdr) + length;
	iomcu_data = kzalloc((size_t)iomcu_len, GFP_KERNEL); //lint !e838
	if (IS_ERR_OR_NULL(iomcu_data)) {
		pr_err("[%s]: kmalloc fail.\n", __func__);
		kfree(iomcu_data);
		return -ENOMEM;
	}

	ret = (int)copy_from_user(iomcu_data->data, (const void __user *)buf,
		(unsigned long)length);
	if (ret) {
		pr_err("[%s]: copy_from_user length is %u ret is %d error.\n", __func__,
			length, ret);
		ret = -EFAULT;
		goto OUT;
	}

	iomcu_data->tag = BLE_IOMCU_SHMEM_TAG;
	iomcu_data->cmd = cmd;
	iomcu_data->transid = ++g_ble_shmem_transid;
	iomcu_data->data_len = length;

	pr_info("[%s]: cmd 0x%x, transid %u, iomcu_len %u.\n", __func__, cmd, iomcu_data->transid, iomcu_len);

	if (iomcu_len > INPUTHUB_WRAPPER_MAX_LEN) {
		pr_info("[%s]: when iomcu len > MAX_LEN, shemm_send.\n", __func__);
		ret = shmem_send(TAG_BT, iomcu_data, (unsigned int)iomcu_len);
	} else {
		pr_info("[%s]: when iomcu len <= MAX_LEN, inputhub.\n", __func__);
		ret = inputhub_wrapper_pack_and_send_cmd(TAG_BT, CMD_CMN_CONFIG_REQ,
			SUB_CMD_BLE_SET_BLE_ADV_CONFIG, (const char *)iomcu_data, iomcu_len, NULL);
	}
	if (ret) {
		pr_err("[%s]: send to sensorhub error.\n", __func__);
		ret = -EFAULT;
		goto OUT;
	}
	if (!IS_ERR_OR_NULL(iomcu_data))
		kfree(iomcu_data);

	return ret;
OUT:
	if (!IS_ERR_OR_NULL(iomcu_data))
		kfree(iomcu_data);
	return ret;
}

static int ble_shmem(struct hal_config_t *config, unsigned short int shmem_cmd)
{
	int ret;

	ret = ble_shmem_send_async(shmem_cmd, (const void __user *)config->buf, (const unsigned long)config->length);
	if (ret) {
		pr_err("[%s]: ble_shmem_send_async send fail.\n", __func__);
		return ret;
	}

	return ret;
}

static int ble_ioctl_get(unsigned long arg, unsigned int subcmd, struct read_info *rd)
{
	int ret;

	ret = inputhub_wrapper_pack_and_send_cmd(TAG_BT, CMD_CMN_CONFIG_REQ, subcmd,
		NULL, (size_t)0, rd);
	pr_info("[%s]: subcmd %d, ret %d.\n", __func__, subcmd, ret);

	return ret;
}

static int ble_ioctl_send(unsigned int cmd, unsigned long arg)
{
	struct hal_config_t config;
	int ret;

	if (arg == 0) {
		pr_err("[%s]: invalid param.\n", __func__);
		return -EFAULT;
	}
	if ((void *)((uintptr_t)arg) == NULL) {
		pr_err("[%s]: arg is null, error!\n", __func__);
		return -EINVAL;
	}
	if (copy_from_user(&config, (void *)((uintptr_t)arg), sizeof(struct hal_config_t))) {
		pr_err("[%s]: copy_from_user hal_config error.\n", __func__);
		return -EFAULT;
	}
	if (config.length > BLE_MAX_BLE_CONFIG_SIZE || config.length == 0) {
		pr_err("[%s]: config size overflow %u.\n", __func__, config.length);
		return -EFAULT;
	}
	ret = ble_shmem(&config, cmd);
	return ret;
}

static int ble_ioctl_set_pid(unsigned long arg)
{
	int data;

	if ((void *)((uintptr_t)arg) == NULL) {
		pr_err("[%s]: arg is null, error!\n", __func__);
		return -EINVAL;
	}
	if (copy_from_user(&data, (void *)((uintptr_t)arg), sizeof(int))) {
		pr_err("[%s]: ble_ioctl copy_from_user error.\n", __func__);
		return -EFAULT;
	}
	pr_info("[%s]: get_pid:%d.\n", __func__, data);
	g_ble_dev.netlinkid = data;
	return 0;
}

static long ble_channel_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	if (file == NULL) {
		pr_err("[%s]: parameter error.\n", __func__);
		return -EINVAL;
	}

	pr_info("[%s]: cmd[0x%x].\n", __func__, cmd);

	mutex_lock(&g_ble_dev.lock);

	if (g_ble_dev.sh_recover_flag == 1)
		pr_info("[%s]: sensorhub in recover mode.\n", __func__);


	switch (cmd) {
	case BLE_IOCTL_OPEN_BT:
		pr_info("[%s]: open sensorhub bt.\n", __func__);
		ret = inputhub_wrapper_pack_and_send_cmd(TAG_BT, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, NULL);
		break;
	case BLE_IOCTL_CLOSE_BT:
		pr_info("[%s]: close sensorhub bt.\n", __func__);
		ret = inputhub_wrapper_pack_and_send_cmd(TAG_BT, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0, NULL);
		break;
	case BLE_IOCTL_SET_CONFIG:
		pr_info("[%s]: enter into ble_ioctl_send.\n", __func__);
		ret = (long)ble_ioctl_send(SUB_CMD_BLE_SET_BLE_ADV_CONFIG, arg);
		break;
	case BLE_IOCTL_GET_MESSAGE:
		pr_info("[%s]: enter into ble_ioctl_get.\n", __func__);
		ret = (long)ble_ioctl_get(arg, SUB_CMD_BLE_GET_BROADCAST_REQ, NULL);
		break;
	case BLE_IOCTL_GET_DEVICE_LIST:
		pr_info("[%s]: enter into ble_ioctl_get.\n", __func__);
		ret = (long)ble_ioctl_get(arg, SUB_CMD_BLE_GET_DEVICE_LIST_REQ, NULL);
		break;
	case BLE_IOCTL_COMMON_SETPID:
		pr_info("[%s]: enter into ble_ioctl_set_pid.\n", __func__);
		ret = (long)ble_ioctl_set_pid(arg);
		break;
	default:
		mutex_unlock(&g_ble_dev.lock);
		pr_err("[%s]: unknown cmd : %d.\n", __func__, cmd);
		return -EINVAL;
	}
	pr_info("[%s]: ret is %ld.\n", __func__, ret);
	mutex_unlock(&g_ble_dev.lock);
	return ret;
}

static int ble_channel_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	if (inode == NULL || file == NULL) {
		pr_err("[%s]: invalid param.\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_ble_dev.lock);
	if (g_ble_dev.ref_cnt != 0) {
		pr_err("[%s]: duplicate open.\n", __func__);
		mutex_unlock(&g_ble_dev.lock);
		return -EFAULT;
	}
	file->private_data = &g_ble_dev;
	g_ble_dev.sh_recover_flag = 0;
	g_ble_dev.ref_cnt++;
	mutex_unlock(&g_ble_dev.lock);

	return ret;
}

static int ble_channel_release(struct inode *inode, struct file *file)
{

	if (inode == NULL || file == NULL) {
		pr_err("[%s]: invalid param.\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_ble_dev.lock);
	if (g_ble_dev.ref_cnt == 0) {
		pr_err("[%s]: ref cnt is 0.\n", __func__);
		mutex_unlock(&g_ble_dev.lock);
		return -EFAULT;
	}

	g_ble_dev.ref_cnt--;
	if (g_ble_dev.ref_cnt == 0)
		pr_info("[%s]: got close resp.\n", __func__);
	mutex_unlock(&g_ble_dev.lock);

	return 0;
}

/*lint -e785*/
static const struct file_operations ble_channel_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.unlocked_ioctl = ble_channel_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ble_channel_ioctl,
#endif
	.open = ble_channel_open,
	.release = ble_channel_release,
};
/*lint +e785*/

/*lint -e785*/
static struct miscdevice ble_channel_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ble_channel",
	.fops = &ble_channel_fops,
};
/*lint +e785*/

static int __init ble_channel_register(void)
{
	int ret;

	if (get_contexthub_dts_status() != 0) {
		pr_err("[%s] contexthub disabled!\n", __func__);
		return -EINVAL;
	}

	(void)memset_s((void *)&g_ble_dev, sizeof(g_ble_dev), 0, sizeof(g_ble_dev));
	ret = genl_register_family(&ble_genl_family);
	if (ret != 0) {
		pr_err("[%s]: genl_register fails!\n", __func__);
		return ret;
	}

	ret = inputhub_wrapper_register_event_notifier(TAG_BT,
		CMD_CMN_CONFIG_REQ, get_data_from_mcu);
	if (ret != 0) {
		pr_err("[%s]: register data report process failed!\n", __func__);
		goto register_failed;
	}

	mutex_init(&g_ble_dev.lock);
	mutex_init(&g_ble_dev.recovery_lock);

	ret = misc_register(&ble_channel_miscdev);
	if (ret != 0) {
		pr_err("[%s]: cannot register ble channel err=%d.\n", __func__, ret);
		goto err;
	}
	(void)register_iom3_recovery_notifier(&sensor_reboot_notify);
	pr_info("[%s]: success.\n", __func__);
	return 0;
err:
	(void)inputhub_wrapper_unregister_event_notifier(TAG_BT,
		CMD_CMN_CONFIG_REQ, get_data_from_mcu);
register_failed:
	genl_unregister_family(&ble_genl_family);
	return ret;
}

static void ble_channel_unregister(void)
{
	(void)inputhub_wrapper_unregister_event_notifier(TAG_BT, CMD_CMN_CONFIG_REQ, get_data_from_mcu);
	genl_unregister_family(&ble_genl_family);
	misc_deregister(&ble_channel_miscdev);

}

static int generic_ble_probe(struct platform_device *pdev)
{
	return ble_channel_register();
}

static int generic_ble_remove(struct platform_device *pdev)
{
	ble_channel_unregister();
	return 0;
}
/*lint -e785*/
static const struct of_device_id generic_ble[] = {
	{.compatible = "hisilicon,contexthub_ble"},
	{},
};
/*lint +e785*/
MODULE_DEVICE_TABLE(of, generic_ble);

/*lint -e785*/
static struct platform_driver generic_ble_platdrv = {
	.driver = {
		.name = "ble_channel",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(generic_ble),
	},
	.probe = generic_ble_probe,
	.remove  = generic_ble_remove,
};
/*lint +e785*/

static int __init ble_init(void)
{
	return platform_driver_register(&generic_ble_platdrv);
}

static void __exit ble_exit(void)
{
	platform_driver_unregister(&generic_ble_platdrv);
}

late_initcall_sync(ble_init);
module_exit(ble_exit);
