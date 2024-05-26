
#include "pcie_host.h"
#include "pcie_comm.h"

void oal_pcie_share_mem_res_unmap(oal_pcie_linux_res *pcie_res)
{
    memset_s((void *)&pcie_res->ep_res.dev_share_mem, sizeof(pci_addr_map), 0, sizeof(pci_addr_map));
}

/* 调用必须在iATU配置, pcie device 使能之后， */
int32_t oal_pcie_share_mem_res_map(oal_pcie_linux_res *pcie_res)
{
    int32_t ret;
    void *pst_share_mem_vaddr = NULL;
    uint32_t share_mem_address = 0xFFFFFFFF; /* Device cpu地址 */
    pci_addr_map addr_map, share_mem_map;
    unsigned long timeout, timeout1;
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;

    /* 忙等50ms 若超时 再等10S 超时 */
    timeout = jiffies + msecs_to_jiffies(hi110x_get_emu_timeout(50));     /* 50ms each timeout */
    timeout1 = jiffies + msecs_to_jiffies(hi110x_get_emu_timeout(10000)); /* 10000ms total timeout */

    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->chip_info.addr_info.sharemem_addr, &addr_map);
    if (ret != OAL_SUCC) {
        /* share mem 地址未映射! */
        pci_print_log(PCI_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n",
                      ep_res->chip_info.addr_info.sharemem_addr);
        return ret;
    }

    pst_share_mem_vaddr = (void *)addr_map.va;
    pci_print_log(PCI_LOG_DBG, "device address:0x%x = va:0x%lx",
                  ep_res->chip_info.addr_info.sharemem_addr, addr_map.va);

    forever_loop() {
        /* Get sharemem's dev_cpu address */
        oal_pcie_io_trans((uintptr_t)&share_mem_address, (uintptr_t)pst_share_mem_vaddr, sizeof(share_mem_address));

        /* 通过检查地址转换可以判断读出的sharemem地址是否是有效值 */
        ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, share_mem_address, &share_mem_map);
        if (ret != OAL_SUCC) {
            return ret;
        }
        /* Device 初始化完成  & PCIE 通信正常 */
        if (share_mem_address != 0) {
            if (share_mem_address != 0xFFFFFFFF) {
                pci_print_log(PCI_LOG_INFO, "share_mem_address 0x%x", share_mem_address);
                ret = OAL_SUCC;
                break;
            } else {
                pci_print_log(PCI_LOG_ERR, "invalid sharemem address 0x%x, maybe linkdown", share_mem_address);
                return -OAL_ENODEV;
            }
        }

        if (!time_after(jiffies, timeout)) {
            cpu_relax();
            continue; /* 未超时，继续 */
        }

        /* 50ms 超时, 开始10S超时探测 */
        if (!time_after(jiffies, timeout1)) {
            oal_msleep(1);
            continue; /* 未超时，继续 */
        } else {
            /* 10s+50ms 超时，退出 */
            pci_print_log(PCI_LOG_ERR, "share_mem_address 0x%x, jiffies:0x%lx, timeout:0x%lx, timeout1:0x%lx",
                          share_mem_address, jiffies, timeout, timeout1);
            ret = -OAL_ETIMEDOUT;
            break;
        }
    }

    if (share_mem_map.va != 0 && (ret == OAL_SUCC)) {
        ep_res->dev_share_mem.va = share_mem_map.va;
        ep_res->dev_share_mem.pa = share_mem_map.pa;
        pci_print_log(PCI_LOG_DBG, "share mem va:0x%lx, pa:0x%lx",
                      ep_res->dev_share_mem.va, ep_res->dev_share_mem.pa);
        return OAL_SUCC;
    }

    /* 此处失败可能的原因(若host第一次加载就失败可能是原因1, 否者可能是原因2):
     * 1.下载完后boot跳转到firmware main函数失败(firmware镜像问题:ram代码跑飞比如切高频失败等)
     * 2.下载之前的EN gpio复位操作失效，重复下载了firmware */
    pci_print_log(PCI_LOG_INFO,
                  "pcie boot timeout, jump firmware failed or subsys reset failed, 0x%x",
                  share_mem_address);

    (void)ssi_dump_err_regs(SSI_ERR_PCIE_WAIT_BOOT_TIMEOUT);

    return -OAL_EFAIL;
}

