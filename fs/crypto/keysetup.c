// SPDX-License-Identifier: GPL-2.0
/*
 * Key setup facility for FS encryption support.
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * Originally written by Michael Halcrow, Ildar Muslukhov, and Uday Savagaonkar.
 * Heavily modified since then.
 */

#include <crypto/aes.h>
#include <crypto/sha.h>
#include <crypto/skcipher.h>
#include <keys/user-type.h>
#include <linux/key.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/ratelimit.h>
#include <linux/fscrypt_common.h>
#ifdef CONFIG_HWDPS
#include <linux/security.h>
#include <securec.h>
#include <huawei_platform/hwdps/hwdps_limits.h>
#include <huawei_platform/hwdps/hwdps_fs_hooks.h>
#endif

#include "fscrypt_private.h"

struct fscrypt_mode fscrypt_modes[] = {
	[FSCRYPT_MODE_AES_256_XTS] = {
		.friendly_name = "AES-256-XTS",
		.cipher_str = "xts(aes)",
		.keysize = 64,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_256_XTS,
	},
	[FSCRYPT_MODE_AES_256_CTS] = {
		.friendly_name = "AES-256-CTS-CBC",
		.cipher_str = "cts(cbc(aes))",
		.keysize = 32,
		.ivsize = 16,
	},
	[FSCRYPT_MODE_AES_128_CBC] = {
		.friendly_name = "AES-128-CBC-ESSIV",
		.cipher_str = "essiv(cbc(aes),sha256)",
		.keysize = 16,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_128_CBC_ESSIV,
	},
	[FSCRYPT_MODE_AES_128_CTS] = {
		.friendly_name = "AES-128-CTS-CBC",
		.cipher_str = "cts(cbc(aes))",
		.keysize = 16,
		.ivsize = 16,
	},
	[FSCRYPT_MODE_ADIANTUM] = {
		.friendly_name = "Adiantum",
		.cipher_str = "adiantum(xchacha12,aes)",
		.keysize = 32,
		.ivsize = 32,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_ADIANTUM,
	},
};

static DEFINE_MUTEX(fscrypt_mode_key_setup_mutex);

static struct fscrypt_mode *
select_encryption_mode(const union fscrypt_policy *policy,
		       const struct inode *inode)
{
	BUILD_BUG_ON(ARRAY_SIZE(fscrypt_modes) != FSCRYPT_MODE_MAX + 1);

	if (S_ISREG(inode->i_mode))
		return &fscrypt_modes[fscrypt_policy_contents_mode(policy)];

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
		return &fscrypt_modes[fscrypt_policy_fnames_mode(policy)];

	WARN_ONCE(1, "fscrypt: filesystem tried to load encryption info for inode %lu, which is not encryptable (file type %d)\n",
		  inode->i_ino, (inode->i_mode & S_IFMT));
	return ERR_PTR(-EINVAL);
}

/* Create a symmetric cipher object for the given encryption mode and key */
struct crypto_skcipher *
fscrypt_allocate_skcipher(struct fscrypt_mode *mode, const u8 *raw_key,
			  const struct inode *inode)
{
	struct crypto_skcipher *tfm;
	int err;

	tfm = crypto_alloc_skcipher(mode->cipher_str, 0, 0);
	if (IS_ERR(tfm)) {
		if (PTR_ERR(tfm) == -ENOENT) {
			fscrypt_warn(inode,
				     "Missing crypto API support for %s (API name: \"%s\")",
				     mode->friendly_name, mode->cipher_str);
			return ERR_PTR(-ENOPKG);
		}
		fscrypt_err(inode, "Error allocating '%s' transform: %ld",
			    mode->cipher_str, PTR_ERR(tfm));
		return tfm;
	}
	if (!xchg(&mode->logged_impl_name, 1)) {
		/*
		 * fscrypt performance can vary greatly depending on which
		 * crypto algorithm implementation is used.  Help people debug
		 * performance problems by logging the ->cra_driver_name the
		 * first time a mode is used.
		 */
		pr_info("fscrypt: %s using implementation \"%s\"\n",
			mode->friendly_name, crypto_skcipher_driver_name(tfm));
	}
	if (WARN_ON(crypto_skcipher_ivsize(tfm) != mode->ivsize)) {
		err = -EINVAL;
		goto err_free_tfm;
	}
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_FORBID_WEAK_KEYS);
	err = crypto_skcipher_setkey(tfm, raw_key, mode->keysize);
	if (err)
		goto err_free_tfm;

	return tfm;

err_free_tfm:
	crypto_free_skcipher(tfm);
	return ERR_PTR(err);
}

/*
 * Prepare the crypto transform object or blk-crypto key in @prep_key, given the
 * raw key, encryption mode, and flag indicating which encryption implementation
 * (fs-layer or blk-crypto) will be used.
 */
int fscrypt_prepare_key(struct fscrypt_prepared_key *prep_key,
			const u8 *raw_key, unsigned int raw_key_size,
			bool is_hw_wrapped, const struct fscrypt_info *ci)
{
	struct crypto_skcipher *tfm;

	if (fscrypt_using_inline_encryption(ci))
		return fscrypt_prepare_inline_crypt_key(prep_key,
				raw_key, raw_key_size, is_hw_wrapped, ci);

	if (WARN_ON(is_hw_wrapped || raw_key_size != ci->ci_mode->keysize))
		return -EINVAL;

	tfm = fscrypt_allocate_skcipher(ci->ci_mode, raw_key, ci->ci_inode);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	/*
	 * Pairs with the smp_load_acquire() in fscrypt_is_key_prepared().
	 * I.e., here we publish ->tfm with a RELEASE barrier so that
	 * concurrent tasks can ACQUIRE it.  Note that this concurrency is only
	 * possible for per-mode keys, not for per-file keys.
	 */
	smp_store_release(&prep_key->tfm, tfm);
	return 0;
}

/* Destroy a crypto transform object and/or blk-crypto key. */
void fscrypt_destroy_prepared_key(struct fscrypt_prepared_key *prep_key)
{
	crypto_free_skcipher(prep_key->tfm);
	fscrypt_destroy_inline_crypt_key(prep_key);
}

