/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Resource management wrappers for D2DP. Introduced for better tracing of
 * resource allocation. It also simplifies fault injection.
 */

#ifndef D2D_RESOURCES_H
#define D2D_RESOURCES_H

#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/net.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include "d2d.h"
#include "fault_inject.h"

/*
 * Control plane resources.
 */

static inline void *d2dp_kzalloc(size_t size, gfp_t flags)
{
	if (d2dp_kzalloc_fail())
		return NULL;

	return kzalloc(size, flags);
}

static inline void d2dp_kfree(void *obj)
{
	kfree(obj);
}

/* 4.15 introduced new bitwise integer type for kernel memory cache flags */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
typedef slab_flags_t d2dp_slab_flags_t;
#else
typedef unsigned long d2dp_slab_flags_t;
#endif

static inline struct kmem_cache *d2dp_kmem_cache_create(const char *name,
							unsigned int size,
							unsigned int align,
							d2dp_slab_flags_t flags,
							void (*ctor)(void *))
{
	if (d2dp_kmem_cache_create_fail())
		return NULL;

	return kmem_cache_create(name, size, align, flags, ctor);
}

static inline void d2dp_kmem_cache_destroy(struct kmem_cache *cache)
{
	kmem_cache_destroy(cache);
}

static inline
struct task_struct *d2dp_kthread_create(int (*threadfn)(void *data),
					void *data, const char *name)
{
	if (d2dp_kthread_create_fail())
		return ERR_PTR(-ENOMEM);

	return kthread_create(threadfn, data, "%s", name);
}

static inline void d2dp_kthread_stop(struct task_struct *task)
{
	kthread_stop(task);
}

static inline struct workqueue_struct *d2d_workqueue_create(const char *wq_name)
{
	if (d2dp_workqueue_create_fail())
		return NULL;

	return alloc_ordered_workqueue("%s", WQ_MEM_RECLAIM, wq_name);
}

static inline void d2d_workqueue_destroy(struct workqueue_struct *wq)
{
	destroy_workqueue(wq);
}

/*
 * Data plane resources.
 */

static inline void *d2dp_kmem_cache_alloc(struct kmem_cache *cache, gfp_t flags)
{
	if (d2dp_kmem_cache_alloc_fail())
		return NULL;

	return kmem_cache_alloc(cache, flags);
}

static inline void d2dp_kmem_cache_free(struct kmem_cache *cache, void *obj)
{
	kmem_cache_free(cache, obj);
}

static inline void *d2dp_frame_kvmalloc(size_t len, gfp_t flags)
{
	if (d2dp_frame_kvmalloc_fail())
		return NULL;

	return kvmalloc(len, flags);
}

static inline void d2dp_kvfree(void *obj)
{
	kvfree(obj);
}

static inline void *d2dp_node_kvmalloc(size_t len, gfp_t flags)
{
	if (d2dp_node_kvmalloc_fail())
		return NULL;

	return kvmalloc(len, flags);
}

static inline int d2dp_encrypt_frame(struct d2d_security *security,
				     void *tx_frame, size_t length,
				     void *tx_encr_frame, size_t txlen)
{
	if (d2dp_encrypt_fail())
		return d2dp_random_crypto_error();

	return security->encrypt(security, tx_frame, length,
				 tx_encr_frame, txlen);
}

static inline int d2dp_decrypt_frame(struct d2d_security *security,
				     void *rx_encr_frame, size_t pktlen,
				     void *rx_frame, size_t decr_len)
{
	if (d2dp_decrypt_fail())
		return d2dp_random_crypto_error();

	return security->decrypt(security, rx_encr_frame, pktlen,
				 rx_frame, decr_len);
}

static inline int d2dp_recvmsg(struct socket *sock, struct msghdr *msg,
			       struct kvec *vec, size_t num, size_t len,
			       int flags)
{
	if (d2dp_socket_recv_fail())
		return d2dp_random_socket_error();

	return kernel_recvmsg(sock, msg, vec, num, len, flags);
}

static inline int d2dp_sendmsg(struct socket *sock, struct msghdr *msg,
			       struct kvec *vec, size_t num,
			       size_t len)
{
	if (d2dp_socket_send_fail())
		return d2dp_random_socket_error();

	return kernel_sendmsg(sock, msg, vec, num, len);
}

#endif /* D2D_RESOURCES_H */
