/*
 * vcodec_vdec.c
 *
 * This is for vdec management
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
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

#include <linux/platform_device.h>
#include "vcodec_vdec.h"
#include "vcodec_vdec_regulator.h"
#include "vcodec_vdec_memory.h"
#include "vcodec_vdec_plat.h"
#include "vfmw_intf_check.h"
#include "vfmw_intf.h"
#include "smmu/smmu.h"
#include "vcodec_vdec_dpm.h"
#include "vcodec_vdec_utils.h"

#define INVALID_IDX             (-1)

static int32_t g_is_normal_init;

static struct class *g_vdec_class = VCODEC_NULL;
static const char g_vdec_drv_name[] = "vcodec_vdec";
static dev_t g_vdec_dev_num;
static performance_info g_perf_info[MAX_OPEN_COUNT];

vdec_entry g_vdec_entry;
static struct dec_file_handle g_file_handle[MAX_OPEN_COUNT];

typedef enum {
	T_IOCTL_ARG,
	T_IOCTL_ARG_COMPAT,
	T_BUTT,
} compat_type_e;

#define check_para_size_return(size, para_size, command) \
	do { \
		if ((size) != (para_size)) { \
			dprint(PRN_FATAL, "%s: prarameter_size is error\n", command); \
			return -EINVAL; \
		} \
	} while (0)

#define check_return(cond, else_print) \
	do { \
		if (!(cond)) { \
			dprint(PRN_FATAL, "%s\n", else_print); \
			return -EINVAL; \
		} \
	} while (0)

typedef int32_t(*fn_ioctl_handler)(struct file *file_handle, vdec_ioctl_msg *pmsg);

typedef struct {
	uint32_t cmd;
	fn_ioctl_handler handle;
} ioctl_command_node;

static int32_t vdec_get_file_index(const struct file *file)
{
	int32_t index;

	for (index = 0; index < MAX_OPEN_COUNT; index++) {
		if (file == g_perf_info[index].file)
			return index;
	}

	return INVALID_IDX;
}

static clk_rate_e get_clk_rate(performance_params_s param)
{
	int32_t i;
	clk_rate_e max_base_rate = VDEC_CLK_RATE_LOWER;
	clk_rate_e clk_rate = VDEC_CLK_RATE_LOWER;
	uint64_t total_load = 0;

	for (i = 0; i < MAX_OPEN_COUNT; i++) {
		if (g_perf_info[i].file) {
			max_base_rate = g_perf_info[i].base_rate > max_base_rate ?
				g_perf_info[i].base_rate : max_base_rate;
			if (ULLONG_MAX - total_load >= g_perf_info[i].load)
				total_load += g_perf_info[i].load;
		}
	}

	for (i = VDEC_CLK_RATE_MAX - 1; i >= 0; i--) {
		if (total_load > param.load_range_map[i].lower_limit &&
			total_load <= param.load_range_map[i].upper_limit) {
			clk_rate = param.load_range_map[i].clk_rate;
			break;
		}
	}
	dprint(PRN_ALWS, "total_load %llu, clk_rate %d, max_base_rate %d\n",
		total_load, clk_rate, max_base_rate);
	clk_rate = max_base_rate > clk_rate ? max_base_rate : clk_rate;

	return clk_rate;
}

vdec_entry *vdec_get_entry()
{
	return &g_vdec_entry;
}

static void vdec_init_all_file_handle(void)
{
	uint32_t instance_id;

	for (instance_id = 0; instance_id < MAX_OPEN_COUNT; instance_id++) {
		g_file_handle[instance_id].instance_id = instance_id;
		g_file_handle[instance_id].is_used = 0;
		g_file_handle[instance_id].vdec_entry = VCODEC_NULL;
	}
}

static int32_t vdec_drv_acquire_file_handle(struct dec_file_handle **file)
{
	int32_t ret;
	uint32_t instance_id;

	check_return(file, "input param is null");

	for (instance_id = 0; instance_id < MAX_OPEN_COUNT; instance_id++)
		if (g_file_handle[instance_id].is_used == 0)
			break;

	if (instance_id == MAX_OPEN_COUNT) {
		dprint(PRN_ERROR, "failed to find idle chan");
		return VCODEC_FAILURE;
	}

	ret = vfmw_acquire_back_up_fifo(instance_id);
	if (ret != 0) {
		dprint(PRN_FATAL, "alloc back up fifo failed");
		return VCODEC_FAILURE;
	}

	g_file_handle[instance_id].is_used = 1;
	*file = &g_file_handle[instance_id];

	return 0;
}

static int32_t vdec_drv_release_file_handle(struct dec_file_handle *file_handle)
{
	check_return(file_handle, "input param is null");

	if (file_handle->is_used == 0) {
		dprint(PRN_ERROR, "file handle is not used");
		return VCODEC_FAILURE;
	}

	vfmw_release_back_up_fifo(file_handle->instance_id);
	file_handle->is_used = 0;

	return 0;
}

static ssize_t vdec_freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count;
	clk_rate_e clk_rate = VDEC_CLK_RATE_MAX;

	if (buf == NULL)
		return 0;

	vdec_plat_get_dynamic_clk_rate(&clk_rate);
	count = sprintf_s(buf, PAGE_SIZE, "%d\n", clk_rate);

	return count;
}

static ssize_t vdec_freq_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long val = 0;

	if (kstrtol(buf, 0, &val) < 0)
		return count;

	if (val >= VDEC_CLK_RATE_MAX || val < 0)
		return count;

	vdec_plat_set_dynamic_clk_rate((clk_rate_e)val);
	dprint(PRN_ALWS, "set clk is %u", (uint32_t)val);

	return count;
}
#ifdef ENABLE_VDEC_PROC
ssize_t vdec_setting_info_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	if (dev == NULL || attr == NULL || buf == NULL)
		return 0;

	return vdec_show_setting_info(buf, PAGE_SIZE, &g_vdec_entry.setting_info);
}

ssize_t vdec_setting_info_store(
	struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if (dev == NULL || attr == NULL || buf == NULL)
		return count;

	if (g_is_normal_init != 0)
		return count;

	return vdec_store_setting_info(&g_vdec_entry.setting_info, buf, count);
}

static DEVICE_ATTR(vdec_setting, 0660, vdec_setting_info_show, vdec_setting_info_store);
#endif

static DEVICE_ATTR(vdec_freq, 0660, vdec_freq_show, vdec_freq_store);

static int32_t vdec_create_class(void)
{
	int32_t rc;

	g_vdec_class = class_create(THIS_MODULE, "vdec_class");
	if (IS_ERR(g_vdec_class)) {
		rc = PTR_ERR(g_vdec_class);
		g_vdec_class = VCODEC_NULL;
		dprint(PRN_FATAL, "call class_create failed, rc = %d\n", rc);
		return rc;
	}

	return 0;
}

static void vdec_destroy_class(void)
{
	class_destroy(g_vdec_class);
	g_vdec_class = VCODEC_NULL;
}

static int32_t vdec_create_device_file(struct device *dev)
{
	int32_t rc;

	rc = device_create_file(dev, &dev_attr_vdec_freq);
	if (rc < 0) {
		dprint(PRN_FATAL, "call device_create_vdec_freq failed, rc = %d\n", rc);
		return rc;
	}

#ifdef ENABLE_VDEC_PROC
	rc = device_create_file(dev, &dev_attr_vdec_setting);
	if (rc < 0) {
		dprint(PRN_FATAL, "call device_create_vdec_setting failed, rc = %d\n", rc);
		device_remove_file(dev, &dev_attr_vdec_freq);
		return rc;
	}
#endif

	return 0;
}

static int32_t vdec_setup_cdev(vdec_entry *vdec, const struct file_operations *fops)
{
	int32_t rc;

	rc = vdec_create_class();
	if (rc) {
		dprint(PRN_FATAL, "call vdec_create_class failed, rc = %d\n", rc);
		return rc;
	}

	rc = alloc_chrdev_region(&g_vdec_dev_num, 0, 1, "vcodec_decoder");
	if (rc) {
		dprint(PRN_FATAL, "call alloc_chrdev_region failed, rc = %d\n", rc);
		goto cls_destroy;
	}

	vdec->class_dev = device_create(g_vdec_class, NULL, g_vdec_dev_num, "%s", "vcodec_vdec");
	if (IS_ERR(vdec->class_dev)) {
		rc = PTR_ERR(vdec->class_dev);
		dprint(PRN_FATAL, "call device_create failed, rc = %d\n", rc);
		goto unregister_region;
	}

	cdev_init(&vdec->cdev, fops);
	vdec->cdev.owner = THIS_MODULE;
	rc = cdev_add(&vdec->cdev, g_vdec_dev_num, 1);
	if (rc < 0) {
		dprint(PRN_FATAL, "call cdev_add failed, rc = %d\n", rc);
		goto dev_destroy;
	}

	rc = vdec_create_device_file(vdec->class_dev);
	if (rc < 0) {
		dprint(PRN_FATAL, "call vdec_create_device_file failed, rc = %d\n", rc);
		goto dev_del;
	}

	return 0;

dev_del:
	cdev_del(&vdec->cdev);
dev_destroy:
	device_destroy(g_vdec_class, g_vdec_dev_num);
unregister_region:
	unregister_chrdev_region(g_vdec_dev_num, 1);
cls_destroy:
	vdec_destroy_class();

	return rc;
}

static int32_t vdec_cleanup_cdev(vdec_entry *vdec)
{
	if (!g_vdec_class) {
		dprint(PRN_FATAL, "vdec class is NULL");
		return VCODEC_FAILURE;
	} else {
		device_remove_file(vdec->class_dev, &dev_attr_vdec_freq);
		cdev_del(&vdec->cdev);
		device_destroy(g_vdec_class, g_vdec_dev_num);
		unregister_chrdev_region(g_vdec_dev_num, 1);
		class_destroy(g_vdec_class);
		g_vdec_class = VCODEC_NULL;
		return 0;
	}
}

static int32_t vdec_enable_sec_mode(struct file *file_handle)
{
	uint8_t idx;
	vdec_chan_mode *chan_mode = &g_vdec_entry.chan_mode;

	for (idx = 0; idx < MAX_OPEN_COUNT; idx++) {
		if (chan_mode->vdec_chan_info[idx].file == NULL) {
			chan_mode->vdec_chan_info[idx].file = file_handle;
			chan_mode->sec_chan_num++;
			return 0;
		}
	}
	return -EFAULT;
}

static void vdec_disable_sec_mode(struct file *file_handle)
{
	uint8_t idx;
	vdec_chan_mode *chan_mode = &g_vdec_entry.chan_mode;

	for (idx = 0; idx < MAX_OPEN_COUNT; idx++) {
		if (chan_mode->vdec_chan_info[idx].file == file_handle) {
			chan_mode->sec_chan_num--;
			chan_mode->vdec_chan_info[idx].file = NULL;
			if (chan_mode->sec_chan_num == 0)
				dprint(PRN_ALWS, "disable_sec_mode successfully.\n");
			return;
		}
	}
}

static void vdec_init_power_state(vdec_entry *vdec)
{
	unsigned long flag;

	spin_lock_irqsave(&vdec->power_state_spin_lock, flag);
	vdec->power_state = true;
	spin_unlock_irqrestore(&vdec->power_state_spin_lock, flag);

	vdec->power_ref_cnt = 0;
}

static vdec_entry *vdec_get_entry_by_file_handle(struct file *file_handle)
{
	struct dec_file_handle *vdec_file = VCODEC_NULL;

	if (file_handle == VCODEC_NULL) {
		dprint(PRN_FATAL, "file_handle is null");
		return VCODEC_NULL;
	}

	if (file_handle->private_data == VCODEC_NULL) {
		dprint(PRN_FATAL, "private_data is null");
		return VCODEC_NULL;
	}

	vdec_file = (struct dec_file_handle *)file_handle->private_data;
	return vdec_file->vdec_entry;
}

static bool vdec_get_smmu_glb_bypass(void)
{
	return vdec_get_entry()->setting_info.smmu_glb_bypass;
}

static void vdec_lock_power_state(unsigned long flag)
{
	vdec_entry *vdec = vdec_get_entry();
	spin_lock_irqsave(&vdec->power_state_spin_lock, flag);
}

static void vdec_unlock_power_state(unsigned long flag)
{
	vdec_entry *vdec = vdec_get_entry();
	spin_unlock_irqrestore(&vdec->power_state_spin_lock, flag);
}

static bool vdec_get_power_state(void)
{
	vdec_entry *vdec = vdec_get_entry();
	return vdec->power_state;
}

static int32_t vdec_open_global_resource(vdec_entry *vdec)
{
	int32_t ret;
	vdec_plat *plt = VCODEC_NULL;
	struct vfmw_glb_vdec_callback callback = {
		vdec_lock_power_state,
		vdec_unlock_power_state,
		vdec_get_power_state,
		vdec_get_smmu_glb_bypass,
		vdec_mem_iommu_map,
		vdec_mem_iommu_unmap,
	};

	(void)memset_s(&g_vdec_entry.vdec_power, sizeof(vdec_power_info),
		0, sizeof(vdec_power_info));
	g_vdec_entry.vdec_power.vdec_open_timestamp_us = get_time_in_us();
	ret = vdec_plat_regulator_enable();
	vfmw_assert_ret_prnt(ret == 0, ret, "regulator enable failed\n");

	plt = vdec_plat_get_entry();
	ret = vfmw_init(&plt->dts_info, &callback);
	vfmw_assert_goto_prnt(ret == 0, vfmw_init_error, "vfmw init failed\n");

	if (!vdec->setting_info.smmu_glb_bypass) {
		ret = smmu_map_reg();
		vfmw_assert_goto_prnt(ret == 0, smmu_map_error, "smmu map reg failed\n");

		ret = smmu_init();
		vfmw_assert_goto_prnt(ret == 0, smmu_init_error, "smmu init failed\n");
	}

	(void)memset_s(&g_vdec_entry.chan_mode, sizeof(vdec_chan_mode),
		0, sizeof(vdec_chan_mode));
	spin_lock_init(&vdec->power_state_spin_lock);
	vdec_init_power_cost_record();

#ifdef VDEC_DPM_ENABLE
	vdec_dpm_init();
#endif
	g_is_normal_init = 1;
	vdec_init_power_state(vdec);
	plt->clk_ctrl.clk_flag = 1;
	plt->clk_ctrl.static_clk = VDEC_CLK_RATE_LOWER;
	/* After power-on, the frequency is the default frequency but not the decoder lowest frequency. */
	plt->clk_ctrl.current_clk = VDEC_CLK_RATE_MAX;

	vdec_init_all_file_handle();

	return ret;
