/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: filesystem cryption extra policy
 * Author: 	laixinyi, hebiao
 * Create: 	2020-12-16
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/fscrypt_common.h>
#include <linux/random.h>
#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/syscalls.h>
#include <platform_include/see/fbe_ctrl.h>

#include "keyinfo_sdp.h"
#include "fbe3_trace.h"

#define FPUBKEY_LEN PUBKEY_LEN

/*
 * In FBE3, regular files' nonce is generated by 2 segments: 48 bytes for
 * ci_key, 16 bytes for metadata; directory files' nonce is generated as usual.
 */
int fscrypt_generate_metadata_nonce(u8 *nonce, struct inode *inode, size_t len)
{
	if (unlikely(!inode->i_sb->s_cop))
		return -EACCES;

	/* Filesytem doesn't support metadata */
	if (!inode->i_sb->s_cop->get_metadata_context ||
	    !inode->i_sb->s_cop->set_ex_metadata_context ||
	    !inode->i_sb->s_cop->get_ex_metadata_context)
		return -EPERM;
	if (!S_ISREG(inode->i_mode) ||
	    !inode->i_sb->s_cop->is_inline_encrypted ||
	    !inode->i_sb->s_cop->is_inline_encrypted(inode))
		return -EPERM;
	get_random_bytes(nonce, CI_KEY_LEN_NEW);
	get_random_bytes(nonce + CI_KEY_LEN_NEW, METADATA_BYTE_IN_KDF);
	return 0;
}

static int fscrypt_ece_metadata_create(u8 *metadata)
{
	ktime_t start_time, end_time;
	uint64_t get_meta_delay = 0;
	int ret;

	if (unlikely(!metadata)) {
		pr_err("%s: invalid metadata\n", __func__);
		return -EINVAL;
	}

	start_time = ktime_get();
	ret = get_metadata(metadata, METADATA_BYTE_IN_KDF);
	end_time = ktime_get();
	if (unlikely(ret)) {
		if (end_time >= start_time) {
			get_meta_delay = ktime_to_ms(end_time - start_time);
			gen_meta_upload_bigdata(ECE_GEN_METADATA, ret,
						get_meta_delay);
		}
		pr_err("%s: get ece metadata failed %d, delay %lu ms", __func__,
		       ret, get_meta_delay);
	}

	return ret;
}

static int fscrypt_sece_metadata_create(u8 *metadata, u8 *fpubkey, u32 keyindex,
					int flag)
{
	ktime_t start_time, end_time;
	uint64_t get_meta_delay = 0;
	int ret;
	struct fbex_sece_param param = { 0 };

	if (unlikely(!metadata || !fpubkey)) {
		pr_err("%s: invalid metadata\n", __func__);
		return -EINVAL;
	}
	param.cmd = flag;
	param.idx = keyindex;
	param.pubkey = fpubkey;
	param.key_len = FPUBKEY_LEN;
	param.metadata = metadata;
	param.iv_len = METADATA_BYTE_IN_KDF;

	start_time = ktime_get();
	ret = get_metadata_sece(&param);
	end_time = ktime_get();
	if (unlikely(ret)) {
		if (end_time >= start_time) {
			get_meta_delay = ktime_to_ms(end_time - start_time);
			gen_meta_upload_bigdata(SECE_GEN_METADATA, ret,
						get_meta_delay);
		}
		pr_err("%s: get sece metadata failed %d, delay %lu ms",
		       __func__, ret, get_meta_delay);
	}

	return ret;
}

int fscrypt_get_metadata(struct inode *inode, struct fscrypt_info *ci_info)
{
	int res;
	const struct fscrypt_operations *cop = inode->i_sb->s_cop;

	if (unlikely(!ci_info || !ci_info->ci_key)) {
		pr_err("%s: no ci_info for inline-encrypted file!\n", __func__);
		return -EINVAL;
	}

	/* Filesytem doesn't support metadata */
	if (!cop->get_metadata_context)
		return -EOPNOTSUPP;

	res = cop->get_metadata_context(inode, ci_info->ci_metadata,
					METADATA_BYTE_IN_KDF, NULL);
	/*
	* if getting metadata failed, this is a new file after hota
	* update. Therefore, we reuse the last 16 Bytes of ci_key as
	* metadata. if not, this is an old file, we keep the old method
	*/
	if (res == -ENODATA) {
		memcpy(ci_info->ci_metadata, ci_info->ci_key + CI_KEY_LEN_NEW,
		       METADATA_BYTE_IN_KDF);
		ci_info->ci_key_len = CI_KEY_LEN_NEW;
	} else if (res < 0) {
		pr_err("%s: inode %lu get metadata failed, res %d\n", __func__,
		       inode->i_ino, res);
		return res;
	} else if (res != METADATA_BYTE_IN_KDF) {
		pr_err("%s: inode %lu metadata invalid length: %d\n", __func__,
		       inode->i_ino, res);
		return -EINVAL;
	}

	ci_info->ci_key_index |= (CD << FILE_ENCRY_TYPE_BEGIN_BIT);
	return 0;
}

