// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * Description: Implementation of 1) get sdp encryption key info;
 *                                2) update sdp context.
 * Create: 2022-02-22
 */

#include "keyinfo_sdp.h"
#include <linux/blk-crypto.h>

#include <linux/scatterlist.h>
#include <linux/hashtable.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <linux/f2fs_fs.h>
#include <linux/fscrypt_common.h>
#include <uapi/linux/keyctl.h>
#include <keys/user-type.h>
#include <crypto/skcipher.h>
#include <crypto/aead.h>
#include <securec.h>
#include "fscrypt_private.h"
#include "adaptor_keyinfo_sdp.h"
#include "ecdh.h"
#include "sdp_internal.h"

#ifdef CONFIG_HWDPS
#include <huawei_platform/hwdps/hwdps_defines.h>
#include <huawei_platform/hwdps/hwdps_fs_hooks.h>
#include <huawei_platform/hwdps/hwdps_limits.h>
#endif

static int get_payload_for_sdp(const struct user_key_payload *payload,
	u8 *raw, int *size, bool file_pub_key)
{
	/*
	 * if file_pub_key is ture, class private key doesn't
	 * need exist.
	 * if file_pub_key is false, class private key must exist
	 */
	int res = 0;
	struct fscrypt_sdp_key *mst_sdp =
		(struct fscrypt_sdp_key *)payload->data;

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	if ((mst_sdp->sdp_class == FSCRYPT_SDP_ECE_CLASS ||
		mst_sdp->sdp_class == FSCRYPT_SDP_SECE_CLASS) &&
		mst_sdp->size == FS_AES_256_GCM_KEY_SIZE) {
		if (memcpy_s(raw, *size, mst_sdp->raw, mst_sdp->size) != EOK)
			return -EINVAL;
		*size = mst_sdp->size;
		res = 0;
	}
#else
	if (mst_sdp->sdp_class == FSCRYPT_SDP_ECE_CLASS &&
		mst_sdp->size == FS_AES_256_GCM_KEY_SIZE) {
		if (memcpy_s(raw, *size, mst_sdp->raw, mst_sdp->size) != EOK)
			return -EINVAL;
		*size = mst_sdp->size;
	} else if (mst_sdp->sdp_class == FSCRYPT_SDP_SECE_CLASS &&
		(!file_pub_key || mst_sdp->size ==
		FS_SDP_ECC_PRIV_KEY_SIZE) &&
		mst_sdp->pub_key_size == FS_SDP_ECC_PUB_KEY_SIZE) {
		if (memcpy_s(raw, *size, mst_sdp->raw, mst_sdp->size) != EOK)
			return -EINVAL;
		if (memcpy_s(raw + mst_sdp->size, *size + mst_sdp->size,
			mst_sdp->pub_key, mst_sdp->pub_key_size) != EOK)
			return -EINVAL;
		*size = mst_sdp->pub_key_size + mst_sdp->size;
	} else {
		pr_err("%s invalid class\n", __func__);
		res = -EKEYREVOKED;
	}
#endif
	return res;
}

static int do_get_keyring_payload(const u8 *descriptor,
	u8 *raw, int *size, bool file_pub_key)
{
	int res = -EKEYREVOKED;
	struct key *keyring_key = NULL;
	const struct user_key_payload *ukp = NULL;
	struct fscrypt_key *mst_key = NULL;

	keyring_key = fscrypt_request_key(descriptor, FSCRYPT_KEY_DESC_PREFIX,
		FSCRYPT_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key))
		return PTR_ERR(keyring_key);

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon)
		goto out;

	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) {
		/* key was revoked before we acquired its semaphore */
		res = -EKEYREVOKED;
		goto out;
	}

	if (ukp->datalen == sizeof(struct fscrypt_key)) {
		mst_key = (struct fscrypt_key *)ukp->data;
		if (mst_key->size == FS_AES_256_GCM_KEY_SIZE) {
			if (memcpy_s(raw, *size, mst_key->raw, mst_key->size) !=
				EOK) {
				res = -EINVAL;
				goto out;
			}
			*size = mst_key->size;
			res = 0;
		}
	} else if (ukp->datalen == sizeof(struct fscrypt_sdp_key)) {
		res = get_payload_for_sdp(ukp, raw, size, file_pub_key);
		if (res != 0)
			pr_err("%s get payload failed res:%d\n", __func__, res);
	}
