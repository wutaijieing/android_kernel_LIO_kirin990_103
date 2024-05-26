// SPDX-License-Identifier: GPL-2.0
/*
 * Key setup for v1 encryption policies
 *
 * Copyright 2015, 2019 Google LLC
 */

/*
 * This file implements compatibility functions for the original encryption
 * policy version ("v1"), including:
 *
 * - Deriving per-file encryption keys using the AES-128-ECB based KDF
 *   (rather than the new method of using HKDF-SHA512)
 *
 * - Retrieving fscrypt master keys from process-subscribed keyrings
 *   (rather than the new method of using a filesystem-level keyring)
 *
 * - Handling policies with the DIRECT_KEY flag set using a master key table
 *   (rather than the new method of implementing DIRECT_KEY with per-mode keys
 *    managed alongside the master keys in the filesystem-level keyring)
 */

#include <crypto/algapi.h>
#include <crypto/skcipher.h>
#include <keys/user-type.h>
#include <linux/hashtable.h>
#include <linux/scatterlist.h>
#include <linux/fscrypt_common.h>

#include "fscrypt_private.h"

/* Table of keys referenced by DIRECT_KEY policies */
static DEFINE_HASHTABLE(fscrypt_direct_keys, 6); /* 6 bits = 64 buckets */
static DEFINE_SPINLOCK(fscrypt_direct_keys_lock);

/*
 * v1 key derivation function.  This generates the derived key by encrypting the
 * master key with AES-128-ECB using the nonce as the AES key.  This provides a
 * unique derived key with sufficient entropy for each inode.  However, it's
 * nonstandard, non-extensible, doesn't evenly distribute the entropy from the
 * master key, and is trivially reversible: an attacker who compromises a
 * derived key can "decrypt" it to get back to the master key, then derive any
 * other key.  For all new code, use HKDF instead.
 *
 * The master key must be at least as long as the derived key.  If the master
 * key is longer, then only the first 'derived_keysize' bytes are used.
 */
static int derive_key_aes(const u8 *master_key,
			  const u8 nonce[FSCRYPT_FILE_NONCE_SIZE],
			  u8 *derived_key, unsigned int derived_keysize)
{
	int res = 0;
	struct skcipher_request *req = NULL;
	DECLARE_CRYPTO_WAIT(wait);
	struct scatterlist src_sg, dst_sg;
	struct crypto_skcipher *tfm = crypto_alloc_skcipher("ecb(aes)", 0, 0);

	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out;
	}
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_FORBID_WEAK_KEYS);
	req = skcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}
	skcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			crypto_req_done, &wait);
	res = crypto_skcipher_setkey(tfm, nonce, FSCRYPT_FILE_NONCE_SIZE);
	if (res < 0)
		goto out;

	sg_init_one(&src_sg, master_key, derived_keysize);
	sg_init_one(&dst_sg, derived_key, derived_keysize);
	skcipher_request_set_crypt(req, &src_sg, &dst_sg, derived_keysize,
				   NULL);
	res = crypto_wait_req(crypto_skcipher_encrypt(req), &wait);
out:
	skcipher_request_free(req);
	crypto_free_skcipher(tfm);
	return res;
}

/*
 * Search the current task's subscribed keyrings for a "logon" key with
 * description prefix:descriptor, and if found acquire a read lock on it and
 * return a pointer to its validated payload in *payload_ret.
 */