/* Given a per-file encryption key, set up the file's crypto transform object */
int fscrypt_set_per_file_enc_key(struct fscrypt_info *ci, const u8 *raw_key)
{
	ci->ci_owns_key = true;
	return fscrypt_prepare_key(&ci->ci_enc_key, raw_key,
				   ci->ci_mode->keysize,
				   false /*is_hw_wrapped*/, ci);
}

static int setup_per_mode_enc_key(struct fscrypt_info *ci,
				  struct fscrypt_master_key *mk,
				  struct fscrypt_prepared_key *keys,
				  u8 hkdf_context, bool include_fs_uuid)
{
	const struct inode *inode = ci->ci_inode;
	const struct super_block *sb = inode->i_sb;
	struct fscrypt_mode *mode = ci->ci_mode;
	const u8 mode_num = mode - fscrypt_modes;
	struct fscrypt_prepared_key *prep_key;
	u8 mode_key[FSCRYPT_MAX_KEY_SIZE];
	u8 hkdf_info[sizeof(mode_num) + sizeof(sb->s_uuid)];
	unsigned int hkdf_infolen = 0;
	int err;

	if (WARN_ON(mode_num > FSCRYPT_MODE_MAX))
		return -EINVAL;

	prep_key = &keys[mode_num];
	if (fscrypt_is_key_prepared(prep_key, ci)) {
		ci->ci_enc_key = *prep_key;
		return 0;
	}

	mutex_lock(&fscrypt_mode_key_setup_mutex);

	if (fscrypt_is_key_prepared(prep_key, ci))
		goto done_unlock;

	if (mk->mk_secret.is_hw_wrapped && S_ISREG(inode->i_mode)) {
		int i;

		if (!fscrypt_using_inline_encryption(ci)) {
			fscrypt_warn(ci->ci_inode,
				     "Hardware-wrapped keys require inline encryption (-o inlinecrypt)");
			err = -EINVAL;
			goto out_unlock;
		}
		for (i = 0; i <= FSCRYPT_MODE_MAX; i++) {
			if (fscrypt_is_key_prepared(&keys[i], ci)) {
				fscrypt_warn(ci->ci_inode,
					     "Each hardware-wrapped key can only be used with one encryption mode");
				err = -EINVAL;
				goto out_unlock;
			}
		}
		err = fscrypt_prepare_key(prep_key, mk->mk_secret.raw,
					  mk->mk_secret.size, true, ci);
		if (err)
			goto out_unlock;
	} else {
		BUILD_BUG_ON(sizeof(mode_num) != 1);
		BUILD_BUG_ON(sizeof(sb->s_uuid) != 16);
		BUILD_BUG_ON(sizeof(hkdf_info) != 17);
		hkdf_info[hkdf_infolen++] = mode_num;
		if (include_fs_uuid) {
			memcpy(&hkdf_info[hkdf_infolen], &sb->s_uuid,
				   sizeof(sb->s_uuid));
			hkdf_infolen += sizeof(sb->s_uuid);
		}
		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  hkdf_context, hkdf_info, hkdf_infolen,
					  mode_key, mode->keysize);
		if (err)
			goto out_unlock;
		err = fscrypt_prepare_key(prep_key, mode_key, mode->keysize,
					  false /*is_hw_wrapped*/, ci);
		memzero_explicit(mode_key, mode->keysize);
		if (err)
			goto out_unlock;
	}
done_unlock:
	ci->ci_enc_key = *prep_key;
	err = 0;
out_unlock:
	mutex_unlock(&fscrypt_mode_key_setup_mutex);
	return err;
}

int fscrypt_derive_dirhash_key(struct fscrypt_info *ci,
			       const struct fscrypt_master_key *mk)
{
	int err;

	err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf, HKDF_CONTEXT_DIRHASH_KEY,
				  ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE,
				  (u8 *)&ci->ci_dirhash_key,
				  sizeof(ci->ci_dirhash_key));
	if (err)
		return err;
	ci->ci_dirhash_key_initialized = true;
	return 0;
}

void fscrypt_hash_inode_number(struct fscrypt_info *ci,
			       const struct fscrypt_master_key *mk)
{
	WARN_ON(ci->ci_inode->i_ino == 0);
	WARN_ON(!mk->mk_ino_hash_key_initialized);

	ci->ci_hashed_ino = (u32)siphash_1u64(ci->ci_inode->i_ino,
					      &mk->mk_ino_hash_key);
}

static int fscrypt_setup_iv_ino_lblk_32_key(struct fscrypt_info *ci,
					    struct fscrypt_master_key *mk)
{
	int err;

	err = setup_per_mode_enc_key(ci, mk, mk->mk_iv_ino_lblk_32_keys,
				     HKDF_CONTEXT_IV_INO_LBLK_32_KEY, true);
	if (err)
		return err;

	/* pairs with smp_store_release() below */
	if (!smp_load_acquire(&mk->mk_ino_hash_key_initialized)) {

		mutex_lock(&fscrypt_mode_key_setup_mutex);

		if (mk->mk_ino_hash_key_initialized)
			goto unlock;

		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  HKDF_CONTEXT_INODE_HASH_KEY, NULL, 0,
					  (u8 *)&mk->mk_ino_hash_key,
					  sizeof(mk->mk_ino_hash_key));
		if (err)
			goto unlock;
		/* pairs with smp_load_acquire() above */
		smp_store_release(&mk->mk_ino_hash_key_initialized, true);
unlock:
		mutex_unlock(&fscrypt_mode_key_setup_mutex);
		if (err)
			return err;
	}

	/*
	 * New inodes may not have an inode number assigned yet.
	 * Hashing their inode number is delayed until later.
	 */
	if (ci->ci_inode->i_ino)
		fscrypt_hash_inode_number(ci, mk);
	return 0;
}

