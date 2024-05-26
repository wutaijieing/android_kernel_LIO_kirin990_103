// SPDX-License-Identifier: GPL-2.0
/*
 * oled_spi_display.c
 *
 * driver for olde spi disaply
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/regulator/consumer.h>
#include <linux/leds.h>
#include <linux/uaccess.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/atomic.h>
#include <huawei_platform/log/hw_log.h>
#include "oled_spi_display.h"
/* lcd_kit 3.0 */
#ifdef CONFIG_LCD_KIT_DRIVER
#include "lcd_kit_sim_serial_disp.h"
#endif

#define HWLOG_TAG oled_disp
HWLOG_REGIST();

static struct fb_info *g_fbinfo;

#ifdef CONFIG_LCD_KIT_DRIVER
static int lcd_kit_spi_on(struct platform_device *pdev,
		struct callback_data *data);
static int lcd_kit_spi_off(struct platform_device *pdev,
		struct callback_data *data);
static int lcd_kit_spi_set_backlight(struct platform_device *pdev,
		struct callback_data *data);
#endif

static struct hisi_oled_seq initseq_edo[] = {
	{'c', 0xFE},
	{'d', 0x01},

	{'c', 0x6E},
	{'d', 0x0A},

	{'c', 0xFE},
	{'d', 0x00},

	{'c', 0xC4},
	{'d', 0x80},

	{'c', 0x35},
	{'d', 0x00},

	{'c', 0x53},
	{'d', 0x20},

	{'c', 0x3A},
	{'d', 0x75}, /* pixel */

	{'c', 0x2A},
	{'d', 0x00},
	{'d', 0x08},
	{'d', 0x01},
	{'d', 0xD9},

	{'c', 0x2B},
	{'d', 0x00},
	{'d', 0x00},
	{'d', 0x01},
	{'d', 0xD1},

	{'c', 0x11},
	{'w', 160},
	{'c', 0x29},

};

static struct oledfb_par *oled_spi_get_fbinfo_par(void)
{
	if (!g_fbinfo || !g_fbinfo->par) {
		hwlog_err("%s: oled g_fbinfo is NULL\n", __func__);
		return NULL;
	}
	return (struct oledfb_par *)g_fbinfo->par;
}

static int oled_spi_gpio_enable(uint32_t gpio_num, const char *label)
{
	int ret;
	ret = gpio_request(gpio_num, label);
	if (ret) {
		hwlog_err("%s: gpio_oled_enable = %d request failed!\n",
			__func__, gpio_num);
		return ret;
	}
	ret = gpio_direction_output(gpio_num, OLED_SPI_GPIO_HIGH);
	if (ret)
		hwlog_err("%s: gpio_direction_output %d failed!\n",
			__func__, gpio_num);
	return ret;
}

static int oled_spi_gpio_disable(uint32_t gpio_num)
{
	int ret;
	ret = gpio_direction_output(gpio_num, OLED_SPI_GPIO_LOW);
	if (ret)
		hwlog_err("%s: gpio_direction_output %d failed!\n",
			__func__, gpio_num);
	gpio_free(gpio_num);
	return ret;
}

static int oled_spi_code_parse(void)
{
	int count;
	u8 buf;
	int sz = ARRAY_SIZE(initseq_edo);
	int ret = 0;

	for (count = 0; count < sz; count++) {
		buf = initseq_edo[count].value;
		if (initseq_edo[count].format == 'c') {
			/* command */
			ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
			if (ret) {
				hwlog_err("%s: oled transfer cmd error", __func__);
				return ret;
			}
		} else if (initseq_edo[count].format == 'd') {
			/* data */
			oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_DATA_MODE);
			if (ret) {
				hwlog_err("%s:oled transfer data error", __func__);
				return ret;
			}
		} else if (initseq_edo[count].format == 'w') {
			mdelay((unsigned int)(initseq_edo[count].value));
		}
	}

	return 0;
}