out:
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return res;
}

static int decrypt_or_encrypt_fek(u8 *descriptor, u8 *nonce, u8 *fek, u8 *iv, int enc)
{
	int res;
	struct crypto_aead *tfm = NULL;
	u8 raw[FSCRYPT_MAX_KEY_SIZE] = {0};
	int size = FSCRYPT_MAX_KEY_SIZE;

	tfm = crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm))
		return (int)PTR_ERR(tfm);

	res = do_get_keyring_payload(descriptor, raw, &size, false);
	if (res != 0)
		goto out;

	res = fscrypt_set_gcm_key(tfm, (const u8 *)raw);
	if (res != 0)
		goto out;

	if (enc == FLAG_ENCRYPT)
		res = fscrypt_derive_gcm_key(tfm, fek, nonce, iv, FLAG_ENCRYPT);
	else
		res = fscrypt_derive_gcm_key(tfm, nonce, fek, iv, FLAG_DECRYPT);
out:
	if (res != 0)
		pr_err("%s derive key failed, enc:%d, res:%d",
			__func__, enc, res);
	crypto_free_aead(tfm);
	memzero_explicit(raw, (size_t)FSCRYPT_MAX_KEY_SIZE);
	return res;
}

static int fill_iv_and_fek(struct inode *inode,
	const union fscrypt_context *ctx,
	u8 *fek, u8 *iv)
{
	bool inherit_key = false;
	int res = 0;

	/* file is not null, ece should inherit ce nonece iv,
	 * sece also support
	 */
	if (i_size_read(inode))
		inherit_key = true;

	if (inherit_key) {
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
		pr_err("fscrypt_sdp:The file has content in crypt v3");
		return -EINVAL;
#else
		res = decrypt_or_encrypt_fek((u8 *)(ctx->v1.master_key_descriptor),
			(u8 *)(ctx->v1.nonce), fek, (u8 *)(ctx->v1.iv), FLAG_DECRYPT);
		if (res != 0) {
			pr_err("%s: get fek fail, res:%d!\n", __func__, res);
			return res;
		}
		(void)memcpy_s(iv, FS_KEY_DERIVATION_IV_SIZE,
			ctx->v1.iv, FS_KEY_DERIVATION_IV_SIZE);
#endif
	} else {
		get_random_bytes(fek, FS_KEY_DERIVATION_NONCE_SIZE);
		get_random_bytes(iv, FS_KEY_DERIVATION_IV_SIZE);
	}
	return res;
}

static int generate_ece_fek(struct inode *inode,
	union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx,
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE])
{
	int res;
	u8 iv[FS_KEY_DERIVATION_IV_SIZE] = {0};

	res = fill_iv_and_fek(inode, ctx, fek, iv);
	if (res != 0) {
		pr_err("fill iv and fek failed, res:%d", res);
		return res;
	}

	(void)memcpy_s(sdp_ctx->iv, sizeof(sdp_ctx->iv), iv, sizeof(iv));
	res = decrypt_or_encrypt_fek((u8 *)(sdp_ctx->master_key_descriptor), sdp_ctx->nonce,
		fek, sdp_ctx->iv, FLAG_ENCRYPT);
	if (res != 0)
		pr_err("%s:inode:%lu get ece key failed, res:%d!\n",
			__func__, inode->i_ino, res);
	return res;
}

static int set_sdp_ece_sece_context(struct inode *inode,
	struct fscrypt_sdp_context *sdp_ctx,
	union fscrypt_context *ctx,
	u32 flag, void *fs_data)
{
	int res;
	/* should set sdp context back for getting the nonce */
	res =  inode->i_sb->s_cop->update_sdp_context(inode, sdp_ctx,
		sizeof(struct fscrypt_sdp_context), fs_data);
	if (res != 0) {
		pr_err("%s: inode:%lu set sdp ctx failed, res:%d\n",
			__func__, inode->i_ino, res);
		return res;
	}

	res = set_sdp_encryption_flags(inode, fs_data, flag);
	if (res != 0) {
		pr_err("%s: set sdp crypt flag failed!\n", __func__);
		return res;
	}

	/* clear the ce crypto info */
	(void)memset_s(ctx->v1.nonce, sizeof(ctx->v1.nonce),
		0, sizeof(ctx->v1.nonce));
	(void)memset_s(ctx->v1.iv, sizeof(ctx->v1.iv),
		0, sizeof(ctx->v1.iv));

	res = inode->i_sb->s_cop->update_context(inode, ctx,
		sizeof(union fscrypt_context), fs_data);
	if (res != 0) {
		pr_err("%s: inode:%lu update ce ctx failed res:%d\n",
			__func__, inode->i_ino, res);
		return res;
	}
	return res;
}

