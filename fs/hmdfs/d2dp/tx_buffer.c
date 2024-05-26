// SPDX-License-Identifier: GPL-2.0
/*
 * D2D TX buffer implementation
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uio.h>

#include "buffer.h"
#include "memory.h"
#include "resources.h"
#include "tx_buffer.h"
#include "wrappers.h"

struct tx_buffer *tx_buf_alloc(size_t max_elem, gfp_t gfp,
			       node_alloc_f alloc,
			       node_dealloc_f dealloc)
{
	struct tx_buffer *txb = NULL;

	if (!max_elem || !alloc || !dealloc)
		return NULL;

	txb = d2dp_kzalloc(sizeof(*txb), GFP_KERNEL);
	if (!txb)
		return NULL;

	INIT_LIST_HEAD(&txb->nodes);
	txb->max_elem = max_elem;
	txb->alloc = alloc;
	txb->dealloc = dealloc;
	txb->node_gfp = gfp;
	txb->node_cache = d2dp_kmem_cache_create("d2dp_tx_kmem",
						 sizeof(struct tx_node),
						 0, 0, NULL);
	if (!txb->node_cache)
		goto free_txb;

	return txb;

free_txb:
	d2dp_kfree(txb);
	return NULL;
}

void tx_buf_set_max_elem(struct tx_buffer *txb, size_t max_elem)
{
	txb->max_elem = max_elem;
}

size_t tx_buf_get_max_elem(const struct tx_buffer *txb)
{
	return txb->max_elem;
}

static struct memseg *tx_memseg_alloc(struct tx_buffer *txb, size_t len)
{
	struct memseg *seg = NULL;

	seg = memseg_alloc(len, txb->alloc);
	if (!seg)
		return NULL;

	txb->mem_usage += sizeof(*seg);
	txb->mem_usage += len;

	return seg;
}

static void tx_memseg_free(struct tx_buffer *txb, struct memseg *seg)
{
	txb->mem_usage -= seg->len;
	txb->mem_usage -= sizeof(*seg);
	memseg_free(seg, txb->dealloc);
}

struct tx_node *tx_node_alloc(struct tx_buffer *txb, size_t len)
{
	struct tx_node *node = NULL;
	struct memseg *seg = NULL;

	node = d2dp_kmem_cache_alloc(txb->node_cache, txb->node_gfp);
	if (!node)
		return NULL;

	txb->mem_usage += sizeof(*node);

	seg = tx_memseg_alloc(txb, len);
	if (!seg)
		goto free_node;

	INIT_LIST_HEAD(&node->list);
	INIT_LIST_HEAD(&node->segs);
	node->state = TX_NODE_STATE_UNSENT;
	node->ndesc = (struct ndesc) { 0 };
	list_add(&seg->list, &node->segs);

	return node;

free_node:
	txb->mem_usage -= sizeof(*node);
	kmem_cache_free(txb->node_cache, node);
	return NULL;
}

static void tx_node_free(struct tx_buffer *txb, struct tx_node *node)
{
	struct list_head *iter = NULL;
	struct list_head *next = NULL;

	list_for_each_safe(iter, next, &node->segs) {
		struct memseg *seg = list_entry(iter, struct memseg, list);

		list_del(iter);
		tx_memseg_free(txb, seg);
	}

	txb->mem_usage -= sizeof(*node);
	d2dp_kmem_cache_free(txb->node_cache, node);
}

void tx_node_list_free(struct tx_buffer *txb, struct list_head *head)
{
	struct list_head *iter = NULL;
	struct list_head *next = NULL;

	list_for_each_safe(iter, next, head) {
		struct tx_node *node = list_entry(iter, struct tx_node, list);

		list_del(&node->list);
		tx_node_free(txb, node);
	}
}

void tx_buf_free(struct tx_buffer *txb)
{
	if (!txb)
		return;

	tx_node_list_free(txb, &txb->nodes);
	d2dp_kmem_cache_destroy(txb->node_cache);
	d2dp_kfree(txb);
}

static void tx_node_copy_data(const struct tx_node *node,
			      void *buffer, size_t len)
{
	struct list_head *iter = NULL;
	void *pos = buffer;

	if (WARN_ON(len < node->ndesc.len))
		return;

	list_for_each(iter, &node->segs) {
		struct memseg *seg = list_entry(iter, struct memseg, list);

		memcpy(pos, seg->data, seg->len);
		pos += seg->len;
	}
}

static bool tx_node_writeable(const struct tx_buffer *txb,
			      const struct tx_node *node)
{
	return node->state <= TX_NODE_STATE_UNSENT &&
		node->ndesc.len < txb->max_elem;
}

static size_t tx_node_free_space(const struct tx_buffer *txb,
				 const struct tx_node *node)
{
	return txb->max_elem - node->ndesc.len;
}

static bool __tx_buf_prealloc_nodes(size_t to_alloc,
				    struct tx_buffer *txb,
				    struct list_head *prealloc)
{
	size_t i        = 0;
	size_t nr_nodes = 0;

	/* calculate amount of nodes to allocate */
	nr_nodes = DIV_ROUND_UP(to_alloc, txb->max_elem);
	if (WARN_ON(!nr_nodes))
		return false;

	/* allocate nodes and store them in temporary list */
	for (i = 0; i < nr_nodes; i++) {
		struct tx_node *node = NULL;
		size_t len = min(to_alloc, txb->max_elem);

		node = tx_node_alloc(txb, len);
		if (!node)
			goto fail;

		list_add_tail(&node->list, prealloc);
		to_alloc -= len;
	}

	return true;

