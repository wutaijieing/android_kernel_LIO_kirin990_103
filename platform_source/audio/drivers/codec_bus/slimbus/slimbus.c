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

#include "slimbus.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/consumer.h>
#include <rdr_audio_adapter.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

#include "audio_log.h"
#include "slimbus_utils.h"
#include "slimbus_drv.h"
#include "slimbus_pm.h"
#include "slimbus_da_combine.h"
#include "slimbus_da_combine_v3.h"
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
#include "slimbus_da_combine_v5.h"
#endif

#ifdef CONFIG_AUDIO_COMMON_IMAGE
#undef SOC_ACPU_SCTRL_BASE_ADDR
#undef SOC_ACPU_ASP_CFG_BASE_ADDR
#undef SOC_ACPU_SLIMBUS_BASE_ADDR
#define SOC_ACPU_SCTRL_BASE_ADDR 0
#define SOC_ACPU_ASP_CFG_BASE_ADDR 0
#define SOC_ACPU_SLIMBUS_BASE_ADDR 0
#endif

/* address of slimbus elements */
#define SLIMBUS_USER_DATA_BASE_ADDR  0x800

/* da_combine page selection register address */
#define PAGE_SELECT_REG_0 (0x1FD + (SLIMBUS_USER_DATA_BASE_ADDR))
#define PAGE_SELECT_REG_1 (0x1FE + (SLIMBUS_USER_DATA_BASE_ADDR))
#define PAGE_SELECT_REG_2 (0x1FF + (SLIMBUS_USER_DATA_BASE_ADDR))

#define READ_1BYTE_TEST_VALUE 0x5A
#define READ_4BYTE_YESY_VALUE 0x6A6A6A6A
#define RETRY_TIMES 3

#define INVALID_VAL 0xFFFFFFFF

#define LOG_TAG "slimbus"

/*lint -e838 -e730 -e747 -e774 -e826 -e529 -e438 -e485 -e785 -e651 -e64 -e527 -e570*/

