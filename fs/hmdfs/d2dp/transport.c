// SPDX-License-Identifier: GPL-2.0
/*
 * D2D transport implementation
 */

#define pr_fmt(fmt) "[D2DP]: " fmt

#include <linux/atomic.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/net.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/socket.h>
#include <linux/wait.h>

#include "d2d.h"
#include "buffer.h"
#include "headers.h"
#include "params.h"
#include "protocol.h"
#include "resources.h"
#include "sack.h"
#include "timers.h"
#include "transport.h"
#include "trace.h"
#include "wrappers.h"

static const struct cred *d2d_override_creds(struct d2d_protocol *proto)
{
	if (proto->security && proto->security->cred)
		return override_creds(proto->security->cred);

	return NULL;
}

static void d2d_revert_creds(const struct cred *creds)
{
	if (creds)
		revert_creds(creds);
}

static bool rx_buf_check_limit_exceeded(const struct rx_buffer *rxb)
{
	size_t fill_percentage = rx_buf_get_fill_percentage(rxb);

	return fill_percentage > D2D_BUFFER_ALLOWED_FILL_PERCENTAGE;
}

static bool prepare_ack(struct d2d_protocol *proto)
{
	struct tx_handler *tx = &proto->tx;
	struct rx_handler *rx = &proto->rx;
	struct d2d_header *ackhdr = NULL;
	struct ack_data *ack_data = NULL;
	struct sack_pairs pairs = { 0 };
	struct tx_ack_state *ack_state = &tx->ack_state;
	size_t sack_max     = 0;
	bool limit_exceeded = false;
	wrap_t seq_id = 0;

	/*
	 * Take two locks here - to protect the ack packet from data races and
	 * to serialize access to the RX buffer.
	 */
	mutex_lock(&tx->lock);

	/* Set pointer to ack that will be filled */
	ack_data = ack_state->ack_to_fill;
	ackhdr = &ack_data->ack_header;

	pairs.pairs = ackhdr->sack_pairs;
	pairs.len = 0;
	sack_max = ARRAY_SIZE(ackhdr->sack_pairs);

	/* Collect the acknowledgement information from RX buffer */
	mutex_lock(&rx->lock);
	limit_exceeded = rx_buf_check_limit_exceeded(rx->buffer);
	rx_buf_generate_sack_pairs(rx->buffer, &pairs, sack_max);
	seq_id = rx_buf_get_ack(rx->buffer);
	mutex_unlock(&rx->lock);

	/* Fill the ack header */
	ackhdr->flags = D2DF_ACK;
	ackhdr->seq_id = seq_id;
	ackhdr->data_len = pairs.len;
	if (limit_exceeded) {
		ackhdr->flags |= D2DF_SUSPEND;
		atomic_inc(&proto->stats.suspend_flag);
	}

	/* Update the ack state */
	ack_data->ack_ready = true;
	ack_state->last_acked = ackhdr->seq_id;

	mutex_unlock(&tx->lock);

	/* Reschedule itself if SACK generated */
	return pairs.len;
}

void ack_timer_action(void *arg)
{
	struct d2d_protocol *proto = arg;
	struct d2d_timer *self = &proto->timers.ack_timer;
	struct tx_handler *tx = &proto->tx;
	ktime_t t_start = 0;
	ktime_t t_end = 0;
	bool resched = false;

	t_start = ktime_get();
	resched = prepare_ack(proto);
	t_end = ktime_get();

	tx_thread_push(tx);

	atomic_inc(&proto->stats.ack_timer_fired);
	trace_d2dp_ack_timer(ktime_sub(t_end, t_start), 1);

	if (resched)
		d2d_timer_enable(self);
}

