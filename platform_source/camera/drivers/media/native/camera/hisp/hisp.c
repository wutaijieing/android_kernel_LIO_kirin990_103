/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description:Implement of hisp
 * Create: 2014-11-11
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#if defined(HISP150_CAMERA)
#include <platform_include/camera/native/hisp150_cfg.h>
#elif defined(HISP160_CAMERA)
#include <platform_include/camera/native/hisp160_cfg.h>
#elif defined(HISP120_CAMERA)
#include <platform_include/camera/native/hisp120_cfg.h>
#elif defined(HISP200_CAMERA)
#include <platform_include/camera/native/hisp200_cfg.h>
#elif defined(HISP250_CAMERA)
#include <platform_include/camera/native/hisp250_cfg.h>
#elif defined(HISP210_CAMERA)
#include <platform_include/camera/native/hisp210_cfg.h>
#elif defined(HISP310_CAMERA)
#include <platform_include/camera/native/hisp250_cfg.h>
#elif defined(HISP300_CAMERA)
#include <platform_include/camera/native/hisp300_cfg.h>
#elif defined(HISP350_CAMERA)
#include <platform_include/camera/native/hisp350_cfg.h>
#elif defined(HISP220_CAMERA)
#include <platform_include/camera/native/hisp210_cfg.h>
#elif defined(HISP230_CAMERA)
#include <platform_include/camera/native/hisp210_cfg.h>
#else
#include <platform_include/camera/native/hisp_cfg.h>
#endif

#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-core.h>
#include <linux/string.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>

#include <dsm/dsm_pub.h>
#include <securec.h>
#include "cam_buf.h"
#include "hisp_intf.h"
#include "cam_log.h"

#define DSM_DEV_BUFF_SIZE 1024
#define CREATE_TRACE_POINTS

static struct dsm_dev g_dev_hisp = {
									.name = "dsm_hisp",
									.device_name = NULL,
									.ic_name = NULL,
									.module_name = NULL,
									.fops = NULL,
									.buff_size = DSM_DEV_BUFF_SIZE,
};

static struct dsm_client *g_client_hisp = NULL;
int hisp_dev_setpinctl(struct pinctrl *isp_pinctrl,
						 struct pinctrl_state *pinctrl_def, struct pinctrl_state *pinctrl_idle);
int hisp_dev_setclkdepend(struct clk *aclk_dss, struct clk *pclk_dss);
int hisp_secmem_pa_set(u64 sec_boot_phymem_addr, unsigned int trusted_mem_size);
int hisp_secmem_pa_release(void);
int hisp_mdcmem_pa_set(u64 mdc_phymem_addr, unsigned int mdc_mem_size,
					   int shared_fd);
int hisp_mdcmem_pa_release(void);

/*
 * @brief adapt to all Histar ISP
 *
 * When Histar ISP is probed, this sturcture will be initialized,
 * the object will be found will be found by subdev and notify.
 *
 * @end
 */
typedef struct _tag_hisp {
	struct v4l2_subdev subdev; /* subscribe event to camera daemon */
	struct mutex lock;
	hisp_intf_t *hw; /* call the specfic Histar ISP implement */
	/* get notification from specfic Histar ISP call back */
	hisp_notify_intf_t notify;
} hisp_t;

static bool g_hisp_mdcmem_set;
static bool g_hisp_secmem_set;
#define SD2HISP(sd) container_of(sd, hisp_t, subdev)
#define Notify2HISP(i) container_of(i, hisp_t, notify)

extern int memset_s(void *dest, size_t destMax, int c, size_t count);

/*
 * when get call back from rpmsg, notify
 * camera daemon throuth v4l2 event
 * i: hisp_notify_intf_t to find hisp_t object
 * hisp_ev: hisp event with specific rpmsg type
 */
