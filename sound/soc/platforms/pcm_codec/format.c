/*
 * format.c
 *
 * format converter for pcm codec
 *
 * Copyright (c) 2019 Huawei Technologies Co., Ltd.
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

#include "format.h"

#include "audio_log.h"
#include "platform_io.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "pcm_codec"
#else
#define LOG_TAG "audio_pcm"
#endif

#define MAX_32 ((int)0x7fffffff)
#define MIN_32 ((int)0x80000000)
#define BYTE_BITS 8
#define SHORT_BITS 16
#define S24_BITS 24
#define INT_BITS 32

/** convert 16bit or 24bit(data format of ap) to 32bit(data format of dma),
 *  we need a convert buffer that its size is 2 mutiple of ap buffer
 */
#define FORMAT_CONVERT_NUMTIPLES 2

static char *trans_buffer;

static int L_shr(int var1, short var2);

/**
 *L_shl - Arithmetically shift the 32 bit input L_var1 left var2 positions
 *
 *@var1: 32 bit long signed integer (Word32) whose value falls in the
 *          range : 0x8000 0000 <= var1 <= 0x7fff ffff.
 *@var2: 16 bit short signed integer (Word16) whose value falls in the
 *          range : 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *ZERO fill the var2 LSB of the result. If var2 is negative, arithmetically
 *shift L_var1 right by -var2 with sign extension. Saturate the result in
 *case of underflows or overflows.
 *
 *Return:32 bit long signed integer (Word32) whose value falls in the
 *       range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
 */
static int L_shl(int var1, short var2)
{
	int L_var_out;

	if (var2 <= 0) {
		if (var2 < -32)
			var2 = -32;
		L_var_out = L_shr(var1, -var2);
	} else {
		for (; var2 > 0; var2--) {
			if (var1 > (int)0X3fffffff) {
				L_var_out = MAX_32;
				break;
			}
			if (var1 < (int)0xc0000000) {
				L_var_out = MIN_32;
				break;
			}

			var1 *= 2;
			L_var_out = var1;
		}
	}

	return L_var_out;
}

/**
 *L_shr - Arithmetically shift the 32 bit input L_var1 right var2 positions with sign extension
 *
 *@var1: 32 bit long signed integer (Word32) whose value falls in the
 *          range : 0x8000 0000 <= var1 <= 0x7fff ffff.
 *@var2: 16 bit short signed integer (Word16) whose value falls in the
 *          range : 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *If var2 is negative, arithmetically shift L_var1 left by -var2 and zero fill the -var2
 *LSB of the result. Saturate the result in case of underflows or overflows
 *
 *Return:32 bit long signed integer (Word32) whose value falls in the
 *       range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
 */

static int L_shr(int var1, short var2)
{
	int L_var_out;

	if (var2 < 0) {
		if (var2 < -32)
			var2 = -32;
		L_var_out = L_shl(var1, -var2);
	} else {
		if (var2 >= 31) {
			L_var_out = (var1 < 0L) ? -1 : 0;
		} else {
			if (var1 < 0)
				L_var_out = ~((~var1) >> var2); /*lint !e502 */
			else
				L_var_out = var1 >> var2;
		}
	}

	return L_var_out;
}

uint32_t audio_bytes_per_sample(enum pcm_format format)
{
	uint32_t size = 0;

	switch (format) {
	case PCM_FORMAT_S16_LE:
		size = sizeof(int16_t);
		break;
	case PCM_FORMAT_S24_3LE:
		/* 3byte */
		size = sizeof(int8_t) * 3;
		break;
	case PCM_FORMAT_S24_LE_LA:
	case PCM_FORMAT_S24_LE_RA:
	case PCM_FORMAT_S32_LE:
		size = sizeof(int32_t);
		break;
	default:
		AUDIO_LOGE("invalid format %d", format);
		break;
	}

	return size;
}

static void memcpy_to_16p_from_24la(int16_t *dst, const int32_t *src,
	uint32_t sample_cnt)
{
	while (sample_cnt--)
		*dst++ = L_shr(*src++, SHORT_BITS);
}

static void memcpy_to_16p_from_24ra(int16_t *dst, const int32_t *src,
	size_t sample_cnt)
{
	while (sample_cnt--)
		*dst++ = *src++ >> BYTE_BITS;
}

static void memcpy_to_16p_from_24p(int16_t *dst, const uint8_t *src,
	size_t sample_cnt)
{
	while (sample_cnt--) {
		/* config MSB/LSB */
		*dst++ = src[1] | (src[2] << BYTE_BITS);
		/* 3byte */
		src += 3;
	}
}