static int fscrypt_setup_v2_file_key(struct fscrypt_info *ci,
				     struct fscrypt_master_key *mk,
				     bool need_dirhash_key)
{
	int err;

	if (mk->mk_secret.is_hw_wrapped &&
	    !(ci->ci_policy.v2.flags & (FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64 |
					FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32))) {
		fscrypt_warn(ci->ci_inode,
			     "Hardware-wrapped keys are only supported with IV_INO_LBLK policies");
		return -EINVAL;
	}

	if (ci->ci_policy.v2.flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) {
		/*
		 * DIRECT_KEY: instead of deriving per-file encryption keys, the
		 * per-file nonce will be included in all the IVs.  But unlike
		 * v1 policies, for v2 policies in this case we don't encrypt
		 * with the master key directly but rather derive a per-mode
		 * encryption key.  This ensures that the master key is
		 * consistently used only for HKDF, avoiding key reuse issues.
		 */
		err = setup_per_mode_enc_key(ci, mk, mk->mk_direct_keys,
					     HKDF_CONTEXT_DIRECT_KEY, false);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64) {
		/*
		 * IV_INO_LBLK_64: encryption keys are derived from (master_key,
		 * mode_num, filesystem_uuid), and inode number is included in
		 * the IVs.  This format is optimized for use with inline
		 * encryption hardware compliant with the UFS standard.
		 */
		err = setup_per_mode_enc_key(ci, mk, mk->mk_iv_ino_lblk_64_keys,
					     HKDF_CONTEXT_IV_INO_LBLK_64_KEY,
					     true);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32) {
		err = fscrypt_setup_iv_ino_lblk_32_key(ci, mk);
	} else {
		u8 derived_key[FSCRYPT_MAX_KEY_SIZE];

		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  HKDF_CONTEXT_PER_FILE_ENC_KEY,
					  ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE,
					  derived_key, ci->ci_mode->keysize);
		if (err)
			return err;

		err = fscrypt_set_per_file_enc_key(ci, derived_key);
		memzero_explicit(derived_key, ci->ci_mode->keysize);
	}
	if (err)
		return err;

	/* Derive a secret dirhash key for directories that need it. */
	if (need_dirhash_key) {
		err = fscrypt_derive_dirhash_key(ci, mk);
		if (err)
			return err;
	}

	return 0;
}

/*
 * Find the master key, then set up the inode's actual encryption key.
 *
 * If the master key is found in the filesystem-level keyring, then the
 * corresponding 'struct key' is returned in *master_key_ret with its semaphore
 * read-locked.  This is needed to ensure that only one task links the
 * fscrypt_info into ->mk_decrypted_inodes (as multiple tasks may race to create
 * an fscrypt_info for the same inode), and to synchronize the master key being
 * removed with a new inode starting to use it.
 */
static int setup_file_encryption_key(struct fscrypt_info *ci,
				     bool need_dirhash_key,
				     struct key **master_key_ret,
				     u8 *derived_key,
				     unsigned int derived_keysize, bool enc)
{
	struct key *key;
	struct fscrypt_master_key *mk = NULL;
	struct fscrypt_key_specifier mk_spec;
	int err;

	switch (ci->ci_policy.version) {
	case FSCRYPT_POLICY_V1:
		mk_spec.type = FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR;
		memcpy(mk_spec.u.descriptor,
		       ci->ci_policy.v1.master_key_descriptor,
		       FSCRYPT_KEY_DESCRIPTOR_SIZE);
		break;
	case FSCRYPT_POLICY_V2:
		mk_spec.type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;
		memcpy(mk_spec.u.identifier,
		       ci->ci_policy.v2.master_key_identifier,
		       FSCRYPT_KEY_IDENTIFIER_SIZE);
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
	}

	key = fscrypt_find_master_key(ci->ci_inode->i_sb, &mk_spec);
	if (IS_ERR(key)) {
		if (key != ERR_PTR(-ENOKEY) ||
		    ci->ci_policy.version != FSCRYPT_POLICY_V1)
			return PTR_ERR(key);

		err = fscrypt_select_encryption_impl(ci, false);
		if (err)
			return err;

		/*
		 * As a legacy fallback for v1 policies, search for the key in
		 * the current task's subscribed keyrings too.  Don't move this
		 * to before the search of ->s_master_keys, since users
		 * shouldn't be able to override filesystem-level keys.
		 */
		return fscrypt_setup_v1_file_key_via_subscribed_keyrings(
			ci, derived_key, derived_keysize, enc);
	}

	mk = key->payload.data[0];
	down_read(&key->sem);

	/* Has the secret been removed (via FS_IOC_REMOVE_ENCRYPTION_KEY)? */
	if (!is_master_key_secret_present(&mk->mk_secret)) {
		err = -ENOKEY;
		goto out_release_key;
	}

	/*
	 * Require that the master key be at least as long as the derived key.
	 * Otherwise, the derived key cannot possibly contain as much entropy as
	 * that required by the encryption mode it will be used for.  For v1
	 * policies it's also required for the KDF to work at all.
	 */
	if (mk->mk_secret.size < ci->ci_mode->keysize) {
		fscrypt_warn(NULL,
			     "key with %s %*phN is too short (got %u bytes, need %u+ bytes)",
			     master_key_spec_type(&mk_spec),
			     master_key_spec_len(&mk_spec), (u8 *)&mk_spec.u,
			     mk->mk_secret.size, ci->ci_mode->keysize);
		err = -ENOKEY;
		goto out_release_key;
	}

	err = fscrypt_select_encryption_impl(ci, mk->mk_secret.is_hw_wrapped);
	if (err)
		goto out_release_key;

	switch (ci->ci_policy.version) {
	case FSCRYPT_POLICY_V1:
		err = fscrypt_setup_v1_file_key(ci, mk->mk_secret.raw);
		break;
	case FSCRYPT_POLICY_V2:
		err = fscrypt_setup_v2_file_key(ci, mk, need_dirhash_key);
		break;
	default:
		WARN_ON(1);
		err = -EINVAL;
		break;
	}
	if (err)
		goto out_release_key;

	*master_key_ret = key;
	return 0;

out_release_key:
	up_read(&key->sem);
	key_put(key);
	return err;
}