int fscrypt_new_ece_metadata(struct inode *inode, struct fscrypt_info *ci_info,
			     void *fs_data)
{
	int res;

	/* Filesytem doesn't support metadata */
	if (!inode->i_sb->s_cop->set_ex_metadata_context ||
	    !inode->i_sb->s_cop->get_ex_metadata_context)
		return 0;

	res = fscrypt_ece_metadata_create(ci_info->ci_metadata);
	if (unlikely(res)) {
		pr_err("%s: ece metadata create fail! \n", __func__);
		return res;
	}

	res = inode->i_sb->s_cop->set_ex_metadata_context(
		inode, ci_info->ci_metadata, METADATA_BYTE_IN_KDF, fs_data);
	if (unlikely(res)) {
		pr_err("%s: ece metadata set fail! \n", __func__);
		return res;
	}

	ci_info->ci_key_index &= FILE_ENCRY_TYPE_MASK;
	ci_info->ci_key_index |= (ECE << FILE_ENCRY_TYPE_BEGIN_BIT);
	return res;
}

int fscrypt_get_ece_metadata(struct inode *inode, struct fscrypt_info *ci_info,
			     void *fs_data, bool create)
{
	int res;

	/* Filesytem doesn't support metadata */
	if (!inode->i_sb->s_cop->set_ex_metadata_context ||
	    !inode->i_sb->s_cop->get_ex_metadata_context)
		return 0;

	if (create)
		return fscrypt_new_ece_metadata(inode, ci_info, fs_data);

	res = inode->i_sb->s_cop->get_ex_metadata_context(
		inode, ci_info->ci_metadata, METADATA_BYTE_IN_KDF, fs_data);
	if (unlikely(res != METADATA_BYTE_IN_KDF)) {
		pr_err("%s: inode(%lu) get ece metadata failed, res %d\n",
		       __func__, inode->i_ino, res);
		return -EINVAL;
	}
	ci_info->ci_key_index &= FILE_ENCRY_TYPE_MASK;
	ci_info->ci_key_index |= (ECE << FILE_ENCRY_TYPE_BEGIN_BIT);
	return 0;
}

int fscrypt_new_sece_metadata(struct inode *inode, struct fscrypt_info *ci_info,
			      void *fs_data)
{
	int res;
	u8 fpubkey[FPUBKEY_LEN] = { 0 };

	/* Filesytem doesn't support metadata */
	if (!inode->i_sb->s_cop->set_ex_metadata_context ||
	    !inode->i_sb->s_cop->get_ex_metadata_context)
		return 0;

	res = fscrypt_sece_metadata_create(ci_info->ci_metadata, fpubkey,
					   ci_info->ci_key_index,
					   SEC_FILE_ENCRY_CMD_ID_NEW_SECE);
	if (unlikely(res)) {
		pr_err("%s: sece metadata create failed, res:%d\n", __func__,
		       res);
		return -EKEYREJECTED;
	}

	res = inode->i_sb->s_cop->set_ex_metadata_context(inode, fpubkey,
							  FPUBKEY_LEN, fs_data);
	if (unlikely(res)) {
		pr_err("%s: inode(%lu) set metadata failed, res %d\n", __func__,
		       inode->i_ino, res);
		return res;
	}

	ci_info->ci_key_index &= FILE_ENCRY_TYPE_MASK;
	ci_info->ci_key_index |= (SECE << FILE_ENCRY_TYPE_BEGIN_BIT);
	return 0;
}

int fscrypt_retrieve_sece_metadata(struct inode *inode,
				   struct fscrypt_info *ci_info, void *fs_data)
{
	u8 fpubkey[FPUBKEY_LEN] = { 0 };
	u8 ci_metadata[METADATA_BYTE_IN_KDF] = { 0 };
	u32 keyindex;
	int res;

