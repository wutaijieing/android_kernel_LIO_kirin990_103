#ifndef __HI_SOCP_H__
#define __HI_SOCP_H__ 
#define HI_SOCP_GLOBAL_SRST_CTRL_OFFSET (0x0)
#define HI_SOCP_ENC_SRST_CTRL_OFFSET (0x4)
#define HI_SOCP_DEC_SRST_CTRL_OFFSET (0x8)
#define HI_SOCP_ENC_CH_STATUS_OFFSET (0xC)
#define HI_SOCP_DEC_CH_STATUS_OFFSET (0x10)
#define HI_SOCP_CLK_CTRL_OFFSET (0x14)
#define HI_SOCP_PRIOR_CFG_OFFSET (0x18)
#define HI_SOCP_DEC_INT_TIMEOUT_OFFSET (0x20)
#define HI_SOCP_INT_TIMEOUT_OFFSET (0x24)
#define HI_SOCP_BUFFER_TIMEOUT_OFFSET (0x28)
#define HI_SOCP_DEC_PKT_LEN_CFG_OFFSET (0x2C)
#define HI_SOCP_ENC_SRCCH_SECCTRL_OFFSET (0x30)
#define HI_SOCP_ENC_DSTCH_SECCTRL_OFFSET (0x34)
#define HI_SOCP_DEC_SRCCH_SECCTRL_OFFSET (0x38)
#define HI_SOCP_DEC_DSTCH_SECCTRL_OFFSET (0x3C)
#define HI_SOCP_GLB_OFFSET_SECCTRL_OFFSET (0x40)
#define HI_SOCP_GLOBAL_INT_STATUS_OFFSET (0x50)
#define HI_SOCP_ENC_CORE0_MASK0_OFFSET (0x54)
#define HI_SOCP_ENC_CORE0_RAWINT0_OFFSET (0x58)
#define HI_SOCP_ENC_CORE0_INT0_OFFSET (0x5C)
#define HI_SOCP_ENC_CORE0_MASK1_OFFSET (0x60)
#define HI_SOCP_ENC_CORE1_MASK1_OFFSET (0x64)
#define HI_SOCP_ENC_RAWINT1_OFFSET (0x68)
#define HI_SOCP_ENC_CORE0_INT1_OFFSET (0x6C)
#define HI_SOCP_ENC_CORE1_INT1_OFFSET (0x70)
#define HI_SOCP_ENC_CORE0_MASK2_OFFSET (0x74)
#define HI_SOCP_ENC_CORE0_RAWINT2_OFFSET (0x78)
#define HI_SOCP_ENC_CORE0_INT2_OFFSET (0x7C)
#define HI_SOCP_ENC_CORE0_MASK3_OFFSET (0x80)
#define HI_SOCP_ENC_CORE1_MASK3_OFFSET (0x84)
#define HI_SOCP_ENC_RAWINT3_OFFSET (0x88)
#define HI_SOCP_ENC_CORE0_INT3_OFFSET (0x8C)
#define HI_SOCP_ENC_CORE1_INT3_OFFSET (0x90)
#define HI_SOCP_DEC_CORE0_MASK0_OFFSET (0xA8)
#define HI_SOCP_DEC_CORE1_MASK0_OFFSET (0xAC)
#define HI_SOCP_DEC_RAWINT0_OFFSET (0xB0)
#define HI_SOCP_DEC_CORE0_INT0_OFFSET (0xB4)
#define HI_SOCP_DEC_CORE1_INT0_OFFSET (0xB8)
#define HI_SOCP_DEC_CORE0_MASK1_OFFSET (0xBC)
#define HI_SOCP_DEC_CORE0_RAWINT1_OFFSET (0xC0)
#define HI_SOCP_DEC_CORE0_INT1_OFFSET (0xC4)
#define HI_SOCP_DEC_CORE0_MASK2_OFFSET (0xC8)
#define HI_SOCP_DEC_CORE1NOTE_MASK2_OFFSET (0xCC)
#define HI_SOCP_DEC_RAWINT2_OFFSET (0xD0)
#define HI_SOCP_DEC_CORE0NOTE_NT2_OFFSET (0xD4)
#define HI_SOCP_DEC_CORE1NOTE_INT2_OFFSET (0xD8)
#define HI_SOCP_ENC_CORE1_MASK0_OFFSET (0xDC)
#define HI_SOCP_ENC_CORE1_INT0_OFFSET (0xE0)
#define HI_SOCP_ENC_CORE1_MASK2_OFFSET (0xE4)
#define HI_SOCP_ENC_CORE1_INT2_OFFSET (0xE8)
#define HI_SOCP_DEC_CORE1_MASK1_OFFSET (0xEC)
#define HI_SOCP_DEC_CORE1_INT1_OFFSET (0xF0)
#define HI_SOCP_BUS_ERROR_MASK_OFFSET (0xF4)
#define HI_SOCP_BUS_ERROR_RAWINT_OFFSET (0xF8)
#define HI_SOCP_BUS_ERROR_INT_OFFSET (0xFC)
#define HI_SOCP_ENC_SRC_BUFM_WPTR_0_OFFSET (0x100)
#define HI_SOCP_ENC_SRC_BUFM_RPTR_0_OFFSET (0x104)
#define HI_SOCP_ENC_SRC_BUFM_ADDR_L_0_OFFSET (0x108)
#define HI_SOCP_ENC_SRC_BUFM_ADDR_H_0_OFFSET (0x10C)
#define HI_SOCP_ENC_SRC_BUFM_DEPTH_0_OFFSET (0x110)
#define HI_SOCP_ENC_SRC_BUFM_CFG_0_OFFSET (0x114)
#define HI_SOCP_ENC_SRC_RDQ_WPTR_0_OFFSET (0x118)
#define HI_SOCP_ENC_SRC_RDQ_RPTR_0_OFFSET (0x11C)
#define HI_SOCP_ENC_SRC_RDQ_BADDR_L_0_OFFSET (0x120)
#define HI_SOCP_ENC_SRC_RDQ_BADDR_H_0_OFFSET (0x124)
#define HI_SOCP_ENC_SRC_RDQ_CFG_0_OFFSET (0x128)
#define HI_SOCP_ENC_DEST_BUFN_WPTR_0_OFFSET (0x900)
#define HI_SOCP_ENC_DEST_BUFN_RPTR_0_OFFSET (0x904)
#define HI_SOCP_ENC_DEST_BUFN_ADDR_L_0_OFFSET (0x908)
#define HI_SOCP_ENC_DEST_BUFN_ADDR_H_0_OFFSET (0x90C)
#define HI_SOCP_ENC_DEST_BUFN_DEPTH_0_OFFSET (0x910)
#define HI_SOCP_ENC_DEST_BUFN_THRH_0_OFFSET (0x914)
#define HI_SOCP_ENC_INT_THRESHOLD_0_OFFSET (0x918)
#define HI_SOCP_ENC_DEST_SB_CFG_OFFSET (0x91C)
#define HI_SOCP_DEC_SRC_BUFX_WPTR_0_OFFSET (0xA00)
#define HI_SOCP_DEC_SRC_BUFX_RPTR_0_OFFSET (0xA04)
#define HI_SOCP_DEC_SRC_BUFX_ADDR_L_0_OFFSET (0xA08)
#define HI_SOCP_DEC_SRC_BUFX_ADDR_H_0_OFFSET (0xA0C)
#define HI_SOCP_DEC_SRC_BUFX_CFG0_0_OFFSET (0xA10)
#define HI_SOCP_DEC_BUFX_STATUS0_0_OFFSET (0xA14)
#define HI_SOCP_DEC_BUFX_STATUS1_0_OFFSET (0xA18)
#define HI_SOCP_DEC_DEST_BUFY_WPTR_0_OFFSET (0xC00)
#define HI_SOCP_DEC_DEST_BUFY_RPTR_0_OFFSET (0xC04)
#define HI_SOCP_DEC_DEST_BUFY_ADDR_L_0_OFFSET (0xC08)
#define HI_SOCP_DEC_DEST_BUFY_ADDR_H_0_OFFSET (0xC0C)
#define HI_SOCP_DEC_DEST_BUFY_CFG0_0_OFFSET (0xC10)
#define HI_SOCP_ENC_OBUF_0_PKT_NUM_OFFSET (0xE80)
#define HI_SOCP_SOCP_VERSION_OFFSET (0xFFC)
#endif
