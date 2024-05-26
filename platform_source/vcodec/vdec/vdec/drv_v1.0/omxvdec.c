/*
 * omxvdec.c
 *
 * This is vdec driver interface.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
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

#include "omxvdec.h"

#include <linux/clk.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/mm_iommu.h>
#include <linux/dma-mapping.h>
#include <linux/dma-iommu.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>

#include "linux_kernel_osal.h"
#include "scd_drv.h"
#include "vdm_drv.h"
#include "vfmw_intf.h"
/*lint -e774*/

#define INVALID_IDX             (-1)
#define CLK_RATE_INDEX_FOR_OMX  0
#define CLK_RATE_INDEX_FOR_HEIF 1

static int32_t g_is_normal_init;

static vcodec_bool       g_is_device_detected  = VCODEC_FALSE;
static struct class *g_omx_vdec_class     = VCODEC_NULL;
static const char g_omx_vdec_drv_name[] = OMXVDEC_NAME;
static dev_t         g_omx_vdec_dev_num;
static vdec_mem_info g_vdec_map_info[MAX_OPEN_COUNT];
static clk_rate_e g_clk_rate = 0;

omx_vdec_entry g_omx_vdec_entry;

// Modified for 64-bit platform
typedef enum {
	T_IOCTL_ARG,
	T_IOCTL_ARG_COMPAT,
	T_BUTT,
} compat_type_e;

enum {
	NORMAL_MODE,
	FPGA_MODE,
	FPGA_MODE_NO_CHECK_RTL,
};

#define check_para_size_return(size, para_size, command) \
	do { \
		if ((size) != (para_size)) { \
			dprint(PRN_FATAL, "%s: prarameter_size is error\n", command); \
			return - EINVAL; \
		} \
	} while (0)

#define check_return(cond, else_print) \
	do { \
		if (!(cond)) { \
			dprint(PRN_FATAL, "%s : %s\n", __func__, else_print); \
			return - EINVAL; \
		} \
	} while (0)

#define check_scene_eq_return(cond, else_print) \
	do { \
		if (cond) { \
			dprint(PRN_ALWS, "%s : %s\n", __func__, else_print); \
			return - EIO; \
		} \
	} while (0)

/* get fd index */
static int32_t vdec_get_file_index(const struct file *file)
{
	int32_t index;

	for (index = 0; index < MAX_OPEN_COUNT; index++) {
		if (file == g_vdec_map_info[index].file)
			return index;
	}

	return INVALID_IDX;
}

static clk_rate_e get_clk_rate(struct performance_params_s param)
{
	int32_t i;
	clk_rate_e max_base_rate = (clk_rate_e)0;
	clk_rate_e clk_rate = (clk_rate_e)0;
	uint64_t total_load = 0;

	for (i = 0; i < MAX_OPEN_COUNT; i++) {
		if (g_vdec_map_info[i].file) {
			max_base_rate = g_vdec_map_info[i].base_rate > max_base_rate ?
				g_vdec_map_info[i].base_rate : max_base_rate;
			if (ULLONG_MAX - total_load >= g_vdec_map_info[i].load)
				total_load += g_vdec_map_info[i].load;
		}
	}

	for (i = VDEC_CLK_RATE_MAX - 1; i >= 0; i--) {
		if (total_load > param.load_range_map[i].lower_limit &&
			total_load <= param.load_range_map[i].upper_limit) {
			clk_rate = param.load_range_map[i].clk_rate;
			break;
		}
	}
	dprint(PRN_ALWS, "%s total_load %llu, clk_rate %d, max_base_rate %d\n",
		__func__, total_load, clk_rate, max_base_rate);
	clk_rate = max_base_rate > clk_rate ? max_base_rate : clk_rate;

	return clk_rate;
}

static ssize_t omxvdec_show(
	struct device *dev, struct device_attribute *attr,
	char *buf)
{
#ifdef USER_ENABLE_VDEC_PROC
	if (buf == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s buf is null\n", __func__);
		return 0;
	}
	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "0x%pK\n", (void *)(uintptr_t)g_smmu_page_base);
#else
	return 0;
#endif
}

static DEVICE_ATTR(omxvdec_misc, 0444, omxvdec_show, NULL);