	/* Filesytem doesn't support metadata */
	if (!inode->i_sb->s_cop->set_ex_metadata_context ||
	    !inode->i_sb->s_cop->get_ex_metadata_context)
		return 0;

	res = inode->i_sb->s_cop->get_ex_metadata_context(inode, fpubkey,
							  FPUBKEY_LEN, fs_data);
	if (unlikely(res != FPUBKEY_LEN)) {
		pr_err("%s: inode(%lu) get sece fpubkey failed, res %d\n",
		       __func__, inode->i_ino, res);
		return -EINVAL;
	}

	keyindex = (u32)ci_info->ci_key_index & FILE_ENCRY_TYPE_MASK;
	res = fscrypt_sece_metadata_create(ci_metadata, fpubkey, keyindex,
					   SEC_FILE_ENCRY_CMD_ID_GEN_METADATA);
	if (unlikely(res)) {
		pr_err("%s: screen locked, inode(%lu) get sece metadata failed, res:%d\n",
		       __func__, inode->i_ino, res);
		return -EKEYREVOKED;
	}

	if (unlikely(memcmp(ci_metadata, ci_info->ci_metadata,
			    METADATA_BYTE_IN_KDF))) {
		pr_err("%s: inode(%lu) metadata doesn't match %d\n", __func__,
		       inode->i_ino, res);
		return -EIO;
	}

	return 0;
}

int fscrypt_get_sece_metadata(struct inode *inode, struct fscrypt_info *ci_info,
			      void *fs_data, bool create)
{
	u8 fpubkey[FPUBKEY_LEN] = { 0 };
	int keyindex;
	int res;

	if (create)
		return fscrypt_new_sece_metadata(inode, ci_info, fs_data);

	/* Filesytem doesn't support metadata */
	if (!inode->i_sb->s_cop->set_ex_metadata_context ||
	    !inode->i_sb->s_cop->get_ex_metadata_context)
		return 0;

	res = inode->i_sb->s_cop->get_ex_metadata_context(inode, fpubkey,
							  FPUBKEY_LEN, fs_data);
	if (unlikely(res != FPUBKEY_LEN)) {
		pr_err("%s: inode(%lu) get sece fpubkey failed, res %d\n",
		       __func__, inode->i_ino, res);
		return -EINVAL;
	}

	keyindex = ci_info->ci_key_index & FILE_ENCRY_TYPE_MASK;
	res = fscrypt_sece_metadata_create(ci_info->ci_metadata, fpubkey,
					   keyindex,
					   SEC_FILE_ENCRY_CMD_ID_GEN_METADATA);
	if (unlikely(res)) {
		pr_err("%s: screen locked, inode(%lu) get sece metadata failed, res:%d\n",
		       __func__, inode->i_ino, res);
		return -EKEYREVOKED;
	}

	ci_info->ci_key_index &= FILE_ENCRY_TYPE_MASK;
	ci_info->ci_key_index |= (SECE << FILE_ENCRY_TYPE_BEGIN_BIT);
	return 0;
}

bool fscrypt_encrypt_file_check(struct inode *inode)
{
	struct fscrypt_info *ci_info = NULL;

	if (unlikely(!inode))
		return false;

	ci_info = inode->i_crypt_info;

	/* If ECE && Lock Screen return true, otherwise, return false */
	if (ci_info) {
		if ((ci_info->ci_hw_enc_flag == FSCRYPT_SDP_ECE_ENABLE_FLAG) &&
		    check_keyring_for_sdp(
			    ci_info->ci_policy.v1.master_key_descriptor, 0)) {
			pr_err("%s: ECE file check sdp keyring failed! \n",
			       __func__);
			return true;
		}
	}
	return false;
}

static enum encrypto_type fscrypt_get_encrypt_type(struct inode *inode)
{
	struct fscrypt_info *ci_info = inode->i_crypt_info;
	if (!ci_info)
		return PLAIN;
	if (ci_info->ci_hw_enc_flag == FSCRYPT_SDP_ECE_ENABLE_FLAG)
		return ECE;
	if (ci_info->ci_hw_enc_flag == FSCRYPT_SDP_SECE_ENABLE_FLAG)
		return SECE;
	return CD;
}

int fscrypt_vm_op_check(struct inode *inode)
{
	if (!inode)
		return 0;

	if (unlikely(fscrypt_encrypt_file_check(inode))) {
		pr_err("%s:fscrypt_encrypt_file_check intercept! \n", __func__);
		WARN_ON(1);
		return VM_FAULT_SIGSEGV;
	}
	return 0;
}