static void hisp_notify_rpmsg_cb(hisp_notify_intf_t *i, hisp_event_t *hisp_ev)
{
	hisp_t *isp = NULL;
	struct v4l2_event ev;
	struct video_device *vdev = NULL;
	hisp_event_t *req = (hisp_event_t *)ev.u.data;
	errno_t err = memset_s(&ev, sizeof(struct v4l2_event), 0,
						   sizeof(struct v4l2_event));
	if (err != EOK)
		cam_err("func %s: memset_s fail", __func__);
	cam_debug("enter %s", __func__);

	if (!hisp_ev || !i) {
		cam_err("func %s: hisp_ev or i is NULL", __func__);
		return;
	}
	isp = Notify2HISP(i);
	if (!isp) {
		cam_err("func %s: isp is NULL", __func__);
		return;
	}
	hisp_assert(isp);
	vdev = isp->subdev.devnode;

	ev.type = HISP_V4L2_EVENT_TYPE;
	ev.id = HISP_HIGH_PRIO_EVENT;

	req->kind = hisp_ev->kind;

	v4l2_event_queue(vdev, &ev);
}

static hisp_notify_vtbl_t s_notify_hisp = {
										   .rpmsg_cb = hisp_notify_rpmsg_cb,
};

static void *hisp_subdev_get_info(hisp_t *isp, hisp_info_t *info)
{
	char *name = NULL;
	size_t min_len;

	if (!isp || !info) {
		cam_err("func %s: isp or info is NULL", __func__);
		return NULL;
	}
	name = hisp_intf_get_name(isp->hw);
	if (name) {
		errno_t err = memset_s(info->name, HISP_NAME_SIZE, 0,
							   HISP_NAME_SIZE);
		if (err != EOK)
			cam_err("func %s: memset_s fail", __func__);

		min_len = strlen(name);
		if (min_len > HISP_NAME_SIZE - 1)
			min_len = HISP_NAME_SIZE - 1;

		err = memcpy_s(info->name, HISP_NAME_SIZE, name, min_len);
		if (err != EOK)
			return NULL;

		return info->name;
	} else {
		cam_err("%s: hisp_intf_get_name() is NULL", __func__);
		return NULL;
	}
}

static void hisp_subdev_get_system_timestamp(
											 hisp_system_time_t *hisp_system_time)
{
	if (!hisp_system_time)
		return;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	hisp_system_time->s_system_counter = arch_counter_get_cntvct();
#else
	hisp_system_time->s_system_counter = __arch_counter_get_cntvct();
#endif
	hisp_system_time->s_system_couter_rate = arch_timer_get_rate();
}

static long hisp_ioctl_select(unsigned int cmd, hisp_t *isp, void *arg,
							  hisp_msg_t *isp_msg)
{
	long rc = 0;
	size_t len = sizeof(hisp_msg_t);

	switch (cmd) {
	case HISP_IOCTL_CFG_ISP:
		cam_debug("Enter HISP_IOCTL_CFG_ISP");
		if (isp && isp->hw && isp->hw->vtbl)
			rc = isp->hw->vtbl->config(isp->hw, arg);
		break;
	case HISP_IOCTL_GET_INFO:
		cam_debug("Enter HISP_IOCTL_GET_INFO");
		if (!hisp_subdev_get_info(isp, (hisp_info_t *)arg)) {
			rc = -EFAULT;
			cam_err("func %s, line %d: ret = %ld",
					__func__, __LINE__, rc);
		}
		break;
	case HISP_IOCTL_POWER_ON:
		break;
	case HISP_IOCTL_GET_SYSTEM_TIME:
		cam_debug("Enter HISP_IOCTL_GET_SYSTEM_TIME");
		hisp_subdev_get_system_timestamp((hisp_system_time_t *)arg);
		break;
	case HISP_IOCTL_POWER_OFF:
		break;
	case HISP_IOCTL_SEND_RPMSG:
		cam_debug("Enter HISP_IOCTL_SEND_RPMSG");
		if (memcpy_s(isp_msg, len, (hisp_msg_t *)arg, len) != EOK)
			cam_err("func %s: memcpy_s fail", __func__);
		if (isp && isp->hw && isp->hw->vtbl)
			rc = isp->hw->vtbl->send_rpmsg(isp->hw, isp_msg, len);
		break;
	case HISP_IOCTL_RECV_RPMSG:
		cam_debug("Enter HISP_IOCTL_RECV_RPMSG");
		if (isp && isp->hw && isp->hw->vtbl)
			rc = isp->hw->vtbl->recv_rpmsg(isp->hw, arg, len);
		break;
	default:
		cam_err("invalid IOCTL CMD(0x%x)", cmd);
		rc = -EINVAL;
		break;
	}

	return rc;
}

