/*
 * da_separate_mbhc.c -- da_separate mbhc driver
 *
 * Copyright (c) 2017 Hisilicon Technologies Co., Ltd.
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

#include "da_separate_mbhc.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/adc.h>
#include <sound/jack.h>
#include <linux/input/matrix_keypad.h>
#include <linux/interrupt.h>
#include <linux/version.h>

#include "asoc_adapter.h"
#ifdef CONFIG_HUAWEI_AUDIO
#include "hs_auto_calib.h"
#endif
#include "da_separate_reg.h"
#ifdef CONFIG_SND_SOC_CODEC_DA_SEPARATE_V3
#include "da_separate_utils.h"
#include "da_separate_v3.h"
#endif
#if (defined(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V5)) || (defined(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V6L))
#include "da_separate_utils.h"
#include "da_separate_v5.h"
#endif

#ifdef CONFIG_SND_SOC_CODEC_DA_SEPARATE_V2
#include "da_separate/da_separate_v2/da_separate_v2_utility.h"
#ifdef CONFIG_PMIC_55V500_PMU
#include "da_separate/da_separate_v5_pmu_reg_def.h"
#else
#include "da_separate/da_separate_v2/da_separate_v2_pmu_reg_def.h"
#endif
#include "da_separate/da_separate_v2/da_separate_v2.h"
#endif

#ifdef CONFIG_SND_SOC_ASP_CODEC_ANA
#include "asp_codec_ana_reg_def.h"
#endif

#ifdef CONFIG_HUAWEI_AUDIO
#include "huawei_platform/audio/usb_analog_hs_interface.h"
#include "huawei_platform/audio/ana_hs_common.h"
#include "ana_hs_kit/ana_hs.h"
#endif

#if defined(CONFIG_HUAWEI_HEADSET_DEBUG) && defined(CONFIG_HUAWEI_AUDIO)
#include "headset_debug.h"
#endif

#include "da_separate_mbhc_custom.h"
#include "audio_log.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "da_separate_mbhc"
#else
#define LOG_TAG "DA_separate_mbhc"
#endif

#define UNUSED_PARAMETER(x)        (void)(x)

#define MAX_UINT32                     0xFFFFFFFF
#define MICBIAS_PD_WAKE_LOCK_MS        3500
#define MICBIAS_PD_DELAY_MS            3000
#define IRQ_HANDLE_WAKE_LOCK_MS        2000
#define LINEOUT_PO_RECHK_WAKE_LOCK_MS  3500
#define LINEOUT_PO_RECHK_DELAY_MS      800
#define DA_SEPARATE_HKADC_CHN               14
#define DA_SEPARATE_INVALID_IRQ             (-1)

/* 0-earphone not pluged, 1-earphone pluged */
#define IRQ_STAT_PLUG_IN (0x1 <<  HS_DET_IRQ_OFFSET)
/* 0-normal btn event    1- no normal btn event */
#define IRQ_STAT_KEY_EVENT (0x1 << HS_MIC_NOR2_IRQ_OFFSET)
/* 0-eco btn event     1- no eco btn event */
#define IRQ_STAT_ECO_KEY_EVENT (0x1 << HS_MIC_ECO_IRQ_OFFSET)
#define JACK_RPT_MSK_BTN (SND_JACK_BTN_0 | SND_JACK_BTN_1 | SND_JACK_BTN_2 | SND_JACK_BTN_3)

enum jack_states {
	PLUG_NONE           = 0,
	PLUG_HEADSET        = 1,
	PLUG_HEADPHONE      = 2,
	PLUG_INVERT         = 3,
	PLUGING             = 4,
	PLUG_INVALID        = 5,
	PLUG_LINEOUT        = 6,
};

enum irq_action_type {
	IRQ_PLUG_OUT = 0,
	IRQ_PLUG_IN,
	IRQ_ECO_BTN_DOWN,
	IRQ_ECO_BTN_UP,
	IRQ_COMP_L_BTN_DOWN,
	IRQ_COMP_L_BTN_UP,
	IRQ_COMP_H_BTN_DOWN,
	IRQ_COMP_H_BTN_UP,
	IRQ_ACTION_NUM,
};

enum delay_action_type {
	DELAY_MICBIAS_ENABLE,
	DELAY_LINEOUT_RECHECK,
	DELAY_ACTION_NUM,
};

enum hs_event_voltage_type {
	VOLTAGE_POLE3 = 0,
	VOLTAGE_POLE4,
	VOLTAGE_PLAY,
	VOLTAGE_VOL_UP,
	VOLTAGE_VOL_DOWN,
	VOLTAGE_VOICE_ASSIST,
	VOLTAGE_NUM,
};

enum delay_time {
	/* ms */
	HS_TIME_PI              = 0,
	HS_TIME_PI_DETECT       = 800,
	HS_TIME_COMP_IRQ_TRIG   = 30,
	HS_TIME_COMP_IRQ_TRIG_2 = 50,
};

struct mbhc_wq {
	struct workqueue_struct *dwq;
	struct delayed_work dw;
};

struct jack_data {
	struct snd_soc_jack jack;
	int report;
	struct switch_dev sdev;
	bool is_dev_registered;
};

struct hs_handler_config {
	void (*handler)(struct work_struct *work);
	const char *name;
};

struct voltage_interval {
	int min;
	int max;
};

struct mbhc_data {
	struct da_separate_mbhc mbhc_pub;
	struct snd_soc_component *codec;
	struct jack_data hs_jack;
	enum jack_states hs_status;
	enum jack_states old_hs_status;
	int pressed_btn_type;
	int adc_voltage;

	struct mbhc_wq irq_wqs[IRQ_ACTION_NUM];
	struct mbhc_wq dma_irq_handle_wqs[DELAY_ACTION_NUM];

	struct mutex io_mutex;
	struct mutex hkadc_mutex;
	struct mutex plug_mutex;
	struct mutex hs_micbias_mutex;
	struct wakeup_source *wake_lock;

	unsigned int hs_micbias_dapm;
	bool hs_micbias_mbhc;
	bool pre_status_is_lineout;
	bool usb_ana_need_recheck;
	bool is_emulater;
	int gpio_intr_pin;
	int gpio_irq;

	struct voltage_interval intervals[VOLTAGE_NUM];
	struct da_separate_mbhc_ops mbhc_ops;
};

static const char *const mbhc_print_str[] = {
	"pole3", "pole4", "play", "vol_up", "vol_down", "voice_assist",
};

struct voltage_dts_node {
	const char *min;
	const char *max;
};

struct voltage_dts_node voltage_config[VOLTAGE_NUM] = {
	{ "hs_3_pole_min_voltage", "hs_3_pole_max_voltage" },
	{ "hs_4_pole_min_voltage", "hs_4_pole_max_voltage" },
	{ "btn_play_min_voltage", "btn_play_max_voltage" },
	{ "btn_volume_up_min_voltage", "btn_volume_up_max_voltage" },
	{ "btn_volume_down_min_voltage", "btn_volume_down_max_voltage" },
	{ "btn_voice_assistant_min_voltage", "btn_voice_assistant_max_voltage" }
};

