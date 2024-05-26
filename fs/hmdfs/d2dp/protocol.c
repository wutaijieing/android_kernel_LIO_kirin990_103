// SPDX-License-Identifier: GPL-2.0
/* D2D protocol base implementation */

#define pr_fmt(fmt) "[D2DP]: " fmt

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <linux/atomic.h>
#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/if_ether.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "d2d.h"
#include "buffer.h"
#include "headers.h"
#include "kobject.h"
#include "options.h"
#include "params.h"
#include "protocol.h"
#include "resources.h"
#include "timers.h"
#include "transport.h"

/**
 * d2d_get_version - get the numeric value for current D2DP implementation
 *
 * Protocol versions:
 * 0: initial version with fixed datagram size (1472B)
 * 1: improved version with variable datagram size (up to 65535 B)
 */
unsigned int d2d_get_version(void)
{
	return D2DP_V1_VAR_DGRAMS;
}

/**
 * d2d_get_current_version - get the numeric value for D2DP version used
 *                           to communicate with remote side
 */
unsigned int d2d_get_current_version(const struct d2d_protocol *proto)
{
	return min(d2d_get_version(), proto->remote_version);
}

static void d2dp_wake_up_all(struct d2d_protocol *proto)
{
	wake_up_interruptible_all(&proto->tx.wq_free);
	wake_up_interruptible_all(&proto->tx.wq_data);
	wake_up_interruptible_all(&proto->rx.wq_data);
}

/**
 * d2dp_notify_all - set the result code and notify all waiters
 * @proto: D2D Protocol instance
 * @code: the error code to set
 */
void d2dp_notify_all(struct d2d_protocol *proto, int code)
{
	atomic_cmpxchg(&proto->exitcode, 0, code);
	d2dp_wake_up_all(proto);
}

/**
 * crypto_check_packet_size - which packet size we can use with encryption
 * @security: security structure instance from the upper layer
 * @datagram_size: full datagram size, upper limit
 *
 * Returns new payload length available for data with encryption overhead in
 * mind or 0 on error.
 */
static size_t crypto_calc_payload_size(const struct d2d_security *security,
				       size_t datagram_size)
{
	size_t payload = 0;
	size_t header = 0;

	payload = datagram_size;
	header = d2d_get_header_packed_size();
	/*
	 * This condition may only be true if MIN_DATAGRAM_SIZE is less than
	 * size of header
	 */
	if (WARN_ON(payload <= header)) {
		pr_err("datagram size (%zuB) is less than header (%zuB)\n",
		       datagram_size, header);
		return 0;
	}

	payload -= header;

	if (security) {
		/* crypto primitives must be implemented */
		if (!security->encrypt || !security->decrypt) {
			pr_err("security is not null, but no implementation\n");
			return 0;
		}

		/* crypto overhead must be less than available packet size */
		if (payload <= security->cr_overhead) {
			pr_err("crypto overhead is too big: %zu vs %zu\n",
			       security->cr_overhead, payload);
			return 0;
		}

		payload -= security->cr_overhead;
	}

	return payload;
}

/*
 * Use kvmalloc/kvfree as we don't need contiguous memory in the protocol
 */
static void *default_node_alloc(size_t size)
{
	return d2dp_node_kvmalloc(size, GFP_KERNEL);
}

static void default_node_dealloc(void *addr)
{
	d2dp_kvfree(addr);
}

static
int init_tx_handler(struct tx_handler *tx, size_t bufsize, size_t packet_size)
{
	struct tx_buffer *tx_buf = NULL;

	tx_buf = tx_buf_alloc(packet_size, GFP_KERNEL,
			      default_node_alloc, default_node_dealloc);
	if (!tx_buf) {
		pr_err("cannot allocate TX buffer for D2DP\n");
		return -ENOMEM;
	}

	memset(tx, 0, sizeof(*tx));

	mutex_init(&tx->lock);
	tx->buffer = tx_buf;
	tx->max_bufsize = bufsize;
	atomic_set(&tx->bytes, tx->max_bufsize);
	init_waitqueue_head(&tx->wq_free);
	init_waitqueue_head(&tx->wq_data);
	tx->ack_state.ack_to_send = tx->ack_state.acks;
	tx->ack_state.ack_to_fill = tx->ack_state.acks;

	return 0;
}

