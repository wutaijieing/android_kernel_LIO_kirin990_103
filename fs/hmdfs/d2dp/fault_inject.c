// SPDX-License-Identifier: GPL-2.0
/*
 * D2DP fault injection implementation.
 */

#include <asm-generic/errno.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fault-inject.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/random.h>

#include "fault_inject.h"

static struct {
	struct fault_attr attr;
	struct dentry *root;
	bool init_alloc_fail;
	bool cache_fail;
	bool frame_fail;
	bool node_fail;
	bool encrypt_fail;
	bool decrypt_fail;
	bool sock_recv_fail;
	bool sock_send_fail;
} d2dp_fail = {
	.attr = FAULT_ATTR_INITIALIZER,
	.root = NULL,
};

bool d2dp_kzalloc_fail(void)
{
	if (!d2dp_fail.init_alloc_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP kzalloc [init]\n");
	return true;
}

bool d2dp_kmem_cache_create_fail(void)
{
	if (!d2dp_fail.init_alloc_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP kmem_cache_create [init]\n");
	return true;
}

bool d2dp_kthread_create_fail(void)
{
	if (!d2dp_fail.init_alloc_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP kthread_create [init]\n");
	return true;
}

bool d2dp_workqueue_create_fail(void)
{
	if (!d2dp_fail.init_alloc_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP alloc_workqueue [init]\n");
	return true;
}

bool d2dp_kmem_cache_alloc_fail(void)
{
	if (!d2dp_fail.cache_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP kmem_cache_alloc\n");
	return true;
}

bool d2dp_frame_kvmalloc_fail(void)
{
	if (!d2dp_fail.frame_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP frame kvmalloc\n");
	return true;
}

bool d2dp_node_kvmalloc_fail(void)
{
	if (!d2dp_fail.node_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP node kvmalloc\n");
	return true;
}

int d2dp_random_crypto_error(void)
{
	static int crypto_errs[] = {
		-ENOMEM,
		-EBADMSG,
		-EINVAL,
		-EPERM,
	};

	return crypto_errs[get_random_u32() % ARRAY_SIZE(crypto_errs)];
}

int d2dp_random_socket_error(void)
{
	static int socket_errs[] = {
		-ENOMEM,
		-EBADMSG,
		-EINVAL,
		-EPIPE,
		-ERESTARTSYS,
		-EBADF,
		-EMSGSIZE,
		-ENOTSOCK,
		-EPERM,
		-ECONNRESET,
		-ECONNREFUSED,
	};

	return socket_errs[get_random_u32() % ARRAY_SIZE(socket_errs)];
}

bool d2dp_encrypt_fail(void)
{
	if (!d2dp_fail.encrypt_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP encrypt\n");
	return true;
}

bool d2dp_decrypt_fail(void)
{
	if (!d2dp_fail.decrypt_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP decrypt\n");
	return true;
}

bool d2dp_socket_recv_fail(void)
{
	if (!d2dp_fail.sock_recv_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP socket recv\n");
	return true;
}

bool d2dp_socket_send_fail(void)
{
	if (!d2dp_fail.sock_send_fail)
		return false;

	if (!should_fail(&d2dp_fail.attr, 1))
		return false;

	pr_err("injected fail in D2DP socket send\n");
	return true;
}

int d2dp_fail_init_debugfs(void)
{
	d2dp_fail.root = fault_create_debugfs_attr("d2dpfail", NULL,
						   &d2dp_fail.attr);
	if (IS_ERR(d2dp_fail.root)) {
		pr_err("cannot allocate debugfs root\n");
		return PTR_ERR(d2dp_fail.root);
	}

	debugfs_create_bool("init_alloc_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.init_alloc_fail);
	debugfs_create_bool("cache_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.cache_fail);
	debugfs_create_bool("frame_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.frame_fail);
	debugfs_create_bool("node_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.node_fail);
	debugfs_create_bool("encrypt_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.encrypt_fail);
	debugfs_create_bool("decrypt_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.decrypt_fail);
	debugfs_create_bool("sock_recv_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.sock_recv_fail);
	debugfs_create_bool("sock_send_fail", 0600, d2dp_fail.root,
			    &d2dp_fail.sock_send_fail);

	pr_info("Initialized D2DP fault injection\n");

	return 0;
}

void d2dp_fail_exit_debugfs(void)
{
	pr_info("D2DP fault injection deinitialized\n");
	debugfs_remove_recursive(d2dp_fail.root);
}
