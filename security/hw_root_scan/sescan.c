/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the sescan.c for selinux status checking
 * Create: 2016-06-18
 */

#include "./include/sescan.h"

#define RSCAN_LOG_TAG "sescan"

#ifdef CONFIG_SECURITY_SELINUX_DEVELOP

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
/* reference security/selinux/include/security.h */
struct selinux_state {
#ifdef CONFIG_SECURITY_SELINUX_DISABLE
	bool disabled;
#endif
	bool enforcing;
};

extern struct selinux_state selinux_state;
#define SELINUX_ENFORCING (selinux_state.enforcing ? 1 : 0)

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
struct selinux_state {
	bool disabled;
	bool enforcing;
};

extern struct selinux_state selinux_state;
#define SELINUX_ENFORCING (selinux_state.enforcing ? 1 : 0)
#else
/* selinux_enforcing is kernel variable */
extern int selinux_enforcing;
#define SELINUX_ENFORCING selinux_enforcing
#endif /* LINUX_VERSION_CODE */

#else /* CONFIG_SECURITY_SELINUX_DEVELOP */
#define SELINUX_ENFORCING 1
#endif /* CONFIG_SECURITY_SELINUX_DEVELOP */

int get_selinux_enforcing(void)
{
	return SELINUX_ENFORCING;
}

int sescan_hookhash(uint8_t *hash, size_t hash_len)
{
	int err;
	struct crypto_shash *tfm = crypto_alloc_shash("sha256", 0, 0);

	SHASH_DESC_ON_STACK(shash, tfm);
	var_not_used(hash_len);

	if (IS_ERR(tfm)) {
		rs_log_error(RSCAN_LOG_TAG, "crypto_alloc_hash(sha256) error %ld",
			PTR_ERR(tfm));
		return -ENOMEM;
	}

	shash->tfm = tfm;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 1, 0)
	shash->flags = 0;
#endif

	err = crypto_shash_init(shash);
	if (err < 0) {
		rs_log_error(RSCAN_LOG_TAG, "crypto_shash_init error: %d", err);
		crypto_free_shash(tfm);
		return err;
	}

/*
 * reference security/security.c: call_void_hook
 *
 * sizeof(P->hook.FUNC) in order to get the size of
 * function pointer that for computing a hash later
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 16, 0)
#define do_one_head(FUNC) do { \
	struct security_hook_list *p = NULL; \
		list_for_each_entry(p, &security_hook_heads.FUNC, list) { \
			crypto_shash_update(shash, (char *)&(p->hook.FUNC), \
				sizeof(p->hook.FUNC)); \
		} \
	} while (0)
#else
#define do_one_head(FUNC) do { \
	struct security_hook_list *p = NULL; \
		hlist_for_each_entry(p, &security_hook_heads.FUNC, list) { \
			crypto_shash_update(shash, (char *)&(p->hook.FUNC), \
				sizeof(p->hook.FUNC)); \
		} \
	} while (0)
#endif
	// reference initialization in security_hook_heads in security/security.c
	do_one_head(binder_set_context_mgr);
	do_one_head(binder_transaction);
	do_one_head(binder_transfer_binder);
	do_one_head(binder_transfer_file);
	do_one_head(ptrace_access_check);
	do_one_head(ptrace_traceme);
	do_one_head(capget);
	do_one_head(capset);
	do_one_head(capable);
	do_one_head(quotactl);
	do_one_head(quota_on);
	do_one_head(syslog);
	do_one_head(settime);
	do_one_head(vm_enough_memory);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	do_one_head(bprm_set_creds);
#else
	do_one_head(bprm_creds_for_exec);
	do_one_head(bprm_creds_from_file);
	do_one_head(inode_init_security_anon);
	do_one_head(kernel_post_load_data);
	do_one_head(task_fix_setgid);
#if defined(CONFIG_SECURITY) && defined(CONFIG_WATCH_QUEUE)
	do_one_head(post_notification);
#endif

#if defined(CONFIG_SECURITY) && defined(CONFIG_KEY_NOTIFICATIONS)
	do_one_head(watch_key);
#endif

#ifdef CONFIG_PERF_EVENTS
	do_one_head(perf_event_open);
	do_one_head(perf_event_alloc);
	do_one_head(perf_event_free);
	do_one_head(perf_event_read);
	do_one_head(perf_event_write);
#endif
#endif  /* LINUX_VERSION_CODE (5, 10, 0) */
	do_one_head(bprm_check_security);
	do_one_head(bprm_committing_creds);
	do_one_head(bprm_committed_creds);
	do_one_head(sb_alloc_security);
	do_one_head(sb_free_security);
	do_one_head(sb_remount);
	do_one_head(sb_kern_mount);
	do_one_head(sb_show_options);
	do_one_head(sb_statfs);
	do_one_head(sb_mount);
	do_one_head(sb_umount);
	do_one_head(sb_pivotroot);
	do_one_head(sb_set_mnt_opts);
	do_one_head(sb_clone_mnt_opts);
	do_one_head(dentry_init_security);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 19, 0)
	do_one_head(sb_copy_data);
	do_one_head(sb_parse_opts_str);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	do_one_head(fs_context_dup);
	do_one_head(fs_context_parse_param);
	do_one_head(sb_free_mnt_opts);
	do_one_head(sb_eat_lsm_opts);
	do_one_head(sb_add_mnt_opt);
	do_one_head(move_mount);
	do_one_head(dentry_create_files_as);
	do_one_head(path_notify);
	do_one_head(inode_copy_up);
	do_one_head(inode_copy_up_xattr);
	do_one_head(kernfs_init_security);
	do_one_head(cred_getsecid);
	do_one_head(kernel_load_data);
	do_one_head(kernel_read_file);
	do_one_head(kernel_post_read_file);
	do_one_head(kernel_module_request);
	do_one_head(inode_invalidate_secctx);
	do_one_head(locked_down);

