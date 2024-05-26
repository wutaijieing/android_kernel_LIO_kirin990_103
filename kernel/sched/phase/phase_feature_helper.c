/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature_helper.c
 *
 * Copyright (c) 2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <securec.h>

#define LINE_LEN		1024

/*
 * scan to consume the first line from start to end, and reset start
 * to the head of new line, return true if touch the end of str
 */
bool consume_line(const char *str, s64 *start, const s64 end)
{
	s64 pos;

	for (pos = *start; pos < end; pos++) {
		if (str[pos] == '\n') {
			pos++;
			*start = pos;
			break;
		}
	}
	return (pos == end);
}

void get_matches(const char *str, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	(void)vsscanf_s(str, format, args);
	va_end(args);
}

char *load_from_file(const char *file_path, loff_t *size)
{
	int ret;
	loff_t pos = 0;
	struct file *fp = NULL;
	struct kstat *stat = NULL;
	char *buf = NULL;
	mm_segment_t fs;

	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_err("file %s open fail\n", file_path);
		goto out;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	stat = kmalloc(sizeof(struct kstat), GFP_KERNEL);
	if (stat == NULL) {
		pr_err("kmalloc stat fail\n");
		goto out_close_file;
	}
	vfs_stat(file_path, stat);

	buf = kmalloc(stat->size * sizeof(char), GFP_KERNEL);
	if (buf == NULL) {
		pr_err("kmalloc buf fail, size=%d\n", stat->size);
		goto out_free_stat;
	}

	ret = vfs_read(fp, buf, stat->size, &pos);
	if (ret < 0) {
		pr_err("file %s read fail\n", file_path);
		goto out_free_buf;
	}

	*size = stat->size;
	kfree(stat);
	set_fs(fs);
	filp_close(fp, NULL);

	return buf;

out_free_buf:
	kfree(buf);
out_free_stat:
	kfree(stat);
out_close_file:
	set_fs(fs);
	filp_close(fp, NULL);
out:
	return NULL;
}

int parse_one_line(int *arr, char *buf, int col)
{
	char line[LINE_LEN];
	char *p = line;
	loff_t m = 0;
	loff_t n = 0;
	int i = 0;

	(void)memset_s(line, sizeof(line), 0, sizeof(line));
	do {
		if (buf[m] == '\n')
			break;
		line[n++] = buf[m++];
	} while(1);
	line[n] = '\0';
	do {
		p = skip_spaces(p);
		if ((*p >= '0' && *p <= '9') || (*p == '-'))
			get_matches(p, "%d", &arr[i++]);

		p = strchr(p, ' ');
		if (p == NULL)
			break;

		if (i > col) {
			pr_err("col %d execeed max_col %d\n", i, col);
			return -1;
		}
	} while (1);

	return i == col ? 0 : -1;
}