int fscrypt_lld_protect(const struct request *request)
{
	int err = 0;
	bool is_encrypt_file = request->ci_key ? true : false;

	if (is_encrypt_file) {
		int file_encrypt_type =
			(u32)request->ci_key_index >> FILE_ENCRY_TYPE_BEGIN_BIT;
		u32 slot_id = (u32)request->ci_key_index & FILE_ENCRY_TYPE_MASK;

		if (file_encrypt_type == ECE) {
			if (fbex_slot_clean(slot_id)) {
				pr_err("[FBE3]%s: key slot is cleared, illegal to send ECE IO\n",
				       __func__);
				err = -EOPNOTSUPP;
			}
		}
	}

	return err;
}

static spinlock_t fe_rw_check_lock;
static int fe_key_file_rw_count;
static struct completion g_fbex_rw_completion;
static void fbe_clear_key(u32 user)
{
	int ret;
	pr_err("[FBE3]%s for user 0x%x!!", __func__, user);

	ret = fbex_ca_lock_screen(user, FILE_ECE);
	if (ret != 0) {
		pr_err("%s, lock key error, %d\n", __func__, ret);
		return;
	}
	fbex_set_status(user, true);
}

static u32 fbe_restore_key(u32 user, u32 file, u8 *iv, u32 iv_len)
{
	int ret;

	pr_err("[FBE3]%s for user 0x%x!!", __func__, user);
	ret = fbex_ca_unlock_screen(user, file, iv, iv_len);
	if (ret != 0) {
		pr_err("%s, unlock key error, %d\n", __func__, ret);
		return ret;
	}
	fbex_set_status(user, false);
	return ret;
}

#define RW_CNT_TIMEOUT 1000
#define USER_NUM 5

struct fbe_user_screen_event {
	int32_t uid;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct delayed_work fbe_screen_work;
#else
	struct work_struct work;
#endif
	ktime_t start;
	ktime_t end;
	struct completion clear_key_completion;
};

static struct fbe_user_screen_event *g_screen_event_table;

