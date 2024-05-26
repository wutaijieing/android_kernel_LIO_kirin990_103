// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * Description: Internal implementation of ECDH for
 *              sdp(sensitive data protection) SECE case.
 * Create: 2022.02.22
 */

#include "ecdh.h"
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <linux/printk.h>
#include <linux/f2fs_fs.h>
#include <securec.h>
#include "sdp_internal.h"

struct tcrypt_result {
	struct completion comp;
	int err;
};

static void complete_crypto(struct crypto_async_request *req, int err)
{
	struct tcrypt_result *ret = req->data;

	if (err == -EINPROGRESS || !ret)
		return;

	ret->err = err;
	complete(&ret->comp);
}

static int wait_sync_opt(int res, struct tcrypt_result *tr)
{
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&tr->comp);
		reinit_completion(&tr->comp);
		res = tr->err;
	}
	return res;
}

static int set_param(u32 curve_id, struct crypto_kpp *tfm)
{
	int err;
	unsigned int ecdh_size;
	void *ecdh_buf = NULL;
	struct ecdh *ecdh_param = NULL;

	ecdh_param = kzalloc(sizeof(struct ecdh), GFP_KERNEL);
	if (!ecdh_param) {
		err = -ENOMEM;
		goto out_and_free;
	}

	ecdh_param->curve_id = curve_id;
	ecdh_param->key = NULL;
	ecdh_param->key_size = 0;
	ecdh_size = crypto_ecdh_key_len(ecdh_param);
	if (ecdh_size == 0 || ecdh_size > MAX_ECDH_SIZE) {
		sdp_pr_err("%s ecdh size err %u\n", __func__, ecdh_size);
		err = -EINVAL;
		goto out_and_free;
	}
	ecdh_buf = kzalloc(ecdh_size, GFP_KERNEL);
	if (!ecdh_buf) {
		err = -ENOMEM;
		goto out_and_free;
	}

	err = crypto_ecdh_encode_key(ecdh_buf, ecdh_size, ecdh_param);
	if (err < 0) {
		sdp_pr_err("%s crypto_ecdh_encode_key failed err %d", __func__,
			err);
		err = -EFAULT;
		goto out_and_free;
	}

	/* Generate file private key */
	err = crypto_kpp_set_secret(tfm, ecdh_buf, ecdh_size);
	if (err < 0) {
		sdp_pr_err("%s crypto_kpp_set_secret failed err %d", __func__,
			err);
		err = -EFAULT;
		goto out_and_free;
	}
	err = 0;
out_and_free:
	kfree_sensitive(ecdh_param);
	kfree_sensitive(ecdh_buf);
	return err;
}

static int gen_file_pub_key(struct kpp_request *req,
	secondary_buffer_t *pub_key_buf, struct scatterlist *dst,
	struct tcrypt_result *result, buffer_t *file_pub_key)
{
	int err;

	if (*(pub_key_buf->len) != FS_SDP_ECC_PUB_KEY_SIZE) {
		sdp_pr_err("%s publen err%u\n", __func__, *(pub_key_buf->len));
		return -EINVAL;
	}
	*(pub_key_buf->data) = kzalloc(*(pub_key_buf->len), GFP_KERNEL);
	if (!*(pub_key_buf->data))
		return -ENOMEM;

	/*
	 * File private key has been in the kpp request.
	 * No other input is needed for generating public key.
	 */
	kpp_request_set_input(req, NULL, 0);
	sg_init_one(dst, *(pub_key_buf->data), *(pub_key_buf->len));
	kpp_request_set_output(req, dst, *(pub_key_buf->len));
	kpp_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
		complete_crypto, result);

	err = wait_sync_opt(crypto_kpp_generate_public_key(req), result);
	if (err != 0) {
		sdp_pr_err("Alg: generates file public key fail err %d\n", err);
		goto fail_and_free;
	}
	if (memcpy_s(file_pub_key->data, file_pub_key->len,
		sg_virt(req->dst), file_pub_key->len) != EOK) {
		err = -EFAULT;
		goto fail_and_free;
	}
	return err;
fail_and_free:
	kfree_sensitive(*(pub_key_buf->data));
	*(pub_key_buf->data) = NULL;
	return err;
}