static void put_crypt_info(struct fscrypt_info *ci)
{
	struct key *key;
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

static int fscrypt_setup_encryption_info(
	struct inode *inode, const union fscrypt_policy *policy,
	const u8 nonce[FS_KEY_DERIVATION_CIPHER_SIZE],
	const u8 iv[FS_KEY_DERIVATION_IV_SIZE], bool need_dirhash_key, bool enc)
{
	struct fscrypt_info *crypt_info;
	struct fscrypt_mode *mode;
	struct key *master_key = NULL;
	u8 *raw_key = NULL;
	int res;
#ifdef CONFIG_FS_INLINE_ENCRYPT_DEBUG
	char *nonce_str = kasprintf(GFP_KERNEL, "%*phN",
				    FS_KEY_DERIVATION_CIPHER_SIZE, nonce);
	fscrypt_debug(inode, "[FBE]%s nonce = %s, nonce + 64 = %s", __func__,
		     nonce_str, nonce_str + FS_KEY_DERIVATION_NONCE_SIZE);
	kfree(nonce_str);
#endif

	res = fscrypt_initialize(inode->i_sb->s_cop->flags);
	if (res)
		return res;

	crypt_info = kmem_cache_zalloc(fscrypt_info_cachep, GFP_KERNEL);
	if (!crypt_info)
		return -ENOMEM;

	crypt_info->ci_inode = inode;
	crypt_info->ci_policy = *policy;
	crypt_info->ci_gtfm = NULL;
	crypt_info->ci_key = NULL;
	crypt_info->ci_key_len = 0;
	crypt_info->ci_key_index = -1;
	memset(crypt_info->ci_metadata, 0, sizeof(crypt_info->ci_metadata));
	memcpy(crypt_info->ci_nonce, nonce, FS_KEY_DERIVATION_CIPHER_SIZE);
	memcpy(crypt_info->ci_iv, iv, FS_KEY_DERIVATION_IV_SIZE);

	mode = select_encryption_mode(&crypt_info->ci_policy, inode);
	if (IS_ERR(mode)) {
		res = PTR_ERR(mode);
		goto out;
	}
	WARN_ON(mode->ivsize > FSCRYPT_MAX_IV_SIZE);
	crypt_info->ci_mode = mode;

	raw_key = kmalloc(FS_KEY_DERIVATION_CIPHER_SIZE, GFP_NOFS);
	if (!raw_key)
		goto out;

	res = setup_file_encryption_key(crypt_info, need_dirhash_key,
					&master_key, raw_key, FS_KEY_DERIVATION_CIPHER_SIZE,
					enc);
	if (res)
		goto out;

	if (S_ISREG(inode->i_mode)) {
		crypt_info->ci_key =
			kzalloc((size_t)FSCRYPT_MAX_KEY_SIZE, GFP_NOFS);
		if (!crypt_info->ci_key) {
			res = -ENOMEM;
			goto out;
		}
	}

	if (S_ISREG(inode->i_mode)) {
		crypt_info->ci_key_len = mode->keysize;
		if (enc) /* plain text nonce set to ci_key */
			memcpy(crypt_info->ci_key, crypt_info->ci_nonce,
			       crypt_info->ci_key_len);
		else /* raw_key (decrypted from ci_nonce) set to ci_key */
			memcpy(crypt_info->ci_key, raw_key,
			       crypt_info->ci_key_len);

		res = fscrypt_get_metadata(inode, crypt_info);
		if (res && res != -EOPNOTSUPP)
			goto out;
		else
			res = 0;
	}

	if (enc) /* raw nonce is encrypted and copied to ci_nonce */
		memcpy(crypt_info->ci_nonce, raw_key,
		       FS_KEY_DERIVATION_CIPHER_SIZE);

	/*
	 * For existing inodes, multiple tasks may race to set ->i_crypt_info.
	 * So use cmpxchg_release().  This pairs with the smp_load_acquire() in
	 * fscrypt_get_info().  I.e., here we publish ->i_crypt_info with a
	 * RELEASE barrier so that other tasks can ACQUIRE it.
	 */
	if (cmpxchg_release(&inode->i_crypt_info, NULL, crypt_info) == NULL) {
		/*
		 * We won the race and set ->i_crypt_info to our crypt_info.
		 * Now link it into the master key's inode list.
		 */
		if (master_key) {
			struct fscrypt_master_key *mk =
				master_key->payload.data[0];

			refcount_inc(&mk->mk_refcount);
			crypt_info->ci_master_key = key_get(master_key);
			spin_lock(&mk->mk_decrypted_inodes_lock);
			list_add(&crypt_info->ci_master_key_link,
				 &mk->mk_decrypted_inodes);
			spin_unlock(&mk->mk_decrypted_inodes_lock);
		}
		crypt_info = NULL;
	}
	res = 0;
#ifdef CONFIG_FS_INLINE_ENCRYPT_DEBUG
	char *ci_key_str =
		kasprintf(GFP_KERNEL, "%*phN", inode->i_crypt_info->ci_key_len,
			  (u8 *)inode->i_crypt_info->ci_key);
	char *metadata_str = kasprintf(GFP_KERNEL, "%*phN", METADATA_BYTE_IN_KDF,
				 inode->i_crypt_info->ci_metadata);
	fscrypt_debug(inode, "[FBE]%s ci_key = %s, metadata = %s", __func__,
		     ci_key_str, metadata_str);
	kfree(ci_key_str);
	kfree(metadata_str);
#endif
out:
	if (master_key) {
		up_read(&master_key->sem);
		key_put(master_key);
	}
	put_crypt_info(crypt_info);
	kfree_sensitive(raw_key);
	return res;
}

static int get_sdp_crypt_info(struct inode *inode, int *flag)
{
	if (!S_ISREG(inode->i_mode))
		return 0;
	if (inode->i_crypt_info && !inode->i_crypt_info->ci_hw_enc_flag)
		return 0;
	if (inode->i_sb->s_cop && inode->i_sb->s_cop->get_keyinfo)
		return inode->i_sb->s_cop->get_keyinfo(inode, NULL, flag);
	return 0;
}

/**
 * fscrypt_get_encryption_info() - set up an inode's encryption key
 * @inode: the inode to set up the key for.  Must be encrypted.
 * @allow_unsupported: if %true, treat an unsupported encryption policy (or
 *		       unrecognized encryption context) the same way as the key
 *		       being unavailable, instead of returning an error.  Use
 *		       %false unless the operation being performed is needed in
 *		       order for files (or directories) to be deleted.
 *
 * Set up ->i_crypt_info, if it hasn't already been done.
 *
 * Note: unless ->i_crypt_info is already set, this isn't %GFP_NOFS-safe.  So
 * generally this shouldn't be called from within a filesystem transaction.
 *
 * Return: 0 if ->i_crypt_info was set or was already set, *or* if the
 *	   encryption key is unavailable.  (Use fscrypt_has_encryption_key() to
 *	   distinguish these cases.)  Also can return another -errno code.
 */
int fscrypt_get_encryption_info(struct inode *inode, bool allow_unsupported)
{
	int res;
	union fscrypt_context ctx;
	union fscrypt_policy policy;
	int flag = 0;

	res = get_sdp_crypt_info(inode, &flag);
	if (res != 0) {
		pr_err("fscrypt_sdp: get_keyinfo failed res:%d", res);
		return res;
	}
	if (flag) /* get sdp or dps crypt info success */
		return 0;

	if (fscrypt_has_encryption_key(inode))
		return 0;

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (res < 0) {
		if (res == -ERANGE && allow_unsupported)
			return 0;
		fscrypt_warn(inode, "Error %d getting encryption context", res);
		return res;
	}

	res = fscrypt_policy_from_context(&policy, &ctx, res);
	if (res) {
		if (allow_unsupported)
			return 0;
		fscrypt_warn(inode,
			     "Unrecognized or corrupt encryption context");
		return res;
	}

	if (!fscrypt_supported_policy(&policy, inode)) {
		if (allow_unsupported)
			return 0;
		return -EINVAL;
	}

	res = fscrypt_setup_encryption_info(
		inode, &policy, fscrypt_context_nonce(&ctx),
		fscrypt_context_iv(&ctx),
		IS_CASEFOLDED(inode) && S_ISDIR(inode->i_mode), false);
	if (res == -ENOPKG && allow_unsupported) /* Algorithm unavailable? */
		res = 0;
	if (res == -ENOKEY)
		res = 0;
	return res;
}

static void derive_crypt_complete(struct crypto_async_request *req, int rc)
{
	struct fscrypt_completion_result *ecr = req->data;

	if (rc == -EINPROGRESS)
		return;

	ecr->res = rc;
	complete(&ecr->completion);
}

int fscrypt_set_gcm_key(struct crypto_aead *tfm,
			const u8 deriving_key[FS_AES_256_GCM_KEY_SIZE])
{
	int res = 0;
	unsigned int iv_len;

	crypto_aead_set_flags(tfm, CRYPTO_TFM_REQ_FORBID_WEAK_KEYS);

	iv_len = crypto_aead_ivsize(tfm);
	if (iv_len > FS_KEY_DERIVATION_IV_SIZE) {
		res = -EINVAL;
		pr_err("fscrypt %s : IV length is incompatible\n", __func__);
		goto out;
	}

	res = crypto_aead_setauthsize(tfm, FS_KEY_DERIVATION_TAG_SIZE);
	if (res < 0) {
		pr_err("fscrypt %s : Failed to set authsize\n", __func__);
		goto out;
	}

	res = crypto_aead_setkey(tfm, deriving_key, FS_AES_256_GCM_KEY_SIZE);
	if (res < 0)
		pr_err("fscrypt %s : Failed to set deriving key\n", __func__);
out:
	return res;
}

int fscrypt_derive_gcm_key(struct crypto_aead *tfm,
			   const u8 source_key[FS_KEY_DERIVATION_CIPHER_SIZE],
			   u8 derived_key[FS_KEY_DERIVATION_CIPHER_SIZE],
			   u8 iv[FS_KEY_DERIVATION_IV_SIZE], int enc)
{
	int res = 0;
	struct aead_request *req = NULL;
	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct scatterlist src_sg, dst_sg;
	unsigned int ilen;
#ifdef CONFIG_VMAP_STACK
	u8 *src = NULL;
	u8 *dst = NULL;

	src = kmalloc(FS_KEY_DERIVATION_CIPHER_SIZE, GFP_NOFS);
	if (!src) {
		res = -ENOMEM;
		goto out;
	}
	dst = kmalloc(FS_KEY_DERIVATION_CIPHER_SIZE, GFP_NOFS);
	if (!dst) {
		res = -ENOMEM;
		goto out;
	}
#endif

	if (!tfm) {
		res = -EINVAL;
		goto out;
	}

	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		goto out;
	}

	req = aead_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}

	aead_request_set_callback(
		req, CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
		derive_crypt_complete, &ecr);

	ilen = enc ? FS_KEY_DERIVATION_NONCE_SIZE :
		     FS_KEY_DERIVATION_CIPHER_SIZE;