OAL_STATIC int32_t oal_pcie_device_shared_addr_res_map(oal_pcie_ep_res *ep_res,
                                                       pcie_share_mem_stru *pst_share_mem)
{
    int32_t ret;
    int32_t i;
    pci_addr_map st_map;
    oal_pcie_linux_res *pcie_res = oal_list_get_entry(ep_res, oal_pcie_linux_res, ep_res);

    for (i = 0; i < PCIE_SHARED_ADDR_BUTT; i++) {
        if (pst_share_mem->device_addr[i] == 0) {
            pci_print_log(PCI_LOG_DBG, "type:%d , device addr is zero", i);
            continue;
        }

        ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, pst_share_mem->device_addr[i], &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "convert device addr type:%d, addr:0x%x failed, ret=%d",
                          i, pst_share_mem->device_addr[i], ret);
            return -OAL_ENOMEM;
        }

        memcpy_s(&ep_res->st_device_shared_addr_map[i], sizeof(pci_addr_map), &st_map, sizeof(pci_addr_map));
    }

    return OAL_SUCC;
}

OAL_STATIC void oal_pcie_device_shared_addr_res_unmap(oal_pcie_ep_res *ep_res)
{
    memset_s(&ep_res->st_device_shared_addr_map, sizeof(ep_res->st_device_shared_addr_map),
        0, sizeof(ep_res->st_device_shared_addr_map));
}

OAL_STATIC void oal_pcie_comm_ringbuf_res_unmap(oal_pcie_ep_res *ep_res)
{
    int32_t i;
    for (i = 0; i < PCIE_COMM_RINGBUF_BUTT; i++) {
        memset_s(&ep_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr,
                 sizeof(ep_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr),
                 0, sizeof(ep_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr));
        memset_s(&ep_res->st_ringbuf_res.comm_rb_res[i].data_daddr,
                 sizeof(ep_res->st_ringbuf_res.comm_rb_res[i].data_daddr),
                 0, sizeof(ep_res->st_ringbuf_res.comm_rb_res[i].data_daddr));
    }
}

OAL_STATIC int32_t oal_pcie_comm_ringbuf_res_map(oal_pcie_ep_res *ep_res)
{
    int32_t i;
    int32_t ret;
    pci_addr_map st_map; /* DEVICE CPU地址 */
    oal_pcie_linux_res *pcie_res = oal_list_get_entry(ep_res, oal_pcie_linux_res, ep_res);

    for (i = 0; i < PCIE_COMM_RINGBUF_BUTT; i++) {
        if (ep_res->st_ringbuf.st_ringbuf[i].base_addr == 0) {
            /* ringbuf invalid */
            continue;
        }

        /* get ringbuf base_addr */
        ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->st_ringbuf.st_ringbuf[i].base_addr, &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "invalid comm ringbuf base address 0x%llx, rb id:%d map failed, ret=%d\n",
                          ep_res->st_ringbuf.st_ringbuf[i].base_addr, i, ret);
            return -OAL_ENOMEM;
        }

        pci_print_log(PCI_LOG_DBG, "comm ringbuf %d base address is 0x%llx",
                      i, ep_res->st_ringbuf.st_ringbuf[i].base_addr);

        /* comm ringbuf data 所在DMA地址 */
        memcpy_s((void *)&ep_res->st_ringbuf_res.comm_rb_res[i].data_daddr,
                 sizeof(pci_addr_map), (void *)&st_map, sizeof(pci_addr_map));

        /* comm ringbuf ctrl address */
        ep_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr.va =
            ep_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res, st_ringbuf[i]);
        ep_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr.pa =
            ep_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res, st_ringbuf[i]);
    }

    return OAL_SUCC;
}