static int get_context_all(struct inode *inode,
	struct fscrypt_sdp_context *sdp_ctx,
	union fscrypt_context *ctx,
	void *fs_data)
{
	int res;

	if (!inode->i_sb->s_cop->get_sdp_context ||
		!inode->i_sb->s_cop->get_context)
		return -EINVAL;

	res = inode->i_sb->s_cop->get_sdp_context(inode, sdp_ctx,
		sizeof(struct fscrypt_sdp_context), fs_data);
	if (res != sizeof(struct fscrypt_sdp_context)) {
		pr_err("%s: get sdp context failed, res:%d\n", __func__, res);
		return -EINVAL;
	}
	res = inode->i_sb->s_cop->get_context(inode, ctx,
		sizeof(union fscrypt_context));
	if (res != sizeof(union fscrypt_context)) {
		pr_err("%s: get context failed, res:%d\n", __func__, res);
		return -EINVAL;
	}
	return 0;
}

static int get_sdp_ece_crypt_info(struct inode *inode, void *fs_data)
{
	int res;
	union fscrypt_context ctx = {0};
	struct fscrypt_sdp_context sdp_ctx = {0};
	struct fscrypt_info *crypt_info = NULL;
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};

	res = get_context_all(inode, &sdp_ctx, &ctx, fs_data);
	if (res != 0)
		return res;

	res = init_crypt_info_from_context(inode, &crypt_info, &sdp_ctx);
	if (res != 0)
		goto out;

	res = decrypt_or_encrypt_fek((u8 *)(sdp_ctx.master_key_descriptor),
		sdp_ctx.nonce, fek, sdp_ctx.iv, FLAG_DECRYPT);
	if (res != 0) {
		pr_err("%s: get fek failed, res:%d\n", __func__, res);
		goto out;
	}
	res = fill_crypt_info(crypt_info, &ctx, &sdp_ctx, fek, FSCRYPT_SDP_ECE_ENABLE_FLAG);
	if (res != 0) {
		pr_err("%s: fill cypt info failed! res:%d\n", __func__, res);
		goto out;
	}
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = fscrypt_get_ece_metadata(
		inode, crypt_info, fs_data,
		false);
	if (res != 0)
		goto out;
#endif
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;

out:
	memzero_explicit(fek, (size_t)FS_KEY_DERIVATION_CIPHER_SIZE);
	if (res == -ENOKEY)
		res = 0;
	put_crypt_info_sdp(crypt_info);
	return res;
}

static int get_and_set_sdp_ece_crypt_info(struct inode *inode, void *fs_data) // todo
{
	int res;

	union fscrypt_context ctx = {0};
	struct fscrypt_sdp_context sdp_ctx = {0};
	struct fscrypt_info *crypt_info = NULL;
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};

	res = get_context_all(inode, &sdp_ctx, &ctx, fs_data);
	if (res != 0)
		return res;

	res = init_crypt_info_from_context(inode, &crypt_info, &sdp_ctx);
	if (res != 0)
		goto out;

	res = generate_ece_fek(inode, &ctx,
		&sdp_ctx, fek);
	if (res != 0) {
		pr_err("%s: get cypt info failed! res:%d\n", __func__, res);
		goto out;
	}
	res = fill_crypt_info(crypt_info, &ctx, &sdp_ctx, fek, FSCRYPT_SDP_ECE_ENABLE_FLAG);
	if (res != 0) {
		pr_err("%s: fill cypt info failed! res:%d\n", __func__, res);
		goto out;
	}
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = fscrypt_get_ece_metadata(
		inode, crypt_info, fs_data,
		true);
	if (res != 0)
		goto out;