static void fe_do_key_protect(struct work_struct *work)
{
	bool rw_inflight = false;
	struct fbe_user_screen_event *screen_event = NULL;
	u32 user;

	if (!work) {
		pr_err("[FBE3]%s: invalid work!", __func__);
		return;
	}

	pr_err("[FBE3]%s: rw check and sync begin!", __func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	screen_event = container_of(work, struct fbe_user_screen_event,
				    fbe_screen_work.work);
#else
	screen_event = container_of(work, struct fbe_user_screen_event, work);
#endif
	user = screen_event->uid;

	spin_lock(&fe_rw_check_lock);
	rw_inflight = !!fe_key_file_rw_count;
	spin_unlock(&fe_rw_check_lock);
	if (rw_inflight) {
		pr_err("%s:waiting for rw count 0", __func__);
		if (!wait_for_completion_timeout(
			    &g_fbex_rw_completion,
			    msecs_to_jiffies(RW_CNT_TIMEOUT))) {
			pr_err("%s wait for 0 rwcnt time out\n", __func__);
			return;
		}
		pr_err("%s: waiting completed!\n", __func__);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ksys_sync();
#else
	sys_sync();
#endif

	screen_event->end = ktime_get();
	if (screen_event->end < screen_event->start)
		return;

	if (ktime_to_ms(screen_event->end - screen_event->start) <=
	    RW_CNT_TIMEOUT) {
		spin_lock(&fe_rw_check_lock);
		rw_inflight = !!fe_key_file_rw_count;
		spin_unlock(&fe_rw_check_lock);
		if (!rw_inflight) {
			fbe_clear_key(user);
			complete(&screen_event->clear_key_completion);
			pr_err("[FBE]%s: cleared ece key for user 0x%x",
			       __func__, user);
			return;
		}
	}
}

static void fe_active_key_protect(u32 user)
{
	int i;

	if (!g_screen_event_table) {
		pr_err("[FBE3]%s: no global screen event table!", __func__);
		return;
	}

	for (i = 0; i < USER_NUM; i++) {
		if (g_screen_event_table[i].uid == user) {
			break;
		} else if (g_screen_event_table[i].uid < 0) {
			g_screen_event_table[i].uid = (int32_t)user;
			break;
		}
	}

	if (i == USER_NUM) {
		pr_err("[FBE3]%s: wrong uid 0x%x", __func__, user);
		return;
	}

	g_screen_event_table[i].start = ktime_get();

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	queue_delayed_work(system_wq,
			   &(g_screen_event_table[i].fbe_screen_work), 0);
#else
	schedule_work(&(g_screen_event_table[i].work));
#endif
}

void fbe3_lock_in(u32 user)
{
	pr_err("user 0x%x %s!\n", user, __func__);
	fe_active_key_protect(user);
}

u32 fbe3_unlock_in(u32 user, u32 file, u8 *iv, u32 iv_len)
{
	int i;
	unsigned long time_out;
	u32 ret = 0;

	pr_err("user 0x%x %s!\n", user, __func__);

	if (!g_screen_event_table) {
		pr_err("[FBE3]%s: no global screen event table!", __func__);
		goto restore_key;
	}

	for (i = 0; i < USER_NUM; i++) {
		if (g_screen_event_table[i].uid == user) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			if (cancel_delayed_work(
				    &g_screen_event_table[i].fbe_screen_work))
#else
			if (cancel_work(&g_screen_event_table[i].work))
#endif
				return ret;
			else
				break;
		}
	}

	if (i == USER_NUM) {
		pr_err("[FBE3]wrong uid 0x%x", user);
		goto restore_key;
	}

	time_out = wait_for_completion_timeout(
		&g_screen_event_table[i].clear_key_completion,
		msecs_to_jiffies(RW_CNT_TIMEOUT));
	pr_err("[FBE3]time_out left for user 0x%x is %u", user,
	       jiffies_to_msecs(time_out));

restore_key:
	ret = fbe_restore_key(user, file, iv, iv_len);
	if (unlikely(ret)) {
		pr_err("[FBE3]%s, unlock key error, %d\n", __func__, ret);
		return ret;
	}

	return ret;
}

static bool is_file_ece(struct inode *inode)
{
	if (inode && fscrypt_get_encrypt_type(inode) == ECE)
		return true;
	else
		return false;
}

int rw_begin(struct file *file)
{
	struct inode *inode = NULL;
	int ret = 0;

	if (file->f_mapping)
		inode = file->f_mapping->host;
	if (is_file_ece(inode)) {
		int lock_check = fscrypt_encrypt_file_check(inode);
		spin_lock(&fe_rw_check_lock);
		if (lock_check) {
			pr_err("[FBE3]%s: rw for %lu is blocked when screen locked\n",
			       __func__, inode->i_ino);
			ret = -EKEYREVOKED;
		} else {
			fe_key_file_rw_count++;
			reinit_completion(&g_fbex_rw_completion);
		}
		spin_unlock(&fe_rw_check_lock);
	}

	return ret;
}

void rw_finish(int read_write, struct file *file)
{
	struct inode *inode = NULL;

	if (file->f_mapping)
		inode = file->f_mapping->host;
	if (is_file_ece(inode)) {
		spin_lock(&fe_rw_check_lock);
		if (!fe_key_file_rw_count) {
			pr_err("[FBE3]%s: no match of rw count\n", __func__);
#ifdef CONFIG_DFX_DEBUG_FS
			BUG();
#endif
		}

		if (!(--fe_key_file_rw_count))
			complete_all(&g_fbex_rw_completion);
		spin_unlock(&fe_rw_check_lock);
	}
}

static int __init fscrypt_extra_ctrl_init(void)
{
	int i;

	init_completion(&g_fbex_rw_completion);
	spin_lock_init(&fe_rw_check_lock);
	g_screen_event_table = kmalloc(
		sizeof(struct fbe_user_screen_event) * USER_NUM, GFP_KERNEL);
	if (!g_screen_event_table)
		return -ENOMEM;

	memset(g_screen_event_table, 0,
	       sizeof(struct fbe_user_screen_event) * USER_NUM);

	/* Initialization of user table */
	for (i = 0; i < USER_NUM; i++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		INIT_DELAYED_WORK(&(g_screen_event_table[i].fbe_screen_work),
				  fe_do_key_protect);
#else
		INIT_WORK(&(g_screen_event_table[i].work), fe_do_key_protect);
#endif
		init_completion(&g_screen_event_table[i].clear_key_completion);
		g_screen_event_table[i].uid = -1;
	}

	return 0;
}
fs_initcall(fscrypt_extra_ctrl_init);