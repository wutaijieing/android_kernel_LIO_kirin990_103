/* SPDX-License-Identifier: GPL-2.0 */
/* D2DP memory utils. */

#ifndef D2D_MEMORY_H
#define D2D_MEMORY_H

#include <linux/types.h>
#include <linux/uio.h>

/* declare simplified alloc/free interfaces to customize allocations */
typedef void *(*memalloc_f)(size_t size);
typedef void (*memfree_f)(void *obj);

/**
 * struct memseg - memory segment, collected in a linked list
 *
 * The segments can be useful for avoiding reallocations.
 *
 * @list: list head for chaining segments together
 * @len: segment length
 * @data: start of data storage with @len length
 */
struct memseg {
	struct list_head list;
	size_t len;
	char data[0];
};

struct memseg *memseg_alloc(size_t size, memalloc_f alloc);
void memseg_free(struct memseg *seg, memfree_f free);
size_t memseg_fill(struct memseg *seg, const void *data, size_t len);
size_t memseg_fill_iter(struct memseg *seg, struct iov_iter *iter);

#endif /* D2D_MEMORY_H */