#endif

	/* should set sdp context back for getting the nonce */
	res = set_sdp_ece_sece_context(inode, &sdp_ctx, &ctx,
		FSCRYPT_SDP_ECE_ENABLE_FLAG, fs_data);
	if (res != 0) {
		pr_err("%s: set ece context failed, res:%d", __func__, res);
		goto out;
	}
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;
out:
	if (res == -ENOKEY)
		res = 0;
	memzero_explicit(fek, (size_t)FS_KEY_DERIVATION_CIPHER_SIZE);
	put_crypt_info_sdp(crypt_info);
	return res;
}

static int get_sece_fek(const struct fscrypt_sdp_context *ctx,
	u8 *fek, u8 has_pub_key)
{
	int res;
	int offset_len = 0;
	struct crypto_aead *tfm = NULL;
	u8 raw[FS_SDP_ECC_PRIV_KEY_SIZE + FS_SDP_ECC_PUB_KEY_SIZE] = {0};
	int size = FS_SDP_ECC_PRIV_KEY_SIZE + FS_SDP_ECC_PUB_KEY_SIZE;
	u8 shared_key[FS_AES_256_GCM_KEY_SIZE] = {0};
	buffer_t shared_key_buf = { shared_key, FS_AES_256_GCM_KEY_SIZE };
	buffer_t raw_key_buf = { raw + offset_len, FS_SDP_ECC_PRIV_KEY_SIZE };
	buffer_t file_pub_key_buf = {
		(u8 *)ctx->file_pub_key, FS_SDP_ECC_PUB_KEY_SIZE
	};

	tfm = crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm))
		return (int)PTR_ERR(tfm);

	res = do_get_keyring_payload(ctx->master_key_descriptor, raw,
		&size, has_pub_key == 1);
	if (res != 0) {
		pr_err("%s:get keyring failed res:%d\n", __func__, res);
		goto out;
	}

	if (has_pub_key == NO_PUB_KEY) {
		offset_len = (size == FS_SDP_ECC_PUB_KEY_SIZE) ?
			0 : FS_SDP_ECC_PRIV_KEY_SIZE;
		raw_key_buf.data = raw + offset_len;
		raw_key_buf.len = FS_SDP_ECC_PUB_KEY_SIZE;
		res = get_file_pubkey_shared_secret(ECC_CURVE_NIST_P256,
			&raw_key_buf, &file_pub_key_buf, &shared_key_buf);
	} else {
		res = get_shared_secret(ECC_CURVE_NIST_P256,
			&raw_key_buf, &file_pub_key_buf, &shared_key_buf);
	}
	if (res != 0) {
		pr_err("%s:get raw key failed, res:%d\n", __func__, res);
		goto out;
	}

	res = fscrypt_set_gcm_key(tfm, (const u8 *)shared_key);
	if (res != 0)
		goto out;

	if (has_pub_key == HAS_PUB_KEY)
		res = fscrypt_derive_gcm_key(tfm, (const u8 *)ctx->nonce, fek,
			(u8 *)ctx->iv, FLAG_DECRYPT);
	else
		res = fscrypt_derive_gcm_key(tfm, (const u8 *)fek, (u8 *)ctx->nonce,
			(u8 *)ctx->iv, FLAG_ENCRYPT);
out:
	crypto_free_aead(tfm);
	memzero_explicit(raw, (size_t)(FS_SDP_ECC_PRIV_KEY_SIZE +
		FS_SDP_ECC_PUB_KEY_SIZE));
	memzero_explicit(shared_key, (size_t)FS_AES_256_GCM_KEY_SIZE);
	return res;
}

#ifndef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
static int set_sece_fek(const struct fscrypt_sdp_context *sdp_ctx, u8 *fek)
{
	return get_sece_fek(sdp_ctx, fek, NO_PUB_KEY);
}
#endif

static int get_sdp_sece_crypt_info(struct inode *inode, void *fs_data)
{
	int res;

	union fscrypt_context ctx = {0};
	struct fscrypt_sdp_context sdp_ctx = {0};
	struct fscrypt_info *crypt_info = NULL;
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};

	res = get_context_all(inode, &sdp_ctx, &ctx, fs_data);
	if (res != 0)
		return res;

	res = init_crypt_info_from_context(inode, &crypt_info, &sdp_ctx);
	if (res != 0)
		goto out;

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = decrypt_or_encrypt_fek((u8 *)(sdp_ctx.master_key_descriptor),
		sdp_ctx.nonce, fek, sdp_ctx.iv, FLAG_DECRYPT);
