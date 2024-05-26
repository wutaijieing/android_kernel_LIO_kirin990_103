/* SPDX-License-Identifier: GPL-2.0 */
/*
 * adapter_crypto.c
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
 * Description: Crypto for fusion communication
 * Author: wangminmin4@huawei.com
 * Create: 2020-03-31
 *
 */

#include <linux/bug.h>
#include <linux/random.h>

#include "adapter_crypto.h"
#include "comm/transport.h"
#include "hmdfs.h"

#define ENCRYPT_FLAG 1
#define DECRYPT_FLAG 0

struct aeadcrypt_result {
	struct completion completion;
	int err;
};

struct aeadcipher_ctx {
	struct aead_request *req;
	struct scatterlist *src;
	struct scatterlist *dst;
	struct aeadcrypt_result result;
	__u8 iv[HMDFS_IV_SIZE];
};

static void aeadcipher_cb(struct crypto_async_request *req, int error)
{
	struct aeadcrypt_result *result = req->data;

	if (error == -EINPROGRESS)
		return;

	result->err = error;
	complete(&result->completion);
}

static int aeadcipher_en_de(struct aead_request *req,
			    struct aeadcrypt_result *result, int flag)
{
	int rc = 0;

	if (flag)
		rc = crypto_aead_encrypt(req);
	else
		rc = crypto_aead_decrypt(req);
	switch (rc) {
	case 0:
		break;
	case -EINPROGRESS:
	case -EBUSY:
		rc = wait_for_completion_interruptible(&result->completion);
		if (!rc)
			rc = result->err;

		break;
	default:
		hmdfs_err("returned rc %d", rc);
		break;
	}
	return rc;
}

static void set_aeadcipher(struct crypto_aead *tfm, struct aead_request *req,
			   struct aeadcrypt_result *result)
{
	result->err = 0;
	init_completion(&result->completion);
	aead_request_set_callback(req,
		CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
		aeadcipher_cb, result);
}

static struct scatterlist* kvmalloc_to_sg(unsigned long addr, int len)
{
	struct page *pg = NULL;
	int nr_pages = 0;
	int i = 0;
	bool is_vmalloc = is_vmalloc_addr((void *)addr);
	struct scatterlist *sglist = NULL;

	nr_pages = (round_up(addr + len, PAGE_SIZE) -
		    round_down(addr, PAGE_SIZE)) >> PAGE_SHIFT;
	sglist = kvmalloc(nr_pages * sizeof(*sglist), GFP_KERNEL);
	if (!sglist)
		return NULL;

	sg_init_table(sglist, nr_pages);
	for (i = 0; i < nr_pages; ++i, addr += PAGE_SIZE) {
		if (is_vmalloc)
			pg = vmalloc_to_page((void *)addr);
		else
			pg = virt_to_page(addr);

		if (!pg) {
			kfree(sglist);
			return NULL;
		}

		sg_set_page(&sglist[i], pg, PAGE_SIZE, 0);
	}
	sglist[0].offset = offset_in_page(addr);

	if (nr_pages == 1) {
		sglist[0].length = len;
	} else {
		sglist[0].length = PAGE_SIZE - sglist[0].offset;
		sglist[nr_pages - 1].length = offset_in_page(addr + len);
		if (sglist[nr_pages - 1].length == 0)
			sglist[nr_pages - 1].length = PAGE_SIZE;
	}

	return sglist;
}

static void aeadcipher_ctx_free(struct aeadcipher_ctx *ctx)
{
	aead_request_free(ctx->req);
	kvfree(ctx->dst);
	kvfree(ctx->src);
	kfree(ctx);
}