smmu_init_error:
	if (!vdec->setting_info.smmu_glb_bypass)
		smmu_unmap_reg();
smmu_map_error:
	vfmw_deinit();
vfmw_init_error:
	vdec_plat_regulator_disable();

	return ret;
}

static int32_t vdec_open(struct inode *inode, struct file *file_handle)
{
	int32_t ret = 0;
	vdec_entry *vdec = VCODEC_NULL;
	struct dec_file_handle *vdec_file = VCODEC_NULL;
	int32_t index;

	check_return(inode != VCODEC_NULL, "inode is null");
	check_return(file_handle != VCODEC_NULL, "file_handle is null");

	vdec = container_of(inode->i_cdev, vdec_entry, cdev);

	vdec_mutex_lock(&vdec->vdec_mutex);
	if (vdec->open_count >= MAX_OPEN_COUNT) {
		dprint(PRN_FATAL, "open vdec instance too much\n");
		ret = -EBUSY;
		goto exit;
	}
	vdec->open_count++;
	if (vdec->open_count == 1) {
		ret = vdec_open_global_resource(vdec);
		if (ret != 0) {
			vdec->open_count--;
			goto exit;
		}
	}
	ret = vdec_drv_acquire_file_handle(&vdec_file);
	if (ret != 0) {
		vdec->open_count--;
		goto exit;
	}
	vdec_file->vdec_entry = vdec;
	/* init perf file handle */
	for (index = 0; index < MAX_OPEN_COUNT; index++) {
		if (!g_perf_info[index].file) {
			g_perf_info[index].file = file_handle;
			break;
		}
	}
	file_handle->private_data = vdec_file;
	dprint(PRN_ALWS, "open_count: %u\n", vdec->open_count);

exit:
	vdec_mutex_unlock(&vdec->vdec_mutex);

	return ret;
}

