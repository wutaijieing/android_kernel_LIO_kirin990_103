/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the kcode.c for kernel code integrity checking
 * Create: 2016-06-18
 */

#include "./include/kcode.h"

#ifdef CONFIG_HW_ROOT_SCAN_ENG_DEBUG
#define MAX_CODE_SIZE 50000000
#else
#define MAX_CODE_SIZE 30000000
#endif

#define RSCAN_LOG_TAG "kcode"

static const struct memrange {
	char *start;
	char *end;
} ranges[] = {
	{_stext, _etext},
	{NULL, NULL}
};

static size_t g_memrange_size;

/* Do sanity check on _stext, __v7_setup_stack, _etext.
 * The size of android-msm-hammerhead-3.4-lollipop-release is 14583504.
 * We think it's good as long as the size is less than 30M.
 */
static int kcode_verify_ranges(void)
{
	const struct memrange *range;
	char *ptr = NULL;

	g_memrange_size = 0;
	for (range = ranges; range->start != NULL; range++) {
		if (range->end <= range->start) {
			rs_log_error(RSCAN_LOG_TAG, "range end error, too small");
			return 1;
		}

		/* kernel code is aligned with 4 bytes */
		if (((unsigned int)((range->end - range->start) - 1) > MAX_CODE_SIZE) ||
					((uintptr_t)range->start % 4) ||
					((uintptr_t)range->end % 4)) {
			rs_log_error(RSCAN_LOG_TAG, "range aligned error, must be 4-byte aligned");
			return 1;
		}

		if ((ptr != NULL) && (range->start <= ptr)) {
			rs_log_error(RSCAN_LOG_TAG,
				"range order error, the following start must be greater than the preceding end");
			return 1;
		}
		ptr = range->end;
		g_memrange_size += (unsigned int)(range->end - range->start);
	}

	if (g_memrange_size > MAX_CODE_SIZE) {
		rs_log_error(RSCAN_LOG_TAG, "range error 4, g_memrange_size=%zu",
			g_memrange_size);
		return 1;
	}
	return 0;
}

int kcode_scan(uint8_t *hash, size_t hash_len)
{
	int i;
	int err;
	struct crypto_shash *tfm = crypto_alloc_shash("sha256", 0, 0);
	bool xom_enable = get_xom_enable();
	SHASH_DESC_ON_STACK(shash, tfm);

	if (IS_ERR(tfm)) {
		if (xom_enable == true) {
			rs_log_debug(RSCAN_LOG_TAG, "kcode can not read when xom enable");
			return 0;
		} else {
			rs_log_error(RSCAN_LOG_TAG, "crypto_alloc_hash(sha256) error %ld",
				PTR_ERR(tfm));
			return -ENOMEM;
		}
	}

	if (xom_enable == true) {
		if (memset_s(hash, hash_len, 0, hash_len) != EOK) {
			rs_log_error(RSCAN_LOG_TAG, "memset_s fail\n");
			return -ENOMEM;
		}
		crypto_free_shash(tfm);
		return 0;
	}

	if (g_memrange_size == 0) {
		if (kcode_verify_ranges()) {
			crypto_free_shash(tfm);
			return -ENOMEM;
		}
	}

	shash->tfm = tfm;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 1, 0)
	shash->flags = 0;
#endif

	err = crypto_shash_init(shash);
	if (err < 0) {
		rs_log_error(RSCAN_LOG_TAG, "crypto_shash_init() error %d", err);
		crypto_free_shash(tfm);
		return err;
	}

	for (i = 0; ranges[i].start != NULL; i++)
		crypto_shash_update(shash, (char *)ranges[i].start,
			(unsigned int)(ranges[i].end - ranges[i].start));

	err = crypto_shash_final(shash, (u8 *)hash);
	rs_log_debug(RSCAN_LOG_TAG, "kscan result %d", err);
	crypto_free_shash(tfm);
	return err;
}

size_t kcode_get_size(void)
{
	return (size_t)(_etext - _stext);
}

int kcode_syscall_scan(uint8_t *hash, size_t hash_len)
{
	size_t size;
	void *ptr = (void *)sys_call_table;
	int err;
	struct crypto_shash *tfm = crypto_alloc_shash("sha256", 0, 0);

	SHASH_DESC_ON_STACK(shash, tfm);
	var_not_used(hash_len);

	if (IS_ERR(tfm)) {
		rs_log_error(RSCAN_LOG_TAG, "crypto_alloc_hash(sha256) error %ld",
				PTR_ERR(tfm));
		return -ENOMEM;
	}

	shash->tfm = tfm;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 1, 0)
	shash->flags = 0;
#endif

	err = crypto_shash_init(shash);
	if (err < 0) {
		rs_log_error(RSCAN_LOG_TAG, "crypto_shash_init error %d", err);
		crypto_free_shash(tfm);
		return err;
	}

	/* define NR_syscalls as 326 */
	size = NR_syscalls * sizeof(void *);

	crypto_shash_update(shash, (char *)ptr, (unsigned int)size);
	err = crypto_shash_final(shash, (u8 *)hash);
	rs_log_debug(RSCAN_LOG_TAG, "syscallscan result %d", err);

	crypto_free_shash(tfm);
	return err;
}