static struct aeadcipher_ctx *
aeadcipher_ctx_init(struct crypto_aead *tfm, __u8 *src_buf, size_t src_len,
		    __u8 *dst_buf, size_t dst_len)
{
	struct aeadcipher_ctx *ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);

	if (!ctx)
		goto err;

	ctx->req = aead_request_alloc(tfm, GFP_KERNEL);
	if (!ctx->req)
		goto free_ctx;

	set_aeadcipher(tfm, ctx->req, &ctx->result);

	ctx->src = kvmalloc_to_sg((uintptr_t)src_buf, src_len);
	if (!ctx->src) {
		hmdfs_err("map src buf to sg failed");
		goto free_req;
	}

	ctx->dst = kvmalloc_to_sg((uintptr_t)dst_buf, dst_len);
	if (!ctx->dst) {
		hmdfs_err("map dst buf to sg failed");
		goto free_src;
	}

	aead_request_set_crypt(ctx->req, ctx->src, ctx->dst, src_len, ctx->iv);
	aead_request_set_ad(ctx->req, 0);
	return ctx;

free_src:
	kfree(ctx->src);
free_req:
	aead_request_free(ctx->req);
free_ctx:
	kfree(ctx);
err:
	return ERR_PTR(-ENOMEM);
}

int aeadcipher_encrypt_buffer(struct crypto_aead *tfm, __u8 *src_buf,
			      size_t src_len, __u8 *dst_buf, size_t dst_len)
{
	int ret = 0;
	struct aeadcipher_ctx *ctx = NULL;

	if (src_len <= 0)
		return -EINVAL;

	ctx = aeadcipher_ctx_init(tfm, src_buf, src_len,
				  dst_buf + HMDFS_IV_SIZE,
				  dst_len - HMDFS_IV_SIZE);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	get_random_bytes(ctx->iv, HMDFS_IV_SIZE);
	memcpy(dst_buf, ctx->iv, HMDFS_IV_SIZE);
	ret = aeadcipher_en_de(ctx->req, &ctx->result, ENCRYPT_FLAG);
	aeadcipher_ctx_free(ctx);

	return ret;
}

int aeadcipher_decrypt_buffer(struct crypto_aead *tfm, __u8 *src_buf,
			      size_t src_len, __u8 *dst_buf, size_t dst_len)
{
	int ret = 0;
	struct aeadcipher_ctx *ctx = NULL;

	if (src_len <= HMDFS_IV_SIZE + HMDFS_TAG_SIZE)
		return -EINVAL;

	ctx = aeadcipher_ctx_init(tfm, src_buf + HMDFS_IV_SIZE,
				  src_len - HMDFS_IV_SIZE, dst_buf, dst_len);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	memcpy(ctx->iv, src_buf, HMDFS_IV_SIZE);
	ret = aeadcipher_en_de(ctx->req, &ctx->result, DECRYPT_FLAG);
	aeadcipher_ctx_free(ctx);

	return ret;
}

static int set_tfm(__u8 *master_key, struct crypto_aead *tfm)
{
	int ret = 0;
	int iv_len;
	__u8 *sec_key = NULL;

	sec_key = master_key;
	crypto_aead_clear_flags(tfm, ~0);
	ret = crypto_aead_setkey(tfm, sec_key, HMDFS_KEY_SIZE);
	if (ret) {
		hmdfs_err("failed to set the key");
		goto out;
	}
	ret = crypto_aead_setauthsize(tfm, HMDFS_TAG_SIZE);
	if (ret) {
		hmdfs_err("authsize length is error");
		goto out;
	}

	iv_len = crypto_aead_ivsize(tfm);
	if (iv_len != HMDFS_IV_SIZE) {
		hmdfs_err("IV recommended value should be set %d", iv_len);
		ret = -ENODATA;
	}
out:
	return ret;
}

struct crypto_aead *hmdfs_alloc_aead(uint8_t *master_key, size_t len)
{
	int err = 0;
	struct crypto_aead *tfm = NULL;

	BUG_ON(len != HMDFS_KEY_SIZE);

	tfm = crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		hmdfs_err("failed to alloc gcm(aes): %ld", PTR_ERR(tfm));
		return NULL;
	}

	err = set_tfm(master_key, tfm);
	if (err) {
		hmdfs_err("failed to set key to tfm, exit fault: %d", err);
		goto out;
	}

	return tfm;

out:
	crypto_free_aead(tfm);
	return NULL;
}
