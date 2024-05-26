/*
 * drv_venc_intf.c
 *
 * This is for venc drv.
 *
 * Copyright (c) 2009-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/mm_iommu.h>
#include <linux/dma-mapping.h>
#include <linux/dma-iommu.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include "drv_common.h"
#include "drv_venc_efl.h"
#include "venc_regulator.h"
#include "tvp_adapter.h"
#include "drv_venc.h"
#include "drv_cmdlist.h"
#include "hal_venc.h"

#ifdef VENC_MCORE_ENABLE
#include "drv_venc_mcore.h"
#endif
/*lint -e750 -e838 -e715*/

typedef enum {
	UDP_DEFAULT,
	FPGA_ES,
	FPGA_CS,
} venc_platform_t;

typedef enum {
	T_IOCTL_ARG,
	T_IOCTL_ARG_COMPAT,
	T_BUTT,
} compat_type_e;

struct semaphore g_encode_sem; /* lock encode ioctrl */
struct platform_device *g_venc_pdev;

bool g_venc_open_flag = false;
bool g_venc_dev_detected = false;
#ifdef SUPPORT_SECURE_VENC
static bool g_secure_venc_initialized;
#endif

// VENC device open times
atomic_t g_venc_count = ATOMIC_INIT(0);
static int32_t  venc_drv_setup_cdev(venc_entry_t *venc, const struct file_operations *fops);
static int32_t  venc_drv_cleanup_cdev(venc_entry_t *venc);
static int32_t  venc_drv_probe(struct platform_device *pdev);
static int32_t  venc_drv_remove(struct platform_device *pdev);
#ifdef VENC_DEBUG_ENABLE
static void venc_drv_set_ops(void);
#endif

#ifdef SUPPORT_VENC_FREQ_CHANGE
DEFINE_MUTEX(g_venc_freq_mutex);
uint32_t g_venc_freq = 0;
#endif

static int32_t venc_drv_cmd_encode(unsigned int cmd,
			struct venc_fifo_buffer *buffer, void __user *user_arg)
{
	int32_t ret;
	void *kernel_arg = NULL;
	struct encode_info *encode_info = NULL;

	ret = vcodec_venc_down_interruptible(&g_encode_sem);
	if (ret != 0) {
		VCODEC_FATAL_VENC("encode: down interruptible failed");
		return ret;
	}

	ret = drv_mem_copy_from_user(cmd, user_arg, &kernel_arg);
	if (ret != 0) {
		VCODEC_FATAL_VENC("encode: copy from user failed");
		vcodec_venc_up_interruptible(&g_encode_sem);
		return ret;
	}

	encode_info = (struct encode_info *)kernel_arg;
	if (encode_info == NULL) {
		VCODEC_FATAL_VENC("encode info is null");
		vcodec_venc_up_interruptible(&g_encode_sem);
		return VCODEC_FAILURE;
	}

#ifdef SUPPORT_SECURE_VENC
	if (encode_info->is_protected && g_secure_venc_initialized == false) {
		ret = init_kernel_ca();
		if (ret != 0) {
			VCODEC_ERR_VENC("secure encode init kernel ca failed, ret value is %d", ret);
			vcodec_venc_up_interruptible(&g_encode_sem);
			return VCODEC_FAILURE;
		}
		g_secure_venc_initialized = true;
	}
#else
	if (encode_info->is_protected) {
		VCODEC_FATAL_VENC("not support secure encode");
		vcodec_venc_up_interruptible(&g_encode_sem);
		return VCODEC_FAILURE;
	}
#endif

	ret = venc_drv_encode(encode_info, buffer);
	if (ret != 0) {
		VCODEC_FATAL_VENC("encode failed");
		vcodec_venc_up_interruptible(&g_encode_sem);
		return ret;
	}

	if (encode_info->is_block) {
		ret = (int32_t)copy_to_user(user_arg, encode_info, sizeof(*encode_info));
		if (ret)
			VCODEC_FATAL_VENC("encode copy to user failed");
	}

	vcodec_venc_up_interruptible(&g_encode_sem);
	return ret;
}