static inline unsigned int irq_status_check(const struct mbhc_data *priv,
	unsigned int irq_stat_bit)
{
	unsigned int irq_state;
	unsigned int ret = 0;

	irq_state = snd_soc_component_read32(priv->codec, ANA_IRQ_SIG_STAT_REG);
	irq_state &= irq_stat_bit;

	switch (irq_stat_bit) {
	case IRQ_STAT_KEY_EVENT:
	case IRQ_STAT_ECO_KEY_EVENT:
		/* convert */
		ret = !irq_state;
		break;
	case IRQ_STAT_PLUG_IN:
		ret = irq_state;
#ifdef CONFIG_HUAWEI_AUDIO
		if (check_usb_analog_hs_support() || ana_hs_support_usb_sw()) {
			if(usb_analog_hs_check_headset_pluged_in() == ANA_HS_PLUG_IN ||
				ana_hs_pluged_state() == ANA_HS_PLUG_IN) {
				ret = ANA_HS_PLUG_IN;
				AUDIO_LOGI("usb ananlog hs is IRQ_PLUG_IN\n");
			} else {
				ret = ANA_HS_PLUG_OUT;
				AUDIO_LOGI("usb ananlog hs is IRQ_PLUG_OUT\n");
			}
		}
#endif
		break;
	default:
		/* no need to convert */
		ret = irq_state;
		break;
	}
	AUDIO_LOGD("bit=0x%x, ret=%d\n", irq_stat_bit, ret);

	return ret;
}

static inline void irqs_clr(struct mbhc_data *priv,
	unsigned int irqs)
{
	AUDIO_LOGD("Before irqs clr,IRQ_REG0=0x%x, clr=0x%x\n",
		snd_soc_component_read32(priv->codec, ANA_IRQ_REG0_REG), irqs);
	snd_soc_component_write_adapter(priv->codec, ANA_IRQ_REG0_REG, irqs);
	AUDIO_LOGD("After irqs clr,IRQ_REG0=0x%x\n",
		snd_soc_component_read32(priv->codec, ANA_IRQ_REG0_REG));
}

static inline void irqs_mask_set(struct mbhc_data *priv,
	unsigned int irqs)
{
	AUDIO_LOGD("Before mask set,IRQM_REG0=0x%x, mskset=0x%x\n",
		snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG), irqs);
	snd_soc_component_update_bits(priv->codec, ANA_IRQM_REG0_REG, irqs, 0xff);
	AUDIO_LOGD("After mask set,IRQM_REG0=0x%x\n",
		snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG));
}

static inline void irqs_mask_clr(struct mbhc_data *priv,
	unsigned int irqs)
{
	AUDIO_LOGD("Before mask clr,IRQM_REG0=0x%x, mskclr=0x%x\n",
		snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG), irqs);
	snd_soc_component_update_bits(priv->codec, ANA_IRQM_REG0_REG, irqs, 0);
	AUDIO_LOGD("After mask clr,IRQM_REG0=0x%x\n",
		snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG));
}

static void hs_micbias_power(struct mbhc_data *priv, bool enable)
{
	unsigned int irq_mask;

	/* to avoid irq while MBHD_COMP power up, mask all COMP irq,when pwr up finished clean it and cancel mask */
	irq_mask = snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG);
	irqs_mask_set(priv, irq_mask | IRQ_MSK_COMP);

	if (enable) {
		/* open ibias */
		priv->mbhc_ops.set_ibias_hsmicbias(priv->codec, true);

		/* disable ECO--key detect mode switch to NORMAL mode */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_ECO_EN_OFFSET), 0);

		/* micbias discharge off */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW7_REG,
			BIT(HSMICB_DSCHG_OFFSET), 0);

		/* hs micbias pu */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW7_REG,
			BIT(HSMICB_PD_OFFSET), 0);
		msleep(10);
		/* enable NORMAL key detect and identify:1.open normal compare 2.key identify adc voltg buffer on */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_COMP_PD_OFFSET), 0);
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_BUFF_PD_OFFSET), 0);
		usleep_range(100, 150);
	} else {
		/* disable NORMAL key detect and identify */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_BUFF_PD_OFFSET), 0xff);
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_COMP_PD_OFFSET), 0xff);

		/* hs micbias pd */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW7_REG,
			BIT(HSMICB_PD_OFFSET), 0xff);
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW7_REG,
			BIT(HSMICB_DSCHG_OFFSET), 0xff);
		msleep(5);
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW7_REG,
			BIT(HSMICB_DSCHG_OFFSET), 0);

		/* key detect mode switch to ECO mode */
		snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_ECO_EN_OFFSET), 0xff);
		msleep(20);

		/* close ibias */
		priv->mbhc_ops.set_ibias_hsmicbias(priv->codec, false);
		irqs_clr(priv, IRQ_MSK_COMP);
		irqs_mask_clr(priv, IRQ_MSK_BTN_ECO);
	}
}

static void set_hs_micbias(struct mbhc_data *priv, bool enable)
{
	AUDIO_LOGI("begin,en = %d\n", enable);
	if (WARN_ON(!priv))
		return;

	/* hs_micbias power up,then power down 3 seconds later */
	cancel_delayed_work(&priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dw);
	flush_workqueue(priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dwq);

	if (enable) {
		/* read hs_micbias pd status,1:pd */
		if (((snd_soc_component_read32(priv->codec, CODEC_ANA_RW7_REG))&(BIT(HSMICB_PD_OFFSET))))
			hs_micbias_power(priv, true);
	} else {
		if ((priv->hs_micbias_dapm == 0) && !priv->hs_micbias_mbhc) {
			__pm_wakeup_event(priv->wake_lock, MICBIAS_PD_WAKE_LOCK_MS);
			mod_delayed_work(priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dwq,
					&priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dw,
					msecs_to_jiffies(MICBIAS_PD_DELAY_MS));
		}
	}
}

void da_separate_mbhc_set_micbias(struct da_separate_mbhc *mbhc, bool enable)
{
	struct mbhc_data *priv = (struct mbhc_data *)mbhc;

	IN_FUNCTION;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null\n");
		return;
	}

	mutex_lock(&priv->hs_micbias_mutex);
	if (enable) {
		if (priv->hs_micbias_dapm == 0)
			set_hs_micbias(priv, true);

		if (priv->hs_micbias_dapm == MAX_UINT32) {
			AUDIO_LOGE("hs_micbias_dapm will overflow\n");
			mutex_unlock(&priv->hs_micbias_mutex);
			return;
		}
		++priv->hs_micbias_dapm;
	} else {
		if (priv->hs_micbias_dapm == 0) {
			AUDIO_LOGE("hs_micbias_dapm is 0, fail to disable micbias\n");
			mutex_unlock(&priv->hs_micbias_mutex);
			return;
		}

		--priv->hs_micbias_dapm;
		if (priv->hs_micbias_dapm == 0)
			set_hs_micbias(priv, false);
	}
	mutex_unlock(&priv->hs_micbias_mutex);

	OUT_FUNCTION;
}

