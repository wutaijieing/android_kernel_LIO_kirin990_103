# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I$(srctree)/include/platform_include/basicplatform/linux/mas
ccflags-y += -I$(srctree)/block

obj-$(CONFIG_MAS_BLK) += mas_blk_core.o mas_blk_dump.o mas_blk_latency.o mas_blk_busy_idle_notify.o \
		mas_blk_flush.o mas_blk_iosched_ufs_mq.o mas_blk_iosched_interface.o \
		mas_blk_busy_idle_interface.o mas_blk_core_interface.o mas_blk_dump_interface.o \
		mas_blk_flush_interface.o mas_blk_latency_interface.o mas_blk_mq_tag.o mas_blk_tz.o \
		mas_blk_unistore_interface.o dsm_block.o
 obj-$(CONFIG_MAS_UNISTORE_PRESERVE) += mas_blk_unistore.o mas_blk_unistore_register.o mas_blk_unistore_sysfs.o
 obj-$(CONFIG_MAS_BKOPS) += mas_bkops_core.o mas_bkops_sysfs.o mas_bkops_core_interface.o mas_bkops_sysfs_interface.o
 obj-$(CONFIG_HP_CORE) += mas_blk_hyperhold.o
 obj-$(CONFIG_SCSI_UFS_INLINE_CRYPTO) += mas_blk_inline_crypt.o