static int32_t omxvdec_setup_cdev(
	omx_vdec_entry *omxvdec,
	const struct file_operations *fops)
{
	int32_t rc;
	struct device *dev = NULL;

	g_omx_vdec_class = class_create(THIS_MODULE, "omxvdec_class");
	if (IS_ERR(g_omx_vdec_class)) {
		rc = PTR_ERR(g_omx_vdec_class);
		g_omx_vdec_class = VCODEC_NULL;
		dprint(PRN_FATAL, "%s call class_create failed, rc : %d\n", __func__, rc);
		return rc;
	}

	rc = alloc_chrdev_region(&g_omx_vdec_dev_num, 0, 1, "vcodec_decoder");
	if (rc) {
		dprint(PRN_FATAL, "%s call alloc_chrdev_region failed, rc : %d\n", __func__, rc);
		goto cls_destroy;
	}

	dev = device_create(g_omx_vdec_class, NULL, g_omx_vdec_dev_num, NULL, OMXVDEC_NAME);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		dprint(PRN_FATAL, "%s call device_create failed, rc : %d\n", __func__, rc);
		goto unregister_region;
	}

	rc = device_create_file(dev, &dev_attr_omxvdec_misc);
	if (rc) {
		dprint(PRN_FATAL, "%s, create device file failed\n", __func__);
		goto dev_destroy;
	}

	cdev_init(&omxvdec->cdev, fops);
	omxvdec->cdev.owner = THIS_MODULE;
	omxvdec->cdev.ops = fops;
	rc = cdev_add(&omxvdec->cdev, g_omx_vdec_dev_num, 1);
	if (rc < 0) {
		dprint(PRN_FATAL, "%s call cdev_add failed, rc : %d\n", __func__, rc);
		goto file_close;
	}

	return 0;

file_close:
	device_remove_file(dev, &dev_attr_omxvdec_misc);
dev_destroy:
	device_destroy(g_omx_vdec_class, g_omx_vdec_dev_num);
unregister_region:
	unregister_chrdev_region(g_omx_vdec_dev_num, 1);
cls_destroy:
	class_destroy(g_omx_vdec_class);
	g_omx_vdec_class = VCODEC_NULL;

	return rc;
}

static int32_t omxvdec_cleanup_cdev(omx_vdec_entry *omxvdec)
{
	if (!g_omx_vdec_class) {
		dprint(PRN_FATAL, "%s: Invalid g_omx_vdec_class is NULL", __func__);
		return VCODEC_FAILURE;
	}

	device_remove_file(omxvdec->device, &dev_attr_omxvdec_misc);
	cdev_del(&omxvdec->cdev);
	device_destroy(g_omx_vdec_class, g_omx_vdec_dev_num);
	unregister_chrdev_region(g_omx_vdec_dev_num, 1);
	class_destroy(g_omx_vdec_class);
	g_omx_vdec_class = VCODEC_NULL;

	return 0;
}

static int32_t omxvdec_open(struct inode *inode, struct file *file)
{
	int32_t ret;
	omx_vdec_entry *omxvdec = VCODEC_NULL;
	int32_t index;

	omxvdec = container_of(inode->i_cdev, omx_vdec_entry, cdev);

	vdec_mutex_lock(&omxvdec->omxvdec_mutex);
	vdec_mutex_lock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_lock(&omxvdec->vdec_mutex_vdh);

	if (omxvdec->open_count < MAX_OPEN_COUNT) {
		omxvdec->open_count++;
		if (omxvdec->open_count == 1) {
			ret = vdec_regulator_enable();
			if (ret != 0) {
				dprint(PRN_FATAL, "%s : vdec_regulator_enable failed\n", __func__);
				goto error0;
			}
			ret = vctrl_open_vfmw();
			if (ret != 0) {
				dprint(PRN_FATAL, "%s : vfmw open failed\n", __func__);
				goto error1;
			}
			g_is_normal_init = 1;
		}
		/* save file */
		for (index = 0; index < MAX_OPEN_COUNT; index++) {
			if (g_vdec_map_info[index].file == VCODEC_NULL)
				break;
		}
		if (index == MAX_OPEN_COUNT) {
			dprint(PRN_FATAL, "%s : in Map Info, open too much\n", __func__);
			ret = -EBUSY;
			goto exit;
		}
		g_vdec_map_info[index].file = file;

		file->private_data = omxvdec;
		ret = 0;
	} else {
		dprint(PRN_FATAL, "%s open omxvdec instance too much\n", __func__);
		ret = -EBUSY;
	}

	dprint(PRN_ALWS, "%s, open_count : %d\n", __func__, omxvdec->open_count);
	goto exit;

error1:
	(void)vdec_regulator_disable();
error0:
	omxvdec->open_count--;
exit:
	vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
	vdec_mutex_unlock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_unlock(&omxvdec->omxvdec_mutex);

	return ret;
}