static void hs_micbias_mbhc_enable(struct mbhc_data *priv, bool enable)
{
	IN_FUNCTION;

	if (WARN_ON(!priv))
		return;

	mutex_lock(&priv->hs_micbias_mutex);
	if (enable) {
		if (!priv->hs_micbias_mbhc) {
			set_hs_micbias(priv, true);
			priv->hs_micbias_mbhc = true;
		}
	} else {
		if (priv->hs_micbias_mbhc) {
			priv->hs_micbias_mbhc = false;
			set_hs_micbias(priv, false);
		}
	}
	mutex_unlock(&priv->hs_micbias_mutex);

	OUT_FUNCTION;
}

static void hs_micbias_delay_pd_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dw.work);

	IN_FUNCTION;

	if (WARN_ON(!priv))
		return;

	hs_micbias_power(priv, false);

	OUT_FUNCTION;
}

static inline int read_hkadc_value(struct mbhc_data *priv)
{
	int hkadc_value;

	if (WARN_ON(!priv))
		return -EINVAL;

	priv->adc_voltage = lpm_adc_get_value(DA_SEPARATE_HKADC_CHN);
	if (priv->adc_voltage < 0)
		return -EFAULT;

	/* HKADC voltage, real value should devided 0.6 */
	hkadc_value = ((priv->adc_voltage)*(10))/(6);
	AUDIO_LOGI("adc_voltage = %d\n", priv->adc_voltage);

	return hkadc_value;
}

static irqreturn_t irq_handler(int irq, void *data)
{
	struct mbhc_data *priv = NULL;
	unsigned int i;
	unsigned int irqs;
	unsigned int irq_mask;
	unsigned int irq_masked;

	unsigned int irq_time[IRQ_ACTION_NUM] = {
		HS_TIME_PI, HS_TIME_PI_DETECT,
		HS_TIME_COMP_IRQ_TRIG, HS_TIME_COMP_IRQ_TRIG_2,
		HS_TIME_COMP_IRQ_TRIG, HS_TIME_COMP_IRQ_TRIG_2,
		HS_TIME_COMP_IRQ_TRIG, HS_TIME_COMP_IRQ_TRIG_2,
	};

	AUDIO_LOGD(">>>>>Begin\n");
	if (data == NULL) {
		AUDIO_LOGE("data is null\n");
		return IRQ_NONE;
	}

	priv = (struct mbhc_data *)data;

	irqs = snd_soc_component_read32(priv->codec, ANA_IRQ_REG0_REG);
	if (irqs == 0)
		return IRQ_NONE;

	irq_mask = snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG);
	irq_mask &= (~IRQ_MSK_PLUG_IN);
	irq_masked = irqs & (~irq_mask);

	if (irq_masked == 0) {
		irqs_clr(priv, irqs);
		return IRQ_HANDLED;
	}

	__pm_wakeup_event(priv->wake_lock, IRQ_HANDLE_WAKE_LOCK_MS);

	for (i = IRQ_PLUG_OUT; i < IRQ_ACTION_NUM; i++) {
		if (irq_masked & BIT(IRQ_ACTION_NUM - i - 1))
			queue_delayed_work(priv->irq_wqs[i].dwq,
					&priv->irq_wqs[i].dw,
					msecs_to_jiffies(irq_time[i]));
	}

	/* clear all read irq bits */
	irqs_clr(priv, irqs);
	AUDIO_LOGD("<<<End, irq_masked=0x%x,irq_read=0x%x, irq_aftclr=0x%x, IRQM=0x%x, IRQ_RAW=0x%x\n",
			irq_masked, irqs,
			snd_soc_component_read32(priv->codec, ANA_IRQ_REG0_REG),
			snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG),
			snd_soc_component_read32(priv->codec, ANA_IRQ_SIG_STAT_REG));

	return IRQ_HANDLED;
}

static_t void hs_jack_report(struct mbhc_data *priv)
{
	int jack_report = 0;

	switch (priv->hs_status) {
	case PLUG_NONE:
		jack_report = 0;
		AUDIO_LOGI("plug out\n");
		break;
	case PLUG_HEADSET:
		jack_report = SND_JACK_HEADSET;
		AUDIO_LOGI("4-pole headset plug in\n");
		break;
	case PLUG_INVERT:
		jack_report = SND_JACK_HEADPHONE;
		AUDIO_LOGI("invert headset plug in\n");
		break;
	case PLUG_HEADPHONE:
		jack_report = SND_JACK_HEADPHONE;
		AUDIO_LOGI("3-pole headphone plug in\n");
		break;
	default:
		AUDIO_LOGE("error hs_status %d\n", priv->hs_status);
		return;
	}

	/* report jack status */
	snd_soc_jack_report(&priv->hs_jack.jack, jack_report, SND_JACK_HEADSET);
#if defined(CONFIG_HUAWEI_HEADSET_DEBUG) && defined(CONFIG_HUAWEI_AUDIO)
	headset_debug_set_state(priv->hs_status, false);
#endif
#ifdef CONFIG_SWITCH
	switch_set_state(&priv->hs_jack.sdev, priv->hs_status);
#endif
	/* for avoiding AQM headset-mic TDD problem */
	if (da_separate_get_mic_mute_status())
		da_separate_hs_mic_mute(priv->codec);
}

static void hs_type_recognize(struct mbhc_data *priv, int hkadc_value)
{
	if (hkadc_value <= priv->intervals[VOLTAGE_POLE3].max) {
		priv->hs_status = PLUG_HEADPHONE;
		AUDIO_LOGI("headphone is 3 pole, saradc=%d\n", hkadc_value);
	} else if (hkadc_value >= priv->intervals[VOLTAGE_POLE4].min &&
			hkadc_value <= priv->intervals[VOLTAGE_POLE4].max) {
		priv->hs_status = PLUG_HEADSET;
		AUDIO_LOGI("headphone is 4 pole, saradc=%d\n", hkadc_value);
#ifndef CONFIG_AUDIO_FACTORY_MODE
	} else if (hkadc_value >= priv->intervals[VOLTAGE_POLE4].max) {
		priv->hs_status = PLUG_LINEOUT;
		AUDIO_LOGI("headphone is lineout, saradc=%d\n", hkadc_value);
#endif
	} else {
		priv->hs_status = PLUG_INVERT;
		AUDIO_LOGI("headphone is invert 4 pole, saradc=%d\n", hkadc_value);
	}
}

static void hs_plug_out_detect(struct mbhc_data *priv)
{
	IN_FUNCTION;
	if (WARN_ON(!priv))
		return;

	mutex_lock(&priv->plug_mutex);
	/*
	 * Avoid hs_micbias_delay_pd_dw waiting for entering eco,
	 * so cancel the delay work then power off hs_micbias.
	 */
	cancel_delayed_work(&priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dw);
	flush_workqueue(priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dwq);
	priv->hs_micbias_mbhc = false;
	hs_micbias_power(priv, false);

	/* mbhc vref pd */
	snd_soc_component_update_bits(priv->codec, DA_SEPARATE_MBHD_VREF_CTRL,
		BIT(MBHD_VREF_PD_OFFSET), 0xff);

	/* disable ECO */
	snd_soc_component_update_bits(priv->codec, CODEC_ANA_RW8_REG,
		BIT(MBHD_ECO_EN_OFFSET), 0);

	/* mask all btn irq */
	irqs_mask_set(priv, IRQ_MSK_COMP);
	mutex_lock(&priv->io_mutex);
	priv->hs_jack.report = 0;

	if (priv->pressed_btn_type != 0) {
		/* report key event */
		AUDIO_LOGI("report type=0x%x, status=0x%x\n",
			priv->hs_jack.report,
			priv->hs_status);
		snd_soc_jack_report(&priv->hs_jack.jack,
			priv->hs_jack.report, JACK_RPT_MSK_BTN);
	}

	priv->pressed_btn_type = 0;
	priv->hs_status = PLUG_NONE;
	priv->old_hs_status = PLUG_INVALID;
	mutex_unlock(&priv->io_mutex);

	priv->pre_status_is_lineout = false;

	/* report headset info */
	hs_jack_report(priv);
#ifdef CONFIG_HUAWEI_AUDIO
	headset_auto_calib_reset_interzone();
#endif
	irqs_clr(priv, IRQ_MSK_COMP);
	irqs_mask_clr(priv, IRQ_MSK_PLUG_IN);
	mutex_unlock(&priv->plug_mutex);

	OUT_FUNCTION;
}