void rto_timer_action(void *arg)
{
	struct d2d_protocol *proto = arg;
	struct tx_handler *tx = &proto->tx;
	struct rto *rto = &proto->timers.rto;
	struct d2d_timer *timer = &rto->timer;
	ktime_t t_start = 0;
	ktime_t t_end = 0;
	int count = 0;

	if (atomic_read_acquire(&proto->exitcode) ||
	    atomic_read_acquire(&proto->closing))
		return;

	/*
	 * Deadlock prevention.
	 *
	 * In `rx_process_acknowledgement_locked` the RTO timer could be
	 * disabled. Sadly, it can be disabled when we got here, so locking the
	 * mutex is dangerous due to following deadlock:
	 *
	 * [rx_process_acknowledgement_locked]         [rto_timer_action]
	 * --------------------------------------------------------------
	 *        <tx.lock is locked>
	 *         tx_buf_process_ack
	 *      tx_buf_is_empty() == true
	 *                                                 <timer fired>
	 *                                                 take tx.lock
	 *         rto_timer_disable()                           |
	 *      cancel_delayed_work_sync()                       |
	 *                  |                                    |
	 *         <wait for rto timer>                  <wait for tx lock>
	 *                  >>>            [DEADLOCK]          <<<
	 *
	 * To avoid that, let's spin a little bit to detect a `disabling` flag
	 * which is set by `rto_timer_disable` to avoid exactly such situation.
	 * Spinning should not harm us very much for two reasons:
	 *
	 * 1. RTO is an unlikely event which happens in bad conditions mostly.
	 * 2. The amount of time the tx lock is taken is in range of 100 us
	 */
	while (!mutex_trylock(&tx->lock)) {
		if (atomic_read_acquire(&rto->disabling))
			return;

		/* release the CPU to not stall the full workqueue */
		schedule();
	}

	t_start = ktime_get();
	tx_buf_mark_as_need_resent(tx->buffer);
	/*
	 * Disabling flowcontrol forcefully to unlock TX buffer
	 * in case of SUSPENDing
	 */
	tx_buf_set_flowcontrol_mode(tx->buffer, false);
	mutex_unlock(&tx->lock);
	t_end = ktime_get();

	count = atomic_inc_return(&rto->count);
	if (count > D2D_RETRY_LIMIT_MAX) {
		pr_err("rto: link failure detected\n");
		d2dp_notify_all(proto, -EIO);
	}

	/* Wake up TX thread to resend marked packets */
	tx_thread_push(tx);

	atomic_inc(&proto->stats.rto_timer_fired);
	trace_d2dp_rto_timer(ktime_sub(t_end, t_start), count);

	/* Requeue itself */
	d2d_timer_enable(timer);
}

static void bw_est_update_datagram_size(struct d2d_protocol *proto,
					struct bw_estimator *estimator,
					uint64_t bandwidth)
{
	size_t new_dgram = 0;

	/* if manual mode is enabled, do not change datagram size */
	if (estimator->manual)
		return;

	if (bandwidth >= D2D_BWEST_HI_WATERLINE_MBPS)
		new_dgram = MAX_DATAGRAM_SIZE;
	else if (bandwidth < D2D_BWEST_LO_WATERLINE_MBPS)
		new_dgram = MIN_DATAGRAM_SIZE;
	else
		return;

	if (new_dgram != estimator->last_dgram) {
		d2d_set_datagram_size(proto, new_dgram);
		estimator->last_dgram = new_dgram;
	}
}

void bw_est_timer_action(void *arg)
{
	struct d2d_protocol *proto = arg;
	struct bw_estimator *estimator = &proto->timers.bw_est;
	struct d2d_timer *timer = &estimator->timer;

	ktime_t now_ktime      = 0;
	uint64_t now_tx_bytes  = 0;
	uint64_t now_rx_bytes  = 0;
	uint64_t now_trx_bytes = 0;
	uint64_t diff_usecs    = 0;
	uint64_t diff_bytes    = 0;
	uint64_t bandwidth     = 0;

	if (atomic_read_acquire(&proto->exitcode) ||
	    atomic_read_acquire(&proto->closing))
		return;

	atomic_inc(&proto->stats.bw_est_timer_fired);

	now_ktime     = ktime_get();
	now_tx_bytes  = atomic64_read(&proto->stats.tx_bytes);
	now_rx_bytes  = atomic64_read(&proto->stats.rx_bytes);
	now_trx_bytes = now_tx_bytes + now_rx_bytes;

	diff_bytes = now_trx_bytes - estimator->last_bytes;
	diff_usecs = ktime_us_delta(now_ktime, estimator->last_time);
	estimator->last_time  = now_ktime;
	estimator->last_bytes = now_trx_bytes;

	/* Paranoid check against zero division */
	if (WARN_ON(!diff_usecs))
		goto out;

	bandwidth = (diff_bytes * 8) / diff_usecs;
	if (bandwidth) {
		mutex_lock(&estimator->lock);
		bw_est_update_datagram_size(proto, estimator, bandwidth);
		mutex_unlock(&estimator->lock);
	}

out:
	/* Requeue itself if some transmission happened */
	if (diff_bytes)
		d2d_timer_enable(timer);
}

/**
 * sock_recv_wait_data - wait on socket until it got some data
 * @sock: underlying socket
 *
 * Blocks the calling thread until there is some data in the socket. The data
 * itself is not consumed and still present in the socket buffer, only the
 * length is returned.
 *
 * Returns the length of incoming data or negative error when socket fails.
 */
