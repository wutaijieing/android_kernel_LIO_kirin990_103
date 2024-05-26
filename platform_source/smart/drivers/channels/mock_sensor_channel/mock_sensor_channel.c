/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: Contexthub /dev/sensorhub mock driver.Test only, not enable in USER build.
 * Create: 2018-03-15
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
#include <linux/uaccess.h>
#include <linux/device.h>

#ifdef CONFIG_CONTEXTHUB_STATUS_CFG
#define CONTEXTHUB_STATUS "hisilicon,contexthub_status"
#else
#define CONTEXTHUB_STATUS  "huawei,sensorhub_status"
#endif

static struct semaphore mock_sensor_sem;

static int mock_shb_disabled(void)
{
	int len = 0;
	struct device_node *sh_node = NULL;
	const char *sh_status = NULL;
	static int ret;
	static int once;
	unsigned int reg_pair[2]; /* 0: reg addr, 1: reg value */
	void __iomem *addr = NULL;

	if (once)
		return ret;

	sh_node = of_find_compatible_node(NULL, NULL, CONTEXTHUB_STATUS);
	if (!sh_node) {
		pr_err("%s, can not find node  sensorhub_status n", __func__);
		ret = -1;
		goto finish;
	}

	sh_status = of_get_property(sh_node, "status", &len);
	if (!sh_status) {
		pr_err("%s, can't find property status\n", __func__);
		ret = -1;
		goto finish;
	}

	if (!strstr(sh_status, "ok")) {
		pr_info("%s, sensorhub disabled!\n", __func__);
		ret = -1;
		goto finish;
	}

	sh_node = of_find_compatible_node(NULL, NULL, "hisilicon,iomcu_logic");
	if (sh_node == NULL)
		goto available;

	if (!of_device_is_available(sh_node))
		goto available;

	ret = of_property_read_u32_array(sh_node, "logic_available_pair", reg_pair, 2);
	if (ret != 0)
		goto available;

	addr = ioremap(reg_pair[0], sizeof(unsigned int));
	if (addr == NULL)
		goto available;

	if ((readl(addr) & (1 << reg_pair[1])) != (1 << reg_pair[1])) {
		pr_warn("%s, iomcu logic not available!\n", __func__);
		ret = -1;
		goto finish;
	}

available:
	pr_info("%s, sensorhub enabled!\n", __func__);
	ret = 0;

finish:
	once = 1;
	if (addr != NULL)
		iounmap(addr);
	return ret;
}


/*******************************************************************************************
Function:       shb_read
Description:    定义/dev/sensorhub节点的读函数，从kernel的事件缓冲区中读数据
Data Accessed:  无
Data Updated:   无
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         无
Return:         实际读取数据的长度
*******************************************************************************************/
static ssize_t mock_shb_read(struct file *file, char __user *buf, size_t count,
				loff_t *pos)
{
	pr_info("%s in\n", __func__);
	// blocking at read, or user thread will read in dead loop when no data returned
	(void)down_interruptible(&mock_sensor_sem);
	pr_info("%s out\n", __func__);
	return 0;
}

static ssize_t mock_shb_write(struct file *file, const char __user *data,
						size_t len, loff_t *ppos)
{
	pr_info("%s need to do...\n", __func__);
	return len;
}

/*******************************************************************************************
Function:       shb_ioctl
Description:    定义/dev/sensorhub节点的ioctl函数，主要用于设置传感器命令和获取MCU是否存在
Data Accessed:  无
Data Updated:   无
Input:          struct file *file, unsigned int cmd, unsigned long arg，cmd是命令码，arg是命令跟的参数
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static long mock_shb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	pr_info("%s ok\n", __func__);
	return -EFAULT;
}

/*******************************************************************************************
Function:       shb_open
Description:    定义/dev/sensorhub节点的open函数，暂未实现实质功能
Data Accessed:  无
Data Updated:   无
Input:          struct inode *inode, struct file *file
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static int mock_shb_open(struct inode *inode, struct file *file)
{
	pr_info("%s ok\n", __func__);
	return 0;
}

/*******************************************************************************************
Function:       shb_release
Description:    定义/dev/sensorhub节点的release函数，暂未实现实质功能
Data Accessed:  无
Data Updated:   无
Input:          struct inode *inode, struct file *file
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static int mock_shb_release(struct inode *inode, struct file *file)
{
	pr_info("%s ok\n", __func__);
	return 0;
}

static const struct file_operations mock_shb_fops = {
	.owner      =   THIS_MODULE,
	.llseek     =   no_llseek,
	.read = mock_shb_read,
	.write      =   mock_shb_write,
	.unlocked_ioctl =   mock_shb_ioctl,
	#ifdef CONFIG_COMPAT
	.compat_ioctl =   mock_shb_ioctl,
	#endif
	.open       =   mock_shb_open,
	.release    =   mock_shb_release,
};

static struct miscdevice mock_senorhub_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "sensorhub",
	.fops =     &mock_shb_fops,
};

/*******************************************************************************************
Function:       mock_sensorhub_init
Description:    sensorhub关闭时，注册msic设备，避免上层因为没有设备节点而反复重启
Data Accessed:  无
Data Updated:   无
Input:          无
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static int __init mock_sensorhub_init(void)
{
	int ret;

	if (!mock_shb_disabled())
		return -EINVAL;

	sema_init(&mock_sensor_sem, 0);

	ret = misc_register(&mock_senorhub_miscdev);
	if (ret != 0) {
		pr_err( "cannot register miscdev err=%d\n", ret);
		return ret;
	}

	pr_info("%s ok\n", __func__);
	return 0;
}

/*******************************************************************************************
Function:       mock_sensorhub_exit
Description:    注销msic设备
Data Accessed:  无
Data Updated:   无
Input:          无
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static void __exit mock_sensorhub_exit(void)
{
	if (!mock_shb_disabled())
		return;

	misc_deregister(&mock_senorhub_miscdev);
	pr_info("exit %s\n", __func__);
}

late_initcall_sync(mock_sensorhub_init);
module_exit(mock_sensorhub_exit);

MODULE_DESCRIPTION("Mock SensorHub driver");
MODULE_LICENSE("GPL");
