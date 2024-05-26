/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    hisp_pcie.h
 * Description:
 *
 * Version      1.0
 ******************************************************************/

#ifndef _HISP_PCIE_H_INCLUDED__
#define _HISP_PCIE_H_INCLUDED__

#ifdef DEBUG_HISP
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>

#define ISP_PCIE_VENDER            0x19e5
#define ISP_PCIE_DEVICE            0x369D
#define ISP_PCIE_RC_ID             0
#define ISP_PCIE_BAR_ID            0

#define ISP_PCIE_BAR0_BASE         0xF9000000
#define ISP_PCIE_OUTBOUND_START    0xE9000000
#define ISP_PCIE_IRQ_NUM           0x2
#define SIZE_16M                   0x01000000

#define ISP_PCIE_READY             1
#define ISP_PCIE_NREADY            (-1)
#define ISP_IPC_OFFSET_PCIE        (0xE9BD4000 - ISP_PCIE_OUTBOUND_START)

struct _hisp_bar_info {
	unsigned long bar_addr;
	unsigned long bar_size;
};

struct _isp_rsc {
	unsigned int start;
	unsigned int size;
};

enum HISP_REG_TYPE {
	ISP_CORE_REG,
	ISP_SUBCTRL_REG,
	ISP_IPC_REG,
	MAX_HISP_PCIE_REG
};

#define ISPCPU_BASE_CPUID_ACPU            (0)
#define ISPCPU_BASE_CPUID_ISPCPU          (1)
#define ISPCPU_BASE_MBOXID_ACPU0          (0)
#define ISPCPU_BASE_MBOXID_ISPCPU0        (2)
#define ISPCPU_INT_MASK                   (0xF)
#define ISPCPU_INT_UNMASK                 (0x0)
#define IPC_UNLOCK_KEY                    (0x1ACCE551)

#define isp_ipc_mbx_source_addr(base, i)  ((base) + (0x000+(i)*64UL))
#define isp_ipc_mbx_dset_addr(base, i)    ((base) + (0x004+(i)*64UL))
#define isp_ipc_mbx_mode_addr(base, i)    ((base) + (0x010+(i)*64UL))
#define isp_ipc_mbx_imask_addr(base, i)   ((base) + (0x014+(i)*64UL))
#define isp_ipc_mbx_iclr_addr(base, i)    ((base) + (0x018+(i)*64UL))
#define isp_ipc_mbx_send_addr(base, i)    ((base) + (0x01C+(i)*64UL))
#define isp_ipc_mbx_data0_addr(base, i)   ((base) + (0x020+(i)*64UL))
#define isp_ipc_ipc_lock_addr(base)       ((base) + (0xA00UL))

typedef union {
	struct {
		unsigned int    auto_answer  : 1  ; /* [0] */
		unsigned int    auto_link    : 1  ; /* [1] */
		unsigned int    rsv_4        : 2  ; /* [3:2] */
		unsigned int    state_status : 4  ; /* [7:4] */
		unsigned int    rsv_5        : 24  ; /* [31:8] */
	} reg;
	unsigned int    value;
} u_mbx_mode;

typedef union {
	struct {
		unsigned int    send  : 3  ; /* [2:0] */
		unsigned int    rsv_8 : 29  ; /* [31:3] */
	} reg;
	unsigned int    value;
} u_mbx_send;

typedef union {
	struct {
		unsigned int    int_clear : 3  ; /* [2:0] */
		unsigned int    rsv_7     : 29  ; /* [31:3] */
	} reg;
	unsigned int    value;
} u_mbx_iclr;

typedef union {
	struct {
		unsigned int    source : 3  ; /* [2:0] */
		unsigned int    rsv_0  : 29  ; /* [31:3] */
	} reg;
	unsigned int    value;
} u_mbx_source;

typedef union {
	struct {
		unsigned int    dset  : 3  ; /* [2:0] */
		unsigned int    rsv_1 : 29  ; /* [31:3] */
	} reg;
	unsigned int    value;
} u_mbx_dset;

typedef union {
	struct {
		unsigned int    int_mask_status : 16  ; /* [15:0] */
		unsigned int    rsv_9           : 16  ; /* [31:16] */
	} reg;
	unsigned int    value;
} u_mbx_imst;

int hisp_pcie_register(void);
int hisp_pcie_unregister(void);
int hisp_get_pcie_outbound(struct isp_pcie_cfg *cfg);
int hisp_pcie_send_ipc2isp(rproc_msg_t *msg, rproc_msg_len_t len);
int __attribute__((weak)) pcie_kport_enumerate(unsigned int rc_idx) {(void)rc_idx; return -1;}
unsigned int __attribute__((weak)) pcie_set_mem_outbound(unsigned int rc_id, struct pci_dev *dev,
	int bar, u64 target) {(void)rc_id, (void)dev, (void)bar, (void)target; return -1;}
#endif
#endif

/* **************************************end*********************************** */
