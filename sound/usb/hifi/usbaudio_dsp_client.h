/*
 * usbaudio_dsp_client.h
 *
 * usbaudio dsp client
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2021. All rights reserved.
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

#ifndef __USBAUDIO_DSP_CLIENT__
#define __USBAUDIO_DSP_CLIENT__

#include <linux/string.h>
#include "../usbaudio_mailbox/usbaudio_mailbox.h"
#include <sound/pcm_params.h>
#include "usbaudio.h"

bool controller_switch(struct usb_device *dev, u32 usb_id,
	struct usb_host_interface *ctrl_intf, int ctrlif, struct usbaudio_pcms *pcms);
int disconnect(struct snd_usb_audio *chip, unsigned int dsp_reset_flag);
int set_pipeout_interface(struct snd_usb_audio *chip,
	struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate);
int set_pipein_interface(struct snd_usb_audio *chip,
	struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate);
bool controller_query(struct snd_usb_audio *chip);
bool send_usbaudioinfo2hifi(struct snd_usb_audio *chip, struct usbaudio_pcms *pcms);
void setinterface_complete(unsigned int dir, unsigned int running, int ret, unsigned int rate);
void usbaudio_nv_check_complete(unsigned int usb_id);
int usb_power_resume(void);

#endif /* __USBAUDIO_DSP_CLIENT__ */
