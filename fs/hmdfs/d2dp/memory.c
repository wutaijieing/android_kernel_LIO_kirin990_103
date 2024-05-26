// SPDX-License-Identifier: GPL-2.0
/* D2DP memory utils. */

#define pr_fmt(fmt) "[D2DP]: " fmt

#include <linux/list.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/uio.h>

#include "memory.h"

struct memseg *memseg_alloc(size_t size, memalloc_f alloc)
{
	struct memseg *seg = NULL;

	if (!size)
		return NULL;

	seg = alloc(size + sizeof(*seg));
	if (!seg)
		return NULL;

	INIT_LIST_HEAD(&seg->list);
	seg->len = size;
	return seg;
}

void memseg_free(struct memseg *seg, memfree_f free)
{
	free(seg);
}

size_t memseg_fill(struct memseg *seg, const void *data, size_t len)
{
	if (!seg || !data || len != seg->len)
		return 0;

	memcpy(seg->data, data, len);
	return len;
}

size_t memseg_fill_iter(struct memseg *seg, struct iov_iter *iter)
{
	if (!seg || !iter || iov_iter_count(iter) < seg->len)
		return 0;

	return copy_from_iter(seg->data, seg->len, iter);
}