static vcodec_bool omxvdec_is_heif_scene(vdec_mem_info *mem_info)
{
	mem_buffer_s *vdh_mem = NULL;

	if (mem_info == NULL)
		return VCODEC_FALSE;

	vdh_mem = mem_info->vdh;
	if (vdh_mem->scene == SCENE_HEIF)
		return VCODEC_TRUE;

	return VCODEC_FALSE;
}

static int32_t handle_omxvdec_open_count(omx_vdec_entry *omxvdec)
{
	int32_t ret = 0;

	if (omxvdec->open_count > 0)
		omxvdec->open_count--;

	if (omxvdec->open_count > 0) {
		ret = vdec_regulator_set_clk_rate(g_clk_rate);
		if (ret != 0)
			dprint(PRN_FATAL, "vdec set clk rate faied\n");
	} else if (omxvdec->open_count == 0) {
		g_clk_rate = 0;

		vctrl_close_vfmw();
		vdec_regulator_disable();
		if (omxvdec->device_locked) {
			vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
			vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
			omxvdec->device_locked = VCODEC_FALSE;
		}
		g_is_normal_init = 0;   //lint !e456
	}

	return ret;
}

static int32_t omxvdec_release(struct inode *inode, struct file *file)
{
	omx_vdec_entry *omxvdec = VCODEC_NULL;
	int32_t ret = 0;
	int32_t index;

	omxvdec = file->private_data;
	if (omxvdec == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s: invalid omxvdec is null\n", __func__);
		return -EFAULT;
	}

	vdec_mutex_lock(&omxvdec->omxvdec_mutex);
	vdec_mutex_lock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_lock(&omxvdec->vdec_mutex_vdh);

	/* release file info */
	index = vdec_get_file_index(file);
	if (index == INVALID_IDX) {
		dprint(PRN_FATAL, "%s file is not open\n", __func__);
		ret = -EFAULT;
		goto exit;
	}

#ifdef MSG_POOL_ADDR_CHECK
	if (omxvdec_is_heif_scene(&g_vdec_map_info[index])) {
		ret = memset_s(&(g_vdec_map_info[index].vdh[VDH_SHAREFD_MESSAGE_POOL]),
			sizeof(g_vdec_map_info[index].vdh[VDH_SHAREFD_MESSAGE_POOL]), 0,
			sizeof(g_vdec_map_info[index].vdh[VDH_SHAREFD_MESSAGE_POOL]));
		if (ret != EOK) {
			dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
			ret = -EFAULT;
			goto exit;
		}
	}
#endif

	ret = memset_s(&g_vdec_map_info[index], sizeof(g_vdec_map_info[index]),
		0, sizeof(g_vdec_map_info[index]));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
		ret = -EFAULT;
		goto exit;
	}

	ret = handle_omxvdec_open_count(omxvdec);
	if (ret != 0)
		goto exit;

	file->private_data = VCODEC_NULL;

exit:
	dprint(PRN_ALWS, "exit %s, open_count: %d\n", __func__, omxvdec->open_count);

	vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
	vdec_mutex_unlock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_unlock(&omxvdec->omxvdec_mutex);

	return ret;
}

/* Modified for 64-bit platform */
static int32_t omxvdec_compat_get_data(
	compat_type_e eType, void __user *pUser, void *pData)
{
	int32_t ret = 0;
	int32_t s32Data = 0;
	compat_ulong_t CompatData = 0;
	vdec_ioctl_msg *pIoctlMsg = (vdec_ioctl_msg *)pData;

	if (pUser == VCODEC_NULL || pData == VCODEC_NULL) {
		dprint(PRN_FATAL, "%s: param is null\n", __func__);
		return VCODEC_FAILURE;
	}

	switch (eType) {
	case T_IOCTL_ARG:
		if (copy_from_user(pIoctlMsg, pUser, sizeof(*pIoctlMsg))) {
			dprint(PRN_FATAL, "%s: puser copy failed\n", __func__);
			ret = VCODEC_FAILURE;
		}
		break;
	case T_IOCTL_ARG_COMPAT: {
		compat_ioctl_msg __user *pCompatMsg = pUser;

		ret += get_user(s32Data, &(pCompatMsg->chan_num));
		pIoctlMsg->chan_num = s32Data;

		ret += get_user(s32Data, &(pCompatMsg->in_size));
		pIoctlMsg->in_size = s32Data;

		ret += get_user(s32Data, &(pCompatMsg->out_size));
		pIoctlMsg->out_size = s32Data;

		ret += get_user(CompatData, &(pCompatMsg->in));
		pIoctlMsg->in = (void *)(uintptr_t)CompatData;

		ret += get_user(CompatData, &(pCompatMsg->out));
		pIoctlMsg->out = (void *)(uintptr_t)CompatData;

		ret = (ret == 0) ? 0 : VCODEC_FAILURE;
	}
		break;
	default:
		dprint(PRN_FATAL, "%s: unkown type %d\n", __func__, eType);
		ret = VCODEC_FAILURE;
		break;
	}

	return ret;
}