#ifdef CONFIG_VMAP_STACK
	memcpy(src, source_key, FS_KEY_DERIVATION_CIPHER_SIZE);
	memcpy(dst, derived_key, FS_KEY_DERIVATION_CIPHER_SIZE);
	sg_init_one(&src_sg, src, FS_KEY_DERIVATION_CIPHER_SIZE);
	sg_init_one(&dst_sg, dst, FS_KEY_DERIVATION_CIPHER_SIZE);
#else
	sg_init_one(&src_sg, source_key, FS_KEY_DERIVATION_CIPHER_SIZE);
	sg_init_one(&dst_sg, derived_key, FS_KEY_DERIVATION_CIPHER_SIZE);
#endif

	aead_request_set_ad(req, 0);

	aead_request_set_crypt(req, &src_sg, &dst_sg, ilen, iv);
	res = enc ? crypto_aead_encrypt(req) : crypto_aead_decrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
	}
#ifdef CONFIG_VMAP_STACK
	memcpy(derived_key, dst, FS_KEY_DERIVATION_CIPHER_SIZE);
out:
	kfree(src);
	kfree(dst);
#else
out:
#endif

	if (req)
		aead_request_free(req);
	return res;
}

struct key *fscrypt_request_key(const u8 *descriptor, const u8 *prefix,
			       int prefix_size)
{
	u8 *full_key_descriptor = NULL;
	struct key *keyring_key = NULL;
	int full_key_len = prefix_size + (FS_KEY_DESCRIPTOR_SIZE * 2) + 1;

	full_key_descriptor = kmalloc(full_key_len, GFP_NOFS);
	if (!full_key_descriptor)
		return (struct key *)ERR_PTR(-ENOMEM);

	memcpy(full_key_descriptor, prefix, prefix_size);
	sprintf(full_key_descriptor + prefix_size, "%*phN",
		FS_KEY_DESCRIPTOR_SIZE, descriptor);
	full_key_descriptor[full_key_len - 1] = '\0';
	keyring_key = request_key(&key_type_logon, full_key_descriptor, NULL);
	kfree(full_key_descriptor);

	return keyring_key;
}