void vdec_update_setting_info(struct vdec_setting_info *info)
{
	vdec_plat *plt_info = vdec_plat_get_entry();
	if (plt_info->dts_info.support_power_control_per_frame == 0)
		info->enable_power_off_when_vdec_idle = false;
}

static void vdec_close_global_resource(vdec_entry *vdec)
{
	if (!vdec->setting_info.smmu_glb_bypass) {
		if (vdec->power_state)
			smmu_deinit();
		smmu_unmap_reg();
	}
	vfmw_deinit();

	if (vdec->power_state) {
		unsigned long flag;
#ifdef VDEC_DPM_ENABLE
		vdec_dpm_deinit();
#endif
		spin_lock_irqsave(&vdec->power_state_spin_lock, flag);
		vdec->power_state = false;
		spin_unlock_irqrestore(&vdec->power_state_spin_lock, flag);

		vdec_plat_regulator_disable();
	}

	if (vdec->device_locked) {
		vdec_mutex_unlock(&vdec->vdec_mutex_sec_vdh);
		vdec_mutex_unlock(&vdec->vdec_mutex_sec_scd);
		vdec->device_locked = VCODEC_FALSE;
	}
#ifdef ENABLE_VDEC_PROC
	vdec_init_setting_info(&g_vdec_entry.setting_info);
	vdec_update_setting_info(&g_vdec_entry.setting_info);
#endif
	g_is_normal_init = 0;
	vdec->power_ref_cnt = 0;
	g_vdec_entry.setting_info.ion_alloc_total_size = 0;
	(void)memset_s(&g_vdec_entry.chan_mode, sizeof(vdec_chan_mode),
		0, sizeof(vdec_chan_mode));
	(void)memset_s(&g_vdec_entry.vdec_power, sizeof(vdec_power_info),
		0, sizeof(vdec_power_info));

	return;
}

static int32_t vdec_close(struct inode *inode, struct file *file_handle)
{
	vdec_entry *vdec = VCODEC_NULL;
	struct dec_file_handle *vdec_file = VCODEC_NULL;
	uint64_t *power_off_duration = &g_vdec_entry.vdec_power.power_off_duration_sum;
	uint64_t *power_off_times = &g_vdec_entry.vdec_power.power_off_on_times;
	int32_t index;

	check_return(inode, "inode is null");
	check_return(file_handle, "file_handle is null");
	check_return(file_handle->private_data, "private_data is null");

	vdec_file = file_handle->private_data;
	vdec = vdec_file->vdec_entry;
	check_return(vdec, "vdec is null");

	vdec_mutex_lock(&vdec->vdec_mutex);
	if (vdec->open_count > 0)
		vdec->open_count--;

	index = vdec_get_file_index(file_handle);
	if (index != INVALID_IDX)
		(void)memset_s(&g_perf_info[index], sizeof(g_perf_info[index]),
			0, sizeof(g_perf_info[index]));

	vdec_drv_release_file_handle(vdec_file);
	vdec_disable_sec_mode(file_handle);
	vfmw_init_seg_buffer_addr(file_handle);
	file_handle->private_data = VCODEC_NULL;

	if (vdec->open_count == 0)
		vdec_close_global_resource(vdec);
	else if (g_vdec_entry.setting_info.enable_power_off_when_vdec_idle)
		vdec_print_power_statistics(power_off_duration, power_off_times);

	dprint(PRN_ALWS, "close vdec success, open_count: %u\n", vdec->open_count);
	vdec_mutex_unlock(&vdec->vdec_mutex);

	return 0;
}