/* ioctl function for v4l2 subdev */
static long hisp_vo_subdev_ioctl(struct v4l2_subdev *sd,
								 unsigned int cmd, void *arg)
{
	long rc = 0;
	hisp_msg_t isp_msg;
	hisp_t *isp = NULL;
	errno_t err = memset_s(&isp_msg, sizeof(hisp_msg_t), 0,
						   sizeof(hisp_msg_t));
	if (err != EOK)
		cam_err("func %s: memset_s fail", __func__);
	hisp_assert(sd);
	if (!arg || !sd) {
		cam_err("func %s: arg or sd is NULL", __func__);
		return -1;
	}
	isp = SD2HISP(sd);
	if (!isp) {
		cam_err("func %s: isp is NULL", __func__);
		return -1;
	}
	cam_debug("cmd is (0x%x), arg(%pK)", cmd, arg);

	rc = hisp_ioctl_select(cmd, isp, arg, &isp_msg);
	return rc;
}

#ifdef CONFIG_COMPAT
static long hisp_usercopy(
	struct v4l2_subdev *sd,
	unsigned int cmd,
	void *kp,
	void __user *up)
{
	long rc;

	rc = memset_s(kp, _IOC_SIZE(cmd), 0, _IOC_SIZE(cmd));
	if (rc != 0) {
		cam_err("%s memset_s return fail", __func__);
		return -EINVAL;
	}

	if (!up || !kp) {
		cam_err("func %s: up or kp is NULL", __func__);
		return -1;
	}

	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(kp, up, _IOC_SIZE(cmd))) {
			cam_err("%s: copy in arg failed", __func__);
			return -EFAULT;
		}
	}

	rc = hisp_vo_subdev_ioctl(sd, cmd, kp);
	if (rc < 0) {
		cam_err("%s subdev_ioctl failed", __func__);
		return -EFAULT;
	}

	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user(up, kp, _IOC_SIZE(cmd))) {
			cam_err("%s: copy back arg failed", __func__);
			return -EFAULT;
		}
		break;
	default:
		break;
	}

	return rc;
}

static long hisp_ioctl_usercopy_data(void *vpdata, unsigned int size_data,
									 struct v4l2_subdev *sd, unsigned int cmd, unsigned long arg)
{
	long rc;

	if (!vpdata || !sd) {
		cam_err("func %s: up or kp is NULL", __func__);
		return -1;
	}

	if (_IOC_SIZE(cmd) > size_data) {
		cam_err("%s: cmd size too large", __func__);
		return -EINVAL;
	}

	rc = hisp_usercopy(sd, cmd, vpdata, (void __user *)arg);
	if (rc < 0)
		cam_err("%s: hisp_usercopy fail cmd = %d", __func__, cmd);

	return rc;
}

static long hisp_vo_subdev_compat_ioctl32(struct v4l2_subdev *sd,
										  unsigned int cmd, unsigned long arg)
{
	long rc = -EFAULT;

	switch (cmd) {
	case HISP_IOCTL_CFG_ISP: {
		struct hisp_cfg_data data;

		rc = hisp_ioctl_usercopy_data((void *)&data, sizeof(data),
									  sd, cmd, arg);
		break;
	}
	case HISP_IOCTL_GET_INFO: {
		hisp_info_t data;

		rc = hisp_ioctl_usercopy_data((void *)&data, sizeof(data),
									  sd, cmd, arg);
		break;
	}
	case HISP_IOCTL_GET_SYSTEM_TIME: {
		hisp_system_time_t data;

		rc = hisp_ioctl_usercopy_data((void *)&data, sizeof(data),
									  sd, cmd, arg);
		break;
	}
	case HISP_IOCTL_SEND_RPMSG: {
		struct hisp_cfg_data data;

		rc = hisp_ioctl_usercopy_data((void *)&data, sizeof(data),
									  sd, cmd, arg);
		break;
	}
	case HISP_IOCTL_RECV_RPMSG: {
		hisp_msg_t data;

		rc = hisp_ioctl_usercopy_data((void *)&data, sizeof(data),
									  sd, cmd, arg);
		break;
	}
	default:
		cam_err("invalid IOCTL CMD(0x%x)", cmd);
		break;
	}

	return rc;
}
#endif