/**
 * fscrypt_prepare_new_inode() - prepare to create a new inode in a directory
 * @dir: a possibly-encrypted directory
 * @inode: the new inode.  ->i_mode must be set already.
 *	   ->i_ino doesn't need to be set yet.
 * @encrypt_ret: (output) set to %true if the new inode will be encrypted
 *
 * If the directory is encrypted, set up its ->i_crypt_info in preparation for
 * encrypting the name of the new file.  Also, if the new inode will be
 * encrypted, set up its ->i_crypt_info and set *encrypt_ret=true.
 *
 * This isn't %GFP_NOFS-safe, and therefore it should be called before starting
 * any filesystem transaction to create the inode.  For this reason, ->i_ino
 * isn't required to be set yet, as the filesystem may not have set it yet.
 *
 * This doesn't persist the new inode's encryption context.  That still needs to
 * be done later by calling fscrypt_set_context().
 *
 * Return: 0 on success, -ENOKEY if the encryption key is missing, or another
 *	   -errno code
 */
int fscrypt_prepare_new_inode(struct inode *dir, struct inode *inode,
			      bool *encrypt_ret)
{
	const union fscrypt_policy *policy;
	u8 nonce[FS_KEY_DERIVATION_NONCE_SIZE];
	u8 plain_text[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};
	u8 iv[FS_KEY_DERIVATION_IV_SIZE];
	int ret = 0;

	policy = fscrypt_policy_to_inherit(dir);
	if (policy == NULL)
		return 0;
	if (IS_ERR(policy))
		return PTR_ERR(policy);

	if (WARN_ON_ONCE(inode->i_mode == 0))
		return -EINVAL;

	/*
	 * Only regular files, directories, and symlinks are encrypted.
	 * Special files like device nodes and named pipes aren't.
	 */
	if (!S_ISREG(inode->i_mode) &&
	    !S_ISDIR(inode->i_mode) &&
	    !S_ISLNK(inode->i_mode))
		return 0;

	*encrypt_ret = true;

	/* get nonce */
	if (fscrypt_generate_metadata_nonce(nonce, inode,
					    FS_KEY_DERIVATION_NONCE_SIZE))
		get_random_bytes(nonce, FS_KEY_DERIVATION_NONCE_SIZE);
	get_random_bytes(iv, FS_KEY_DERIVATION_IV_SIZE);
	memcpy(plain_text, nonce, FS_KEY_DERIVATION_NONCE_SIZE);

	ret = fscrypt_setup_encryption_info(
		inode, policy, plain_text, iv,
		IS_CASEFOLDED(dir) && S_ISDIR(inode->i_mode), true);
	return ret;
}
EXPORT_SYMBOL_GPL(fscrypt_prepare_new_inode);

/**
 * fscrypt_put_encryption_info() - free most of an inode's fscrypt data
 * @inode: an inode being evicted
 *
 * Free the inode's fscrypt_info.  Filesystems must call this when the inode is
 * being evicted.  An RCU grace period need not have elapsed yet.
 */
void fscrypt_put_encryption_info(struct inode *inode)
{
	put_crypt_info(inode->i_crypt_info);
	inode->i_crypt_info = NULL;
}
EXPORT_SYMBOL(fscrypt_put_encryption_info);

/**
 * fscrypt_free_inode() - free an inode's fscrypt data requiring RCU delay
 * @inode: an inode being freed
 *
 * Free the inode's cached decrypted symlink target, if any.  Filesystems must
 * call this after an RCU grace period, just before they free the inode.
 */
void fscrypt_free_inode(struct inode *inode)
{
	if (IS_ENCRYPTED(inode) && S_ISLNK(inode->i_mode)) {
		kfree(inode->i_link);
		inode->i_link = NULL;
	}
}
EXPORT_SYMBOL(fscrypt_free_inode);

/**
 * fscrypt_drop_inode() - check whether the inode's master key has been removed
 * @inode: an inode being considered for eviction
 *
 * Filesystems supporting fscrypt must call this from their ->drop_inode()
 * method so that encrypted inodes are evicted as soon as they're no longer in
 * use and their master key has been removed.
 *
 * Return: 1 if fscrypt wants the inode to be evicted now, otherwise 0
 */
int fscrypt_drop_inode(struct inode *inode)
{
	const struct fscrypt_info *ci = fscrypt_get_info(inode);
	const struct fscrypt_master_key *mk;

	/*
	 * If ci is NULL, then the inode doesn't have an encryption key set up
	 * so it's irrelevant.  If ci_master_key is NULL, then the master key
	 * was provided via the legacy mechanism of the process-subscribed
	 * keyrings, so we don't know whether it's been removed or not.
	 */
	if (!ci || !ci->ci_master_key)
		return 0;
	mk = ci->ci_master_key->payload.data[0];

	/*
	 * With proper, non-racy use of FS_IOC_REMOVE_ENCRYPTION_KEY, all inodes
	 * protected by the key were cleaned by sync_filesystem().  But if
	 * userspace is still using the files, inodes can be dirtied between
	 * then and now.  We mustn't lose any writes, so skip dirty inodes here.
	 */
	if (inode->i_state & I_DIRTY_ALL)
		return 0;

	/*
	 * Note: since we aren't holding the key semaphore, the result here can
	 * immediately become outdated.  But there's no correctness problem with
	 * unnecessarily evicting.  Nor is there a correctness problem with not
	 * evicting while iput() is racing with the key being removed, since
	 * then the thread removing the key will either evict the inode itself
	 * or will correctly detect that it wasn't evicted due to the race.
	 */
	return !is_master_key_secret_present(&mk->mk_secret);
}
EXPORT_SYMBOL_GPL(fscrypt_drop_inode);

