/*
 * Adp Acm function driver
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "chip_adp_acm.h"

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/skbuff.h>

#include <platform_include/basicplatform/linux/usb/drv_acm.h>
#include <platform_include/basicplatform/linux/usb/bsp_acm.h>
#include <platform_include/basicplatform/linux/usb/drv_udi.h>

#include "modem_acm.h"


#ifdef DEBUG
#define adp_acm_dbg(format, arg...) pr_info("[%s]" format, __func__, ##arg)
#else
#define adp_acm_dbg(format, arg...) do {} while (0)
#endif

struct adp_acm_stat {
	int stat_open_err;
	long stat_open_last_err;
	int stat_read_err;
	int stat_read_last_err;
	int stat_write_err;
	int stat_write_last_err;
	int stat_ioctl_err;
	int stat_ioctl_last_err;
	int stat_wait_close;
};

struct adp_acm_context {
	char *dev_name;
	unsigned int		port_num;
	int			is_open;
	atomic_t		opt_cnt;
	struct adp_acm_stat	stat;
};

static struct adp_acm_context adp_acm_ctx[] = {
	/* dev_name		port_num		 is_open opt_cnt stat */
	{ "/dev/acm_ctrl",	4, 0, {0}, {0} }, /* UDI_USB_ACM_CTRL */
	{ "/dev/acm_at",	0, 0, {0}, {0} }, /* UDI_USB_ACM_AT */
	{ "/dev/acm_c_shell",	3, 0, {0}, {0} }, /* UDI_USB_ACM_SHELL */
	{ "/dev/acm_4g_diag",	5, 0, {0}, {0} }, /* UDI_USB_ACM_LTE_DIAG */
	{ "/dev/acm_3g_diag",	1, 0, {0}, {0} }, /* UDI_USB_ACM_OM */
	{ "/dev/acm_modem",	9, 0, {0}, {0} }, /* UDI_USB_ACM_MODEM */
	{ "/dev/acm_gps",	6, 0, {0}, {0} }, /* UDI_USB_ACM_GPS */
	{ "/dev/acm_a_shell",	2, 0, {0}, {0} }, /* UDI_USB_ACM_3G_GPS */
	{ "/dev/acm_3g_pcvoice",	0xf, 0, {0}, {0} }, /* UDI_USB_ACM_3G_PCVOICE */
	{ "/dev/acm_pcvoice",	0xf, 0, {0}, {0} }, /* UDI_USB_ACM_PCVOICE */
	{ "/dev/acm_skytone",	8, 0, {0}, {0} }, /* UDI_USB_ACM_SKYTONE */
	{ "/dev/acm_cdma_log",	7, 0, {0}, {0} }, /* UDI_USB_ACM_CDMA_LOG */
	{ "/dev/acm_err",	0xf, 0, {0}, {0} },
};

static int stat_invalid_devid;
static int stat_invalid_filp;

static u32 port_dev_id[] = {
	UDI_USB_ACM_AT,
	UDI_USB_ACM_OM,
	UDI_USB_ACM_3G_GPS,
	UDI_USB_ACM_SHELL,
	UDI_USB_ACM_CTRL,
	UDI_USB_ACM_LTE_DIAG,
	UDI_USB_ACM_GPS,
	UDI_USB_ACM_CDMA_LOG, /* invalid */
	UDI_USB_ACM_SKYTONE,
	UDI_USB_ACM_MODEM,
};

#define port_num_handle(__port_num)	\
	((void *)(uintptr_t)((unsigned long)(__port_num) + ACM_CDEV_COUNT))
#define handle_port_num(__handle)	\
	((unsigned int)(unsigned long)(uintptr_t)(__handle) - ACM_CDEV_COUNT)
#define port_num_invalid(__port_num)	\
	((__port_num) >= ARRAY_SIZE(port_dev_id))

static struct adp_acm_context *port_num_to_ctx(unsigned int port_num)
{
	u32 dev_id = port_dev_id[port_num];

	if (dev_id >= ARRAY_SIZE(adp_acm_ctx))
		return NULL;
	return (struct adp_acm_context *)(&adp_acm_ctx[dev_id]);
}


void *bsp_acm_open(u32 dev_id)
{
	unsigned int port_num;
	int ret;

	if (dev_id >= ARRAY_SIZE(adp_acm_ctx)) {
		pr_err("%s: invalid dev_id %u\n", __func__, dev_id);
		stat_invalid_devid++;
		return ERR_PTR(-EINVAL);
	}

	port_num = adp_acm_ctx[dev_id].port_num;
	adp_acm_dbg("dev_id %d, port_num %u\n", dev_id, port_num);

	ret = gs_acm_open(port_num);
	if (ret) {
		pr_err("%s: gs_acm_open failed\n", __func__);
		adp_acm_ctx[dev_id].stat.stat_open_err++;
		adp_acm_ctx[dev_id].stat.stat_open_last_err = ret;
		return ERR_PTR(-EIO);
	}

	adp_acm_ctx[dev_id].is_open = 1;

	return port_num_handle(port_num);
}


int bsp_acm_close(void *handle)
{
	struct adp_acm_context *ctx = NULL;
	unsigned int port_num = handle_port_num(handle);

	if (unlikely(port_num_invalid(port_num))) {
		pr_err("%s: invalid handle %pK\n", __func__, handle);
		stat_invalid_filp++;
		return -EINVAL;
	}
	adp_acm_dbg("dev_id %u, port_num %u\n", port_dev_id[port_num], port_num);

	ctx = port_num_to_ctx(port_num);
	if (ctx == NULL)
		return -ENODEV;

	ctx->is_open = 0;

	/* wait for file opt complete */
	while (atomic_read(&ctx->opt_cnt)) {
		ctx->stat.stat_wait_close++;
		msleep(20);
	}

	return gs_acm_close(port_num);
}