static void memcpy_to_24la_from_16p(int32_t *dst, const int16_t *src,
	uint32_t sample_cnt)
{
	while (sample_cnt--)
		*dst++ = L_shl(*src++, SHORT_BITS);
}

static void memcpy_to_24la_from_24ra(int32_t *dst, const int32_t *src,
	uint32_t sample_cnt)
{
	while (sample_cnt--)
		*dst++ = L_shl(*src++, BYTE_BITS);
}

static void memcpy_to_24ra_from_24la(int32_t *dst, const int32_t *src,
	uint32_t sample_cnt)
{
	while (sample_cnt--)
		*dst++ = L_shr(*src++, BYTE_BITS);
}

static void memcpy_to_24la_from_24p(int32_t *dst, const uint8_t *src,
	uint32_t sample_cnt)
{
	while (sample_cnt--) {
		/* config MSB/LSB */
		*dst++ = (src[0] << BYTE_BITS) | (src[1] << SHORT_BITS) | (src[2] << S24_BITS);
		src += 3;
	}
}

static void memcpy_to_24p_from_24la(uint8_t *dst, const int32_t *src,
	uint32_t sample_cnt)
{
	int32_t ival;

	while (sample_cnt--) {
		ival = *src++ >> BYTE_BITS;
		*dst++ = ival;
		*dst++ = ival >> BYTE_BITS;
		*dst++ = ival >> SHORT_BITS;
	}
}

static bool is_format_valid(enum pcm_format format)
{
	if (format == PCM_FORMAT_S16_LE ||
		format == PCM_FORMAT_S24_LE_LA ||
		format == PCM_FORMAT_S24_LE_RA ||
		format == PCM_FORMAT_S32_LE ||
		format == PCM_FORMAT_S24_3LE)
		return true;
	return false;
}

static bool is_same_format(enum pcm_format dst_format,
	enum pcm_format src_format)
{
	if ((dst_format == src_format && is_format_valid(dst_format)) ||
		(dst_format == PCM_FORMAT_S32_LE &&
		src_format == PCM_FORMAT_S24_LE_LA) ||
		(dst_format == PCM_FORMAT_S24_LE_LA &&
		src_format == PCM_FORMAT_S32_LE)) {
		return true;
	}
	return false;
}

static void memcpy_by_pcm_format(
	void *dst, enum pcm_format dst_format,
	const void *src, enum pcm_format src_format,
	uint32_t sample_cnt)
{
	if (is_same_format(dst_format, src_format)) {
		memcpy(dst, src, sample_cnt * audio_bytes_per_sample(src_format));
		return;
	}

	if (dst_format == PCM_FORMAT_S16_LE) {
		if (src_format == PCM_FORMAT_S24_LE_LA ||
			src_format == PCM_FORMAT_S32_LE)
			memcpy_to_16p_from_24la(dst, src, sample_cnt);
		else if (src_format == PCM_FORMAT_S24_LE_RA)
			memcpy_to_16p_from_24ra(dst, src, sample_cnt);
		else if (src_format == PCM_FORMAT_S24_3LE)
			memcpy_to_16p_from_24p(dst, src, sample_cnt);
	} else if (dst_format == PCM_FORMAT_S24_LE_LA) {
		if (src_format == PCM_FORMAT_S16_LE)
			memcpy_to_24la_from_16p(dst, src, sample_cnt);
		else if (src_format == PCM_FORMAT_S24_LE_RA)
			memcpy_to_24la_from_24ra(dst, src, sample_cnt);
		else if (src_format == PCM_FORMAT_S24_3LE)
			memcpy_to_24la_from_24p(dst, src, sample_cnt);
	} else if (dst_format == PCM_FORMAT_S24_LE_RA) {
		if (src_format == PCM_FORMAT_S24_LE_LA)
			memcpy_to_24ra_from_24la(dst, src, sample_cnt);
	} else if (dst_format == PCM_FORMAT_S24_3LE) {
		if (src_format == PCM_FORMAT_S24_LE_LA)
			memcpy_to_24p_from_24la(dst, src, sample_cnt);
	} else {
		AUDIO_LOGE("invalid src format: %d, dst format: %d",
		src_format, dst_format);
	}
}

