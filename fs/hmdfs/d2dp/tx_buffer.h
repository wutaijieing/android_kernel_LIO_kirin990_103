/* SPDX-License-Identifier: GPL-2.0 */
/*
 * D2D TX buffer internal structures
 */

#ifndef D2D_TX_BUFFER_H
#define D2D_TX_BUFFER_H

#include <linux/mm_types.h>
#include <linux/types.h>

#include "buffer.h"
#include "memory.h"
#include "wrappers.h"

enum tx_node_state {
	TX_NODE_STATE_UNSENT,
	TX_NODE_STATE_SENT,
	TX_NODE_STATE_NEED_RESEND,
};

struct tx_node {
	struct list_head list;
	struct list_head segs;
	struct ndesc ndesc;
	enum tx_node_state state;
};

struct tx_buffer {
	struct list_head nodes;
	struct tx_node *first_to_send;
	size_t max_elem;
	size_t mem_usage;
	node_alloc_f alloc;
	node_dealloc_f dealloc;
	struct kmem_cache *node_cache;
	gfp_t node_gfp;
	wrap_t tx_seq_id;
	bool flowcontrol_mode;
};

void tx_node_list_free(struct tx_buffer *txb, struct list_head *head);
struct tx_node *tx_node_alloc(struct tx_buffer *txb, size_t len);
void tx_buf_reset_first_to_send(struct tx_buffer *txb);

#endif /* D2D_TX_BUFFER_H */