static int32_t vdec_compat_get_data(compat_type_e eType, void __user *pUser,
	void *pData)
{
	int32_t ret = 0;
	int32_t get_user_data_ret = 0;
	int32_t s32Data = 0;
	compat_ulong_t CompatData = 0;
	vdec_ioctl_msg *pIoctlMsg = (vdec_ioctl_msg *)pData;

	if (!pUser || !pData) {
		dprint(PRN_FATAL, "param is null\n");
		return VCODEC_FAILURE;
	}

	switch (eType) {
	case T_IOCTL_ARG:
		if (copy_from_user(pIoctlMsg, pUser, sizeof(*pIoctlMsg))) {
			dprint(PRN_FATAL, "puser copy failed\n");
			ret = VCODEC_FAILURE;
		}
		break;
	case T_IOCTL_ARG_COMPAT: {
		compat_ioctl_msg __user *pCompatMsg = pUser;

		get_user_data_ret |= get_user(s32Data, &pCompatMsg->chan_num);
		pIoctlMsg->chan_num = s32Data;

		get_user_data_ret |= get_user(s32Data, &pCompatMsg->in_size);
		pIoctlMsg->in_size = s32Data;

		get_user_data_ret |= get_user(s32Data, &pCompatMsg->out_size);
		pIoctlMsg->out_size = s32Data;

		get_user_data_ret |= get_user(CompatData, &pCompatMsg->in);
		pIoctlMsg->in = (void *)(uintptr_t)((vcodec_virt_addr_t)CompatData);

		get_user_data_ret |= get_user(CompatData, &pCompatMsg->out);
		pIoctlMsg->out = (void *)(uintptr_t)((vcodec_virt_addr_t)CompatData);

		ret = (get_user_data_ret != 0) ? VCODEC_FAILURE : 0;
	}
		break;
	default:
		dprint(PRN_FATAL, "unknown type %d\n", eType);
		ret = VCODEC_FAILURE;
		break;
	}

	return ret;
}

static int32_t vdec_ioctl_set_clk_rate(struct file *file_handle,
	vdec_ioctl_msg *pvdec_msg)
{
	clk_rate_e clk_rate = VDEC_CLK_RATE_MAX;
	clk_rate_e target_clk_rate;
	performance_params_s performance_params;
	int32_t index;

	check_return(pvdec_msg->in != VCODEC_NULL,
		"VDEC_IOCTL_SET_CLK_RATE, invalid input prarameter");
	check_para_size_return(sizeof(performance_params), pvdec_msg->in_size,
		"VDEC_IOCTL_SET_CLK_RATE");
	if (copy_from_user(&performance_params, pvdec_msg->in, sizeof(performance_params))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_SET_CLK_RATE: copy_from_user failed\n");
		return VCODEC_FAILURE;
	}

	vdec_mutex_lock(&g_vdec_entry.vdec_mutex);
	index = vdec_get_file_index(file_handle);
	if (index == INVALID_IDX) {
		dprint(PRN_FATAL, "%s file handle is wrong\n", __func__);
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex);
		return VCODEC_FAILURE;
	}
	g_perf_info[index].load = performance_params.load;
	g_perf_info[index].base_rate =  performance_params.base_rate;
	clk_rate = get_clk_rate(performance_params);
	target_clk_rate = clk_rate;
	vdec_plat_get_target_clk_rate(&target_clk_rate);
	if (vdec_plat_regulator_set_clk_rate(target_clk_rate) != 0) {
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex);
		return VCODEC_FAILURE;
	}
	vdec_plat_set_static_clk_rate(clk_rate);
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex);

	return 0;
}

static int32_t handle_scd_proc(struct file *file_handle,
	vdec_ioctl_msg *pvdec_msg)
{
	scd_reg scd_state_reg;
	scd_reg_ioctl scd_reg_cfg;
	int32_t ret;

	check_return(pvdec_msg->in != VCODEC_NULL,
		"VDEC_IOCTL_SCD_PROC, invalid input prarameter");
	check_return(pvdec_msg->out != VCODEC_NULL,
		"VDEC_IOCTL_SCD_PROC, invalid output prarameter");
	check_para_size_return(sizeof(scd_reg_cfg), pvdec_msg->in_size,
		"VDEC_IOCTL_SCD_WRITE_REG_IN");
	check_para_size_return(sizeof(scd_state_reg), pvdec_msg->out_size,
		"VDEC_IOCTL_SCD_WRITE_REG_OUT");

	if (copy_from_user(&scd_reg_cfg, pvdec_msg->in, sizeof(scd_reg_cfg))) {
		dprint(PRN_FATAL, "copy_from_user failed\n");
		return -EFAULT;
	}

	ret = vdec_mutex_lock_interruptible(&g_vdec_entry.vdec_mutex_sec_scd);
	if (ret != 0) {
		scd_state_reg.ret_errno = ret;
		dprint(PRN_WARN, "lock scd failed %d\n", ret);
		goto exit;
	}
	vdec_mutex_lock(&g_vdec_entry.vdec_mutex_scd);
	ret = vfmw_control(file_handle, VFMW_CID_STM_PROCESS,
		&scd_reg_cfg, &scd_state_reg);
	if (ret != 0) {
		dprint(PRN_FATAL, "vctrl_scd_hal_process failed\n");
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_scd);
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_sec_scd);
		return -EFAULT;
	}

	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_scd);
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_sec_scd);
exit:
	if (copy_to_user(pvdec_msg->out, &scd_state_reg, sizeof(scd_state_reg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_SCD_WRITE_REG: copy_to_user failed\n");
		return -EFAULT;
	}

	return ret;
}

static int32_t handle_vdm_enable(struct file *file_handle,
	vdec_ioctl_msg *pvdec_msg)
{
	dec_dev_cfg dev_cfg;
	int32_t ret;

	check_return(pvdec_msg->out != VCODEC_NULL,
		"VDEC_IOCTL_GET_ACTIVE_REG, invalid output prarameter");
	check_para_size_return(sizeof(dev_cfg), pvdec_msg->out_size,
		"invalid VDEC_IOCTL_GET_ACTIVE_REG out param");

	ret = vdec_mutex_lock_interruptible(&g_vdec_entry.vdec_mutex_sec_vdh);
	if (ret != 0) {
		dev_cfg.ret_errno = ret;
		dprint(PRN_WARN, "lock vdh failed %d\n", ret);
		goto exit;
	}

	vdec_mutex_lock(&g_vdec_entry.vdec_mutex_vdh);
	if (vfmw_control(VCODEC_NULL, VFMW_CID_GET_ACTIVE_REG, &dev_cfg, VCODEC_NULL) != 0) {
		dprint(PRN_FATAL,
			"VDEC_IOCTL_GET_ACTIVE_REG: get current active reg failed\n");
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_vdh);
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_sec_vdh);
		return -EFAULT;
	}
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_vdh);
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_sec_vdh);
exit:
	if (copy_to_user(pvdec_msg->out, &dev_cfg, sizeof(dev_cfg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_GET_ACTIVE_REG: copy_to_user failed\n");
		return -EFAULT;
	}

	return ret;
}

static int32_t handle_vdm_dec_proc(struct file *file_handle, dec_dev_cfg *dev_cfg, dec_back_up *back_up)
{
	int32_t ret;

	ret = vdec_mutex_lock_interruptible(&g_vdec_entry.vdec_mutex_sec_vdh);
	if (ret != 0) {
		dprint(PRN_WARN, "lock vdh failed %d\n", ret);
		return ret;
	}

	vdec_mutex_lock(&g_vdec_entry.vdec_mutex_vdh);
	ret = vfmw_control(file_handle, VFMW_CID_DEC_PROCESS, dev_cfg, VCODEC_NULL);
	if (ret != 0) {
		dprint(PRN_FATAL, "start decode failed, ret %d\n", ret);
		goto exit;
	}

	if (dev_cfg->dev_cfg_info.work_mode == SYNC_MODE) {
		ret = vfmw_query_image(file_handle, dev_cfg, back_up);
		if (ret != 0) {
			dprint(PRN_FATAL, "vfmw_query_image failed, ret %d\n", ret);
			goto exit;
		}
	}

exit:
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_vdh);
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_sec_vdh);

	if (ret != 0)
		ret = -EFAULT;
	return ret;
}

