/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 */

#ifndef LIGHT_RING_H
#define LIGHT_RING_H

#include <linux/i2c.h>
#include <linux/leds.h>

#define LED_NUM 24
#define FRAMES_BUF_NUM 2
#define FRAMES_BUF_LEN (100 * 1024)

struct light_ring_data {
	struct i2c_client *i2c;
	struct led_classdev cdev;
	int32_t gpio_init;
	uint8_t led_brightness_data[LED_NUM];
	uint8_t led_color_data[LED_NUM];
	bool frames_loop;
	uint8_t *frames_buf[FRAMES_BUF_NUM];
	int32_t frames_idle_index;
	int32_t frames_reading_index;
	int32_t frames_num;
	bool frames_update;
};

typedef int32_t (*fn_chip_update)(struct light_ring_data *data);
typedef struct {
	fn_chip_update chip_update;
} light_ring_ext_func;

light_ring_ext_func *light_ring_ext_func_get(void);
int32_t light_ring_i2c_read(const struct i2c_client *i2c, unsigned char reg_addr);
int32_t light_ring_i2c_write_byte(const struct i2c_client *i2c, unsigned char reg_addr,
	unsigned char reg_data);
int32_t light_ring_i2c_write_bytes(const struct i2c_client *i2c, unsigned char reg_addr,
	unsigned char *buf, uint32_t len);

int32_t light_ring_init_data(struct i2c_client *i2c, struct light_ring_data *data);
void light_ring_deinit_data(struct light_ring_data *data);

int32_t light_ring_register(struct light_ring_data *data);
void light_ring_unregister(struct light_ring_data *data);

#endif /* LIGHT_RING_H */