static long handle_set_clk_rate(
	vdec_ioctl_msg vdec_msg, omx_vdec_entry *omxvdec,
	struct file *file)
{
	int32_t ret;
	struct performance_params_s performance_params;
	int32_t index;

	check_return(vdec_msg.in != VCODEC_NULL, "VDEC_IOCTL_SET_CLK_RATE, invalid input prarameter");
	check_para_size_return(sizeof(performance_params), (uint32_t)vdec_msg.in_size, "VDEC_IOCTL_SET_CLK_RATE");
	if (copy_from_user(&performance_params, vdec_msg.in, sizeof(performance_params))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_SET_CLK_RATE : copy_from_user failed\n");
		return -EFAULT;
	}

	vdec_mutex_lock(&omxvdec->omxvdec_mutex);
	vdec_mutex_lock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_lock(&omxvdec->vdec_mutex_vdh);

	index = vdec_get_file_index(file);
	if (index == INVALID_IDX) {
		dprint(PRN_FATAL, "%s file is wrong\n", __func__);
		ret = -EIO;
		goto exit;
	}
	g_vdec_map_info[index].load = performance_params.load;
	g_vdec_map_info[index].base_rate =  performance_params.base_rate;

	g_clk_rate = get_clk_rate(performance_params);

	ret = vdec_regulator_set_clk_rate(g_clk_rate);
	if (ret != 0)
		dprint(PRN_FATAL, "vdec_regulator_set_clk_rate faied\n");

exit:
	vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
	vdec_mutex_unlock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_unlock(&omxvdec->omxvdec_mutex);

	return ret;
}