static int hisp_subdev_subscribe_event(struct v4l2_subdev *sd,
									   struct v4l2_fh *fh, struct v4l2_event_subscription *sub)
{
	(void)sd;
	cam_info("enter %s", __func__);
	/* 128:size */
	return v4l2_event_subscribe(fh, sub, 128, NULL);
}

static int hisp_subdev_unsubscribe_event(struct v4l2_subdev *sd,
										 struct v4l2_fh *fh, struct v4l2_event_subscription *sub)
{
	(void)sd;
	cam_info("enter %s", __func__);
	return v4l2_event_unsubscribe(fh, sub);
}

static struct v4l2_subdev_core_ops s_subdev_core_ops_hisp = {
															 .ioctl = hisp_vo_subdev_ioctl,
#ifdef CONFIG_COMPAT
															 .compat_ioctl32 = hisp_vo_subdev_compat_ioctl32,
#endif
															 .subscribe_event = hisp_subdev_subscribe_event,
															 .unsubscribe_event = hisp_subdev_unsubscribe_event,
};

static int hisp_subdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	hisp_t *isp = NULL;
	int rc = 0;
	(void)fh;

	if (!sd) {
		cam_err("func %s: sd is NULL", __func__);
		return -1;
	}
	isp = SD2HISP(sd);
	if (!isp) {
		cam_err("func %s: isp is NULL", __func__);
		return -1;
	}

	if (isp->hw && isp->hw->vtbl && isp->hw->vtbl->open)
		rc = isp->hw->vtbl->open(isp->hw);

	return rc;
}

static int hisp_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	hisp_t *isp = NULL;
	int rc = 0;
	(void)fh;

	if (!sd) {
		cam_err("func %s: sd is NULL", __func__);
		return -1;
	}
	isp = SD2HISP(sd);
	if (!isp) {
		cam_err("func %s: isp is NULL", __func__);
		return -1;
	}

	if (isp->hw && isp->hw->vtbl && isp->hw->vtbl->close)
		rc = isp->hw->vtbl->close(isp->hw);

	return rc;
}

static struct v4l2_subdev_internal_ops s_subdev_internal_ops_hisp = {
																	 .open = hisp_subdev_open,
																	 .close = hisp_subdev_close,
};

static struct v4l2_subdev_ops s_subdev_ops_hisp = {
												   .core = &s_subdev_core_ops_hisp,
};

int get_isp_dt_aclk(struct device_node *dev_node, hisp_dt_data_t *dt,
					struct device *dev)
{
	int ret;
	const char *clk_name = NULL;

	ret = of_property_read_string_index(dev_node, "clock-names",
										0, &clk_name);
	cam_info("%s: clk(%s)", __func__, clk_name);
	if (ret < 0) {
		cam_err("%s: could not get aclk name", __func__);
		return ret;
	}

	dt->aclk = devm_clk_get(dev, clk_name);
	if (IS_ERR_OR_NULL(dt->aclk)) {
		cam_err("%s: could not get aclk", __func__);
		ret = PTR_ERR(dt->aclk);
		return ret;
	}
	return 0;
}

int get_isp_dt_aclk_dss(struct device_node *dev_node, hisp_dt_data_t *dt,
						struct device *dev)
{
	int ret;
	const char *clk_name = NULL;

	ret = of_property_read_string_index(dev_node,
										"clock-names", 1, &clk_name);
	cam_info("%s: clk(%s)", __func__, clk_name);
	if (ret < 0) {
		cam_err("%s: could not get aclk_dss name", __func__);
		return ret;
	}