#ifdef CONFIG_SECURITY_NETWORK
	do_one_head(socket_socketpair);
	do_one_head(sctp_assoc_request);
	do_one_head(sctp_bind_connect);
	do_one_head(sctp_sk_clone);
#endif  /* CONFIG_SECURITY_NETWORK */

#ifdef CONFIG_BPF_SYSCALL
	do_one_head(bpf);
	do_one_head(bpf_map);
	do_one_head(bpf_prog);
	do_one_head(bpf_map_alloc_security);
	do_one_head(bpf_map_free_security);
	do_one_head(bpf_prog_alloc_security);
	do_one_head(bpf_prog_free_security);
#endif  /* CONFIG_BPF_SYSCALL */

#endif  /* LINUX_VERSION_CODE (5, 4, 0) */
#ifdef CONFIG_SECURITY_PATH
	do_one_head(path_unlink);
	do_one_head(path_mkdir);
	do_one_head(path_rmdir);
	do_one_head(path_mknod);
	do_one_head(path_truncate);
	do_one_head(path_symlink);
	do_one_head(path_link);
	do_one_head(path_rename);
	do_one_head(path_chmod);
	do_one_head(path_chown);
	do_one_head(path_chroot);
#endif  /* CONFIG_SECURITY_PATH */
	do_one_head(inode_alloc_security);
	do_one_head(inode_free_security);
	do_one_head(inode_init_security);
	do_one_head(inode_create);
	do_one_head(inode_link);
	do_one_head(inode_unlink);
	do_one_head(inode_symlink);
	do_one_head(inode_mkdir);
	do_one_head(inode_rmdir);
	do_one_head(inode_mknod);
	do_one_head(inode_rename);
	do_one_head(inode_readlink);
	do_one_head(inode_follow_link);
	do_one_head(inode_permission);
	do_one_head(inode_setattr);
	do_one_head(inode_getattr);
	do_one_head(inode_setxattr);
	do_one_head(inode_post_setxattr);
	do_one_head(inode_getxattr);
	do_one_head(inode_listxattr);
	do_one_head(inode_removexattr);
	do_one_head(inode_need_killpriv);
	do_one_head(inode_killpriv);
	do_one_head(inode_getsecurity);
	do_one_head(inode_setsecurity);
	do_one_head(inode_listsecurity);
	do_one_head(inode_getsecid);
	do_one_head(file_permission);
	do_one_head(file_alloc_security);
	do_one_head(file_free_security);
	do_one_head(file_ioctl);
	do_one_head(mmap_addr);
	do_one_head(mmap_file);
	do_one_head(file_mprotect);
	do_one_head(file_lock);
	do_one_head(file_fcntl);
	do_one_head(file_set_fowner);
	do_one_head(file_send_sigiotask);
	do_one_head(file_receive);
	do_one_head(file_open);
	do_one_head(task_alloc);
	do_one_head(task_free);
	do_one_head(cred_alloc_blank);
	do_one_head(cred_free);
	do_one_head(cred_prepare);
	do_one_head(cred_transfer);
	do_one_head(kernel_act_as);
	do_one_head(kernel_create_files_as);
	do_one_head(task_fix_setuid);
	do_one_head(task_setpgid);
	do_one_head(task_getpgid);
	do_one_head(task_getsid);
	do_one_head(task_getsecid);
	do_one_head(task_setnice);
	do_one_head(task_setioprio);
	do_one_head(task_getioprio);
	do_one_head(task_prlimit);
	do_one_head(task_setrlimit);
	do_one_head(task_setscheduler);
	do_one_head(task_getscheduler);
	do_one_head(task_movememory);
	do_one_head(task_kill);
	do_one_head(task_prctl);
	do_one_head(task_to_inode);
	do_one_head(ipc_permission);
	do_one_head(ipc_getsecid);
	do_one_head(msg_msg_alloc_security);
	do_one_head(msg_msg_free_security);
	do_one_head(msg_queue_alloc_security);
	do_one_head(msg_queue_free_security);
	do_one_head(msg_queue_associate);
	do_one_head(msg_queue_msgctl);
	do_one_head(msg_queue_msgsnd);
	do_one_head(msg_queue_msgrcv);
	do_one_head(shm_alloc_security);
	do_one_head(shm_free_security);
	do_one_head(shm_associate);
	do_one_head(shm_shmctl);
	do_one_head(shm_shmat);
	do_one_head(sem_alloc_security);
	do_one_head(sem_free_security);
	do_one_head(sem_associate);
	do_one_head(sem_semctl);
	do_one_head(sem_semop);
	do_one_head(netlink_send);
	do_one_head(d_instantiate);
	do_one_head(getprocattr);
	do_one_head(setprocattr);
	do_one_head(ismaclabel);
	do_one_head(secid_to_secctx);
	do_one_head(secctx_to_secid);
	do_one_head(release_secctx);
	do_one_head(inode_notifysecctx);
	do_one_head(inode_setsecctx);
	do_one_head(inode_getsecctx);