#ifdef CONFIG_HWDPS
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
#define MAX_HISI_KEY_INDEX 31
#define FS_KEY_INDEX_OFFSET 63
static int hwdps_get_key_index(u8 *descriptor)
{
	int res;

	struct key *keyring_key = NULL;
	const struct user_key_payload *ukp = NULL;
	struct fscrypt_key *primary_key = NULL;

	keyring_key = fscrypt_request_key(descriptor, FSCRYPT_KEY_DESC_PREFIX,
		FSCRYPT_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key)) {
		pr_err("hwdps request_key failed!\n");
		return PTR_ERR(keyring_key);
	}

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon) {
		pr_err("hwdps key type must be logon\n");
		res = -ENOKEY;
		goto out;
	}
	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) {
		/* key was revoked before we acquired its semaphore */
		pr_warn_once("hwdps key was revoked\n");
		res = -EKEYREVOKED;
		goto out;
	}
	if (ukp->datalen != sizeof(struct fscrypt_key)) {
		pr_warn_once("hwdps fscrypt key size err %d", ukp->datalen);
		res = -EINVAL;
		goto out;
	}
	primary_key = (struct fscrypt_key *)ukp->data;
	if (primary_key->size != FS_AES_256_GCM_KEY_SIZE) {
		pr_warn_once("hwdps key size err %d", primary_key->size);
		res = -ENOKEY;
		goto out;
	}
	res = (int) (*(primary_key->raw + FS_KEY_INDEX_OFFSET) & 0xff);

out:
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return res;
}
#endif

/*
 * Description: Get the context of a file inode whether support the hwdps.
 * Input: inode: the inode requesting fek struct
 * Input: ctx: the fscrypt_context of the inode
 * Return: 0: successfully get the context
 *         -EOPNOTSUPP: not support
 */
static int hwdps_do_get_context(struct inode *inode,
	union fscrypt_context *ctx, union fscrypt_policy *policy)
{
	int err;
	const struct fscrypt_context_v1 *ctx_u = NULL;
	struct fscrypt_policy_v1 *policy_u = NULL;

	pr_info_once("%s enter\n", __func__);
	if (!inode || !inode->i_sb || !inode->i_sb->s_cop ||
		!inode->i_sb->s_cop->get_context || !ctx)
		return -EOPNOTSUPP;

	err = inode->i_sb->s_cop->get_context(inode, ctx, sizeof(*ctx));
	if (err < 0 || err != sizeof(*ctx)) {
		pr_err("Error %d getting encryption context", err);
		return err;
	}
	ctx_u = &ctx->v1;
	policy_u = &policy->v1;

	policy_u->version = FSCRYPT_POLICY_V1;
	policy_u->contents_encryption_mode = ctx_u->contents_encryption_mode;
	policy_u->filenames_encryption_mode = ctx_u->filenames_encryption_mode;
	policy_u->flags = ctx_u->flags;
	(void)memcpy_s(policy_u->master_key_descriptor, sizeof(policy_u->master_key_descriptor),
		ctx_u->master_key_descriptor,
		sizeof(policy_u->master_key_descriptor));

	return 0;
}

int hwdps_update_context(struct inode *inode, uid_t new_uid)
{
	int err;
	uint32_t fek_len;
	union fscrypt_context ctx;
	union fscrypt_policy policy;
	encrypt_id id;
	uint8_t *encoded_wfek = NULL;
	uint8_t *fek = NULL;
	secondary_buffer_t fek_buffer = { &fek, &fek_len };
	buffer_t encoded_wfek_buffer = { NULL, 0 };
	uint32_t flags = 0;

	if (hwdps_check_support(inode, &flags) != 0)
		return 0;
	encoded_wfek = hwdps_do_get_attr(inode, HWDPS_ENCODED_WFEK_SIZE, flags);
	if (!encoded_wfek) {
		pr_err("hwdps_do_get_attr failed\n");
		return -ENOMEM;
	}
	encoded_wfek_buffer.data = encoded_wfek;
	encoded_wfek_buffer.len = HWDPS_ENCODED_WFEK_SIZE;
	err = hwdps_has_access(inode, &encoded_wfek_buffer, flags);
	if (err != 0) {
		pr_err("hwdps_has_access %d\n", err);
		goto free_encoded_wfek;
	}
	err = hwdps_do_get_context(inode, &ctx, &policy);
	if (err != 0) {
		pr_err("hwdps_do_get_context failed %d\n", err);
		goto free_encoded_wfek;
	}
	id.uid = new_uid;
	id.task_uid = inode->i_uid.val;
	err = hwdps_update_fek(ctx.v1.master_key_descriptor, &encoded_wfek_buffer,
		&fek_buffer, new_uid, inode->i_uid.val);
	if (err != 0) {
		pr_err("hwdps ino %lu update_fek err %d\n", inode->i_ino, err);
		goto free_fek;
	}
	if (inode->i_sb->s_cop->update_hwdps_attr)
		err = inode->i_sb->s_cop->update_hwdps_attr(inode, encoded_wfek,
			HWDPS_ENCODED_WFEK_SIZE, NULL);
	else
		err = -EINVAL;
	if (err != 0)
		pr_err("hwdps ino %lu update xattr err %d\n", inode->i_ino, err);

free_fek:
	kfree_sensitive(fek);
	if (err == -ENOKEY)
		err = 0;
free_encoded_wfek:
	kfree_sensitive(encoded_wfek);
	return err;
}
EXPORT_SYMBOL(hwdps_update_context);

static struct fscrypt_info *hwdps_get_fscrypt_info(union fscrypt_context *ctx,
	struct inode *inode, union fscrypt_policy *policy)
{
	int err;
	struct fscrypt_info *crypt_info = NULL;
	struct fscrypt_mode *mode = NULL;

	err = fscrypt_initialize(inode->i_sb->s_cop->flags);
	if (err != 0) {
		pr_err("hwdps ino %lu init fscrypt fail\n", inode->i_ino);
		return ERR_PTR(-EINVAL);
	}

	crypt_info = kmem_cache_zalloc(fscrypt_info_cachep, GFP_KERNEL);
	if (!crypt_info)
		return ERR_PTR(-ENOMEM);

	(void)memset_s(crypt_info, sizeof(*crypt_info), 0, sizeof(*crypt_info));
	crypt_info->ci_owns_key = true;
#ifdef CONFIG_FS_ENCRYPTION_INLINE_CRYPT
	crypt_info->ci_inlinecrypt = false;
#endif;
	crypt_info->ci_inode = inode;
	crypt_info->ci_gtfm = NULL;
	crypt_info->ci_key = NULL;
	crypt_info->ci_key_len = 0;
	crypt_info->ci_key_index = -1;

