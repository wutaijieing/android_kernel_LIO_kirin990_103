# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_HW_F2FS_FS) += f2fs.o
ifdef CONFIG_IOCACHE
subdir-ccflags-y += -I$(srctree)/drivers/md/iocache
endif

f2fs-y		:= dir.o file.o inode.o namei.o hash.o super.o inline.o
f2fs-y		+= checkpoint.o gc.o data.o node.o segment.o recovery.o
f2fs-y		+= shrinker.o extent_cache.o sysfs.o
f2fs-y		+= trace.o
f2fs-$(CONFIG_HP_FILE) += hp_preallocate.o
f2fs-$(CONFIG_FS_VERITY) += verity.o
ifneq ($(CONFIG_F2FS_FS_COMPRESSION)$(CONFIG_F2FS_FS_COMPRESSION_EX),)
f2fs-y += compress.o
endif
f2fs-y += f2fs_dump_info.o
f2fs-$(CONFIG_F2FS_STAT_FS) += debug.o
f2fs-$(CONFIG_F2FS_FS_XATTR) += xattr.o
f2fs-$(CONFIG_F2FS_FS_POSIX_ACL) += acl.o
f2fs-$(CONFIG_F2FS_TURBO_ZONE) += turbo_zone.o
f2fs-$(CONFIG_HWDPS) += hwdps_context.o


ifdef CONFIG_HW_F2FS_CHIP_KIRIN
f2fs-y += sdp_context.o
f2fs-y += sdp_metadata.o
endif

ifdef CONFIG_HW_F2FS_CHIP_QCOM
f2fs-$(CONFIG_SDP_ENCRYPTION) += sdp_context.o
endif




