/*
 * vcodec_vdec_dbg.c
 *
 * This is for vdec debug
 *
 * Copyright (c) 2020-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "vcodec_vdec_dbg.h"
#include "securec.h"
#include "dbg.h"
#include <linux/kernel.h>
#include <stddef.h>

#define VDEC_MIN_RESERVED_PHYADDR 0x80000000
#define VDEC_MAX_RESERVED_PHYADDR 0xA0000000
#define VDEC_MAX_ION_ALLOC_TOTAL_SIZE 0x80000000
const struct vdec_setting_option setting_options[] = {
	{ "enable_power_off_when_vdec_idle", "enable powering off when vdec is idle",
		offsetof(struct vdec_setting_info, enable_power_off_when_vdec_idle),
		SETTING_OPT_TYPE_BOOL, { .enable = true}, 0, 1},

	{ "smmu_glb_bypass", "smmu glb bypass",
		offsetof(struct vdec_setting_info, smmu_glb_bypass),
		SETTING_OPT_TYPE_BOOL, {.enable = false}, 0, 1},

	{ "vdec_reserved_phyaddr", "vdec reserved memory",
		offsetof(struct vdec_setting_info, vdec_reserved_phyaddr),
		SETTING_OPT_TYPE_UINT64, {.i64 = VDEC_MIN_RESERVED_PHYADDR}, VDEC_MIN_RESERVED_PHYADDR, VDEC_MAX_RESERVED_PHYADDR},

	{ "ion_alloc_total_size", "ion alloc total size",
		offsetof(struct vdec_setting_info, ion_alloc_total_size),
		SETTING_OPT_TYPE_UINT64, {.i64 = 0}, 0, VDEC_MAX_ION_ALLOC_TOTAL_SIZE},
};

void vdec_init_setting_info(const struct vdec_setting_info *info)
{
	size_t i;
	void *dst = NULL;

	for (i = 0; i < ARRAY_SIZE(setting_options); i++) {
		const struct vdec_setting_option *option = &setting_options[i];
		dst = ((unsigned char *)info) + option->offset;
		switch (option->type) {
		case SETTING_OPT_TYPE_BOOL:
			*(bool *)dst = option->default_value.enable;
			break;
		case SETTING_OPT_TYPE_INT:
			*(int *)dst = option->default_value.i32;
			break;
		case SETTING_OPT_TYPE_UINT:
			*(unsigned int *)dst = option->default_value.i32;
			break;
		case SETTING_OPT_TYPE_INT64:
			*(long long *)dst = option->default_value.i64;
			break;
		case SETTING_OPT_TYPE_UINT64:
			*(unsigned long long *)dst = option->default_value.i64;
			break;
		case SETTING_OPT_TYPE_STRING: {
				int ret = strcpy_s((unsigned char *)dst, MAX_SETTING_STRING_LEN, option->default_value.str);
				if (ret != 0)
					dprint(PRN_ERROR, "Failed to init string setting value. ret:%d.", ret);
		}
			break;
		default:
			break;
		}
	}
}

#ifdef ENABLE_VDEC_PROC

static const struct vdec_setting_option_type_to_string  {
	const enum vdec_setting_option_type type;
	const char *str;
} vdec_setting_option_type_to_string_table[] = {
	{ SETTING_OPT_TYPE_BOOL,   "<bool>"   },
	{ SETTING_OPT_TYPE_INT,    "<int>"    },
	{ SETTING_OPT_TYPE_UINT,   "<uint>"   },
	{ SETTING_OPT_TYPE_INT64,  "<int64>"  },
	{ SETTING_OPT_TYPE_UINT64, "<uint64>" },
	{ SETTING_OPT_TYPE_STRING, "<string>" },
};

static ssize_t vdec_fill_setting_option(const struct vdec_setting_option *option, char *buf, ssize_t len)
{
	ssize_t ret;

	switch (option->type) {
	case SETTING_OPT_TYPE_BOOL:
		ret = sprintf_s(buf, len, " (default %s)\n",
			option->default_value.enable ? "true" : "false");
		break;
	case SETTING_OPT_TYPE_INT:
		ret = sprintf_s(buf, len, " (from %lld to %lld) (default %d)\n",
			option->min_value, option->min_value, option->default_value.i32);
		break;
	case SETTING_OPT_TYPE_UINT:
		ret = sprintf_s(buf, len, " (from %llu to %llu) (default %u)\n",
			option->min_value, option->min_value, option->default_value.i32);
		break;
	case SETTING_OPT_TYPE_INT64:
		ret = sprintf_s(buf, len, " (from %lld to %lld) (default %lld)\n",
			option->min_value, option->min_value, option->default_value.i64);
		break;
	case SETTING_OPT_TYPE_UINT64:
		ret = sprintf_s(buf, len, " (from %llu to %llu) (default %llu)\n",
			option->min_value, option->min_value, option->default_value.i64);
		break;
	case SETTING_OPT_TYPE_STRING:
		ret = sprintf_s(buf, len, " (default %s)\n",
			option->default_value.str);
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static ssize_t vdec_show_setting_option(char *buf, ssize_t start_pos, ssize_t buf_capacity)
{
	ssize_t filled_len = start_pos;
	ssize_t ret;
	size_t i;
	size_t j;

	ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "Setting option:\n");
	if (ret < 0)
		return filled_len;

	filled_len += ret;

	for (i = 0; i < ARRAY_SIZE(setting_options); i++) {
		const struct vdec_setting_option *option = &setting_options[i];
		ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "  %-40s", option->name);
		if (ret < 0)
			return filled_len;

		filled_len += ret;

		for (j = 0; j < ARRAY_SIZE(vdec_setting_option_type_to_string_table); j++) {
			const struct vdec_setting_option_type_to_string *map_info = &vdec_setting_option_type_to_string_table[j];
			if (option->type == map_info->type) {
				ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%-12s %s", map_info->str, option->help);
				if (ret < 0)
					return filled_len;

				filled_len += ret;
				break;
			}
		}
		ret = vdec_fill_setting_option(option, buf + filled_len, buf_capacity - filled_len);
		if (ret < 0)
			return filled_len;

		filled_len += ret;
	}

	return filled_len;
}

static ssize_t vdec_show_setting_usage(char *buf, ssize_t start_pos, ssize_t buf_capacity)
{
	ssize_t ret;
	ssize_t filled_len = start_pos;

	ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "\n\nUsage:\n");
	if (ret < 0)
		return filled_len;

	filled_len += ret;

	ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "  echo key1=value1:key2=value2[: ...] > vdec_setting\n");
	if (ret < 0)
		return filled_len;

	filled_len += ret;

	return filled_len;
}

static ssize_t vdec_show_setting_value(char *buf, ssize_t start_pos, ssize_t buf_capacity,
	const struct vdec_setting_info *setting_info)
{
	size_t i;
	ssize_t ret;
	ssize_t filled_len = start_pos;

	if (!setting_info)
		return filled_len;

	ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "\n\nThe actual option:\n");
	if (ret < 0)
		return filled_len;

	filled_len += ret;

	for (i = 0; i < ARRAY_SIZE(setting_options); i++) {
		const struct vdec_setting_option *option = &setting_options[i];
		char *target_addr = (char *)setting_info + option->offset;
		ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "  %s=", option->name);
		if (ret < 0)
			return filled_len;

		filled_len += ret;

		switch (option->type) {
		case SETTING_OPT_TYPE_BOOL:
			ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%d\n", *(bool *)(target_addr));
			break;
		case SETTING_OPT_TYPE_INT:
			ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%d\n", *(int *)(target_addr));
			break;
		case SETTING_OPT_TYPE_UINT:
			ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%u\n", *(unsigned int *)(target_addr));
			break;
		case SETTING_OPT_TYPE_INT64:
			ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%lld\n", *(long long *)(target_addr));
			break;
		case SETTING_OPT_TYPE_UINT64:
			ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%llu\n", *(unsigned long long *)(target_addr));
			break;
		case SETTING_OPT_TYPE_STRING:
			ret = sprintf_s(buf + filled_len, buf_capacity - filled_len, "%s\n", target_addr);
			break;
		default:
			ret = 0;
			break;
		}

		if (ret < 0)
			return filled_len;

		filled_len += ret;
	}

	return filled_len;
}

ssize_t vdec_show_setting_info(char *buf, const size_t count, const struct vdec_setting_info *setting_info)
{
	ssize_t filled_len = 0;
	if (!buf || !setting_info)
		return 0;

	filled_len = vdec_show_setting_option(buf, filled_len, count);

	filled_len = vdec_show_setting_usage(buf, filled_len, count);

	filled_len = vdec_show_setting_value(buf, filled_len, count, setting_info);

	return filled_len;
}

static int write_number(void *dst, const struct vdec_setting_option *option,
	long long value)
{
	if (value > option->max_value || value < option->min_value) {
		dprint(PRN_ERROR, "Invalid value(%lld) is out of range [%lld, %lld].", value, option->min_value, option->max_value);
		return -1;
	}

	dprint(PRN_ALWS, "set '%s' is %lld", option->help, value);
	switch (option->type) {
	case SETTING_OPT_TYPE_BOOL:
		*(bool *)dst = (bool)value;
		break;
	case SETTING_OPT_TYPE_INT:
		*(int *)dst = (int)value;
		break;
	case SETTING_OPT_TYPE_UINT:
		*(unsigned int *)dst = (unsigned int)value;
		break;
	case SETTING_OPT_TYPE_INT64:
		*(long long *)dst = (long long)value;
		break;
	case SETTING_OPT_TYPE_UINT64:
		*(unsigned long long *)dst = (unsigned long long)value;
		break;
	default:
		dprint(PRN_ERROR, "Invalid type:%d", option->type);
		break;
	}
	return 0;
}

static void vdec_store_setting_value(const void *start_addr, const char *key, const int key_len, const char *value)
{
	void *dst = NULL;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(setting_options); i++) {
		const struct vdec_setting_option *option = &setting_options[i];
		dst = ((unsigned char *)start_addr) + option->offset;
		if (strlen(option->name) == key_len &&
			!strncmp(key, option->name, key_len)) {
			switch (option->type) {
			case SETTING_OPT_TYPE_BOOL:
			case SETTING_OPT_TYPE_INT:
			case SETTING_OPT_TYPE_UINT:
			case SETTING_OPT_TYPE_INT64:
			case SETTING_OPT_TYPE_UINT64: {
				long long val = 0;
				if (kstrtoll(value, 0, &val) < 0)
					return;
				write_number(dst, option, val);
			}
				return;
			case SETTING_OPT_TYPE_STRING: {
				int ret = strcpy_s((unsigned char *)dst, MAX_SETTING_STRING_LEN, value);
				if (ret != 0) {
					dprint(PRN_ERROR, "Failed to set string value. ret:%d.", ret);
				} else {
					dprint(PRN_ALWS, "set '%s' is %s.", option->help, value);
				}
			}
				return;
			}
		}
	}
}

ssize_t vdec_store_setting_info(struct vdec_setting_info *setting_info, const char *buf, const size_t count)
{
	char *pos = NULL;
	char *token = NULL;
	char *next_token = NULL;
	char data[MAX_SET_INFO_DATE_SIZE] = "";
	char *p_data = data;
	int rc;

	if (count > MAX_FRAME_WARN_SIZE) {
		dprint(PRN_ERROR, "count is out of range (size:%zu).", count);
		return count;
	}

	if (!buf || !setting_info)
		return count;

	rc = strcpy_s(p_data, sizeof(data), buf);
	if (rc != 0) {
		dprint(PRN_ERROR, "failed to copy buf (size:%zu, error:%d).", count, rc);
		return count;
	}

	token = strtok_s(p_data, ":", &next_token);
	if (token != NULL) {
		pos = strchr(token, '=');
		if (pos != NULL && (pos - token > 0) && strlen(pos) > 1)
			vdec_store_setting_value(setting_info, token, pos - token, pos + 1);
	}

	while (p_data != NULL) {
		token = strtok_s(NULL, ":", &next_token);
		if (token == NULL)
			break;

		pos = strchr(token, '=');
		if (pos != NULL && (pos - token > 0) && strlen(pos) > 1)
			vdec_store_setting_value(setting_info, token, pos - token, pos + 1);
	}

	return count;
}

#endif