static int32_t handle_vdm_proc(struct file *file_handle, vdec_ioctl_msg *pvdec_msg)
{
	dec_dev_cfg dev_cfg;
	dec_back_up back_up = {0};
	int32_t ret = -EFAULT;

	check_return(pvdec_msg->in != VCODEC_NULL,
		"VDEC_IOCTL_DEC_START_IN, invalid input prarameter");
	check_return(pvdec_msg->out != VCODEC_NULL,
		"VDEC_IOCTL_DEC_START_OUT, invalid output prarameter");
	check_para_size_return(sizeof(dev_cfg), pvdec_msg->in_size,
		"VDEC_IOCTL_DEC_START_IN");
	check_para_size_return(sizeof(back_up), pvdec_msg->out_size,
		"VDEC_IOCTL_DEC_START_OUT");
	check_return(file_handle, "file_handle null, invalid input prarameter");

	if (copy_from_user(&dev_cfg, pvdec_msg->in, sizeof(dev_cfg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_DEC_START: copy_from_user failed\n");
		return ret;
	}

	ret = handle_vdm_dec_proc(file_handle, &dev_cfg, &back_up);
	if (ret != 0)
		back_up.ret_errno = ret;

	if (copy_to_user(pvdec_msg->out, &back_up, sizeof(back_up)))
		dprint(PRN_FATAL, "VDEC_IOCTL_DEC_START : copy_to_user failed\n");

	return ret;
}

static int32_t vdec_ioctl_dec_irq_query(struct file *file_handle, vdec_ioctl_msg *pvdec_msg)
{
	dec_dev_cfg dev_reg_cfg;
	dec_back_up back_up;
	check_return(pvdec_msg->in != VCODEC_NULL, "VDEC_IOCTL_DEC_IRQ_QUERY_IN, invalid input parameter");
	check_return(pvdec_msg->out != VCODEC_NULL, "VDEC_IOCTL_DEC_IRQ_QUERY_OUT, invalid output parameter");
	check_para_size_return(sizeof(dev_reg_cfg), pvdec_msg->in_size, "VDEC_IOCTL_DEC_IRQ_QUERY__IN");
	check_para_size_return(sizeof(back_up), pvdec_msg->out_size, "VDEC_IOCTL_DEC_IRQ_QUERY_OUT");
	check_return(file_handle, "file_handle null, invalid input prarameter");

	if (copy_from_user(&dev_reg_cfg, pvdec_msg->in, sizeof(dev_reg_cfg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_DEC_IRQ_QUERY : copy_from_user failed\n");
		return -EFAULT;
	}

	if (vfmw_query_image(file_handle, &dev_reg_cfg, &back_up) != 0) {
		dprint(PRN_FATAL, "VDEC_IOCTL_DEC_IRQ_QUERY : vfmw_query_image failed\n");
		return -EFAULT;
	}

	if (copy_to_user(pvdec_msg->out, &back_up, sizeof(back_up))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_DEC_IRQ_QUERY : copy_to_user failed\n");
		return -EFAULT;
	}

	return 0;
}

static int32_t handle_vdm_cancel_dec_proc(struct file *file, vdec_ioctl_msg *pvdec_msg)
{
	dec_dev_cfg dev_cfg;

	check_return(pvdec_msg->in, "vdec cancel decode, invalid input prarameter");
	check_para_size_return(sizeof(dev_cfg), pvdec_msg->in_size, "vdec cancel decode");
	if (copy_from_user(&dev_cfg, pvdec_msg->in, sizeof(dev_cfg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_CANCEL_DECODE: copy_from_user failed\n");
		return -EFAULT;
	}

	vdec_mutex_lock(&g_vdec_entry.vdec_mutex_vdh);
	if (vfmw_control(file, VFMW_CID_CANCEL_PROCESS, &dev_cfg, VCODEC_NULL) != 0) {
		dprint(PRN_FATAL, "VDEC_IOCTL_CANCEL_DECODE: cancel decode failed\n");
		vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_vdh);
		return -EFAULT;
	}
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex_vdh);

	return 0;
}

static int32_t handle_iommu_map(struct file *file_handle,
	vdec_ioctl_msg *vdec_msg)
{
	int32_t ret;
	unsigned long size;
	vdec_buffer_record buf_record;
	vdec_entry *vdec = vdec_get_entry_by_file_handle(file_handle);

	check_return(vdec, "vdec is null");
	check_return(vdec_msg->in != VCODEC_NULL,
		"VDEC_IOCTL_IOMMU_MAP, invalid input prarameter");
	check_return(vdec_msg->out != VCODEC_NULL,
		"VDEC_IOCTL_IOMMU_MAP, invalid output prarameter");
	check_para_size_return(sizeof(buf_record), vdec_msg->in_size,
		"VDEC_IOCTL_IOMMU_MAP_IN");
	check_para_size_return(sizeof(buf_record), vdec_msg->out_size,
		"VDEC_IOCTL_IOMMU_MAP_OUT");

	if (copy_from_user(&buf_record, vdec_msg->in, sizeof(buf_record))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_MAP: copy_from_user failed\n");
		return -EFAULT;
	}

	ret = vdec_mem_iommu_map((void *)vdec->device, buf_record.share_fd,
		&buf_record.iova, &size);
	if (ret != 0) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_MAP failed\n");
		return -EFAULT;
	}

	if (copy_to_user(vdec_msg->out, &buf_record, sizeof(buf_record))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_MAP: copy_to_user failed\n");
		(void)vdec_mem_iommu_unmap((void *)vdec->device, buf_record.share_fd,
			buf_record.iova);
		return -EFAULT;
	}

	return ret;
}

static int32_t handle_iommu_unmap(struct file *file_handle,
	vdec_ioctl_msg *vdec_msg)
{
	int32_t ret;
	vdec_buffer_record buf_record;
	vdec_entry *vdec = vdec_get_entry_by_file_handle(file_handle);

	check_return(vdec, "vdec is null");
	check_return(vdec_msg->in != VCODEC_NULL,
		"VDEC_IOCTL_IOMMU_UNMAP, invalid input prarameter");
	check_para_size_return(sizeof(buf_record), vdec_msg->in_size,
		"VDEC_IOCTL_IOMMU_UNMAP_IN");

	if (copy_from_user(&buf_record,
		vdec_msg->in, sizeof(buf_record))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_UNMAP: copy_from_user failed\n");
		return -EFAULT;
	}

	ret = vdec_mem_iommu_unmap((void *)vdec->device, buf_record.share_fd,
		buf_record.iova);
	if (ret != 0) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_UNMAP failed\n");
		return -EFAULT;
	}

	return ret;
}

static int32_t handle_lock_hw(struct file *file_handle, vdec_ioctl_msg *vdec_msg)
{
	vcodec_bool scene_ident_res;
	vdec_entry *omxvdec = vdec_get_entry_by_file_handle(file_handle);

	check_return(omxvdec, "vdec is null");
	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_scd);
	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_vdh);

	scene_ident_res = vfmw_scene_ident();
	if (scene_ident_res == VCODEC_FALSE) {
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
		dprint(PRN_ALWS, "%s : lock hw error\n", __func__);
		return -EIO;
	}

	if (omxvdec->device_locked)
		dprint(PRN_ALWS, "hw have locked\n");
	omxvdec->device_locked = VCODEC_TRUE;

	dprint(PRN_DBG, "out\n");

	return 0;
}