static int sock_recv_wait_data(struct socket *sock)
{
	char byte = 0;
	struct msghdr msg = { 0 };
	struct kvec iov[] = {
		{
			.iov_base = &byte,
			.iov_len = sizeof(byte),
		},
	};

	/* use `PEEK | TRUNC` to not consume data and to report real length */
	return d2dp_recvmsg(sock, &msg, iov, ARRAY_SIZE(iov), sizeof(byte),
			    MSG_PEEK | MSG_TRUNC);
}

/**
 * sock_recv_drop_packet - drop the last packet in the socket
 * @sock: underlying socket
 *
 * Consumes last accepted packet from the underlying socket buffer. The function
 * is non-blocking, and returns immediately even when there is no data in the
 * socket.
 *
 * Returns non-negative int when succeed or negative error when socket fails or
 * there is no data.
 */
static int sock_recv_drop_packet(struct socket *sock)
{
	char byte = 0;
	struct msghdr msg = { 0 };
	struct kvec iov[] = {
		{
			.iov_base = &byte,
			.iov_len = sizeof(byte),
		},
	};

	/* DGRAM socket is message-based, reading 1 byte consumes whole msg */
	return d2dp_recvmsg(sock, &msg, iov, ARRAY_SIZE(iov), sizeof(byte),
			    MSG_DONTWAIT);
}

/**
 * sock_recv_pull_packet - pull the last packet in the given buffer
 * @sock: underlying socket
 * @buffer: buffer to contain data
 * @buflen: buffer length
 *
 * Consumes last accepted packet from the underlying socket buffer and write the
 * data to the provided buffer. The function is non-blocking, and returns
 * immediately even when there is no data in the socket.
 *
 * Returns how many bytes have been written to @buffer when succeed or negative
 * error when socket fails or there is no data.
 */
static int sock_recv_pull_packet(struct socket *sock, char *buffer,
				 size_t buflen)
{
	struct msghdr msg = { 0 };
	struct kvec iov[] = {
		{
			.iov_base = buffer,
			.iov_len  = buflen,
		},
	};

	return d2dp_recvmsg(sock, &msg, iov, ARRAY_SIZE(iov), buflen,
			    MSG_DONTWAIT);
}

static int sock_send_packet(struct socket *sock, char *buffer, size_t buflen)
{
	struct msghdr msg = { 0 };
	struct kvec iov[] = {
		{
			.iov_base = buffer,
			.iov_len  = buflen,
		},
	};

	return d2dp_sendmsg(sock, &msg, iov, ARRAY_SIZE(iov), buflen);
}

static bool tx_wait_cond(struct d2d_protocol *proto, int *reason)
{
	int exitcode = 0;
	bool wakeup = false;
	struct tx_handler *tx = &proto->tx;

	exitcode = atomic_read_acquire(&proto->exitcode);
	if (unlikely(exitcode)) {
		pr_err("txwait: proto error: %d\n", exitcode);
		*reason = TX_WAKE_REASON_PROTO_ERROR;
		return true;
	}

	if (unlikely(atomic_read_acquire(&proto->closing))) {
		pr_info("txwait: closing\n");
		*reason = TX_WAKE_REASON_CLOSING;
		return true;
	}

	wakeup = atomic_xchg(&tx->has_work, 0);
	if (wakeup) {
		*reason = TX_WAKE_REASON_OK;
		atomic_inc(&proto->stats.tx_wakeups);
	} else {
		atomic_inc(&proto->stats.tx_sleeps);
	}

	return wakeup;
}

static int tx_wait_data(struct d2d_protocol *proto)
{
	int ret = 0;
	int reason = TX_WAKE_REASON_OK;

	ret = wait_event_interruptible(proto->tx.wq_data,
				       tx_wait_cond(proto, &reason));
	if (ret == -ERESTARTSYS && reason == TX_WAKE_REASON_OK) {
		pr_err("txwait: interrupted\n");
		reason = TX_WAKE_REASON_ERESTARTSYS;
	}

	return reason;
}

static int alloc_frame(void **frame, size_t len)
{
	*frame = d2dp_frame_kvmalloc(len, GFP_KERNEL);
	if (!*frame) {
		pr_err("Cannot alloc frame\n");
		return -ENOMEM;
	}
	return len;
}

static void dealloc_frame(void **frame)
{
	d2dp_kvfree(*frame);
	*frame = NULL;
}

static int tx_transport_encrypt(struct d2d_security *security,
				struct net_frames *frames,
				size_t length)
{
	int ret = length;

	if (security) {
		mutex_lock(&frames->crypto_tx_mtx);
		ret = d2dp_encrypt_frame(security, frames->tx_frame, length,
					 frames->tx_encr_frame,
					 frames->txlen);
		mutex_unlock(&frames->crypto_tx_mtx);
	}

	return ret;
}