#ifdef CONFIG_HUAWEI_AUDIO
static void usb_ana_hs_recheck(struct mbhc_data *priv, int hkadc_value)
{
	irqs_mask_set(priv, IRQ_MSK_COMP);
	hs_micbias_power(priv, false);
	usb_ana_hs_mic_switch_change_state();
	ana_hs_mic_gnd_swap();
	msleep(10);
	hs_micbias_power(priv, true);
	mutex_lock(&priv->hkadc_mutex);
	hkadc_value = read_hkadc_value(priv);
	if (hkadc_value < 0) {
		AUDIO_LOGE("invalid hkadc: %d\n", hkadc_value);
		mutex_unlock(&priv->hkadc_mutex);
		return;
	}
	hs_type_recognize(priv, hkadc_value);
	mutex_unlock(&priv->hkadc_mutex);
}
#endif

static void hs_status_process(struct mbhc_data *priv)
{
#ifdef CONFIG_HUAWEI_AUDIO
	int hkadc_value = 0;
	bool usb_ana_hs_support =
		check_usb_analog_hs_support() || ana_hs_support_usb_sw();
#endif

	if ((priv->hs_status != PLUG_LINEOUT) &&
		(priv->hs_status != PLUG_NONE) &&
		(priv->hs_status != priv->old_hs_status)) {
#ifdef CONFIG_HUAWEI_AUDIO
		if (usb_ana_hs_support &&
			(priv->hs_status == PLUG_INVERT ||
			priv->hs_status == PLUG_HEADPHONE))
			usb_ana_hs_recheck(priv, hkadc_value);
#endif

		priv->old_hs_status = priv->hs_status;
		AUDIO_LOGI("hs status=%d, pre_status_is_lineout:%d\n",
			priv->hs_status, priv->pre_status_is_lineout);
		/* report headset info */
		hs_jack_report(priv);
	} else if (priv->hs_status == PLUG_LINEOUT) {
		priv->pre_status_is_lineout = true;
		AUDIO_LOGI("hs status=%d, old_hs_status=%d, lineout is plugin\n",
			priv->hs_status, priv->old_hs_status);
		/* not the first time recognize as lineout, headphone/set plug out from lineout */
		if (priv->old_hs_status != PLUG_INVALID) {
			priv->hs_status = PLUG_NONE;
			priv->old_hs_status = PLUG_INVALID;
			/* headphone/set plug out from lineout,need report plugout event */
			hs_jack_report(priv);
			priv->hs_status = PLUG_LINEOUT;
		}
	} else {
		AUDIO_LOGI("hs status=%d(old=%d) not changed\n", priv->hs_status, priv->old_hs_status);
		/* recheck usb headset when invert or hp && same as old */
#ifdef CONFIG_HUAWEI_AUDIO
		if (priv->usb_ana_need_recheck && usb_ana_hs_support &&
			(priv->hs_status == PLUG_INVERT ||
			priv->hs_status == PLUG_HEADPHONE)) {
			usb_ana_hs_recheck(priv, hkadc_value);
			if (priv->hs_status != priv->old_hs_status) {
				hs_jack_report(priv);
				priv->old_hs_status = priv->hs_status;
			}
		}
#endif
	}
}

static void hs_plug_in_detect(struct mbhc_data *priv)
{
	int hkadc_value;

	IN_FUNCTION;

	if (WARN_ON(!priv))
		return;

	/* check state - plugin */
	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		AUDIO_LOGI("plug_in SIG STAT: not plug in, irq_state=0x%x, IRQM=0x%x, RAW_irq =0x%x\n",
				snd_soc_component_read32(priv->codec, ANA_IRQ_REG0_REG),
				snd_soc_component_read32(priv->codec, ANA_IRQM_REG0_REG),
				snd_soc_component_read32(priv->codec, ANA_IRQ_SIG_STAT_REG));

		hs_plug_out_detect(priv);
		return;
	}

	mutex_lock(&priv->plug_mutex);
	/* mask plug out */
	irqs_mask_set(priv, IRQ_MSK_PLUG_OUT | IRQ_MSK_COMP);
	mutex_lock(&priv->hkadc_mutex);
	snd_soc_component_update_bits(priv->codec, DA_SEPARATE_MBHD_VREF_CTRL,
		BIT(MBHD_VREF_PD_OFFSET), 0);
	hs_micbias_mbhc_enable(priv, true);
	msleep(150);
	hkadc_value = read_hkadc_value(priv);
	if (hkadc_value < 0) {
		AUDIO_LOGE("get adc fail,can't read adc value\n");
		mutex_unlock(&priv->hkadc_mutex);
		mutex_unlock(&priv->plug_mutex);
		return;
	}

	/* value greater than 2565 can not trigger eco btn,
	 * so,the hs_micbias can't be closed until second detect finish.
	 */
	if ((hkadc_value <= priv->intervals[VOLTAGE_POLE4].max) &&
		(priv->pre_status_is_lineout == false))
		hs_micbias_mbhc_enable(priv, false);

	mutex_unlock(&priv->hkadc_mutex);

	mutex_lock(&priv->io_mutex);
	hs_type_recognize(priv, hkadc_value);

	irqs_clr(priv, IRQ_MSK_PLUG_OUT);
	irqs_mask_clr(priv, IRQ_MSK_PLUG_OUT);
	mutex_unlock(&priv->io_mutex);

	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		AUDIO_LOGI("plug out happens\n");
		mutex_unlock(&priv->plug_mutex);
		hs_plug_out_detect(priv);
		return;
	}

	hs_status_process(priv);

	/* to avoid irq while MBHD_COMP power up, mask the irq then clean it */
	irqs_clr(priv, IRQ_MSK_COMP);
	irqs_mask_clr(priv, IRQ_MSK_BTN_NOR);
	mutex_unlock(&priv->plug_mutex);

	OUT_FUNCTION;
}

static int hs_btn_type_recognize(const struct voltage_interval *voltages, int hkadc_value)
{
	int btn_type = 0;
	unsigned int i;
	int jacks[] = { SND_JACK_BTN_0,
		SND_JACK_BTN_1, SND_JACK_BTN_2, SND_JACK_BTN_3 };

	for (i = VOLTAGE_PLAY; i < VOLTAGE_NUM; i++) {
		if (hkadc_value >= voltages[i].min && hkadc_value <= voltages[i].max) {
			btn_type = jacks[i - VOLTAGE_PLAY];
			AUDIO_LOGI("key %s is pressed down, saradc value: %d\n",
				mbhc_print_str[i], hkadc_value);
			break;
		}
	}

	if (i == VOLTAGE_NUM)
		AUDIO_LOGE("hkadc value: %d is not in range\n", hkadc_value);

	return btn_type;
}