int bsp_acm_write(void *handle, void *buf, unsigned int size)
{
	struct adp_acm_context *ctx = NULL;
	unsigned int port_num = handle_port_num(handle);
	int status;
	loff_t pos = 0;

	if (unlikely(port_num_invalid(port_num))) {
		pr_err("%s: invalid handle %pK\n", __func__, handle);
		stat_invalid_filp++;
		return -EINVAL;
	}
	ctx = port_num_to_ctx(port_num);
	if (ctx == NULL)
		return -ENODEV;

	atomic_inc(&ctx->opt_cnt);

	if (unlikely(!ctx->is_open)) {
		pr_err("%s: port%d is not open\n", __func__, port_num);
		status = -ENXIO;
		goto write_ret;
	}

	status = gs_acm_write(port_num, (void __force __user *)buf, size, &pos);

write_ret:
	atomic_dec(&ctx->opt_cnt);
	if (status < 0) {
		pr_err("%s: port %d write failed\n", __func__, port_num);
		ctx->stat.stat_write_err++;
		ctx->stat.stat_write_last_err = status;
	}

	return status;
}


int bsp_acm_read(void *handle, void *buf, unsigned int size)
{
	struct adp_acm_context *ctx = NULL;
	unsigned int port_num = handle_port_num(handle);
	int status;
	loff_t pos = 0;

	if (unlikely(port_num_invalid(port_num))) {
		pr_err("%s: invalid handle %pK\n", __func__, handle);
		stat_invalid_filp++;
		/* protect system running, usr often use read in while(1). */
		msleep(20);
		return -EINVAL;
	}
	ctx = port_num_to_ctx(port_num);
	if (ctx == NULL)
		return -ENODEV;

	atomic_inc(&ctx->opt_cnt);
	if (unlikely(!ctx->is_open)) {
		pr_err("%s: port%u is not open\n", __func__, port_num);
		status = -ENXIO;
		goto read_ret;
	}

	status = gs_acm_read(port_num, (void *)buf, size, &pos);

read_ret:
	atomic_dec(&ctx->opt_cnt);
	if (status <= 0) {
		pr_err("%s: port %u read failed\n", __func__, port_num);
		/* protect system running, usr often use read in while(1). */
		msleep(100);
		ctx->stat.stat_read_err++;
		ctx->stat.stat_read_last_err = status;
	}

	return status;
}


int bsp_acm_ioctl(void *handle, unsigned int cmd, void *para)
{
	struct adp_acm_context *ctx = NULL;
	unsigned int port_num = handle_port_num(handle);
	int status;

	if (unlikely(port_num_invalid(port_num))) {
		pr_err("%s: invalid handle %pK\n", __func__, handle);
		stat_invalid_filp++;
		return -EINVAL;
	}
	adp_acm_dbg("dev_id %u, port_num %u, cmd 0x%x\n", port_dev_id[port_num],
						port_num, cmd);

	ctx = port_num_to_ctx(port_num);
	if (ctx == NULL)
		return -ENODEV;

	atomic_inc(&ctx->opt_cnt);
	if (unlikely(!ctx->is_open)) {
		pr_err("%s: port%d is not open\n", __func__, port_num);
		status = -ENXIO;
		goto ioctl_ret;
	}

	status = gs_acm_ioctl(port_num, (unsigned int)cmd, (uintptr_t)para);

ioctl_ret:
	atomic_dec(&ctx->opt_cnt);
	if (status < 0) {
		if (!is_modem_port(port_num))
			pr_err("%s: port %u ioctl failed\n", __func__, port_num);

		ctx->stat.stat_ioctl_err++;
		ctx->stat.stat_ioctl_last_err = status;
	}

	return status;
}

void acm_adp_dump(struct seq_file *s)
{
	unsigned int i;

	if (s == NULL) {
		pr_err("adp dump error, seq null!\n");
		return;
	}

	seq_printf(s, "stat_invalid_devid      :%d\n",
		stat_invalid_devid);
	seq_printf(s, "stat_invalid_filp       :%d\n",
		stat_invalid_filp);
	for (i = 0; i < ARRAY_SIZE(adp_acm_ctx); i++) {
		seq_printf(s, "==== dump dev:%s ====\n",
			adp_acm_ctx[i].dev_name);
		seq_printf(s, "is_open             :%d\n",
			adp_acm_ctx[i].is_open);
		seq_printf(s, "opt_cnt             :%d\n",
			atomic_read(&adp_acm_ctx[i].opt_cnt));
		seq_printf(s, "stat_open_err       :%d\n",
			adp_acm_ctx[i].stat.stat_open_err);
		seq_printf(s, "stat_open_last_err  :%ld\n",
			adp_acm_ctx[i].stat.stat_open_last_err);
		seq_printf(s, "stat_read_err       :%d\n",
			adp_acm_ctx[i].stat.stat_read_err);
		seq_printf(s, "stat_read_last_err  :%d\n",
			adp_acm_ctx[i].stat.stat_read_last_err);
		seq_printf(s, "stat_write_err      :%d\n",
			adp_acm_ctx[i].stat.stat_write_err);
		seq_printf(s, "stat_write_last_err :%d\n",
			adp_acm_ctx[i].stat.stat_write_last_err);
		seq_printf(s, "stat_ioctl_err      :%d\n",
			adp_acm_ctx[i].stat.stat_ioctl_err);
		seq_printf(s, "stat_ioctl_last_err :%d\n",
			adp_acm_ctx[i].stat.stat_ioctl_last_err);
		seq_printf(s, "stat_wait_close     :%d\n",
			adp_acm_ctx[i].stat.stat_wait_close);
		seq_puts(s, "\n");
	}
}