static int tx_transport_send_ack(struct d2d_protocol *proto,
				 struct d2d_header *ackhdr)
{
	size_t hdrlen = 0;
	int to_send   = 0;
	void *sendbuf = NULL;
	struct net_frames *frames = &proto->frames;

	/* set packet number field */
	proto->tx_packet_id++;
	ackhdr->packet_id = proto->tx_packet_id;

	hdrlen = d2d_header_write(frames->tx_frame, frames->txlen, ackhdr);

	to_send = tx_transport_encrypt(proto->security, frames, hdrlen);
	if (to_send < 0) {
		pr_err("txloop: cannot encrypt ack packet: %d\n", to_send);
		return to_send;
	}

	sendbuf = (proto->security) ? frames->tx_encr_frame : frames->tx_frame;

	atomic64_inc(&proto->stats.tx_packets);
	atomic64_add(to_send, &proto->stats.tx_bytes);

	return sock_send_packet(proto->sock, sendbuf, to_send);
}

static int tx_transport_send_data(struct d2d_protocol *proto,
				  const struct ndesc *desc,
				  bool resend)
{
	size_t pktlen = 0;
	int to_send   = 0;
	void *sendbuf = NULL;
	size_t hdrlen = d2d_get_header_packed_size();
	struct tx_handler *tx     = &proto->tx;
	struct d2d_header *hdr    = &tx->tx_header;
	struct net_frames *frames = &proto->frames;

	pktlen = desc->len;

	/* Prepare the packet header first */
	hdr->flags    = D2DF_NONE;
	hdr->data_len = desc->len;
	hdr->seq_id   = desc->seq;

	/* set packet number field */
	proto->tx_packet_id++;
	hdr->packet_id = proto->tx_packet_id;

	/* append the header to the frame */
	hdrlen = d2d_header_write(frames->tx_frame, hdrlen, hdr);

	to_send = tx_transport_encrypt(proto->security,
				       frames, hdrlen + pktlen);
	if (to_send < 0) {
		pr_err("txloop: cannot encrypt data packet: %d\n", to_send);
		return to_send;
	}

	sendbuf = (proto->security) ? frames->tx_encr_frame : frames->tx_frame;

	atomic64_inc(&proto->stats.tx_packets);
	atomic64_add(to_send, &proto->stats.tx_bytes);
	if (resend)
		atomic_add(to_send, &proto->stats.tx_resend_bytes);

	return sock_send_packet(proto->sock, sendbuf, to_send);
}

static void tx_switch_ack_pointer(struct tx_ack_state *ack_state,
				  struct ack_data **ack_to_switch)
{
	*ack_to_switch = (*ack_to_switch == &ack_state->acks[0]) ?
		&ack_state->acks[1] :
		&ack_state->acks[0];
}

static void tx_switch_ack_to_fill(struct tx_ack_state *ack_state)
{
	tx_switch_ack_pointer(ack_state, &ack_state->ack_to_fill);
}

static void tx_switch_ack_to_send(struct tx_ack_state *ack_state)
{
	tx_switch_ack_pointer(ack_state, &ack_state->ack_to_send);
}

/**
 * tx_transport_try_ack_locked - try to send ACK packet if ready
 * @proto: D2DP instance
 *
 * This function is called with TX lock being held. The function itself
 * temporary releases the lock to send the ACK packet (if it is ready), but then
 * it re-aquires it to restore invariant and flush TX ack state. Note that the
 * lock is released when the sending operation fails.
 *
 * Returns:
 * 0               - when no acknowledgement has been sent (ACK is not ready)
 * positive number - number of bytes sent (ACK was ready)
 * negative error  - socket operation failed (lock is released in this case)
 */
static int tx_transport_try_ack_locked(struct d2d_protocol *proto)
{
	int ret = 0;
	struct tx_handler *tx = &proto->tx;
	struct tx_ack_state *ack_state = &tx->ack_state;
	struct d2d_header *ackhdr = NULL;

	if (ack_state->ack_to_send->ack_ready) {
		/*
		 * Move ack_to_fill pointer to fill another ack while this one
		 * is in sending state
		 */
		tx_switch_ack_to_fill(ack_state);
		ackhdr = &ack_state->ack_to_send->ack_header;

		/*
		 * Send the acknowledgement without holding the lock. Our header
		 * is protected because ack_to_fill pointer has been switched.
		 */
		mutex_unlock(&tx->lock);

		ret = tx_transport_send_ack(proto, ackhdr);
		if (ret < 0) {
			pr_err("txloop: cannot send ack: %d\n", ret);
			return ret;
		}

		trace_d2dp_ack_send(ackhdr);

		/* take the lock back to hold invariant and flush ack state */
		mutex_lock(&tx->lock);
		ack_state->ack_to_send->ack_ready = false;
		tx_switch_ack_to_send(ack_state);
	}

	return ret;
}

