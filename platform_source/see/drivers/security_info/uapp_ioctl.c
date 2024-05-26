/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: uapp provide service for user via ioctl
 * Create: 2022/04/20
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <platform_include/see/security_info.h>
#include <platform_include/see/uapp.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <securec.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/compat.h>
#endif

s32 security_info_get_uapp_enable_state(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	u32 state = 0;
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (argp == NULL) {
		pr_err("error, argp is NULL\n");
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_GET_UAPP_ENABLE_STATE) {
		pr_err("error, cmd=0x%08x\n", cmd);
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(state)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	ret = uapp_get_enable_state(&state);
	if (ret != UAPP_OK) {
		pr_err("error, get uapp enable state ret=0x%08X\n", ret);
		return -EFAULT;
	}

	ret = (u32)copy_to_user(argp, &state, sizeof(state));
	if (ret != 0) {
		pr_err("error, copy_to uapp enable state ret=0x%08X\n", ret);
		return -EFAULT;
	}

	return 0;
}

s32 security_info_set_uapp_enable_state(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	u32 state;
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (argp == NULL) {
		pr_err("error, argp is NULL\n");
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_SET_UAPP_ENABLE_STATE) {
		pr_err("error, cmd=0x%08x\n", cmd);
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(state)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	ret = (u32)copy_from_user(&state, argp, sizeof(state));
	if (ret != 0) {
		pr_err("error, copy_from uapp enable state ret=0x%08X\n", ret);
		return -EFAULT;
	}

	ret = uapp_set_enable_state(state);
	if (ret != UAPP_OK) {
		pr_err("error, set uapp enable state ret=0x%08X\n", ret);
		return -EFAULT;
	}

	return 0;
}

s32 security_info_valid_uapp_bindfile_pubkey(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	u32 key_idx = 0;
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (argp == NULL) {
		pr_err("error, argp is NULL\n");
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(key_idx)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_VALID_UAPP_BINDFILE_PUBKEY) {
		pr_err("error, cmd=0x%08x\n", cmd);
		return -EFAULT;
	}

	ret = (u32)copy_from_user(&key_idx, argp, sizeof(key_idx));
	if (ret != 0) {
		pr_err("error, copy_from uapp valid key_idx ret=0x%08X\n", ret);
		return -EFAULT;
	}

	ret = uapp_valid_bindfile_pubkey(key_idx);
	if (ret != UAPP_OK) {
		pr_err("error, uapp_valid_bindfile_pubkey ret=0x%08X\n", ret);
		return -EFAULT;
	}

	return 0;
}

s32 security_info_revoke_uapp_bindfile_pubkey(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	u32 key_idx = 0;
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (argp == NULL) {
		pr_err("error, argp is NULL\n");
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_REVOKE_UAPP_BINDFILE_PUBKEY) {
		pr_err("error, cmd=0x%08x\n", cmd);
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(key_idx)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	ret = (u32)copy_from_user(&key_idx, argp, sizeof(key_idx));
	if (ret != 0) {
		pr_err("error, copy_from uapp key_idx ret=0x%08X\n", ret);
		return -EFAULT;
	}

	ret = uapp_revoke_bindfile_pubkey(key_idx);
	if (ret != UAPP_OK) {
		pr_err("error, uapp_valid_bindfile_pubkey ret=0x%08X\n", ret);
		return -EFAULT;
	}

	return 0;
}

s32 security_info_get_uapp_bindfile_pos(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	struct uapp_bindfile_pos bindfile = {{0}, {0}};
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (!argp) {
		pr_err("error, argp is NULL\n");
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_GET_UAPP_BINDFILE_POS) {
		pr_err("error, cmd=0x%08x\n", cmd);
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(bindfile)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	ret = uapp_get_bindfile_pos(&bindfile);
	if (ret != UAPP_OK) {
		pr_err("error 0x%x, get uapp bindfile info\n", ret);
		return -EFAULT;
	}

	ret = (u32)copy_to_user(argp, &bindfile, sizeof(bindfile));
	if (ret != 0) {
		pr_err("error 0x%x, copy bindfile to userland\n", ret);
		return -EFAULT;
	}

	return 0;
}

s32 security_info_get_uapp_empower_cert_pos(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	struct uapp_file_pos empower_cert = { 0 };
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (!argp) {
		pr_err("error, argp is NULL\n");
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_GET_UAPP_EMPOWER_CERT_POS) {
		pr_err("error, cmd=0x%08x\n", cmd);
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(empower_cert)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	ret = uapp_get_empower_cert_pos(&empower_cert);
	if (ret != UAPP_OK) {
		pr_err("error 0x%x, get uapp empower cert pos\n", ret);
		return -EFAULT;
	}

	ret = (u32)copy_to_user(argp, &empower_cert, sizeof(empower_cert));
	if (ret != 0) {
		pr_err("error 0x%x, copy empower_cert pos to userland\n", ret);
		return -EFAULT;
	}

	return 0;
}

s32 security_info_get_rotpk_hash(u32 cmd, uintptr_t arg)
{
	u32 ret;
	void __user *argp = (void __user *)arg;
	struct rotpk_hash hash = {0};
	uint32_t size_in_cmd = _IOC_SIZE(cmd);

	/* if declare and define argp, static check warning */
	if (argp == NULL) {
		security_info_err("argp is NULL\n");
		return -EFAULT;
	}

	if (cmd != SECURUTY_INFO_GET_ROTPK_HASH) {
		security_info_err("error, cmd 0x%08x\n", cmd);
		return -EFAULT;
	}

	if (size_in_cmd != sizeof(hash)) {
		pr_err("error, size_in_cmd=0x%08x\n", size_in_cmd);
		return -EFAULT;
	}

	ret = uapp_get_rotpk_hash(&hash);
	if (ret != 0) {
		security_info_err("error 0x%x, get rotpk_hash\n", ret);
		return -EFAULT;
	}

	ret = (u32)copy_to_user(argp, &hash, sizeof(hash));
	if (ret != 0) {
		security_info_err("error 0x%x,copy rotpk_hash to userland\n", ret);
		return -EFAULT;
	}

	return 0;
}