static int set_param_for_gen_secret(u32 curve_id, struct crypto_kpp *tfm,
	const buffer_t *dev_priv_key)
{
	unsigned int ecdh_size;
	int err;
	struct ecdh *ecdh_param = NULL;
	void *ecdh_buf = NULL;

	/* Set the ecdh params */
	ecdh_param = kzalloc(sizeof(struct ecdh), GFP_KERNEL);
	if (!ecdh_param) {
		err = -ENOMEM;
		goto out_and_free;
	}
	ecdh_param->curve_id = curve_id;
	ecdh_param->key_size = FS_SDP_ECC_PRIV_KEY_SIZE;
	ecdh_param->key = dev_priv_key->data;
	ecdh_size = crypto_ecdh_key_len(ecdh_param);
	if (ecdh_size == 0 || ecdh_size > MAX_ECDH_SIZE) {
		sdp_pr_err("%s ecdh size err %u", __func__, ecdh_size);
		err = -EFAULT;
		goto out_and_free;
	}
	ecdh_buf = kzalloc(ecdh_size, GFP_KERNEL);
	if (!ecdh_buf) {
		err = -ENOMEM;
		goto out_and_free;
	}
	err = crypto_ecdh_encode_key(ecdh_buf, ecdh_size, ecdh_param);
	if (err < 0) {
		sdp_pr_err("%s encode_key err %d\n", __func__, err);
		err = -EFAULT;
		goto out_and_free;
	}
	/* Set the device private key */
	err = crypto_kpp_set_secret(tfm, ecdh_buf, ecdh_size);
	if (err < 0) {
		sdp_pr_err("%s set_secret err %d\n", __func__, err);
		err = -EFAULT;
		goto out_and_free;
	}
	err = 0;
out_and_free:
	kfree_sensitive(ecdh_param);
	kfree_sensitive(ecdh_buf);
	return err;
}

/*
 * tfm: Kpp transformation with ecdh algorithm
 * curve_id: Curve id, ECC_CURVE_NIST_P256 now
 * dev_pub_key: Device public key. Get from keyring.
 * file_pub_key[out]: Generated file public key. To be stored in file xattr.
 * shared_secret[out: Used to decrypt the file content encryption key in GCM
 *                    mode. Not stored.
 */
static int ecdh_gen_file_pubkey_secret(struct crypto_kpp *tfm, u32 curve_id,
	const buffer_t *dev_pub_key, buffer_t *file_pub_key, buffer_t *out)
{
	unsigned int pub_key_size;
	unsigned int privkey_len;
	int err;
	struct tcrypt_result result;
	struct scatterlist src;
	struct scatterlist dst;
	struct kpp_request *req = NULL;
	void *output_buf = NULL;
	void *input_buf = NULL;
	secondary_buffer_t pub_key_buf = { (u8 **)&output_buf, NULL };

	req = kpp_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return -EFAULT;
	init_completion(&result.comp);
	err = set_param(curve_id, tfm);
	if (err != 0)
		goto free_all;

	/* Compute and store file public key */
	pub_key_size = crypto_kpp_maxsize(tfm);
	if (pub_key_size != FS_SDP_ECC_PUB_KEY_SIZE) {
		sdp_pr_err("%s: pub_key_size:%u\n", __func__, pub_key_size);
		err = -EINVAL;
		goto free_all;
	}
	pub_key_buf.len = &pub_key_size;
	err = gen_file_pub_key(req, &pub_key_buf, &dst, &result, file_pub_key);
	if (err != 0)
		goto free_all;

	/*
	 * Calculate shared secret with file private key and
	 * device public key
	 */
	input_buf = kzalloc(pub_key_size, GFP_KERNEL);
	if (!input_buf) {
		err = -ENOMEM;
		goto free_all;
	}
	(void)memcpy_s(input_buf, pub_key_size, dev_pub_key->data,
		pub_key_size);
	privkey_len = pub_key_size >> 1;
	sg_init_one(&src, input_buf, pub_key_size);
	sg_init_one(&dst, output_buf, privkey_len);
	kpp_request_set_input(req, &src, pub_key_size);
	kpp_request_set_output(req, &dst, privkey_len);
	kpp_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
		 complete_crypto, &result);
	err = wait_sync_opt(crypto_kpp_compute_shared_secret(req), &result);
	if (err != 0) {
		sdp_pr_err("computes shared secret failed err %d\n", err);
		goto free_all;
	}

	if (memcpy_s(out->data, out->len, sg_virt(req->dst), out->len) != EOK)
		err = -EFAULT;
free_all:
	kfree_sensitive(input_buf);
	kfree_sensitive(output_buf);
	kpp_request_free(req);
	return err;
}

/*
 * tfm: Kpp transformation with ecdh algorithm
 * curve_id: Curve id, only ECC_CURVE_NIST_P256 now
 * dev_privkey: Device private key which is gotten from keyring.
 * file_pub_key: File public key which is gotten from file xattr.
 * shared_secret[out]: Used to decrypt the file content encryption key in GCM
 *                     mode. Not stored.
 */