static int32_t handle_unlock_hw(struct file *file_handle, vdec_ioctl_msg *vdec_msg)
{
	vcodec_bool scene_ident_res;
	vdec_entry *omxvdec = vdec_get_entry_by_file_handle(file_handle);

	check_return(omxvdec, "vdec is null");
	scene_ident_res = vfmw_scene_ident();
	if (scene_ident_res == VCODEC_FALSE) {
		dprint(PRN_ALWS, "%s : unlock hw erro\n", __func__);
		return -EIO;
	}

	if (omxvdec->device_locked) {
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
		omxvdec->device_locked = VCODEC_FALSE;
	}

	dprint(PRN_DBG, "out\n");
	return 0;
}

static void vdec_suspend_process(bool sec_device_online)
{
	vdec_plat *plt = vdec_plat_get_entry();
	unsigned long flag;

	vfmw_suspend(sec_device_online);
	smmu_deinit();
#ifdef VDEC_DPM_ENABLE
	vdec_dpm_deinit();
#endif

	spin_lock_irqsave(&g_vdec_entry.power_state_spin_lock, flag);
	g_vdec_entry.power_state = false;
	spin_unlock_irqrestore(&g_vdec_entry.power_state_spin_lock, flag);

	vdec_plat_regulator_disable();
	plt->clk_ctrl.clk_flag = 0;
}

static int32_t vdec_resume_process(bool sec_device_online)
{
	int32_t ret;
	clk_rate_e clk_rate = VDEC_CLK_RATE_MAX;
	vdec_plat *plt = vdec_plat_get_entry();
	unsigned long flag;

	vdec_plat_get_target_clk_rate(&clk_rate);
	ret = vdec_plat_regulator_enable();
	if (ret != 0) {
		dprint(PRN_FATAL, "enable regulator failed\n");
		return VCODEC_FAILURE;
	}

#ifdef VDEC_DPM_ENABLE
	vdec_dpm_init();
#endif
	vdec_plat_regulator_set_clk_rate(clk_rate);

	if (smmu_init() != 0)
		return VCODEC_FAILURE;

	vfmw_resume(sec_device_online);

	spin_lock_irqsave(&g_vdec_entry.power_state_spin_lock, flag);
	g_vdec_entry.power_state = true;
	spin_unlock_irqrestore(&g_vdec_entry.power_state_spin_lock, flag);

	plt->clk_ctrl.clk_flag = 1;

	return 0;
}

static int32_t handle_enable_sec_mode(struct file *file_handle, vdec_ioctl_msg *vdec_msg)
{
	int32_t ret;
	vdec_entry *vdec = vdec_get_entry_by_file_handle(file_handle);
	check_return(vdec, "vdec is null");

	dprint(PRN_ALWS, "enable_sec_mode\n");

	vdec_mutex_lock(&vdec->vdec_mutex);
	if ((g_vdec_entry.chan_mode.sec_chan_num > 0) &&
		(g_is_normal_init != 0) && (g_vdec_entry.power_state)) {
		ret = vdec_enable_sec_mode(file_handle);
		vdec_mutex_unlock(&vdec->vdec_mutex);
		return ret;
	}

	if ((!vdec->power_state) && (g_is_normal_init != 0)) {
		if (vdec_resume_process(false) == VCODEC_FAILURE) {
			vdec_mutex_unlock(&vdec->vdec_mutex);
			return -EFAULT;
		}

		g_vdec_entry.vdec_power.power_off_timestamp_us = 0;
	}

	ret = vdec_enable_sec_mode(file_handle);
	vdec_mutex_unlock(&vdec->vdec_mutex);
	dprint(PRN_ALWS, "enable_sec_mode and power on successfully.\n");

	return ret;
}

