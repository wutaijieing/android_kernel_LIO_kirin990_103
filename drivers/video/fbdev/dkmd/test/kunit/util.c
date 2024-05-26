/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "util.h"
#include <linux/string.h>

/** XML entity code mapping structure and special characters. */
static const struct {
	char special_char;
	char* replacement;
} bindings [] = {
	{'&', "&amp;"},
	{'>', "&gt;"},
	{'<', "&lt;"}
};

/**
 * @brief Get the index object
 *
 * @param ch
 * @return int
 */
static int get_index(char ch)
{
	int length = sizeof(bindings) / sizeof(bindings[0]);
	int counter;

	for (counter = 0; counter < length && bindings[counter].special_char != ch; ++counter)
		;

	return (counter < length ? counter : -1);
}

/**
 * @brief Converts XML special characters to XML entity code
 *
 * @param sz_src
 * @param sz_dest
 * @param maxlen
 * @return int
 */
int ku_translate_special_characters(const char* sz_src, char* sz_dest, size_t maxlen)
{
	int count = 0;
	size_t src = 0;
	size_t dest = 0;
	size_t length = strlen(sz_src);
	int conv_index;

	if (!sz_dest)
		return 0;

	memset(sz_dest, 0, maxlen);
	while ((dest < maxlen) && (src < length)) {
		if ((-1 != (conv_index = get_index(sz_src[src]))) &&
			((dest + (int)strlen(bindings[conv_index].replacement)) <= maxlen)) {
			strcat(sz_dest, bindings[conv_index].replacement);
			dest += (int)strlen(bindings[conv_index].replacement);
			++count;
		} else {
			sz_dest[dest++] = sz_src[src];
		}
		++src;
	}

	return count;
}
