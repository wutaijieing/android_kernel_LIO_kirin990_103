/*
 * fscrypt_common.h: common declarations for per-file encryption
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * Written by Michael Halcrow, 2015.
 * Modified by Jaegeuk Kim, 2015.
 */

#ifndef _LINUX_FSCRYPT_COMMON_H
#define _LINUX_FSCRYPT_COMMON_H

#include <linux/key.h>
#include <linux/fs.h>
#include <linux/fscrypt.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/dcache.h>
#include <linux/siphash.h>
#include <crypto/skcipher.h>
#include <crypto/aead.h>
#include <uapi/linux/fs.h>

#define FSCRYPT_FILE_NONCE_SIZE 16

#define FS_CRYPTO_BLOCK_SIZE 16

/* Encryption parameters */
#define FS_IV_SIZE 16
#define FS_KEY_DERIVATION_NONCE_SIZE 64
#define FS_KEY_DERIVATION_IV_SIZE 16
#define FS_KEY_DERIVATION_TAG_SIZE 16
#define FS_KEY_DERIVATION_CIPHER_SIZE (64 + 16) /* nonce + tag */
#define METADATA_BYTE_IN_KDF 16
#define FS_KEY_DESCRIPTOR_SIZE 8

/**
 * Encryption context for inode
 *
 * Protector format:
 *  1 byte: Protector format (2 = this version)
 *  1 byte: File contents encryption mode
 *  1 byte: File names encryption mode
 *  1 byte: Flags
 *  8 bytes: Master Key descriptor
 *  80 bytes: Encryption Key derivation nonce (encrypted)
 *  12 bytes: IV
 */
struct fscrypt_context_v1 {
	u8 version;
	u8 contents_encryption_mode;
	u8 filenames_encryption_mode;
	u8 flags;
	u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
	u8 nonce[FS_KEY_DERIVATION_CIPHER_SIZE];
	u8 iv[FS_KEY_DERIVATION_IV_SIZE];
} __packed;

struct fscrypt_prepared_key {
	struct crypto_skcipher *tfm;
#ifdef CONFIG_FS_ENCRYPTION_INLINE_CRYPT
	struct fscrypt_blk_crypto_key *blk_key;
#endif
};

union fscrypt_policy {
	u8 version;
	struct fscrypt_policy_v1 v1;
	struct fscrypt_policy_v2 v2;
};

/*
 * fscrypt_info - the "encryption key" for an inode
 *
 * When an encrypted file's key is made available, an instance of this struct is
 * allocated and stored in ->i_crypt_info.  Once created, it remains until the
 * inode is evicted.
 */
struct fscrypt_info {
	/* The key in a form prepared for actual encryption/decryption */
	struct fscrypt_prepared_key ci_enc_key;

	/* True if ci_enc_key should be freed when this fscrypt_info is freed */
	bool ci_owns_key;

#ifdef CONFIG_FS_ENCRYPTION_INLINE_CRYPT
	/*
	 * True if this inode will use inline encryption (blk-crypto) instead of
	 * the traditional filesystem-layer encryption.
	 */
	bool ci_inlinecrypt;
#endif

	/*
	 * Encryption mode used for this inode.  It corresponds to either the
	 * contents or filenames encryption mode, depending on the inode type.
	 */
	struct fscrypt_mode *ci_mode;

	/* Back-pointer to the inode */
	struct inode *ci_inode;

	/*
	 * The master key with which this inode was unlocked (decrypted).  This
	 * will be NULL if the master key was found in a process-subscribed
	 * keyring rather than in the filesystem-level keyring.
	 */
	struct key *ci_master_key;

	/*
	 * Link in list of inodes that were unlocked with the master key.
	 * Only used when ->ci_master_key is set.
	 */
	struct list_head ci_master_key_link;

	/*
	 * If non-NULL, then encryption is done using the master key directly
	 * and ci_enc_key will equal ci_direct_key->dk_key.
	 */
	struct fscrypt_direct_key *ci_direct_key;

	/*
	 * This inode's hash key for filenames.  This is a 128-bit SipHash-2-4
	 * key.  This is only set for directories that use a keyed dirhash over
	 * the plaintext filenames -- currently just casefolded directories.
	 */
	siphash_key_t ci_dirhash_key;
	bool ci_dirhash_key_initialized;

	/* The encryption policy used by this inode */
	union fscrypt_policy ci_policy;

	/* This inode's nonce, copied from the fscrypt_context */
	u8 ci_nonce[FS_KEY_DERIVATION_CIPHER_SIZE];
	u8 ci_iv[FS_KEY_DERIVATION_IV_SIZE];