static long handle_vdm_proc(
	vdec_ioctl_msg vdec_msg, omx_vdec_entry *omxvdec,
	const struct file *file)
{
	int32_t ret;
	int32_t fd_index;
	vdmhal_backup_s vdm_state_reg;
	omxvdh_reg_cfg_s vdm_reg_cfg;

	ret = memset_s(&vdm_state_reg, sizeof(vdm_state_reg), 0, sizeof(vdm_state_reg));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
		return -EFAULT;
	}
	check_return(vdec_msg.in != VCODEC_NULL,
		"VDEC_IOCTL_VDM_PROC, invalid input prarameter");
	check_return(vdec_msg.out != VCODEC_NULL,
		"VDEC_IOCTL_VDM_PROC, invalid output prarameter");
	check_para_size_return(sizeof(vdm_reg_cfg),
		(uint32_t)vdec_msg.in_size, "VDEC_IOCTL_VDM_PROC_IN");
	check_para_size_return(sizeof(vdm_state_reg),
		(uint32_t)vdec_msg.out_size, "VDEC_IOCTL_VDM_PROC_OUT");
	if (copy_from_user(&vdm_reg_cfg,
			vdec_msg.in, sizeof(vdm_reg_cfg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_VDM_PROC : copy_from_user failed\n");
		return -EFAULT;
	}

	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_vdh);
	vdec_mutex_lock(&omxvdec->vdec_mutex_vdh);
	fd_index = vdec_get_file_index(file);
	if (fd_index == INVALID_IDX) {
		dprint(PRN_FATAL, "%s file is wrong\n", __func__);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
		return -EIO;
	}

	dsb(sy);

	ret = vctrl_vdm_hal_process(&vdm_reg_cfg, &vdm_state_reg,
		&(g_vdec_map_info[fd_index].vdh[0]));
	if (ret != 0) {
		dprint(PRN_FATAL, "vctrl_vdm_hal_process failed\n");
		vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
		return -EIO;
	}

	vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
	vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
	if (copy_to_user(vdec_msg.out,
			&vdm_state_reg, sizeof(vdm_state_reg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_VDM_PROC : copy_to_user failed\n");
		return -EFAULT;
	}

	return ret;
}

static long handle_get_vdm_hwstate(
	vdec_ioctl_msg vdec_msg, omx_vdec_entry *omxvdec)
{
	int32_t vdm_is_run;

	check_return(vdec_msg.out != VCODEC_NULL,
		"VDEC_IOCTL_GET_VDM_HWSTATE, invalid output prarameter");
	check_para_size_return(sizeof(vdm_is_run),
		(uint32_t)vdec_msg.out_size, "VDEC_IOCTL_GET_VDM_HWSTATE");
	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_vdh);
	vdec_mutex_lock(&omxvdec->vdec_mutex_vdh);
	vdm_is_run = vctrl_vdm_hal_is_run();

	vdec_mutex_unlock(&omxvdec->vdec_mutex_vdh);
	vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
	if (copy_to_user(vdec_msg.out, &vdm_is_run, sizeof(vdm_is_run))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_GET_VDM_HWSTATE : copy_to_user failed\n");
		return -EFAULT;
	}

	return 0;
}

static long handle_scd_proc(
	vdec_ioctl_msg vdec_msg, omx_vdec_entry *omxvdec,
	const struct file *file)
{
	omx_scd_reg_cfg_s scd_reg_cfg;
	scd_state_reg_s scd_state_reg;
	int32_t fd_index;
	int32_t ret;

	check_return(vdec_msg.in != VCODEC_NULL,
		"VDEC_IOCTL_SCD_PROC, invalid input prarameter");
	check_return(vdec_msg.out != VCODEC_NULL,
		"VDEC_IOCTL_SCD_PROC, invalid output prarameter");
	check_para_size_return(sizeof(scd_reg_cfg),
		(uint32_t)vdec_msg.in_size, "VDEC_IOCTL_SCD_PROC_IN");
	check_para_size_return(sizeof(scd_state_reg),
		(uint32_t)vdec_msg.out_size, "VDEC_IOCTL_SCD_PROC_OUT");
	if (copy_from_user(&scd_reg_cfg,
			vdec_msg.in, sizeof(scd_reg_cfg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_SCD_PROC :  copy_from_user failed\n");
		return -EFAULT;
	}

	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_scd);
	vdec_mutex_lock(&omxvdec->vdec_mutex_scd);
	fd_index = vdec_get_file_index(file);
	if (fd_index == INVALID_IDX) {
		vdec_mutex_unlock(&omxvdec->vdec_mutex_scd);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
		dprint(PRN_FATAL, "%s file is wrong\n", __func__);
		return -EIO;
	}

	dsb(sy);

	ret = vctrl_scd_hal_process(&scd_reg_cfg,
		&scd_state_reg, &(g_vdec_map_info[fd_index].scd[0]));
	if (ret != 0) {
		dprint(PRN_FATAL, "vctrl_scd_hal_process failed\n");
		vdec_mutex_unlock(&omxvdec->vdec_mutex_scd);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
		return -EIO;
	}

	vdec_mutex_unlock(&omxvdec->vdec_mutex_scd);
	vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
	if (copy_to_user(vdec_msg.out,
			&scd_state_reg, sizeof(scd_state_reg))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_SCD_PROC : copy_to_user failed\n");
		return -EFAULT;
	}

	return ret;
}

static long handle_iommu_map(vdec_ioctl_msg vdec_msg)
{
	int32_t ret;
	vdec_buffer_record buf_record;

	check_return(vdec_msg.in != VCODEC_NULL,
		"VDEC_IOCTL_IOMMU_MAP, invalid input prarameter");
	check_return(vdec_msg.out != VCODEC_NULL,
		"VDEC_IOCTL_IOMMU_MAP, invalid output prarameter");
	check_para_size_return(sizeof(buf_record),
		(uint32_t)vdec_msg.in_size, "VDEC_IOCTL_IOMMU_MAP_IN");
	check_para_size_return(sizeof(buf_record),
		(uint32_t)vdec_msg.out_size, "VDEC_IOCTL_IOMMU_MAP_OUT");
	if (copy_from_user(&buf_record,
			vdec_msg.in, sizeof(buf_record))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_MAP : copy_from_user failed\n");
		return -EFAULT;
	}

	ret = vdec_mem_iommu_map(buf_record.share_fd, &buf_record.iova);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s:VDEC_IOCTL_IOMMU_MAP failed\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user(vdec_msg.out,
			&buf_record, sizeof(buf_record))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_MAP : copy_to_user failed\n");
		(void)vdec_mem_iommu_unmap(buf_record.share_fd, buf_record.iova);
		return -EFAULT;
	}

	return ret;
}

static long handle_iommu_unmap(vdec_ioctl_msg vdec_msg)
{
	int32_t ret;
	vdec_buffer_record buf_record;

	check_return(vdec_msg.in != VCODEC_NULL, "VDEC_IOCTL_IOMMU_UNMAP, invalid input prarameter");
	check_para_size_return(sizeof(buf_record), (uint32_t)vdec_msg.in_size, "VDEC_IOCTL_IOMMU_UNMAP_IN");
	if (copy_from_user(&buf_record,
			vdec_msg.in, sizeof(buf_record))) {
		dprint(PRN_FATAL, "VDEC_IOCTL_IOMMU_UNMAP : copy_from_user failed\n");
		return -EFAULT;
	}

	ret = vdec_mem_iommu_unmap(buf_record.share_fd, buf_record.iova);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s:VDEC_IOCTL_IOMMU_UNMAP failed\n", __func__);
		return -EFAULT;
	}

	return ret;
}