static void hs_btn_report(struct mbhc_data *priv, int btn_type)
{
	mutex_lock(&priv->io_mutex);
	priv->pressed_btn_type = btn_type;
	priv->hs_jack.report = btn_type;
	mutex_unlock(&priv->io_mutex);

	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		AUDIO_LOGI("plug out happened\n");
	} else {
		/* report key event */
		AUDIO_LOGI("report type=0x%x, status=0x%x\n",
			priv->hs_jack.report, priv->hs_status);
		snd_soc_jack_report(&priv->hs_jack.jack,
			priv->hs_jack.report, JACK_RPT_MSK_BTN);
	}
}

static void hs_btn_down_detect(struct mbhc_data *priv)
{
	int pr_btn_type;
	int hkadc_value;

	IN_FUNCTION;

	if (WARN_ON(!priv))
		return;

	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		AUDIO_LOGI("plug out happened\n");
		return;
	}

	if (priv->hs_status != PLUG_HEADSET) {
		/* enter the second detect,it's triggered by btn irq  */
		AUDIO_LOGI("enter btn_down 2nd time hp type recognize, btn_type=0x%x\n",
			priv->pressed_btn_type);
		hs_plug_in_detect(priv);
		return;
	}

	if (priv->pressed_btn_type != PLUG_NONE) {
		AUDIO_LOGE("btn_type:0x%x has been pressed\n", priv->pressed_btn_type);
		return;
	}

	/* hs_micbias power up,then power down 3 seconds later */
	cancel_delayed_work(&priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dw);
	flush_workqueue(priv->dma_irq_handle_wqs[DELAY_MICBIAS_ENABLE].dwq);

	mutex_lock(&priv->hkadc_mutex);
	hs_micbias_mbhc_enable(priv, true);
	hkadc_value = read_hkadc_value(priv);
	if (hkadc_value < 0) {
		AUDIO_LOGE("get adc fail,can't read adc value, %d\n", hkadc_value);
		mutex_unlock(&priv->hkadc_mutex);
		return;
	}

	if (!priv->pre_status_is_lineout)
		hs_micbias_mbhc_enable(priv, false);

	mutex_unlock(&priv->hkadc_mutex);
	msleep(30);
	/* micbias power up have done, now is in normal mode, clean all COMP IRQ and cancel NOR int mask */
	irqs_clr(priv, IRQ_MSK_COMP);
	irqs_mask_clr(priv, IRQ_MSK_BTN_NOR);
	AUDIO_LOGI("mask clean\n");

	pr_btn_type = hs_btn_type_recognize(priv->intervals, hkadc_value);
	if (pr_btn_type == SND_JACK_BTN_3)
		goto VOCIE_ASSISTANT_KEY;

#ifdef CONFIG_HUAWEI_AUDIO
	startup_fsm(REC_JUDGE, hkadc_value, &pr_btn_type);
#endif

VOCIE_ASSISTANT_KEY:
	hs_btn_report(priv, pr_btn_type);

	OUT_FUNCTION;
}

static void hs_btn_up_detect(struct mbhc_data *priv)
{
	IN_FUNCTION;

	if (WARN_ON(!priv))
		return;

	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		AUDIO_LOGI("plug out happened\n");
		return;
	}

	mutex_lock(&priv->io_mutex);
	if (priv->pressed_btn_type == 0) {
		mutex_unlock(&priv->io_mutex);

		/* second detect */
		if ((priv->hs_status == PLUG_INVERT) ||
		(priv->hs_status == PLUG_HEADPHONE)
		|| (priv->hs_status == PLUG_LINEOUT)) {
			AUDIO_LOGI("enter btn_up 2nd time hp type recognize\n");
				hs_plug_in_detect(priv);
		} else {
			AUDIO_LOGI("ignore the key up irq\n");
		}

		return;
	}

	priv->hs_jack.report = 0;

	/* report key event */
	AUDIO_LOGI("report type=0x%x, status=0x%x\n",
		priv->hs_jack.report, priv->hs_status);
	snd_soc_jack_report(&priv->hs_jack.jack,
		priv->hs_jack.report, JACK_RPT_MSK_BTN);
	priv->pressed_btn_type = 0;
	mutex_unlock(&priv->io_mutex);

	OUT_FUNCTION;
}

static void lineout_rm_recheck_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, dma_irq_handle_wqs[DELAY_LINEOUT_RECHECK].dw.work);
	int hkadc_value;

	if (WARN_ON(!priv))
		return;

	IN_FUNCTION;

	mutex_lock(&priv->plug_mutex);
	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		irqs_clr(priv, IRQ_MSK_COMP);
		irqs_mask_clr(priv, IRQ_MSK_PLUG_IN);
		mutex_unlock(&priv->plug_mutex);
		AUDIO_LOGI("plugout has happened,ignore this irq\n");
		return;
	}

	mutex_lock(&priv->hkadc_mutex);
	hkadc_value = read_hkadc_value(priv);
	mutex_unlock(&priv->hkadc_mutex);
	if (hkadc_value < 0) {
		AUDIO_LOGE("get adc fail,can't read adc value\n");
		mutex_unlock(&priv->plug_mutex);
		return;
	}

	if (hkadc_value <= priv->intervals[VOLTAGE_POLE4].max) {
		/* btn_up event */
		mutex_unlock(&priv->plug_mutex);
		return;
	}

	/* report plugout event,and set hs_status to lineout mode */
	if (priv->hs_status == PLUG_LINEOUT) {
		mutex_unlock(&priv->plug_mutex);
		AUDIO_LOGI("lineout recheck, hs_status is lineout, just return\n");
		return;
	}

	priv->hs_status = PLUG_NONE;
	priv->old_hs_status = PLUG_INVALID;
	/* report plugout event */
	hs_jack_report(priv);
	priv->hs_status = PLUG_LINEOUT;
	priv->pre_status_is_lineout = true;

	irqs_clr(priv, IRQ_MSK_COMP);
	irqs_mask_clr(priv, IRQ_MSK_BTN_NOR);
	AUDIO_LOGI("lineout recheck,and report remove\n");

	mutex_unlock(&priv->plug_mutex);
}

static void hs_lineout_rm_recheck(struct mbhc_data *priv)
{
	if (WARN_ON(!priv))
		return;

	cancel_delayed_work(&priv->dma_irq_handle_wqs[DELAY_LINEOUT_RECHECK].dw);
	flush_workqueue(priv->dma_irq_handle_wqs[DELAY_LINEOUT_RECHECK].dwq);

	__pm_wakeup_event(priv->wake_lock, LINEOUT_PO_RECHK_WAKE_LOCK_MS);
	mod_delayed_work(priv->dma_irq_handle_wqs[DELAY_LINEOUT_RECHECK].dwq,
			&priv->dma_irq_handle_wqs[DELAY_LINEOUT_RECHECK].dw,
			msecs_to_jiffies(LINEOUT_PO_RECHK_DELAY_MS));
}