#ifdef CONFIG_DPU_FB_AP_AOD
static int oled_spi_aod_on(void)
{
	int ret = -1;
	u8 buf = OLED_SPI_PAGE_REG;
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return ret;

	if (atomic_read(&par->oled_aod_state) == OLED_SPI_PWR_ON) {
		hwlog_info("panel is aod on, do nothing.\n");
		return 0;
	}
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		return ret;
	buf = OLED_SPI_PAGE_VAL;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_DATA_MODE);
	if (ret)
		return ret;
	buf = OLED_SPI_IDLE_IN;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		return ret;
	atomic_set(&par->oled_aod_state, OLED_SPI_PWR_ON);

	return 0;
}

static int oled_spi_aod_off(void)
{
	int ret = -1;
	u8 buf = OLED_SPI_PAGE_REG;
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return ret;

	if (atomic_read(&par->oled_aod_state) == OLED_SPI_PWR_OFF) {
		hwlog_info("panel is aod off, do nothing.\n");
		return 0;
	}
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		return ret;
	buf = OLED_SPI_PAGE_VAL;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_DATA_MODE);
	if (ret)
		return ret;
	buf = OLED_SPI_IDLE_OUT;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		return ret;
	atomic_set(&par->oled_aod_state, OLED_SPI_PWR_OFF);

	return 0;
}
#endif

int oled_spi_set_brightness(u8 backlight)
{
	int ret;
	u8 brightness_cmd = OLED_SPI_BRIGHTNESS_REG;

	/* set DCX 0, cmd mode */
	ret = oled_data_cmd_transfer(&brightness_cmd, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret) {
		hwlog_err("%s:oled transfer brightness cmd error", __func__);
		return ret;
	}
	ret = oled_data_cmd_transfer(&backlight, OLED_SPI_CMD_LEN, DCX_DATA_MODE);
	if (ret) {
		hwlog_err("%s:oled transfer brightness data error", __func__);
		return ret;
	}

	hwlog_info("%s: oled set brightness success, current brightness:%u!\n",
		__func__, backlight);
	return 0;
}

static u32 oled_spi_fb_line_length(u32 xres, u32 bpp)
{
	 /*
	  * The adreno GPU hardware requires that the pitch be aligned to
	  * 32 pixels for color buffers, so for the cases where the GPU
	  * is writing directly to fb0, the framebuffer pitch
	  * also needs to be 32 pixel aligned
	  */
	return (u32)(xres * bpp);
}

static s32 oled_spi_fb_fill(struct fb_info *info, struct oledfb_par *par)
{
	struct fb_var_screeninfo *var = &info->var;

	switch (par->fb_img_type) {
	case MDP_RGB_565:
		info->fix.type = FB_TYPE_PACKED_PIXELS;
		info->fix.xpanstep = 1;
		info->fix.ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;
		var->blue.offset = 0;
		var->green.offset = BITS_OFFSET_5;
		var->red.offset = BITS_OFFSET_11;
		var->blue.length = BITS_5;
		var->green.length = BITS_6;
		var->red.length = BITS_5;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->bits_per_pixel = PIXEL_BIT_RGB565;
		break;
	case MDP_RGB_888:
		info->fix.type = FB_TYPE_PACKED_PIXELS;
		info->fix.xpanstep = 1;
		info->fix.ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;
		var->blue.offset = 0;
		var->green.offset = BITS_OFFSET_8;
		var->red.offset = BITS_OFFSET_16;
		var->blue.length = BITS_8;
		var->green.length = BITS_8;
		var->red.length = BITS_8;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->bits_per_pixel = PIXEL_BIT_RGB888;
		break;
	case MDP_ARGB_8888:
		info->fix.type = FB_TYPE_PACKED_PIXELS;
		info->fix.xpanstep = 1;
		info->fix.ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;
		var->blue.offset = 0;
		var->green.offset = BITS_OFFSET_8;
		var->red.offset = BITS_OFFSET_16;
		var->blue.length = BITS_8;
		var->green.length = BITS_8;
		var->red.length = BITS_8;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.offset = BITS_OFFSET_24;
		var->transp.length = BITS_8;
		var->bits_per_pixel = PIXEL_BIT_RGB8888;
		break;
	case MDP_RGBA_8888:
		info->fix.type = FB_TYPE_PACKED_PIXELS;
		info->fix.xpanstep = 1;
		info->fix.ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;
		var->blue.offset = BITS_OFFSET_8;
		var->green.offset = BITS_OFFSET_16;
		var->red.offset = BITS_OFFSET_24;
		var->blue.length = BITS_8;
		var->green.length = BITS_8;
		var->red.length = BITS_8;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = BITS_8;
		var->bits_per_pixel = PIXEL_BIT_RGB8888;
		break;
	case MDP_YCRYCB_H2V1:
		info->fix.type = FB_TYPE_INTERLEAVED_PLANES;
		info->fix.xpanstep = STEP_TWO;
		info->fix.ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;
		var->blue.offset = 0;
		var->green.offset = BITS_OFFSET_5;
		var->red.offset = BITS_OFFSET_11;
		var->blue.length = BITS_5;
		var->green.length = BITS_6;
		var->red.length = BITS_5;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->bits_per_pixel = PIXEL_BIT_RGB565;
		break;
	default:
		hwlog_err("oled_fb_init: fb %d unkown image type!\n", info->node);
		return -1;
	}
	return 0;
}