static int32_t venc_compat_get_data(
	compat_type_e type, void __user *user_data, void *out_data)
{
	int32_t ret = 0;
	int32_t s32_data = 0;
	compat_ulong_t compat_data = 0;
	venc_ioctl_msg *ioctl_msg = (venc_ioctl_msg *)out_data;

	if (!user_data || !out_data) {
		VCODEC_FATAL_VENC("%s: param is null\n", __func__);
		return VCODEC_FAILURE;
	}

	switch (type) {
	case T_IOCTL_ARG:
		if (copy_from_user(ioctl_msg, user_data, sizeof(*ioctl_msg))) {
			VCODEC_FATAL_VENC("%s: copy_from_user failed\n", __func__);
			ret = VCODEC_FAILURE;
		}
		break;
	case T_IOCTL_ARG_COMPAT: {
		compat_ioctl_msg __user *compat_msg = user_data;

		ret += get_user(s32_data, &(compat_msg->chan_num));
		ioctl_msg->chan_num = s32_data;

		ret += get_user(s32_data, &(compat_msg->in_size));
		ioctl_msg->in_size = s32_data;

		ret += get_user(s32_data, &(compat_msg->out_size));
		ioctl_msg->out_size = s32_data;

		ret += get_user(compat_data, &(compat_msg->in));
		ioctl_msg->in = (void *)(uintptr_t)compat_data;

		ret += get_user(compat_data, &(compat_msg->out));
		ioctl_msg->out = (void *)(uintptr_t)compat_data;

		ret = (ret == 0) ? 0 : VCODEC_FAILURE;
	}
		break;
	default:
		VCODEC_FATAL_VENC("%s: unkown type %d\n", __func__, type);
		ret = VCODEC_FAILURE;
		break;
	}

	return ret;
}

static long venc_ioctl_common(struct file *file, unsigned int cmd, unsigned long arg, compat_type_e type)
{
	int32_t ret;
	void __user *user_arg = (void __user *)(uintptr_t)arg;
	struct encode_done_info info = {0};

	venc_buffer_record_t buf_node = {0};
	venc_ioctl_msg ioctl_msg = {0};

	if (!user_arg) {
		VCODEC_FATAL_VENC("uarg is NULL");
		return VCODEC_FAILURE;
	}

	if (!file->private_data) {
		VCODEC_FATAL_VENC("file->private_data is null");
		return VCODEC_FAILURE;
	}

	switch (cmd) {
	case CMD_VENC_ENCODE:
		ret = venc_drv_cmd_encode(cmd, (struct venc_fifo_buffer *) file->private_data, user_arg);
		if (ret != 0)
			return VCODEC_FAILURE;
		break;
	case CMD_VENC_GET_ENCODE_DONE_INFO:
		ret = venc_drv_get_encode_done_info((struct venc_fifo_buffer *) file->private_data, &info);
		if (ret != 0) {
			VCODEC_FATAL_VENC("encode done info buffer is empty");
			return VCODEC_FAILURE;
		}

		if (copy_to_user(user_arg, &info, sizeof(info))) {
			VCODEC_FATAL_VENC("get encode done info: copy to user failed");
			return VCODEC_FAILURE;
		}
		break;
	case CMD_VENC_IOMMU_MAP:
		ret = venc_compat_get_data(type, user_arg, &ioctl_msg);
		if (ret != 0) {
			VCODEC_FATAL_VENC("venc_compat_get_data failed");
			return VCODEC_FAILURE;
		}

		if (copy_from_user(&buf_node, ioctl_msg.in, sizeof(buf_node))) {
			VCODEC_FATAL_VENC("iommu map: copy frome user failed");
			return VCODEC_FAILURE;
		}

		ret = drv_mem_iommumap(&buf_node, g_venc_pdev);
		if (ret != 0) {
			VCODEC_FATAL_VENC("iommu map failed");
			return VCODEC_FAILURE;
		}

		if (copy_to_user(ioctl_msg.out, &buf_node, sizeof(buf_node))) {
			VCODEC_FATAL_VENC("iommu map: copy to user failed");
			return VCODEC_FAILURE;
		}
		break;
	case CMD_VENC_IOMMU_UNMAP:
		ret = venc_compat_get_data(type, user_arg, &ioctl_msg);
		if (ret != 0) {
			VCODEC_FATAL_VENC("venc_compat_get_data failed");
			return VCODEC_FAILURE;
		}

		if (copy_from_user(&buf_node, ioctl_msg.in, sizeof(buf_node))) {
			VCODEC_FATAL_VENC("iommu unmap: copy frome user failed");
			return VCODEC_FAILURE;
		}

		ret = drv_mem_iommuunmap(buf_node.shared_fd, buf_node.iova, g_venc_pdev);
		if (ret != 0) {
			VCODEC_FATAL_VENC("iommu unmap failed");
			return VCODEC_FAILURE;
		}
		break;
	default:
		VCODEC_ERR_VENC("venc cmd unknown:0x%x", cmd);
		ret = VCODEC_FAILURE;
		break;
	}

	return ret;
}