static bool hs_lineout_plug_out(struct mbhc_data *priv)
{
	int hkadc_value;

	mutex_lock(&priv->plug_mutex);
	if (!irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
		irqs_clr(priv, IRQ_MSK_COMP);
		irqs_mask_clr(priv, IRQ_MSK_PLUG_IN);
		mutex_unlock(&priv->plug_mutex);
		AUDIO_LOGI("plugout has happened,ignore this irq\n");
		return true;
	}

	mutex_lock(&priv->hkadc_mutex);
	hkadc_value = read_hkadc_value(priv);
	mutex_unlock(&priv->hkadc_mutex);
	if (hkadc_value < 0) {
		AUDIO_LOGE("get adc fail,can't read adc value, %d\n", hkadc_value);
		mutex_unlock(&priv->plug_mutex);
		return false;
	}

	if (hkadc_value <= priv->intervals[VOLTAGE_POLE4].max) {
		/* btn_up event */
		mutex_unlock(&priv->plug_mutex);
		hs_lineout_rm_recheck(priv);
		return false;
	}

	/* report plugout event,and set hs_status to lineout mode */
	mutex_lock(&priv->io_mutex);
	priv->hs_jack.report = 0;
	if (priv->pressed_btn_type != 0) {
		/* report key event */
		AUDIO_LOGI("report type=0x%x, status=0x%x\n",
			priv->hs_jack.report, priv->hs_status);
		snd_soc_jack_report(&priv->hs_jack.jack,
			priv->hs_jack.report, JACK_RPT_MSK_BTN);
	}
	priv->pressed_btn_type = 0;
	mutex_unlock(&priv->io_mutex);

	priv->hs_status = PLUG_NONE;
	priv->old_hs_status = PLUG_INVALID;
	/* report plugout event */
	hs_jack_report(priv);
	priv->hs_status = PLUG_LINEOUT;
	priv->pre_status_is_lineout = true;

	irqs_clr(priv, IRQ_MSK_COMP);
	irqs_mask_clr(priv, IRQ_MSK_BTN_NOR);
	mutex_unlock(&priv->plug_mutex);
	hs_lineout_rm_recheck(priv);

	return true;
}

static void hs_pi_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_PLUG_IN].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

#ifdef CONFIG_HUAWEI_AUDIO
	if(check_usb_analog_hs_support()) {
		AUDIO_LOGI("skip hsd in irq\n");
		return;
	}
#endif
	hs_plug_in_detect(priv);
}

static void hs_po_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_PLUG_OUT].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

#ifdef CONFIG_HUAWEI_AUDIO
	if(check_usb_analog_hs_support()) {
		AUDIO_LOGI("skip hsd out irq\n");
		return;
	}
#endif
	hs_plug_out_detect(priv);
}

static void hs_comp_l_btn_down_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_COMP_L_BTN_DOWN].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

	hs_btn_down_detect(priv);
}

static void hs_comp_l_btn_up_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_COMP_L_BTN_UP].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

	hs_btn_up_detect(priv);
}

static void hs_comp_h_btn_down_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_COMP_H_BTN_DOWN].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

	if (hs_lineout_plug_out(priv)) {
		AUDIO_LOGI("hs plugout from lineout, return\n");
		return;
	}

	hs_btn_down_detect(priv);
}

static void hs_comp_h_btn_up_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_COMP_H_BTN_UP].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

	hs_btn_up_detect(priv);
}

static void hs_eco_btn_down_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_ECO_BTN_DOWN].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

	hs_btn_down_detect(priv);
}

static void hs_eco_btn_up_work(struct work_struct *work)
{
	struct mbhc_data *priv = container_of(work, struct mbhc_data, irq_wqs[IRQ_ECO_BTN_UP].dw.work);

	AUDIO_LOGI("enter\n");
	if (WARN_ON(!priv))
		return;

	hs_btn_up_detect(priv);
}

static void mbhc_reg_init(struct snd_soc_component *codec, struct mbhc_data *priv)
{
	if (priv->mbhc_ops.enable_hsd)
		priv->mbhc_ops.enable_hsd(codec);

	/* eliminate btn dithering */
	snd_soc_component_write_adapter(codec, DEB_CNT_HS_MIC_CFG_REG, 0x14);

	/* MBHC compare config 125mV 800mV 95% */
	snd_soc_component_write_adapter(codec, DA_SEPARATE_MBHD_VREF_CTRL, 0x9E);

	/* HSMICBIAS config voltage 2.7V */
	snd_soc_component_write_adapter(codec, DA_SEPARATE_HSMICB_CFG, 0x0B);

	/* clear HP MIXER channel select */
	snd_soc_component_write_adapter(codec, CODEC_ANA_RW20_REG, 0x0);

	/* config HP PGA gain to -20dB */
	snd_soc_component_write_adapter(codec, CODEC_ANA_RW21_REG, 0x0);

	/* Charge Pump clk pd, freq 768kHz */
	snd_soc_component_write_adapter(codec, DA_SEPARATE_CHARGE_PUMP_CLK_PD, 0x1A);

	/* disable ECO */
	snd_soc_component_update_bits(codec, CODEC_ANA_RW8_REG, BIT(MBHD_ECO_EN_OFFSET), 0);

	/* HSMICBIAS PD */
	snd_soc_component_update_bits(codec, CODEC_ANA_RW7_REG,
		BIT(HSMICB_PD_OFFSET), 0xff);

	/* avoid irq triggered while codec power up */
	snd_soc_component_update_bits(codec, ANA_IRQM_REG0_REG, IRQ_MSK_HS_ALL, 0xff);
	snd_soc_component_write_adapter(codec, ANA_IRQ_REG0_REG, IRQ_MSK_HS_ALL);
}

static void mbhc_config_print(const struct voltage_interval *intervals)
{
	unsigned int i;

	for (i = VOLTAGE_POLE3; i < VOLTAGE_NUM; i++)
		AUDIO_LOGI("headset voltage %s min: %d, max: %d\n", mbhc_print_str[i],
			intervals[i].min, intervals[i].max);
}

static void mbhc_config_init(const struct device_node *np,
	struct mbhc_data *priv)
{
	unsigned int i;
	unsigned int val = 0;

	priv->intervals[VOLTAGE_VOICE_ASSIST].min = -1;
	priv->intervals[VOLTAGE_VOICE_ASSIST].max = -1;
	for (i = VOLTAGE_POLE3; i < VOLTAGE_NUM; i++) {
		if (!of_property_read_u32(np, voltage_config[i].min, &val))
			priv->intervals[i].min = val;
		if (!of_property_read_u32(np, voltage_config[i].max, &val))
			priv->intervals[i].max = val;
	}

	mbhc_config_print(priv->intervals);

	priv->is_emulater = false;
	if (of_property_read_bool(np, "is_emulater"))
		priv->is_emulater = true;

	AUDIO_LOGI("emulater is %d\n", priv->is_emulater);
}