#else
	res = get_sece_fek(&sdp_ctx, fek, HAS_PUB_KEY);
#endif
	if (res != 0) {
		pr_err("%s: get sece fek failed, res:%d\n", __func__, res);
		goto out;
	}
	res = fill_crypt_info(crypt_info, &ctx, &sdp_ctx, fek, FSCRYPT_SDP_SECE_ENABLE_FLAG);
	if (res != 0) {
		pr_err("%s: fill cypt info failed! res:%d\n", __func__, res);
		goto out;
	}
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = fscrypt_get_sece_metadata(
		inode, crypt_info, fs_data,
		false);
	if (res != 0)
		goto out;
#endif
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;

out:
	memzero_explicit(fek, (size_t)FS_KEY_DERIVATION_CIPHER_SIZE);
	if (res == -ENOKEY)
		res = 0;
	put_crypt_info_sdp(crypt_info);
	return res;
}

static int generate_sece_key(struct inode *inode,
	union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx,
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE])
{
	int res;
	u8 iv[FS_KEY_DERIVATION_IV_SIZE] = {0};

	res = fill_iv_and_fek(inode, ctx, fek, iv);
	if (res != 0)
		return res;
	(void)memcpy_s(sdp_ctx->iv, sizeof(sdp_ctx->iv), iv, sizeof(iv));
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = decrypt_or_encrypt_fek((u8 *)(sdp_ctx->master_key_descriptor),
		sdp_ctx->nonce, fek, sdp_ctx->iv, FLAG_ENCRYPT);
#else
	res = set_sece_fek(sdp_ctx, fek);
#endif
	if (res != 0)
		pr_err("%s: inode:%lu get sece key failed, res:%d\n",
			__func__, inode->i_ino, res);

	return res;
}

static int get_and_set_sdp_sece_crypt_info(struct inode *inode, void *fs_data) // todo
{
	int res;
	union fscrypt_context ctx = {0};
	struct fscrypt_sdp_context sdp_ctx = {0};
	struct fscrypt_info *crypt_info = NULL;
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};

	res = get_context_all(inode, &sdp_ctx, &ctx, fs_data);
	if (res != 0)
		return res;

	res = init_crypt_info_from_context(inode, &crypt_info, &sdp_ctx);
	if (res != 0)
		goto out;

	res = generate_sece_key(inode, &ctx,
		&sdp_ctx, fek);
	if (res != 0) {
		pr_err("%s: get sece crypt info failed\n", __func__);
		goto out;
	}
	res = fill_crypt_info(crypt_info, &ctx, &sdp_ctx, fek, FSCRYPT_SDP_SECE_ENABLE_FLAG);
	if (res != 0) {
		pr_err("%s: fill cypt info failed! res:%d\n", __func__, res);
		goto out;
	}
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = fscrypt_get_sece_metadata(
		inode, crypt_info, fs_data,
		true);
	if (res != 0)
		goto out;
#endif

	/* should set sdp context back for getting the nonce */
	res = set_sdp_ece_sece_context(inode, &sdp_ctx, &ctx,
		FSCRYPT_SDP_SECE_ENABLE_FLAG, fs_data);
	if (res != 0) {
		pr_err("%s: set ece context failed, res:%d", __func__, res);
		goto out;
	}
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;
out:
	if (res == -ENOKEY)
		res = 0;
	memzero_explicit(fek, (size_t)FS_KEY_DERIVATION_CIPHER_SIZE);
	put_crypt_info_sdp(crypt_info);
	return res;
}

static fbe_new_meta_cb_func_list g_fbe_meta_cb_func_list[] = {
	{ PLAIN, NULL },
	{ CD, NULL },
	{ ECE, fscrypt_new_ece_metadata },
	{ SECE, fscrypt_new_sece_metadata }
};

