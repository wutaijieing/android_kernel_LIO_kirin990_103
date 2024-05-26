/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: analog headset interface driver
 * Author: lijinkui
 * Create: 2019-7-16
 */

#ifndef __ANA_HS_H__
#define __ANA_HS_H__

#include <linux/init.h>
#include <linux/device.h>
#ifdef CONFIG_ANA_HS_EXTERN_OPS_KIT
#include "ana_hs_common.h"
#else
#include "huawei_platform/audio/ana_hs_common.h"
#endif

#define DECIMAL        10

#ifdef CONFIG_ANA_HS_EXTERN_OPS_KIT
enum ana_hs_state {
	ANA_HS_PLUG_OUT = 0,
	ANA_HS_PLUG_IN,
	/* for aux switch in display port */
	DP_PLUG_IN,
	DP_PLUG_IN_CROSS,
	DP_PLUG_OUT,
	DIRECT_CHARGE_IN,
	DIRECT_CHARGE_OUT,
	POGOPIN_PLUG_IN,
	POGOPIN_PLUG_OUT
};
#endif

enum ana_hs_gpio_type {
	ANA_HS_GPIO_SOC           = 0,
	ANA_HS_GPIO_CODEC         = 1,
};

enum ana_hs_register_status {
	ANA_HS_NOT_REGISTER       = 0,
	ANA_HS_ALREADY_REGISTER   = 1,
};

enum ana_hs_priority {
	ANA_PRIORITY_L = 0,
	ANA_PRIORITY_H,
};

#ifdef CONFIG_ANA_HS_EXTERN_OPS_KIT
/* codec operate used by anc_hs driver */
struct ana_hs_codec_ops {
	bool (*check_headset_in)(void *);
	void (*plug_in_detect)(void *);
	void (*plug_out_detect)(void *);
	int (*get_headset_type)(void *);
	void (*hs_high_resistence_enable)(void *, bool);
};

struct ana_hs_codec_dev {
	char *name;
	struct ana_hs_codec_ops ops;
};
#endif

struct ana_hs_ops {
	int (*ana_hs_pluged_state)(void);
	int (*ana_hs_dev_register)(struct ana_hs_codec_dev *dev,
		void *codec_data);
	bool (*ana_hs_support_usb_sw)(void);
	void (*ana_hs_plug_handle)(int hs_state);
	void (*ana_hs_mic_gnd_swap)(void);
	void (*invert_hs_control)(int);
};

enum ana_hs_tag {
	TAG_ANA_HS_CORE = 0,
	TAG_INVERT_HS,
};

struct ana_hs_dev {
	struct list_head list;
	enum ana_hs_tag tag;
	struct ana_hs_ops ops;
};

#ifdef CONFIG_ANA_HS
void ana_hs_refresh_headset_type(int headset_type);
int ana_hs_pluged_state(void);
bool ana_hs_support_usb_sw(void);
void ana_hs_plug_handle(int hs_state);
void ana_hs_mic_gnd_swap(void);
void ana_hs_invert_hs_control(int connect);
/* these Functions is call by inner analog hs module */
void ana_hs_dev_register(struct ana_hs_dev *dev, int priority);
void ana_hs_dev_deregister(enum ana_hs_tag tag);
/* these Functions is call by extern module: MBHC */
int ana_hs_codec_dev_register(struct ana_hs_codec_dev *dev, void *codec_data);
int ana_hs_load_gpio(struct device *dev, int *gpio_index,
	int out_value, const char *gpio_name);
#else
static inline void ana_hs_refresh_headset_type(int headset_type)
{
}

static inline int ana_hs_pluged_state(void)
{
	return 0;
}

static inline bool ana_hs_support_usb_sw(void)
{
	return false;
}

static inline void ana_hs_plug_handle(int hs_state)
{
}

static inline void ana_hs_mic_gnd_swap(void)
{
}

static inline void ana_hs_invert_hs_control(int connect)
{
}

static inline void ana_hs_dev_register(struct ana_hs_dev *dev, int priority)
{
}

static inline void ana_hs_dev_deregister(enum ana_hs_tag tag)
{
}

static inline int ana_hs_codec_dev_register(struct ana_hs_codec_dev *dev, void *codec_data)
{
	return 0;
}

static inline int ana_hs_load_gpio(struct device *dev, int *gpio_index,
	int out_value, const char *gpio_name)
{
	return 0;
}

#endif

#endif // __ANA_HS_H__