static long venc_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return venc_ioctl_common(file, cmd, arg, T_IOCTL_ARG);
}

#ifdef CONFIG_COMPAT
static long venc_drv_compat_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	void *user_ptr = compat_ptr(arg);

	return  venc_ioctl_common(file, cmd,
		(unsigned long)(uintptr_t)user_ptr, T_IOCTL_ARG_COMPAT);
}
#endif

static int32_t venc_drv_open(struct inode *finode, struct file  *ffile)
{
	int32_t ret;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	ret = vcodec_venc_down_interruptible(&venc->hw_sem);
	if (ret) {
		VCODEC_FATAL_VENC("Open down interruptible failed");
		return VCODEC_FAILURE;
	}

	if (atomic_read(&g_venc_count) == MAX_OPEN_COUNT) {
		VCODEC_FATAL_VENC("open venc too much");
		vcodec_venc_up_interruptible(&venc->hw_sem);
		return -EAGAIN;
	}

	ret = venc_drv_alloc_encode_done_info_buffer(ffile);
	if (ret != 0) {
		VCODEC_FATAL_VENC("malloc buffer fail");
		vcodec_venc_up_interruptible(&venc->hw_sem);
		return VCODEC_FAILURE;
	}

	if (atomic_inc_return(&g_venc_count) == 1) {
#ifdef VENC_DEBUG_ENABLE
		venc_drv_set_ops();
#endif
		venc_drv_init_pm();
		ret = venc_regulator_enable();
		if (ret != 0) {
			VCODEC_FATAL_VENC("init failed, ret value is %d", ret);
			atomic_dec(&g_venc_count);
			venc_drv_free_encode_done_info_buffer(ffile);
			vcodec_venc_up_interruptible(&venc->hw_sem);
			return VCODEC_FAILURE;
		}

		ret = venc_drv_open_vedu();
		if (ret != 0) {
			VCODEC_FATAL_VENC("venc firmware layer open failed, ret value is %d", ret);
			atomic_dec(&g_venc_count);
			venc_regulator_disable();
			venc_drv_free_encode_done_info_buffer(ffile);
			vcodec_venc_up_interruptible(&venc->hw_sem);
			return VCODEC_FAILURE;
		}
#ifdef SUPPORT_SECURE_VENC
		g_secure_venc_initialized = false;
#endif
	}

	g_venc_open_flag = true;
	vcodec_venc_up_interruptible(&venc->hw_sem);

	VCODEC_INFO_VENC("Open venc device successfully");
	return 0;
}

static int32_t venc_drv_close(struct inode *finode, struct file  *ffile)
{
	int32_t ret;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	ret = vcodec_venc_down_interruptible(&venc->hw_sem);
	if (ret) {
		VCODEC_FATAL_VENC("Close down interruptible failed");
		return VCODEC_FAILURE;
	}

	if (atomic_dec_and_test(&g_venc_count)) {
#ifdef SUPPORT_SECURE_VENC
		if (g_secure_venc_initialized != false) {
			deinit_kernel_ca();
			g_secure_venc_initialized = false;
		}
#endif
		venc_drv_close_vedu();
		venc_regulator_disable();
		venc_drv_print_pm_info();
		venc_drv_deinit_pm();
		g_venc_open_flag = false;
	}

	ret = venc_drv_free_encode_done_info_buffer(ffile);
	if (ret != 0) {
		VCODEC_FATAL_VENC("free buffer fail");
		vcodec_venc_up_interruptible(&venc->hw_sem);
		return VCODEC_FAILURE;
	}

	vcodec_venc_up_interruptible(&venc->hw_sem);

	VCODEC_INFO_VENC("Close venc device successfully");
	return 0;
}