static const struct hs_handler_config irq_wq_cfgs[IRQ_ACTION_NUM] = {
	{ hs_po_work, "hs_po_dwq" },
	{ hs_pi_work, "hs_pi_dwq" },
	{ hs_eco_btn_down_work, "hs_eco_btn_down_dwq" },
	{ hs_eco_btn_up_work, "hs_eco_btn_up_dwq" },
	{ hs_comp_l_btn_down_work, "hs_comp_l_btn_down_dwq" },
	{ hs_comp_l_btn_up_work, "hs_comp_l_btn_up_dwq" },
	{ hs_comp_h_btn_down_work, "hs_comp_h_btn_down_dwq" },
	{ hs_comp_h_btn_up_work, "hs_comp_h_btn_up_dwq" },
};

static const struct hs_handler_config dma_irq_handle_wq_cfgs[DELAY_ACTION_NUM] = {
	{ hs_micbias_delay_pd_work, "hs_micbias_delay_pd_dwq" },
	{ lineout_rm_recheck_work, "lineout_po_recheck_dwq" },
};

static int mbhc_wq_init(struct mbhc_data *priv)
{
	unsigned int i;

	for (i = IRQ_PLUG_OUT; i < IRQ_ACTION_NUM; i++) {
		priv->irq_wqs[i].dwq = create_singlethread_workqueue(irq_wq_cfgs[i].name);
		if (priv->irq_wqs[i].dwq == NULL) {
			AUDIO_LOGE("%s workqueue create failed\n", irq_wq_cfgs[i].name);
			return -EFAULT;
		}
		INIT_DELAYED_WORK(&priv->irq_wqs[i].dw, irq_wq_cfgs[i].handler);
	}

	for (i = DELAY_MICBIAS_ENABLE; i < DELAY_ACTION_NUM; i++) {
		priv->dma_irq_handle_wqs[i].dwq = create_singlethread_workqueue(dma_irq_handle_wq_cfgs[i].name);
		if (priv->dma_irq_handle_wqs[i].dwq == NULL) {
			AUDIO_LOGE("%s workqueue create failed\n", dma_irq_handle_wq_cfgs[i].name);
			return -EFAULT;
		}
		INIT_DELAYED_WORK(&priv->dma_irq_handle_wqs[i].dw, dma_irq_handle_wq_cfgs[i].handler);
	}

	return 0;
}

static void mbhc_wq_deinit(struct mbhc_data *priv)
{
	unsigned int i;

	for (i = IRQ_PLUG_OUT; i < IRQ_ACTION_NUM; i++) {
		if (priv->irq_wqs[i].dwq != NULL) {
			cancel_delayed_work(&priv->irq_wqs[i].dw);
			flush_workqueue(priv->irq_wqs[i].dwq);
			destroy_workqueue(priv->irq_wqs[i].dwq);
			priv->irq_wqs[i].dwq = NULL;
		}
	}

	for (i = DELAY_MICBIAS_ENABLE; i < DELAY_ACTION_NUM; i++) {
		if (priv->dma_irq_handle_wqs[i].dwq != NULL) {
			cancel_delayed_work(&priv->dma_irq_handle_wqs[i].dw);
			flush_workqueue(priv->dma_irq_handle_wqs[i].dwq);
			destroy_workqueue(priv->dma_irq_handle_wqs[i].dwq);
			priv->dma_irq_handle_wqs[i].dwq = NULL;
		}
	}
}