struct slimbus_channel_property audio_playback[SLIMBUS_AUDIO_PLAYBACK_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property audio_capture[SLIMBUS_AUDIO_CAPTURE_MULTI_MIC_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property voice_down[SLIMBUS_VOICE_DOWN_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property voice_up[SLIMBUS_VOICE_UP_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property img_download[SLIMBUS_IMAGE_DOWNLOAD_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property ec_ref[SLIMBUS_ECREF_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property sound_trigger[SLIMBUS_SOUND_TRIGGER_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property audio_debug[SLIMBUS_DEBUG_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property audio_direct_playback[SLIMBUS_AUDIO_PLAYBACK_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property audio_fast_playback[SLIMBUS_AUDIO_PLAYBACK_CHANNELS] = {{0, {0,}, {{0,},},},};
struct slimbus_channel_property audio_kws[SLIMBUS_KWS_CHANNELS] = {{0, {0,}, {{0,},},},};

enum {
	DA_COMBINE_V3_BUS_TRACK_PLAY_ON                   = BIT(DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY),
	DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON                = BIT(DA_COMBINE_V3_BUS_TRACK_AUDIO_CAPTURE),
	DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON             = BIT(DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN),
	DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON               = BIT(DA_COMBINE_V3_BUS_TRACK_VOICE_UP),
	DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD_ON             = BIT(DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD),
	DA_COMBINE_V3_BUS_TRACK_EC_ON                     = BIT(DA_COMBINE_V3_BUS_TRACK_ECREF),
	DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON          = BIT(DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER),
	DA_COMBINE_V3_BUS_TRACK_DEBUG_ON                  = BIT(DA_COMBINE_V3_BUS_TRACK_DEBUG),
	DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON            = BIT(DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY),
	DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON              = BIT(DA_COMBINE_V3_BUS_TRACK_FAST_PLAY),
	DA_COMBINE_V3_BUS_TRACK_KWS_ON                    = BIT(DA_COMBINE_V3_BUS_TRACK_KWS),
};

static struct slimbus_private_data *pdata;
struct slimbus_device_info *slimbus_devices = NULL;
uint32_t slimbus_log_count = 300;
uint32_t slimbus_rdwrerr_times;

static struct slimbus_track_config track_config_table[DA_COMBINE_V3_BUS_TRACK_TYPE_MAX] = {
	/* play back */
	{
		.params.channels = SLIMBUS_AUDIO_PLAYBACK_CHANNELS,
		.params.rate = SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &audio_playback[0],
	},
	/* capture */
	{
		.params.channels = SLIMBUS_AUDIO_CAPTURE_MULTI_MIC_CHANNELS,
		.params.rate = SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &audio_capture[0],
	},
	/* voice down */
	{
		.params.channels = SLIMBUS_VOICE_DOWN_CHANNELS,
		.params.rate = SAMPLE_RATE_8K,
		.params.callback = NULL,
		.channel_pro = &voice_down[0],
	},
	/* voice up */
	{
		.params.channels = SLIMBUS_VOICE_UP_CHANNELS,
		.params.rate = SAMPLE_RATE_8K,
		.params.callback = NULL,
		.channel_pro = &voice_up[0],
	},
	/* img download */
	{
		.params.channels = SLIMBUS_IMAGE_DOWNLOAD_CHANNELS,
		.params.rate = SAMPLE_RATE_768K,
		.params.callback = NULL,
		.channel_pro = &img_download[0],
	},
	/* ec */
	{
		.params.channels = SLIMBUS_ECREF_CHANNELS,
		.params.rate = SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &ec_ref[0],
	},
	/* sound trigger */
	{
		.params.channels = SLIMBUS_SOUND_TRIGGER_CHANNELS,
		.params.rate = SAMPLE_RATE_192K,
		.params.callback = NULL,
		.channel_pro = &sound_trigger[0],
	},
	/* debug */
	{
		.params.channels = SLIMBUS_DEBUG_CHANNELS,
		.params.rate = SAMPLE_RATE_192K,
		.params.callback = NULL,
		.channel_pro = &audio_debug[0],
	},
	/* direct play */
	{
		.params.channels = SLIMBUS_AUDIO_PLAYBACK_CHANNELS,
		.params.rate = SAMPLE_RATE_192K,
		.params.callback = NULL,
		.channel_pro = &audio_direct_playback[0],
	},
	/* fast play */
	{
		.params.channels = SLIMBUS_AUDIO_PLAYBACK_CHANNELS,
		.params.rate = SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &audio_fast_playback[0],
	},
	/* kws */
	{
		.params.channels = SLIMBUS_KWS_CHANNELS,
		.params.rate = SAMPLE_RATE_16K,
		.params.callback = NULL,
		.channel_pro = &audio_kws[0],
	},

};

/*
 * slimbus bus configuration
 */
static struct slimbus_bus_config bus_config[SLIMBUS_BUS_CONFIG_MAX] = {
	/* normal run */
	{
		.sm = SLIMBUS_SM_1_CSW_32_SL,    /* control space:4; subframe length:32; */
		.cg = SLIMBUS_CG_10,             /* clock gear */
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ */
	},
	/* img download */
	{
		.sm = SLIMBUS_SM_3_CSW_8_SL,     /* control space:3; subframe length:8; */
		.cg = SLIMBUS_CG_10,             /* clock gear */
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ */
	},
	/* switch framer */
	{
		.sm = SLIMBUS_SM_1_CSW_32_SL,    /* control space:4; subframe length:32; */
		.cg = SLIMBUS_CG_10,             /* clock gear */
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ */
	},
	/* reg write img download */
	{
		.sm = SLIMBUS_SM_8_CSW_8_SL,     /* control space:8; subframe length:8; */
		.cg = SLIMBUS_CG_10,             /* clock gear */
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ */
	},
};


uint32_t slimbus_logcount_get(void)
{
	return slimbus_log_count;
}

void slimbus_logcount_set(uint32_t count)
{
	slimbus_log_count = count;
}

uint32_t slimbus_logtimes_get(void)
{
	return slimbus_rdwrerr_times;
}

void slimbus_logtimes_set(uint32_t times)
{
	slimbus_rdwrerr_times = times;
}

static int32_t slimbus_element_read(struct slimbus_device_info *dev, uint32_t byte_address,
	enum slimbus_slice_size slice_size, void *value)
{
#ifdef CONFIG_HUAWEI_DSM
	static bool slimbus_dmd_flag = true;
#endif
	uint32_t reg_page = byte_address & (~0xff);
	uint8_t *paddr = (uint8_t *)&byte_address;
	uint8_t ret = 0;

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		slimbus_limit_err("slice size is invalid, slice_size:%d\n", slice_size);
		return -EINVAL;
	}

	mutex_lock(&dev->rw_mutex);
	if (dev->page_sel_addr != reg_page) {
		ret  = slimbus_drv_element_write(dev->generic_la, PAGE_SELECT_REG_0, SLIMBUS_SS_1_BYTE, (paddr + 1));
		ret += slimbus_drv_element_write(dev->generic_la, PAGE_SELECT_REG_1, SLIMBUS_SS_1_BYTE, (paddr + 2));
		ret += slimbus_drv_element_write(dev->generic_la, PAGE_SELECT_REG_2, SLIMBUS_SS_1_BYTE, (paddr + 3));

		dev->page_sel_addr = reg_page;
	}
	ret += slimbus_drv_element_read(dev->generic_la, SLIMBUS_USER_DATA_BASE_ADDR + *paddr, slice_size, (uint8_t *)value);
	mutex_unlock(&dev->rw_mutex);

	if (ret) {
		slimbus_limit_err("read error! slice_size=%d, addr=0x%pK!\n", slice_size, (void *)(uintptr_t)byte_address);
#ifdef CONFIG_HUAWEI_DSM
		if (slimbus_dmd_flag == true) {
			if (audio_dsm_report_info(AUDIO_CODEC, DSM_HI6402_SLIMBUS_READ_ERR,
				"slice_size=%d, addr=0x%pK\n", slice_size, (void *)(uintptr_t)byte_address) >= 0)
				slimbus_dmd_flag = false;
		}
#endif
		return -EFAULT;
	}
	slimbus_recover_info("read recover, slice_size=%d, addr=%pK!\n",
		slice_size, (void *)(uintptr_t)byte_address); /*lint !e571*/

	return 0;
}

static int32_t slimbus_element_write(struct slimbus_device_info *dev, uint32_t byte_address,
	enum slimbus_slice_size slice_size, void *value)
{
	uint32_t reg_page = byte_address & (~0xff);
	uint8_t *paddr = (uint8_t *)&byte_address;
	uint8_t ret = 0;

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		slimbus_limit_err("slice size is invalid, slice_size:%d\n", slice_size);
		return -EINVAL;
	}

	mutex_lock(&dev->rw_mutex);
	if (dev->page_sel_addr != reg_page) {
		ret = slimbus_drv_element_write(dev->generic_la, PAGE_SELECT_REG_0, SLIMBUS_SS_1_BYTE, (paddr + 1));
		ret += slimbus_drv_element_write(dev->generic_la, PAGE_SELECT_REG_1, SLIMBUS_SS_1_BYTE, (paddr + 2));
		ret += slimbus_drv_element_write(dev->generic_la, PAGE_SELECT_REG_2, SLIMBUS_SS_1_BYTE, (paddr + 3));

		dev->page_sel_addr = reg_page;
	}
	ret += slimbus_drv_element_write(dev->generic_la,
		(SLIMBUS_USER_DATA_BASE_ADDR + *paddr), slice_size, (uint8_t *)value);
	mutex_unlock(&dev->rw_mutex);

	if (ret) {
		slimbus_limit_err("write error! slice_size=%d, addr=0x%pK!\n",
			slice_size, (void *)(uintptr_t)byte_address); /*lint !e571*/
		return -EFAULT;
	}
	slimbus_recover_info("write recover, slice_size=%d, addr=%pK!\n",
		slice_size, (void *)(uintptr_t)byte_address); /*lint !e571*/

	return 0;
}

uint32_t slimbus_read_1byte(uint32_t reg)
{
	static uint32_t value;
	int32_t retry_count = 0;

	/* check requested values to ensure if there exists abnormal in slimbus */
	static uint32_t info0 = 0xa1;
	static uint32_t info1 = 0xa2;
	static uint32_t info2 = 0xa3;
	static uint32_t info3 = 0xa4;

	if (slimbus_devices == NULL) {
		slimbus_limit_err("slimbus device not allocate!\n");
		return INVALID_VAL;
	}

	value = READ_1BYTE_TEST_VALUE;
	do {
		slimbus_element_read(slimbus_devices, reg, SLIMBUS_SS_1_BYTE, &value);

		if (value == READ_1BYTE_TEST_VALUE) {
			slimbus_limit_info("slimbus read1byte retry: reg:0x%pK, val:%#x !\n", (void *)(uintptr_t)reg, value);
			retry_count++;
			mdelay(1);
		}
	} while ((value == READ_1BYTE_TEST_VALUE) && (retry_count <= RETRY_TIMES));

	if (retry_count > 0) {
		info0 = 0xa1;
		info1 = 0xa2;
		info2 = 0xa3;
		info3 = 0xa4;
		slimbus_drv_request_info(0x21, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &info0);
		slimbus_drv_request_info(0x40, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &info1);
		slimbus_drv_request_info(0x20, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &info2);
		slimbus_drv_request_info(0x41, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &info3);
		slimbus_limit_info("slimbus info: %#x, %#x, %#x, %#x\n", info0, info1, info2, info3);
	}
	return value;
}

uint32_t slimbus_read_4byte(uint32_t reg)
{
	static uint32_t value;
	static int32_t valbyte;
	int32_t retry_count = 0;

	if (slimbus_devices == NULL) {
		slimbus_limit_err("slimbus device not allocate!\n");
		return INVALID_VAL;
	}

	value = READ_4BYTE_YESY_VALUE;
	valbyte = 0;
	do {
		slimbus_element_read(slimbus_devices, reg, SLIMBUS_SS_4_BYTES, &value);
		slimbus_element_read(slimbus_devices, 0x20007022, SLIMBUS_SS_1_BYTE, &valbyte);
		slimbus_element_read(slimbus_devices, 0x20007023, SLIMBUS_SS_4_BYTES, &value);

		if (value == READ_4BYTE_YESY_VALUE) {
			slimbus_limit_info("slimbus read4byte retry: reg:0x%pK, val:%#x !\n", (void *)(uintptr_t)reg, value);
			retry_count++;
			mdelay(1);
		}
	} while ((value == READ_4BYTE_YESY_VALUE) && (retry_count <= RETRY_TIMES));

	return value;
}

void slimbus_write_1byte(uint32_t reg, uint32_t val)
{
	if (slimbus_devices == NULL) {
		slimbus_limit_err("slimbus device not allocate!\n");
		return;
	}

	slimbus_element_write(slimbus_devices, reg, SLIMBUS_SS_1_BYTE, &val);
}

void slimbus_write_4byte(uint32_t reg, uint32_t val)
{
	if (slimbus_devices == NULL) {
		slimbus_limit_err("slimbus device not allocate!\n");
		return;
	}

	slimbus_element_write(slimbus_devices, reg, SLIMBUS_SS_4_BYTES, &val);
}

void slimbus_read_pageaddr(void)
{
	static int32_t page0;
	static int32_t page1;
	static int32_t page2;

	if (slimbus_devices == NULL) {
		slimbus_limit_err("slimbus device not allocate!\n");
		return;
	}

	page0 = 0xA5;
	page1 = 0xA5;
	page2 = 0xA5;
	mutex_lock(&slimbus_devices->rw_mutex);
	slimbus_drv_element_read(slimbus_devices->generic_la, PAGE_SELECT_REG_0, SLIMBUS_SS_1_BYTE, (uint8_t *)&page0);
	slimbus_drv_element_read(slimbus_devices->generic_la, PAGE_SELECT_REG_1, SLIMBUS_SS_1_BYTE, (uint8_t *)&page1);
	slimbus_drv_element_read(slimbus_devices->generic_la, PAGE_SELECT_REG_2, SLIMBUS_SS_1_BYTE, (uint8_t *)&page2);
	mutex_unlock(&slimbus_devices->rw_mutex);

	AUDIO_LOGI("cdc page addr:0x%pK, page0:%#x, page1:%#x, page2:%#x",
		(void *)(uintptr_t)(slimbus_devices->page_sel_addr), page0, page1, page2);
}

int slimbus_runtime_get(void)
{
	int pm_ret;

	if (pdata->dev != NULL) {
		if (is_pm_runtime_support()) {
			pm_ret = pm_runtime_get_sync(pdata->dev);
			if (pm_ret < 0) {
				AUDIO_LOGE("pm resume error, pm_ret: %d", pm_ret);
#ifdef CONFIG_DFX_BB
				rdr_system_error(RDR_AUDIO_RUNTIME_SYNC_FAIL_MODID, 0, 0);
#endif
				return pm_ret;
			}
			return 0;
		}
		return 0;
	}
	return -EFAULT;
}

void slimbus_runtime_put(void)
{
	if (pdata->dev != NULL) {
		if (is_pm_runtime_support()) {
			pm_runtime_mark_last_busy(pdata->dev);
			pm_runtime_put_autosuspend(pdata->dev);
		}
	}
}

int32_t slimbus_enum_device(void)
{
	int32_t ret = 0;

	ret = slimbus_wait_codec_device_enum();
	if (ret < 0) {
		AUDIO_LOGE("slimbus codec enumerate failed: 0x%x", ret);
		return ret;
	}

	ret = slimbus_activate_track(DA_COMBINE_V5_BUS_TRACK_AUDIO_PLAY, NULL);
	if (ret != 0)
		AUDIO_LOGE("activate audio play failed");
	usleep_range(1000, 1100);
	ret = slimbus_deactivate_track(DA_COMBINE_V5_BUS_TRACK_AUDIO_PLAY, NULL);
	if (ret != 0)
		AUDIO_LOGE("deactivate audio play failed");

	return ret;
}

uint32_t slimbus_trackstate_get(void)
{
	uint32_t trackstate = 0;
	uint32_t track;

	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return SLIMBUS_TRACK_ERROR;
	}

	if (!pdata->track_state) {
		AUDIO_LOGE("cannot get track state");
		return SLIMBUS_TRACK_ERROR;
	}

	for (track = 0; track < pdata->slimbus_track_max; track++) {
		if (pdata->track_state[track])
			trackstate |= (1 << track);
	}

	return trackstate;
}

static bool is_image_scene(uint32_t active_tracks)
{
	if (active_tracks & DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD_ON)
		return true;

	return false;
}

static bool is_play_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_EC_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_call_only_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_call_12288_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_anc_call_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON |
		DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON |
		DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
		if (track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels == 2)
			return true;
		break;
	default:
		break;
	}

	return false;
}

static bool is_enhance_soundtrigger_6144_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_KWS_ON:
	case DA_COMBINE_V3_BUS_TRACK_KWS_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_KWS_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_KWS_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON:
		if (track_config_table[DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER].params.rate != SAMPLE_RATE_16K) {
			AUDIO_LOGI("not st scene");
			return false;
		}
		AUDIO_LOGI("enhance st scene");
		return true;
	default:
		break;
	}

	return false;
}

static bool is_direct_play_scene(uint32_t active_tracks)
{
	uint32_t tmp;

	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON |
		DA_COMBINE_V3_BUS_TRACK_EC_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
		return true;
	default:
		tmp = (DA_COMBINE_V3_BUS_TRACK_DEBUG_ON |
			DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON |
			DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON);
		if ((active_tracks & tmp) == tmp)
			return false;

		tmp = (DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON);
		if ((active_tracks & tmp) == tmp)
			return false;

		tmp = (DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON);
		if ((active_tracks & tmp) == tmp)
			return true;

		tmp = (DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_KWS_ON);
		if ((active_tracks & tmp) == tmp)
			return true;

		break;
	}

	return false;
}

static bool is_has_fast(uint32_t active_tracks)
{
	return ((active_tracks & DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON) == DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON);
}

static bool is_fast_play_scene(uint32_t active_tracks)
{
	if (!is_has_fast(active_tracks))
		return false;

	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_fast_play_record_scene(uint32_t active_tracks)
{
	uint32_t tmp;

	if (!is_has_fast(active_tracks))
		return false;

	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_CAPTURE_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON:
		return true;
	default:
		tmp = (DA_COMBINE_V3_BUS_TRACK_DEBUG_ON |
			DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON |
			DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON);
		if ((active_tracks & tmp) == tmp)
			return false;

		tmp = (DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON);
		if ((active_tracks & tmp) == tmp)
			return false;

		tmp = (DA_COMBINE_V3_BUS_TRACK_DEBUG_ON | DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON);
		if ((active_tracks & tmp) == tmp)
			return true;

		break;
	}

	return false;
}

static bool is_fast_play_soundtrigger_scene(uint32_t active_tracks)
{
	uint32_t tmp = 0;

	if (!is_has_fast(active_tracks))
		return false;

	switch (active_tracks) {
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_EC_ON |
		DA_COMBINE_V3_BUS_TRACK_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY_ON |
		DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON:
		return true;
	default:
		tmp = (DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN_ON | DA_COMBINE_V3_BUS_TRACK_VOICE_UP_ON);
		if ((active_tracks & tmp) == tmp)
			return false;

		tmp = (DA_COMBINE_V3_BUS_TRACK_FAST_PLAY_ON | DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER_ON);
		if ((active_tracks & tmp) == tmp)
			return true;

		break;
	}

	return false;
}

void slimbus_trackstate_set(uint32_t track, bool state)
{
	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return;
	}

	if (!pdata->track_state || (track >= pdata->slimbus_track_max)) {
		AUDIO_LOGE("track state cannot set");
		return;
	}

	pdata->track_state[track] = state;
}

