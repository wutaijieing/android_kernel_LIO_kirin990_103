/*
 * ap_clk_correct.c - HW ap clk correct in kernel, it is used for pass through correct
 * data between AtCmdServer and pmu/sd card.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/pm_wakeup.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/thermal.h>
#include <platform_include/basicplatform/linux/mfd/pmic_dcxo.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <linux/of.h>
#include <asm/memory.h>
#include <asm/types.h>
#include "dw_mmc_clk_for_calibration.h"

/*宏定义*/
/*lint -e528 -e753 */
#define DTS_COMP_AP_CLK_CORRECT_NAME "hisilicon,ap_clk_correct"
#define AP_CLK_CORRECT_SET_CMD  _IO('A',  0x10)
#define AP_CLK_CORRECT_GET_CMD  _IO('A',  0x20)
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif
#define AP_CLK_CORRECT_DCXO_SET                 0x00000001
#define AP_CLK_CORRECT_DCXO_GET                 0x00000002
#define AP_CLK_CORRECT_DCXO_CLK_SWITCH_ON       0x00000003
#define AP_CLK_CORRECT_DCXO_CLK_SWITCH_OFF      0x00000004
#define AP_CLK_CORRECT_DCXO_CLK_SWITCH_GET      0x00000005
#define AP_CLK_CORRECT_DCXO_TEMPERATURE_GET     0x00000006
#define AP_CLK_CORRECT_DCXO_NV_SET              0x00000007
#define AP_CLK_CORRECT_DCXO_NV_GET              0x00000008
#define HKADC_XOADC "dcxo0"
#define AP_CLK_CORRECT_SD_CLK_OPEN              1
#define AP_CLK_CORRECT_SD_CLK_CLOSE             0

extern void output_clk_for_hifi_calibration(int input);

/*结构体定义*/
struct dcxo_c1c2_val {
	uint16_t dcxo_ctrim;
	uint16_t dcxo_c2_fix;
};
struct ap_clk_correct_msg_head {
	uint16_t msg_id;
	uint16_t msg_len;
};

/*内部全局变量*/
static struct mutex ap_clk_correct_ioctl_mutex;

static int hkadc_get_xoadc_temp(void)
{
	int  ret;
	struct thermal_zone_device *tz = NULL;
	int temp = 0;

	tz = thermal_zone_get_zone_by_name(HKADC_XOADC);
	ret = thermal_zone_get_temp(tz, &temp);
	if (ret)
		temp = 20000; // 20C
	printk("the xoadc's temp is %d\n", temp);
	return temp;
}

/*函数实现*/
static int ap_clk_correct_get_proc(uintptr_t arg)
{
	long ret = 0;
	struct ap_clk_correct_msg_head msg_head = {0};
	int xo_hkadc;
	struct dcxo_c1c2_val c1c2_value = {0};

	ret = copy_from_user(&msg_head, (void*)arg, sizeof(msg_head));
	if (ret) {
		printk("copy_from_user fail.\n");
		return ret;
	}
	switch (msg_head.msg_id) {
		case AP_CLK_CORRECT_DCXO_NV_GET:
			ret = pmu_dcxo_get(&c1c2_value.dcxo_ctrim, &c1c2_value.dcxo_c2_fix);
			if (ret) {
				printk("dcxo get c1c2 fail ret %lu\n", ret);
				break;
			}
			ret = copy_to_user((void*)((uint8_t*)arg + sizeof(struct ap_clk_correct_msg_head)), &c1c2_value, sizeof(c1c2_value));
			if (ret) {
				printk("copy to user fail ret %lu\n", ret);
			}
			break;
		case AP_CLK_CORRECT_DCXO_GET:
				ret = pmu_dcxo_reg_get(&c1c2_value.dcxo_ctrim, &c1c2_value.dcxo_c2_fix);
				if (ret) {
					printk("dcxo get c1c2 fail ret %lu\n", ret);
					break;
				}
				ret = copy_to_user((void*)((uint8_t*)arg + sizeof(struct ap_clk_correct_msg_head)), &c1c2_value, sizeof(c1c2_value));
				if (ret) {
					printk("copy to user fail ret %lu\n", ret);
				}
				break;
		case AP_CLK_CORRECT_DCXO_TEMPERATURE_GET:
			xo_hkadc = hkadc_get_xoadc_temp();
			ret = copy_to_user((void*)((uint8_t*)arg + sizeof(struct ap_clk_correct_msg_head)), &xo_hkadc, sizeof(xo_hkadc));
			if (ret) {
				printk("copy to user fail ret %lu\n", ret);
			}
			break;
		default:
			printk("get proc: Invalid msgid = 0x%x\n", (int32_t)msg_head.msg_id);
			ret = -EINVAL;
			break;
		}
	return ret;
}
static int ap_clk_correct_set_proc(uintptr_t arg)
{
	long ret = 0;
	struct ap_clk_correct_msg_head msg_head = {0};
	struct dcxo_c1c2_val c1c2_value = {0};

	ret = copy_from_user(&msg_head, (void*)arg, sizeof(msg_head));
	if (ret) {
		printk("copy_from_user fail.\n");
		return ret;
	}
	switch (msg_head.msg_id) {
		case AP_CLK_CORRECT_DCXO_CLK_SWITCH_ON:
			output_clk_for_hifi_calibration(AP_CLK_CORRECT_SD_CLK_OPEN);
			break;
		case AP_CLK_CORRECT_DCXO_CLK_SWITCH_OFF:
			output_clk_for_hifi_calibration(AP_CLK_CORRECT_SD_CLK_CLOSE);
			break;
		case AP_CLK_CORRECT_DCXO_SET:
			ret = copy_from_user(&c1c2_value, (void*)((uint8_t*)arg + sizeof(struct ap_clk_correct_msg_head)), sizeof(c1c2_value));
			if (ret) {
				printk("copy from user fail ret %lu\n", ret);
				break;
			}
			ret = pmu_dcxo_reg_set(c1c2_value.dcxo_ctrim, c1c2_value.dcxo_c2_fix);
			if (ret) {
				printk("dcxo set c1c2 fail ret %lu\n", ret);
			}
			break;
		case AP_CLK_CORRECT_DCXO_NV_SET:
			ret = copy_from_user(&c1c2_value, (void*)((uint8_t*)arg + sizeof(struct ap_clk_correct_msg_head)), sizeof(c1c2_value));
			if (ret) {
				printk("copy from user fail ret %lu\n", ret);
				break;
			}
			ret = pmu_dcxo_set(c1c2_value.dcxo_ctrim, c1c2_value.dcxo_c2_fix);
			if (ret) {
				printk("dcxo set c1c2 fail ret %lu\n", ret);
			}
			break;
		default:
			printk("set proc: Invalid msgid = 0x%x\n", (int32_t)msg_head.msg_id);
			ret = -EINVAL;
			break;
		}
	return ret;
}