static void deinit_tx_handler(struct tx_handler *tx)
{
	tx_buf_free(tx->buffer);
}

static int init_rx_handler(struct rx_handler *rx, size_t bufsize)
{
	struct rx_buffer *rx_buf = NULL;

	rx_buf = rx_buf_alloc(bufsize, D2D_RECV_WAITALL_LIMIT,
			      default_node_alloc, default_node_dealloc);
	if (!rx_buf) {
		pr_err("cannot allocate RX buffer for D2DP\n");
		return -ENOMEM;
	}

	memset(rx, 0, sizeof(*rx));

	mutex_init(&rx->lock);
	rx->buffer = rx_buf;
	rx->max_bufsize = bufsize;
	init_waitqueue_head(&rx->wq_data);

	return 0;
}

static void deinit_rx_handler(struct rx_handler *rx)
{
	rx_buf_free(rx->buffer);
}

/**
 * init_frames - helper function to init data buffers.
 */
static void init_frames(struct net_frames *frames, size_t txlen)
{
	frames->tx_frame      = NULL;
	frames->rx_frame      = NULL;
	frames->tx_encr_frame = NULL;
	frames->rx_encr_frame = NULL;
	frames->txlen         = txlen;
	mutex_init(&frames->crypto_tx_mtx);
	mutex_init(&frames->crypto_rx_mtx);
}

static void deallocate_frames(struct net_frames *frames)
{
	kvfree(frames->tx_frame);
	kvfree(frames->rx_frame);
	kvfree(frames->tx_encr_frame);
	kvfree(frames->rx_encr_frame);
}

static int init_proto_timers(struct d2d_protocol *proto)
{
	struct timers *timers = &proto->timers;

	timers->timer_wq = d2d_workqueue_create("d2dp_timer_wq");
	if (!timers->timer_wq) {
		pr_err("cannot allocate timer workqueue for D2DP\n");
		return -ENOMEM;
	}

	d2d_timer_init(timers->timer_wq, &timers->ack_timer,
		       D2D_ACK_PERIOD_MSEC, ack_timer_action, proto);
	d2d_timer_init(timers->timer_wq, &timers->rto.timer,
		       D2D_RTO_PERIOD_MSEC, rto_timer_action, proto);

	if (proto->remote_version >= D2DP_V1_VAR_DGRAMS) {
		/* starting with version 1, D2DP uses adaptive datagram size */
		timers->bw_est.last_time = ktime_get();
		mutex_init(&timers->bw_est.lock);
		d2d_timer_init(timers->timer_wq, &timers->bw_est.timer,
			       D2D_BWEST_PERIOD_MSEC, bw_est_timer_action,
			       proto);
	}

	return 0;
}

static void deinit_proto_timers(struct d2d_protocol *proto)
{
	struct timers *timers = &proto->timers;

	d2d_timer_disable(&timers->ack_timer);
	d2d_timer_disable(&timers->rto.timer);
	if (proto->remote_version >= D2DP_V1_VAR_DGRAMS)
		d2d_timer_disable(&timers->bw_est.timer);

	d2d_workqueue_destroy(timers->timer_wq);
}

static int create_transport_threads(struct d2d_protocol *proto)
{
	int err = 0;
	struct task_struct *tx = NULL;
	struct task_struct *rx = NULL;

	init_completion(&proto->transport.tx_init);
	init_completion(&proto->transport.rx_init);

	tx = d2dp_kthread_create(tx_transport_thread, proto, "d2dp_tx_thread");
	if (IS_ERR(tx)) {
		err = PTR_ERR(tx);
		pr_err("cannot launch TX thread for D2DP: %d\n", err);
		goto fail;
	}

	rx = d2dp_kthread_create(rx_transport_thread, proto, "d2dp_rx_thread");
	if (IS_ERR(rx)) {
		err = PTR_ERR(rx);
		pr_err("cannot launch RX thread for D2DP: %d\n", err);
		goto free_tx_thread;
	}

	/*
	 * Take the additional reference before waking up the transport threads.
	 * Threads may fail early due to network errors, so to not make things
	 * complex, just increase refcount to make objects valid all the time.
	 */
	get_task_struct(tx);
	get_task_struct(rx);

	/* assign the tasks pointers before waking the tasks up */
	proto->transport.tx_task = tx;
	proto->transport.rx_task = rx;

	wake_up_process(tx);
	wake_up_process(rx);

	/* Make sure that threads are ready before returning to the caller */
	wait_for_completion(&proto->transport.tx_init);
	wait_for_completion(&proto->transport.rx_init);

	return 0;

free_tx_thread:
	/* as TX thread has not been launched yet, we can safely stop it */
	d2dp_kthread_stop(tx);
fail:
	return err;
}