static int change_sdp_context(struct inode *inode,
	struct fscrypt_sdp_context *sdp_ctx,
	union fscrypt_context *ctx,
	struct fscrypt_info *ci_info,
	void *fs_data)
{
	int res;

	if (sdp_ctx->sdp_class == FSCRYPT_SDP_ECE_CLASS) {
		res = set_sdp_ece_sece_context(inode, sdp_ctx, ctx,
			FSCRYPT_SDP_ECE_ENABLE_FLAG, fs_data);
			ci_info->ci_hw_enc_flag = FSCRYPT_SDP_ECE_ENABLE_FLAG;
	} else if (sdp_ctx->sdp_class == FSCRYPT_SDP_SECE_CLASS) {
		res = set_sdp_ece_sece_context(inode, sdp_ctx, ctx,
			FSCRYPT_SDP_SECE_ENABLE_FLAG, fs_data);
			ci_info->ci_hw_enc_flag = FSCRYPT_SDP_SECE_ENABLE_FLAG;
	} else {
		pr_err("%s, invalid sdp_ctx:%d", __func__, sdp_ctx->sdp_class);
		res = -EINVAL;
	}
	if (res != 0)
		pr_err("%s: set sdp context failed, res:%d\n", __func__, res);

	return res;
}

static int change_crypt_info_and_context_to_sdp(struct inode *inode, union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx, void *fs_data)
{
	int res;
	struct fscrypt_info *ci_info = NULL;
	u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};

	ci_info = inode->i_crypt_info;
	if (!ci_info) {
		pr_err("%s: ci_info failed!", __func__);
		return -EINVAL;
	}
	if (sdp_ctx->sdp_class == FSCRYPT_SDP_ECE_CLASS) {
		res = generate_ece_fek(inode, ctx,
			sdp_ctx, fek);
	} else if (sdp_ctx->sdp_class == FSCRYPT_SDP_SECE_CLASS) {
		res = generate_sece_key(inode, ctx,
			sdp_ctx, fek);
	} else {
		pr_err("%s: invalid sdp_class:%d", __func__, sdp_ctx->sdp_class);
		res = -EINVAL;
	}
	if (res != 0) {
		pr_err("%s: get crypt info failed, res: %d\n", __func__, res);
		goto out;
	}

	res = fill_and_change_crypt_info(ci_info, ctx,
		sdp_ctx, fek);
	if (res != 0)
		goto out;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	res = g_fbe_meta_cb_func_list[sdp_ctx->sdp_class].file_encry_meta_cb_func(inode, ci_info, fs_data);
	if (res != 0)
		goto out;
#endif
	res = change_sdp_context(inode, sdp_ctx, ctx, ci_info, fs_data);
	if (res != 0) {
		pr_err("fscrypt_sdp %s: get sdp flag failed res:%d", __func__,
		       res);
		goto out;
	}
out:
	memzero_explicit(fek, (size_t)FS_KEY_DERIVATION_CIPHER_SIZE);
	return res;
}

int change_to_sdp_crypto(struct inode *inode, void *fs_data)
{
	int res;

	union fscrypt_context ctx = {0};
	struct fscrypt_sdp_context sdp_ctx = {0};

	if (!inode) /* fs_data is NULL so not need to jduge */
		return -EINVAL;

	res = get_context_all(inode, &sdp_ctx, &ctx, fs_data);
	if (res != 0) {
		pr_err("%s: get sdp_ctx size failed res:%d", __func__, res);
		return -EINVAL;
	}

	res = change_crypt_info_and_context_to_sdp(inode, &ctx,
		&sdp_ctx, fs_data);
	if (res != 0) {
		pr_err("%s: get crypt info failed, res: %d\n", __func__, res);
		fscrypt_put_encryption_info(inode);
	}

	return res;
}

static int get_sdp_crypt_info_inner(struct inode *inode, void *fs_data)
{
	int res;
	u32 flag;
	struct fscrypt_info *ci_info = NULL;

	if (!inode) /* fs_data can be null so not need to check */
		return -EINVAL;

	ci_info = inode->i_crypt_info;
	res = get_sdp_encrypt_flags(inode, fs_data, &flag);
	if (res != 0)
		return res;

	if (ci_info && inode_is_enabled_sdp_encryption(flag)) {
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
		if (inode_is_enabled_sdp_sece_encryption(flag)) {
			res = fscrypt_retrieve_sece_metadata(inode, ci_info, fs_data);
			if (unlikely(res)) {
				pr_err("%s: fscrypt_retrieve_sece_metadata failed, res = %d\n",
					   __func__, res);
				return res;
			}
			return res;
		}
#endif
		return check_keyring_for_sdp(ci_info->ci_policy.v1.master_key_descriptor,
			ENHANCED_CHECK_KEYING);
	}
	/* should change from ce to sdp crypto and set it to enable */
	if (ci_info && inode_is_config_sdp_encryption(flag))
		return change_to_sdp_crypto(inode, fs_data);

	/* create ci_info for enable ece file */
	if (inode_is_enabled_sdp_ece_encryption(flag))
		return get_sdp_ece_crypt_info(inode, fs_data);

	/* create ci_info for enable sece file */
	if (inode_is_enabled_sdp_sece_encryption(flag))
		return get_sdp_sece_crypt_info(inode, fs_data);

	/* create ci_info for config ece file and change it to enable */
	if (inode_is_config_sdp_ece_encryption(flag))
		return get_and_set_sdp_ece_crypt_info(inode, fs_data);

	/* create ci_info for config sece file and change it to enable */
	if (inode_is_config_sdp_sece_encryption(flag))
		return get_and_set_sdp_sece_crypt_info(inode, fs_data);

	pr_err("%s, bad flag", __func__);
	return -EINVAL;
}