	dt->aclk_dss = devm_clk_get(dev, clk_name);
	if (IS_ERR_OR_NULL(dt->aclk_dss)) {
		cam_err("%s: could not get aclk_dss", __func__);
		ret = PTR_ERR(dt->aclk_dss);
		return ret;
	}
	return 0;
}

int get_isp_dt_pclk_dss(struct device_node *dev_node, hisp_dt_data_t *dt,
						struct device *dev)
{
	int ret;
	const char *clk_name = NULL;
	/* 2: Stores the specified string. */
	ret = of_property_read_string_index(dev_node,
										"clock-names", 2, &clk_name);
	cam_info("%s: clk(%s)", __func__, clk_name);
	if (ret < 0) {
		cam_err("%s: could not get pclk_dss name", __func__);
		return ret;
	}

	dt->pclk_dss = devm_clk_get(dev, clk_name);
	if (IS_ERR_OR_NULL(dt->pclk_dss)) {
		cam_err("%s: could not get pclk_dss", __func__);
		ret = PTR_ERR(dt->pclk_dss);
		return ret;
	} else {
		/* 120000000UL: size of clk */
		ret = clk_set_rate(dt->pclk_dss, 120000000UL);
		if (ret < 0)
			cam_err("%s: could not set pclk_dss rate",
					__func__);
	}
	return 0;
}

int set_isp_dt_parameter(struct device *dev, hisp_dt_data_t *dt)
{
	int ret = 0;

	dt->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(dt->pinctrl)) {
		cam_err("%s: could not get pinctrl", __func__);
		ret = PTR_ERR(dt->pinctrl);
		return ret;
	}

	dt->pinctrl_default = pinctrl_lookup_state(dt->pinctrl,
											   PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(dt->pinctrl_default)) {
		cam_err("%s: could not get default pinstate", __func__);
		return -1;
	}

	dt->pinctrl_idle = pinctrl_lookup_state(dt->pinctrl,
											PINCTRL_STATE_IDLE);
	if (IS_ERR_OR_NULL(dt->pinctrl_idle)) {
		cam_err("%s: could not get idle pinstate", __func__);
		return -1;
	} else {
		ret = pinctrl_select_state(dt->pinctrl, dt->pinctrl_idle);
		if (ret != 0) {
			cam_err("%s: could not set idle pinstate",
					__func__);
			return -1;
		}
	}
	return 0;
}

int hisp_get_dt_data(struct platform_device *pdev, hisp_dt_data_t *dt)
{
	int ret;
	struct device *dev = NULL;
	struct device_node *dev_node = NULL;
	unsigned int is_fpga = 0;

	if (!pdev || !dt) {
		cam_err("%s: pdev or dt is NULL", __func__);
		return -1;
	}
	dev = &pdev->dev;
	dev_node = dev->of_node;

	if (!dev_node) {
		cam_err("%s: of node NULL", __func__);
		return -1;
	}

	ret = of_property_read_u32(dev_node, "vendor,is_fpga", &is_fpga);
	if (ret < 0)
		cam_err("%s: get FPGA flag failed", __func__);

	if (is_fpga) {
		cam_info("%s: Ignored FPGA", __func__);
		return 0;
	}

	ret = get_isp_dt_aclk(dev_node, dt, dev);
	if (ret < 0 || IS_ERR_OR_NULL(dt->aclk))
		return ret;

#ifndef HISP350_CAMERA
	ret = get_isp_dt_aclk_dss(dev_node, dt, dev);
	if (ret < 0 || IS_ERR_OR_NULL(dt->aclk_dss))
		return ret;

	ret = get_isp_dt_pclk_dss(dev_node, dt, dev);
	if (ret < 0 || IS_ERR_OR_NULL(dt->pclk_dss))
		return ret;
#endif

	ret = set_isp_dt_parameter(dev, dt);
	if (ret < 0 || IS_ERR_OR_NULL(dt->pinctrl))
		return ret;

	if (hisp_dev_setpinctl(dt->pinctrl, dt->pinctrl_default,
							 dt->pinctrl_idle) < 0) {
		cam_err("%s Failed: setpinctl", __func__);
		return -1;
	}

#ifndef HISP350_CAMERA
	if (hisp_dev_setclkdepend(dt->aclk_dss, dt->pclk_dss) < 0) {
		cam_err("%s Failed: clkdepend", __func__);
		return -1;
	}
#endif

	return 0;
}