static struct key *
find_and_lock_process_key(const char *prefix,
			  const u8 descriptor[FSCRYPT_KEY_DESCRIPTOR_SIZE],
			  unsigned int min_keysize,
			  const struct fscrypt_key **payload_ret)
{
	char *description;
	struct key *key;
	const struct user_key_payload *ukp;
	struct fscrypt_key *payload = NULL;

	description = kasprintf(GFP_KERNEL, "%s%*phN", prefix,
				FSCRYPT_KEY_DESCRIPTOR_SIZE, descriptor);
	if (!description)
		return ERR_PTR(-ENOMEM);

#ifdef CONFIG_FS_INLINE_ENCRYPT_DEBUG
	fscrypt_debug(NULL, "[FBE]%s key descriptor = %s", __func__, description);
#endif

	key = request_key(&key_type_logon, description, NULL);
	kfree(description);
	if (IS_ERR(key)) {
		fscrypt_warn(
			NULL,
			"[FBE]%s line %d, prefix %s, prefix len %d, err = %d",
			__func__, __LINE__, prefix, strlen(prefix),
			PTR_ERR(key));
		return key;
	}

	down_read(&key->sem);
	ukp = user_key_payload_locked(key);
	if (!ukp) { /* was the key revoked before we acquired its semaphore? */
		fscrypt_warn(NULL, "[FBE]%s line %d err = %d", __func__,
			     __LINE__, PTR_ERR(ukp));
		goto invalid;
	}

	payload = (struct fscrypt_key *)ukp->data;

	if (ukp->datalen != sizeof(struct fscrypt_key) ||
	    payload->size < 1 || payload->size > FSCRYPT_MAX_KEY_SIZE) {
		fscrypt_warn(NULL,
			     "key with description '%s' has invalid payload",
			     key->description);
		goto invalid;
	}

	/* force the size equal to FS_AES_256_GCM_KEY_SIZE since user space might pass FS_AES_256_XTS_KEY_SIZE */
	payload->size = FS_AES_256_GCM_KEY_SIZE;
	if (payload->size != FS_AES_256_GCM_KEY_SIZE) {
		fscrypt_warn(NULL,
			     "key with description '%s' is too short (got %u bytes, need %u+ bytes)",
			     key->description, payload->size, min_keysize);
		goto invalid;
	}

	*payload_ret = payload;
	return key;

invalid:
	up_read(&key->sem);
	key_put(key);
	return ERR_PTR(-ENOKEY);
}

/* Master key referenced by DIRECT_KEY policy */
struct fscrypt_direct_key {
	struct hlist_node		dk_node;
	refcount_t			dk_refcount;
	const struct fscrypt_mode	*dk_mode;
	struct fscrypt_prepared_key	dk_key;
	u8				dk_descriptor[FSCRYPT_KEY_DESCRIPTOR_SIZE];
	u8				dk_raw[FSCRYPT_MAX_KEY_SIZE];
};

static void free_direct_key(struct fscrypt_direct_key *dk)
{
	if (dk) {
		fscrypt_destroy_prepared_key(&dk->dk_key);
		kfree_sensitive(dk);
	}
}

void fscrypt_put_direct_key(struct fscrypt_direct_key *dk)
{
	if (!refcount_dec_and_lock(&dk->dk_refcount, &fscrypt_direct_keys_lock))
		return;
	hash_del(&dk->dk_node);
	spin_unlock(&fscrypt_direct_keys_lock);

	free_direct_key(dk);
}

/*
 * Find/insert the given key into the fscrypt_direct_keys table.  If found, it
 * is returned with elevated refcount, and 'to_insert' is freed if non-NULL.  If
 * not found, 'to_insert' is inserted and returned if it's non-NULL; otherwise
 * NULL is returned.
 */
