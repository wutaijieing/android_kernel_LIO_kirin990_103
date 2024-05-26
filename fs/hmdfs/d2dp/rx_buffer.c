// SPDX-License-Identifier: GPL-2.0
/*
 * D2D RX buffer implementation
 */

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "buffer.h"
#include "resources.h"
#include "rx_buffer.h"
#include "wrappers.h"

static size_t rx_buf_reserve(size_t reserve)
{
	/* worst case: the reserved area is filled by 1-byte packets */
	reserve *= sizeof(struct rx_node) + 1;

	/* reserve at least one full node for sure */
	reserve += sizeof(struct rx_node) + U16_MAX;

	return reserve;
}

struct rx_buffer *rx_buf_alloc(size_t bufsize,
			       size_t waitall_limit,
			       node_alloc_f alloc,
			       node_dealloc_f dealloc)
{
	struct rx_buffer *rxb = NULL;

	if (!bufsize || !waitall_limit || !alloc || !dealloc)
		return NULL;

	rxb = d2dp_kzalloc(sizeof(*rxb), GFP_KERNEL);
	if (!rxb)
		return NULL;

	INIT_LIST_HEAD(&rxb->seqlist);
	INIT_LIST_HEAD(&rxb->ooolist);

	rxb->reader_pos = 0;
	rxb->node_read_offset = 0;
	rxb->seqbytes = 0;
	rxb->usage = 0;
	rxb->softlimit = bufsize;
	rxb->hardlimit = bufsize + rx_buf_reserve(waitall_limit);
	rxb->alloc = alloc;
	rxb->dealloc = dealloc;

	return rxb;
}

void rx_buf_free(struct rx_buffer *rxb)
{
	if (!rxb)
		return;

	if (!list_empty(&rxb->seqlist)) {
		struct rx_node *node = NULL;
		struct rx_node *next = NULL;

		list_for_each_entry_safe(node, next, &rxb->seqlist, list) {
			list_del(&node->list);
			rxb->dealloc(node);
		}
	}

	if (!list_empty(&rxb->ooolist)) {
		struct rx_node *node = NULL;
		struct rx_node *next = NULL;

		list_for_each_entry_safe(node, next, &rxb->ooolist, list) {
			list_del(&node->list);
			rxb->dealloc(node);
		}
	}

	d2dp_kfree(rxb);
}

static struct rx_node *rx_node_alloc(struct rx_buffer *rxb,
				     const struct ndesc *desc,
				     const void *data)
{
	struct rx_node *node = NULL;

	node = rxb->alloc(sizeof(*node) + desc->len);
	if (!node)
		return NULL;

	INIT_LIST_HEAD(&node->list);
	node->ndesc = *desc;
	memcpy(node->data, data, desc->len);

	rxb->usage += sizeof(*node);
	rxb->usage += desc->len;

	return node;
}

static void rx_node_dealloc(struct rx_buffer *rxb, struct rx_node *node)
{
	rxb->usage -= sizeof(*node);
	rxb->usage -= node->ndesc.len;
	rxb->dealloc(node);
}

size_t rx_buf_get_available_bytes(const struct rx_buffer *rxb)
{
	return rxb->seqbytes;
}

uint32_t rx_buf_get_ack(const struct rx_buffer *rxb)
{
	const struct rx_node *lastseq = NULL;

	if (list_empty(&rxb->seqlist))
		return rxb->reader_pos;

	lastseq = list_last_entry(&rxb->seqlist, struct rx_node, list);
	return lastseq->ndesc.seq + lastseq->ndesc.len;
}

static void __append_sack_pair(struct sack_pairs *s_pairs, u32 start, u32 end)
{
	size_t s_len = s_pairs->len;

	s_pairs->pairs[s_len].l = start;
	s_pairs->pairs[s_len].r = end;
	s_pairs->len += 1;
}

static bool __check_sack_pair_len_limits(const struct sack_pairs *s_pairs,
					 size_t len)
{
	return s_pairs->len + 1 <= len;
}