bool slimbus_track_state_is_on(uint32_t track)
{
	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return false;
	}

	if (!pdata->track_state || (track >= pdata->slimbus_track_max)) {
		AUDIO_LOGE("cannot get track state");
		return false;
	}

	return pdata->track_state[track];
}

static int32_t process_direct_and_play_conflict(unsigned int track)
{
	int32_t ret = 0;

	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY) && (track == DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY)) {
		AUDIO_LOGI("direct conflict");
		return -EPERM;
	}
	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY) && (track == DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY)) {
		ret = slimbus_drv_track_deactivate(track_config_table[DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY].channel_pro,
				track_config_table[DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY].params.channels);

		slimbus_trackstate_set(DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY, false);
	}

	return ret;
}

static int32_t process_play_and_debug_conflict(unsigned int track, int32_t *need_callback)
{
	int32_t ret = 0;

	if ((slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY) ||
		slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_FAST_PLAY)) && (track == DA_COMBINE_V3_BUS_TRACK_DEBUG)) {
		AUDIO_LOGI("debug conflict");
		return -EPERM;
	}
	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_DEBUG)) {
		*need_callback = 1;
		ret = slimbus_drv_track_deactivate(track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].channel_pro,
						track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels);

		slimbus_trackstate_set(DA_COMBINE_V3_BUS_TRACK_DEBUG, false);
		track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels = 1;
	}

	return ret;
}

static int32_t process_image_and_other_conflict(unsigned int track)
{
	int32_t ret = 0;
	int32_t i;

	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD) && (track != DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD)) {
		AUDIO_LOGI("image conflict");
		return -EPERM;
	}
	if (track == DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD) {
		for (i = 0; i < DA_COMBINE_V3_BUS_TRACK_TYPE_MAX; i++) {
			if (slimbus_track_state_is_on(i) && (i != track)) {
				AUDIO_LOGI("image load, deactivate track:%#x", i);
				ret += slimbus_drv_track_deactivate(track_config_table[i].channel_pro,
						track_config_table[i].params.channels);
				slimbus_trackstate_set(i, false);
			}
		}
	}

	return ret;
}

static int32_t process_soundtrigger_and_debug_conflict(unsigned int track, int32_t *need_callback)
{
	int32_t ret = 0;

	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER) &&
		(track == DA_COMBINE_V3_BUS_TRACK_DEBUG)) {
		AUDIO_LOGI("st and debug conflict");
		return -EPERM;
	}
	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_DEBUG) &&
		(track == DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER)) {
		*need_callback = 1;
		ret = slimbus_drv_track_deactivate(track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].channel_pro,
				track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels);

		slimbus_trackstate_set(DA_COMBINE_V3_BUS_TRACK_DEBUG, false);
	}

	return ret;
}

static int32_t process_soundtrigger_params_conflict(unsigned int track)
{
	bool is_fast_track = ((track_config_table[DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER].params.channels == 2) &&
		(track_config_table[DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER].params.rate == SAMPLE_RATE_192K));

	if ((track == DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER) && is_fast_track) {
		track_config_table[track].params.channels = 1;
		AUDIO_LOGI("soundtrigger conflict");
		return -EPERM;
	}

	return 0;
}