static long handle_lock_hw(unsigned int cmd, omx_vdec_entry *omxvdec)
{
	vcodec_bool x_scene;

	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_scd);
	vdec_mutex_lock(&omxvdec->vdec_mutex_sec_vdh);
	x_scene = vctrl_scen_ident(cmd);
	if (x_scene == VCODEC_FALSE) {
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
		dprint(PRN_ALWS, "%s : lock hw error\n", __func__);
		return -EIO;
	}
	if (omxvdec->device_locked)
		dprint(PRN_ALWS, "hw have locked\n");
	omxvdec->device_locked = VCODEC_TRUE;

	return 0;
}

static long handle_unlock_hw(unsigned int cmd, omx_vdec_entry *omxvdec)
{
	vcodec_bool x_scene;

	x_scene = vctrl_scen_ident(cmd);
	check_scene_eq_return(x_scene == VCODEC_FALSE, "unlock hw error");
	if (omxvdec->device_locked) {
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_vdh);
		vdec_mutex_unlock(&omxvdec->vdec_mutex_sec_scd);
		omxvdec->device_locked = VCODEC_FALSE;
	}

	return 0;
}

static long omxvdec_ioctl_common(
	struct file *file, unsigned int cmd,
	unsigned long arg, compat_type_e type)
{
	int32_t ret;
	vdec_ioctl_msg  vdec_msg;
	void __user *u_arg   = (void __user *)(uintptr_t)arg;
	omx_vdec_entry    *omxvdec = file->private_data;
	check_return(omxvdec != VCODEC_NULL, "omxvdec is null");
	ret = memset_s(&vdec_msg, sizeof(vdec_msg), 0, sizeof(vdec_msg));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
		return -EFAULT;
	}
	if ((cmd != VDEC_IOCTL_UNLOCK_HW) && (cmd != VDEC_IOCTL_LOCK_HW)) {
		ret = omxvdec_compat_get_data(type, u_arg, &vdec_msg);
		check_return(ret == 0, "compat data get failed");
	}

	switch (cmd) {
	case VDEC_IOCTL_SET_CLK_RATE:
		ret = handle_set_clk_rate(vdec_msg, omxvdec, file);
		break;
	case VDEC_IOCTL_VDM_PROC:
		ret = handle_vdm_proc(vdec_msg, omxvdec, file);
		break;
	case VDEC_IOCTL_GET_VDM_HWSTATE:
		ret = handle_get_vdm_hwstate(vdec_msg, omxvdec);
		break;

	case VDEC_IOCTL_SCD_PROC:
		ret = handle_scd_proc(vdec_msg, omxvdec, file);
		break;
	case VDEC_IOCTL_IOMMU_MAP:
		ret = handle_iommu_map(vdec_msg);
		break;
	case VDEC_IOCTL_IOMMU_UNMAP:
		ret = handle_iommu_unmap(vdec_msg);
		break;
		/*lint -e454 -e456 -e455*/
	case VDEC_IOCTL_LOCK_HW:
		ret = handle_lock_hw(cmd, omxvdec);
		break;
	case VDEC_IOCTL_UNLOCK_HW:
		ret = handle_unlock_hw(cmd, omxvdec);
		break;
		/*lint +e454 +e456 +e455*/
	default:
		/* could not handle ioctl */
		dprint(PRN_FATAL, "%s %d:  cmd : %d is not supported\n", __func__, __LINE__, _IOC_NR(cmd));
		return -ENOTTY;
	}

	return ret;    //lint !e454
}

static long omxvdec_ioctl(
	struct file *file, unsigned int cmd,
	unsigned long arg)
{
	return omxvdec_ioctl_common(file, cmd, arg, T_IOCTL_ARG);
}