void vdec_update_power_off_duration(uint32_t power_off_gap)
{
	uint8_t idx;
	uint32_t *gap_array = g_vdec_entry.vdec_power.power_off_duration;

	for (idx = 0; idx < POWER_OFF_GAP_NUM - 1; idx++)
		gap_array[idx] = gap_array[idx + 1];

	gap_array[idx] = power_off_gap;
}

vcodec_bool vdec_check_power_off_duration(void)
{
	uint32_t *gap_array = g_vdec_entry.vdec_power.power_off_duration;

	// if recent two power off duration less then POWER_OFF_THRESHOLD, do not power off,
	// return false: do not power off, return true: need power off
	if ((gap_array[POWER_OFF_GAP_NUM - 1] > POWER_OFF_THRESHOLD) &&
		(gap_array[POWER_OFF_GAP_NUM - 2] > POWER_OFF_THRESHOLD) &&
		(gap_array[POWER_OFF_GAP_NUM - 3] > POWER_OFF_THRESHOLD))
		return true;

	return false;
}

int32_t vdec_check_power_on(void)
{
	int32_t ret;
	uint64_t power_off_duration;
	vdec_entry *vdec = &g_vdec_entry;
	vdec_power_info *power_stats = &g_vdec_entry.vdec_power;
	clk_rate_e clk_rate = VDEC_CLK_RATE_MAX;

	vdec_mutex_lock(&vdec->vdec_mutex);
	vdec->power_ref_cnt++;

	if (vdec->power_ref_cnt > 1) {
		/* update clk rate */
		vdec_plat_get_target_clk_rate(&clk_rate);
		ret = vdec_plat_regulator_set_clk_rate(clk_rate);
		vdec_mutex_unlock(&vdec->vdec_mutex);
		return ret;
	}

	if (power_stats->power_off_timestamp_us != 0) {
		power_off_duration = get_time_in_us() - power_stats->power_off_timestamp_us;
		vdec_update_power_off_duration(power_off_duration);
	}

	if ((g_is_normal_init != 0) && (!vdec->power_state)) {
		if (vdec_resume_process(false) == VCODEC_FAILURE) {
			vdec->power_ref_cnt--;
			vdec_mutex_unlock(&vdec->vdec_mutex);
			return VCODEC_FAILURE;
		}

		power_stats->power_off_on_times++;
		power_off_duration = get_time_in_us() - power_stats->power_off_timestamp_us;
		power_stats->power_off_duration_sum += power_off_duration;
	}

	power_stats->power_off_timestamp_us = 0;

	vdec_mutex_unlock(&vdec->vdec_mutex);

	return 0;
}

int32_t vdec_check_power_off(unsigned int cmd)
{
	vdec_entry *vdec = &g_vdec_entry;

	vdec_mutex_lock(&vdec->vdec_mutex);
	vdec->power_ref_cnt--;
	if (vdec->power_ref_cnt > 0) {
		vdec_mutex_unlock(&vdec->vdec_mutex);
		return 0;
	}

	if (cmd == VDEC_IOCTL_LOCK_HW) {
		vdec_mutex_unlock(&vdec->vdec_mutex);
		return 0;
	}

	if ((g_is_normal_init != 0) && (vdec->power_state) &&
		(g_vdec_entry.chan_mode.sec_chan_num == 0)) {
		g_vdec_entry.vdec_power.power_off_timestamp_us = get_time_in_us();
		if (!vdec_check_power_off_duration()) {
			vdec_mutex_unlock(&vdec->vdec_mutex);
			return VCODEC_FAILURE;
		}
		vdec_suspend_process(false);
	}

	vdec_mutex_unlock(&vdec->vdec_mutex);

	return 0;
}

static const ioctl_command_node g_ioctl_command_table[] = {
	{ VDEC_IOCTL_SCD_PROC, handle_scd_proc },
	{ VDEC_IOCTL_IOMMU_MAP, handle_iommu_map },
	{ VDEC_IOCTL_IOMMU_UNMAP, handle_iommu_unmap },
	{ VDEC_IOCTL_GET_ACTIVE_REG, handle_vdm_enable },
	{ VDEC_IOCTL_VDM_PROC, handle_vdm_proc },
	{ VDEC_IOCTL_SET_CLK_RATE, vdec_ioctl_set_clk_rate },
	{ VDEC_IOCTL_LOCK_HW, handle_lock_hw },
	{ VDEC_IOCTL_UNLOCK_HW, handle_unlock_hw },
	{ VDEC_IOCTL_CANCEL_DECODE, handle_vdm_cancel_dec_proc},
	{ VDEC_IOCTL_DEC_IRQ_QUERY, vdec_ioctl_dec_irq_query },
	{ 0, VCODEC_NULL },
};

fn_ioctl_handler vdec_ioctl_get_handler(uint32_t cmd)
{
	uint32_t index;
	fn_ioctl_handler target_handler = VCODEC_NULL;
	uint32_t cmd_table_size = ARRAY_SIZE(g_ioctl_command_table);

	if (g_vdec_entry.setting_info.enable_power_off_when_vdec_idle) {
		if (cmd == VDEC_IOCTL_ENABLE_SEC_MODE)
			return handle_enable_sec_mode;
	}

	for (index = 0; index < cmd_table_size; index++) {
		if (cmd == g_ioctl_command_table[index].cmd) {
			target_handler = g_ioctl_command_table[index].handle;
			break;
		}
	}

	return target_handler;
}

