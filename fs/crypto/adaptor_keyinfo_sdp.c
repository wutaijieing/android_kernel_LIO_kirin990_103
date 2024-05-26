// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * Description: Implementation of 1) fill fscyrpt_info for sdp;
 *                                2) setup inline crypt or soft crypt for sdp.
 * Create: 2022-02-20
 */

#include "adaptor_keyinfo_sdp.h"
#include <uapi/linux/keyctl.h>
#include <keys/user-type.h>
#include <securec.h>

static struct fscrypt_mode sdp_mode = {
	.friendly_name = "AES-256-XTS",
	.cipher_str = "xts(aes)",
	.keysize = FSCRYPT_MAX_KEY_SIZE,
	.ivsize = FS_KEY_IV_SIZE,
	.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_256_XTS
};

void put_crypt_info_sdp(struct fscrypt_info *ci)
{
	struct key *key = NULL;
	void *prev = NULL;
	void *ci_key = NULL;

	if (!ci)
		return;

	if (ci->ci_direct_key)
		fscrypt_put_direct_key(ci->ci_direct_key);
	else if (ci->ci_owns_key)
		fscrypt_destroy_prepared_key(&ci->ci_enc_key);

	key = ci->ci_master_key;
	if (key) {
		struct fscrypt_master_key *mk = key->payload.data[0];

		/*
		 * Remove this inode from the list of inodes that were unlocked
		 * with the master key.
		 *
		 * In addition, if we're removing the last inode from a key that
		 * already had its secret removed, invalidate the key so that it
		 * gets removed from ->s_master_keys.
		 */
		spin_lock(&mk->mk_decrypted_inodes_lock);
		list_del(&ci->ci_master_key_link);
		spin_unlock(&mk->mk_decrypted_inodes_lock);
		if (refcount_dec_and_test(&mk->mk_refcount))
			key_invalidate(key);
		key_put(key);
	}

	ci_key = READ_ONCE(ci->ci_key);
	prev = cmpxchg(&ci->ci_key, ci_key, NULL);
	if (prev == ci_key && ci_key) {
		memzero_explicit(ci_key, (size_t)FSCRYPT_MAX_KEY_SIZE);
		kfree(ci_key);
		ci->ci_key_len = 0;
		ci->ci_key_index = -1;
	}
	if (ci->ci_gtfm)
		crypto_free_aead(ci->ci_gtfm);

	memzero_explicit(ci, sizeof(*ci));
	kmem_cache_free(fscrypt_info_cachep, ci);
}

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
static int do_get_keyindex(u8 *descriptor, int *keyindex)
{
	int res = -EKEYREVOKED;
	struct key *keyring_key = NULL;
	const struct user_key_payload *ukp = NULL;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	struct fscrypt_sdp_key *master_key = NULL;
#else
	struct fscrypt_key *master_key = NULL;
#endif

	if (!descriptor || !keyindex)
		return res;

	keyring_key = fscrypt_request_key(descriptor, FSCRYPT_KEY_DESC_PREFIX,
		FSCRYPT_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key))
		return PTR_ERR(keyring_key);

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon)
		goto out;

	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) /* key was revoked before we acquired its semaphore */
		goto out;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	if (ukp->datalen == sizeof(struct fscrypt_sdp_key)) {
		master_key = (struct fscrypt_sdp_key *)ukp->data;
		if (master_key->size == FS_AES_256_GCM_KEY_SIZE) {
			*keyindex = (int)(*(master_key->raw +
			FS_KEY_INDEX_OFFSET) & 0xff);
			res = 0;
		}
	}
#else
	if (ukp->datalen == sizeof(struct fscrypt_key)) {
		master_key = (struct fscrypt_key *)ukp->data;
		if (master_key->size == FS_AES_256_GCM_KEY_SIZE) {
			*keyindex = (int)(*(master_key->raw +
			FS_KEY_INDEX_OFFSET) & 0xff);
			res = 0;
		}
	}
#endif
out:
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return res;
}
#endif

static int fscrypt_policy_from_sdp_context(union fscrypt_policy *policy_u,
	const struct fscrypt_sdp_context *ctx, int ctx_size)
{
	struct fscrypt_policy_v1 *policy = NULL;

	if (ctx->contents_encryption_mode != FSCRYPT_MODE_AES_256_XTS ||
		ctx->filenames_encryption_mode != FSCRYPT_MODE_AES_256_CTS)
		return -EINVAL;

	policy = &policy_u->v1;

	policy->version = FSCRYPT_POLICY_V1;
	policy->contents_encryption_mode =
		ctx->contents_encryption_mode;
	policy->filenames_encryption_mode =
		ctx->filenames_encryption_mode;
	policy->flags = ctx->flags;
	(void)memcpy_s(policy->master_key_descriptor, sizeof(policy->master_key_descriptor),
		ctx->master_key_descriptor, sizeof(ctx->master_key_descriptor));
	return 0;
}