static int32_t process_normal_scene_conflict(unsigned int track,
	bool track_enable, int32_t *need_callback)
{
	int32_t ret = 0;

	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_VOICE_UP) &&
		(track == DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER) && track_enable)
		AUDIO_LOGI("st conflict");

	if (((track == DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY) ||
		(track == DA_COMBINE_V3_BUS_TRACK_FAST_PLAY)) && track_enable) {
		AUDIO_LOGI("conflict");
		return -EPERM;
	}
	if ((track == DA_COMBINE_V3_BUS_TRACK_DEBUG) &&
		(track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels == 2)) {
		track_config_table[track].params.channels = 1;
		AUDIO_LOGI("debug conflict");
		return -EPERM;
	}
	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY)) {
		ret = slimbus_drv_track_deactivate(track_config_table[DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY].channel_pro,
				track_config_table[DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY].params.channels);

		slimbus_trackstate_set(DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY, false);
	}
	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_FAST_PLAY)) {
		ret = slimbus_drv_track_deactivate(track_config_table[DA_COMBINE_V3_BUS_TRACK_FAST_PLAY].channel_pro,
				track_config_table[DA_COMBINE_V3_BUS_TRACK_FAST_PLAY].params.channels);

		slimbus_trackstate_set(DA_COMBINE_V3_BUS_TRACK_FAST_PLAY, false);
	}
	if (slimbus_track_state_is_on(DA_COMBINE_V3_BUS_TRACK_DEBUG) &&
		(track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels == 2)) {
		*need_callback = 1;
		ret = slimbus_drv_track_deactivate(track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].channel_pro,
				track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels);

		slimbus_trackstate_set(DA_COMBINE_V3_BUS_TRACK_DEBUG, false);
		track_config_table[DA_COMBINE_V3_BUS_TRACK_DEBUG].params.channels = 1;
	}

	if (process_soundtrigger_and_debug_conflict(track, need_callback)) {
		AUDIO_LOGI("st and debug conflict");
		return -EPERM;
	}

	ret = process_soundtrigger_params_conflict(track);

	return ret;
}

static int32_t process_other_scenes_conflict(unsigned int track,
	int32_t *need_callback)
{
	int32_t ret = 0;

	if (process_direct_and_play_conflict(track)) {
		AUDIO_LOGI("direct conflict");
		return -EPERM;
	}
	if (process_play_and_debug_conflict(track, need_callback)) {
		AUDIO_LOGI("debug conflict");
		return -EPERM;
	}
	if (process_image_and_other_conflict(track)) {
		AUDIO_LOGI("image conflict");
		return -EPERM;
	}

	ret = process_soundtrigger_params_conflict(track);

	return ret;
}

static int32_t slimbus_check_scenes(uint32_t track, uint32_t scenes, bool track_enable)
{
	uint32_t i;
	int32_t ret = 0;
	int32_t need_callback = 0;

	if (track >= DA_COMBINE_V3_BUS_TRACK_TYPE_MAX || scenes >= SLIMBUS_SCENE_CONFIG_MAX) {
		AUDIO_LOGE("param is invalid, track: %u, scenes: %u", track, scenes);
		return -EINVAL;
	}

	switch (scenes) {
	case SLIMBUS_SCENE_CONFIG_IMAGE_LOAD:
	case SLIMBUS_SCENE_CONFIG_DIRECT_PLAY:
	case SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_REC:
	case SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_ST:
		ret = process_other_scenes_conflict(track, &need_callback);
		break;
	case SLIMBUS_SCENE_CONFIG_NORMAL:
		ret = process_normal_scene_conflict(track, track_enable, &need_callback);
		break;
	default:
		break;
	}

	for (i = 0; i < DA_COMBINE_V3_BUS_TRACK_TYPE_MAX; i++) {
		if (slimbus_track_state_is_on(i) &&
			(i == DA_COMBINE_V3_BUS_TRACK_DEBUG) &&
			track_config_table[i].params.callback &&
			need_callback)
			track_config_table[i].params.callback((void *)&(track_config_table[i].params));
	}

	return ret;
}

static struct select_scene_node scene_table[] = {
	{ SLIMBUS_SCENE_CONFIG_IMAGE_LOAD, SLIMBUS_CG_10, SLIMBUS_SM_3_CSW_8_SL, is_image_scene },
	{ SLIMBUS_SCENE_CONFIG_PLAY, SLIMBUS_CG_8, SLIMBUS_SM_8_CSW_32_SL, is_play_scene },
	{ SLIMBUS_SCENE_CONFIG_CALL, SLIMBUS_CG_8, SLIMBUS_SM_6_CSW_24_SL, is_call_only_scene },
	{ SLIMBUS_SCENE_CONFIG_CALL_12288, SLIMBUS_CG_9, SLIMBUS_SM_8_CSW_32_SL, is_call_12288_scene },
	{ SLIMBUS_SCENE_CONFIG_ANC_CALL, SLIMBUS_CG_10, SLIMBUS_SM_1_CSW_32_SL, is_anc_call_scene },
	{ SLIMBUS_SCENE_CONFIG_ENHANCE_ST_6144, SLIMBUS_CG_8, SLIMBUS_SM_4_CSW_32_SL, is_enhance_soundtrigger_6144_scene },
	{ SLIMBUS_SCENE_CONFIG_DIRECT_PLAY, SLIMBUS_CG_10, SLIMBUS_SM_2_CSW_32_SL, is_direct_play_scene },
	{ SLIMBUS_SCENE_CONFIG_FAST_PLAY, SLIMBUS_CG_8, SLIMBUS_SM_8_CSW_32_SL, is_fast_play_scene },
	{ SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_REC, SLIMBUS_CG_10, SLIMBUS_SM_2_CSW_32_SL, is_fast_play_record_scene },
	{ SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_ST, SLIMBUS_CG_10, SLIMBUS_SM_4_CSW_32_SL, is_fast_play_soundtrigger_scene },
};

static int32_t slimbus_select_scenes(struct slimbus_device_info *dev,
	uint32_t track, const struct scene_param *params, bool track_enable)
{
	enum slimbus_scene_config_type scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
	enum slimbus_subframe_mode sm = SLIMBUS_SM_1_CSW_32_SL;
	enum slimbus_clock_gear cg = SLIMBUS_CG_10;
	uint32_t len = ARRAY_SIZE(scene_table);
	uint32_t active_tracks;
	int32_t ret;
	uint32_t i;

	if (track >= DA_COMBINE_V3_BUS_TRACK_TYPE_MAX) {
		AUDIO_LOGE("track is invalid, track: %u", track);
		return -EINVAL;
	}

	active_tracks = slimbus_trackstate_get();
	if (active_tracks == SLIMBUS_TRACK_ERROR)
		return -EINVAL;

	if (track_enable)
		active_tracks |= (1 << track);

	for (i = 0; i < len; i++) {
		if (scene_table[i].is_scene_matched(active_tracks)) {
			scene_config_type = scene_table[i].scene_type;
			cg = scene_table[i].cg;
			sm = scene_table[i].subframe_mode;
			break;
		}
	}

	ret = slimbus_check_scenes(track, scene_config_type, track_enable);
	if (ret) {
		AUDIO_LOGI("ret %d", ret);
		return ret;
	}

	if (dev->scene_config_type != scene_config_type) {
		AUDIO_LOGI("scene changed from %u to %d", dev->scene_config_type, scene_config_type);
		dev->cg = cg;
		dev->sm = sm;
		dev->scene_config_type = scene_config_type;
	}

	pdata->dev_ops->slimbus_device_param_update(dev, track, params);

	return 0;
}

static int32_t slimbus_da_combine_track_soundtrigger_activate(uint32_t track,
	bool slimbus_dynamic_freq_enable, struct slimbus_device_info *dev,
	struct scene_param *params)
{
	int32_t ret;
	struct slimbus_sound_trigger_params st_params;
	struct scene_param st_normal_params;
	struct slimbus_track_config *track_config_t = NULL;

	if (track != DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER) {
		AUDIO_LOGE("track %u activate is not soundtriger", track);
		return -1;
	}

	memset(&st_params, 0, sizeof(st_params));
	memset(&st_normal_params, 0, sizeof(st_normal_params));

	pdata->dev_ops->slimbus_get_soundtrigger_params(&st_params);
	st_normal_params.channels = st_params.channels;
	st_normal_params.rate = st_params.sample_rate;

	ret = pdata->dev_ops->slimbus_device_param_set(dev, st_params.track_type,
				&st_normal_params);
	ret += pdata->dev_ops->slimbus_device_param_set(dev, track,
				params);
	if (ret) {
		AUDIO_LOGE("slimbus device param set failed, ret: %d", ret);
		return ret;
	}

	/*  request soc slimbus clk to 24.576M */
	slimbus_utils_freq_request();
	track_config_t = dev->slimbus_da_combine_para->track_config_table;