/* Modified for 64-bit platform */
#ifdef CONFIG_COMPAT
static long omxvdec_compat_ioctl(
	struct file *file, unsigned int cmd,
	unsigned long arg)
{
	void *user_ptr = compat_ptr(arg);

	return  omxvdec_ioctl_common(file, cmd,
		(unsigned long)(uintptr_t)user_ptr, T_IOCTL_ARG_COMPAT);
}
#endif

static const struct file_operations omxvdec_fops = {
	.owner          = THIS_MODULE,
	.open           = omxvdec_open,
	.unlocked_ioctl = omxvdec_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = omxvdec_compat_ioctl,
#endif
	.release        = omxvdec_release,
};

static int32_t omxvdec_device_probe(struct platform_device *pltdev)
{
	uint32_t fpga = 0;
	uint32_t idle;
	uint32_t peri_state  = 0;
	uint32_t peri_mask   = 0;
	uint32_t *pctrl_addr = VCODEC_NULL;
	struct device *dev = &pltdev->dev;

	if (of_property_read_u32(dev->of_node, "vdec_fpga", &fpga)
		|| fpga == NORMAL_MODE || fpga == FPGA_MODE_NO_CHECK_RTL) {
		dprint(PRN_ALWS, "not vdec fpga platform or no need check, return succ as normal mode\n");
		return 0;
	}

	dprint(PRN_ALWS, "vdec fpga platform, check whether rtl logic exist\n");

	if (of_property_read_u32(dev->of_node, "pctrl_peri_state", &peri_state)) {
		dprint(PRN_ALWS, "pctrlPeriState read failed, do not init vdec driver\n");
		return VCODEC_FAILURE;
	}
	if (of_property_read_u32(dev->of_node, "pctrl_peri_vdec_mask", &peri_mask)) {
		dprint(PRN_ALWS, "pctrlPeriMask read failed, do not init vdec driver\n");
		return VCODEC_FAILURE;
	}

	pctrl_addr = (uint32_t *)ioremap(peri_state, (unsigned long)4); //lint !e446
	if (pctrl_addr == VCODEC_NULL) {
		dprint(PRN_ALWS, "ioremap vdec state reg failed, do not init vdec driver\n");
		return VCODEC_FAILURE;
	}

	idle = readl(pctrl_addr) & peri_mask;
	iounmap(pctrl_addr);

	return ((idle != 0) ? 0 : VCODEC_FAILURE);
}

static int32_t omxvdec_probe(struct platform_device *pltdev)
{
	int32_t ret;

	if (omxvdec_device_probe(pltdev) == VCODEC_FAILURE) {
		dprint(PRN_ERROR, "fpga platform check vdec rtl failed, bypass driver probe\n");
		return VCODEC_FAILURE;
	}

	if (g_is_device_detected == VCODEC_TRUE) {
		dprint(PRN_DBG, "Already probe omxvdec\n");
		return 0;
	}

	platform_set_drvdata(pltdev, VCODEC_NULL);
	ret = memset_s(&g_omx_vdec_entry, sizeof(omx_vdec_entry), 0,
		sizeof(omx_vdec_entry));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function, ret = %d\n", __func__, __LINE__, ret);
		return VCODEC_FAILURE;
	}
	ret = memset_s(g_vdec_map_info, sizeof(g_vdec_map_info), 0, sizeof(g_vdec_map_info));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function, ret = %d\n", __func__, __LINE__, ret);
		return VCODEC_FAILURE;
	}
	vdec_init_mutex(&g_omx_vdec_entry.omxvdec_mutex);
	vdec_init_mutex(&g_omx_vdec_entry.vdec_mutex_scd);
	vdec_init_mutex(&g_omx_vdec_entry.vdec_mutex_vdh);
	vdec_init_mutex(&g_omx_vdec_entry.vdec_mutex_sec_scd);
	vdec_init_mutex(&g_omx_vdec_entry.vdec_mutex_sec_vdh);

	ret = vdec_mem_probe(&pltdev->dev);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s call vdec_mem_probe failed, ret = %d\n", __func__, ret);
		return VCODEC_FAILURE;
	}

	ret = omxvdec_setup_cdev(&g_omx_vdec_entry, &omxvdec_fops);
	if (ret < 0) {
		dprint(PRN_FATAL, "%s call omxvdec_setup_cdev failed, ret = %d\n", __func__, ret);
		return VCODEC_FAILURE;
	}

	g_omx_vdec_entry.device = &pltdev->dev;

	ret = vdec_regulator_probe(&pltdev->dev);
	if (ret != 0) {
		dprint(PRN_FATAL, "%s call Regulator_Initialize failed, ret = %d\n", __func__, ret);
		omxvdec_cleanup_cdev(&g_omx_vdec_entry);
		return VCODEC_FAILURE;
	}

	platform_set_drvdata(pltdev, &g_omx_vdec_entry);
	g_is_device_detected = VCODEC_TRUE;

	dprint(PRN_ALWS, "vdec probe success\n");
	return 0;
}

