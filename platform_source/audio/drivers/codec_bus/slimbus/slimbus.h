/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#ifndef __SLIMBUS_H__
#define __SLIMBUS_H__

#include <linux/types.h>
#include <sound/asound.h>

#include "codec_bus.h"
#include "codec_bus_inner.h"
#include "slimbus_types.h"

#define SLIMBUS_TRACK_ERROR 0xFFFFFFFF

#define SLIMBUS_DRV_ADDRESS 0x400

/* slimbus can attach more than one devices */
enum slimbus_device {
	SLIMBUS_DEVICE_DA_COMBINE_V3 = 0,
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	SLIMBUS_DEVICE_DA_COMBINE_V5,
#endif
	SLIMBUS_DEVICE_NUM,
};

enum slimbus_bus_type {
	SLIMBUS_BUS_CONFIG_NORMAL          = 0,
	SLIMBUS_BUS_CONFIG_IMGDOWN         = 1,
	SLIMBUS_BUS_CONFIG_SWITCH_FRAMER   = 2,
	SLIMBUS_BUS_CONFIG_REGIMGDOWN      = 3,
	SLIMBUS_BUS_CONFIG_MAX,
};

struct slimbus_sound_trigger_params {
	uint32_t channels;
	uint32_t sample_rate;
	uint32_t track_type;
};

struct select_scene {
	enum slimbus_scene_config_type scene_config_type;
	enum slimbus_clock_gear cg;
	enum slimbus_subframe_mode sm;
	bool (*select_scene)(uint32_t active_tracks);
};

struct slimbus_track_config {
	struct scene_param params;
	struct slimbus_channel_property *channel_pro;
};

struct slimbus_private_data {
	struct device                *dev;
	void __iomem                 *asp_cfg_mem;
	void __iomem                 *sctrl_mem;
	void __iomem                 *slimbus_mem;
	uint32_t                     portstate;
	enum framer_type             framerstate;
	enum framer_type             lastframer;
	enum platform_type           type;
	enum slimbus_device          device_type;
	bool                         slimbus_dynamic_freq_enable;
	struct slimbus_device_ops    *dev_ops;
	struct slimbus_track_config  *track_config_table;
	uint32_t                     slimbus_track_max;
	bool                         switch_framer_disable;
	bool                         *track_state;
	bool                         freq_update_disable;
};

struct slimbus_device_ops {
	int32_t (*create_slimbus_device)(struct slimbus_device_info **device);
	void (*release_slimbus_device)(struct slimbus_device_info *device);
	void (*slimbus_device_param_init)(const struct slimbus_device_info *dev);
	int32_t (*slimbus_device_param_set)(struct slimbus_device_info *dev,
		uint32_t track_type, struct scene_param *params);
	void (*slimbus_device_param_update)(const struct slimbus_device_info *dev,
		uint32_t track_type, const struct scene_param *params);
	void (*slimbus_get_soundtrigger_params)(struct slimbus_sound_trigger_params *params);

	int32_t (*slimbus_track_soundtrigger_activate)(uint32_t track, bool slimbus_dynamic_freq_enable,
		struct slimbus_device_info *dev, struct scene_param *params);

	int32_t (*slimbus_track_soundtrigger_deactivate)(uint32_t track);
	bool (*slimbus_track_is_fast_soundtrigger)(uint32_t track);

	int32_t (*slimbus_check_scenes)(uint32_t track,
		uint32_t scenes, bool track_enable);
	int32_t (*slimbus_select_scenes)(struct slimbus_device_info *dev,
		uint32_t track, const struct scene_param *params, bool track_enable);
};

/*
 * setup channel, this step should be done by sending CONNECT_SOURCE,
 * CONNECT_SINK, NEXT_DEFINE_CHANNEL, NEXT_DEFINE_CONTENT NEXT_ACTIVATE_CHANNEL messages
 * @track, track type
 * @params, pcm parameters
 *
 * return 0 if successful, otherwise, failed
 */
extern int32_t slimbus_activate_track(uint32_t track, struct scene_param *params);


/*
 * de-activate channel, this step should be done by sending
 * NEXT_DEACTIVATE_CHANNEL NEXT_REMOVE_CHANNEL message
 * @track,	track type
 *
 * return 0 if successful, otherwise, failed
 */
extern int32_t slimbus_deactivate_track(uint32_t track, struct scene_param *params);

/*
 * switch framer, this step should be done by sending
 * NEXT_ACTIVE_FRAMER message
 * @framer_type, which device switch to
 *
 * return 0 if successful, otherwise, failed
 */
extern int32_t slimbus_switch_framer(enum framer_type framer_type);

/*
 * pause clock, this step should be done by sending
 * NEXT_PAUSE_CLOCK message
 * @newrestarttime restart time control flag.
 *
 * return 0 if successful, otherwise, failed
 */
extern int32_t slimbus_pause_clock(enum slimbus_restart_time newrestarttime);

extern int32_t slimbus_track_recover(void);

extern enum slimbus_device slimbus_debug_get_device_type(void);
extern uint32_t slimbus_trackstate_get(void);
extern void slimbus_trackstate_set(uint32_t track, bool state);
extern bool slimbus_track_state_is_on(uint32_t track);
extern struct slimbus_bus_config *slimbus_get_bus_config(enum slimbus_bus_type type);
extern struct slimbus_track_config *slimbus_get_track_config_table(void);
extern uint32_t slimbus_read_4byte(uint32_t reg);
extern uint32_t slimbus_read_1byte(uint32_t reg);
extern void slimbus_write_1byte(uint32_t reg, uint32_t val);
extern void slimbus_write_4byte(uint32_t reg, uint32_t val);
extern void slimbus_read_pageaddr(void);
extern int32_t slimbus_bus_configure(enum slimbus_bus_type type);
extern void slimbus_logcount_set(uint32_t count);
extern uint32_t slimbus_logcount_get(void);
extern uint32_t slimbus_logtimes_get(void);
extern void slimbus_logtimes_set(uint32_t times);
extern volatile uint32_t slimbus_drv_lostms_get(void);
extern void slimbus_drv_lostms_set(uint32_t count);
extern struct slimbus_device_info *slimbus_get_devices_info(void);
extern void slimbus_clear_port_fifo(uint8_t port);

#endif /* __SLIMBUS_H__ */