/**
 * tx_transport_run_workcycle - perform the TX transport activity
 * @proto: D2DP instance
 *
 * This function tries to send all the UNSENT/NEED_RESEND packets from TX buffer
 * and send ACKs when it is ready.
 *
 * Returns: amount of DATA (not ACK) packets sent or negative error.
 */
static int tx_transport_run_workcycle(struct d2d_protocol *proto)
{
	int ret = 0;
	struct tx_handler *tx = &proto->tx;
	size_t hdrlen = d2d_get_header_packed_size();
	enum tx_buf_get_packet_result res = TX_BUF_GET_NO_PACKETS;
	struct net_frames *frames = &proto->frames;

	mutex_lock(&tx->lock);

	while (true) {
		struct ndesc desc = { 0 };

		/* try ACK first to guarantee at least one ACK in whole call */
		ret = tx_transport_try_ack_locked(proto);
		if (ret < 0)
			return ret;

		/* check whether there is a packet here to send */
		res = tx_buf_get_packet(tx->buffer, &desc,
					frames->tx_frame + hdrlen,
					frames->txlen - hdrlen);
		if (res != TX_BUF_GET_NO_PACKETS) {
			bool resend = res == TX_BUF_GET_RESEND;

			/* start RTO timer before sending the packet */
			d2d_timer_enable(&proto->timers.rto.timer);

			/* send the packet without holding the lock */
			mutex_unlock(&tx->lock);

			ret = tx_transport_send_data(proto, &desc, resend);
			if (ret < 0) {
				pr_err("txloop: cannot send data: %d\n", ret);
				return ret;
			}

			mutex_lock(&tx->lock);
		} else {
			if (!atomic_xchg(&tx->has_work, 0))
				break;
		}
	}

	mutex_unlock(&tx->lock);
	return 0;
}

static int tx_preallocate_frames(struct d2d_protocol *proto)
{
	int ret = 0;
	struct net_frames *frames = &proto->frames;

	ret = alloc_frame(&frames->tx_frame, frames->txlen);
	if (ret < 0)
		goto fail;

	if (proto->security) {
		ret = alloc_frame(&frames->tx_encr_frame, frames->txlen);
		if (ret < 0)
			goto free_tx_frame;
	}

	return 0;

free_tx_frame:
	dealloc_frame(&frames->tx_frame);
fail:
	return ret;
}

static void tx_deallocate_frames(struct d2d_protocol *proto)
{
	struct net_frames *frames = &proto->frames;

	dealloc_frame(&frames->tx_frame);
	if (proto->security)
		dealloc_frame(&frames->tx_encr_frame);
}

static int tx_transport_loop(struct d2d_protocol *proto)
{
	int ret = 0;

	while (true) {
		ret = tx_wait_data(proto);
		switch (ret) {
		case TX_WAKE_REASON_OK:
			break;
		case TX_WAKE_REASON_PROTO_ERROR:
			return atomic_read_acquire(&proto->exitcode);
		case TX_WAKE_REASON_ERESTARTSYS:
			return -ERESTARTSYS;
		case TX_WAKE_REASON_CLOSING:
			return 0;
		default:
			WARN(true, "Invalid tx wake reason: %d\n", ret);
			return -EINVAL;
		}

		/* allocate frames before enter workcycle */
		ret = tx_preallocate_frames(proto);
		if (ret < 0)
			return ret;

		ret = tx_transport_run_workcycle(proto);
		if (ret < 0)
			return ret;

		/* deallocate frames after workcycle completion */
		tx_deallocate_frames(proto);
	}
}

int tx_transport_thread(void *arg)
{
	int err = 0;
	struct d2d_protocol *proto = arg;
	const struct cred *creds = d2d_override_creds(proto);

	/* allow the signal as TX thread can hang up in `send` call */
	allow_signal(SIGTERM);
	complete(&proto->transport.tx_init);

	err = tx_transport_loop(proto);
	pr_info("txloop: exited with %d\n", err);

	d2dp_notify_all(proto, err);
	d2d_revert_creds(creds);
	return 0;
}

