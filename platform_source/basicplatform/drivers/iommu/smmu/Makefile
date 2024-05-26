LINUXINCLUDE += -I$(srctree)/drivers/iommu
LINUXINCLUDE += -I$(srctree)/drivers/staging/android/ion

obj-$(CONFIG_MM_IOMMU_LEGACY) += mm_smmu.o
ifeq ($(CONFIG_MM_IODOMAIN_API),y)
ifeq ($(CONFIG_MM_IOMMU_BYPASS),y)
obj-y += mm_iommu_map_bypass.o
else
obj-y += mm_iommu_map.o
endif
endif
obj-$(CONFIG_MM_IOMMU_LPAE) += mm_smmu_lpae.o
obj-$(CONFIG_MM_IOMMU_TEST) += smmu_test.o vsmmu_test.o
obj-$(CONFIG_MM_SVM) += mm_svm.o
obj-$(CONFIG_MM_SMMU_V3) += platform_smmu_v3.o mm_smmuv3.o
obj-$(CONFIG_ARM_SMMU_V3) += mm_smmuv3.o
obj-$(CONFIG_MM_IOMMU_DMA) += mm_iommu_dma.o
obj-$(CONFIG_MM_VIRTIO_IOMMU) += mm_virtio_iommu.o