	if (slimbus_dynamic_freq_enable) {
		ret = slimbus_drv_track_update(dev->cg, dev->sm, track, dev,
			track_config_t[track].params.channels, track_config_t[track].channel_pro);
		ret += slimbus_drv_track_update(dev->cg, dev->sm, st_params.track_type, dev,
			SLIMBUS_VOICE_UP_SOUNDTRIGGER, track_config_t[st_params.track_type].channel_pro);
	} else {
		ret = slimbus_drv_track_activate(track_config_t[track].channel_pro, track_config_t[track].params.channels);
		ret += slimbus_drv_track_activate(track_config_t[st_params.track_type].channel_pro, SLIMBUS_VOICE_UP_SOUNDTRIGGER);
	}

	return ret;
}

static int32_t slimbus_check_pm(uint32_t track)
{
	int32_t ret = 0;

	if (is_pm_runtime_support()) {
		if (pdata->track_state[track]) {
			AUDIO_LOGI("track: %u has been configured", track);
		} else {
			ret = slimbus_runtime_get();
			if (ret < 0)
				return ret;

			if (!slimbus_trackstate_get()) {
				ret = slimbus_switch_framer(FRAMER_CODEC);
				if (ret)
					AUDIO_LOGE("slimbus switch framer failed");
			}
		}
	}

	return ret;
}

static bool slimbus_da_combine_track_is_fast_soundtrigger(uint32_t track)
{
	if (track != DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER)
		return false;

	return ((track == DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER) &&
		(track_config_table[track].params.rate == SAMPLE_RATE_192K));
}

static bool slimbus_scene_params_is_valid(uint32_t track)
{
	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return false;
	}

	if (!pdata->track_state || (track >= pdata->slimbus_track_max)) {
		AUDIO_LOGE("params error, track %u", track);
		return false;
	}

	if (slimbus_devices == NULL) {
		AUDIO_LOGE("slimbus havn't been init");
		return false;
	}

	if (slimbus_devices->slimbus_da_combine_para == NULL) {
		AUDIO_LOGE("slimbus para havn't been init");
		return false;
	}

	if (slimbus_devices->slimbus_da_combine_para->track_config_table == NULL) {
		AUDIO_LOGE("slimbus track config havn't been init");
		return false;
	}

	return true;
}

static int32_t activate_track(struct slimbus_device_info *dev, uint32_t track,
		struct scene_param *params, const struct slimbus_track_config *config)
{
	int32_t ret;
	bool is_fast_soundtrigger = pdata->dev_ops->slimbus_track_is_fast_soundtrigger(track);

	if (is_fast_soundtrigger) {
		ret = pdata->dev_ops->slimbus_track_soundtrigger_activate(track,
				pdata->slimbus_dynamic_freq_enable, dev, params);
	} else {
		ret = pdata->dev_ops->slimbus_device_param_set(dev, track, params);
		if (pdata->slimbus_dynamic_freq_enable) {
			ret += slimbus_drv_track_update(dev->cg, dev->sm, track, dev,
				config[track].params.channels, config[track].channel_pro);
		} else {
			ret += slimbus_drv_track_activate(config[track].channel_pro,
				config[track].params.channels);
		}
	}

	if (!ret)
		pdata->track_state[track] = true;

	return ret;
}

static struct scene_match_track slimbus_v3_match[DA_COMBINE_V3_BUS_TRACK_TYPE_MAX] = {
	{"AUDIO_PLAY", DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY},
	{"AUDIO_CAPTURE", DA_COMBINE_V3_BUS_TRACK_AUDIO_CAPTURE},
	{"VOICE_DOWN", DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN},
	{"VOICE_UP", DA_COMBINE_V3_BUS_TRACK_VOICE_UP},
	{"IMAGE_LOAD", DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD},
	{"ECREF", DA_COMBINE_V3_BUS_TRACK_ECREF},
	{"SOUND_TRIGGER", DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER},
	{"DEBUG", DA_COMBINE_V3_BUS_TRACK_DEBUG},
	{"DIRECT_PLAY", DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY},
	{"FAST_PLAY", DA_COMBINE_V3_BUS_TRACK_FAST_PLAY},
	{"KWS", DA_COMBINE_V3_BUS_TRACK_KWS},
};

static struct scene_match_track slimbus_v5_match[DA_COMBINE_V5_BUS_TRACK_TYPE_MAX] = {
	{"AUDIO_PLAY", DA_COMBINE_V5_BUS_TRACK_AUDIO_PLAY},
	{"AUDIO_CAPTURE", DA_COMBINE_V5_BUS_TRACK_AUDIO_CAPTURE},
	{"VOICE_DOWN", DA_COMBINE_V5_BUS_TRACK_VOICE_DOWN},
	{"VOICE_UP", DA_COMBINE_V5_BUS_TRACK_VOICE_UP},
	{"ECREF", DA_COMBINE_V5_BUS_TRACK_ECREF},
	{"SOUND_TRIGGER", DA_COMBINE_V5_BUS_TRACK_SOUND_TRIGGER},
	{"DEBUG", DA_COMBINE_V5_BUS_TRACK_DEBUG},
	{"DIRECT_PLAY", DA_COMBINE_V5_BUS_TRACK_DIRECT_PLAY},
	{"2PA_IV", DA_COMBINE_V5_BUS_TRACK_2PA_IV},
	{"4PA_IV", DA_COMBINE_V5_BUS_TRACK_4PA_IV},
	{"AUDIO_PLAY_D3D4", DA_COMBINE_V5_BUS_TRACK_AUDIO_PLAY_D3D4},
	{"KWS", DA_COMBINE_V5_BUS_TRACK_KWS},
	{"5MIC_CAPTURE", DA_COMBINE_V5_BUS_TRACK_5MIC_CAPTURE},
	{"VOICE_UP_4PA", DA_COMBINE_V5_BUS_TRACK_VOICE_UP_4PA},
	{"BT_DOWN", DA_COMBINE_V5_BUS_TRACK_BT_DOWN},
	{"BT_UP", DA_COMBINE_V5_BUS_TRACK_BT_UP},
	{"ULTRA_UP", DA_COMBINE_V5_BUS_TRACK_ULTRA_UP},
};

static int32_t get_track_type(const char* scene_name)
{
	uint32_t i;
	struct scene_match_track *match_track = NULL;
	uint32_t track_max;

	if (pdata->device_type == SLIMBUS_DEVICE_DA_COMBINE_V3) {
		match_track = slimbus_v3_match;
		track_max = DA_COMBINE_V3_BUS_TRACK_TYPE_MAX;
	} else {
		match_track = slimbus_v5_match;
		track_max = DA_COMBINE_V5_BUS_TRACK_TYPE_MAX;
	}

	for (i = 0; i < track_max; i++) {
		if (strcmp(match_track[i].codec_scene, scene_name) == 0)
			return match_track[i].track;
	}

	AUDIO_LOGW("get track type error, scene_name = %s", scene_name);
	return -EINVAL;
}

static int32_t slimbus_activate(const char* codec_scene_name, struct scene_param *params)
{
	int32_t track = get_track_type(codec_scene_name);

	return track >= 0 ? slimbus_activate_track(track, params) : 0;
}

int32_t slimbus_activate_track(uint32_t track, struct scene_param *params)
{
	int32_t ret;
	struct slimbus_track_config *cfg = NULL;

	if (!slimbus_scene_params_is_valid(track)) {
		AUDIO_LOGE("params error, track %u", track);
		return -EINVAL;
	}

	cfg = slimbus_devices->slimbus_da_combine_para->track_config_table;

	mutex_lock(&slimbus_devices->track_mutex);

	ret = slimbus_check_pm(track);
	if (ret < 0) {
		mutex_unlock(&slimbus_devices->track_mutex);
		return ret;
	}

	if (params) {
		cfg[track].params.channels = params->channels;
		cfg[track].params.rate = params->rate;
		cfg[track].params.callback = params->callback;
	}

	if (pdata->slimbus_dynamic_freq_enable)
		ret = pdata->dev_ops->slimbus_select_scenes(slimbus_devices, track, params, true);
	else
		ret = pdata->dev_ops->slimbus_check_scenes(track, slimbus_devices->scene_config_type, true);

	if (ret) {
		AUDIO_LOGE("slimbus activate track %u fail", track);
		if (!pdata->track_state[track])
			slimbus_runtime_put();
		mutex_unlock(&slimbus_devices->track_mutex);
		return ret;
	}

	ret = activate_track(slimbus_devices, track, params, cfg);

	set_bus_active(!!(slimbus_trackstate_get()));

	mutex_unlock(&slimbus_devices->track_mutex);

	return ret;
}