static void rto_timer_disable(struct rto *rto)
{
	/*
	 * Deadlock prevention.
	 *
	 * Straightforward code will deadlock here, as `rto_timer_disable` is
	 * called with `txlock` being held and we have to keep the lock to save
	 * the RTO timer consistency. However, the underlying kernel function
	 * `cancel_delayed_work_sync` will wait for the timer to complete, and
	 * the `rto_timer_action` body also tries to take `txlock`, which can
	 * lead to a dealock.
	 *
	 * To avoid that, set special flag to let the RTO timer know about that.
	 * See also the corresponding workaround in `rto_timer_action`.
	 */
	atomic_set_release(&rto->disabling, 1);
	d2d_timer_disable(&rto->timer);
	atomic_set_release(&rto->disabling, 0);
	atomic_set_release(&rto->count, 0);
}

static void rto_timer_reset(struct rto *rto)
{
	d2d_timer_reset(&rto->timer);
	atomic_set_release(&rto->count, 0);
}

struct process_ack_result {
	size_t freed;
	bool key_update_acked;
	bool flowctl_disabled;
};

static ssize_t
rx_process_acknowledgement_locked(struct d2d_protocol *proto,
				  const struct d2d_header *header,
				  struct process_ack_result *result)
{
	struct tx_handler *tx = &proto->tx;
	ssize_t acked = 0;
	size_t old_usage = 0;
	size_t new_usage = 0;
	u32 ack_id = header->seq_id;
	bool flowctl_curr = false;
	bool flowctl_prev = false;
	bool key_update_acked = false;
	const struct sack_pairs sacks = {
		.pairs = (struct sack_pair *) header->sack_pairs,
		.len   = header->data_len,
	};

	old_usage = tx_buf_get_usage(tx->buffer);

	acked = tx_buf_process_ack(tx->buffer, ack_id, &sacks);
	if (acked < 0)
		return acked;

	new_usage = tx_buf_get_usage(tx->buffer);

	if (tx_buf_is_empty(tx->buffer))
		rto_timer_disable(&proto->timers.rto);
	else if (acked)
		rto_timer_reset(&proto->timers.rto);

	flowctl_prev = tx_buf_get_flowcontrol_mode(tx->buffer);
	/* Check flowcontrol flag and set TX buffer mode */
	flowctl_curr = header->flags & D2DF_SUSPEND;
	tx_buf_set_flowcontrol_mode(tx->buffer, flowctl_curr);

	/*
	 * If we block sending due to received SUSPEND flag we should
	 * be able to start sending regardless remote side behavior.
	 * When RTO timer is fired flowcontrol mode will be forcefully disabled.
	 */
	if (flowctl_curr)
		d2d_timer_enable(&proto->timers.rto.timer);

	/* check whether TX thread waits for KEY_UPDATE acknowledgement */
	if (atomic_read_acquire(&tx->key_update)) {
		if (d2d_wrap_ge(ack_id, tx->key_update_id)) {
			atomic_set_release(&tx->key_update, 0);
			key_update_acked = true;
		}
	}

	result->freed = (new_usage < old_usage) ? (old_usage - new_usage) : 0;
	result->key_update_acked = key_update_acked;
	result->flowctl_disabled = flowctl_prev && !flowctl_curr;
	return acked;
}

static int rx_process_acknowledgement(struct d2d_protocol *proto,
				      const struct d2d_header *header)
{
	ssize_t acked = 0;
	struct tx_handler *tx = &proto->tx;
	struct process_ack_result result = { 0 };

	trace_d2dp_ack_recv(header);

	mutex_lock(&tx->lock);
	acked = rx_process_acknowledgement_locked(proto, header, &result);
	mutex_unlock(&tx->lock);

	if (acked < 0)
		return acked;

	if (result.freed > 0) {
		atomic_add_return_release(result.freed, &tx->bytes);
		wake_up_interruptible_all(&tx->wq_free);
	}

	if (header->data_len)
		atomic_inc(&proto->stats.rx_sacks_arrived);

	/*
	 * We have to wake up TX thread in next situations:
	 * 1. This ACK contains some SACK pairs, but TX thread may be
	 *    sleeping, wake it to resend missing fragments.
	 * 2. If flow control mode changed its state from ON to OFF
	 *    TX buffer might contain some unsent yet packets.
	 * 3. If key update request was acknowledged.
	 */
	if (header->data_len ||
	    result.flowctl_disabled ||
	    result.key_update_acked)
		 tx_thread_push(tx);

	return 0;
}