void rx_buf_generate_sack_pairs(const struct rx_buffer *rxb,
				struct sack_pairs *pairs,
				size_t len)
{
	const struct rx_node *iter = NULL;
	u32 start_seq = 0;
	u32 end_seq   = 0;

	if (list_empty(&rxb->ooolist)) {
		pairs->len = 0;
		return;
	}

	iter = list_first_entry(&rxb->ooolist, struct rx_node, list);

	/* set initial seq ids */
	start_seq = iter->ndesc.seq;
	end_seq = start_seq + iter->ndesc.len;

	/* continue after last acked node */
	list_for_each_entry_continue(iter, &rxb->ooolist, list) {
		if (d2d_wrap_eq(iter->ndesc.seq, end_seq)) {
			end_seq = iter->ndesc.seq + iter->ndesc.len;
			continue;
		}

		/*
		 * sack pair end found, check if there is enough space to
		 * store it
		 */
		if (!__check_sack_pair_len_limits(pairs, len))
			return;

		__append_sack_pair(pairs, start_seq, end_seq - 1);

		/* new sack pair started */
		start_seq = iter->ndesc.seq;
		end_seq = start_seq + iter->ndesc.len;
	}

	/* append last pair found if there is enough space */
	if (__check_sack_pair_len_limits(pairs, len))
		__append_sack_pair(pairs, start_seq, end_seq - 1);
}

size_t rx_buf_get(struct rx_buffer *rxb, void *data, size_t len)
{
	struct rx_node *node = NULL;
	struct rx_node *next = NULL;
	size_t total_copied = 0;

	/* seqlist data is always ready to read, so traverse all */
	list_for_each_entry_safe(node, next, &rxb->seqlist, list) {
		size_t offset  = 0;
		size_t to_copy = 0;
		struct ndesc *ndesc = &node->ndesc;

		/*
		 * The result of subtraction is non-negative
		 * (total_copied <= len, node_read_offset <= len)
		 */
		offset = rxb->node_read_offset;
		to_copy = min(len - total_copied, ndesc->len - offset);
		memcpy(data + total_copied, node->data + offset, to_copy);
		rxb->node_read_offset += to_copy;
		total_copied += to_copy;

		/* This node has no data anymore */
		if (rxb->node_read_offset == ndesc->len) {
			rxb->node_read_offset = 0;
			list_del(&node->list);
			rx_node_dealloc(rxb, node);
		}

		if (total_copied == len)
			break;
	}

	rxb->reader_pos += total_copied;
	rxb->seqbytes -= total_copied;

	return total_copied;
}

enum node_point cmp_node_point(const struct ndesc *desc, wrap_t point)
{
	wrap_t lo = desc->seq;
	wrap_t hi = desc->seq + desc->len;

	if (d2d_wrap_lt(hi, point))
		return NODE_POINT_LEFT;

	if (d2d_wrap_eq(hi, point))
		return NODE_POINT_LADJACENT;

	if (d2d_wrap_eq(lo, point))
		return NODE_POINT_RADJACENT;

	if (d2d_wrap_gt(lo, point))
		return NODE_POINT_RIGHT;

	/* point is inside the node otherwise */
	return NODE_POINT_COVER;
}

enum node_node cmp_node_node(const struct ndesc *n1, const struct ndesc *n2)
{
	wrap_t lo1 = n1->seq;
	wrap_t hi1 = n1->seq + n1->len;
	wrap_t lo2 = n2->seq;
	wrap_t hi2 = n2->seq + n2->len;

	if (d2d_wrap_lt(hi1, lo2))
		return NODE_NODE_LEFT;

	if (d2d_wrap_eq(hi1, lo2))
		return NODE_NODE_LADJACENT;

	if (d2d_wrap_eq(lo1, lo2) && d2d_wrap_eq(hi1, hi2))
		return NODE_NODE_EXACT;

	if (d2d_wrap_eq(lo1, hi2))
		return NODE_NODE_RADJACENT;

	if (d2d_wrap_gt(lo1, hi2))
		return NODE_NODE_RIGHT;

	/* otherwise the nodes intersect */
	return NODE_NODE_INTERSECT;
}

static struct rx_buf_scan_result __scan_error(int error)
{
	return (struct rx_buf_scan_result) {
		.verdict = error,
	};
}

static struct rx_buf_scan_result scan_error_overflow(void)
{
	return __scan_error(RX_BUF_SCAN_OVERFLOW);
}

static struct rx_buf_scan_result scan_error_overlap(void)
{
	return __scan_error(RX_BUF_SCAN_BADPACKET);
}

static struct rx_buf_scan_result scan_error_duplicate(void)
{
	return __scan_error(RX_BUF_SCAN_DUPLICATE);
}

static struct rx_buf_scan_result scan_error_consumed(void)
{
	return __scan_error(RX_BUF_SCAN_CONSUMED);
}

static struct rx_buf_scan_result scan_error_bug(void)
{
	return __scan_error(RX_BUF_SCAN_BUG);
}

static struct rx_buf_scan_result scan_outoforder(void)
{
	return (struct rx_buf_scan_result) {
		.verdict = RX_BUF_SCAN_OUTOFORDER,
	};
}