static int32_t slimbus_da_combine_track_soundtrigger_deactivate(uint32_t track)
{
	int32_t ret;
	struct slimbus_sound_trigger_params st_params;

	if (track != DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER) {
		AUDIO_LOGE("track %u deactive is not soundtriger", track);
		return -1;
	}

	memset(&st_params, 0, sizeof(st_params));
	pdata->dev_ops->slimbus_get_soundtrigger_params(&st_params);

	/*  release soc slimbus clk to 21.777M */
	slimbus_utils_freq_release();

	ret = slimbus_drv_track_deactivate(track_config_table[track].channel_pro,
		track_config_table[track].params.channels);
	ret += slimbus_drv_track_deactivate(track_config_table[st_params.track_type].channel_pro,
		SLIMBUS_VOICE_UP_SOUNDTRIGGER);

	return ret;
}

static int32_t deactivate_track(struct slimbus_device_info *dev, uint32_t track,
		const struct slimbus_track_config *config)
{
	int32_t ret;
	bool is_fast_soundtrigger = pdata->dev_ops->slimbus_track_is_fast_soundtrigger(track);

	if (is_fast_soundtrigger) {
		ret = pdata->dev_ops->slimbus_track_soundtrigger_deactivate(track);
	} else {
		ret = slimbus_drv_track_deactivate(config[track].channel_pro,
				config[track].params.channels);
	}

	if (!ret)
		pdata->track_state[track] = false;
	else
		AUDIO_LOGE("fail track %u, ret %d", track, ret);

	return ret;
}

static int32_t slimbus_deactivate(const char* codec_scene_name, struct scene_param *params)
{
	int32_t track = get_track_type(codec_scene_name);

	return track >= 0 ? slimbus_deactivate_track(track, params) : -EINVAL;
}

int32_t slimbus_deactivate_track(uint32_t track, struct scene_param *params)
{
	int32_t ret;
	struct slimbus_track_config *cfg = NULL;

	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return -EINVAL;
	}

	if (!slimbus_scene_params_is_valid(track)) {
		AUDIO_LOGE("params error, track %u", track);
		return -EINVAL;
	}

	cfg = slimbus_devices->slimbus_da_combine_para->track_config_table;

	mutex_lock(&slimbus_devices->track_mutex);

	if (is_pm_runtime_support()) {
		if (!pdata->track_state[track]) {
			AUDIO_LOGE("track: %u has been removed", track);
			mutex_unlock(&slimbus_devices->track_mutex);
			return 0;
		}
	}

	if (params) {
		cfg[track].params.channels = params->channels;
		cfg[track].params.rate = params->rate;
		cfg[track].params.callback = params->callback;
	}

	ret = deactivate_track(slimbus_devices, track, cfg);

	if (pdata->slimbus_dynamic_freq_enable) {
		ret += pdata->dev_ops->slimbus_select_scenes(slimbus_devices, track, NULL, false);
		ret += slimbus_drv_track_update(slimbus_devices->cg, slimbus_devices->sm,
			track, slimbus_devices, 0, NULL);
	}

	AUDIO_LOGI("track: %u trackstate:0x%x portstate:0x%x pm_usage_count:0x%x",
		track, slimbus_trackstate_get(),
		slimbus_utils_port_state_get(pdata->slimbus_mem),
		atomic_read(&(pdata->dev->power.usage_count)));

	if (is_pm_runtime_support()) {
		if (!slimbus_trackstate_get()) {
			ret += slimbus_switch_framer(FRAMER_SOC);
			if (ret)
				AUDIO_LOGE("slimbus switch framer to soc failed");
		}
		slimbus_runtime_put();
	}

	set_bus_active(!!(slimbus_trackstate_get()));

	mutex_unlock(&slimbus_devices->track_mutex);

	return ret;
}

static bool slimbus_switch_framer_params_check(enum framer_type framer_type)
{
	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return false;
	}

	if (framer_type >= FRAMER_NUM) {
		AUDIO_LOGE("param is invalid, framer_type: %d", framer_type);
		return false;
	}

	return true;
}

int32_t slimbus_switch_framer(enum framer_type framer_type)
{
	int32_t ret = -1;
	uint8_t la;
	struct slimbus_bus_config *bus_cfg = NULL;

	if (!slimbus_switch_framer_params_check(framer_type)) {
		AUDIO_LOGE("params check fail");
		return -EINVAL;
	}

	if (slimbus_devices == NULL) {
		AUDIO_LOGE("slimbus havn't been init");
		return -EINVAL;
	}

	if (framer_type == FRAMER_CODEC) {
		/* modify soc slimbus clk to 24.576M */
		slimbus_utils_freq_request();
		bus_cfg = &bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER];
	} else if (framer_type == FRAMER_SOC) {
		bus_cfg = &bus_config[SLIMBUS_BUS_CONFIG_NORMAL];
		/* modify soc slimbus clk to low to avoid signal interference to GPS */
		slimbus_utils_freq_release();
	} else {
		AUDIO_LOGE("invalid framer_type: %#x", framer_type);
		return -EINVAL;
	}

	la = slimbus_drv_get_framerla(framer_type);
	if (la == 0) {
		AUDIO_LOGE("invalid la: %d framer_type: %#x", la, framer_type);
		return -EINVAL;
	}

	if (pdata->switch_framer_disable)
		return 0;

	if (slimbus_devices->rf == SLIMBUS_RF_6144) {
		if (framer_type == FRAMER_CODEC)
			ret = slimbus_drv_switch_framer(la, 4, 18, bus_cfg);
		else if (framer_type == FRAMER_SOC)
			ret = slimbus_drv_switch_framer(la, 17, 3, bus_cfg);
	} else if (slimbus_devices->rf == SLIMBUS_RF_24576) {
		ret = slimbus_drv_switch_framer(la, 4, 3, bus_cfg);
	}

	if (ret == EOK)
		pdata->framerstate =  framer_type;
	else
		AUDIO_LOGE("faild, ret = %d, framer_type = %d", ret, framer_type);

	return ret;
}

int32_t slimbus_pause_clock(enum slimbus_restart_time time)
{
	int32_t ret;

	if (time > SLIMBUS_RT_UNSPECIFIED_DELAY) {
		AUDIO_LOGE("new start time is invalid, time: %d", time);
		return -EINVAL;
	}

	if (slimbus_devices == NULL) {
		AUDIO_LOGE("slimbus havn't been init");
		return -1;
	}

	ret = slimbus_drv_pause_clock(time);

	return ret;
}

int32_t slimbus_track_recover(void)
{
	uint32_t track_type;
	int32_t ret = 0;

	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return -EINVAL;
	}

	if (!pdata->track_state) {
		AUDIO_LOGE("cannot get track state");
		return -1;
	}

	for (track_type = 0; track_type < pdata->slimbus_track_max; track_type++) {
		if (pdata->track_state[track_type]) {
			AUDIO_LOGI("recover track: %#x", track_type);
			ret += slimbus_activate_track(track_type, NULL);
		}
	}

	return ret;
}

enum framer_type slimbus_debug_get_framer(void)
{
	if (!pdata) {
		AUDIO_LOGE("cannot get framer");
		return FRAMER_SOC;
	}
	return pdata->framerstate;
}

enum slimbus_device slimbus_debug_get_device_type(void)
{
	if (!pdata) {
		AUDIO_LOGE("cannot get device type");
		return SLIMBUS_DEVICE_NUM;
	}
	return pdata->device_type;
}

int32_t slimbus_bus_configure(enum slimbus_bus_type type)
{
	int32_t ret;

	if (!pdata) {
		AUDIO_LOGE("pdata is null");
		return -EINVAL;
	}

	if (type >= SLIMBUS_BUS_CONFIG_MAX) {
		AUDIO_LOGE("type is invalid, type: %d", type);
		return -EINVAL;
	}

	ret = slimbus_runtime_get();
	if (ret < 0)
		return ret;

	ret = slimbus_drv_bus_configure(&bus_config[type]);

	slimbus_runtime_put();

	return ret;
}

struct slimbus_bus_config *slimbus_get_bus_config(enum slimbus_bus_type type)
{
	if (type >= SLIMBUS_BUS_CONFIG_MAX) {
		AUDIO_LOGE("bus type is invalid:%d", type);
		return NULL;
	}

	return &bus_config[type];
}

struct slimbus_track_config *slimbus_get_track_config_table(void)
{
	return track_config_table;
}

struct slimbus_device_info *slimbus_get_devices_info(void)
{
	return slimbus_devices;
}