void oal_pcie_ringbuf_res_unmap(oal_pcie_linux_res *pcie_res)
{
    int32_t i;
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;

    memset_s(&ep_res->st_ringbuf_map, sizeof(ep_res->st_ringbuf_map),
        0, sizeof(ep_res->st_ringbuf_map));
    memset_s(&ep_res->st_ringbuf, sizeof(ep_res->st_ringbuf),
        0, sizeof(ep_res->st_ringbuf));
    memset_s(&ep_res->st_rx_res.ringbuf_data_dma_addr,
        sizeof(ep_res->st_rx_res.ringbuf_data_dma_addr),
        0, sizeof(ep_res->st_rx_res.ringbuf_data_dma_addr));
    memset_s(&ep_res->st_rx_res.ringbuf_ctrl_dma_addr,
        sizeof(ep_res->st_rx_res.ringbuf_ctrl_dma_addr),
        0, sizeof(ep_res->st_rx_res.ringbuf_ctrl_dma_addr));
    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        memset_s(&ep_res->st_tx_res[i].ringbuf_data_dma_addr, sizeof(ep_res->st_tx_res[i].ringbuf_data_dma_addr),
            0, sizeof(ep_res->st_tx_res[i].ringbuf_data_dma_addr));
        memset_s(&ep_res->st_tx_res[i].ringbuf_ctrl_dma_addr, sizeof(ep_res->st_tx_res[i].ringbuf_ctrl_dma_addr),
            0, sizeof(ep_res->st_tx_res[i].ringbuf_ctrl_dma_addr));
    }

    memset_s(&ep_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr,
        sizeof(ep_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr),
        0, sizeof(ep_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr));
    memset_s(&ep_res->st_message_res.d2h_res.ringbuf_data_dma_addr,
        sizeof(ep_res->st_message_res.d2h_res.ringbuf_data_dma_addr),
        0, sizeof(ep_res->st_message_res.d2h_res.ringbuf_data_dma_addr));

    memset_s(&ep_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr,
        sizeof(ep_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr),
        0, sizeof(ep_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr));
    memset_s(&ep_res->st_message_res.h2d_res.ringbuf_data_dma_addr,
        sizeof(ep_res->st_message_res.h2d_res.ringbuf_data_dma_addr),
        0, sizeof(ep_res->st_message_res.h2d_res.ringbuf_data_dma_addr));

    memset_s(&ep_res->st_device_stat, sizeof(ep_res->st_device_stat),
        0, sizeof(ep_res->st_device_stat));

    oal_pcie_device_shared_addr_res_unmap(ep_res);

    oal_pcie_comm_ringbuf_res_unmap(ep_res);
}