fail:
	tx_node_list_free(txb, prealloc);
	return false;
}

static ssize_t __tx_buf_fill_node_iter(struct tx_buffer *txb,
				       struct tx_node *node,
				       struct iov_iter *iter)
{
	size_t copied = 0;
	struct memseg *seg = NULL;

	if (WARN_ON(!tx_node_writeable(txb, node)))
		return -EINVAL;

	seg = list_last_entry(&node->segs, struct memseg, list);
	copied = memseg_fill_iter(seg, iter);
	if (WARN_ON(!copied))
		return -EINVAL;

	txb->tx_seq_id += copied;
	node->ndesc.len += copied;
	node->ndesc.seq = txb->tx_seq_id - node->ndesc.len;
	if (!txb->first_to_send)
		txb->first_to_send = node;

	return copied;
}

static ssize_t __tx_buf_fill_nodes_iter(struct tx_buffer *txb,
					struct tx_node *node,
					struct iov_iter *iter)
{
	ssize_t res = 0;
	ssize_t len = iov_iter_count(iter);

	while (iov_iter_count(iter)) {
		res = __tx_buf_fill_node_iter(txb, node, iter);
		if (res <= 0)
			return res;

		node = list_next_entry(node, list);
	}

	return len;
}

static ssize_t __tx_buf_append_last(struct tx_buffer *txb,
				    struct tx_node *last,
				    struct iov_iter *iter)
{
	size_t to_append = 0;
	struct memseg *seg = NULL;

	/* allocate a segment for the last node */
	to_append = iov_iter_count(iter);
	seg = tx_memseg_alloc(txb, to_append);
	if (!seg)
		return -ENOMEM;

	list_add_tail(&seg->list, &last->segs);

	return __tx_buf_fill_node_iter(txb, last, iter);
}

static ssize_t __tx_buf_append_last_alloc(struct tx_buffer *txb,
					  struct tx_node *last,
					  struct iov_iter *iter)
{
	ssize_t len = 0;
	ssize_t res = 0;
	size_t free = 0;
	bool allocated = false;
	struct memseg *seg = NULL;
	LIST_HEAD(prealloc);

	/* allocate a segment to complete the last node */
	free = tx_node_free_space(txb, last);
	seg = tx_memseg_alloc(txb, free);
	if (!seg)
		goto alloc_fail;