int fscrypt_get_sdp_crypt_keyinfo(struct inode *inode, void *fs_data, int *flag)
{
	int res;
	u32 sdpflag;

	if (!inode)
		return -EINVAL;
	if (!flag) /* fs_data can be null so not check it */
		return -EINVAL;
	/* 0 for getting original ce crypt info, otherwise be 1 */
	*flag = 0;
#ifdef CONFIG_HWDPS
	if (!inode->i_crypt_info || (inode->i_crypt_info &&
		((inode->i_crypt_info->ci_hw_enc_flag &
		HWDPS_ENABLE_FLAG) != 0))) {
		res = hwdps_get_context(inode);
		if (res == -EOPNOTSUPP) {
			goto get_sdp_encryption_info;
		} else if (res != 0) {
			*flag = 1; /* enabled */
			return -EACCES;
		}
		*flag = 1; /* enabled */
		return 0;
	}
get_sdp_encryption_info:
#endif
	if (!S_ISREG(inode->i_mode))
		return 0;
	(void)inode->i_sb->s_cop->operate_sdp_sem(inode, 0); /* 0 for down_write() */
	res = inode->i_sb->s_cop->get_sdp_encrypt_flags(inode, fs_data, &sdpflag);
	if (res != 0) {
		pr_err("%s: get_sdp_encrypt_flags failed, res:%d",
			__func__, res);
		goto unlock;
	}

	/* get sdp crypt info */
	if (inode_is_sdp_encrypted(sdpflag)) {
		*flag = 1; /* enabled */
		res = get_sdp_crypt_info_inner(inode, fs_data);
		if (res != 0)
			pr_err("%s: inode:%lu, get keyinfo failed res:%d\n",
				__func__, inode->i_ino, res);
	}
unlock:
	(void)inode->i_sb->s_cop->operate_sdp_sem(inode, 1); /* 1 for up_write() */
	return res;
}

int check_keyring_for_sdp(const u8 *descriptor,
	int enforce)
{
	int res = -EKEYREVOKED;
	struct key *keyring_key = NULL;
	const struct user_key_payload *payload = NULL;
	struct fscrypt_sdp_key *master_sdp_key = NULL;

	if (!descriptor)
		return -EINVAL;

	keyring_key = fscrypt_request_key(descriptor,
		FSCRYPT_KEY_DESC_PREFIX, FSCRYPT_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key))
		return PTR_ERR(keyring_key);

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon) {
		pr_err("%s: key type must be logon\n", __func__);
		goto out;
	}

	payload = user_key_payload_locked(keyring_key);
	if (!payload)
		goto out;

	if (payload->datalen != sizeof(struct fscrypt_sdp_key)) {
		pr_err("%s: sdp full key size incorrect:%d\n",
			__func__, payload->datalen);
		goto out;
	}

	master_sdp_key = (struct fscrypt_sdp_key *)payload->data;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	if (master_sdp_key->sdp_class != FSCRYPT_SDP_SECE_CLASS &&
		master_sdp_key->sdp_class != FSCRYPT_SDP_ECE_CLASS) {
		goto out;
	}