int32_t venc_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	int32_t ret;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	VCODEC_INFO_VENC("+");

	ret = vcodec_venc_down_interruptible(&venc->hw_sem);
	if (ret) {
		VCODEC_ERR_VENC("Suspend down interruptible failed");
		return VCODEC_FAILURE;
	}

	if (!g_venc_open_flag)
		goto exit;

	ret = venc_drv_suspend_vedu();
	if (ret != 0) {
		VCODEC_FATAL_VENC("venc firmware layer suspend failed, ret value is %d", ret);
		vcodec_venc_up_interruptible(&venc->hw_sem);
		return VCODEC_FAILURE;
	}

	venc_regulator_disable();
exit:
	vcodec_venc_up_interruptible(&venc->hw_sem);
	VCODEC_INFO_VENC("-");
	return 0;
}

int32_t venc_drv_resume(struct platform_device *pdev)
{
	int32_t ret;
	struct clock_info info;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	VCODEC_INFO_VENC("+");

	ret = vcodec_venc_down_interruptible(&venc->hw_sem);
	if (ret) {
		VCODEC_FATAL_VENC("Resume down interruptible failed");
		return VCODEC_FAILURE;
	}

	if (!g_venc_open_flag) {
		goto exit;
	}

	venc_get_clock_info(&info);
	/* power on at least one core. */
	info.core_num = info.core_num ? info.core_num : 1;
	ret = venc_regulator_update(&info);
	if (ret != 0) {
		VCODEC_FATAL_VENC("board init failed, ret value is %d", ret);
		vcodec_venc_up_interruptible(&venc->hw_sem);
		return VCODEC_FAILURE;
	}

	ret = venc_drv_resume_vedu();
	if (ret != 0) {
		VCODEC_FATAL_VENC("venc firmware layer resume failed, ret value is %d", ret);
		vcodec_venc_up_interruptible(&venc->hw_sem);
		return VCODEC_FAILURE;
	}
exit:
	vcodec_venc_up_interruptible(&venc->hw_sem);
	VCODEC_INFO_VENC("-");
	return 0;
}

static struct file_operations venc_fops = {
	.owner          = THIS_MODULE, /*lint !e64*/
	.open           = venc_drv_open,
	.unlocked_ioctl = venc_drv_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = venc_drv_compat_ioctl,
#endif
	.release        = venc_drv_close,
}; /*lint !e785*/

static const struct of_device_id venc_of_match[] = {
	{ .compatible = VENC_COMPATIBLE, }, /*lint !e785 !e651*/
	{ } /*lint !e785*/
};

static struct platform_driver venc_driver = {
	.probe   = venc_drv_probe,
	.remove  = venc_drv_remove,
	.suspend = venc_drv_suspend,
	.resume  = venc_drv_resume,
	.driver  = {
		.name    = "vcodec_venc",
		.owner   = THIS_MODULE, /*lint !e64*/
		.of_match_table = venc_of_match
	}, /*lint !e785*/
}; /*lint !e785*/

#ifdef VENC_DEBUG_ENABLE
static void venc_drv_set_ops(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	VCODEC_INFO_VENC("debug flag:0x%x", venc->debug_flag);
#ifdef VENC_MCORE_ENABLE
	if (venc->debug_flag & (1LL << MCORE_ENABLE)) {
		venc_init_ops();
		return;
	}
#endif

#ifdef SUPPORT_CMDLIST
	if (venc->debug_flag & (1LL << CMDLIST_DISABLE))
		venc_init_ops();
	else
		cmdlist_init_ops();
#endif
}

static int show_hardware_usage(char *buf, uint32_t size)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	int32_t total_len = 0;
	int32_t cur_len;
	uint32_t i;
	uint32_t core_num = venc_get_core_num();

	for (i = 0; i < MAX_SUPPORT_CORE_NUM && i < core_num; i++) {
		cur_len = pm_show_period_account(&venc->ctx[i].pm, buf + total_len, size - total_len);
		if (cur_len <= 0)
			return 0;
		total_len += cur_len;

		cur_len = pm_show_total_account(&venc->ctx[i].pm, buf + total_len, size - total_len);
		if (cur_len <= 0)
			return 0;
		total_len += cur_len;
	}

	return total_len;
}
#endif

static ssize_t venc_drv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
#ifdef VENC_DEBUG_ENABLE
	int32_t len;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	if (buf == VCODEC_NULL) {
		VCODEC_ERR_VENC("%s buf is null", __func__);
		return 0;
	}

	len = sprintf_s(buf, PAGE_SIZE, "smmu page addr: 0x%pK, debug flag: 0x%x\n",
		(void *)(uintptr_t)venc_get_smmu_ttb(), venc->debug_flag);
	if (len <= 0)
		return 0;

	if (venc->debug_flag & (1LL << PRINT_HARDWARE_USAGE))
		len += show_hardware_usage(buf + len, PAGE_SIZE - len);

	return len;
