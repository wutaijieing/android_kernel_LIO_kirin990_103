/*
 * vfmw_proc.c
 *
 * This is for vfmw proc.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
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

#include "vfmw_proc.h"
#include "dbg.h"
#include "vfmw_proc_stm.h"
#include "vfmw_proc_dec.h"

#define PROC_VFMW_STM "vfmw_stm"
#define PROC_VFMW_DEC "vfmw_dec"
#define PROC_VFMW_COM "vfmw_com"

static vfmw_proc_entry g_proc_entry;

vfmw_proc_entry *vfmw_proc_get_entry(void)
{
	return &g_proc_entry;
}

int str_to_val(const char *str, unsigned int *data)
{
	unsigned int i;
	unsigned int d;
	unsigned int weight;
	unsigned int dat = 0;

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		i = 2; // 2: start idx
		weight = 16; // 16: decimal
	} else {
		i = 0;
		weight = 10; // 10: decimal
	}

	for (; i < 10; i++) { // 10: maxlen
		if (str[i] < 0x20)
			break;
		else if (weight == 16 && str[i] >= 'a' && str[i] <= 'f') // 16: decimal
			d = str[i] - 'a' + 10; // 10: convert to digit
		else if (weight == 16 && str[i] >= 'A' && str[i] <= 'F') // 16: decimal
			d = str[i] - 'A' + 10; // 10: convert to digit
		else if (str[i] >= '0' && str[i] <= '9')
			d = str[i] - '0';
		else
			return -1;

		dat = dat * weight + d;
	}

	*data = dat;

	return 0;
}

void vfmw_proc_parse_str(const char *cmd, unsigned int count, int *index,
	char *str)
{
	unsigned int i = *index;
	unsigned int j = 0;

	for (; i < count; i++) {
		if (j == 0 && cmd[i] == ' ')
			continue;

		if (cmd[i] > ' ') {
			if (j >= (CMD_PARAM_MAX_LEN - 1)) {
				dprint(PRN_ERROR, "cmd size: %u > %d\n", j,
					CMD_PARAM_MAX_LEN);
				break;
			}
			str[j++] = cmd[i];
		}

		if (j > 0 && cmd[i] == ' ')
			break;
	}
	str[j] = 0;

	*index = i;
}

unsigned int vfmw_proc_parse_cmd(const char *buf, int count, cmd_str_ptr cmd_str)
{
	int pos = 0;
	int index = 0;
	char cmd[CMD_STR_MAX_LEN];

	if (!buf) {
		dprint(PRN_ERROR, "your parameter buf is NULL\n");
		return 0;
	}

	if (count <= 0 || count >= CMD_STR_MAX_LEN) {
		dprint(PRN_ERROR,
			"your parameter string size: %d >= %d or <= 0\n", count,
			CMD_STR_MAX_LEN);
		return 0;
	}

	(void)memset_s(cmd, sizeof(cmd), 0, sizeof(cmd));

	if (OS_COPY_FROM_USER(cmd, buf, count))
		return 0;

	cmd[count] = 0;

	while (pos < count && index < CMD_PARAM_MAX_COUNT)
		vfmw_proc_parse_str(cmd, count, &pos, (*cmd_str)[index++]);

	return index;
}

int vfmw_create_proc(void)
{
	int ret;
	vfmw_proc_entry *entry = vfmw_proc_get_entry();

	ret = OS_PROC_CREATE(PROC_VFMW_STM, stm_read_proc, stm_write_proc);
	vfmw_assert_ret_prnt(ret == 0, -1, "create proc %s failed\n", PROC_VFMW_STM);

	ret = OS_PROC_CREATE(PROC_VFMW_DEC, dec_read_proc, dec_write_proc);
	vfmw_assert_ret_prnt(ret == 0, -1, "create proc %s failed\n", PROC_VFMW_DEC);
	entry->init = 1;

	OS_SEMA_INIT(&entry->proc_sema);

	return ret;
}

void vfmw_destroy_proc(void)
{
	vfmw_proc_entry *entry = vfmw_proc_get_entry();

	if (!entry->init)
		return;

	OS_PROC_DESTROY(PROC_VFMW_STM);
	OS_PROC_DESTROY(PROC_VFMW_DEC);
	entry->init = 0;

	OS_SEMA_EXIT(entry->proc_sema);
}