void tx_thread_push(struct tx_handler *tx)
{
	atomic_set_release(&tx->has_work, 1);
	wake_up_interruptible_all(&tx->wq_data);
}


static size_t d2d_choose_datagram_size(unsigned int remote_version)
{
	switch (remote_version) {
	case D2DP_V0_INITIAL:
		return MIN_DATAGRAM_SIZE;
	case D2DP_V1_VAR_DGRAMS:
	default:
		return MAX_DATAGRAM_SIZE;
	}
}

static struct d2d_protocol *d2d_protocol_alloc(struct socket *sock,
					       struct d2d_security *security,
					       unsigned int remote_version)
{
	int err = 0;
	size_t payload = 0;
	size_t max_tx_datagram = 0;
	struct d2d_protocol *proto = NULL;

	/*
	 * Initialize protocol with min TX payload size. It is safe, as we can
	 * start work in bad network conditions. Later, if the connection is
	 * good, we will increase TX datagram size in the bandwidth estimation
	 * timer.
	 */
	payload = crypto_calc_payload_size(security, MIN_DATAGRAM_SIZE);
	if (!payload)
		return NULL;

	proto = d2dp_kzalloc(sizeof(*proto), GFP_KERNEL);
	if (!proto) {
		pr_err("cannot allocate D2DP instance\n");
		return NULL;
	}

	proto->sock = sock;
	proto->remote_version = remote_version;
	d2d_options_init(&proto->options);
	proto->security = security;
	max_tx_datagram = d2d_choose_datagram_size(remote_version);
	init_frames(&proto->frames, max_tx_datagram);

	err = init_tx_handler(&proto->tx, D2D_BUFFER_SIZE, payload);
	if (err)
		goto free_frames;

	err = init_rx_handler(&proto->rx, D2D_BUFFER_SIZE);
	if (err)
		goto free_tx;

	err = init_proto_timers(proto);
	if (err)
		goto free_rx;

	err = create_transport_threads(proto);
	if (err)
		goto free_timers;

	return proto;

free_timers:
	deinit_proto_timers(proto);
free_rx:
	deinit_rx_handler(&proto->rx);
free_tx:
	deinit_tx_handler(&proto->tx);
free_frames:
	deallocate_frames(&proto->frames);
	d2dp_kfree(proto);
	return NULL;
}

struct d2d_protocol *d2d_protocol_create(struct socket *sock,
					 struct d2d_security *security,
					 unsigned int remote_version)
{
	struct d2d_protocol *proto = NULL;

	if (!sock) {
		pr_err("cannot create D2DP with null socket\n");
		return NULL;
	}

	proto = d2d_protocol_alloc(sock, security, remote_version);
	if (!proto)
		return NULL;

	if (d2dp_register_kobj(proto)) {
		/* now we have to destroy the protocol as a kobject */
		kobject_put(&proto->kobj);
		return NULL;
	}

	return proto;
}

static void d2dp_stop_transport(struct d2d_protocol *proto)
{
	struct pid *pid = NULL;

	/* set `closing` flag and notify all */
	atomic_set_release(&proto->closing, 1);
	d2dp_wake_up_all(proto);

	/*
	 * Get task pids without checks as we have taken the additional
	 * references to them in `create_transport_threads`.
	 */

	/* send kill to RX thread as it may be blocked on network recv */
	pid = get_task_pid(proto->transport.rx_task, PIDTYPE_PID);
	kill_pid(pid, SIGTERM, 1);
	put_pid(pid);

	/* send kill to TX thread too, as it may hang up in socket send */
	pid = get_task_pid(proto->transport.tx_task, PIDTYPE_PID);
	kill_pid(pid, SIGTERM, 1);
	put_pid(pid);

	/* we hold a reference to the tasks, so just call `kthread_stop` */
	d2dp_kthread_stop(proto->transport.rx_task);
	d2dp_kthread_stop(proto->transport.tx_task);

	put_task_struct(proto->transport.rx_task);
	put_task_struct(proto->transport.tx_task);
}

