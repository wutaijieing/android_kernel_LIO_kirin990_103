/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
 * Description: compat 32 source file.
 * Create: 2016-04-01
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

#include "cam_compat32.h"
#include "cam_log.h"
#include "cam_intf.h"

#define check_ret_value(expression, ret) do { \
	if ((expression) != 0) { \
		printk(KERN_ERR "%s %d", __func__, __LINE__); \
		ret = -EINVAL; \
	} \
} while (0)

long compat_get_v4l2_event_data(struct v4l2_event __user *pdata,
	struct v4l2_event32 __user *pdata32)
{
	long ret = 0;
	compat_uint_t type;
	compat_uint_t pending;
	compat_uint_t sequence;
	cam_timespec timestamp;
	compat_uint_t id;
	errno_t err;

	err = memset_s(&timestamp, sizeof(cam_timespec), 0, sizeof(cam_timespec));
	if (err != EOK) {
		printk(KERN_ERR "[%s]memset_s fail, err=%d\n", __func__, err);
		return -EFAULT;
	}

	if (!cam_access_ok(VERIFY_READ, pdata32, sizeof(struct v4l2_event32)))
		return -EFAULT;
	if (pdata == NULL)
		return -EFAULT;

	check_ret_value(get_user(type, &pdata32->type), ret);
	check_ret_value(put_user(type, &pdata->type), ret);
	check_ret_value(copy_in_user(&pdata->u, &pdata32->u, sizeof(pdata->u)), ret);
	check_ret_value(get_user(pending, &pdata32->pending), ret);
	check_ret_value(put_user(pending, &pdata->pending), ret);
	check_ret_value(get_user(sequence, &pdata32->sequence), ret);
	check_ret_value(put_user(sequence, &pdata->sequence), ret);
	check_ret_value(cam_compat_get_timespec(&timestamp, &pdata32->timestamp), ret);
	check_ret_value(copy_to_user(&pdata->timestamp, &timestamp, sizeof(timestamp)), ret);
	check_ret_value(get_user(id, &pdata32->id), ret);
	check_ret_value(put_user(id, &pdata->id), ret);
	check_ret_value(copy_in_user(pdata->reserved, pdata32->reserved,
				8 * sizeof(__u32)), ret); /* 8: reserved data len */

	return ret;
}

long compat_put_v4l2_event_data(struct v4l2_event __user *pdata,
	struct v4l2_event32 __user *pdata32)
{
	long ret = 0;
	compat_uint_t type;
	compat_uint_t pending;
	compat_uint_t sequence;
	cam_timespec timestamp;
	compat_uint_t id;

	if (!cam_access_ok(VERIFY_WRITE, pdata32, sizeof(struct v4l2_event32)))
		return -EFAULT;

	if (pdata == NULL)
		return -EFAULT;
	check_ret_value(get_user(type, &pdata->type), ret);
	check_ret_value(put_user(type, &pdata32->type), ret);
	check_ret_value(copy_in_user(&pdata32->u, &pdata->u, sizeof(pdata->u)), ret);
	check_ret_value(get_user(pending, &pdata->pending), ret);
	check_ret_value(put_user(pending, &pdata32->pending), ret);
	check_ret_value(get_user(sequence, &pdata->sequence), ret);
	check_ret_value(put_user(sequence, &pdata32->sequence), ret);
	check_ret_value(copy_from_user(&timestamp, &pdata->timestamp, sizeof(timestamp)), ret);
	check_ret_value(cam_compat_put_timespec(&timestamp, &pdata32->timestamp), ret);
	check_ret_value(get_user(id, &pdata->id), ret);
	check_ret_value(put_user(id, &pdata32->id), ret);
	check_ret_value(copy_in_user(pdata32->reserved, pdata->reserved,
				8 * sizeof(__u32)), ret); /* 8: reserved data len */
	return ret;
}

static int compat_get_v4l2_window(struct v4l2_window *kp,
	struct v4l2_window32 __user *up)
{
	if (up == NULL)
		return -EFAULT;
	if (!cam_access_ok(VERIFY_READ, up, sizeof(struct v4l2_window32)) ||
		copy_from_user(&kp->w, &up->w, sizeof(kp->w)) ||
		get_user(kp->field, &up->field) ||
		get_user(kp->chromakey, &up->chromakey) ||
		get_user(kp->clipcount, &up->clipcount))
		return -EFAULT;
	if (kp->clipcount > 2048) /* 2048: clip count boundary */
		return -EINVAL;
	if (kp->clipcount) {
		struct v4l2_clip32 __user *uclips = NULL;
		struct v4l2_clip __user *kclips = NULL;
		int n = (int)kp->clipcount;
		compat_caddr_t p;

		if (get_user(p, &up->clips))
			return -EFAULT;
		uclips = compat_ptr(p);
		kclips = compat_alloc_user_space((size_t )n * sizeof(struct v4l2_clip));
		kp->clips = kclips;
		while (--n >= 0) {
			if (copy_in_user(&kclips->c,
				&uclips->c, sizeof(uclips->c)))
				return -EFAULT;
			if (put_user(n ? kclips + 1 : NULL, &kclips->next))
				return -EFAULT;
			uclips += 1;
			kclips += 1;
		}
	} else {
		kp->clips = NULL;
	}
	return 0;
}

static int compat_put_v4l2_window(struct v4l2_window *kp,
	struct v4l2_window32 __user *up)
{
	if (!cam_access_ok(VERIFY_WRITE, up, sizeof(struct v4l2_window32)))
		return -EFAULT;