static struct fscrypt_direct_key *
find_or_insert_direct_key(struct fscrypt_direct_key *to_insert,
			  const u8 *raw_key, const struct fscrypt_info *ci)
{
	unsigned long hash_key;
	struct fscrypt_direct_key *dk;

	/*
	 * Careful: to avoid potentially leaking secret key bytes via timing
	 * information, we must key the hash table by descriptor rather than by
	 * raw key, and use crypto_memneq() when comparing raw keys.
	 */

	BUILD_BUG_ON(sizeof(hash_key) > FSCRYPT_KEY_DESCRIPTOR_SIZE);
	memcpy(&hash_key, ci->ci_policy.v1.master_key_descriptor,
	       sizeof(hash_key));

	spin_lock(&fscrypt_direct_keys_lock);
	hash_for_each_possible(fscrypt_direct_keys, dk, dk_node, hash_key) {
		if (memcmp(ci->ci_policy.v1.master_key_descriptor,
			   dk->dk_descriptor, FSCRYPT_KEY_DESCRIPTOR_SIZE) != 0)
			continue;
		if (ci->ci_mode != dk->dk_mode)
			continue;
		if (!fscrypt_is_key_prepared(&dk->dk_key, ci))
			continue;
		if (crypto_memneq(raw_key, dk->dk_raw, ci->ci_mode->keysize))
			continue;
		/* using existing tfm with same (descriptor, mode, raw_key) */
		refcount_inc(&dk->dk_refcount);
		spin_unlock(&fscrypt_direct_keys_lock);
		free_direct_key(to_insert);
		return dk;
	}
	if (to_insert)
		hash_add(fscrypt_direct_keys, &to_insert->dk_node, hash_key);
	spin_unlock(&fscrypt_direct_keys_lock);
	return to_insert;
}

/* Prepare to encrypt directly using the master key in the given mode */
static struct fscrypt_direct_key *
fscrypt_get_direct_key(const struct fscrypt_info *ci, const u8 *raw_key)
{
	struct fscrypt_direct_key *dk;
	int err;

	/* Is there already a tfm for this key? */
	dk = find_or_insert_direct_key(NULL, raw_key, ci);
	if (dk)
		return dk;

	/* Nope, allocate one. */
	dk = kzalloc(sizeof(*dk), GFP_KERNEL);
	if (!dk)
		return ERR_PTR(-ENOMEM);
	refcount_set(&dk->dk_refcount, 1);
	dk->dk_mode = ci->ci_mode;
	err = fscrypt_prepare_key(&dk->dk_key, raw_key, ci->ci_mode->keysize,
				  false /*is_hw_wrapped*/, ci);
	if (err)
		goto err_free_dk;
	memcpy(dk->dk_descriptor, ci->ci_policy.v1.master_key_descriptor,
	       FSCRYPT_KEY_DESCRIPTOR_SIZE);
	memcpy(dk->dk_raw, raw_key, ci->ci_mode->keysize);

	return find_or_insert_direct_key(dk, raw_key, ci);

err_free_dk:
	free_direct_key(dk);
	return ERR_PTR(err);
}

/* v1 policy, DIRECT_KEY: use the master key directly */
static int setup_v1_file_key_direct(struct fscrypt_info *ci,
				    const u8 *raw_master_key)
{
	struct fscrypt_direct_key *dk;

	dk = fscrypt_get_direct_key(ci, raw_master_key);
	if (IS_ERR(dk))
		return PTR_ERR(dk);
	ci->ci_direct_key = dk;
	ci->ci_enc_key = dk->dk_key;
	return 0;
}

/* v1 policy, !DIRECT_KEY: derive the file's encryption key */
static int setup_v1_file_key_derived(struct fscrypt_info *ci,
				     const u8 *raw_master_key)
{
	u8 *derived_key;
	int err;

	/*
	 * This cannot be a stack buffer because it will be passed to the
	 * scatterlist crypto API during derive_key_aes().
	 */
	derived_key = kmalloc(ci->ci_mode->keysize, GFP_KERNEL);
	if (!derived_key)
		return -ENOMEM;

	memcpy(derived_key, raw_master_key, ci->ci_mode->keysize);
	err = fscrypt_set_per_file_enc_key(ci, derived_key);
out:
	kfree_sensitive(derived_key);
	return err;
}

int fscrypt_setup_v1_file_key(struct fscrypt_info *ci, const u8 *raw_master_key)
{
	if (ci->ci_policy.v1.flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY)
		return setup_v1_file_key_direct(ci, raw_master_key);
	else
		return setup_v1_file_key_derived(ci, raw_master_key);
}