static void slimbus_io_param_get(void)
{
	uint32_t slimbusclk_drv = 0;
	uint32_t slimbusdata_drv = 0;
	uint32_t slimbusclk_offset = 0;
	uint32_t slimbusdata_offset = 0;
	uint32_t slimbusclk_cfg_offset = 0;
	uint32_t slimbusdata_cfg_offset = 0;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusclk_io_driver", &slimbusclk_drv))
		slimbus_devices->slimbusclk_drv = slimbusclk_drv;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusdata_io_driver", &slimbusdata_drv))
		slimbus_devices->slimbusdata_drv = slimbusdata_drv;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusclk_offset", &slimbusclk_offset))
		slimbus_devices->slimbusclk_offset = slimbusclk_offset;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusdata_offset", &slimbusdata_offset))
		slimbus_devices->slimbusdata_offset = slimbusdata_offset;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusclk_cfg_offset", &slimbusclk_cfg_offset))
		slimbus_devices->slimbusclk_cfg_offset = slimbusclk_cfg_offset;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusdata_cfg_offset", &slimbusdata_cfg_offset))
		slimbus_devices->slimbusdata_cfg_offset = slimbusdata_cfg_offset;
}

static void slimbus_port_version_get(void)
{
	uint32_t voice_up_port_version;
	uint32_t audio_up_port_version;

	if (!of_property_read_u32(pdata->dev->of_node, "voice_up_port_version", &voice_up_port_version)) {
		slimbus_devices->voice_up_port_version = voice_up_port_version;
	} else {
		slimbus_devices->voice_up_port_version = 0;
	}

	if (!of_property_read_u32(pdata->dev->of_node, "audio_up_port_version", &audio_up_port_version)) {
		slimbus_devices->audio_up_port_version = audio_up_port_version;
	} else {
		slimbus_devices->audio_up_port_version = 0;
	}

	pr_info("[%s:%d] voice, audio up port version is %d, %d\n",
		__FUNCTION__, __LINE__,
		slimbus_devices->voice_up_port_version,
		slimbus_devices->audio_up_port_version);
}

static int32_t slimbus_select_platform_type(const char *platform_type,
	enum slimbus_device device_type, enum platform_type *type)
{
	if (platform_type == NULL) {
		AUDIO_LOGI("platform not define! default:ASIC");
		slimbus_devices->rf = SLIMBUS_RF_24576;
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_10;
		slimbus_devices->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
		return 0;
	}

	if (!strcmp(platform_type, "ASIC")) {
		slimbus_devices->rf = SLIMBUS_RF_24576;
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_10;
		slimbus_devices->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
	} else if (!strcmp(platform_type, "UDP")) {
		slimbus_devices->rf = SLIMBUS_RF_24576;
		slimbus_devices->slimbusclk_drv = 0xC0;
		slimbus_devices->slimbusdata_drv = 0xC3;
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_10;
		slimbus_devices->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
		*type = PLATFORM_UDP;
	} else if (!strcmp(platform_type, "FPGA")) {
		slimbus_devices->rf = SLIMBUS_RF_6144;
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_4_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_4_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_8;
		if (device_type == SLIMBUS_DEVICE_DA_COMBINE_V3)
			slimbus_devices->scene_config_type = SLIMBUS_SCENE_CONFIG_6144_FPGA;
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
		else if (device_type == SLIMBUS_DEVICE_DA_COMBINE_V5)
			slimbus_devices->scene_config_type = SLIMBUS_SCENE_CONFIG_6144_FPGA;
#endif
		*type = PLATFORM_FPGA;
	} else {
		AUDIO_LOGE("platform type define err, platform_type %s", platform_type);
		return -EINVAL;
	}

	return 0;
}

static int32_t slimbus_init_platform_params(const char *platform_type,
	enum slimbus_device device_type, enum platform_type *type)
{
	int32_t ret;

	slimbus_devices->rf = SLIMBUS_RF_6144;
	slimbus_devices->slimbusclk_drv = 0xA8;
	slimbus_devices->slimbusdata_drv = 0xA3;
	slimbus_devices->slimbusclk_offset = IOC_SYS_IOMG_011;
	slimbus_devices->slimbusdata_offset = IOC_SYS_IOMG_012;
	slimbus_devices->slimbusclk_cfg_offset = IOC_SYS_IOCG_013;
	slimbus_devices->slimbusdata_cfg_offset = IOC_SYS_IOCG_014;
	*type = PLATFORM_PHONE;

	ret = slimbus_select_platform_type(platform_type, device_type, type);
	if (ret)
		return ret;

	slimbus_io_param_get();
	slimbus_port_version_get();

	if ((device_type == SLIMBUS_DEVICE_DA_COMBINE_V3) && (*type != PLATFORM_FPGA)) {
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_1_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_1_CSW_32_SL;
		slimbus_devices->cg = SLIMBUS_CG_10;
		slimbus_devices->sm = SLIMBUS_SM_1_CSW_32_SL;
	}
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	if ((device_type == SLIMBUS_DEVICE_DA_COMBINE_V5) && (*type != PLATFORM_FPGA)) {
		AUDIO_LOGI("static init param\n");
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_2_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_2_CSW_32_SL;
		slimbus_devices->cg = SLIMBUS_CG_10;
		slimbus_devices->sm = SLIMBUS_SM_2_CSW_32_SL;
	}
#endif
	AUDIO_LOGI("platform type: %s device_type:%d slimbusclk: 0x%x, slimbusdata: 0x%x, slimbusclk_cfg: 0x%x slimbusdata_cfg: 0x%x, slimbusclk_drv:0x%x, slimbusdata_drv:0x%x",
		platform_type, device_type,
		slimbus_devices->slimbusclk_offset,
		slimbus_devices->slimbusdata_offset,
		slimbus_devices->slimbusclk_cfg_offset,
		slimbus_devices->slimbusdata_cfg_offset,
		slimbus_devices->slimbusclk_drv,
		slimbus_devices->slimbusdata_drv);

	if (pdata)
		pdata->dev_ops->slimbus_device_param_init(slimbus_devices);

	return ret;
}

static void slimbus_da_combine_register(struct slimbus_device_ops *dev_ops,
	struct slimbus_private_data *pd)
{
	dev_ops->release_slimbus_device = slimbus_da_combine_release_device;
	dev_ops->slimbus_track_soundtrigger_activate = slimbus_da_combine_track_soundtrigger_activate;
	dev_ops->slimbus_track_soundtrigger_deactivate = slimbus_da_combine_track_soundtrigger_deactivate;
	dev_ops->slimbus_track_is_fast_soundtrigger = slimbus_da_combine_track_is_fast_soundtrigger;
	dev_ops->slimbus_check_scenes = slimbus_check_scenes;
	dev_ops->slimbus_select_scenes = slimbus_select_scenes;

	pd->track_config_table = track_config_table;
	pd->slimbus_track_max = (uint32_t)DA_COMBINE_V3_BUS_TRACK_TYPE_MAX;

	if (pd->device_type == SLIMBUS_DEVICE_DA_COMBINE_V3)
		slimbus_da_combine_v3_callback_register(dev_ops, pd);
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	else if (pd->device_type == SLIMBUS_DEVICE_DA_COMBINE_V5)
		slimbus_da_combine_v5_callback_register(dev_ops, pd);
#endif
}

static int reg_remap_init(struct slimbus_private_data *priv)
{
	/* 1k */
	priv->asp_cfg_mem = devm_ioremap(priv->dev,
		get_phy_addr(SOC_ACPU_ASP_CFG_BASE_ADDR, PLT_ASP_CFG_MEM), 0x400);
	if (priv->asp_cfg_mem == NULL) {
		AUDIO_LOGE("no memory for asp cfg");
		return -ENOMEM;
	}
	/* 4k */
	priv->sctrl_mem = devm_ioremap(priv->dev,
		get_phy_addr(SOC_ACPU_SCTRL_BASE_ADDR, PLT_SCTRL_MEM), 0x1000);
	if (priv->sctrl_mem == NULL) {
		AUDIO_LOGE("no memory for sctrl");
		return -ENOMEM;
	}

	priv->slimbus_mem = devm_ioremap(priv->dev,
		get_phy_addr(SOC_ACPU_SLIMBUS_BASE_ADDR, PLT_SLIMBUS_MEM), 0xb00);
	if (priv->slimbus_mem == NULL) {
		AUDIO_LOGE("no memory for slimbus");
		return -ENOMEM;
	}

	return 0;
}

static int32_t slimbus_resource_init(struct platform_device *pdev,
	struct slimbus_private_data *pd)
{
	int32_t ret;
	uint32_t clk_asp_subsys = 0;

	ret = reg_remap_init(pd);
	if (ret != 0) {
		AUDIO_LOGE("remap mem failed");
		return ret;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "clk_asp_subsys", &clk_asp_subsys);
	if (ret) {
		AUDIO_LOGI("clk_asp_subsyss not define! use default:480000M");
		clk_asp_subsys = 480000;
	}