#else
	return 0;
#endif
}

static ssize_t venc_drv_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef VENC_DEBUG_ENABLE
	unsigned long val = 0;
	uint64_t mask = 1LL << CMDLIST_DISABLE;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	if (kstrtol(buf, 0, &val) < 0)
		return count;

	if ((((uint32_t)val & mask) != (venc->debug_flag & mask)) &&
		(atomic_read(&g_venc_count) != 0)) {
		VCODEC_WARN_VENC("The configuration mode cannot be changed during encoding");
		return count;
	}

	venc->debug_flag = (uint32_t)val;
#endif
	return count;
}

#ifdef SUPPORT_SWITCH_POWER_OFF_PER_FRAME
static ssize_t venc_frame_power_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t len;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	if (!buf) {
		VCODEC_ERR_VENC("%s buf is null", __func__);
		return 0;
	}
	len = sprintf_s(buf, PAGE_SIZE, "power off per frame switch: 0x%x\n", venc->popf_switch);
	if (len <= 0)
		return 0;

	return len;
}

static ssize_t venc_frame_power_off_store(struct device *dev,
	struct device_attribute *attr,const char *buf, size_t count)
{
	bool val = false;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	if (kstrtobool(buf, &val))
		return -EINVAL;

	if ((((uint32_t)val) != venc->popf_switch) &&
		(atomic_read(&g_venc_count) != 0)) {
		VCODEC_ERR_VENC("The configuration mode cannot be changed during encoding");
		return count;
	}

	venc->popf_switch = (uint32_t)val;
	return count;
}
#endif

#ifdef SUPPORT_VENC_FREQ_CHANGE
static ssize_t venc_freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count;
	if (!buf) {
		VCODEC_ERR_VENC("buf is NULL");
		return -EINVAL;
	}

	mutex_lock(&g_venc_freq_mutex);
	count = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%u", g_venc_freq);
	mutex_unlock(&g_venc_freq_mutex);

	return count;
}

static ssize_t venc_freq_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t vencfreq = 0;
	int ret;

	if (!buf) {
		VCODEC_ERR_VENC("buf is NULL");
		return -EINVAL;
	}

	ret = kstrtouint(buf, 10, &vencfreq);
	if (ret != 0) {
		VCODEC_ERR_VENC("invalid vencFreqStr, vencfreq is :%u, ret = %d", vencfreq, ret);
		return -EINVAL;
	}

#ifdef VCODECV500
	if (vencfreq > 3) {
#else
	if (vencfreq > 2) {
#endif
		VCODEC_ERR_VENC("invalid venc freq from perf");
		mutex_lock(&g_venc_freq_mutex);
		g_venc_freq = 0;
		mutex_unlock(&g_venc_freq_mutex);
		return -EINVAL;
	}

	mutex_lock(&g_venc_freq_mutex);
	g_venc_freq = vencfreq;
	VCODEC_INFO_VENC("%s g_venc_freq:%d", __func__, g_venc_freq);
	mutex_unlock(&g_venc_freq_mutex);

	return count;
}

static DEVICE_ATTR(venc_freq, 0660, venc_freq_show, venc_freq_store);
#endif

static DEVICE_ATTR(venc_misc, 0660, venc_drv_show, venc_drv_store);

#ifdef SUPPORT_SWITCH_POWER_OFF_PER_FRAME
static DEVICE_ATTR(venc_frame_power_off, 0660, venc_frame_power_off_show, venc_frame_power_off_store);
#endif

