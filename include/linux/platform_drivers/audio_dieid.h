/*
 * audio_dieid.h -- audio dieid API define
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 */

#ifndef __AUDIO_DIEID_H__
#define __AUDIO_DIEID_H__

#if defined (CONFIG_DA_COMBINE_V3_CODEC) || defined(CONFIG_SND_SOC_DA_COMBINE_V5)
#include <linux/platform_drivers/da_combine/da_combine_utils.h>
#endif

#ifdef CONFIG_PLATFORM_DIEID
static inline int codec_get_dieid(char *dieid, unsigned int len)
{
#if defined (CONFIG_DA_COMBINE_V3_CODEC) || defined(CONFIG_SND_SOC_DA_COMBINE_V5)
	return da_combine_codec_get_dieid(dieid, len);
#endif
	return -1;
}
#endif

#endif