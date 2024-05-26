/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Extend functions for per-f2fs sdp(sensitive data protection).
 * Create: 2017-11-10
 * History: 2020-10-10 add hwdps
 */

#ifndef __HMFS_SDP_H__
#define __HMFS_SDP_H__

#ifdef CONFIG_HMFS_KIRIN_ENCRYPTION
/* the config is enable in hisi */
#ifndef CONFIG_SDP_ENCRYPTION
#define CONFIG_SDP_ENCRYPTION 1
#endif
#endif

#ifdef CONFIG_SDP_ENCRYPTION
/* sdp crypto policy */
struct fscrypt_sdp_policy {
	__u8 version;
	__u8 sdpclass;
	__u8 contents_encryption_mode;
	__u8 filenames_encryption_mode;
	__u8 flags;
	__u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
} __packed;

/* crypto policy type */
struct fscrypt_policy_type {
	__u8 version;
	__u8 encryption_type;
	__u8 contents_encryption_mode;
	__u8 filenames_encryption_mode;
	__u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
} __packed;

#ifdef CONFIG_HWDPS
#define RESERVED_DATA_LEN 8

/* dps crypto policy */
struct fscrypt_dps_policy {
	__u8 version;
	__u8 reserved[RESERVED_DATA_LEN];
} __packed;

#endif

#define F2FS_IOC_SET_SDP_ENCRYPTION_POLICY     _IOW(F2FS_IOCTL_MAGIC, 80,\
	struct fscrypt_sdp_policy)
#define F2FS_IOC_GET_SDP_ENCRYPTION_POLICY     _IOR(F2FS_IOCTL_MAGIC, 81, \
	struct fscrypt_sdp_policy)
#define F2FS_IOC_GET_ENCRYPTION_POLICY_TYPE    _IOR(F2FS_IOCTL_MAGIC, 82,\
	struct fscrypt_policy_type)
#ifdef CONFIG_HWDPS
#define F2FS_IOC_SET_DPS_ENCRYPTION_POLICY     _IOW(F2FS_IOCTL_MAGIC, 83, \
	struct fscrypt_dps_policy)
#endif

int fscrypt_ioctl_set_sdp_policy(struct file *filp,
	const void __user *arg);

int fscrypt_ioctl_get_sdp_policy(struct file *filp,
	void __user *arg);

int fscrypt_ioctl_get_policy_type(struct file *filp,
	void __user *arg);

#ifdef CONFIG_HWDPS
int fscrypt_ioctl_set_dps_policy(struct file *filp,
	const void __user *arg);
#endif

int fscrypt_check_sdp_encrypted(struct inode *inode);

int fscrypt_sdp_crypt_inherit(struct inode *parent, struct inode *child,
	void *dpage, void *fs_data);

int fscrypt_get_sdp_crypt_keyinfo(struct inode *inode, void *fs_data, int *flag);
#endif
#endif