static int32_t venc_drv_setup_cdev(venc_entry_t *venc, const struct file_operations *fops)
{
	int32_t err;

	VCODEC_INFO_VENC("enter %s()", __func__);
	err = alloc_chrdev_region(&venc->dev, 0, 1, "vcodec_encoder");
	if (err < 0)
		return VCODEC_FAILURE;

	(void)memset_s((void *)(&(venc->cdev)), sizeof(struct cdev), 0, sizeof(struct cdev));
	cdev_init(&(venc->cdev), &venc_fops);

	venc->cdev.owner = THIS_MODULE; /*lint !e64*/
	err = cdev_add(&(venc->cdev), venc->dev, 1);
	if (err < 0) {
		VCODEC_ERR_VENC("Fail to add venc, err value is %d", err);
		goto unregister_region;
	}

	/* Create the device category directory vcodec_venc in the /sys/class/ directory */
	venc->venc_class = class_create(THIS_MODULE, "vcodec_venc"); /*lint !e64*/
	if (IS_ERR(venc->venc_class)) {
		err = PTR_ERR(venc->venc_class); /*lint !e712*/
		VCODEC_ERR_VENC("Fail to create vcodec_venc class, err value is %d", err);
		goto dev_del;
	}

	/* Create the device file vcodec_venc in /dev/ and /sys/class/vcodec_venc respectively */
	venc->venc_device = device_create(venc->venc_class, NULL, venc->dev, "%s", "vcodec_venc");
	if (IS_ERR(venc->venc_device)) {
		err = PTR_ERR(venc->venc_device); /*lint !e712*/
		VCODEC_ERR_VENC("Fail to create vcodec_venc device, err value is %d", err);
		goto cls_destroy;
	}

	err = device_create_file(venc->venc_device, &dev_attr_venc_misc);
	if (err) {
		VCODEC_ERR_VENC("%s, failed to create device file", __func__);
		goto dev_destroy;
	}

#ifdef SUPPORT_VENC_FREQ_CHANGE
	err = device_create_file(venc->venc_device, &dev_attr_venc_freq);
	if (err) {
		VCODEC_ERR_VENC("%s, failed to create device file", __func__);
		goto venc_freq_failed;
	}
#endif

#ifdef SUPPORT_SWITCH_POWER_OFF_PER_FRAME
	err = device_create_file(venc->venc_device, &dev_attr_venc_frame_power_off);
	if (err) {
		VCODEC_ERR_VENC("%s, failed to create device file", __func__);
		goto venc_frame_power_off_failed;
	}
#endif

	VCODEC_INFO_VENC("exit %s()", __func__);
	return 0;

#ifdef SUPPORT_SWITCH_POWER_OFF_PER_FRAME
venc_frame_power_off_failed:
#endif
#ifdef SUPPORT_VENC_FREQ_CHANGE
	device_remove_file(venc->venc_device, &dev_attr_venc_freq);
venc_freq_failed:
#endif
#if ((defined SUPPORT_VENC_FREQ_CHANGE) || (defined SUPPORT_SWITCH_POWER_OFF_PER_FRAME))
	device_remove_file(venc->venc_device, &dev_attr_venc_misc);
#endif
dev_destroy:
	device_destroy(venc->venc_class, venc->dev);
cls_destroy:
	class_destroy(venc->venc_class);
	venc->venc_class = VCODEC_NULL;
dev_del:
	cdev_del(&venc->cdev);
unregister_region:
	unregister_chrdev_region(venc->dev, 1);
	return VCODEC_FAILURE;
}

static int32_t venc_drv_cleanup_cdev(venc_entry_t *venc)
{
	if (venc->venc_class) {
		device_remove_file(venc->venc_device, &dev_attr_venc_misc);
#ifdef SUPPORT_VENC_FREQ_CHANGE
		device_remove_file(venc->venc_device, &dev_attr_venc_freq);
#endif

#ifdef SUPPORT_SWITCH_POWER_OFF_PER_FRAME
		device_remove_file(venc->venc_device, &dev_attr_venc_frame_power_off);
#endif
		device_destroy(venc->venc_class, venc->dev);
		class_destroy(venc->venc_class);
	}

	cdev_del(&(venc->cdev));
	unregister_chrdev_region(venc->dev, 1);

	return 0;
}