/*
 * Initialize the hisp_t structure, called from Histar ISP probe
 * pdev: the rpmsg_channel used in Histar ISP driver
 * hw: to call the hook functions in Histar ISP driver
 * notify: get notified when Histar ISP driver needs
 */
int32_t hisp_register(struct platform_device *pdev, hisp_intf_t *hw,
					  hisp_notify_intf_t **notify)
{
	int rc = 0;
	struct v4l2_subdev *subdev = NULL;
	hisp_t *isp = NULL;
	int ret;

	cam_notice("%s enter", __func__);

	if (!notify) {
		cam_err("func %s: notify is NULL", __func__);
		return -1;
	}

	isp = kzalloc(sizeof(hisp_t), GFP_KERNEL);
	if (!isp) {
		rc = -ENOMEM;
		cam_err("func %s, line %d: out of memory", __func__,
				__LINE__);
		goto alloc_fail;
	}

	hisp_assert(hw);

	subdev = &isp->subdev;
	if (!subdev) {
		cam_err("func %s: subdev is NULL", __func__);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		kzfree(isp);
#else
		kfree_sensitive(isp);
#endif
		return -1;
	}

	mutex_init(&isp->lock);

	v4l2_subdev_init(subdev, &s_subdev_ops_hisp);
	subdev->internal_ops = &s_subdev_internal_ops_hisp;
	ret = snprintf_s(subdev->name, sizeof(subdev->name),
					 sizeof(subdev->name) - 1, "%s", "cam-cfg-hisp");
	if (ret < 0)
		cam_err("func %s: snprintf_s fail", __func__);
	subdev->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	subdev->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;

	init_subdev_media_entity(subdev, CAM_SUBDEV_HISP);
	cam_cfgdev_register_subdev(subdev, CAM_SUBDEV_HISP);
	subdev->devnode->lock = &isp->lock;

	platform_set_drvdata(pdev, subdev);
	isp->hw = hw;
	isp->notify.vtbl = &s_notify_hisp;
	*notify = &(isp->notify);

	/* register hisp dsm client */
	g_client_hisp = dsm_register_client(&g_dev_hisp);

alloc_fail:
	return rc;
}

void hisp_unregister(struct platform_device *pdev)
{
	struct v4l2_subdev *subdev = NULL;
	hisp_t *isp = NULL;

	if (!pdev)
		return;
	subdev = platform_get_drvdata(pdev);
	if (!subdev) {
		cam_err("%s: subdev is NULL", __func__);
		return;
	}
	isp = SD2HISP(subdev);
	if (!isp) {
		cam_err("%s: isp is NULL", __func__);
		return;
	}

	media_entity_cleanup(&subdev->entity);
	cam_cfgdev_unregister_subdev(subdev);
	mutex_destroy(&isp->lock);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	kzfree(isp);
#else
	kfree_sensitive(isp);
#endif
}

int hisp_get_sg_table(int fd, struct device *dev, struct dma_buf **buf,
					  struct dma_buf_attachment **attach, struct sg_table **table)
{
	*buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(*buf))
		goto err_dma_buf;
	*attach = dma_buf_attach(*buf, dev);
	if (IS_ERR_OR_NULL(*attach))
		goto err_dma_buf_attach;

	*table = dma_buf_map_attachment(*attach, DMA_BIDIRECTIONAL);

	if (IS_ERR_OR_NULL(*table)) {
		cam_err("%s Failed : dma_buf_map_attachment.%lu",
				__func__, PTR_ERR(*table));
		goto err_dma_buf_map_attachment;
	}
	cam_info("func %s: dma_buf_map_attachment ok", __func__);
	return 0;