	if (up == NULL)
		return -EFAULT;
	if (copy_to_user(&up->w, &kp->w, sizeof(kp->w)) ||
		put_user(kp->field, &up->field) ||
		put_user(kp->chromakey, &up->chromakey) ||
		put_user(kp->clipcount, &up->clipcount))
		return -EFAULT;
	return 0;
}

long compat_get_v4l2_format_data(struct v4l2_format *kp,
	struct v4l2_format32 __user *up)
{
	unsigned long ret = 0;
	if (up == NULL)
		return -EFAULT;

	if (!cam_access_ok(VERIFY_READ, up, sizeof(struct v4l2_format32)))
		return -EFAULT;

	if (get_user(kp->type, &up->type))
		return -EFAULT;

	if (kp->type == 0) {
		printk(KERN_INFO "%s: cam type is 0, return ok",
			__func__);
		return 0;
	}

	switch (kp->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		ret |= copy_from_user(&kp->fmt.pix, &up->fmt.pix,
			sizeof(struct v4l2_pix_format));
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		ret |= copy_from_user(&kp->fmt.pix_mp, &up->fmt.pix_mp,
			sizeof(struct v4l2_pix_format_mplane));
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		ret |= (unsigned long)compat_get_v4l2_window(&kp->fmt.win, &up->fmt.win);
		break;
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		ret |= copy_from_user(&kp->fmt.vbi, &up->fmt.vbi,
			sizeof(struct v4l2_vbi_format));
		break;
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		ret |= copy_from_user(&kp->fmt.sliced, &up->fmt.sliced,
			sizeof(struct v4l2_sliced_vbi_format));
		break;
	default:
		printk(KERN_INFO "compat_ioctl32: unexpected VIDIOC_FMT type %d",
			kp->type);
		return -EINVAL;
	}
	return ret == 0 ? 0 : -EINVAL;
}

long compat_put_v4l2_format_data(struct v4l2_format *kp,
	struct v4l2_format32 __user *up)
{
	long ret = 0;

	if (!cam_access_ok(VERIFY_WRITE, up, sizeof(struct v4l2_format32)))
		return -EFAULT;
	if (up == NULL)
		return -EFAULT;

	check_ret_value(put_user(kp->type, &up->type), ret);
	switch (kp->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		check_ret_value(copy_to_user(&up->fmt.pix, &kp->fmt.pix,
					sizeof(struct v4l2_pix_format)), ret);
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		check_ret_value(copy_to_user(&up->fmt.pix_mp, &kp->fmt.pix_mp,
					sizeof(struct v4l2_pix_format_mplane)), ret);
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		check_ret_value(compat_put_v4l2_window(&kp->fmt.win, &up->fmt.win), ret);
		break;
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		check_ret_value(copy_to_user(&up->fmt.vbi, &kp->fmt.vbi,
					sizeof(struct v4l2_vbi_format)), ret);
		break;
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		check_ret_value(copy_to_user(&up->fmt.sliced, &kp->fmt.sliced,
					sizeof(struct v4l2_sliced_vbi_format)), ret);
		break;
	default:
		printk(KERN_INFO "compat_ioctl32: unexpected VIDIOC_FMT type %d",
			kp->type);
		return -EINVAL;
	}
	return ret;
}

long compat_get_cam_buf_status_data(cam_buf_status_t __user *pdata,
	cam_buf_status_t32 __user *pdata32)
{
	long ret = 0;
	compat_int_t id;
	compat_int_t status;
	cam_timeval tv;
	errno_t err;

	err = memset_s(&tv, sizeof(cam_timeval), 0, sizeof(cam_timeval));
	if (err != EOK) {
		printk(KERN_ERR "[%s]memset_s fail, err=%d\n", __func__, err);
		return -EFAULT;
	}

	if (!cam_access_ok(VERIFY_READ, pdata32, sizeof(cam_buf_status_t32)))
		return -EFAULT;
	if (pdata == NULL)
		return -EFAULT;

	check_ret_value(get_user(id, &pdata32->id), ret);
	check_ret_value(put_user(id, &pdata->id), ret);
	check_ret_value(get_user(status, &pdata32->buf_status), ret);
	check_ret_value(put_user(status, &pdata->buf_status), ret);
	check_ret_value(cam_compat_get_timeval(&tv, &pdata32->tv), ret);
	check_ret_value(copy_to_user(&pdata->tv, &tv, sizeof(tv)), ret);
	return ret;
}

long compat_put_cam_buf_status_data(cam_buf_status_t __user *pdata,
	cam_buf_status_t32 __user *pdata32)
{
	long ret = 0;
	compat_int_t id;
	compat_int_t status;
	cam_timeval tv;

	if (!cam_access_ok(VERIFY_WRITE, pdata32, sizeof(cam_buf_status_t32)))
		return -EFAULT;
	if (pdata == NULL)
		return -EFAULT;

	check_ret_value(get_user(id, &pdata->id), ret);
	check_ret_value(put_user(id, &pdata32->id), ret);
	check_ret_value(get_user(status, &pdata->buf_status), ret);
	check_ret_value(put_user(status, &pdata32->buf_status), ret);
	check_ret_value(copy_from_user(&tv, &pdata->tv, sizeof(tv)), ret);
	check_ret_value(cam_compat_put_timeval(&tv, &pdata32->tv), ret);

	return ret;
}