static int rx_process_data(struct d2d_protocol *proto,
			    const struct d2d_header *header,
			    const void *data)
{
	const struct ndesc desc = {
		.len = header->data_len,
		.seq = header->seq_id,
	};
	struct rx_handler *rx = &proto->rx;
	enum rx_buf_put_result res = RX_BUF_PUT_OK;
	size_t old_ready = 0;
	size_t new_ready = 0;
	size_t diff_ready = 0;

	mutex_lock(&rx->lock);

	old_ready = rx_buf_get_available_bytes(rx->buffer);
	res = rx_buf_put(rx->buffer, &desc, data);
	new_ready = rx_buf_get_available_bytes(rx->buffer);

	if (res == RX_BUF_PUT_OK) {
		diff_ready = new_ready - old_ready;
		if (diff_ready)
			/* ensure `rx_buf_put` is completed before updating */
			atomic_add_return_release(diff_ready, &rx->seqbytes);
	}
	mutex_unlock(&rx->lock);

	d2d_timer_enable(&proto->timers.ack_timer);

	switch (res) {
	case RX_BUF_PUT_OK:
		/* wake up only if packet is ready to be read */
		if (diff_ready)
			wake_up_interruptible_all(&rx->wq_data);

		break;
	case RX_BUF_PUT_CONSUMED:
	case RX_BUF_PUT_DUPLICATE:
		atomic_inc(&proto->stats.rx_dups_arrived);
		break;
	case RX_BUF_PUT_OVERFLOW:
		/* no free space in the RX buffer - drop the packet */
		atomic_inc(&proto->stats.rx_overflow_arrived);
		break;
	case RX_BUF_PUT_NOMEM:
		/* node allocation failed */
		return -ENOMEM;
	default:
		/* buggy or malicious packet - better safe than sorry */
		WARN(true, "bad D2DP packet, err=%d: len=%u, seq=%u\n",
		     res, desc.len, desc.seq);
		return -EINVAL;
	}

	return 0;
}

static int rx_process_packet(struct d2d_protocol *proto,
			     struct d2d_header *header,
			     const void *data)
{
	if (header->flags & D2DF_ACK)
		return rx_process_acknowledgement(proto, header);
	else
		return rx_process_data(proto, header, data);
}

static int rx_transport_sock_err(const struct d2d_protocol *proto,
				 int err, int *result)
{
	if (err == -ERESTARTSYS) {
		if (atomic_read_acquire(&proto->closing)) {
			pr_info("rxloop: signal for closing\n");
			return RX_WAKE_REASON_CLOSING;
		}

		pr_err("rxloop: interrupted\n");
		return RX_WAKE_REASON_ERESTARTSYS;
	}

	/* other protocol errors (EAGAIN is also an error for us) */
	*result = err;
	pr_err("rxloop: socket error: %d\n", err);
	return RX_WAKE_REASON_PROTO_ERROR;
}

static int rx_transport_recv_wait_data(struct d2d_protocol *proto, int *result)
{
	int ret = 0;
	int min_valid_data_len = d2d_get_header_packed_size();

	if (proto->security)
		min_valid_data_len += proto->security->cr_overhead;

	/* waiting for some data in socket */
	ret = sock_recv_wait_data(proto->sock);
	if (unlikely(ret < 0))
		return rx_transport_sock_err(proto, ret, result);

	/*
	 * Check this before ret == 0 check, otherwise we can hung forever when
	 * the socket is `shutdown`ed
	 */
	if (unlikely(atomic_read_acquire(&proto->closing))) {
		pr_info("rxloop: closing\n");
		return RX_WAKE_REASON_CLOSING;
	}

	if (WARN_ON(ret > U16_MAX)) {
		/* We got a UDP datagram longer than U16MAX, wow. */
		pr_err("UDP socket returned impossible datagram, check it\n");
		return rx_transport_sock_err(proto, -EBADMSG, result);
	}

	if (unlikely(ret >= 0 && ret < min_valid_data_len)) {
		/* this datagram is too short for D2DP to be correct */
		ret = sock_recv_drop_packet(proto->sock);
		if (ret < 0)
			return rx_transport_sock_err(proto, ret, result);

		atomic_inc(&proto->stats.rx_drops_short);
		return RX_WAKE_REASON_SHORT_PACKET;
	}

	/* we have some adequate data in the socket */
	*result = ret;
	return RX_WAKE_REASON_OK;
}

static int rx_transport_recv(struct d2d_protocol *proto, int *result)
{
	int ret = 0;
	void *recvbuf = NULL;
	struct net_frames *frames = &proto->frames;
	size_t bytes = 0;

	ret = rx_transport_recv_wait_data(proto, result);
	if (ret != RX_WAKE_REASON_OK)
		return ret;

	/* there is some OK data in the socket, allocate buffer for that */
	bytes = *result;
	if (alloc_frame(&recvbuf, bytes) < 0)
		return rx_transport_sock_err(proto, -ENOMEM, result);

	/* choose the right buffer to store input data from the network */
	if (proto->security)
		frames->rx_encr_frame = recvbuf;
	else
		frames->rx_frame = recvbuf;

	/* receive data from socket */
	ret = sock_recv_pull_packet(proto->sock, recvbuf, bytes);
	if (unlikely(ret < 0))
		return rx_transport_sock_err(proto, ret, result);

	atomic64_inc(&proto->stats.rx_packets);
	atomic64_add(bytes, &proto->stats.rx_bytes);

	*result = ret;
	return RX_WAKE_REASON_OK;
}