#ifdef CONFIG_SECURITY_NETWORK
	do_one_head(unix_stream_connect);
	do_one_head(unix_may_send);
	do_one_head(socket_create);
	do_one_head(socket_post_create);
	do_one_head(socket_bind);
	do_one_head(socket_connect);
	do_one_head(socket_listen);
	do_one_head(socket_accept);
	do_one_head(socket_sendmsg);
	do_one_head(socket_recvmsg);
	do_one_head(socket_getsockname);
	do_one_head(socket_getpeername);
	do_one_head(socket_getsockopt);
	do_one_head(socket_setsockopt);
	do_one_head(socket_shutdown);
	do_one_head(socket_sock_rcv_skb);
	do_one_head(socket_getpeersec_stream);
	do_one_head(socket_getpeersec_dgram);
	do_one_head(sk_alloc_security);
	do_one_head(sk_free_security);
	do_one_head(sk_clone_security);
	do_one_head(sk_getsecid);
	do_one_head(sock_graft);
	do_one_head(inet_conn_request);
	do_one_head(inet_csk_clone);
	do_one_head(inet_conn_established);
	do_one_head(secmark_relabel_packet);
	do_one_head(secmark_refcount_inc);
	do_one_head(secmark_refcount_dec);
	do_one_head(req_classify_flow);
	do_one_head(tun_dev_alloc_security);
	do_one_head(tun_dev_free_security);
	do_one_head(tun_dev_create);
	do_one_head(tun_dev_attach_queue);
	do_one_head(tun_dev_attach);
	do_one_head(tun_dev_open);
#endif  /* CONFIG_SECURITY_NETWORK */
#ifdef CONFIG_SECURITY_INFINIBAND
	do_one_head(ib_pkey_access);
	do_one_head(ib_endport_manage_subnet);
	do_one_head(ib_alloc_security);
	do_one_head(ib_free_security);
#endif  /* CONFIG_SECURITY_INFINIBAND */
#ifdef CONFIG_SECURITY_NETWORK_XFRM
	do_one_head(xfrm_policy_alloc_security);
	do_one_head(xfrm_policy_clone_security);
	do_one_head(xfrm_policy_free_security);
	do_one_head(xfrm_policy_delete_security);
	do_one_head(xfrm_state_alloc);
	do_one_head(xfrm_state_alloc_acquire);
	do_one_head(xfrm_state_free_security);
	do_one_head(xfrm_state_delete_security);
	do_one_head(xfrm_policy_lookup);
	do_one_head(xfrm_state_pol_flow_match);
	do_one_head(xfrm_decode_session);
#endif  /* CONFIG_SECURITY_NETWORK_XFRM */
#ifdef CONFIG_KEYS
	do_one_head(key_alloc);
	do_one_head(key_free);
	do_one_head(key_permission);
	do_one_head(key_getsecurity);
#endif  /* CONFIG_KEYS */
#ifdef CONFIG_AUDIT
	do_one_head(audit_rule_init);
	do_one_head(audit_rule_known);
	do_one_head(audit_rule_match);
	do_one_head(audit_rule_free);
#endif /* CONFIG_AUDIT */
	err = crypto_shash_final(shash, (u8 *)hash);
	rs_log_debug(RSCAN_LOG_TAG, "sescan result %d", err);

	crypto_free_shash(tfm);
	return err;
}