	/* allocate for all the length minus free bytes in the last node */
	len = iov_iter_count(iter);
	allocated = __tx_buf_prealloc_nodes(len - free, txb, &prealloc);
	if (!allocated)
		goto free_memseg;

	/* add a segment to the last node to complete it */
	list_add_tail(&seg->list, &last->segs);

	/* add allocated nodes to the end of tx buffer */
	list_splice_tail(&prealloc, &txb->nodes);

	/* start filling with the last node */
	res = __tx_buf_fill_nodes_iter(txb, last, iter);
	if (WARN_ON(res != len))
		return -EINVAL;

	return len;

free_memseg:
	tx_memseg_free(txb, seg);
alloc_fail:
	return -ENOMEM;
}

static ssize_t __tx_buf_append_alloc(struct tx_buffer *txb,
				     struct iov_iter *iter)
{
	ssize_t len = 0;
	ssize_t res = 0;
	bool allocated = false;
	LIST_HEAD(prealloc);
	struct tx_node *node = NULL;

	/* allocate nodes for all the length */
	len = iov_iter_count(iter);
	allocated = __tx_buf_prealloc_nodes(len, txb, &prealloc);
	if (!allocated)
		return -ENOMEM;

	/* add allocated nodes to the end of tx buffer */
	node = list_first_entry(&prealloc, struct tx_node, list);
	list_splice_tail(&prealloc, &txb->nodes);

	/* start filling with the first allocated node */
	res = __tx_buf_fill_nodes_iter(txb, node, iter);
	if (WARN_ON(res != len))
		return -EINVAL;

	return len;
}

static void init_kvec_iter_read(struct iov_iter *iter, const struct kvec *kvec,
				unsigned long nr_segs, size_t count)
{
	iter->type = ITER_KVEC | READ;
	iter->kvec = kvec;
	iter->nr_segs = nr_segs;
	iter->iov_offset = 0;
	iter->count = count;
}

ssize_t tx_buf_append_iov(struct tx_buffer *txb, const struct kvec *iov,
			  size_t iovlen, size_t len)
{
	struct iov_iter iter = { 0 };

	if (!len)
		return -EINVAL;

	init_kvec_iter_read(&iter, iov, iovlen, len);

	if (!list_empty(&txb->nodes)) {
		struct tx_node *last = list_last_entry(&txb->nodes,
						       struct tx_node, list);
		if (tx_node_writeable(txb, last)) {
			size_t free = 0;

			free = tx_node_free_space(txb, last);
			if (len <= free)
				return __tx_buf_append_last(txb, last, &iter);
			else
				return __tx_buf_append_last_alloc(txb, last,
								  &iter);
		}
	}

	return __tx_buf_append_alloc(txb, &iter);
}

bool tx_buf_get_last_seq_id(const struct tx_buffer *txb, wrap_t *res)
{
	struct tx_node *node = NULL;

	if (list_empty(&txb->nodes))
		return false;

	node = list_last_entry(&txb->nodes, struct tx_node, list);
	*res = node->ndesc.seq + node->ndesc.len;
	return true;
}

static void tx_buf_switch_next_to_send(struct tx_buffer *txb)
{
	struct tx_node *node = txb->first_to_send;

	/*
	 * This function is called only from tx_buf_reset_first_to_send
	 * and tx_buf_get_packet.
	 * Both of them will not call us if first_to_send is NULL as
	 * they check it before.
	 */
	if (WARN_ON(!node))
		return;

	list_for_each_entry_continue(node, &txb->nodes, list) {
		if (node->state == TX_NODE_STATE_UNSENT ||
		    node->state == TX_NODE_STATE_NEED_RESEND) {
			txb->first_to_send = node;
			return;
		}
	}
	txb->first_to_send = NULL;
}

void tx_buf_reset_first_to_send(struct tx_buffer *txb)
{
	txb->first_to_send = list_first_entry_or_null(&txb->nodes,
						      struct tx_node,
						      list);
	if (txb->first_to_send &&
	    txb->first_to_send->state == TX_NODE_STATE_SENT)
		tx_buf_switch_next_to_send(txb);
}