static int register_hs_jack_btn(struct mbhc_data *priv)
{
	/* Headset jack */
	int ret = snd_soc_card_jack_new(priv->codec->card, "Headset Jack",
		SND_JACK_HEADSET, (&priv->hs_jack.jack), NULL, 0);
	if (ret) {
		AUDIO_LOGE("jack new error, ret = %d\n", ret);
		return ret;
	}

	/* set a key mapping on a jack */
	ret = snd_jack_set_key(priv->hs_jack.jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	if (ret) {
		AUDIO_LOGE("jack set key(0x%x) error, ret = %d\n", SND_JACK_BTN_0, ret);
		return ret;
	}

	ret = snd_jack_set_key(priv->hs_jack.jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	if (ret) {
		AUDIO_LOGE("jack set key(0x%x) error, ret = %d\n", SND_JACK_BTN_1, ret);
		return ret;
	}

	ret = snd_jack_set_key(priv->hs_jack.jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	if (ret) {
		AUDIO_LOGE("jack set key(0x%x) error, ret = %d\n", SND_JACK_BTN_2, ret);
		return ret;
	}

	ret = snd_jack_set_key(priv->hs_jack.jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);
	if (ret) {
		AUDIO_LOGE("jack set key(0x%x) error, ret = %d\n", SND_JACK_BTN_3, ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_HUAWEI_AUDIO
static void plug_in_detect(void *priv)
{
	struct mbhc_data *private_data = (struct mbhc_data *)priv;

	hs_plug_in_detect(private_data);
}

static void plug_out_detect(void *priv)
{
	struct mbhc_data *private_data = (struct mbhc_data *)priv;

	hs_plug_out_detect(private_data);
}

static void hs_high_resistence_enable(void *priv, bool enable)
{
	UNUSED_PARAMETER(priv);
	if (enable)
		AUDIO_LOGI("change hs resistance high to reduce pop noise\n");
	else
		AUDIO_LOGI("reset hs resistance to default\n");
}

static struct ana_hs_codec_dev usb_analog_dev = {
	.name = "usb_analog_hs",
	.ops = {
		.check_headset_in = NULL,
		.plug_in_detect = plug_in_detect,
		.plug_out_detect = plug_out_detect,
		.get_headset_type = NULL,
		.hs_high_resistence_enable = hs_high_resistence_enable,
	},
};
#endif

static int mbhc_lock_init(struct mbhc_data *priv)
{
	mutex_init(&priv->io_mutex);
	mutex_init(&priv->hs_micbias_mutex);
	mutex_init(&priv->hkadc_mutex);
	mutex_init(&priv->plug_mutex);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	priv->wake_lock = wakeup_source_register(NULL, "da_separate_mbhc");
#else
	priv->wake_lock = wakeup_source_register("da_separate_mbhc");
#endif
	if (priv->wake_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		return -ENOMEM;
	}

	return 0;
}

static void mbhc_lock_deinit(struct mbhc_data *priv)
{
	mutex_destroy(&priv->io_mutex);
	mutex_destroy(&priv->hs_micbias_mutex);
	mutex_destroy(&priv->hkadc_mutex);
	mutex_destroy(&priv->plug_mutex);
	wakeup_source_unregister(priv->wake_lock);
}

static int mbhc_priv_init(struct snd_soc_component *codec, struct mbhc_data *priv,
						const struct da_separate_mbhc_ops *ops)
{
	int ret;

	priv->codec = codec;
	ret = mbhc_lock_init(priv);
	if (ret != 0)
		return ret;

	mbhc_config_init(codec->dev->of_node, priv);

	priv->hs_status = PLUG_NONE;
	priv->old_hs_status = PLUG_INVALID;
	priv->hs_jack.report = 0;
	priv->pressed_btn_type = 0;
	priv->pre_status_is_lineout = false;
	priv->hs_micbias_mbhc = false;
	priv->hs_micbias_dapm = 0;

	memcpy(&priv->mbhc_ops, ops, sizeof(priv->mbhc_ops)); /* unsafe_function_ignore: memset memcpy */
	return 0;
}

static void mbhc_ana_usb_cfg_get(struct mbhc_data *priv, struct device_node *np)
{
	unsigned int val = 0;

	priv->usb_ana_need_recheck = false;
	if (of_property_read_u32(np, "usb_ana_need_recheck", &val))
		return;
	if (val != 0)
		priv->usb_ana_need_recheck = true;
}

static int mbhc_gpio_irq_init(struct mbhc_data *priv, struct device_node *np)
{
	int ret;

	if (priv->is_emulater) {
		AUDIO_LOGI("emulater no need gpio\n");
		return 0;
	}

	priv->gpio_intr_pin = of_get_named_gpio(np, "gpios", 0);
	if (!gpio_is_valid(priv->gpio_intr_pin)) {
		AUDIO_LOGE("gpio_intr_pin gpio:%d is invalied\n", priv->gpio_intr_pin);
		ret = -EINVAL;
		goto gpio_pin_err;
	}

	/* this gpio is shared by pmu and acore, and by quested in pmu, so need not requet here */
	priv->gpio_irq = gpio_to_irq(priv->gpio_intr_pin);
	if (priv->gpio_irq < 0) {
		AUDIO_LOGE("gpio_to_irq err, gpio_irq=%d, gpio_intr_pin=%d\n",
			priv->gpio_irq, priv->gpio_intr_pin);
		ret = -EINVAL;
		goto gpio_to_irq_err;
	}
	AUDIO_LOGI("gpio_to_irq succ, gpio_irq=%d, gpio_intr_pin=%d\n",
		priv->gpio_irq, priv->gpio_intr_pin);

	/* irq shared with pmu */
	ret = request_irq(priv->gpio_irq, irq_handler,
		IRQF_TRIGGER_LOW | IRQF_SHARED | IRQF_NO_SUSPEND, "codec_irq", priv);
	if (ret) {
		AUDIO_LOGE("request_irq failed, ret = %d\n", ret);
		goto gpio_to_irq_err;
	}

	return ret;

gpio_to_irq_err:
	priv->gpio_irq = DA_SEPARATE_INVALID_IRQ;
gpio_pin_err:
	priv->gpio_intr_pin = ARCH_NR_GPIOS;

	return ret;
}

static int mbhc_switch_init(struct mbhc_data *priv)
{
#ifdef CONFIG_SWITCH
	int ret;

	priv->hs_jack.sdev.name = "h2w";
	ret = switch_dev_register(&(priv->hs_jack.sdev));
	if (ret) {
		AUDIO_LOGE("Registering switch device error, ret=%d\n", ret);
		return ret;
	}
	priv->hs_jack.is_dev_registered = true;
#endif

	return 0;
}

static void mbhc_switch_deinit(struct mbhc_data *priv)
{
#ifdef CONFIG_SWITCH
	if (priv->hs_jack.is_dev_registered) {
		switch_dev_unregister(&(priv->hs_jack.sdev));
		priv->hs_jack.is_dev_registered = false;
	}
#endif
}

static void mbhc_detect_initial_state(struct mbhc_data *priv)
{
	/* detect headset present or not */
	AUDIO_LOGI("irq soure stat %#04x",
		snd_soc_component_read32(priv->codec, ANA_IRQ_SIG_STAT_REG));

	if (irq_status_check(priv, IRQ_STAT_PLUG_IN)) {
#ifdef CONFIG_HUAWEI_AUDIO
		if (check_usb_analog_hs_support())
			usb_analog_hs_plug_in_out_handle(ANA_HS_PLUG_IN);
		else if (ana_hs_support_usb_sw())
			ana_hs_plug_handle(ANA_HS_PLUG_IN);
		else
#endif
			hs_plug_in_detect(priv);
	} else {
		irqs_mask_clr(priv, IRQ_MSK_PLUG_IN);
	}
}

int da_separate_mbhc_init(struct snd_soc_component *codec, struct da_separate_mbhc **mbhc,
						const struct da_separate_mbhc_ops *ops)
{
	int ret;
	struct mbhc_data *priv = NULL;

	AUDIO_LOGI("Begin\n");

	if (codec == NULL || mbhc == NULL || ops == NULL) {
		AUDIO_LOGE("invalid input parameters\n");
		return -EINVAL;
	}

	priv = devm_kzalloc(codec->dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		AUDIO_LOGE("priv devm_kzalloc failed\n");
		return -ENOMEM;
	}

	priv->codec = codec;

	ret = mbhc_priv_init(codec, priv, ops);
	if (ret != 0)
		goto mbhc_priv_init_err;

	mbhc_reg_init(codec, priv);

#ifdef CONFIG_HUAWEI_AUDIO
	ret = usb_analog_hs_dev_register(&usb_analog_dev, priv);
	if (ret)
		AUDIO_LOGE("Not support usb analog headset\n");

	ret = ana_hs_codec_dev_register(&usb_analog_dev, priv);
	if (ret)
		AUDIO_LOGE("ana hs dev register fail, ignore\n");
#endif

	ret = register_hs_jack_btn(priv);
	if (ret)
		goto jack_err;

	ret = mbhc_switch_init(priv);
	if (ret)
		goto jack_err;

#if defined(CONFIG_HUAWEI_HEADSET_DEBUG) && defined(CONFIG_HUAWEI_AUDIO)
	headset_debug_init(&priv->hs_jack.jack, &priv->hs_jack.sdev);
#endif

	ret = mbhc_wq_init(priv);
	if (ret)
		goto wq_init_err;

	ret = mbhc_gpio_irq_init(priv, codec->dev->of_node);
	if (ret)
		goto wq_init_err;

	mbhc_ana_usb_cfg_get(priv, codec->dev->of_node);
	AUDIO_LOGI("priv->usb_ana_need_recheck %d\n", priv->usb_ana_need_recheck);

	mbhc_detect_initial_state(priv);
#ifdef CONFIG_HUAWEI_AUDIO
	headset_auto_calib_init(codec->dev->of_node);
#endif
	da_separate_mbhc_custom_init(codec->dev->of_node);

	*mbhc = &priv->mbhc_pub;

	goto end;

wq_init_err:
	mbhc_wq_deinit(priv);
	mbhc_switch_deinit(priv);

jack_err:
mbhc_priv_init_err:
	mbhc_lock_deinit(priv);
end:
	AUDIO_LOGI("End\n");
	return ret;
}

void da_separate_mbhc_deinit(struct da_separate_mbhc *mbhc)
{
	struct mbhc_data *priv = (struct mbhc_data *)mbhc;

	IN_FUNCTION;

	if (priv == NULL) {
		AUDIO_LOGE("priv is NULL\n");
		return;
	}

	mbhc_lock_deinit(priv);

#if defined(CONFIG_HUAWEI_HEADSET_DEBUG) && defined(CONFIG_HUAWEI_AUDIO)
	headset_debug_uninit();
#endif

	mbhc_switch_deinit(priv);
	mbhc_wq_deinit(priv);

	if (priv->gpio_irq >= 0) {
		free_irq(priv->gpio_irq, priv);
		priv->gpio_irq = DA_SEPARATE_INVALID_IRQ;
	}

	OUT_FUNCTION;
}

MODULE_DESCRIPTION("da_separate_mbhc");
MODULE_LICENSE("GPL");

