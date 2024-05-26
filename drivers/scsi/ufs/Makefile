# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I$(srctree)/drivers/platform_drivers/tzdriver

# UFSHCD makefile

# The link order is important here. ufshcd-core must initialize
# before vendor drivers.
obj-$(CONFIG_SCSI_UFSHCD)		+= ufshcd-core.o
ufshcd-core-y				+= ufshcd.o ufs-sysfs.o
ufshcd-core-$(CONFIG_DEBUG_FS)		+= ufs-debugfs.o
ufshcd-core-$(CONFIG_SCSI_UFS_BSG)	+= ufs_bsg.o
ufshcd-core-$(CONFIG_SCSI_UFS_CRYPTO)	+= ufshcd-crypto.o
ufshcd-core-$(CONFIG_SCSI_UFS_HPB)	+= ufshpb.o

obj-$(CONFIG_SCSI_UFS_DWC_TC_PCI) += tc-dwc-g210-pci.o ufshcd-dwc.o tc-dwc-g210.o
obj-$(CONFIG_SCSI_UFS_DWC_TC_PLATFORM) += tc-dwc-g210-pltfrm.o ufshcd-dwc.o tc-dwc-g210.o
obj-$(CONFIG_SCSI_UFS_CDNS_PLATFORM) += cdns-pltfrm.o
obj-$(CONFIG_SCSI_UFS_QCOM) += ufs_qcom.o
ufs_qcom-y += ufs-qcom.o
ufs_qcom-$(CONFIG_SCSI_UFS_CRYPTO) += ufs-qcom-ice.o
obj-$(CONFIG_SCSI_UFS_EXYNOS) += ufs-exynos.o
obj-$(CONFIG_SCSI_UFSHCD_PCI) += ufshcd-pci.o
obj-$(CONFIG_SCSI_UFSHCD_PLATFORM) += ufshcd-pltfrm.o
obj-$(CONFIG_SCSI_UFS_HISI) += ufs-hisi.o
obj-$(CONFIG_SCSI_UFS_MEDIATEK) += ufs-mediatek.o
obj-$(CONFIG_SCSI_UFS_TI_J721E) += ti-j721e-ufs.o

obj-$(CONFIG_SCSI_HUFS) += hufs_plat.o
obj-$(CONFIG_SCSI_HUFS) += ufshcd_hufs_extend.o
obj-$(CONFIG_SCSI_HUFS) += hufs_mphy.o
obj-$(CONFIG_SCSI_HUFS) += ufs_mphy_firmware.o
obj-$(CONFIG_SCSI_UFS_TAURUS) += ufs-taurus.o
obj-$(CONFIG_SCSI_UFS_PISCES) += ufs-pisces.o
obj-$(CONFIG_SCSI_UFS_SCORPIO) += ufs_scorpio.o
obj-$(CONFIG_SCSI_UFS_LEO) += ufs-leo.o
obj-$(CONFIG_SCSI_UFS_SGR) += ufs-sgr.o
obj-$(CONFIG_SCSI_UFS_ANTLIA) += ufs_antlia.o
obj-$(CONFIG_SCSI_HUFS) += ufs-hpb.o
obj-$(CONFIG_HUFS_HC) += hufs_hcd.o
obj-$(CONFIG_SCSI_UFS_LIBRA) += ufs-libra.o
obj-$(CONFIG_HUFS_HC) += hufs-dump.o
obj-$(CONFIG_RPMB_UFS) += ufs_rpmb.o
obj-$(CONFIG_HUAWEI_UFS_VENDOR_MODE) += ufs_vendor_mode.o
obj-$(CONFIG_SCSI_UFS_FAULT_INJECT) += ufs-fault-inject.o
obj-$(CONFIG_DFX_DEBUG_FS) += ufs_debugfs.o
obj-$(CONFIG_HUAWEI_UFS_DSM) += dsm_ufs.o
obj-$(CONFIG_AB_PARTITION_TABLE) += hufs-lib.o
obj-$(CONFIG_HUFS_MANUAL_BKOPS) += hufs-bkops.o hufs_bkops_interface.o
obj-$(CONFIG_SCSI_UFS_HI1861_VCMD) += ufs_vendor_cmd.o
obj-$(CONFIG_SCSI_UFS_HI1861_VCMD) += ufs_vcmd_proc.o
obj-$(CONFIG_SCSI_UFS_HI1861_VCMD) += ufstt.o
obj-$(CONFIG_SCSI_UFS_UNISTORE) += ufs_unistore_core.o
obj-$(CONFIG_SCSI_UFS_UNISTORE) += ufs_unistore_read.o
obj-$(CONFIG_SCSI_UFS_UNISTORE) += ufs_unistore_write.o

ifeq ($(TARGET_VERSION_MODE), factory)
	obj-$(CONFIG_HUAWEI_UFS_TRACE) += ufs_trace.o
	EXTRA_CFLAGS += -DUFS_TRACE_FLAG
endif