static void interlace(int8_t *dst, const int8_t *src,
	uint32_t period_size, uint32_t channels, uint32_t bytes)
{
	uint32_t i, j, k;
	uint32_t cnt = period_size;

	for (i = 0; i < cnt; i++) {
		for (j = 0; j < channels; j++) {
			for (k = 0; k < bytes; k++)
				dst[bytes * (i * channels + j) + k] =
					src[bytes * (i + cnt * j) + k];
		}
	}
}

static void deinterlace(int8_t *dst, const int8_t *src,
	uint32_t period_size, uint32_t channels, uint32_t bytes)
{
	uint32_t i, j, k;
	uint32_t cnt = period_size;

	for (i = 0; i < cnt; i++) {
		for (j = 0; j < channels; j++) {
			for (k = 0; k < bytes; k++)
				dst[bytes * (i + cnt * j) + k] =
					src[bytes * (i * channels + j) + k];
		}
	}
}

static void memcpy_by_data_format(struct data_format_conversion_cfg *dst_cfg,
	struct data_format_conversion_cfg *src_cfg,
	uint32_t period_size, uint32_t channels,
	uint32_t merge_interlace_channel)
{
	if (dst_cfg->data_format == src_cfg->data_format) {
		memcpy(dst_cfg->addr, src_cfg->addr,
			period_size * channels * audio_bytes_per_sample(src_cfg->pcm_format));
		return;
	}

	switch (dst_cfg->data_format) {
	case DATA_FORMAT_INTERLACED:
		interlace(dst_cfg->addr, src_cfg->addr, period_size,
			channels / merge_interlace_channel,
			audio_bytes_per_sample(src_cfg->pcm_format) * merge_interlace_channel);
		return;
	case DATA_FORMAT_NONE_INTERLACED:
		deinterlace(dst_cfg->addr, src_cfg->addr, period_size,
			channels / merge_interlace_channel,
			audio_bytes_per_sample(src_cfg->pcm_format) * merge_interlace_channel);
		return;
	default:
		break;
	}

	AUDIO_LOGE("invalid src data format: %d, dst data format: %d",
		src_cfg->data_format, dst_cfg->data_format);
}

unsigned int get_bytes_per_sample(snd_pcm_format_t format)
{
	return snd_pcm_format_width(format) / BYTE_BITS;
}

static int check_convert_format_params(struct data_convert_info *info)
{
	if (info == NULL || info->dest_cfg.addr == NULL || info->src_cfg.addr == NULL) {
		AUDIO_LOGE("convert info is NULL");
		return -EINVAL;
	}

	if (info->merge_interlace_channel == 0 ||
		info->merge_interlace_channel > info->channels) {
		AUDIO_LOGE("invalid merge interlace channel:%d",
			info->merge_interlace_channel);
		return -EINVAL;
	}

	return 0;
}

void convert_format(struct data_convert_info *info)
{
	void *dest_addr = NULL;
	int ret;

	if (info == NULL) {
		AUDIO_LOGE("info is NULL");
		return;
	}
	if (trans_buffer == NULL) {
		AUDIO_LOGE("trans buffer is NULL");
		return;
	}

	ret = check_convert_format_params(info);
	if (ret != 0) {
		AUDIO_LOGE("convert format params is invalid");
		return;
	}

	dest_addr = info->dest_cfg.addr;
	info->dest_cfg.addr = trans_buffer;
	memcpy_by_data_format(&info->dest_cfg, &info->src_cfg, info->period_size,
		info->channels, info->merge_interlace_channel);

	info->dest_cfg.addr = dest_addr;
	memcpy_by_pcm_format(dest_addr, info->dest_cfg.pcm_format,
			trans_buffer, info->src_cfg.pcm_format,
			info->period_size * info->channels);
}

int pcm_format_init(struct device *dev, unsigned int bytes)
{
	if (dev == NULL) {
		AUDIO_LOGE("device is NULL");
		return -EINVAL;
	}

	if (bytes == 0) {
		AUDIO_LOGE("get hardware buffer max bytes is 0");
		return -EINVAL;
	}

	bytes = bytes * FORMAT_CONVERT_NUMTIPLES;

	trans_buffer = devm_kzalloc(dev, bytes, GFP_KERNEL);
	if (trans_buffer == NULL) {
		AUDIO_LOGE("malloc memory fail, request size is %u", bytes);
		return -ENOMEM;
	}

	return 0;
}

void pcm_format_deinit(struct device *dev)
{
	if (trans_buffer == NULL) {
		AUDIO_LOGE("buff is NULL, no need free");
		return;
	}

	devm_kfree(dev, trans_buffer);
	trans_buffer = NULL;
}