static bool d2d_protocol_is_flushed(struct d2d_protocol *proto)
{
	bool txb_is_empty = false;
	wrap_t last_acked = 0;
	wrap_t rx_buf_ack = 0;
	struct tx_handler *tx = &proto->tx;
	struct rx_handler *rx = &proto->rx;

	mutex_lock(&tx->lock);
	mutex_lock(&rx->lock);

	txb_is_empty = tx_buf_is_empty(tx->buffer);
	rx_buf_ack   = rx_buf_get_ack(rx->buffer);
	last_acked   = tx->ack_state.last_acked;

	mutex_unlock(&rx->lock);
	mutex_unlock(&tx->lock);

	return txb_is_empty && last_acked == rx_buf_ack;
}

/**
 * d2d_protocol_destroy - the finalizer for the D2D Protocol instance
 * @proto: D2DP instance
 *
 * This function is aimed to completely destroy the protocol. It does not care
 * about TX/RX buffers status so if you need it, wait for a completion before
 * calling this function.
 *
 * This function is also registered as a `release` function for D2DP kobject
 */
void d2d_protocol_destroy(struct d2d_protocol *proto)
{
	/* stop transport before timers, as threads can re-enable timers */
	d2dp_stop_transport(proto);
	deinit_proto_timers(proto);
	deinit_rx_handler(&proto->rx);
	deinit_tx_handler(&proto->tx);
	deallocate_frames(&proto->frames);
	d2dp_kfree(proto);
}

/**
 * d2d_protocol_flush - try to flush the D2DP buffers
 * @proto: D2D Protocol instance
 *
 * Returns:
 *   0   when all packets in TX buffer were acked
 *  -EIO when packets have not been acked in D2D_DESTROY_TIMEOUT_MSEC
 */
static int d2d_protocol_flush(struct d2d_protocol *proto)
{
	int exitcode = 0;
	unsigned long timeout =
		jiffies + msecs_to_jiffies(D2D_DESTROY_TIMEOUT_MSEC);

	/* Do not wait when the transport failed */
	exitcode = atomic_read_acquire(&proto->exitcode);
	if (exitcode) {
		pr_err("protocol error: %d, no flush\n", exitcode);
		return -EIO;
	}

	/* Wait for the empty tx_buffer and for rx_buffer to be fully acked */
	while (!d2d_protocol_is_flushed(proto)) {
		exitcode = atomic_read_acquire(&proto->exitcode);
		if (exitcode) {
			pr_err("protocol error: %d, stop flush\n", exitcode);
			return -EIO;
		}

		if (time_after(jiffies, timeout)) {
			pr_err("flush timeout, stop flush\n");
			return -EIO;
		}

		/* heuristics: force ACK to allow remote side flush faster */
		d2d_timer_enable(&proto->timers.ack_timer);
		msleep(D2D_ACK_PERIOD_MSEC * 2);
	}

	return 0;
}

int d2d_protocol_close(struct d2d_protocol *proto)
{
	int ret = 0;

	ret = d2d_protocol_flush(proto);
	kobject_put(&proto->kobj);

	return ret;
}

/**
 * d2d_set_datagram_size - set maximum datagram size for TX path
 * @proto: D2DP instance
 * @new_size: new datagram size
 *
 * Returns 0 on success or negative error.
 */
int d2d_set_datagram_size(struct d2d_protocol *proto, size_t new_size)
{
	size_t new_payload = 0;

	if (new_size > MAX_DATAGRAM_SIZE) {
		pr_err("new size %zuB > max dgram size %uB\n",
		       new_size, MAX_DATAGRAM_SIZE);
		return -EINVAL;
	}

	if (new_size < MIN_DATAGRAM_SIZE) {
		pr_err("new size %zuB < min dgram size %uB\n",
		       new_size, MIN_DATAGRAM_SIZE);
		return -EINVAL;
	}

	new_payload = crypto_calc_payload_size(proto->security, new_size);
	if (!new_payload) {
		pr_err("new size %zuB does not fit security\n", new_size);
		return -EINVAL;
	}

	mutex_lock(&proto->tx.lock);
	tx_buf_set_max_elem(proto->tx.buffer, new_payload);
	mutex_unlock(&proto->tx.lock);

	return 0;
}