enum tx_buf_get_packet_result tx_buf_get_packet(struct tx_buffer *txb,
						struct ndesc *desc,
						void *buffer, size_t size)
{
	struct tx_node *node = NULL;
	enum tx_buf_get_packet_result res = TX_BUF_GET_NO_PACKETS;

	node = txb->first_to_send;
	if (!node)
		/* nothing to send */
		return TX_BUF_GET_NO_PACKETS;

	WARN_ON(node->state == TX_NODE_STATE_SENT);

	if (txb->flowcontrol_mode && node->state != TX_NODE_STATE_NEED_RESEND)
		/* flowcontrol is on and nothing to resend */
		return TX_BUF_GET_NO_PACKETS;

	/* check whether the node is for resend */
	res = (node->state == TX_NODE_STATE_UNSENT)
		? TX_BUF_GET_NEW
		: TX_BUF_GET_RESEND;

	*desc = node->ndesc;
	tx_node_copy_data(node, buffer, size);

	/* update the node state and switch next to send */
	node->state = TX_NODE_STATE_SENT;
	tx_buf_switch_next_to_send(txb);

	return res;
}

void tx_buf_mark_as_need_resent(struct tx_buffer *txb)
{
	struct list_head *iter = NULL;

	list_for_each(iter, &txb->nodes) {
		struct tx_node *node = list_entry(iter, struct tx_node, list);

		/* Invariant: the list tail consists of UNSENT nodes */
		if (node->state == TX_NODE_STATE_UNSENT)
			break;

		node->state = TX_NODE_STATE_NEED_RESEND;
	}

	tx_buf_reset_first_to_send(txb);
}

/*
 * ACK packet is ill-formed, when ACK id intersects any packet
 *
 *        pkt.seq            pkt.seq + pkt.len
 *          |               |
 * pkt:     [XXXXXXXXXXXXXX]
 *                 |
 *                ACK
 *
 * (pkt.seq < ACK) AND (pkt.seq + pkt.len > ACK)
 */
static bool ill_ack_condition(const struct tx_node *node, wrap_t ack)
{
	return d2d_wrap_lt(node->ndesc.seq, ack) &&
		d2d_wrap_gt(node->ndesc.seq + node->ndesc.len, ack);
}

static ssize_t __tx_buf_free_ack(struct tx_buffer *txb, wrap_t ack_id)
{
	size_t freed = 0;
	struct tx_node *node = NULL;
	struct tx_node *next = NULL;
	bool need_reset = false;

	list_for_each_entry_safe(node, next, &txb->nodes, list) {
		/*
		 * First of all, check the correctess of the ACK
		 */
		if (ill_ack_condition(node, ack_id))
			return -EBADMSG;

		/*
		 * Internal list buffer keeps packets in the sending order so
		 * when condition is true for the first time, it is true for the
		 * rest of the list.
		 */
		if (d2d_wrap_ge(node->ndesc.seq, ack_id))
			break;

		/* Check that we are not deleting `first_to_send` node. */
		if (node == txb->first_to_send) {
			txb->first_to_send = NULL;
			need_reset = true;
		}

		++freed;
		list_del(&node->list);
		tx_node_free(txb, node);
	}

	/* Reset `first_to_send` because the node has been deleted. */
	if (need_reset)
		tx_buf_reset_first_to_send(txb);

	return freed;
}

/*
 * SACK packet is ill-formed when some packet interects some sack pair and not
 * strictly inside this pair
 */
static bool ill_sack_condition(const struct tx_node *node,
			       const struct sack_pair *sack)
{
	bool intersects =
		d2d_wrap_ge(node->ndesc.seq + node->ndesc.len - 1, sack->l) &&
		d2d_wrap_le(node->ndesc.seq, sack->r);

	bool inside =
		d2d_wrap_ge(node->ndesc.seq, sack->l) &&
		d2d_wrap_le(node->ndesc.seq + node->ndesc.len - 1, sack->r);

	return intersects && !inside;
}

