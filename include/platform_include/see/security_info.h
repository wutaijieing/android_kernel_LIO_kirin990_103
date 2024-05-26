/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: security info head file
 * Create: 2021/7/15
 */
#ifndef __SECURITY_INFO_H__
#define __SECURITY_INFO_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include <platform_include/see/uapp.h>

#define SHA256_BYTES                     0x20
#define BYPASS_NET_CERT_MAX_BYTES        1024
#define DIEID_HASH_MAX_BYTES             0x40
#define SECURITY_INFO_IOCTL_MAGIC        0xe

#define SECURUTY_INFO_VERIFY_BYPASS_NET_CERT \
	_IOW(SECURITY_INFO_IOCTL_MAGIC, 3, struct bypass_net_cert)

#define SECURUTY_INFO_GET_ROTPK_HASH \
	_IOW(SECURITY_INFO_IOCTL_MAGIC, 4, struct rotpk_hash)

#define SECURUTY_INFO_GET_UAPP_BINDFILE_POS \
	_IOW(SECURITY_INFO_IOCTL_MAGIC, 5, struct uapp_bindfile_pos)

#define SECURUTY_INFO_GET_UAPP_EMPOWER_CERT_POS \
	_IOW(SECURITY_INFO_IOCTL_MAGIC, 6, struct uapp_file_pos)

#define SECURUTY_INFO_GET_DIEID_HASH \
	_IOR(SECURITY_INFO_IOCTL_MAGIC, 7, struct dieid_hash)

#define SECURUTY_INFO_GET_UAPP_ENABLE_STATE \
	_IOW(SECURITY_INFO_IOCTL_MAGIC, 9, u32)

#define SECURUTY_INFO_SET_UAPP_ENABLE_STATE \
	_IOR(SECURITY_INFO_IOCTL_MAGIC, 10, u32)

#define SECURUTY_INFO_VALID_UAPP_BINDFILE_PUBKEY \
	_IOR(SECURITY_INFO_IOCTL_MAGIC, 11, u32)

#define SECURUTY_INFO_REVOKE_UAPP_BINDFILE_PUBKEY \
	_IOR(SECURITY_INFO_IOCTL_MAGIC, 12, u32)

#define security_info_err(fmt, ...)        \
	pr_err("[security_info]: <%s> "fmt, __func__, ##__VA_ARGS__)
#define security_info_info(fmt, ...)       \
	pr_info("[security_info]: <%s> "fmt, __func__, ##__VA_ARGS__)


struct bypass_net_cert {
	u32 cert_bytes;
	u8 cert[BYPASS_NET_CERT_MAX_BYTES];
};

struct dieid_hash {
	u32 hash_bytes;
	u8 hash[DIEID_HASH_MAX_BYTES];
};

struct ioctl_security_info_func_t {
	u32 cmd;
	char *name;
	s32 (*func)(u32 cmd, uintptr_t arg);
};

/* secboot CA */
s32 teek_init(TEEC_Session *session, TEEC_Context *context);

s32 security_info_verify_bypass_net_cert(u32 cmd, uintptr_t arg);
s32 security_info_get_uapp_enable_state(u32 cmd, uintptr_t arg);
s32 security_info_set_uapp_enable_state(u32 cmd, uintptr_t arg);
s32 security_info_valid_uapp_bindfile_pubkey(u32 cmd, uintptr_t arg);
s32 security_info_revoke_uapp_bindfile_pubkey(u32 cmd, uintptr_t arg);
s32 security_info_compute_sha256(u8 *msg, u32 msg_len, u8 *digst, u32 digst_len);
s32 security_info_get_uapp_empower_cert_pos(u32 cmd, uintptr_t arg);
s32 security_info_get_rotpk_hash(u32 cmd, uintptr_t arg);
s32 security_info_get_uapp_bindfile_pos(u32 cmd, uintptr_t arg);
#endif /* __SECURITY_INFO_H__ */