static int oled_spi_dts_parse(void)
{
	int ret = -1;
	struct device_node *dt = NULL;
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return ret;

	atomic_set(&par->oled_pwr_state, OLED_SPI_PWR_ON);
	dt = of_find_compatible_node(NULL, NULL, DTS_COMP_OLED);
	if (!dt) {
		hwlog_err("%s: not found device node %s!\n",
			__func__, DTS_COMP_OLED);
		return ret;
	}
	par->gpio_oled_rst = of_get_named_gpio(dt, "gpio-reset", 0);
	if (par->gpio_oled_rst < 0) {
		hwlog_err("%s: not found device gpio rst!\n", __func__);
		return ret;
	}
	par->gpio_oled_dcx = of_get_named_gpio(dt, "gpio-dcx", 0);
	if (par->gpio_oled_dcx < 0) {
		hwlog_err("%s: not found device gpio dcx!\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(dt, "smurfs_oled_id", &par->smurfs_oled_id);
	if (ret) {
		hwlog_err("%s:get oled id failed!\n", __func__);
		return ret;
	}
	hwlog_info("%s: gpio_oled_rst:%d, gpio_oled_dcx:%d, smurfs_oled_id:%d\n",
		__func__, par->gpio_oled_rst, par->gpio_oled_dcx,
		par->smurfs_oled_id);
	return 0;
}

int oled_spi_set_dcx(int status)
{
	int ret = -1;
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return ret;

	/* DCX: 0, cmd mode; DCX: 1, data mode */
	ret = gpio_direction_output(par->gpio_oled_dcx, status);
	if (ret) {
		hwlog_err("%s: set spi dcx error!\n", __func__);
		return ret;
	}

	return 0;
}

static s32 oled_spi_light(struct fb_info *info)
{
	s32 ret = -1;
	u8 buf = OLED_SPI_SLEEP_OUT;
	struct oledfb_par *par = (struct oledfb_par *)info->par;
	if (!par)
		return ret;

	hwlog_info("oled will be light!\n");

	down(&(par->g_screensemmux));
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		goto err_out;

	msleep(SLEEP_TIME_120MS); /* wait for oled sleep out */
	buf = OLED_SPI_DISPLAY_ON;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		goto err_out;

	msleep(SLEEP_TIME_120MS); /* to avoid screen white flash */
	up(&(par->g_screensemmux));

	return 0;

err_out:
	hwlog_err("transfer sleep out cmd failed!\n");
	up(&(par->g_screensemmux));
	return ret;
}

static s32 oled_spi_sleep(struct fb_info *info)
{
	s32 ret;
	u8 buf = OLED_SPI_DISPLAY_OFF;
	struct oledfb_par *par = (struct oledfb_par *)info->par;
	if (!par) {
		hwlog_err("par NULL Pointer\n");
		return -EINVAL;
	}

	hwlog_info("oled will be sleep!\n");

	down(&(par->g_screensemmux));
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		goto err_out;

	buf = OLED_SPI_SLEEP_IN;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		goto err_out;

	mdelay(SLEEP_TIME_120MS); /* OLED sleep delay */
	up(&(par->g_screensemmux));

	return 0;

err_out:
	hwlog_err("transfer sleep int cmd failed!\n");
	up(&(par->g_screensemmux));
	return ret;
}

static int oled_spi_disp_open(struct fb_info *info, int user)
{
	if (!info) {
		hwlog_err("info NULL Pointer\n");
		return -EINVAL;
	}

	struct oledfb_par *par = (struct oledfb_par *)info->par;
	if (!par) {
		hwlog_err("par NULL Pointer\n");
		return -EINVAL;
	}

	if (!par->ref_cnt)
		hwlog_info("first open fb %d\n", info->node);
	par->ref_cnt++;
	return 0;
}

static int oled_spi_disp_release(struct fb_info *info, int user)
{
	if (!info) {
		hwlog_err("info NULL Pointer\n");
		return -EINVAL;
	}

	struct oledfb_par *par = (struct oledfb_par *)info->par;
	if (!par) {
		hwlog_err("par NULL Pointer\n");
		return -EINVAL;
	}

	if (!par->ref_cnt) {
		hwlog_err("try to close unopened fb %d\n", info->node);
		return -EINVAL;
	}
	par->ref_cnt--;
	if (!par->ref_cnt)
		hwlog_err("last close fb %d\n", info->node);

	return 0;
}

static s32 oled_spi_disp_blank(int blank_mode, struct fb_info *info)
{
	if (!info) {
		hwlog_err("info NULL Pointer\n");
		return -EINVAL;
	}

	hwlog_info("blank_mode is %d", blank_mode);
	switch (blank_mode) {
	case VESA_NO_BLANKING: /* lcd power 1; backlight power1 */
		(void)oled_spi_light(info);
		break;
	case VESA_VSYNC_SUSPEND: /* lcd on; backlight off/sleepin */
		(void)oled_spi_sleep(info);
		break;
	case VESA_HSYNC_SUSPEND:
	case VESA_POWERDOWN: /* lcd off; backlight: off */
		oled_spi_panel_off();
		break;
	default:
		break;
	}

	return 0;
}

static int oled_spi_disp_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	u32 len;

	if (!var || !info) {
		hwlog_err("pan display var or info NULL Pointer!\n");
		return -EINVAL;
	}

	if (var->rotate != FB_ROTATE_UR)
		return -EINVAL;

	if (var->grayscale != info->var.grayscale)
		return -EINVAL;
	switch (var->bits_per_pixel) {
	case PIXEL_BIT_RGB565:
		if ((var->green.offset != BITS_OFFSET_5) || !((var->blue.offset == BITS_OFFSET_11) ||
			(var->blue.offset == 0)) || !((var->red.offset == BITS_OFFSET_11) ||
			(var->red.offset == 0)) || (var->blue.length != BITS_5) ||
			(var->green.length != BITS_6) || (var->red.length != BITS_5) ||
			(var->blue.msb_right != 0) || (var->green.msb_right != 0) ||
			(var->red.msb_right != 0) || (var->transp.length != 0) ||
			(var->transp.length != 0))
			return -EINVAL;
		break;
	case PIXEL_BIT_RGB888:
		if ((var->blue.offset != 0) || (var->green.offset != BITS_OFFSET_8) ||
			(var->red.offset != BITS_OFFSET_16) || (var->blue.length != BITS_8) ||
			(var->green.length != BITS_8) || (var->red.length != BITS_8) ||
			(var->blue.msb_right != 0) || (var->green.msb_right != 0) ||
			(var->red.msb_right != 0) || !(((var->transp.offset == 0) &&
			(var->transp.length == 0)) || ((var->transp.offset == BITS_OFFSET_24) &&
			(var->transp.length == BITS_8))))
			return -EINVAL;
		break;
	case PIXEL_BIT_RGB8888:
		 /*
		  * Figure out if the user meant RGBA or ARGB
		  * and verify the position of the RGB components
		  */
		if (var->transp.offset == BITS_OFFSET_24) {
			if ((var->blue.offset != 0) || (var->green.offset != BITS_OFFSET_8) ||
				(var->red.offset != BITS_OFFSET_16))
				return -EINVAL;
		} else if (var->transp.offset == 0) {
			if ((var->blue.offset != BITS_OFFSET_8) || (var->green.offset != BITS_OFFSET_16) ||
				(var->red.offset != BITS_OFFSET_24))
				return -EINVAL;
		} else {
			return -EINVAL;
		}
		/* Check the common values for both RGBA and ARGB */
		if ((var->blue.length != BITS_8) || (var->green.length != BITS_8) ||
			(var->red.length != BITS_8) || (var->transp.length != BITS_8) ||
			(var->blue.msb_right != 0) || (var->green.msb_right != 0) ||
			(var->red.msb_right != 0))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	if ((var->xres_virtual == 0) || (var->yres_virtual == 0))
		return -EINVAL;
	len = var->xres_virtual * var->yres_virtual * (var->bits_per_pixel / BITS_8);
	if (len > info->fix.smem_len)
		return -EINVAL;
	if ((var->xres == 0) || (var->yres == 0))
		return -EINVAL;
	if (var->xoffset > (var->xres_virtual - var->xres))
		return -EINVAL;
	if (var->yoffset > (var->yres_virtual - var->yres))
		return -EINVAL;
	return 0;
}

static int oled_spi_set_pixel(struct fb_info *info)
{
	if (!info) {
		hwlog_err("info NULL Pointer\n");
		return -EINVAL;
	}

	struct oledfb_par *par = (struct oledfb_par *)(info->par);
	if (!par)
		return -EINVAL;

	struct fb_var_screeninfo *var = &info->var;
	switch (var->bits_per_pixel) {
	case PIXEL_BIT_RGB565:
		if (var->red.offset == 0)
			par->fb_img_type = MDP_BGR_565;
		else
			par->fb_img_type = MDP_RGB_565;
		break;
	case PIXEL_BIT_RGB888:
		if ((var->transp.offset == 0) && (var->transp.length == 0)) {
			par->fb_img_type = MDP_RGB_888;
		} else if ((var->transp.offset == BITS_OFFSET_24) &&
			(var->transp.length == BITS_8)) {
			par->fb_img_type = MDP_ARGB_8888;
			info->var.bits_per_pixel = PIXEL_BIT_RGB8888;
		}
		break;
	case PIXEL_BIT_RGB8888:
		if (var->transp.offset == BITS_OFFSET_24)
			par->fb_img_type = MDP_ARGB_8888;
		else
			par->fb_img_type = MDP_RGBA_8888;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int oled_spi_disp_set_par(struct fb_info *info)
{
	if (!info) {
		hwlog_err("info NULL Pointer\n");
		return -EINVAL;
	}

	struct oledfb_par *par = (struct oledfb_par *)(info->par);
	if (!par)
		return -EINVAL;

	struct fb_var_screeninfo *var = &info->var;
	u32 old_img_type;
	int blank = 0;
	int ret;

	old_img_type = par->fb_img_type;
	ret = oled_spi_set_pixel(info);
	if (ret)
		return ret;
	if ((par->var_pixclock != var->pixclock) ||
		(par->hw_refresh && ((par->fb_img_type != old_img_type) ||
		(par->var_pixclock != var->pixclock) ||
		(par->var_xres != var->xres) ||
		(par->var_yres != var->yres)))) {
		par->var_xres = var->xres;
		par->var_yres = var->yres;
		par->var_pixclock = var->pixclock;
		blank = 1;
	}
	par->info->fix.line_length = oled_spi_fb_line_length(var->xres,
		var->bits_per_pixel / BITS_8);
	if (blank) {
		oled_spi_disp_blank(VESA_POWERDOWN, info);
		oled_spi_panel_on();
	}
	return 0;
}

static s32 oled_spi_refresh(struct fb_info *info)
{
	s32 ret;
	struct oledfb_par *par = info->par;
	struct fb_var_screeninfo var = info->var;
	struct fb_fix_screeninfo fix = info->fix;

	u8 *buf = info->screen_base + var.yoffset * fix.line_length +
		var.xoffset ; /* for spi */
	u32 len = (fix.line_length) * (var.yres);

	down(&(par->g_screensemmux));

	ret = oled_display_transfer(buf, len);
	if (ret)
		hwlog_err("%s: oled_display_transfer error", __func__);

	up(&(par->g_screensemmux));

	return ret;
}

static int oled_spi_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	static int is_first_dispaly = 1;

	if (!var || !info) {
		hwlog_err("pan display var or info NULL Pointer!\n");
		return -EINVAL;
	}

	if (var->xoffset > (info->var.xres_virtual - info->var.xres)) {
		hwlog_err("var.xoffset is unnormal and the value is %x\n",
			var->xoffset);
		return -EINVAL;
	}
	if (var->yoffset > (info->var.yres_virtual - info->var.yres)) {
		hwlog_err("var.yoffset is unnormal and the value is %x\n",
			var->yoffset);
		return -EINVAL;
	}
	if (info->fix.xpanstep)
		info->var.xoffset = (var->xoffset / info->fix.xpanstep) *
			info->fix.xpanstep;

	if (info->fix.ypanstep)
		info->var.yoffset = (var->yoffset / info->fix.ypanstep) *
			info->fix.ypanstep;
	/*
	 * Do not initialize lcd until app fresh.
	 * Keep the fastboot static image before boot animation.
	 */
	if (is_first_dispaly) {
		is_first_dispaly = 0;
		hwlog_info("oled:the oled is display for the first time!\n");
	}
	oled_spi_refresh(info);

	return 0;
}

static struct fb_ops oledfb_ops = {
	.owner        = THIS_MODULE,
	.fb_open = oled_spi_disp_open,
	.fb_release = oled_spi_disp_release,
	.fb_blank   = oled_spi_disp_blank,
	.fb_check_var = oled_spi_disp_check_var,
	.fb_set_par = oled_spi_disp_set_par,
	.fb_pan_display = oled_spi_pan_display,
};

int oled_spi_register_fb(void)
{
	int ret = 0;

	if (!g_fbinfo) {
		hwlog_err("%s: oled g_fbinfo is NULL\n", __func__);
		return -ENOMEM;
	}
	ret = register_framebuffer(g_fbinfo);
	if (ret)
		hwlog_err("%s: oled fb register failed !", __func__);

	return ret;
}

#ifdef CONFIG_LCD_KIT_DRIVER
struct lcd_callback on_func_cb[] = {
	{ .func = lcd_kit_spi_on, },
	{ .func = lcd_kit_spi_on, },
};
struct lcd_callback off_func_cb[] = {
	{ .func = lcd_kit_spi_off, },
	{ .func = lcd_kit_spi_off, },
};
struct lcd_callback backlight_cb[] = {
	{ .func = lcd_kit_spi_set_backlight, },
};

static void oled_init_lcd_kit_cb(void)
{
	uint32_t i;
	for (i = OLED_SPI_PANEL_ID; i < OLED_SPI_PANEL_NUM; i++) {
		lcd_kit_regist_postproc(i, LCD_KIT_FUNC_ON, &on_func_cb[i]);
		lcd_kit_regist_preproc(i, LCD_KIT_FUNC_OFF, &off_func_cb[i]);
	}
	lcd_kit_regist_preproc(OLED_SPI_PANEL_ID, LCD_KIT_FUNC_SET_BACKLIGHT,
		&backlight_cb[OLED_SPI_PANEL_ID]);
}
#endif

static s32 __init oled_spi_disp_probe(struct platform_device *pdev)
{
	struct oledfb_par *par;
	struct fb_var_screeninfo *var;
	size_t oled_size;
	s32 ret;

	if (!pdev) {
		hwlog_err("%s: pdev is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = oled_spi_init();
	if (ret) {
		hwlog_err("%s: oled spi init error", __func__);
		return ret;
	}

	g_fbinfo = framebuffer_alloc(sizeof(struct oledfb_par), &pdev->dev);
	if (!g_fbinfo) {
		hwlog_err("%s: oled framebuffer alloc failed\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, g_fbinfo);
	par = (struct oledfb_par *)g_fbinfo->par;
	sema_init(&(par->g_screensemmux), OLED_SPI_SEM_FULL);
	par->fb_page = OLED_SPI_FB_NUM; /* dual buffer support */
	par->fb_img_type = MDP_RGB_565;
	par->ref_cnt = 0;
	g_fbinfo->fbops = &oledfb_ops;

	ret = oled_spi_fb_fill(g_fbinfo, par);
	if (ret) {
		hwlog_err("%s: oled fb fill error!\n", __func__);
		goto exit;
	}

	var = &g_fbinfo->var;
	var->xres = OLED_SPI_WIDTH;
	var->yres = OLED_SPI_HIGHT;
	var->xres_virtual = OLED_SPI_WIDTH;
	var->yres_virtual = var->yres * par->fb_page;

	par->var_xres = var->xres;
	par->var_yres = var->yres;
	par->var_pixclock = var->pixclock;

	g_fbinfo->fix.line_length = oled_spi_fb_line_length(var->xres, var->bits_per_pixel / 8);
	oled_size = g_fbinfo->fix.line_length * var->yres * par->fb_page;
	par->vmem = kmalloc(oled_size, GFP_KERNEL | GFP_DMA);
	if (!par->vmem) {
		hwlog_err("%s: unable to alloc fbmem size = %zu\n",
			__func__, oled_size);
		goto exit;
	}

	g_fbinfo->screen_base = (char *)(par->vmem);
	g_fbinfo->fix.smem_start = virt_to_phys(par->vmem);
	g_fbinfo->fix.smem_len = oled_size;
	g_fbinfo->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;
	par->info = g_fbinfo;
	par->pdev = pdev;
	hwlog_info("fb%d: %s frame buffer device,\n\tusing %d Byte of video memory\n",
		g_fbinfo->node, g_fbinfo->fix.id, g_fbinfo->fix.smem_len);

	ret = oled_spi_dts_parse();
	if (ret) {
		hwlog_err("%s: oled dts parse failed\n", __func__);
		goto probe_err;
	}
	ret = oled_spi_gpio_enable(par->gpio_oled_dcx, "OLED_SPI_DCX");
	if (ret) {
		hwlog_err("oled dcx(gpio:%d) enable failed\n", par->gpio_oled_dcx);
		goto probe_err;
	}

#ifdef CONFIG_LCD_KIT_DRIVER
	oled_init_lcd_kit_cb();
#endif
	return 0;

probe_err:
	kfree(par->vmem);
exit:
	platform_set_drvdata(pdev, NULL);
	framebuffer_release(g_fbinfo);
	return ret;
}

static s32 oled_spi_disp_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);

	if (!info) {
		hwlog_err("%s %d platform_get_drvdata info is NULL\n",
			__func__, __LINE__);
		return -1;
	}
	platform_set_drvdata(pdev, NULL);
	unregister_framebuffer(info);
	kfree(info->screen_base);
	framebuffer_release(info);

	return 0;
}

int oled_spi_panel_off(void)
{
	int ret = -1;
	u8 buf = OLED_SPI_DISPLAY_OFF;
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return ret;

	if (atomic_read(&par->oled_pwr_state) == OLED_SPI_PWR_OFF) {
		hwlog_info("panel is power off, do nothing.\n");
		return 0;
	}
	hwlog_info("%s:fb4 +\n", __func__);

	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		return ret;
	buf = OLED_SPI_SLEEP_IN;
	ret = oled_data_cmd_transfer(&buf, OLED_SPI_CMD_LEN, DCX_CMD_MODE);
	if (ret)
		return ret;
	mdelay(SLEEP_TIME_120MS);
	ret = oled_spi_gpio_disable(par->gpio_oled_rst);
	if (ret)
		return ret;

	atomic_set(&par->oled_pwr_state, OLED_SPI_PWR_OFF);
	return 0;
}

int oled_spi_panel_on(void)
{
	int ret = -1;
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return ret;

	if (atomic_read(&par->oled_pwr_state) == OLED_SPI_PWR_ON) {
		hwlog_info("panel is power on, do nothing.\n");
		return 0;
	}
	hwlog_info("%s:fb4 +\n", __func__);

	ret = oled_spi_gpio_enable(par->gpio_oled_rst, "MOUTH_OLED_RST");
	if (ret)
		return ret;

	mdelay(SLEEP_TIME_15MS); /* wait 15ms for reset */

	/* transfer init code */
	if (par->smurfs_oled_id == OLED_SPI_EDO_ID) {
		ret = oled_spi_code_parse();
		if (ret) {
			hwlog_err("%s: oled_spi_code_parse failde", __func__);
			return ret;
		}
	}

	atomic_set(&par->oled_pwr_state, OLED_SPI_PWR_ON);
	return 0;
}

#ifdef CONFIG_LCD_KIT_DRIVER
static int lcd_kit_spi_on(struct platform_device *pdev,
		struct callback_data *data)
{
#ifdef CONFIG_DPU_FB_AP_AOD
	struct oledfb_par *par = oled_spi_get_fbinfo_par();
	if (!par)
		return LCD_KIT_FAIL;

	if (atomic_read(&par->oled_aod_state) == OLED_SPI_PWR_ON) {
		if (oled_spi_aod_off())
			return LCD_KIT_FAIL;
		return LCD_KIT_OK;
	}
#endif
	if (lcd_kit_get_alpm_setting_status() == ALPM_DISPLAY_OFF ||
		lcd_kit_get_alpm_setting_status() == ALPM_EXIT) {
		if (oled_spi_panel_on())
			return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

static int lcd_kit_spi_off(struct platform_device *pdev,
		struct callback_data *data)
{
#ifdef CONFIG_DPU_FB_AP_AOD
	if (lcd_kit_get_alpm_setting_status() == ALPM_ON_MIDDLE_LIGHT) {
		if (oled_spi_aod_on())
			return LCD_KIT_FAIL;
		return LCD_KIT_OK;
	}
#endif
	if (oled_spi_panel_off())
		return LCD_KIT_FAIL;
	return LCD_KIT_OK;
}

static int lcd_kit_spi_set_backlight(struct platform_device *pdev,
		struct callback_data *data)
{
	uint32_t brightness;
	if (!data || !data->data) {
		hwlog_err("invalid data, return.\n");
		return LCD_KIT_FAIL;
	}
	if (lcd_kit_get_alpm_setting_status() == ALPM_ON_MIDDLE_LIGHT)
		return LCD_KIT_OK;
	brightness = *(uint32_t *)data->data;
	if (oled_spi_set_brightness((u8)brightness))
		return LCD_KIT_FAIL;
	return LCD_KIT_OK;
}
#endif

static const struct of_device_id kirin_oled_of_match[] = {
	{ .compatible = DTS_COMP_OLED, },
	{},
};

static struct platform_driver oledfb_driver = {
	.driver = {
		.name   = "oled_spi_display",
		.owner  = THIS_MODULE,
		.pm     = NULL,
		.of_match_table = of_match_ptr(kirin_oled_of_match),
	},
	.probe  = oled_spi_disp_probe,
	.remove = oled_spi_disp_remove,
};

static int __init oled_spi_disp_init(void)
{
	return platform_driver_register(&oledfb_driver);
}

static void __exit oled_spi_disp_exit(void)
{
	platform_driver_unregister(&oledfb_driver);
}

late_initcall(oled_spi_disp_init);
module_exit(oled_spi_disp_exit);
MODULE_DESCRIPTION("SPI OLED DISPLAY DRIVER");
MODULE_LICENSE("GPL V2");