static int32_t omxvdec_remove(struct platform_device *pltdev)
{
	omx_vdec_entry *omxvdec = VCODEC_NULL;

	omxvdec = platform_get_drvdata(pltdev);
	if (omxvdec != VCODEC_NULL) {
		if (IS_ERR(omxvdec)) {
			dprint(PRN_ERROR, "call platform_get_drvdata err, errno : %ld\n", PTR_ERR(omxvdec));
		} else {
			omxvdec_cleanup_cdev(omxvdec);
			vdec_regulator_remove(&pltdev->dev);
			platform_set_drvdata(pltdev, VCODEC_NULL);
			g_is_device_detected = VCODEC_FALSE;
		}
	}

	return 0;
}
omx_vdec_entry omx_vdec_get_entry(void)
{
	return g_omx_vdec_entry;
}

static int32_t omxvdec_suspend(
	struct platform_device *pltdev,
	pm_message_t state)
{
	SINT32 ret;

	dprint(PRN_ALWS, "%s +\n", __func__);

	if (g_is_normal_init != 0)
		vctrl_suspend();

	ret = vdec_regulator_disable();
	if (ret != 0)
		dprint(PRN_FATAL, "%s disable regulator failed\n", __func__);

	dprint(PRN_ALWS, "%s -\n", __func__);

	return 0;
}

static int32_t omxvdec_resume(struct platform_device *pltdev)
{
	SINT32 ret;
	clk_rate_e resume_clk = VDEC_CLK_RATE_NORMAL;

	dprint(PRN_ALWS, "%s +\n", __func__);
	vdec_regulator_get_clk_rate(&resume_clk);

	if (g_is_normal_init != 0) {
		ret = vdec_regulator_enable();
		if (ret != 0) {
			dprint(PRN_FATAL, "%s, enable regulator failed\n", __func__);
			return VCODEC_FAILURE;
		}

		ret = vdec_regulator_set_clk_rate(resume_clk);
		if (ret != 0)
			dprint(PRN_ERROR, "%s, set clk failed\n", __func__);

		vctrl_resume();
	}

	dprint(PRN_ALWS, "%s -\n", __func__);

	return 0;
}

static const struct of_device_id vdec_match_table[] = {
	{.compatible = "hisilicon,VCodecV200-vdec", },
	{.compatible = "hisilicon,VCodecV210-vdec", },
	{.compatible = "hisilicon,VCodecV300-vdec", },
	{.compatible = "hisilicon,VCodecV310-vdec", },
	{.compatible = "hisilicon,VCodecV320-vdec", },
	{.compatible = "hisilicon,VCodecV500-vdec", },
	{.compatible = "hisilicon,VCodecV520-vdec", },
	{ }
};

MODULE_DEVICE_TABLE(of, vdec_match_table);

static struct platform_driver omxvdec_driver = {

	.probe   = omxvdec_probe,
	.remove  = omxvdec_remove,
	.suspend = omxvdec_suspend,
	.resume  = omxvdec_resume,
	.driver  = {
		.name  = (char *) g_omx_vdec_drv_name,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(vdec_match_table),
	},
};

int32_t __init omx_vdec_drv_mod_init(void)
{
	int ret = platform_driver_register(&omxvdec_driver);
	if (ret) {
		dprint(PRN_FATAL, "%s register platform driver failed\n", __func__);
		return ret;
	}

#ifdef MODULE
	dprint(PRN_ALWS, "Load vcodec_omxvdec.ko :%d success\n", OMXVDEC_VERSION);
#endif

	return ret;
}

void __exit omx_vdec_drv_mod_exit(void)
{
	platform_driver_unregister(&omxvdec_driver);

#ifdef MODULE
	dprint(PRN_ALWS, "Unload vcodec_omxvdec.ko : %d success\n", OMXVDEC_VERSION);
#endif
}

module_init(omx_vdec_drv_mod_init);
module_exit(omx_vdec_drv_mod_exit);

MODULE_DESCRIPTION("vdec driver");
MODULE_LICENSE("GPL");