int fscrypt_setup_v1_file_key_via_subscribed_keyrings(
	struct fscrypt_info *ci, u8 *derived_key, unsigned int derived_keysize,
	bool enc)
{
	struct key *key;
	const struct fscrypt_key *payload;
	struct crypto_aead *tfm = NULL;
	u8 plain_text[FS_KEY_DERIVATION_CIPHER_SIZE] = { 0 };
	int err;

	key = find_and_lock_process_key(FSCRYPT_KEY_DESC_PREFIX,
					ci->ci_policy.v1.master_key_descriptor,
					ci->ci_mode->keysize, &payload);
	if (key == ERR_PTR(-ENOKEY) && ci->ci_inode->i_sb->s_cop->key_prefix) {
		key = find_and_lock_process_key(ci->ci_inode->i_sb->s_cop->key_prefix,
						ci->ci_policy.v1.master_key_descriptor,
						ci->ci_mode->keysize, &payload);
	}
	if (IS_ERR(key))
		return PTR_ERR(key);

	ci->ci_key_index = (int)*(payload->raw + KEY_SLOT_OFFSET) & 0xff;

	tfm = (struct crypto_aead *)crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		err = (int)PTR_ERR(tfm);
		tfm = NULL;
		pr_err("fscrypt %s : tfm allocation failed!\n", __func__);
		goto out;
	}

#ifdef CONFIG_FS_INLINE_ENCRYPT_DEBUG
	char *payload_str = kasprintf(GFP_KERNEL, "%*phN", FSCRYPT_MAX_KEY_SIZE,
				      payload->raw);
	fscrypt_debug(ci->ci_inode, "[FBE]%s payload raw = %s", __func__, payload_str);
	kfree(payload_str);
#endif

	err = fscrypt_set_gcm_key(tfm, payload->raw);
	if (err)
		goto out;

#ifdef CONFIG_FS_INLINE_ENCRYPT_DEBUG
	char *nonce = kasprintf(GFP_KERNEL, "%*phN",
				FS_KEY_DERIVATION_CIPHER_SIZE, ci->ci_nonce);
	char *iv = kasprintf(GFP_KERNEL, "%*phN", FS_KEY_DERIVATION_IV_SIZE,
			     ci->ci_iv);
	fscrypt_debug(
		ci->ci_inode,
		"[FBE]%s key index = %d, mode size = %d, payload size = %d, nonce = %s, nonce + 64 = %s iv = %s, enc = %d",
		__func__, ci->ci_key_index, ci->ci_mode->keysize, payload->size,
		nonce, nonce + FS_KEY_DERIVATION_NONCE_SIZE, iv, enc);
	kfree(nonce);
	kfree(iv);
#endif
	err = fscrypt_derive_gcm_key(tfm, ci->ci_nonce, plain_text, ci->ci_iv,
				     enc);
	if (err)
		goto out;

#ifdef CONFIG_FS_INLINE_ENCRYPT_DEBUG
	char *plain_text_str = kasprintf(
		GFP_KERNEL, "%*phN", FS_KEY_DERIVATION_CIPHER_SIZE, plain_text);
	fscrypt_debug(ci->ci_inode, "[FBE]%s after gcm nonce: %s, nonce + 64 %s",
		     __func__, plain_text_str,
		     plain_text_str + FS_KEY_DERIVATION_NONCE_SIZE);
	kfree(plain_text_str);
#endif
	memcpy(derived_key, plain_text, derived_keysize);
	ci->ci_gtfm = tfm;
	err = fscrypt_setup_v1_file_key(ci, enc ? ci->ci_nonce : derived_key);
	up_read(&key->sem);
	key_put(key);
	return 0;
out:

	if (tfm)
		crypto_free_aead(tfm);
	up_read(&key->sem);
	key_put(key);
	return err;
}