static int set_encryption_params(struct fscrypt_info *crypt_info,
	union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx,
	const u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE])
{
	int res;
	int keyindex;
	struct crypto_skcipher *tfm = NULL;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = do_get_keyindex(sdp_ctx->master_key_descriptor, &keyindex);
#else
	res = do_get_keyindex(ctx->v1.master_key_descriptor, &keyindex);
#endif
	if (res != 0) {
		pr_err("%s: get keyindex key fail, res:%d!\n", __func__, res);
		return res;
	}
	crypt_info->ci_key_index = keyindex;
	if (crypt_info->ci_key_index < 0 || crypt_info->ci_key_index >
		MAX_UFS_SLOT_INDEX) {
		pr_err("%s: key index:%d\n", __func__,
			crypt_info->ci_key_index);
		return -EINVAL;
	}
#endif
	if (!crypt_info->ci_key) {
		crypt_info->ci_key = kzalloc((size_t)FSCRYPT_MAX_KEY_SIZE, GFP_NOFS);
		if (!crypt_info->ci_key)
			return -ENOMEM;
	}
	crypt_info->ci_key_len = FSCRYPT_MAX_KEY_SIZE;
	(void)memcpy_s(crypt_info->ci_key, FSCRYPT_MAX_KEY_SIZE, fek, FSCRYPT_MAX_KEY_SIZE);
	res = fscrypt_select_encryption_impl(crypt_info, false);
	if (res != 0) {
		pr_err("%s:select encryption key fail, res:%d!\n", __func__, res);
		return res;
	}
	tfm = fscrypt_allocate_skcipher(crypt_info->ci_mode, fek, crypt_info->ci_inode);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	/* Pairs with the smp_load_acquire() in fscrypt_is_key_prepared() */
	smp_store_release(&(crypt_info->ci_enc_key.tfm), tfm);
	return res;
}

int init_crypt_info_from_context(struct inode *inode,
	struct fscrypt_info **crypt_info,
	const struct fscrypt_sdp_context *ctx)
{
	int res;

	if (!inode || !crypt_info || *crypt_info != NULL || !ctx)
		return -EINVAL;
	res = fscrypt_initialize(inode->i_sb->s_cop->flags);
	if (res != 0)
		return res;

	*crypt_info = kmem_cache_alloc(fscrypt_info_cachep, GFP_NOFS);
	if (!(*crypt_info))
		return -ENOMEM;
	(void)memset_s(*crypt_info, sizeof(struct fscrypt_info), 0, sizeof(struct fscrypt_info));

	(*crypt_info)->ci_owns_key = true;
#ifdef CONFIG_FS_ENCRYPTION_INLINE_CRYPT
	(*crypt_info)->ci_inlinecrypt = false;
#endif
	(*crypt_info)->ci_mode = &sdp_mode;
	(*crypt_info)->ci_inode = inode;
	(*crypt_info)->ci_gtfm = NULL;
	(*crypt_info)->ci_key = NULL;
	(*crypt_info)->ci_key_len = 0;
	(*crypt_info)->ci_key_index = -1;

	res = fscrypt_policy_from_sdp_context(&((*crypt_info)->ci_policy), ctx, sizeof(*ctx));
	if (res != 0) {
		pr_err("%s: set policy failed, res:%d", __func__, res);
		put_crypt_info_sdp(*crypt_info);
		*crypt_info = NULL;
	}
	return res;
}

int fill_and_change_crypt_info(struct fscrypt_info *crypt_info, union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx, u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE])
{
	int res;

	if (!crypt_info || !ctx || !sdp_ctx)
		return -EINVAL;
	res = fscrypt_policy_from_sdp_context(&(crypt_info->ci_policy),
		sdp_ctx, sizeof(*sdp_ctx));
	if (res != 0) {
		pr_err("%s get context failed, res:%d", __func__, res);
		return res;
	}

	res = set_encryption_params(crypt_info, ctx, sdp_ctx, fek);
	if (res != 0) {
		pr_err("%s set encryption params failed, res:%d", __func__, res);
		kfree_sensitive(crypt_info->ci_key);
		crypt_info->ci_key = NULL;
		crypt_info->ci_key_len = 0;
		crypt_info->ci_key_index = -1;
	}
	return res;
}

int fill_crypt_info(struct fscrypt_info *crypt_info, union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx, u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE], int flag)
{
	int res;

	if (!crypt_info || !ctx || !sdp_ctx)
		return -EINVAL;
	res = set_encryption_params(crypt_info, ctx, sdp_ctx, fek);
	if (res != 0) {
		kfree_sensitive(crypt_info->ci_key);
		crypt_info->ci_key = NULL;
		crypt_info->ci_key_len = 0;
		crypt_info->ci_key_index = -1;
		return res;
	}
	crypt_info->ci_hw_enc_flag = flag;
	return res;
}