/**
 * d2d_get_datagram_size - get maxiumum datagram size for TX path
 * @proto: D2DP instance
 *
 * Returns the datagram size used in the moment of calling.
 */
size_t d2d_get_datagram_size(struct d2d_protocol *proto)
{
	size_t payload = 0;
	size_t datagram = 0;

	mutex_lock(&proto->tx.lock);
	payload = tx_buf_get_max_elem(proto->tx.buffer);
	mutex_unlock(&proto->tx.lock);

	datagram += d2d_get_header_packed_size();
	datagram += payload;
	if (proto->security)
		datagram += proto->security->cr_overhead;

	return datagram;
}

/**
 * atomic_try_sub_if_enough - try to subtract value from atomic
 * @i: value to subtract
 * @v: atomic value
 *
 * This function tries to subtract @i from @v only when the resulting value is
 * not negative. When the resulting value is not negative, the atomic value is
 * decreased, and the operation has a full barrier effect. Otherwise, the atomic
 * value is not changed, and no special ordering is guaranteed.
 *
 * Returns true when value is changed, false otherwise.
 */
static bool atomic_try_sub_if_enough(int i, atomic_t *v)
{
	int old = 0;
	int res = 0;

	old = atomic_read(v);
	while (old >= i) {
		res = atomic_cmpxchg(v, old, old - i);
		if (res == old)
			return true;

		old = res;
	}

	return false;
}

static bool __d2d_send_waitcond(struct d2d_protocol *proto,
				size_t bytes_wait, int *reason)
{
	int exitcode = 0;
	bool has_space = false;
	struct tx_handler *tx = &proto->tx;

	exitcode = atomic_read_acquire(&proto->exitcode);
	if (unlikely(exitcode)) {
		pr_err("sendwait: proto error: %d\n", exitcode);
		*reason = D2D_SEND_WAKEUP_REASON_PROTO_ERROR;
		return true;
	}

	/*
	 * It is highly unsafe to close a protocol instance while the instance
	 * is still used, because use-after-free may happen. Avoid that.
	 */
	if (WARN_ON(atomic_read_acquire(&proto->closing))) {
		pr_err("sendwait: closing while used\n");
		*reason = D2D_SEND_WAKEUP_REASON_CLOSING;
		return true;
	}

	/* try to capture the bytes for sending */
	has_space = atomic_try_sub_if_enough(bytes_wait, &tx->bytes);
	if (has_space)
		*reason = D2D_SEND_WAKEUP_REASON_OK;

	return has_space;
}

/**
 * __d2d_send_wait - wait for free space in the underlying D2DP buffer
 * @proto: D2DP instance
 * @bytes: how many bytes to wait
 *
 * This function blocks until @bytes of bytes are freed in the TX buffer or
 * error happens. The return value is the reason of wakeup (ok or error).
 */
static int __d2d_send_wait(struct d2d_protocol *proto, size_t bytes)
{
	int reason = D2D_SEND_WAKEUP_REASON_OK;
	int err = 0;

	err = wait_event_interruptible(proto->tx.wq_free,
				       __d2d_send_waitcond(proto,
							   bytes,
							   &reason));
	if (err == -ERESTARTSYS && reason == D2D_SEND_WAKEUP_REASON_OK) {
		pr_err("sendwait: interrupted\n");
		reason = D2D_SEND_WAKEUP_REASON_ERESTARTSYS;
	}

	return reason;
}

