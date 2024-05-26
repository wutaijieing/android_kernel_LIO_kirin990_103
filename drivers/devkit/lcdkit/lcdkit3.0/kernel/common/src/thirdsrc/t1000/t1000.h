/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: for t1000 driver ic
 */
/*
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
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

#ifndef _T1000_H_
#define _T1000_H_

#include <linux/i2c-dev.h>

#define T1000_ALLOW_SUSPEND      0
#define T1000_ALLOW_RESUME       1

#define T1000_READY 1
#define T1000_BUSY  0

#define T1000_EINK_INFO_LEN	128

enum {
	T1000_REG_CMD         = 0x10,
	T1000_REG_DATA        = 0x34,
};

/* custom defined */
enum {
	UCMD_GET_MIPI_MODE  = 0x60,
	UCMD_SET_MIPI_MODE  = 0x61,
	UCMD_SET_COMMAND_CODE  = 0x67,
};

#define	T1000_SLEEP_MODE     0x0F

/* power state */
enum {
	POWEROFF = 0x1,
	POWERON = 0x2,
	ACTIVE = 0x4,
	SLEEP = 0x8,
};

struct t1000_command {
	uint8_t  reg;
	uint16_t command_code;
	uint16_t reserved;

	union args {
		struct mipi_mode {
			uint16_t mode;
			uint16_t sub_mode;
		}mipi_mode;
		struct data {
			uint16_t far_data;
			uint16_t sub_data;
		}data;
		uint16_t uartlogen;
		uint16_t fboot_direction;
	} args;
} __packed;

struct t1000_data {
	struct i2c_client       *client;
	struct device           *dev;
	struct regulator_desc   rdesc;
	struct regulator_dev    *rdev;
	struct mutex            ioctl_mutex;
	struct mutex            pm_mutex;
	struct regulator        *vgen_regulator;

	int                     gpio_ready;
	int                     gpio_wakeup; /* 0:wake t1000 from sleep mode */
	int                     gpio_wake_source; /* 0:wake t1000 from sleep mode */
	int                     gpio_reset;
	int                     gpio_fastboot; /* 0:fastboot 1:firstboot */
	int                     gpio_3v3_en; /* 1:power for t1000 */
	int                     gpio_test_cfg;

	int                     disp_ready; /* display ready in fastread or read mode 1:ready */
	int                     hw_spi_ready; /* spi transfer ready 1:ready */
	int                     rdev_enabled;
	int                     vcom;

	int                     mipimode;
	int                     pending_set_mode;

	int                     power_state;

	atomic_t                pm_cnt;
	int                     autopm_enable;
	bool                    bpower_alwayson;
	char                    eink_info[T1000_EINK_INFO_LEN];
};

struct t1000_gpio_config {
	int gpio;
	int value;
};

static struct device *t1000dev;

struct t1000cdev {
	unsigned int major;
	struct class *cls;
	struct device *dev;
	int data;
};
static struct t1000cdev t1000_cdev;
static char *t1000_chrname = "t1000cdev";
#define T1000CDEV_MAJOR     50


/* ioctl commds */
#define T1000_IOCTL_MAGIC_NUMBER 'F'

#define T1000_WAIT_DISPLAY_READY _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x2B, int32_t)
#define T1000_WAIT_HW_SPI_READY _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x2C, int32_t)
#define T1000_SET_POWER_ONOFF _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x2D, int32_t)
#define T1000_SET_SLEEP_WAKEUP _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x2E, int32_t)
#define T1000_GET_POWER_STATE _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x2F, int32_t)
#define T1000_OPEN_T1000LOG _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x30, int32_t)
#define T1000_SET_IGNOREMODE _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x31, int32_t)
#define T1000_GET_HW_ENGINE_STATUS _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x32, int32_t)
#define T1000_SET_WAKEUP_SOURCE _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x33, int32_t)
#define T1000_I2C_RDWR  _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x34, struct i2c_rdwr_ioctl_data)
#define T1000_GPIO_CTRL  _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x35, struct t1000_gpio_config)
#define T1000_SEND_PEN_STATUS _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x36, bool)
#define T1000_SET_UPGRADE_STATUS _IOW(T1000_IOCTL_MAGIC_NUMBER, 0x37, int32_t)
#define T1000_READ_POWERON_TIMING _IOWR(T1000_IOCTL_MAGIC_NUMBER, 0x38, int32_t)
#endif // _T1000_H_