static uint32_t venc_drv_device_idle(venc_platform_t platform_type)
{
	uint32_t idle   = 0;
	uint32_t *platform_ctrl = VCODEC_NULL;
	switch (platform_type) {
#ifdef VENC_ES_SUPPORT
	case FPGA_ES:
		platform_ctrl = (uint32_t *)ioremap(VENC_PCTRL_PERI_ES, (unsigned long)4); /*lint !e747*/
		if (platform_ctrl)
			idle = readl(platform_ctrl) & VENC_EXISTBIT_ES; /*lint !e50 !e64*/
		break;
#endif

#ifdef VENC_CS_SUPPORT
	case FPGA_CS: {
#ifdef VCODECV320
		uint32_t *pctrl_msk = (uint32_t *)ioremap(VENC_PCTRL_PERI_MSK, (unsigned long)4);
		if (pctrl_msk) {
			writel(VENC_PCTRL_PERI_MSK_DATA, pctrl_msk);
			iounmap(pctrl_msk);
		} else {
			break;
		}
#endif
		platform_ctrl = (uint32_t *)ioremap(VENC_PCTRL_PERI_CS, (unsigned long)4);//lint !e446
		if (platform_ctrl)
			idle = readl(platform_ctrl) & VENC_EXISTBIT_CS; /*lint !e50 !e64*/
		break;
	}
#endif

	default:
		printk(KERN_ERR "unkown platform_type %d", platform_type);
		break;
	}

	if (!platform_ctrl) {
		printk(KERN_ERR "ioremap failed");
		return false;
	} else {
		iounmap(platform_ctrl);
	}
#if (defined(VCODECV600) || defined(VCODECV700))
	idle = (uint32_t)(idle == VENC_EXIST_TRUE);
#endif

	return (uint32_t)((idle != 0) ? true : false);
}

static int32_t venc_drv_check_device_status(struct platform_device *pdev)
{
	int32_t ret;
	uint32_t fpga_cs_flag = 0;
	uint32_t fpga_es_flag = 0;
	struct device *dev = NULL;
	venc_platform_t platform_name = UDP_DEFAULT;

	dev = &pdev->dev;

#ifdef VENC_ES_SUPPORT
	ret = of_property_read_u32(dev->of_node, VENC_FPGAFLAG_ES, &fpga_es_flag);
	if (ret)
		VCODEC_INFO_VENC("read failed, but fpga_es has defualt value");
	if (fpga_es_flag == 1)
		platform_name = FPGA_ES;
#endif

#ifdef VENC_CS_SUPPORT
	ret = of_property_read_u32(dev->of_node, VENC_FPGAFLAG_CS, &fpga_cs_flag);
	if (ret)
		VCODEC_INFO_VENC("read failed, but fpga_cs has defualt value");
	if (fpga_cs_flag == 1)
		platform_name = FPGA_CS;
#endif

	if ((fpga_cs_flag == 1) || (fpga_es_flag == 1)) {
#ifdef VCODECV700
#ifdef PCIE_LINK
		VCODEC_INFO_VENC("current is fpga_pcie platform");
		return 0;
#else
		return VCODEC_FAILURE;
#endif
#endif
		VCODEC_INFO_VENC("fpga platform");
		if (venc_drv_device_idle(platform_name) == false) {
			VCODEC_INFO_VENC("venc is not exist");
			return VCODEC_FAILURE;
		}
	} else {
		VCODEC_INFO_VENC("not fpga platform");
	}

	return 0;
}

static int32_t venc_drv_init_device(struct platform_device *pdev)
{
	venc_entry_t *venc = NULL;
	int32_t ret;
	int32_t i;

	vcodec_venc_init_sem(&g_encode_sem);
	ret = dma_set_mask_and_coherent(&pdev->dev, ~0ULL);
	if (ret) {
		VCODEC_ERR_VENC("dma set mask and coherent failed");
		return ret;
	}

	ret = venc_drv_create_queue();
	if (ret != 0) {
		VCODEC_ERR_VENC("create queue failed");
		return ret;
	}

	venc = (venc_entry_t *)vcodec_mem_valloc(sizeof(venc_entry_t));
	if (venc == NULL) {
		VCODEC_FATAL_VENC("call vmalloc failed");
		goto destroy_queue;
	}
	(void)memset_s((void *)venc, sizeof(venc_entry_t), 0, sizeof(venc_entry_t));
	vcodec_venc_init_sem(&venc->hw_sem);
	mutex_init(&venc->backup_info.lock);
	venc_drv_osal_init_event(&venc->event);
	for (i = 0; i < MAX_SUPPORT_CORE_NUM; i++)
		spin_lock_init(&venc->ctx[i].lock);

	ret = venc_drv_setup_cdev(venc, &venc_fops);
	if (ret < 0) {
		VCODEC_ERR_VENC("setup char device failed");
		goto free;
	}

	platform_set_drvdata(pdev, venc);
	venc_set_device(pdev);

#ifdef VCODECV700
	venc->debug_flag = 1;
#endif

	return 0;

free:
	vcodec_mem_vfree(venc);
destroy_queue:
	venc_drv_destroy_queue();

	return VCODEC_FAILURE;
}