static struct rx_buf_scan_result scan_ok(const struct list_head *iter)
{
	return (struct rx_buf_scan_result) {
		.verdict = RX_BUF_SCAN_OK,
		.iter = iter,
	};
}

struct rx_buf_scan_result __rx_buf_scan_seqlist_new(const struct rx_buffer *rxb,
						    const struct ndesc *desc)
{
	switch (cmp_node_point(desc, rxb->reader_pos)) {
	case NODE_POINT_LEFT:
	case NODE_POINT_LADJACENT:
		return scan_error_consumed();
	case NODE_POINT_COVER:
		return scan_error_overlap();
	case NODE_POINT_RADJACENT:
		return scan_ok(&rxb->seqlist); // insert to the beginning
	case NODE_POINT_RIGHT:
		return scan_outoforder(); // cannot insert to seqlist
	default:
		WARN_ON(true);
		return scan_error_bug();
	}
}

struct rx_buf_scan_result
__rx_buf_scan_seqlist_append(const struct rx_buffer *rxb,
			     const struct ndesc *desc)
{
	struct rx_node *last = NULL;

	last = list_last_entry(&rxb->seqlist, struct rx_node, list);
	switch (cmp_node_node(desc, &last->ndesc)) {
	case NODE_NODE_LEFT:
	case NODE_NODE_LADJACENT:
		return scan_error_consumed();
	case NODE_NODE_EXACT:
		return scan_error_duplicate();
	case NODE_NODE_INTERSECT:
		return scan_error_overlap();
	case NODE_NODE_RADJACENT:
		return scan_ok(&last->list); // append to the tail
	case NODE_NODE_RIGHT:
		return scan_outoforder(); // cannot insert to seqlist
	default:
		WARN_ON(true);
		return scan_error_bug();
	}
}

struct rx_buf_scan_result __rx_buf_scan_seqlist(const struct rx_buffer *rxb,
						const struct ndesc *desc)
{
	struct rx_buf_scan_result res = { 0 };

	if (list_empty(&rxb->seqlist))
		res = __rx_buf_scan_seqlist_new(rxb, desc);
	else
		res = __rx_buf_scan_seqlist_append(rxb, desc);

	/* We are ready to add in seqlist, check it does not clash ooo */
	if (res.verdict == RX_BUF_SCAN_OK && !list_empty(&rxb->ooolist)) {
		struct rx_node *fst = list_first_entry(&rxb->ooolist,
						       struct rx_node,
						       list);

		/* the node must be on the left side of ooo list */
		switch (cmp_node_node(desc, &fst->ndesc)) {
		case NODE_NODE_LEFT:
		case NODE_NODE_LADJACENT:
			return res;
		case NODE_NODE_INTERSECT:
			return scan_error_overlap();
		default:
			/* the invariant is broken */
			WARN_ON(true);
			return scan_error_bug();
		}
	}

	return res;
}

struct rx_buf_scan_result __rx_buf_scan_ooolist(const struct rx_buffer *rxb,
						const struct ndesc *desc)
{
	struct rx_node *iter = NULL;

	list_for_each_entry_reverse(iter, &rxb->ooolist, list) {
		switch (cmp_node_node(desc, &iter->ndesc)) {
		case NODE_NODE_EXACT:
			return scan_error_duplicate();
		case NODE_NODE_INTERSECT:
			return scan_error_overlap();
		case NODE_NODE_RIGHT:
		case NODE_NODE_RADJACENT:
			return scan_ok(&iter->list); // insert after this node
		case NODE_NODE_LEFT:
		case NODE_NODE_LADJACENT:
			continue;
		default:
			WARN_ON(true);
			return scan_error_bug();
		}
	}

	return scan_ok(&rxb->ooolist);
}

struct rx_buf_scan_result rx_buf_scan_hardlimit(const struct rx_buffer *rxb,
						const struct ndesc *desc)
{
	return scan_error_overflow();
}

struct rx_buf_scan_result rx_buf_scan_softlimit(const struct rx_buffer *rxb,
						const struct ndesc *desc)
{
	struct rx_buf_scan_result result = { 0 };

	result = __rx_buf_scan_seqlist(rxb, desc);
	if (result.verdict == RX_BUF_SCAN_OUTOFORDER)
		/* out of order packets are dropped in softlimit mode */
		return scan_error_overflow();

	/* otherwise, return the scan result */
	return result;
}

struct rx_buf_scan_result rx_buf_scan_nolimit(const struct rx_buffer *rxb,
					      const struct ndesc *desc)
{
	struct rx_buf_scan_result result = { 0 };

	result = __rx_buf_scan_seqlist(rxb, desc);
	if (result.verdict == RX_BUF_SCAN_OUTOFORDER)
		/* out of order, insert it to ooo list */
		return __rx_buf_scan_ooolist(rxb, desc);

	/* otherwise, return the scan result */
	return result;
}