	/* Hashed inode number.  Only set for IV_INO_LBLK_32 */
	u32 ci_hashed_ino;
	u8 ci_flags;
	struct crypto_aead *ci_gtfm;
	struct crypto_cipher *ci_essiv_tfm;
	void *ci_key;
	int ci_key_len;
	int ci_key_index;
	u8 ci_metadata[METADATA_BYTE_IN_KDF];
	u8 ci_hw_enc_flag;
};

#ifdef CONFIG_HWDPS
#define HWDPS_XATTR_NAME "hwdps"
#define HWDPS_XATTR_ENABLE_FLAG_NEW 0x0020
#define HWDPS_XATTR_ENABLE_FLAG 0x0010
#endif

static inline void *fscrypt_ci_key(struct inode *inode)
{
#if IS_ENABLED(CONFIG_F2FS_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_key;
#else
	return NULL;
#endif
}

static inline int fscrypt_ci_key_len(struct inode *inode)
{
#if IS_ENABLED(CONFIG_F2FS_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_key_len;
#else
	return 0;
#endif
}

static inline int fscrypt_ci_key_index(struct inode *inode)
{
#if IS_ENABLED(CONFIG_F2FS_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_key_index;
#else
	return -1;
#endif
}

static inline u8 *fscrypt_ci_metadata(struct inode *inode)
{
#if IS_ENABLED(CONFIG_F2FS_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_metadata;
#else
	return NULL;
#endif
}

struct fscrypt_ctx {
	union {
		struct {
			struct page *bounce_page; /* Ciphertext page */
			struct page *control_page; /* Original page  */
		} w;
		struct {
			struct bio *bio;
			struct work_struct work;
		} r;
		struct list_head free_list; /* Free list */
	};
	u8 flags; /* Flags */
};

#define FSTR_INIT(n, l)                                                        \
	{                                                                      \
		.name = n, .len = l                                            \
	}
#define FSTR_TO_QSTR(f) QSTR_INIT((f)->name, (f)->len)
#define fname_name(p) ((p)->disk_name.name)
#define fname_len(p) ((p)->disk_name.len)

/*
 * fscrypt superblock flags
 */
#define FS_CFLG_OWN_PAGES (1U << 1)

#ifndef CONFIG_SDP_ENCRYPTION
#define CONFIG_SDP_ENCRYPTION 1
#endif

#define FS_ENCRYPTION_CONTEXT_FORMAT_V2 2

#define CI_KEY_LEN_NEW 48
#define FILE_ENCRY_TYPE_BEGIN_BIT 8
#define FILE_ENCRY_TYPE_MASK 0xFF
#define KEY_SLOT_OFFSET 63

enum encrypto_type {
	PLAIN = 0,
	CD,
	ECE,
	SECE,
};
extern int hwdps_get_context(struct inode *inode);
extern int hwdps_update_context(struct inode *inode, uid_t new_uid);

extern int fscrypt_set_gcm_key(struct crypto_aead *tfm, const u8 *);
extern int fscrypt_derive_gcm_key(struct crypto_aead *, const u8 *, u8 *, u8 *,
				  int);
extern struct key *fscrypt_request_key(const u8 *descriptor, const u8 *prefix,
				       int prefix_size);
extern int fscrypt_generate_metadata_nonce(u8 *nonce, struct inode *inode,
					   size_t len);
extern struct crypto_skcipher *fscrypt_allocate_skcipher(
		struct fscrypt_mode *mode, const u8 *raw_key,
		const struct inode *inode);

extern int fscrypt_get_metadata(struct inode *inode,
				struct fscrypt_info *ci_info);
extern int fscrypt_new_ece_metadata(struct inode *inode,
				    struct fscrypt_info *ci_info,
				    void *fs_data);
extern int fscrypt_new_sece_metadata(struct inode *inode,
				     struct fscrypt_info *ci_info,
				     void *fs_data);
extern int fscrypt_get_ece_metadata(struct inode *inode,
				    struct fscrypt_info *ci_info, void *fs_data,
				    bool create);
extern int fscrypt_get_sece_metadata(struct inode *inode,
				     struct fscrypt_info *ci_info,
				     void *fs_data, bool create);
extern int fscrypt_retrieve_sece_metadata(struct inode *inode,
					  struct fscrypt_info *ci_info,
					  void *fs_data);
extern int fscrypt_vm_op_check(struct inode *inode);
extern int fscrypt_lld_protect(const struct request *request);

extern int rw_begin(struct file *file);
extern void rw_finish(int read_write, struct file *file);
extern void fbe3_lock_in(u32 user);
extern u32 fbe3_unlock_in(u32 user, u32 file, u8 *iv, u32 iv_len);

typedef int (*metadata_cb)(struct inode *inode, struct fscrypt_info *ci_info,
			   void *fs_data);
typedef struct file_encry_meta_list {
	enum encrypto_type file_type;
	metadata_cb file_encry_meta_cb_func;
} fbe_new_meta_cb_func_list;

#endif /* _LINUX_FSCRYPT_COMMON_H */
