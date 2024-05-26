/* SPDX-License-Identifier: GPL-2.0 */
#ifndef D2D_RX_BUFFER_H
#define D2D_RX_BUFFER_H

#include <linux/types.h>

#include "buffer.h"
#include "wrappers.h"

struct rx_node {
	struct list_head list;
	struct ndesc ndesc;
	char data[0];
};

struct rx_buffer {
	struct list_head seqlist;
	struct list_head ooolist;
	wrap_t reader_pos;
	size_t node_read_offset;
	size_t seqbytes;
	size_t usage;
	size_t softlimit;
	size_t hardlimit;
	node_alloc_f alloc;
	node_dealloc_f dealloc;
};

enum node_point {
	NODE_POINT_LEFT,       /* [=====] |         */
	NODE_POINT_LADJACENT,  /*   [=====|         */
	NODE_POINT_COVER,      /*      [==|==]      */
	NODE_POINT_RADJACENT,  /*         |=====]   */
	NODE_POINT_RIGHT,      /*         | [=====] */
};
enum node_point cmp_node_point(const struct ndesc *desc, wrap_t point);

enum node_node {
	NODE_NODE_LEFT,       /* [==1==]  [==2==]          */
	NODE_NODE_LADJACENT,  /*   [==1==][==2==]          */
	NODE_NODE_INTERSECT,  /*       [==[==]==]          */
	NODE_NODE_EXACT,      /*          [=1=2=]          */
	NODE_NODE_RADJACENT,  /*          [==2==][==1==]   */
	NODE_NODE_RIGHT,      /*          [==2==]  [==1==] */
};
enum node_node cmp_node_node(const struct ndesc *n1, const struct ndesc *n2);

struct rx_buf_scan_result {
	enum {
		RX_BUF_SCAN_OK,
		RX_BUF_SCAN_CONSUMED,
		RX_BUF_SCAN_DUPLICATE,
		RX_BUF_SCAN_BADPACKET,
		RX_BUF_SCAN_OVERFLOW,
		RX_BUF_SCAN_OUTOFORDER,
		RX_BUF_SCAN_BUG,
	} verdict;
	const struct list_head *iter;
};

struct rx_buf_scan_result __rx_buf_scan_seqlist_new(const struct rx_buffer *rxb,
						    const struct ndesc *desc);

struct rx_buf_scan_result
__rx_buf_scan_seqlist_append(const struct rx_buffer *rxb,
			     const struct ndesc *desc);

struct rx_buf_scan_result __rx_buf_scan_seqlist(const struct rx_buffer *rxb,
						const struct ndesc *desc);

struct rx_buf_scan_result __rx_buf_scan_ooolist(const struct rx_buffer *rxb,
						const struct ndesc *desc);

struct rx_buf_scan_result rx_buf_scan_hardlimit(const struct rx_buffer *rxb,
						const struct ndesc *desc);

struct rx_buf_scan_result rx_buf_scan_softlimit(const struct rx_buffer *rxb,
						const struct ndesc *desc);

struct rx_buf_scan_result rx_buf_scan_nolimit(const struct rx_buffer *rxb,
					      const struct ndesc *desc);

size_t rx_buf_put_calc_seqcount(const struct rx_buffer *rxb);
void rx_buf_put_splice_ooo(struct rx_buffer *rxb);

#endif /* D2D_RX_BUFFER_H */