static int rx_transport_decrypt(struct d2d_security *security,
				struct net_frames *frames,
				size_t pktlen)
{
	int ret = pktlen;

	if (security) {
		int res = 0;
		size_t decr_len = pktlen - security->cr_overhead;

		res = alloc_frame(&frames->rx_frame, decr_len);
		if (res < 0)
			return -ENOMEM;

		mutex_lock(&frames->crypto_rx_mtx);
		ret = d2dp_decrypt_frame(security,
					 frames->rx_encr_frame, pktlen,
					 frames->rx_frame, decr_len);
		mutex_unlock(&frames->crypto_rx_mtx);

		dealloc_frame(&frames->rx_encr_frame);
	}

	return ret;
}

static int rx_transport_do_work(struct d2d_protocol *proto, int data_len)
{
	int ret = 0;
	size_t hdrlen = 0;
	size_t pktlen = 0;
	struct rx_handler *rx = &proto->rx;
	struct net_frames *frames = &proto->frames;
	u64 left_edge = 0;

	ret = rx_transport_decrypt(proto->security, frames, data_len);
	if (unlikely(ret < 0)) {
		atomic_inc(&proto->stats.rx_drops_decrypt);
		/* EBADMSG may be not an error when updating keys */
		if (ret == -EBADMSG) {
			/* force ack to not deadlock when updating keys */
			d2d_timer_enable(&proto->timers.ack_timer);
			return 0;
		}
		/* interpret any other code as an error */
		pr_err("decrypt error happened: %d\n", ret);
		return ret;
	}

	pktlen = ret;
	hdrlen = d2d_header_read(frames->rx_frame, pktlen, &rx->rx_header);
	if (!hdrlen) {
		atomic_inc(&proto->stats.rx_drops_badhdr);
		return 0;
	}

	/*
	 * Replay attack protection. Ignore outdated packets as they may
	 * be re-sent by malicious users by eavesdropping the air.
	 */
	left_edge = rx->rx_header.packet_id + D2D_PACKET_ID_RCV_WINDOW;
	if (left_edge <= proto->rx_packet_id) {
		atomic_inc(&proto->stats.rx_drops_pktid);
		return 0;
	}

	/* Count gaps in packet order */
	if (rx->rx_header.packet_id > proto->rx_packet_id + 1) {
		u64 old = proto->rx_packet_id;
		u64 new = rx->rx_header.packet_id;
		u64 gap = new - old - 1;

		atomic_add(gap, &proto->stats.rx_gaps_arrived);
	}

	proto->rx_packet_id = rx->rx_header.packet_id;
	return rx_process_packet(proto, &rx->rx_header,
				 frames->rx_frame + hdrlen);
}

static int rx_transport_loop(struct d2d_protocol *proto)
{
	int reason = 0;
	int result = 0;
	struct net_frames *frames = &proto->frames;

	while (true) {
		reason = rx_transport_recv(proto, &result);
		switch (reason) {
		case RX_WAKE_REASON_OK:
			break;
		case RX_WAKE_REASON_SHORT_PACKET:
			continue;
		case RX_WAKE_REASON_PROTO_ERROR:
			/* the error code is stored in result */
			return result;
		case RX_WAKE_REASON_CLOSING:
			return 0;
		case RX_WAKE_REASON_ERESTARTSYS:
			return -ERESTARTSYS;
		default:
			WARN(true, "Invalid rx wake reason: %d\n", reason);
			return -EINVAL;
		}

		result = rx_transport_do_work(proto, result);
		if (result < 0)
			return result;

		dealloc_frame(&frames->rx_frame);
	}

	return 0;
}

int rx_transport_thread(void *arg)
{
	int err = 0;
	struct d2d_protocol *proto = arg;
	const struct cred *creds = d2d_override_creds(proto);

        /*
         * Allow the signal to get the possibility to interrupt `recv` call and
	 * fill the completion _after_ subscribing to SIGTERM. Otherwise, the
	 * signal may be lost in `create`/`close` race condidions.
	 */
	allow_signal(SIGTERM);
	complete(&proto->transport.rx_init);

	err = rx_transport_loop(proto);
	pr_info("rxloop: exited with %d\n", err);

	d2dp_notify_all(proto, err);
	d2d_revert_creds(creds);
	return 0;
}