/* 初始化Host ringbuf 和 Device ringbuf 的映射 */
int32_t oal_pcie_ringbuf_res_map(oal_pcie_linux_res *pcie_res)
{
    int32_t ret;
    int32_t i;
    uint8_t reg = 0;
    oal_pci_dev_stru *pst_pci_dev;
    pci_addr_map st_map; /* DEVICE CPU地址 */
    pcie_share_mem_stru st_share_mem;
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;

    pst_pci_dev = pcie_res->comm_res->pcie_dev;

    oal_pci_read_config_byte(pst_pci_dev, PCI_CACHE_LINE_SIZE, &reg);
    pci_print_log(PCI_LOG_INFO, "L1_CACHE_BYTES: %d\n", reg);
    pci_print_log(PCI_LOG_INFO, "ep_res->dev_share_mem.va:%lx",
        ep_res->dev_share_mem.va);
    oal_pcie_io_trans((uintptr_t)&st_share_mem, ep_res->dev_share_mem.va,
        sizeof(pcie_share_mem_stru));
    oal_print_hex_dump((uint8_t *)ep_res->dev_share_mem.va, sizeof(pcie_share_mem_stru),
        HEX_DUMP_GROUP_SIZE, "st_share_mem: ");
    pci_print_log(PCI_LOG_INFO, "st_share_mem.ringbuf_res_paddr :0x%x\n", st_share_mem.ringbuf_res_paddr);

    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, st_share_mem.ringbuf_res_paddr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid ringbuf device address 0x%x, map failed\n",
            st_share_mem.ringbuf_res_paddr);
        oal_print_hex_dump((uint8_t *)&st_share_mem, sizeof(st_share_mem),
            HEX_DUMP_GROUP_SIZE, "st_share_mem: ");
        return -OAL_ENOMEM;
    }

    /* h->h */
    memcpy_s(&ep_res->st_ringbuf_map, sizeof(pci_addr_map), &st_map, sizeof(pci_addr_map));

    /* device的ringbuf管理结构同步到Host */
    oal_pcie_io_trans((uintptr_t)&ep_res->st_ringbuf, ep_res->st_ringbuf_map.va,
        sizeof(ep_res->st_ringbuf));

    /* 初始化ringbuf 管理结构体的映射 */
    ep_res->st_rx_res.ringbuf_ctrl_dma_addr.pa = ep_res->st_ringbuf_map.pa +
                                                      OAL_OFFSET_OF(pcie_ringbuf_res, st_d2h_buf);
    ep_res->st_rx_res.ringbuf_ctrl_dma_addr.va = ep_res->st_ringbuf_map.va +
                                                      OAL_OFFSET_OF(pcie_ringbuf_res, st_d2h_buf);

    /* 初始化TX BUFF, 不考虑大小端，host/dev 都是小端，否者这里的base_addr需要转换 */
    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->st_ringbuf.st_d2h_buf.base_addr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid h2d ringbuf base address 0x%llx, map failed\n",
            ep_res->st_ringbuf.st_d2h_buf.base_addr);
        return -OAL_ENOMEM;
    }

    memcpy_s((void *)&ep_res->st_rx_res.ringbuf_data_dma_addr, sizeof(pci_addr_map),
        (void *)&st_map, sizeof(pci_addr_map));

    /* 初始化RX BUFF */
    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        uintptr_t offset;
        ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->st_ringbuf.st_h2d_buf[i].base_addr, &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "invalid d2h ringbuf[%d] base address 0x%llx, map failed, ret=%d\n",
                          i, ep_res->st_ringbuf.st_h2d_buf[i].base_addr, ret);
            return -OAL_ENOMEM;
        }
        memcpy_s(&ep_res->st_tx_res[i].ringbuf_data_dma_addr,
                 sizeof(pci_addr_map), &st_map, sizeof(pci_addr_map));
        offset = (uintptr_t)(&ep_res->st_ringbuf.st_h2d_buf[i]) - (uintptr_t)(&ep_res->st_ringbuf);
        ep_res->st_tx_res[i].ringbuf_ctrl_dma_addr.pa = ep_res->st_ringbuf_map.pa + offset;
        ep_res->st_tx_res[i].ringbuf_ctrl_dma_addr.va = ep_res->st_ringbuf_map.va + offset;
    }

    /* 初始化消息TX RINGBUFF */
    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->st_ringbuf.st_h2d_msg.base_addr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid h2d message ringbuf base address 0x%llx, map failed, ret=%d\n",
                      ep_res->st_ringbuf.st_h2d_msg.base_addr, ret);
        return -OAL_ENOMEM;
    }

    /* h2d message data 所在DMA地址 */
    memcpy_s((void *)&ep_res->st_message_res.h2d_res.ringbuf_data_dma_addr,
             sizeof(pci_addr_map), (void *)&st_map, sizeof(pci_addr_map));

    /* h2d message ctrl 结构体 所在DMA地址 */
    ep_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.va =
        ep_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res, st_h2d_msg);
    ep_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.pa =
        ep_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res, st_h2d_msg);

    /* 初始化消息RX RINGBUFF */
    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->st_ringbuf.st_d2h_msg.base_addr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid d2h message ringbuf base address 0x%llx, map failed, ret=%d\n",
                      ep_res->st_ringbuf.st_d2h_msg.base_addr, ret);
        return -OAL_ENOMEM;
    }

    /* d2h message data 所在DMA地址 */
    memcpy_s((void *)&ep_res->st_message_res.d2h_res.ringbuf_data_dma_addr,
             sizeof(pci_addr_map), (void *)&st_map, sizeof(pci_addr_map));

    /* d2h message ctrl 结构体 所在DMA地址 */
    ep_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.va =
        ep_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res, st_d2h_msg);
    ep_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.pa =
        ep_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res, st_d2h_msg);

#ifdef _PRE_PLAT_FEATURE_PCIE_DEVICE_STAT
    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, st_share_mem.device_stat_paddr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid device_stat_paddr  0x%x, map failed\n", st_share_mem.device_stat_paddr);
        return -OAL_ENOMEM;
    }

    /* h->h */
    memcpy_s(&ep_res->st_device_stat_map, sizeof(pci_addr_map), &st_map, sizeof(pci_addr_map));
    oal_pcie_io_trans((uintptr_t)&ep_res->st_device_stat,
        ep_res->st_device_stat_map.va, sizeof(ep_res->st_device_stat));