static ssize_t __d2d_sendiov_push(struct d2d_protocol *proto,
				  const struct kvec *iov, size_t iovlen,
				  size_t to_send, size_t reserved,
				  uint32_t flags)
{
	size_t old_usage = 0;
	size_t new_usage = 0;
	ssize_t res = 0;
	struct tx_handler *tx = &proto->tx;

	mutex_lock(&tx->lock);

	if (flags & D2D_SEND_FLAG_KEY_UPDATE) {
		/* check if someone is already updating the keys */
		if (atomic_read_acquire(&tx->key_update)) {
			mutex_unlock(&tx->lock);
			pr_err("TX keys are being updated by someone else\n");
			return -EALREADY;
		}
	}

	old_usage = tx_buf_get_usage(tx->buffer);
	res = tx_buf_append_iov(tx->buffer, iov, iovlen, to_send);
	if (res < 0) {
		mutex_unlock(&tx->lock);
		pr_err("cannot append (%zu KVECs: %zuB): error %zd\n",
		       iovlen, to_send, res);
		return res;
	}
	new_usage = tx_buf_get_usage(tx->buffer);

	if (flags & D2D_SEND_FLAG_KEY_UPDATE) {
		/* this should always succeed as we have just appended data */
		tx_buf_get_last_seq_id(tx->buffer, &tx->key_update_id);
		atomic_set_release(&tx->key_update, 1);
	}

	mutex_unlock(&tx->lock);

	/* notify TX thread that we have some work */
	tx_thread_push(tx);

	/* return unused bytes if any */
	if (new_usage - old_usage != reserved) {
		int unused = reserved - (new_usage - old_usage);

		atomic_add_return_release(unused, &tx->bytes);
		wake_up_interruptible_all(&tx->wq_free);
	}

	if (proto->remote_version >= D2DP_V1_VAR_DGRAMS)
		d2d_timer_enable(&proto->timers.bw_est.timer);

	return res;
}

/**
 * __d2d_sendiov - common entry point for all D2DP sending operations
 * @proto: D2DP instance
 * @iov: kvec array with sparse data
 * @iovlen: number of entries in @iov array
 * @length: total amount of bytes to send
 * @flags: D2DP sending flags
 *
 * This operation is atomic, either all the data is pushed to underlying buffer,
 * or no data is pushed when an error happened.
 *
 * Returns @length when success or negative error:
 * -EINVAL      when @length is longer than underlying buffer length
 * -ERESTARTSYS when the operation was interrupted by a signal
 * -ENOMEM      when the memory allocation for TX buffer failed
 * -EALREADY    when D2D_SEND_FLAG_KEY_UPDATE is set but update is in progress
 * other errors when the underlying networking socket is inoperable
 */
static ssize_t __d2d_sendiov(struct d2d_protocol *proto, const struct kvec *iov,
			     size_t iovlen, size_t length, u32 flags)
{
	int ret = 0;
	size_t reserved = 0;
	const struct tx_handler *tx = &proto->tx;

	/* check if requested amount of bytes to send exceeds buffer length */
	if (unlikely(length > tx->max_bufsize)) {
		pr_err("sendiov: too big buffer size to send: %zu vs %zu\n",
		       length, tx->max_bufsize);
		return -EINVAL;
	}

	/* estimate maximum usage possible for TX buffer for given size */
	reserved = tx_buf_estimate_usage(proto->tx.buffer, length);

	ret = __d2d_send_wait(proto, reserved);
	switch (ret) {
	case D2D_SEND_WAKEUP_REASON_OK:
		break;
	case D2D_SEND_WAKEUP_REASON_INVALID_SIZE:
		return -EINVAL;
	case D2D_SEND_WAKEUP_REASON_PROTO_ERROR:
		return atomic_read_acquire(&proto->exitcode);
	case D2D_SEND_WAKEUP_REASON_ERESTARTSYS:
		return -ERESTARTSYS;
	case D2D_SEND_WAKEUP_REASON_CLOSING:
		return -EPIPE;
	default:
		WARN(true, "bad enum value: %d\n", ret);
		return -EINVAL;
	}

	return __d2d_sendiov_push(proto, iov, iovlen, length, reserved, flags);
}

ssize_t d2d_send(struct d2d_protocol *proto, const void *data, size_t len,
		 u32 flags)
{
	const struct kvec iov = {
		.iov_base = (void *)data,
		.iov_len  = len,
	};

	return __d2d_sendiov(proto, &iov, 1, len, flags);
}

/*
 * TODO: make this function safe against size_t overflow
 */
static size_t d2d_iov_length(const struct kvec *iov, size_t iovlen)
{
	size_t i = 0;
	size_t ret = 0;

	for (i = 0; i < iovlen; i++)
		ret += iov[i].iov_len;

	return ret;
}