static ssize_t __tx_buf_free_sack(struct tx_buffer *txb,
				  const struct sack_pairs *sack)
{
	struct list_head *iter = txb->nodes.next;
	struct list_head *next = iter->next;
	size_t pair_num = 0;
	size_t freed    = 0;

	/*
	 * We use the fact that SACKs and buffer nodes are ordered
	 */
	while (iter != &txb->nodes) {
		struct tx_node *node = list_entry(iter, struct tx_node, list);
		wrap_t l_sack = 0;
		wrap_t r_sack = 0;

		if (pair_num >= sack->len)
			break;

		if (ill_sack_condition(node, &sack->pairs[pair_num]))
			return -EBADMSG;

		l_sack = sack->pairs[pair_num].l;
		r_sack = sack->pairs[pair_num].r;

		/*
		 * The node is located on the left from [l_sack, r_sack] range.
		 * Switch to the next node.
		 */
		if (d2d_wrap_lt(node->ndesc.seq, l_sack)) {
			node->state = TX_NODE_STATE_NEED_RESEND;

			iter = next;
			next = iter->next;
			continue;
		}

		/*
		 * The node is located in [l_sack, r_sack] range, it is
		 * acknowledged so we safely delete it from the buffer.
		 */
		if (d2d_wrap_le(node->ndesc.seq, r_sack)) {
			/*
			 * This packet is the last in this SACK pair, switch to
			 * the next one on the next iteration.
			 */
			if (d2d_wrap_gt(node->ndesc.seq + node->ndesc.len,
					r_sack))
				pair_num++;

			/*
			 * Check that we are not deleting `first_to_send` node.
			 * Otherwise, if the next SACK is ill-formed,
			 * we will leave without resetting the node,
			 * so use-after-free will be possible.
			 */
			if (node == txb->first_to_send)
				txb->first_to_send = NULL;

			++freed;
			list_del(&node->list);
			tx_node_free(txb, node);

			iter = next;
			next = iter->next;
			continue;
		}
		/*
		 * The node is located on the right from [l_sack, r_sack].
		 * Switch to the next SACK pair without switching the node.
		 */
		pair_num++;
	}

	/*
	 * Rewind the `first_unsent` pointer to the packets not
	 * acknowledged by this SACK.
	 */
	tx_buf_reset_first_to_send(txb);

	return freed;
}

ssize_t tx_buf_process_ack(struct tx_buffer *txb, wrap_t ack_id,
			   const struct sack_pairs *sack)
{
	ssize_t ack_freed = 0;
	ssize_t sack_freed = 0;

	ack_freed = __tx_buf_free_ack(txb, ack_id);
	if (ack_freed < 0)
		return ack_freed;

	if (sack && sack->len) {
		sack_freed = __tx_buf_free_sack(txb, sack);
		if (sack_freed < 0)
			return sack_freed;
	}

	return ack_freed + sack_freed;
}

bool tx_buf_is_empty(const struct tx_buffer *txb)
{
	return list_empty(&txb->nodes);
}

void tx_buf_set_flowcontrol_mode(struct tx_buffer *txb, bool mode)
{
	txb->flowcontrol_mode = mode;
}

bool tx_buf_get_flowcontrol_mode(struct tx_buffer *txb)
{
	return txb->flowcontrol_mode;
}

size_t tx_buf_estimate_usage(const struct tx_buffer *txb, size_t len)
{
	size_t elem_size = txb->max_elem;
	size_t nr_nodes = DIV_ROUND_UP(len, elem_size);
	size_t usage = 0;

	/* add empty memseg to be sure that we estimated max usage for append */
	usage += sizeof(struct memseg);

	/* account the data length itself */
	usage += len;

	/* account the overhead for node/segment structures */
	usage += nr_nodes * (sizeof(struct memseg) + sizeof(struct tx_node));

	return usage;
}

size_t tx_buf_get_usage(const struct tx_buffer *txb)
{
	return txb->mem_usage;
}
