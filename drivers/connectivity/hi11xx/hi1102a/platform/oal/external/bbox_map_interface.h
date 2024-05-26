
#ifndef __BBOX_MAP_INTERFACE_H__
#define __BBOX_MAP_INTERFACE_H__

#ifdef CONFIG_ARCH_PLATFORM
extern void *dfx_bbox_map(phys_addr_t paddr, size_t size);
extern void dfx_bbox_unmap(const void *vaddr);
#define external_bbox_map dfx_bbox_map
#define external_bbox_unmap dfx_bbox_unmap
#else
extern void *hisi_bbox_map(phys_addr_t paddr, size_t size);
extern void hisi_bbox_unmap(const void *vaddr);
#define external_bbox_map hisi_bbox_map
#define external_bbox_unmap hisi_bbox_unmap
#endif /* end for CONFIG_ARCH_PLATFORM */

#endif /* end for __BBOX_MAP_INTERFACE_H__ */