	crypt_info->ci_policy = *policy;
	mode = select_encryption_mode(&crypt_info->ci_policy, inode);
	if (IS_ERR(mode)) {
		put_crypt_info(crypt_info);
		return ERR_PTR(-EINVAL);
	}
	WARN_ON(mode->ivsize > FSCRYPT_MAX_IV_SIZE);
	crypt_info->ci_mode = mode;
	return crypt_info;
}

static int hwdps_set_inline_encryption(struct inode *inode,
	secondary_buffer_t fek_buffer, struct fscrypt_info *ci)
{
	ci->ci_key = kzalloc(FSCRYPT_MAX_KEY_SIZE, GFP_NOFS);
	if (!ci->ci_key)
		return -ENOMEM;
	ci->ci_key_len = *fek_buffer.len;
	if (memcpy_s(ci->ci_key, FSCRYPT_MAX_KEY_SIZE,
		*fek_buffer.data, *fek_buffer.len) != EOK)
		 return -EINVAL;
	ci->ci_hw_enc_flag = (u8)(HWDPS_XATTR_ENABLE_FLAG_NEW);
	/* cached kmem may have dirty data */
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
	if (inode->i_crypt_info && (inode->i_crypt_info->ci_key_index >= 0) &&
		(inode->i_crypt_info->ci_key_index <= MAX_HISI_KEY_INDEX))
		ci->ci_key_index = inode->i_crypt_info->ci_key_index;
	else
		ci->ci_key_index = hwdps_get_key_index(ci->ci_policy.v1.master_key_descriptor);
		if ((ci->ci_key_index < 0) || (ci->ci_key_index > MAX_HISI_KEY_INDEX)) {
			pr_err("ci_key_index: %d\n", ci->ci_key_index);
			return -EINVAL;
	}
#endif
	return 0;
}

static int hwdps_do_set_cipher(struct inode *inode,
	secondary_buffer_t fek_buffer, struct fscrypt_info *ci)
{
	int err;
	struct crypto_skcipher *tfm = NULL;

	if (S_ISREG(inode->i_mode) && inode->i_sb->s_cop->is_inline_encrypted &&
		inode->i_sb->s_cop->is_inline_encrypted(inode)) {
		err = hwdps_set_inline_encryption(inode, fek_buffer, ci);
		if (err != 0)
			pr_err("set inline encryption err:%d", err);
		return err;
	}
	err = fscrypt_select_encryption_impl(ci, false);
	if (err != 0) {
		pr_err("%s:select encryption key fail, res:%d!\n", __func__, err);
		return err;
	}

	tfm = fscrypt_allocate_skcipher(ci->ci_mode, (const u8 *)(*fek_buffer.data), ci->ci_inode);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	/*
	 * Pairs with the smp_load_acquire() in fscrypt_is_key_prepared().
	 * I.e., here we publish ->tfm with a RELEASE barrier so that
	 * concurrent tasks can ACQUIRE it.  Note that this concurrency is only
	 * possible for per-mode keys, not for per-file keys.
	 */
	smp_store_release(&(ci->ci_enc_key.tfm), tfm);
	ci->ci_hw_enc_flag = (u8)(HWDPS_XATTR_ENABLE_FLAG_NEW);
	return 0;
}

static int hwdps_check_support_and_get_attr(struct inode *inode, u32 *flags,
	buffer_t *encoded_wfek)
{
	int err;

	err = hwdps_check_support(inode, flags);
	if (err != 0)
		return err;

	if (*flags == HWDPS_XATTR_ENABLE_FLAG) {
		encoded_wfek->data = hwdps_do_get_attr(inode,
			HWDPS_ENCODE_WFEK_SIZE_OLD, *flags);
		encoded_wfek->len = HWDPS_ENCODE_WFEK_SIZE_OLD;
	} else {
		encoded_wfek->data = hwdps_do_get_attr(inode,
			HWDPS_ENCODED_WFEK_SIZE, *flags);
		encoded_wfek->len = HWDPS_ENCODED_WFEK_SIZE;
	}
	if (!encoded_wfek->data)
		return -ENOMEM;
	return 0;
}

/*
 * mainly copied from fscrypt_get_encryption_info
 *
 * Return:
 *  o -EAGAIN: file is not protected by HWDPS
 *  o -EPERM: no permission to open the file
 *  o 0: SUCC
 *  o other errno: other errors
 */
int hwdps_get_context(struct inode *inode)
{
	int err;
	union fscrypt_context ctx;
	union fscrypt_policy policy;
	struct fscrypt_info *ci = NULL;
	buffer_t encoded_wfek = { NULL, 0 };
	uint8_t *fek = NULL;
	uint32_t fek_len = 0;
	uint32_t flags = 0;
	secondary_buffer_t fek_buffer = { &fek, &fek_len };
	err = hwdps_check_support_and_get_attr(inode, &flags, &encoded_wfek);
	if (err != 0)
		return err;

	if (inode->i_crypt_info) {
		err = hwdps_has_access(inode, &encoded_wfek, flags);
		goto free_encoded_wfek; /* return anyway */
	}

	err = hwdps_do_get_context(inode, &ctx, &policy);
	if (err != 0)
		goto free_encoded_wfek;

	ci = hwdps_get_fscrypt_info(&ctx, inode, &policy);
	if (IS_ERR(ci)) {
		err = PTR_ERR(ci);
		goto free_encoded_wfek;
	}
	err = hwdps_get_fek(ctx.v1.master_key_descriptor, inode, &encoded_wfek,
		&fek_buffer, flags);
	if (err != 0)
		goto free_fek;
	err = hwdps_do_set_cipher(inode, fek_buffer, ci);
	if (err != 0)
		goto free_fek;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	err = fscrypt_get_metadata(inode, ci);
	if (err && err != -EOPNOTSUPP)
		goto free_fek;
	else
		err = 0;
#endif
	if (cmpxchg(&inode->i_crypt_info, NULL, ci) == NULL)
		ci = NULL;
free_fek:
	kfree_sensitive(fek);
	if (err == -ENOKEY)
		err = 0;
	put_crypt_info(ci);
free_encoded_wfek:
	kfree_sensitive(encoded_wfek.data);
	return err;
}
EXPORT_SYMBOL(hwdps_get_context);
#endif