#endif

    ret = oal_pcie_comm_ringbuf_res_map(ep_res);
    if (ret != OAL_SUCC) {
        return ret;
    }

    ret = oal_pcie_device_shared_addr_res_map(ep_res, &st_share_mem);
    if (ret != OAL_SUCC) {
        return ret;
    }

    pci_print_log(PCI_LOG_INFO, "oal_pcie_ringbuf_res_map succ");
    return OAL_SUCC;
}


/* PCIE EP Master 口访问,主芯片CPU访问片内地址空间(注意不能访问片内PCIE Slave口空间) for WiFi TAE
 *    将devcpu看到的地址转换成host侧看到的物理地址和虚拟地址 */
int32_t oal_pcie_devca_to_hostva(uint32_t ul_chip_id, uint64_t dev_cpuaddr, uint64_t *host_va)
{
    return hcc_bus_master_address_switch(oal_pcie_get_bus(ul_chip_id), dev_cpuaddr, host_va, MASTER_WCPUADDR_TO_VA);
}
oal_module_symbol(oal_pcie_devca_to_hostva);

/* 虚拟地址转换成WCPU看到的地址 */
int32_t oal_pcie_get_dev_ca(uint32_t chip_id, void *host_va, uint64_t* dev_cpuaddr)
{
    return hcc_bus_master_address_switch(oal_pcie_get_bus(chip_id), (uint64_t)(uintptr_t)host_va, dev_cpuaddr,
        MASTER_VA_TO_WCPUADDR);
}
oal_module_symbol(oal_pcie_get_dev_ca);

/* PCIE EP Slave 口看到的地址转换 --片内外设通过PCIE访问主芯片DDR空间 */
/* 地址转换主芯片的DDR设备地址转换成device的SLAVE口地址
 *  注意这里的DDR设备地址(iova)不是的直接物理地址，而是对应Kernel的DMA地址
 *  设备地址不能通过phys_to_virt/virt_to_phys直接转换
 */
int32_t pcie_if_hostca_to_devva(uint32_t chip_id, uint64_t host_iova, uint64_t *addr)
{
    return hcc_bus_slave_address_switch(oal_pcie_get_bus(chip_id), host_iova, addr, SLAVE_IOVA_TO_PCI_SLV);
}
EXPORT_SYMBOL_GPL(pcie_if_hostca_to_devva);

/* 地址转换主芯片的DDR设备地址转换成device的SLAVE口地址 */
int32_t pcie_if_devva_to_hostca(uint32_t chip_id, uint64_t devva, uint64_t *host_iova)
{
    return hcc_bus_slave_address_switch(oal_pcie_get_bus(chip_id), devva, host_iova, SLAVE_PCI_SLV_TO_IOVA);
}
EXPORT_SYMBOL_GPL(pcie_if_devva_to_hostca);

int32_t oal_pcie_get_ca_by_pa(oal_pcie_comm_res *comm_res, uintptr_t paddr, uint64_t *cpuaddr)
{
    int32_t index;
    int32_t region_num;
    uint64_t offset;
    oal_pcie_region *region_base;
    uintptr_t end;

    region_num = comm_res->regions.region_nums;
    region_base = comm_res->regions.pst_regions;

    if (cpuaddr == NULL) {
        return -OAL_EINVAL;
    }

    *cpuaddr = 0;

    if (oal_warn_on(!comm_res->regions.inited)) {
        return -OAL_ENODEV;
    }

    for (index = 0; index < region_num; index++, region_base++) {
        if (region_base->vaddr == NULL) {
            continue;
        }

        end = (uintptr_t)region_base->paddr + region_base->size - 1;

        if ((paddr >= (uintptr_t)region_base->paddr) && (paddr <= end)) {
            /* 地址在范围内 */
            offset = paddr - (uintptr_t)region_base->paddr;
            *cpuaddr = region_base->cpu_start + offset;
            return OAL_SUCC;
        } else {
            continue;
        }
    }

    return -OAL_ENOMEM;
}