ssize_t d2d_sendiov(struct d2d_protocol *proto, const struct kvec *iov,
		    size_t iovlen, size_t to_send, u32 flags)
{
	size_t iov_total_len = 0;

	iov_total_len = d2d_iov_length(iov, iovlen);
	if (unlikely(to_send > iov_total_len)) {
		pr_err("send: invalid arg: to_send=%zuB, but kvec=%zuB\n",
		       to_send, iov_total_len);
		return -EINVAL;
	}

	return __d2d_sendiov(proto, iov, iovlen, to_send, flags);
}

static bool __d2d_recv_waitcond(struct d2d_protocol *proto,
				size_t bytes_wait, bool waitall, int *reason)
{
	int exitcode = 0;
	int bytes = 0;
	bool wakeup = false;
	const struct rx_handler *rx = &proto->rx;

	exitcode = atomic_read_acquire(&proto->exitcode);
	if (unlikely(exitcode)) {
		pr_err("recvwait: proto error: %d\n", exitcode);
		*reason = D2D_RECV_WAKEUP_REASON_PROTO_ERROR;
		return true;
	}

	/*
	 * It is highly unsafe to close a protocol instance while the instance
	 * is still used, because use-after-free may happen. Avoid that.
	 */
	if (WARN_ON(atomic_read_acquire(&proto->closing))) {
		pr_err("recvwait: closing while used\n");
		*reason = D2D_RECV_WAKEUP_REASON_CLOSING;
		return true;
	}

	/*
	 * Synchronizes with:
	 * - `atomic_add_return_release` in `rx_process_data`
	 * - `atomic_sub_return_release` in `__d2d_recviov_pull`
	 */
	bytes = atomic_read_acquire(&rx->seqbytes);

	/* when D2D_RECV_FLAG_WAITALL we need to get all data in once */
	if (waitall)
		wakeup = bytes >= bytes_wait;
	else
		wakeup = bytes > 0;

	if (wakeup)
		*reason = D2D_RECV_WAKEUP_REASON_OK;

	return wakeup;
}

static int __d2d_recv_wait_notimeout(struct d2d_protocol *proto,
				     size_t bytes, bool waitall)
{
	int ret = 0;
	int reason = D2D_RECV_WAKEUP_REASON_OK;

	ret = wait_event_interruptible(proto->rx.wq_data,
				       __d2d_recv_waitcond(proto,
							   bytes,
							   waitall,
							   &reason));
	if (ret == -ERESTARTSYS && reason == D2D_RECV_WAKEUP_REASON_OK) {
		pr_err("recvwait: interrupted\n");
		reason = D2D_RECV_WAKEUP_REASON_ERESTARTSYS;
	}

	return reason;
}

static int __d2d_recv_wait_timeout(struct d2d_protocol *proto,
				   int timeout_ms, size_t bytes, bool waitall)
{
	int ret = 0;
	int reason = D2D_RECV_WAKEUP_REASON_OK;

	ret = wait_event_interruptible_timeout(proto->rx.wq_data,
					       __d2d_recv_waitcond(proto,
								   bytes,
								   waitall,
								   &reason),
					       msecs_to_jiffies(timeout_ms));
	if (ret == -ERESTARTSYS && reason == D2D_RECV_WAKEUP_REASON_OK) {
		pr_err("recvwait: interrupted\n");
		reason = D2D_RECV_WAKEUP_REASON_ERESTARTSYS;
	} else if (!ret && reason == D2D_RECV_WAKEUP_REASON_OK) {
		reason = D2D_RECV_WAKEUP_REASON_TIMEOUT;
	}
	return reason;
}

static int __d2d_recv_wait(struct d2d_protocol *proto, int timeout_ms,
			   size_t bytes, bool waitall)
{
	if (!timeout_ms)
		return __d2d_recv_wait_notimeout(proto, bytes, waitall);
	else
		return __d2d_recv_wait_timeout(proto, timeout_ms,
					       bytes, waitall);
}

static ssize_t __d2d_recviov_pull(struct d2d_protocol *proto,
				  struct kvec *iov, size_t iovlen,
				  size_t to_recv)
{
	size_t i = 0;
	size_t bytes_read = 0;
	struct rx_handler *rx = &proto->rx;

	mutex_lock(&rx->lock);

	for (i = 0; i < iovlen; i++) {
		size_t len   = min(iov[i].iov_len, to_recv);
		size_t bytes = rx_buf_get(rx->buffer, iov[i].iov_base, len);

		bytes_read += bytes;
		to_recv    -= bytes;
		if (!to_recv)
			break;
	}

	/* make sure we completed `rx_buf_get` before subtracting */
	atomic_sub_return_release(bytes_read, &proto->rx.seqbytes);

	mutex_unlock(&rx->lock);

	d2d_timer_enable(&proto->timers.ack_timer);

	return bytes_read;
}