	slimbus_utils_init(pd->asp_cfg_mem, clk_asp_subsys);

	return 0;
}

static int32_t slimbus_device_init(struct platform_device *pdev,
	struct slimbus_private_data *pd)
{
	int32_t ret;
	struct slimbus_device_ops *dev_ops = NULL;
	const char *codectype = NULL;
	struct device *dev = &pdev->dev;

	dev_ops = devm_kzalloc(dev, sizeof(*dev_ops), GFP_KERNEL);
	if (!dev_ops) {
		AUDIO_LOGE("kzalloc error");
		return -EFAULT;
	}

	pd->dev_ops = dev_ops;

	ret = of_property_read_string(pdev->dev.of_node, "codec-type", &codectype);
	if (ret == 0 && !strcmp(codectype, "slimbus-da_combine_v3cs")) {
		pd->device_type = SLIMBUS_DEVICE_DA_COMBINE_V3;
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	} else if (ret == 0 && !strcmp(codectype, "slimbus-da_combine_v5")) {
		pd->device_type = SLIMBUS_DEVICE_DA_COMBINE_V5;
#endif
	} else {
		AUDIO_LOGE("do not support");
		WARN_ON(true);
	}

	slimbus_da_combine_register(dev_ops, pd);
	pd->track_state = devm_kzalloc(dev, sizeof(bool) * pd->slimbus_track_max, GFP_KERNEL);
	if (!pd->track_state) {
		AUDIO_LOGE("kzalloc error");
		return -EFAULT;
	}

	ret = dev_ops->create_slimbus_device(&slimbus_devices);
	if (ret) {
		AUDIO_LOGE("slimbus device create failed");
		return -EFAULT;
	}

	return ret;
}

static void slimbus_pm_init(struct platform_device *pdev,
	struct slimbus_private_data *pd)
{
	if (is_pm_runtime_support()) {
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_set_autosuspend_delay(&pdev->dev, 2000); /* 2000ms */
		pm_runtime_mark_last_busy(&pdev->dev);
		pm_runtime_set_active(&pdev->dev);
		pm_suspend_ignore_children(&pdev->dev, true);
		pm_runtime_enable(&pdev->dev);
	}
}

static void release_slimbus_resource(struct slimbus_private_data *pd)
{
	slimbus_utils_deinit();
}

static int32_t process_property(struct platform_device *pdev,
	struct slimbus_private_data *pd, const char **platform_type)
{
	const char *property_value = NULL;
	bool soc_clk1_div_freq_disable = false;
	int32_t ret = of_property_read_string(pdev->dev.of_node, "slimbus_dynamic_freq", &property_value);

	if (!ret && (!strcmp(property_value, "true")))
		pd->slimbus_dynamic_freq_enable = true;

	ret = of_property_read_string(pdev->dev.of_node, "platform-type", platform_type);
	if (ret) {
		AUDIO_LOGE("of_property_read_string return error ret: %d", ret);
		return -EFAULT;
	}

	if (of_property_read_bool(pdev->dev.of_node, "switch_framer_disable"))
		pd->switch_framer_disable = true;

	if (of_property_read_bool(pdev->dev.of_node, "soc_clk1_div_freq_disable"))
		soc_clk1_div_freq_disable = true;
	else
		soc_clk1_div_freq_disable = false;

	slimbus_utils_set_soc_div_freq_disable(soc_clk1_div_freq_disable);

	AUDIO_LOGI("soc div diable: %d", soc_clk1_div_freq_disable);

	AUDIO_LOGI("freq update disable: %d", pd->freq_update_disable);

	AUDIO_LOGI("platform type:%s device_type: %d, dynamic freq %d switch_framer_disable: %d",
		*platform_type, pd->device_type, pd->slimbus_dynamic_freq_enable, pd->switch_framer_disable);

	return 0;
}

static int32_t slimbus_drv_preprocess(struct slimbus_private_data *pd)
{
	int32_t ret = slimbus_drv_init(pd->type,
		pd->slimbus_mem,
		pd->asp_cfg_mem,
		pd->sctrl_mem,
		get_irq_id());

	if (ret) {
		AUDIO_LOGE("slimbus drv init failed");
		goto exit;
	}

	ret = slimbus_drv_bus_configure(&bus_config[SLIMBUS_BUS_CONFIG_NORMAL]);
	if (ret) {
		AUDIO_LOGE("slimbus bus configure failed");
		slimbus_drv_release(get_irq_id());
		goto exit;
	}

exit:
	return ret;
}

static void param_init(struct platform_device *pdev, struct slimbus_private_data *pd)
{
	pd->framerstate = FRAMER_SOC;
	pd->lastframer = FRAMER_SOC;
	platform_set_drvdata(pdev, pd);
	slimbus_pm_init(pdev, pd);
}

static struct codec_bus_ops slimbus_ops = {
	.runtime_get = slimbus_runtime_get,
	.runtime_put = slimbus_runtime_put,
	.switch_framer = slimbus_switch_framer,
	.enum_device = slimbus_enum_device,
	.get_framer_type = slimbus_debug_get_framer,
	.activate = slimbus_activate,
	.deactivate = slimbus_deactivate,
};

static int32_t slimbus_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int32_t ret;
	const char *platform_type = NULL;
	struct slimbus_private_data *pd = devm_kzalloc(dev, sizeof(struct slimbus_private_data), GFP_KERNEL);

	if (!pd) {
		AUDIO_LOGE("not enough memory for slimbus_private_data");
		return -ENOMEM;
	}

	pdata = pd;
	pdata->dev = dev;

	ret = slimbus_resource_init(pdev, pd);
	if (ret != 0) {
		AUDIO_LOGE("resource init fail!");
		return ret;
	}

	ret = slimbus_device_init(pdev, pd);
	if (ret) {
		AUDIO_LOGE("device init fail!");
		goto device_init_err;
	}
	pd->slimbus_dynamic_freq_enable = false;

	ret = process_property(pdev, pd, &platform_type);
	if (ret)
		goto slimbus_err;

	ret = slimbus_init_platform_params(platform_type, pd->device_type, &(pd->type));
	if (ret) {
		AUDIO_LOGE("slimbus platform param init fail");
		goto slimbus_err;
	}

	slimbus_utils_module_enable(slimbus_devices, true);

	ret = slimbus_drv_preprocess(pd);
	if (ret)
		goto slimbus_err;

	param_init(pdev, pd);

	register_ops(&slimbus_ops);

	AUDIO_LOGI("slimbus probe success");
	return 0;

slimbus_err:
	if (pdata)
		pdata->dev_ops->release_slimbus_device(slimbus_devices);
device_init_err:
	release_slimbus_resource(pd);
	pdata = NULL;

	return -EFAULT;
}

static int32_t slimbus_remove(struct platform_device *pdev)
{
	int32_t ret;
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	if (!pd) {
		AUDIO_LOGE("pd is null");
		return -EINVAL;
	}

	if (is_pm_runtime_support()) {
		pm_runtime_resume(dev);
		pm_runtime_disable(dev);
	}

	ret = slimbus_switch_framer(FRAMER_SOC);
	if (ret)
		AUDIO_LOGE("switch framer to SLIMBUS_DEVICE_SOC fail, ret: %d", ret);

	slimbus_drv_release(get_irq_id());

	pd->dev_ops->release_slimbus_device(slimbus_devices);

	release_slimbus_resource(pd);
	platform_set_drvdata(pdev, NULL);

	if (is_pm_runtime_support())
		pm_runtime_set_suspended(dev);

	return 0;
}

static const struct of_device_id slimbus_match[] = {
	{
		.compatible = "candance,slimbus",
	},
	{},
};
MODULE_DEVICE_TABLE(of, slimbus_match);

static struct platform_driver slimbus_driver = {
	.probe = slimbus_probe,
	.remove = slimbus_remove,
	.driver = {
		.name = "hisilicon,slimbus",
		.owner = THIS_MODULE,
		.of_match_table = slimbus_match,
	},
};

static int32_t __init slimbus_init(void)
{
	int32_t ret;

	slimbus_driver.driver.pm = slimbus_pm_get_ops();
	ret = platform_driver_register(&slimbus_driver);
	if (ret)
		AUDIO_LOGE("driver register failed");

	return ret;
}

static void __exit slimbus_exit(void)
{
	platform_driver_unregister(&slimbus_driver);
}
fs_initcall_sync(slimbus_init);
module_exit(slimbus_exit);

MODULE_LICENSE("GPL");