#else
	if (master_sdp_key->sdp_class == FSCRYPT_SDP_SECE_CLASS) {
		if (enforce == ENHANCED_CHECK_KEYING) {
			u8 sdp_pri_key[FSCRYPT_MAX_KEY_SIZE] = {0};
			if (master_sdp_key->size == 0 ||
				memcmp(master_sdp_key->raw, sdp_pri_key,
					master_sdp_key->size) == 0)
				goto out;
		}
	} else if (master_sdp_key->sdp_class != FSCRYPT_SDP_ECE_CLASS) {
		goto out;
	}
#endif

	res = 0;
out:
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return res;
}

#ifdef CONFIG_HWDPS
int set_dps_attr_and_flag(struct inode *inode)
{
	int ret;
	uint8_t *encoded_wfek = NULL;
	uint32_t encoded_len = HWDPS_ENCODED_WFEK_SIZE;
	secondary_buffer_t buffer_wfek = { &encoded_wfek, &encoded_len };

	if (!inode || !inode->i_crypt_info)
		return -EINVAL;
	if (S_ISREG(inode->i_mode)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		ret = hwdps_get_fek_from_origin(
			inode->i_crypt_info->ci_policy.v1.master_key_descriptor,
			inode, &buffer_wfek);
#else
		ret = hwdps_get_fek_from_origin(
			inode->i_crypt_info->ci_master_key,
			inode, &buffer_wfek);
#endif
		if (ret != 0) {
			pr_err("f2fs_dps hwdps_get_fek_from_origin err %d\n",
				ret);
			goto err;
		}
	} else {
		encoded_wfek = kzalloc(HWDPS_ENCODED_WFEK_SIZE, GFP_NOFS);
		if (!encoded_wfek) {
			pr_err("f2fs_dps kzalloc err\n");
			ret = -ENOMEM;
			goto err;
		}
	}
	ret = inode->i_sb->s_cop->set_hwdps_attr(inode, encoded_wfek,
		encoded_len, NULL);
	if (ret != 0) {
		pr_err("f2fs_dps set_hwdps_attr err\n");
		goto err;
	}

	ret = f2fs_set_hwdps_enable_flags(inode, NULL);
	if (ret != 0) {
		pr_err("f2fs_dps f2fs_set_hwdps_enable_flags err %d\n", ret);
		goto err;
	}
err:
	kfree_sensitive(encoded_wfek);
	return ret;
}
#endif

int fscrypt_check_sdp_encrypted(struct inode *inode)
{
	u32 flag;

	/* if the inode is not null, i_sb, s_cop is not null */
	if (!inode || !inode->i_sb || !inode->i_sb->s_cop)
		return 0; /* In this condition 0 means not enable sdp */
	if (!inode->i_sb->s_cop->get_sdp_encrypt_flags)
		return 0;
	if (inode->i_sb->s_cop->get_sdp_encrypt_flags(inode, NULL, &flag))
		return 0;
	return inode_is_sdp_encrypted(flag);
}

int get_sdp_encrypt_flags(struct inode *inode, void *fs_data,
	u32 *flag)
{
	int res;

	if (!inode || !flag || !inode->i_sb ||
		!inode->i_sb->s_cop) /* fs_data can be null, no need to check */
		return -EINVAL;
	if (!inode->i_sb->s_cop->get_sdp_encrypt_flags)
		return -EOPNOTSUPP;

	res = inode->i_sb->s_cop->get_sdp_encrypt_flags(inode, fs_data, flag);
	if (res != 0)
		pr_err("%s: get flag failed, res:%d!\n", __func__, res);
	return res;
}

int set_sdp_encryption_flags(struct inode *inode, void *fs_data,
	u32 sdp_enc_flag)
{
	int res;
	u32 flags;

	/* if the inode is not null, i_sb, s_cop is not null */
	if (!inode || !inode->i_sb || !inode->i_sb->s_cop)
		return 0;

	if ((sdp_enc_flag & (~0x0F)) != 0)
		return -EINVAL;
	if (!inode->i_sb->s_cop->get_sdp_encrypt_flags ||
		!inode->i_sb->s_cop->set_sdp_encrypt_flags)
		return -EOPNOTSUPP;
	res = inode->i_sb->s_cop->get_sdp_encrypt_flags(inode, fs_data, &flags);
	if (res != 0)
		return res;

	flags |= sdp_enc_flag;
	res = inode->i_sb->s_cop->set_sdp_encrypt_flags(inode, fs_data, &flags);
	if (res != 0)
		pr_err("%s: set flag failed, res:%d!\n", __func__, res);
	return res;
}