static ssize_t __d2d_recviov(struct d2d_protocol *proto, struct kvec *iov,
			     size_t iovlen, size_t length, u32 flags)
{
	int ret = 0;
	int timeout_ms = 0;
	bool waitall = flags & D2D_RECV_FLAG_WAITALL;

	/* check if requested amount of bytes to recv exceeds buffer length */
	if (unlikely(waitall && length > D2D_RECV_WAITALL_LIMIT)) {
		pr_err("recviov: requested too much: %zuB vs %zuB\n",
		       length, D2D_RECV_WAITALL_LIMIT);
		return -EINVAL;
	}

	if (WARN_ON(d2d_get_opt(proto, D2D_OPT_RCVTIMEO, &timeout_ms)))
		return -EINVAL;

	ret = __d2d_recv_wait(proto, timeout_ms, length, waitall);
	switch (ret) {
	case D2D_RECV_WAKEUP_REASON_OK:
		break;
	case D2D_RECV_WAKEUP_REASON_PROTO_ERROR:
		return atomic_read_acquire(&proto->exitcode);
	case D2D_RECV_WAKEUP_REASON_INVALID_SIZE:
		return -EINVAL;
	case D2D_RECV_WAKEUP_REASON_ERESTARTSYS:
		return -ERESTARTSYS;
	case D2D_RECV_WAKEUP_REASON_TIMEOUT:
		return -EAGAIN;
	case D2D_RECV_WAKEUP_REASON_CLOSING:
		return -EPIPE;
	default:
		WARN(true, "bad enum value: %d\n", ret);
		return -EINVAL;
	}

	return __d2d_recviov_pull(proto, iov, iovlen, length);
}

ssize_t d2d_recv(struct d2d_protocol *proto, void *data, size_t len, u32 flags)
{
	struct kvec iov = {
		.iov_base = data,
		.iov_len  = len,
	};

	return __d2d_recviov(proto, &iov, 1, len, flags);
}

ssize_t d2d_recviov(struct d2d_protocol *proto, struct kvec *iov,
		    size_t iovlen, size_t to_recv, u32 flags)
{
	size_t iov_total_len = 0;

	iov_total_len = d2d_iov_length(iov, iovlen);
	if (unlikely(to_recv > iov_total_len)) {
		pr_err("recv: invalid args: to_recv=%zuB, but kvec=%zuB\n",
		       to_recv, iov_total_len);
		return -EINVAL;
	}

	return __d2d_recviov(proto, iov, iovlen, to_recv, flags);
}

static int __d2d_crypto_wait_condition(struct d2d_protocol *proto)
{
	if (atomic_read_acquire(&proto->exitcode))
		return true;

	return !atomic_read_acquire(&proto->tx.key_update);
}

int __must_check d2d_crypto_tx_lock(struct d2d_protocol *proto)
{
	int exitcode = 0;
	int ret = wait_event_interruptible(proto->tx.wq_data,
					   __d2d_crypto_wait_condition(proto));

	/* exitcode overrides -ERESTARTSYS */
	exitcode = atomic_read_acquire(&proto->exitcode);
	if (exitcode) {
		pr_err("txlock: proto error: %d\n", exitcode);
		return exitcode;
	}

	/* also return if interrupted */
	if (ret) {
		pr_err("txlock: interrupted\n");
		return ret;
	}

	/* OK, KEY_UPDATE request is acknowledged, we can lock crypto */
	mutex_lock(&proto->frames.crypto_tx_mtx);

	return 0;
}

void d2d_crypto_tx_unlock(struct d2d_protocol *proto)
{
	mutex_unlock(&proto->frames.crypto_tx_mtx);
}

void d2d_crypto_rx_lock(struct d2d_protocol *proto)
{
	mutex_lock(&proto->frames.crypto_rx_mtx);
}

void d2d_crypto_rx_unlock(struct d2d_protocol *proto)
{
	mutex_unlock(&proto->frames.crypto_rx_mtx);
}