static void venc_drv_deinit_device(struct platform_device *pdev)
{
	venc_entry_t *venc = NULL;

	venc = platform_get_drvdata(pdev);
	if (venc != NULL) {
		venc_drv_cleanup_cdev(venc);
		if (venc->mcore_image != NULL)
			vcodec_mem_vfree(venc->mcore_image);
		vcodec_mem_vfree(venc);
	} else {
		VCODEC_ERR_VENC("get platform drvdata err");
	}

	platform_set_drvdata(pdev, NULL);
	venc_drv_destroy_queue();
}

static void encode_mode_init(void)
{
#ifdef SUPPORT_CMDLIST
	cmdlist_init_ops();
#else
	venc_init_ops();
#endif

#ifdef VENC_MCORE_ENABLE
	venc_init_ops();
#endif
}

static int32_t venc_drv_probe(struct platform_device *pdev)
{
	int32_t ret;

	if (!pdev) {
		VCODEC_FATAL_VENC("invalid argument");
		return VCODEC_FAILURE;
	}

	VCODEC_INFO_VENC("venc prepare to probe");

	if (g_venc_dev_detected) {
		VCODEC_INFO_VENC("venc device detected already");
		return 0;
	}

	if (venc_drv_check_device_status(pdev) != 0)
		return VCODEC_FAILURE;

	/* 0 init device */
	if (venc_drv_init_device(pdev) != 0) {
		VCODEC_FATAL_VENC("init device failed");
		return VCODEC_FAILURE;
	}

	/* 1 read venc dts info from dts */
	ret = venc_get_dts_config_info(pdev);
	if (ret != 0) {
		VCODEC_FATAL_VENC("get venc DTS config info failed");
		goto cleanup;
	}

	/* 2 get regulator interface */
	ret = venc_get_regulator_info(pdev);
	if (ret != 0) {
		VCODEC_FATAL_VENC("get venc regulator failed");
		goto cleanup;
	}

	/* 3 init ops */
	encode_mode_init();
#ifdef VENC_MCORE_ENABLE
	ret = venc_mcore_load_image();
	if (ret != 0) {
		VCODEC_FATAL_VENC("load mcu image failed");
		goto cleanup;
	}
#endif

	g_venc_dev_detected = true;

	VCODEC_INFO_VENC("venc probe successfully");

	return 0;

cleanup:
	venc_drv_deinit_device(pdev);
	return ret;
}

static int32_t venc_drv_remove(struct platform_device *pdev)
{
	VCODEC_INFO_VENC("venc prepare to remove");

	venc_drv_deinit_device(pdev);
	g_venc_dev_detected = false;

	VCODEC_INFO_VENC("remove venc successfully");
	return 0;
}

struct platform_device *venc_get_device(void)
{
	return g_venc_pdev;
}

void venc_set_device(struct platform_device *dev)
{
	g_venc_pdev = dev;
}

int32_t __init venc_drv_mod_init(void)
{
	int32_t ret;

	VCODEC_INFO_VENC("enter %s()", __func__);

	if (venc_device_need_bypass()) {
		VCODEC_WARN_VENC("by pass venc driver register");
		return -EINVAL;
	}

	ret = platform_driver_register(&venc_driver); /*lint !e64*/
	if (ret) {
		VCODEC_ERR_VENC("register platform driver failed");
		return ret;
	}
	VCODEC_INFO_VENC("success");

#ifdef MODULE
	VCODEC_INFO_VENC("Load vcodec_venc.ko success\t(%s)", VERSION_STRING);
#endif

	VCODEC_INFO_VENC("exit %s()", __func__);

	return ret;
}

void venc_drv_mod_exit(void)
{
	VCODEC_INFO_VENC("enter %s()", __func__);

	if (venc_device_need_bypass()) {
		VCODEC_WARN_VENC("need by pass venc driver register");
		return;
	}

	platform_driver_unregister(&venc_driver);

	VCODEC_INFO_VENC("exit %s()", __func__);
	return;
}
/*lint -e528*/
module_init(venc_drv_mod_init); /*lint !e528*/
module_exit(venc_drv_mod_exit); /*lint !e528*/
/*lint -e753*/
MODULE_LICENSE("Dual BSD/GPL"); /*lint !e753*/