static int ecdh_gen_secret(struct crypto_kpp *tfm, u32 curve_id,
	const buffer_t *dev_priv_key, const buffer_t *file_pub_key,
	buffer_t *out)
{
	unsigned int pub_key_size;
	unsigned int privkey_len;
	struct tcrypt_result result;
	struct scatterlist src;
	struct scatterlist dst;
	struct kpp_request *req = NULL;

	void *output_buf = NULL;
	void *input_buf = NULL;
	int err;

	req = kpp_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return -ENOMEM;
	init_completion(&result.comp);
	err = set_param_for_gen_secret(curve_id, tfm, dev_priv_key);
	if (err != 0)
		goto free_all;
	err = -EFAULT;
	/* Malloc for file public key (input) and shared secret (output) */
	pub_key_size = crypto_kpp_maxsize(tfm);
	privkey_len = pub_key_size >> 1;
	if (pub_key_size != FS_SDP_ECC_PUB_KEY_SIZE) {
		sdp_pr_err("%s pub key size err:%u\n", __func__, pub_key_size);
		goto free_all;
	}
	output_buf = kzalloc(privkey_len, GFP_KERNEL);
	if (!output_buf)
		goto free_all;

	input_buf = kzalloc(pub_key_size, GFP_KERNEL);
	if (!input_buf)
		goto free_all;

	(void)memcpy_s(input_buf, pub_key_size, file_pub_key->data,
		file_pub_key->len);
	sg_init_one(&src, input_buf, pub_key_size);
	sg_init_one(&dst, output_buf, privkey_len);
	kpp_request_set_input(req, &src, pub_key_size);
	kpp_request_set_output(req, &dst, privkey_len);
	kpp_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
		complete_crypto, &result);
	err = wait_sync_opt(crypto_kpp_compute_shared_secret(req), &result);
	if (err != 0) {
		sdp_pr_err("%s shared secret failed err %d\n", __func__, err);
		goto free_all;
	}

	/* store shared secret */
	if (memcpy_s(out->data, out->len, sg_virt(req->dst), out->len) != EOK)
		err = -EFAULT;
free_all:
	kfree_sensitive(input_buf);
	kfree_sensitive(output_buf);
	kpp_request_free(req);
	return err;
}

static int check_param_for_pub_key(u32 curve_id,
	const buffer_t *dev_pub_key,
	const buffer_t *file_pub_key,
	const buffer_t *shared_secret)
{
	if (!dev_pub_key || !file_pub_key || !shared_secret) {
		sdp_pr_err("%s, input param is NULL\n", __func__);
		return -EINVAL;
	}

	if (!dev_pub_key->data || !file_pub_key->data ||
		!shared_secret->data) {
		sdp_pr_err("%s data is NULL\n", __func__);
		return -EINVAL;
	}

	if (curve_id != ECC_CURVE_NIST_P256 ||
		dev_pub_key->len != FS_SDP_ECC_PUB_KEY_SIZE ||
		file_pub_key->len != FS_SDP_ECC_PUB_KEY_SIZE ||
		shared_secret->len != FS_SDP_ECC_PRIV_KEY_SIZE)
		return -EINVAL;
	return 0;
}

static int check_param_for_shared_key(u32 curve_id,
	const buffer_t *dev_priv_key,
	const buffer_t *file_pub_key,
	const buffer_t *shared_secret)
{
	if (!dev_priv_key || !file_pub_key || !shared_secret) {
		sdp_pr_err("%s, input param is NULL\n", __func__);
		return -EINVAL;
	}

	if (!dev_priv_key->data || !file_pub_key->data ||
		!shared_secret->data) {
		sdp_pr_err("%s data is NULL\n", __func__);
		return -EINVAL;
	}

	if (curve_id != ECC_CURVE_NIST_P256 ||
		dev_priv_key->len != FS_SDP_ECC_PRIV_KEY_SIZE ||
		file_pub_key->len != FS_SDP_ECC_PUB_KEY_SIZE ||
		shared_secret->len != FS_SDP_ECC_PRIV_KEY_SIZE)
		return -EINVAL;
	return 0;
}

int get_file_pubkey_shared_secret(u32 curve_id, const buffer_t *dev_pub_key,
	buffer_t *file_pub_key, buffer_t *shared_secret)
{
	int err;
	const char *alg_name = "ecdh";
	struct crypto_kpp *tfm = NULL;

	err = check_param_for_pub_key(curve_id, dev_pub_key, file_pub_key,
		shared_secret);
	if (err != 0)
		return err;

	tfm = crypto_alloc_kpp(alg_name, CRYPTO_ALG_TYPE_KPP,
		CRYPTO_ALG_TYPE_MASK);
	if (IS_ERR(tfm)) {
		sdp_pr_err("failed to alloc_kpp for %s: %ld\n", alg_name,
			PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}
	err = ecdh_gen_file_pubkey_secret(tfm, curve_id, dev_pub_key,
		file_pub_key, shared_secret);
	if (err != 0)
		sdp_pr_err("failed to gen file pub key and shared secret\n");

	crypto_free_kpp(tfm);
	return err;
}

int get_shared_secret(u32 curve_id, const buffer_t *dev_priv_key,
	const buffer_t *file_pub_key, buffer_t *shared_secret)
{
	int err;
	const char *alg_name = "ecdh";
	struct crypto_kpp *tfm = NULL;

	err = check_param_for_shared_key(curve_id, dev_priv_key, file_pub_key,
		shared_secret);
	if (err != 0)
		return err;

	tfm = crypto_alloc_kpp(alg_name, CRYPTO_ALG_TYPE_KPP,
		CRYPTO_ALG_TYPE_MASK);
	if (IS_ERR(tfm)) {
		sdp_pr_err("failed to load tfm for %s: %ld\n", alg_name,
			PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	err = ecdh_gen_secret(tfm, curve_id, dev_priv_key, file_pub_key,
		shared_secret);
	if (err != 0)
		sdp_pr_err("failed to generate shared secret\n");

	crypto_free_kpp(tfm);
	return err;
}
