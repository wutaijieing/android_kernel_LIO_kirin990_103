/*
 * usbaudio_dtsi_property.c
 *
 * usbaudio dtsi property
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
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

#ifndef __USBAUDIO_DTSI_PROPERTY_H__
#define __USBAUDIO_DTSI_PROPERTY_H__

#ifdef CONFIG_USB_AUDIO_DTSI_PROPERTY
bool get_usbaudio_need_auto_suspend(void);
#else
static inline bool get_usbaudio_need_auto_suspend(void)
{
	return false;
}
#endif /* CONFIG_USB_AUDIO_DTSI_PROPERTY */

#endif /* __USBAUDIO_DTSI_PROPERTY_H */