void rx_buf_put_splice_ooo(struct rx_buffer *rxb)
{
	struct rx_node *iter = NULL;
	struct list_head newseq = { 0 };
	wrap_t nextseq = 0;

	INIT_LIST_HEAD(&newseq);

	/* find the seqential sublist in ooo list */
	iter = list_first_entry(&rxb->ooolist, struct rx_node, list);
	nextseq = iter->ndesc.seq + iter->ndesc.len;
	list_for_each_entry_continue(iter, &rxb->ooolist, list) {
		if (!d2d_wrap_eq(iter->ndesc.seq, nextseq))
			break;

		nextseq = iter->ndesc.seq + iter->ndesc.len;
	}

	list_cut_before(&newseq, &rxb->ooolist, &iter->list);
	list_splice_tail(&newseq, &rxb->seqlist);
}

size_t rx_buf_put_calc_seqcount(const struct rx_buffer *rxb)
{
	const struct rx_node *first = NULL;
	const struct rx_node *last = NULL;
	wrap_t first_byte = 0;
	wrap_t last_byte_ex = 0;

	if (list_empty(&rxb->seqlist))
		return 0;

	first = list_first_entry(&rxb->seqlist, struct rx_node, list);
	last = list_last_entry(&rxb->seqlist, struct rx_node, list);

	first_byte = first->ndesc.seq;
	last_byte_ex = last->ndesc.seq + last->ndesc.len;

	/*
	 * V    first            last   V
	 * [--|-----)[-------)[---------) seqlist
	 *    node_read_offset
	 */
	return d2d_wrap_sub(last_byte_ex, first_byte) - rxb->node_read_offset;
}

enum rx_buf_put_result rx_buf_put(struct rx_buffer *rxb,
				  const struct ndesc *desc,
				  const void *data)
{
	struct rx_node *node = NULL;
	struct list_head *iter = NULL;
	struct rx_buf_scan_result result = { 0 };

	/* perform different scans based on current memory usage */
	if (rxb->usage >= rxb->hardlimit)
		result = rx_buf_scan_hardlimit(rxb, desc);
	else if (rxb->usage >= rxb->softlimit)
		result = rx_buf_scan_softlimit(rxb, desc);
	else
		result = rx_buf_scan_nolimit(rxb, desc);

	switch (result.verdict) {
	case RX_BUF_SCAN_OK: {
		iter = (struct list_head *)result.iter; // const cast for insert

		node = rx_node_alloc(rxb, desc, data);
		if (!node)
			return RX_BUF_PUT_NOMEM;

		list_add(&node->list, iter);

		/* check whether the ooo list is now connected with seqlist */
		if (!list_empty(&rxb->seqlist) && !list_empty(&rxb->ooolist)) {
			struct rx_node *seq = list_last_entry(&rxb->seqlist,
							      struct rx_node,
							      list);
			struct rx_node *ooo = list_first_entry(&rxb->ooolist,
							       struct rx_node,
							       list);

			switch (cmp_node_node(&seq->ndesc, &ooo->ndesc)) {
			case NODE_NODE_LADJACENT:
				rx_buf_put_splice_ooo(rxb);
				break;
			case NODE_NODE_LEFT:
				break;
			default:
				/* the invariant is broken */
				WARN_ON(true);
				return RX_BUF_PUT_INTERNAL_ERROR;
			}
		}

		/* update seqcount as we may have new data for the reader */
		rxb->seqbytes = rx_buf_put_calc_seqcount(rxb);
		return RX_BUF_PUT_OK;
	}
	case RX_BUF_SCAN_CONSUMED:
		return RX_BUF_PUT_CONSUMED;
	case RX_BUF_SCAN_DUPLICATE:
		return RX_BUF_PUT_DUPLICATE;
	case RX_BUF_SCAN_OVERFLOW:
		return RX_BUF_PUT_OVERFLOW;
	case RX_BUF_SCAN_BADPACKET:
		return RX_BUF_PUT_BAD_PACKET;
	default:
		/* other scan errors must not happen here */
		WARN_ON(true);
		return RX_BUF_PUT_INTERNAL_ERROR;
	}
}

size_t rx_buf_get_fill_percentage(const struct rx_buffer *rxb)
{
	/* Calculate current RX buffer fill percentage */
	return DIV_ROUND_UP(rxb->usage * 100, rxb->softlimit);
}

size_t rx_buf_get_usage(const struct rx_buffer *rxb)
{
	return rxb->usage;
}
