/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Fault injection interface for D2D Protocol.
 */

#ifndef D2D_FAULT_INJECTION_H
#define D2D_FAULT_INJECTION_H

#include <linux/types.h>

#ifdef CONFIG_FAULT_INJECTION_DEBUG_FS

int d2dp_fail_init_debugfs(void);
void d2dp_fail_exit_debugfs(void);

bool d2dp_kzalloc_fail(void);
bool d2dp_kmem_cache_create_fail(void);
bool d2dp_kthread_create_fail(void);
bool d2dp_workqueue_create_fail(void);

bool d2dp_kmem_cache_alloc_fail(void);
bool d2dp_frame_kvmalloc_fail(void);
bool d2dp_node_kvmalloc_fail(void);
bool d2dp_encrypt_fail(void);
bool d2dp_decrypt_fail(void);
bool d2dp_socket_recv_fail(void);
bool d2dp_socket_send_fail(void);

int d2dp_random_crypto_error(void);
int d2dp_random_socket_error(void);

#else /* CONFIG_FAULT_INJECTION_DEBUG_FS */

static inline int d2dp_fail_init_debugfs(void)
{
	return 0;
}

static inline void d2dp_fail_exit_debugfs(void)
{
}

static inline bool d2dp_kzalloc_fail(void)
{
	return false;
}

static inline bool d2dp_kmem_cache_create_fail(void)
{
	return false;
}

static inline bool d2dp_workqueue_create_fail(void)
{
	return false;
}

static inline bool d2dp_kthread_create_fail(void)
{
	return false;
}

static inline bool d2dp_kmem_cache_alloc_fail(void)
{
	return false;
}

static inline bool d2dp_frame_kvmalloc_fail(void)
{
	return false;
}

static inline bool d2dp_node_kvmalloc_fail(void)
{
	return false;
}

static inline bool d2dp_encrypt_fail(void)
{
	return false;
}

static inline bool d2dp_decrypt_fail(void)
{
	return false;
}

static inline bool d2dp_socket_recv_fail(void)
{
	return false;
}

static inline bool d2dp_socket_send_fail(void)
{
	return false;
}

static inline int d2dp_random_crypto_error(void)
{
	return 0;
}

static inline int d2dp_random_socket_error(void)
{
	return 0;
}

#endif /* CONFIG_FAULT_INJECTION_DEBUG_FS */

#endif /* D2D_FAULT_INJECTION_H */