static long vdec_ioctl_common(struct file *file_handle, unsigned int cmd,
	unsigned long arg, compat_type_e type)
{
	int32_t ret;
	long ioctl_ret;
	vdec_ioctl_msg vdec_msg;
	void *u_arg = (void *)(uintptr_t)arg;
	fn_ioctl_handler ioctl_handler = VCODEC_NULL;
	vdec_entry *vdec = vdec_get_entry_by_file_handle(file_handle);
	uint64_t timestamp;

	check_return(vdec, "vdec is null");

	(void)memset_s(&vdec_msg, sizeof(vdec_msg), 0, sizeof(vdec_msg));

	if ((cmd != VDEC_IOCTL_ENABLE_SEC_MODE) && (cmd != VDEC_IOCTL_UNLOCK_HW) && (cmd != VDEC_IOCTL_LOCK_HW)) {
		check_return(u_arg != VCODEC_NULL, "arg is null");
		ret = vdec_compat_get_data(type, u_arg, &vdec_msg);
		check_return(ret == 0, "compat data get failed");
	}

	ioctl_handler = vdec_ioctl_get_handler(cmd);
	if (ioctl_handler == VCODEC_NULL) {
		dprint(PRN_ERROR, "error cmd %d is not supported\n", _IOC_NR(cmd));
		return -ENOTTY;
	}

	if (vdec->setting_info.enable_power_off_when_vdec_idle) {
		timestamp = get_time_in_us();
		vfmw_check_return(vdec_check_power_on() == 0, -ENOTTY);
		vdec_record_power_operation_cost(timestamp, vdec_get_power_on_cost_record());
	}

	ioctl_ret = ioctl_handler(file_handle, &vdec_msg);

	if (vdec->setting_info.enable_power_off_when_vdec_idle) {
		timestamp = get_time_in_us();
		(void)vdec_check_power_off(cmd);
		vdec_record_power_operation_cost(timestamp, vdec_get_power_off_cost_record());
	}

	return ioctl_ret;
}

static long vdec_ioctl(struct file *file_handle, unsigned int cmd,
	unsigned long arg)
{
	return vdec_ioctl_common(file_handle, cmd, arg, T_IOCTL_ARG);
}

#ifdef CONFIG_COMPAT
static long vdec_compat_ioctl(struct file *file_handle, unsigned int cmd,
	unsigned long arg)
{
	void *user_ptr = compat_ptr(arg);

	return vdec_ioctl_common(file_handle, cmd, (unsigned long)(uintptr_t)user_ptr, T_IOCTL_ARG_COMPAT);
}
#endif

static const struct file_operations g_vdec_fops = {
	.owner = THIS_MODULE,
	.open = vdec_open,
	.unlocked_ioctl = vdec_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vdec_compat_ioctl,
#endif
	.release = vdec_close,
};

static int32_t vdec_probe(struct platform_device *pltdev)
{
	int32_t ret;

	if (vdec_device_probe(pltdev) != 0) {
		dprint(PRN_FATAL, "vdec device probe failed\n");
		return VCODEC_FAILURE;
	}

	ret = vdec_mem_probe(&pltdev->dev);
	if (ret != 0) {
		dprint(PRN_ERROR, "vdec memory probe failed\n");
		return VCODEC_FAILURE;
	}

	platform_set_drvdata(pltdev, VCODEC_NULL);
	(void)memset_s(&g_vdec_entry, sizeof(vdec_entry), 0, sizeof(vdec_entry));
	(void)memset_s(g_perf_info, sizeof(g_perf_info), 0, sizeof(g_perf_info));
	vdec_init_mutex(&g_vdec_entry.vdec_mutex);
	vdec_init_mutex(&g_vdec_entry.vdec_mutex_scd);
	vdec_init_mutex(&g_vdec_entry.vdec_mutex_vdh);
	vdec_init_mutex(&g_vdec_entry.vdec_mutex_sec_scd);
	vdec_init_mutex(&g_vdec_entry.vdec_mutex_sec_vdh);

	ret = vdec_setup_cdev(&g_vdec_entry, &g_vdec_fops);
	if (ret < 0) {
		dprint(PRN_FATAL, "vdec_setup_cdev failed, ret = %d\n", ret);
		goto cleanup0;
	}

	g_vdec_entry.device = &pltdev->dev;
	platform_set_drvdata(pltdev, &g_vdec_entry);
	ret = vdec_plat_init(pltdev);
	if (ret != 0) {
		dprint(PRN_FATAL, "vdec init failed, ret = %d\n", ret);
		goto cleanup1;
	}
	vdec_init_setting_info(&g_vdec_entry.setting_info);
	vdec_update_setting_info(&g_vdec_entry.setting_info);

	dprint(PRN_ALWS, "vdec probe success\n");

	return 0;
cleanup1:
	vdec_cleanup_cdev(&g_vdec_entry);
cleanup0:

	return VCODEC_FAILURE;
}

static int32_t vdec_remove(struct platform_device *pltdev)
{
	vdec_entry *vdec = VCODEC_NULL;

	vdec = platform_get_drvdata(pltdev);
	if (IS_ERR_OR_NULL(vdec)) {
		dprint(PRN_ERROR, "errno = %ld\n", PTR_ERR(vdec));
		return VCODEC_FAILURE;
	}

	vdec_cleanup_cdev(vdec);
	vdec_plat_deinit();
	platform_set_drvdata(pltdev, VCODEC_NULL);

	dprint(PRN_ALWS, "remove vdec success\n");
	return 0;
}

static int32_t vdec_suspend(struct platform_device *pltdev, pm_message_t state)
{
	vdec_mutex_lock(&g_vdec_entry.vdec_mutex);

	if ((g_is_normal_init != 0) && (g_vdec_entry.power_state))
		vdec_suspend_process(true);
	else
		dprint(PRN_ALWS, "vdec already power off\n");

	g_vdec_entry.power_ref_cnt = 0;
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex);

	dprint(PRN_ALWS, "-\n");

	return 0;
}

static int32_t vdec_resume(struct platform_device *pltdev)
{
	vdec_mutex_lock(&g_vdec_entry.vdec_mutex);
	if ((g_is_normal_init != 0) && (!g_vdec_entry.power_state)) {
		if (vdec_resume_process(true) == VCODEC_FAILURE) {
			vdec_mutex_unlock(&g_vdec_entry.vdec_mutex);
			dprint(PRN_ALWS, "failed.\n");
			return VCODEC_FAILURE;
		}
	}

	g_vdec_entry.power_ref_cnt = 0;
	vdec_mutex_unlock(&g_vdec_entry.vdec_mutex);

	dprint(PRN_ALWS, "+\n");

	return 0;
}

static const struct of_device_id g_vdec_dt_match[] = {
	{ .compatible = "hisilicon,VCodecV600-vdec", },
	{ .compatible = "hisilicon,VCodecV700-vdec", },
	{ .compatible = "hisilicon,VCodecV720-vdec", },
	{ }
};

MODULE_DEVICE_TABLE(of, g_vdec_dt_match);

static struct platform_driver g_vdec_driver = {
	.probe = vdec_probe,
	.remove = vdec_remove,
	.suspend = vdec_suspend,
	.resume = vdec_resume,
	.driver = {
		.name = (char *)g_vdec_drv_name,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_vdec_dt_match),
	},
};

module_platform_driver(g_vdec_driver)

MODULE_DESCRIPTION("vdec driver");
MODULE_LICENSE("GPL");