static long ap_clk_correct_ioctl(struct file *fd, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	void __user *data32 = (void __user *)(uintptr_t)arg;

	UNUSED_PARAMETER(fd);

	mutex_lock(&ap_clk_correct_ioctl_mutex);
	switch (cmd) {
		case AP_CLK_CORRECT_SET_CMD:
			ret = ap_clk_correct_set_proc((uintptr_t)data32);
			break;
		case AP_CLK_CORRECT_GET_CMD:
			ret = ap_clk_correct_get_proc((uintptr_t)data32);
			break;
		default:
			printk("ioctl: Invalid CMD = 0x%x\n", (int32_t)cmd);
			ret = -EINVAL;
			break;
	}
	mutex_unlock(&ap_clk_correct_ioctl_mutex);

	return ret;
}

static int ap_clk_correct_open(struct inode *finode, struct file *fd)
{
	printk("ap clk correct open\n");
	UNUSED_PARAMETER(finode);
	UNUSED_PARAMETER(fd);

	return 0;
}

static int ap_clk_correct_close(struct inode *node, struct file *filp)
{
	printk("ap clk correct close");
	UNUSED_PARAMETER(node);
	UNUSED_PARAMETER(filp);

	return 0;
}

static const struct file_operations ap_clk_correct_misc_fops = {
	.owner = THIS_MODULE,/*lint !e64*/
	.open = ap_clk_correct_open,
	.release = ap_clk_correct_close,
	.unlocked_ioctl = ap_clk_correct_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = ap_clk_correct_ioctl,
#endif
};/*lint !e785*/

static struct miscdevice ap_clk_correct_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ap_clk_correct",
	.fops = &ap_clk_correct_misc_fops,
};/*lint !e785*/

static int ap_clk_correct_probe(struct platform_device *pdev)
{
	int32_t ret;

	UNUSED_PARAMETER(pdev);
	printk("ap clk correct prob,pdev name[%s]\n", pdev->name);

	mutex_init(&ap_clk_correct_ioctl_mutex);
	ret = misc_register(&ap_clk_correct_misc_device);
	if (ret) {
		printk("ap clk correct misc register fail\n");
		return ret;
	}
	return ret;
}

static int ap_clk_correct_remove(struct platform_device *pdev)
{
	printk("ap clk correct remove,pdev name[%s]\n", pdev->name);

	UNUSED_PARAMETER(pdev);

	mutex_destroy(&ap_clk_correct_ioctl_mutex);
	misc_deregister(&ap_clk_correct_misc_device);

	return 0;
}


static const struct of_device_id ap_clk_correct_match_table[] = {
	{
		.compatible = DTS_COMP_AP_CLK_CORRECT_NAME,
		.data = NULL,
	},
	{}/*lint !e785*/
};

static struct platform_driver ap_clk_correct_driver = {
	.driver = {
		.name  = "ap clk correct",
		.owner = THIS_MODULE,/*lint !e64*/
		.of_match_table = of_match_ptr(ap_clk_correct_match_table),
	},/*lint !e785*/
	.probe = ap_clk_correct_probe,
	.remove = ap_clk_correct_remove,
};/*lint !e785*/

static int __init ap_clk_correct_init( void )
{
	int32_t ret;

	printk("Audio:ap clk correct init\n");

	ret = platform_driver_register(&ap_clk_correct_driver);/*lint !e64*/
	if (ret) {
		printk("ap clk correct driver register fail,ERROR is %d\n", ret);
	}

	return ret;
}

static void __exit ap_clk_correct_exit( void )
{
	platform_driver_unregister(&ap_clk_correct_driver);
}

module_init(ap_clk_correct_init);
module_exit(ap_clk_correct_exit);

MODULE_DESCRIPTION("ap clk correct driver");
MODULE_LICENSE("GPL");