/*
 * 将Device Cpu看到的地址转换为 Host侧的虚拟地址,
 * 虚拟地址返回NULL为无效地址，Device Cpu地址有可能为0,
 * local ip inbound cpu address to host virtual address,
 * 函数返回非0为失败
 */
int32_t oal_pcie_inbound_ca_to_va(oal_pcie_comm_res *comm_res, uint64_t dev_cpuaddr, pci_addr_map *addr_map)
{
    int32_t index;
    int32_t region_num;
    uint64_t offset;
    oal_pcie_region *region_base;

    region_num = comm_res->regions.region_nums;
    region_base = comm_res->regions.pst_regions;

    if (addr_map != NULL) {
        addr_map->pa = 0;
        addr_map->va = 0;
    }

    if (oal_unlikely(comm_res->link_state < PCI_WLAN_LINK_MEM_UP)) {
        pci_print_log(PCI_LOG_WARN, "addr request 0x%llx failed, link_state:%s",
                      dev_cpuaddr, oal_pcie_get_link_state_str(comm_res->link_state));
        return -OAL_EBUSY;
    }

    if (oal_warn_on(!comm_res->regions.inited)) {
        return -OAL_ENODEV;
    }

    for (index = 0; index < region_num; index++, region_base++) {
        if (region_base->vaddr == NULL) {
            continue;
        }

        if ((dev_cpuaddr >= region_base->cpu_start) && (dev_cpuaddr <= region_base->cpu_end)) {
            /* 地址在范围内 */
            offset = dev_cpuaddr - region_base->cpu_start;
            if (addr_map != NULL) {
                /* 返回HOST虚拟地址 */
                addr_map->va = (uintptr_t)(region_base->vaddr + offset);
                /* 返回HOST物理地址 */
                addr_map->pa = (uintptr_t)(region_base->paddr + offset);
            }
            return OAL_SUCC;
        } else {
            continue;
        }
    }

    return -OAL_ENOMEM;
}

/* 地址转换均为线性映射，注意起始地址和大小 */
int32_t oal_pcie_inbound_va_to_ca(oal_pcie_comm_res *comm_res, uint64_t host_va, uint64_t *dev_cpuaddr)
{
    int32_t index;
    int32_t region_num;
    uint64_t offset;
    oal_pcie_region *region_base;
    uint64_t end;
    uint64_t vaddr = host_va;

    region_num = comm_res->regions.region_nums;
    region_base = comm_res->regions.pst_regions;

    if (oal_warn_on(dev_cpuaddr == NULL)) {
        return -OAL_EINVAL;
    }

    *dev_cpuaddr = 0;

    if (oal_warn_on(!comm_res->regions.inited)) {
        return -OAL_ENODEV;
    }

    for (index = 0; index < region_num; index++, region_base++) {
        if (region_base->vaddr == NULL) {
            continue;
        }

        end = (uintptr_t)region_base->vaddr + region_base->size - 1;

        if ((vaddr >= (uintptr_t)region_base->vaddr) && (vaddr <= end)) {
            /* 地址在范围内 */
            offset = vaddr - (uintptr_t)region_base->vaddr;
            *dev_cpuaddr = region_base->cpu_start + offset;
            return OAL_SUCC;
        } else {
            continue;
        }
    }

    return -OAL_ENOMEM;
}

/* 检查通过PCIE操作的HOST侧虚拟地址是否合法 ，是否映射过 */
int32_t oal_pcie_vaddr_isvalid(oal_pcie_comm_res *comm_res, const void *vaddr)
{
    int32_t index;
    int32_t region_num;
    oal_pcie_region *region_base = NULL;
    if (oal_warn_on(!comm_res->regions.inited)) {
        return OAL_FALSE;
    }

    region_num = comm_res->regions.region_nums;
    region_base = comm_res->regions.pst_regions;

    for (index = 0; index < region_num; index++, region_base++) {
        if (region_base->vaddr == NULL) {
            continue;
        }

        if (((uintptr_t)vaddr >= (uintptr_t)region_base->vaddr)
            && ((uintptr_t)vaddr < (uintptr_t)region_base->vaddr + region_base->size)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