err_dma_buf_map_attachment:
	dma_buf_detach(*buf, *attach);
err_dma_buf_attach:
	dma_buf_put(*buf);
err_dma_buf:
	return -ENODEV;
}

void hisp_free_dma_buf(struct dma_buf **buf, struct dma_buf_attachment **attach,
					   struct sg_table **table)
{
	dma_buf_unmap_attachment(*attach, *table, DMA_BIDIRECTIONAL);
	dma_buf_detach(*buf, *attach);
	dma_buf_put(*buf);
	*table = NULL;
	*attach = NULL;
	*buf = NULL;
}

int hisp_set_sec_fw_buffer(struct hisp_cfg_data *cfg)
{
	int rc;
	uint32_t size;
	phys_addr_t phys;
	struct sg_table *sgt = NULL;

	sgt = cam_buf_get_sgtable(cfg->share_fd);
	if (IS_ERR_OR_NULL(sgt)) {
		cam_err("%s: error get sg_table of share_fd:%d",
				__func__, cfg->share_fd);
		return PTR_ERR(sgt);
	}

	phys = sg_phys(sgt->sgl);
	size = sgt->sgl->length;
	if (size < cfg->buf_size) {
		cam_err("%s: real size [%u] less than claimed [%u]",
				__func__, size, cfg->buf_size);
		rc = -ERANGE;
		goto err_fw_buf_size;
	}

	/*
	 * incase something happens, mdc is not release as expected,
	 * like Tomestone. we release it here before next set.
	 */
	if (g_hisp_secmem_set) {
		cam_info("%s: abnormal case, but do it", __func__);
		(void)hisp_release_sec_fw_buffer();
	}

	rc = hisp_secmem_pa_set(phys, size);
	if (rc < 0)
		cam_err("%s: pa set error:%d", __func__, rc);
	else
		g_hisp_secmem_set = true;

err_fw_buf_size:
	cam_buf_put_sgtable(sgt);
	return rc;
}

int hisp_release_sec_fw_buffer(void)
{
	int rc = hisp_secmem_pa_release();
	if (rc < 0)
		cam_err("%s: fail, rc:%d", __func__, rc);
	else
		g_hisp_secmem_set = false;
	return rc;
}

int hisp_set_mdc_buffer(struct hisp_cfg_data *cfg)
{
	int rc;
	uint32_t size;
	phys_addr_t phys;
	struct sg_table *sgt = NULL;

	if (!cfg) {
		cam_err("%s: input param is null", __func__);
		return -ERANGE;
	}

	sgt = cam_buf_get_sgtable(cfg->share_fd);
	if (IS_ERR_OR_NULL(sgt)) {
		cam_err("%s: error get sg_table of share_fd:%d",
				__func__, cfg->share_fd);
		return PTR_ERR(sgt);
	}

	phys = sg_phys(sgt->sgl);
	size = sgt->sgl->length;

	if (size < cfg->buf_size) { /*lint -e644*/ /*lint -e530*/
		cam_err("%s: real size [%u] less than claimed [%u]",
				__func__,
				size, cfg->buf_size); /*lint -e644*/ /*lint -e530*/

		rc = -ERANGE;
		goto err_buf_size;
	}

	/*
	 * incase something happens, mdc is not release as expected,
	 * like Tomestone. we release it here before next set.
	 */
	if (g_hisp_mdcmem_set) {
		cam_info("%s: abnormal case, but do it", __func__);
		(void)hisp_release_mdc_buffer();
	}

	rc = hisp_mdcmem_pa_set(phys, size, cfg->share_fd); /*lint -e644*/
	if (rc < 0)
		cam_err("%s: pa set error:%d", __func__, rc);
	else
		g_hisp_mdcmem_set = true;

err_buf_size:
	cam_buf_put_sgtable(sgt);
	return rc;
}

int hisp_release_mdc_buffer(void)
{
	int rc = hisp_mdcmem_pa_release();
	if (rc < 0)
		cam_err("%s: fail, rc:%d", __func__, rc);
	else
		g_hisp_mdcmem_set = false;
	return rc;
}
