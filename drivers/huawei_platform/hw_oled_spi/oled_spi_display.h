// SPDX-License-Identifier: GPL-2.0
/*
 * oled_spi_display.h
 *
 * driver for oled spi display
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

#ifndef _OLED_SPI_DISPLAY_H
#define _OLED_SPI_DISPLAY_H


#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#define OLED_SPI_SEM_FULL      (1)
#define OLED_SPI_SEM_EMPTY     (0)
#define OLED_SPI_GPIO_LOW       0
#define OLED_SPI_GPIO_HIGH      1
#define OLED_SPI_HIGHT          466
#define OLED_SPI_WIDTH          466
#define OLED_SPI_MSG_MAXSZ      (OLED_SPI_HIGHT * OLED_SPI_WIDTH * 4)
#define OLED_SPI_EDO_ID         0
#define OLED_SPI_CMD_LEN        1
#define OLED_SPI_FB_NUM         2
#define OLED_SPI_PWR_ON         1
#define OLED_SPI_PWR_OFF        0
#define OLED_SPI_PANEL_NUM      2
#define OLED_SPI_PANEL_ID       0
#define OLED_SPI_PAGE_REG       0xfe
#define OLED_SPI_PAGE_VAL       0x00
#define OLED_SPI_IDLE_IN        0x39
#define OLED_SPI_IDLE_OUT       0x38
#define OLED_SPI_MEMORY_WRITE   0x2c
#define OLED_SPI_SLEEP_IN       0x10
#define OLED_SPI_SLEEP_OUT      0x11
#define OLED_SPI_DISPLAY_ON     0x29
#define OLED_SPI_DISPLAY_OFF    0x28
#define OLED_SPI_BRIGHTNESS_REG 0x51

#define PIXEL_BIT_RGB565        16
#define PIXEL_BIT_RGB888        24
#define PIXEL_BIT_RGB8888       32
#define BITS_5                  5
#define BITS_6                  6
#define BITS_8                  8
#define BITS_16                 16
#define BITS_OFFSET_5           5
#define BITS_OFFSET_8           8
#define BITS_OFFSET_11          11
#define BITS_OFFSET_16          16
#define BITS_OFFSET_24          24
#define TIME_NS_UINT            1000000000
#define TIME_UINT_1000          1000
#define SLEEP_TIME_15MS         15
#define SLEEP_TIME_120MS        120
#define STEP_TWO                2
#define ARG_NUM_MAX             255
#define SPI_MAX_HZ              41500000

#define DTS_COMP_OLED "hisilicon,oled_spi_display"

struct v7r2_oled {
	struct class *oled_class;
	struct oled_spidev_data *spidev_data;
};

struct hisi_oled_seq {
	u8 format;
	u8 value;
};

struct oled_spidev_data {
	dev_t devt;
	spinlock_t spi_lock;
	struct spi_device *spi;
	struct list_head device_entry;

	/* buffer is NULL unless this device is open (users > 0) */
	struct mutex buf_lock;
	struct semaphore sem_cmd;
	unsigned int users;
	u8 *buffer;
};

struct oled_panel_info {
	u32 xres;
	u32 yres;
	u32 pixclock;
	u32 is_3d_panel;
};

enum {
	DCX_CMD_MODE = 0,
	DCX_DATA_MODE,
};

enum {
	MDP_RGB_565,        /* RGB 565 planar */
	MDP_BGR_565,

	MDP_XRGB_8888,      /* RGB 888 padded */
	MDP_Y_CBCR_H2V2,    /* Y and CbCr, pseudo planar w/ Cb is in MSB */
	MDP_ARGB_8888,      /* ARGB 888 */
	MDP_RGB_888,        /* RGB 888 planar */
	MDP_Y_CRCB_H2V2,    /* Y and CrCb, pseudo planar w/ Cr is in MSB */
	MDP_YCRYCB_H2V1,    /* YCrYCb interleave */
	MDP_Y_CRCB_H2V1,    /* Y and CrCb, pseduo planar w/ Cr is in MSB */
	MDP_Y_CBCR_H2V1,    /* Y and CrCb, pseduo planar w/ Cr is in MSB */

	MDP_RGBA_8888,      /* ARGB 888 */
	MDP_BGRA_8888,      /* ABGR 888 */
	MDP_RGBX_8888,      /* RGBX 888 */
	MDP_IMGTYPE_LIMIT   /* Non valid image type after this enum */
};


struct oledfb_par {
	struct platform_device *pdev;
	struct fb_info *info;
	u32 *vmem;
	struct semaphore    g_screensemmux;
	u32 fb_img_type;
	u32 hw_refresh;
	u32 var_pixclock;
	u32 var_xres;
	u32 var_yres;
	u32 fb_page;
	struct oled_panel_info panel_info;
	int ref_cnt;
	uint32_t gpio_oled_rst;
	uint32_t gpio_oled_dcx;
	uint32_t smurfs_oled_id;
	atomic_t oled_pwr_state;
	atomic_t oled_aod_state;
};

int oled_data_cmd_transfer(u8 *tx_buf, unsigned int len, int mode);
int oled_spi_init(void);
int oled_display_transfer(u8 *tx_buf, unsigned int len);
int oled_spi_register_fb(void);
int oled_spi_set_brightness(u8 brightnes);
int oled_spi_panel_on(void);
int oled_spi_panel_off(void);
int oled_spi_set_dcx(int status);

#endif /* _OLED_SPI_DISPLAY_H */
