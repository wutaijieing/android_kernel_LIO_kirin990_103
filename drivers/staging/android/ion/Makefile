# SPDX-License-Identifier: GPL-2.0
ccflags-y +=	-I$(srctree)/drivers/platform_drivers/tzdriver
obj-$(CONFIG_ION) +=	ion.o ion-ioctl.o ion_heap.o
obj-$(CONFIG_ION_SYSTEM_HEAP) += heaps/
obj-$(CONFIG_ION_CARVEOUT_HEAP) += ion_carveout_heap.o
obj-$(CONFIG_ION_CHUNK_HEAP) += ion_chunk_heap.o
obj-$(CONFIG_ION_CMA_HEAP) += heaps/
obj-$(CONFIG_ION_MM_CMA_HEAP) += ion_mm_cma_heap.o
obj-$(CONFIG_ION_MM) += mm/
obj-$(CONFIG_ION_MM_SECSG) += ion_seccg_heap.o ion_sec_contig.o ion_secsg_heap.o
obj-$(CONFIG_ION_MM_DMA_POOL) += ion_dma_pool_heap.o
obj-$(CONFIG_ION_MM_CPA) += ion_cpa_heap.o
