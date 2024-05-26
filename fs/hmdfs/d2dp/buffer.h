/* SPDX-License-Identifier: GPL-2.0 */
/*
 * D2D TX/RX buffer interface
 */

#ifndef D2D_BUFFER_H
#define D2D_BUFFER_H

#include <linux/types.h>
#include <linux/net.h>

#include "sack.h"
#include "wrappers.h"

/* forward declarations */
struct tx_buffer;
struct rx_buffer;

/**
 * struct ndesc - D2DP node descriptor
 * @len: D2DP message length
 * @seq: sequence id for this D2DP message
 */
struct ndesc {
	u32    len;
	wrap_t seq;
};

/* Function pointers to customize node allocations from the calling code. */
typedef void *(*node_alloc_f)(size_t);
typedef void (*node_dealloc_f)(void *);

/* TX operations */
struct tx_buffer *tx_buf_alloc(size_t max_elem, gfp_t gfp,
			       node_alloc_f alloc,
			       node_dealloc_f dealloc);
void tx_buf_free(struct tx_buffer *txb);
ssize_t tx_buf_append_iov(struct tx_buffer *txb, const struct kvec *iov,
			  size_t iovlen, size_t to_append);
bool tx_buf_get_last_seq_id(const struct tx_buffer *txb, wrap_t *res);

/**
 * enum tx_buf_get_packet_result - the result of `tx_buf_put_packet` operation
 * @NEW: this is a new packet to send
 * @RESEND: this is a packet which needs to be re-send
 * @NO_PACKETS: there are no packets ready to send
 */
enum tx_buf_get_packet_result {
	TX_BUF_GET_NEW,
	TX_BUF_GET_RESEND,
	TX_BUF_GET_NO_PACKETS,
};

/**
 * tx_buf_get_packet - try to get a packet to send from TX buffer
 * @txb: TX buffer instance
 * @desc: out node descriptor
 * @data: out data buffer for the packet
 * @size: data buffer length
 */
enum tx_buf_get_packet_result tx_buf_get_packet(struct tx_buffer *txb,
						struct ndesc *desc,
						void *data, size_t size);

ssize_t tx_buf_process_ack(struct tx_buffer *txb, wrap_t ack_id,
			   const struct sack_pairs *pairs);
void tx_buf_mark_as_need_resent(struct tx_buffer *txb);
bool tx_buf_is_empty(const struct tx_buffer *txb);
void tx_buf_set_flowcontrol_mode(struct tx_buffer *txb, bool mode);
bool tx_buf_get_flowcontrol_mode(struct tx_buffer *txb);
size_t tx_buf_estimate_usage(const struct tx_buffer *txb, size_t len);
size_t tx_buf_get_usage(const struct tx_buffer *txb);
size_t tx_buf_get_max_elem(const struct tx_buffer *txb);
void tx_buf_set_max_elem(struct tx_buffer *txb, size_t max_elem);

/* RX operations */
struct rx_buffer *rx_buf_alloc(size_t bufsize,
			       size_t waitall_limit,
			       node_alloc_f alloc,
			       node_dealloc_f dealloc);
void rx_buf_free(struct rx_buffer *rxb);
size_t rx_buf_get_available_bytes(const struct rx_buffer *rxb);
size_t rx_buf_get(struct rx_buffer *rxb, void *data, size_t len);

/**
 * enum rx_buf_put_result - the result of `rx_buf_put` operation
 * @OK: the packet has been successfully inserted to RXB
 * @OVERFLOW: there are no free space in RXB to place the new packet
 * @CONSUMED: the reader has already consumed the data with this seq_id
 * @DUPLICATE: there is exactly the same packet in the RXB already
 * @BAD_PACKET: this packet is malformed or the RX buffer is corrupted
 * @NOMEM: operation failed due to lack of memory
 * @INTERNAL_ERROR: internal error in RX buffer, close immediately
 */
enum rx_buf_put_result {
	RX_BUF_PUT_OK,
	RX_BUF_PUT_OVERFLOW,
	RX_BUF_PUT_CONSUMED,
	RX_BUF_PUT_DUPLICATE,
	RX_BUF_PUT_BAD_PACKET,
	RX_BUF_PUT_NOMEM,
	RX_BUF_PUT_INTERNAL_ERROR,
};

/**
 * rx_buf_put - try to put the packet in RX buffer
 * @rxb: RX buffer instance
 * @desc: D2DP node descriptor
 * @data: buffer with data to insert
 */
enum rx_buf_put_result rx_buf_put(struct rx_buffer *rxb,
				  const struct ndesc *desc,
				  const void *data);

wrap_t rx_buf_get_ack(const struct rx_buffer *rxb);
void rx_buf_generate_sack_pairs(const struct rx_buffer *rxb,
				struct sack_pairs *pairs, size_t len);
size_t rx_buf_get_fill_percentage(const struct rx_buffer *rxb);
size_t rx_buf_get_usage(const struct rx_buffer *rxb);

#endif /* D2D_BUFFER_H */
