# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_FS_ENCRYPTION)	+= fscrypto.o

fscrypto-y := crypto.o \
	      fname.o \
	      hkdf.o \
	      hooks.o \
	      keyring.o \
	      keysetup.o \
	      keysetup_v1.o \
	      policy.o \
	      fscrypt_extra_ctrl.o
fscrypto-y += ecdh.o keyinfo_sdp.o policy_sdp.o adaptor_keyinfo_sdp.o
fscrypto-$(CONFIG_BLOCK) += bio.o
fscrypto-$(CONFIG_FS_ENCRYPTION_INLINE_CRYPT) += inline_crypt.o
fscrypto-$(CONFIG_FS_INLINE_ENCRYPT_BIG_DATA) += fbe3_trace.o
