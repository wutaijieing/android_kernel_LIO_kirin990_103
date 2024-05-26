/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
  /**
 *  @brief   socp模块在COMMON上的对外头文件
 *  @file    mdrv_socp_common.h
 *  @author  yaningbo
 *  @version v1.0
 *  @date    2019.11.29
 *  @note    修改记录(版本号|修订日期|说明) 
 *  <ul><li>v1.0|2012.11.29|创建文件</li></ul>
 *  <ul><li>v2.0|2019.11.29|接口自动化</li></ul>
 *  @since   
 */

#ifndef __MDRV_SOCP_COMMON_H__
#define __MDRV_SOCP_COMMON_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <product_config.h>

/**************************************************************************
  宏定义
**************************************************************************/
/**
 * @brief socp coder source channel type
 */
#define SOCP_CODER_SRC_CHAN                 0x00
/**
 * @brief socp coder dest channel type
 */
#define SOCP_CODER_DEST_CHAN                0x01
/**
 * @brief socp decoder source channel type
 */
#define SOCP_DECODER_SRC_CHAN               0x02
/**
 * @brief socp decoder dest channel type
 */
#define SOCP_DECODER_DEST_CHAN              0x03
/**
 * @brief socp source channel packet max length
 */
#define SOCP_CODER_SRC_PKT_LEN_MAX              (4*1024)

/**
 * @brief socp channel type define
 */
#define SOCP_CHAN_DEF(chan_type, chan_id)   ((chan_type<<16)|chan_id)
/**
 * @brief get socp channel id by socp channel
 */
#define SOCP_REAL_CHAN_ID(unique_chan_id)   (unique_chan_id & 0xFFFF)
/**
 * @brief get socp channel type by socp channel
 */
#define SOCP_REAL_CHAN_TYPE(unique_chan_id) (unique_chan_id>>16)

/**
 * @brief socp dest channel dsm state:enable
 */
#define SOCP_DEST_DSM_ENABLE                0x01
/**
 * @brief socp dest channel dsm state:disable
 */
#define SOCP_DEST_DSM_DISABLE               0x00

/* 编码源通道ID枚举定义 */
/* 见soc_socp_adapter.h 中enum SOCP_CODER_SRC_ENUM枚举定义 */
/**
 * @brief socp coder source channel
 */
typedef unsigned int SOCP_CODER_SRC_ENUM_U32;

/**
 * @brief socp decoder source channel, now no used
 */
enum SOCP_DECODER_SRC_ENUM
{
    SOCP_DECODER_SRC_LOM        = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN,0),   /**< LTE OM channel */
    SOCP_DECODER_SRC_HDLC_AT    = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN,1),   /**< AT channel */
    SOCP_DECODER_SRC_GUOM       = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN,2),   /**< GU OM Channel */
    SOCP_DECODER_SRC_RFU        = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN,3),   /**< Reserve */
    SOCP_DECODER_SRC_BUTT
};
/**
 * @brief socp decoder source channel
 */
typedef unsigned int SOCP_DECODER_SRC_ENUM_U32;
/**
 * @brief socp coder dest channel
 */
enum SOCP_CODER_DST_ENUM
    {
    SOCP_CODER_DST_OM_CNF        = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,0), /**< diag cnf channel */
    SOCP_CODER_DST_OM_IND        = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,1), /**< diag ind channel */
    SOCP_CODER_DST_HDLC_AT       = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,2), /**< secure dump channel */
    SOCP_CODER_DST_RFU0          = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,3), /**< log server channel */
    SOCP_CODER_DST_RFU1          = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,4), /**< before v765, it is 7 channels. after it's 4 channels */
    SOCP_CODER_DST_RFU2          = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,5), /**< before v765, it is 7 channels. after it's 4 channels */
    SOCP_CODER_DST_RFU3          = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN,6), /**< before v765, it is 7 channels. after it's 4 channels */
    SOCP_CODER_DST_BUTT
};
/**
 * @brief socp coder dest channel
 */
typedef unsigned int SOCP_CODER_DST_ENUM_U32;

/**
 * @brief socp decoder dest channel, now no used
 */
enum SOCP_DECODER_DST_ENUM
    {
    SOCP_DECODER_DST_LOM        = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN,0),  /**< LTE OM channel */
    SOCP_DECODER_DST_HDLC_AT    = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN,1),  /**< AT channel */
    SOCP_DECODER_DST_GUOM       = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN,2),  /**< GU OM Channel */
    SOCP_DECODER_DST_RFU        = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN,3),  /**< Reserve */
    SOCP_DECODER_CBT            = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN,16), /**< GU CBT Channel */
    SOCP_DECODER_DST_BUTT
};
/**
 * @brief socp decoder dest channel
 */
typedef unsigned int SOCP_DECODER_DST_ENUM_U32;

/**
 * @brief socp coder source channel base
 */
#define SOCP_CODER_SRC_CHAN_BASE            0x00000000
/**
 * @brief socp coder dest channel base
 */
#define SOCP_CODER_DEST_CHAN_BASE           0x00010000
/**
 * @brief socp decoder source channel base
 */
#define SOCP_DECODER_SRC_CHAN_BASE          0x00020000
/**
 * @brief socp decoder dest channel base
 */
#define SOCP_DECODER_DEST_CHAN_BASE         0x00030000

/**************************************************************************
  结构定义
**************************************************************************/
/**
 * @brief socp BD data type
 */
enum SOCP_BD_TYPE_ENUM
{
    SOCP_BD_DATA            = 0,    /**< data type  */
    SOCP_BD_LIST            = 1,    /**< list type */
    SOCP_BD_BUTT                    /**< hdlc flag butt */
};
/**
 * @brief socp BD data type
 */
typedef unsigned short SOCP_BD_TYPE_ENUM_UINT16;

/**
 * @brief socp hdlc flag
 */
typedef enum SOCP_HDLC_FLAG_ENUM
{
    SOCP_HDLC_ENABLE         = 0,    /**< enable */
    SOCP_HDLC_DISABLE        = 1,    /**< disable */
    SOCP_HDLC_FLAG_BUTT              /**< hdlc flag butt */
} socp_hdlc_flag_e;
/**
 * @brief socp hdlc flag
 */
typedef unsigned int SOCP_HDLC_FLAG_UINT32;
/**
 * @brief socp state
 */
enum SOCP_STATE_ENUM
{
    SOCP_IDLE               = 0,    /**< socp is in idle */
    SOCP_BUSY,                      /**< socp is busy */
    SOCP_UNKNOWN_BUTT              /**< socp is in unknown state */
};
/**
 * @brief socp state
 */
typedef unsigned int SOCP_STATE_ENUM_UINT32;

/**
 * @brief socp data type
 */
enum SCM_DATA_TYPE_ENUM
    {
    SCM_DATA_TYPE_TL            = 0,            /**< tl data type */
    SCM_DATA_TYPE_GU,                           /**< gu data type */
    SCM_DATA_TYPE_BUTT                          /**< data type butt */
};
/**
 * @brief socp data type
 */
typedef unsigned  char SOCP_DATA_TYPE_ENUM_UIN8;

/**
 * @brief socp bd struct, for list mode
 */
typedef struct
{
#ifdef FEATURE_SOCP_ADDR_64BITS
    unsigned long long                 ulDataAddr;     /**< bd data start address, socp is going to use */
    unsigned short                     usMsgLen;       /**< bd data length */
    unsigned short                     enDataType;     /**< bd data datatype */
    unsigned int                       reservd;        /**< Reserve */
#else
    unsigned int                      ulDataAddr;       /**< bd data start address, socp is going to use */
    unsigned short                    usMsgLen;         /**< bd data length */
    unsigned short                    enDataType;       /**< bd data datatype */
#endif
}SOCP_BD_DATA_STRU;

/**
 * @brief socp rd struct, for list mode
 */
typedef struct
{
#ifdef FEATURE_SOCP_ADDR_64BITS
    unsigned long long                 ulDataAddr;      /**< rd data start address, user to free */
    unsigned short                     usMsgLen;        /**< rd data length */
    unsigned short                     enDataType;      /**< rd data datatype */
    unsigned int                       reservd;         /**< Reserve */
#else
    unsigned int                       ulDataAddr;      /**< rd data start address, user to free */
    unsigned short                     usMsgLen;        /**< rd data length */
    unsigned short                     enDataType;      /**< rd data datatype */
#endif
}SOCP_RD_DATA_STRU;
/**
 * @brief socp event
 */
enum tagSOCP_EVENT_E
{
        SOCP_EVENT_PKT_HEADER_ERROR         = 0x1,    /**< socp packet header is error */
        SOCP_EVENT_OUTBUFFER_OVERFLOW       = 0x2,    /**< socp dest channel is overflow */
        SOCP_EVENT_RDBUFFER_OVERFLOW        = 0x4,    /**< socp dest channel rd buffer is overflow */
        SOCP_EVENT_DECODER_UNDERFLOW        = 0x8,    /**< socp deccode source channel is underflow */
        SOCP_EVENT_PKT_LENGTH_ERROR         = 0x10,   /**< socp deccode packet length error */
        SOCP_EVENT_CRC_ERROR                = 0x20,   /**< socp deccode packet crc is error */
        SOCP_EVENT_DATA_TYPE_ERROR          = 0x40,   /**< socp deccode data type is error */
        SOCP_EVENT_HDLC_HEADER_ERROR        = 0x80,   /**< socp deccode hdlc header is error */
        SOCP_EVENT_OUTBUFFER_THRESHOLD_OVERFLOW = 0x100, /**< socp dest channel is threshold overflow */
        SOCP_EVENT_BUTT                                /**< event butt */
};
/**
 * @brief socp event
 */
typedef unsigned int SOCP_EVENT_ENUM_UIN32;

/**
 * @brief socp coder source channel mode
 */
typedef enum tagSOCP_ENCSRC_CHNMODE_E
{
    SOCP_ENCSRC_CHNMODE_CTSPACKET       = 0,    /**< coder source block ring buffer */
    SOCP_ENCSRC_CHNMODE_FIXPACKET,              /**< coder source fix length ring buffer */
    SOCP_ENCSRC_CHNMODE_LIST,                   /**< coder source list ring buffer */
    SOCP_ENCSRC_CHNMODE_BUTT                    /**< coder souce channel mode butt */
} socp_encsrc_chnmode_e;
/**
 * @brief socp coder source channel mode
 */
typedef unsigned int SOCP_ENCSRC_CHNMODE_ENUM_UIN32;

/**
 * @brief socp decoder source channel mode
 */
enum tagSOCP_DECSRC_CHNMODE_E
{
    SOCP_DECSRC_CHNMODE_BYTES        = 0,       /**< decoder source bytes*/
    SOCP_DECSRC_CHNMODE_LIST,                   /**< decoder source list*/
    SOCP_DECSRC_CHNMODE_BUTT                    /**< decoder souce channel mode butt */
};
/**
 * @brief socp decoder source channel mode
 */
typedef unsigned int SOCP_DECSRC_CHNMODE_ENUM_UIN32;

/**
 * @brief socp timeout mode
 */
enum tagSOCP_TIMEOUT_EN_E
{
    SOCP_TIMEOUT_BUFOVF_DISABLE        = 0,       /**< buffer overflow, no interrupt */
    SOCP_TIMEOUT_BUFOVF_ENABLE,                   /**< buffer overflow, when counts exceed report interrupt */
    SOCP_TIMEOUT_TRF,                             /**< transfer interrupt */
    SOCP_TIMEOUT_TRF_LONG,                        /**< transfer for long time */
    SOCP_TIMEOUT_TRF_SHORT,                       /**< transfer for short time */
    SOCP_TIMEOUT_DECODE_TRF,                      /**< decode transfer interrupt */
    SOCP_TIMEOUT_BUTT                             /**< timeout mode butt */
};
/**
 * @brief socp timeout mode
 */
typedef unsigned int SOCP_TIMEOUT_EN_ENUM_UIN32;

/**
 * @brief socp source channel priority
 */
typedef enum tagSOCP_CHAN_PRIORITY_E
{
    SOCP_CHAN_PRIORITY_0     = 0,               /**< lowest priority */
    SOCP_CHAN_PRIORITY_1,                       /**< lower priority */
    SOCP_CHAN_PRIORITY_2,                       /**< higher priority */
    SOCP_CHAN_PRIORITY_3,                       /**< highest priority */
    SOCP_CHAN_PRIORITY_BUTT                     /**< priority butt */
} socp_chan_priority_e;
/**
 * @brief socp source channel priority
 */
typedef unsigned int SOCP_CHAN_PRIORITY_ENUM_UIN32;

/**
 * @brief socp data type
 */
typedef enum tagSOCP_DATA_TYPE_E
{
    SOCP_DATA_TYPE_0            = 0,            /**< lte oam data type*/
    SOCP_DATA_TYPE_1,                           /**< gu oam data type */
    SOCP_DATA_TYPE_2,                           /**< reserve*/
    SOCP_DATA_TYPE_3,                           /**< reserve*/
    SOCP_DATA_TYPE_BUTT                         /**< data type butt */
} socp_data_type_e;
/**
 * @brief socp data type
 */
typedef unsigned int SOCP_DATA_TYPE_ENUM_UIN32;


/**
 * @brief bbp data sample mode
 */
enum tagSOCP_BBP_DS_MODE_E
{
    SOCP_BBP_DS_MODE_DROP           = 0,        /**< data drop mode */
    SOCP_BBP_DS_MODE_OVERRIDE,                  /**< data override mode */
    SOCP_BBP_DS_MODE_BUTT                       /**< data sample mode butt */
};
/**
 * @brief bbp data sample mode
 */
typedef unsigned int SOCP_BBP_DS_MODE_ENUM_UIN32;

/**
 * @brief socp source channel data type state
 */
typedef enum tagSOCP_DATA_TYPE_EN_E
{
    SOCP_DATA_TYPE_EN           = 0,        /**< data type enable, default*/
    SOCP_DATA_TYPE_DIS,                     /**< data type disable*/
    SOCP_DATA_TYPE_EN_BUTT                  /**< data type butt*/
} socp_data_type_en_e;
/**
 * @brief socp source channel data type state
 */
typedef unsigned int SOCP_DATA_TYPE_EN_ENUM_UIN32;

/**
 * @brief socp source channel trans id state
 */
typedef enum tagSOCP_TRANS_ID_EN_E
{
    SOCP_TRANS_ID_DIS           = 0,      /**< trans id disable, default*/
    SOCP_TRANS_ID_EN,                     /**< trans id enable*/
    SOCP_TRANS_ID_EN_BUTT                 /**< trans id state butt*/
} socp_trans_id_en_e;
/**
 * @brief socp source channel trans id state
 */
typedef unsigned int SOCP_TRANS_ID_EN_ENUM_UINT32;

/**
 * @brief socp source channel pointer imgae state
 */
typedef enum tagSOCP_PTR_IMG_EN_E
{
    SOCP_PTR_IMG_DIS           = 0,      /**< pointer image disable, default*/
    SOCP_PTR_IMG_EN,                     /**< pointer image enable*/
    SOCP_PTR_IMG_EN_BUTT                 /**< pointer image butt*/
} socp_ptr_img_en_e;
/**
 * @brief socp source channel pointer imgae state
 */
typedef unsigned int SOCP_PTR_IMG_EN_ENUM_UINT32;

/**
 * @brief socp source channel debug state
 */
typedef enum tagSOCP_ENC_DEBUG_EN_E
{
    SOCP_ENC_DEBUG_DIS          = 0,       /**< debug disable, default*/
    SOCP_ENC_DEBUG_EN,                     /**< debug enable*/
    SOCP_ENC_DEBUG_EN_BUTT                 /**< debug state butt*/
} socp_enc_debug_en_e;
/**
 * @brief socp source channel debug state
 */
typedef unsigned int SOCP_ENC_DEBUG_EN_ENUM_UIN32;

/**
 * @brief socp dest channel buffer mode
 */
typedef enum
{
    SOCP_DST_CHAN_NOT_CFG = 0,    /**< not config */
    SOCP_DST_CHAN_DELAY,          /**< delay report mode, need more dest channel buffer*/
    SOCP_DST_CHAN_DTS             /**< config dest channel buffer bye dts*/
} SOCP_DST_CHAN_CFG_TYPE_ENUM;
/**
 * @brief deflate dest channel buffer mode
 */
typedef enum
{
    DEFLATE_DST_CHAN_NOT_CFG = 0,    /**< not config */
    DEFLATE_DST_CHAN_DELAY,          /**< delay report mode, need more dest channel buffer*/
    DEFLATE_DST_CHAN_DTS             /**< config dest channel buffer bye dts*/
} DEFLATE_DST_CHAN_CFG_TYPE_ENUM;

/**
 * @brief socp decode packet length struct
 */
typedef struct tagSOCP_DEC_PKTLGTH_S
{
    unsigned int             u32PktMax;         /**< max packet length */
    unsigned int             u32PktMin;         /**< min packet length*/
}SOCP_DEC_PKTLGTH_STRU;


/**
 * @brief socp src channel buffer struct
 */
typedef struct
{

    unsigned  char *pucInputStart;      /**< block buffer start address */
    unsigned  char *pucInputEnd;        /**< block buffer end address */
    unsigned  char *pucRDStart;         /**< rd buffer start address */
    unsigned  char *pucRDEnd;           /**< rd buffer end address */
    unsigned  int u32RDThreshold;      /**< rd buffer report threshold */
    unsigned  int ulRsv;               /**< reserve */
}SOCP_SRC_SETBUF_STRU;

/**
 * @brief socp dest channel buffer struct
 */
typedef struct
{
    unsigned char *pucOutputStart;     /**< output buffer start address */
    unsigned char *pucOutputEnd;       /**< output buffer end address */
    unsigned int u32Threshold;        /**< buffer report threshold */
    unsigned int ulRsv;               /**< reserve */

}SOCP_DST_SETBUF_STRU;

typedef struct {
    unsigned char *input_start;    /**< block buffer start address */
    unsigned char *input_end;      /**< block buffer end address */
    unsigned char *rd_input_start; /**< rd buffer start address */
    unsigned char *rd_input_end;   /**< rd buffer end address */
    unsigned int rd_threshold;     /**< rd buffer report threshold */
    unsigned short rd_timeout;     /**< rd buffer time out cost */
    unsigned short rsv;            /**< reserve */
} socp_src_setbuf_s;

#pragma pack(push)

#pragma pack(4)

/**
 * @brief socp src channel struct
 */
typedef struct tagSOCP_CODER_SRC_CHAN_S
{
    unsigned int                    u32DestChanID;      /**< dest channel id */
    SOCP_HDLC_FLAG_UINT32           u32BypassEn;        /**< bypass enable or not */
    SOCP_TRANS_ID_EN_ENUM_UINT32    eTransIdEn;         /**< trans id enable or not */
    SOCP_PTR_IMG_EN_ENUM_UINT32     ePtrImgEn;          /**< pointer image enable or not */
    SOCP_DATA_TYPE_ENUM_UIN32       eDataType;          /**< data type */
    SOCP_DATA_TYPE_EN_ENUM_UIN32    eDataTypeEn;        /**< data type enable or not */
    SOCP_ENC_DEBUG_EN_ENUM_UIN32    eDebugEn;           /**< debug enable or not */
    SOCP_ENCSRC_CHNMODE_ENUM_UIN32  eMode;              /**< channel mode */
    SOCP_CHAN_PRIORITY_ENUM_UIN32   ePriority;          /**< channel priority */
    unsigned long                   eRptrImgPhyAddr;    /**< block, read pointer image phyical address */
    unsigned long                   eRptrImgVirtAddr;   /**< block, read pointer image virtual address */
    SOCP_SRC_SETBUF_STRU            sCoderSetSrcBuf;    /**< source channel buffer set for */
}SOCP_CODER_SRC_CHAN_S;

/**
 * @brief socp rd descriptor structure
 */
typedef struct {
    unsigned int dest_chan_id;               /**< dest channel id */
    socp_hdlc_flag_e bypass_en;              /**< bypass enable or not */
    socp_trans_id_en_e trans_id_en;          /**< trans id enable or not */
    socp_ptr_img_en_e rptr_img_en;           /**< pointer image enable or not */
    socp_data_type_e data_type;              /**< data type */
    socp_data_type_en_e data_type_en;        /**< data type enable or not */
    socp_enc_debug_en_e debug_en;            /**< debug enable or not */
    socp_encsrc_chnmode_e mode;              /**< channel mode */
    socp_chan_priority_e priority;           /**< channel priority */
    unsigned long read_ptr_img_phy_addr;     /**< block, read pointer image phyical address */
    unsigned long read_ptr_img_vir_addr;     /**< block, read pointer image virtual address */
    unsigned long rd_write_ptr_img_phy_addr; /**< rd, write pointer image phyical address  */
    unsigned long rd_write_ptr_img_vir_addr; /**< rd, write pointer image virtual address */
    unsigned int chan_group;                 /**< socp source channel group number */
    socp_src_setbuf_s coder_src_setbuf;
} socp_src_chan_cfg_s;
#pragma pack(pop)

/**
 * @brief socp coder dest channel  struct
 */
typedef struct tagSOCP_CODER_DEST_CHAN_S
{
    unsigned int                 u32EncDstThrh;           /**< coder dest channel buffer threshold */
    unsigned int                 u32EncDstTimeoutMode;    /**< coder dest channel timeout mode, after socp2.06 use */
    SOCP_DST_SETBUF_STRU         sCoderSetDstBuf;         /**< dest channel buffer set for */
}SOCP_CODER_DEST_CHAN_S;

/**
 * @brief socp decoder source channel  struct
 */
typedef struct tagSOCP_DECODER_SRC_CHAN_S
{
    SOCP_DATA_TYPE_EN_ENUM_UIN32     eDataTypeEn;        /**< data type enable or not */
    SOCP_DECSRC_CHNMODE_ENUM_UIN32   eMode;              /**< channel mode */
    SOCP_SRC_SETBUF_STRU       sDecoderSetSrcBuf;        /**< decoder source channel buffer set for */
}SOCP_DECODER_SRC_CHAN_STRU;

/**
 * @brief socp decoder dest channel  struct
 */
typedef struct tagSOCP_DECODER_DEST_CHAN_S
{
    unsigned int                 u32SrcChanID;       /**< source channel id */
    SOCP_DATA_TYPE_ENUM_UIN32        eDataType;      /**< data type */
    SOCP_DST_SETBUF_STRU       sDecoderDstSetBuf;    /**< decoder source channel buffer set for */
}SOCP_DECODER_DEST_CHAN_STRU;

#pragma pack(push)

#pragma pack(4)

/**
 * @brief socp read/write buffer
 */
typedef struct tagSOCP_BUFFER_RW_S
{
    char    *pBuffer;                                /**< ring buffer pointer */
    char    *pRbBuffer;                              /**< ring buffer pointer for rollback */
    unsigned int     u32Size;                        /**< ring buffer size */
    unsigned int     u32RbSize;                      /**< ring buffer rollback size */
}SOCP_BUFFER_RW_STRU;

#pragma pack(pop)

/**
 * @brief socp decoder error count debug info struct
 */
typedef struct tagSOCP_DECODER_ERROR_CNT_S
{
    unsigned int     u32PktlengthCnt;                /**< packet length error count */
    unsigned int     u32CrcCnt;                      /**< crc check error count */
    unsigned int     u32DataTypeCnt;                 /**< decode datatype error count */
    unsigned int     u32HdlcHeaderCnt;               /**< hdlc header check error count */
}SOCP_DECODER_ERROR_CNT_STRU;

/**
 * @brief socp reserve coder channel scope struct
 */
typedef struct tagSOCP_ENCSRC_RSVCHN_SCOPE_S
{
    unsigned int                 u32RsvIDMin;        /**< reserve channel min id */
    unsigned int                 u32RsvIDMax;        /**< reserve channel max id */
}SOCP_ENCSRC_RSVCHN_SCOPE_STRU;

/**
 * @brief socp vote, to delete
 */
enum SOCP_VOTE_ID_ENUM
{
    SOCP_VOTE_GU_DSP,       /**< gu dsp */
    SOCP_VOTE_DIAG_APP,     /**< drv app */
    SOCP_VOTE_DIAG_COMM,    /**< diag comm:ldsp, drv comm */
    SOCP_VOTE_TL_DSP,       /**< tl dsp */
    SOCP_VOTE_SOCP_REG,     /**< socp register */
    SOCP_VOTE_ID_BUTT
};
typedef unsigned int SOCP_VOTE_ID_ENUM_U32;

/**
 * @brief socp vote type, to delete
 */
enum SOCP_VOTE_TYPE_ENUM
{
    SOCP_VOTE_FOR_SLEEP,    /**< vote sleep */
    SOCP_VOTE_FOR_WAKE,     /**< vote wake */
    SOCP_VOTE_TYPE_BUTT     /**< vote type butt */
};
/**
 * @brief socp vote type, to delete
 */
typedef unsigned int SOCP_VOTE_TYPE_ENUM_U32;

/**
 * @brief socp event callback function pointer
 */
typedef int (*socp_event_cb)(unsigned int u32ChanID, SOCP_EVENT_ENUM_UIN32 u32Event, unsigned int u32Param);
/**
 * @brief socp read callback function pointer
 */
typedef int (*socp_read_cb)(unsigned int u32ChanID);
/**
 * @brief socp rd read callback function pointer
 */
typedef int (*socp_rd_cb)(unsigned int u32ChanID);

/**
 * @brief socp coder dest buff config
 */
typedef struct SOCP_ENC_DST_BUF_LOG_CFG
{
    void*           pVirBuffer;      /**< socp coder dest channel buffer virtual address */
    unsigned long   ulPhyBufferAddr; /**< socp coder dest channel buffer phyical address */
    unsigned int    BufferSize;      /**< socp coder dest channel buffer size*/
    unsigned int    overTime;        /**< socp coder dest channel buffer timeout */
    unsigned int    logOnFlag;       /**< socp coder dest channel buffer log is on flag */
    unsigned int    ulCurTimeout;    /**< socp coder dest channel buffer current timeout cost */
} SOCP_ENC_DST_BUF_LOG_CFG_STRU;
/**
 * @brief socp inner log max length
 */
#define INNER_LOG_DATA_MAX                   0x40
/**
 * @brief socp inner log record struct
 */
typedef struct
{
    unsigned int                  ulSlice;/**< slice */
    unsigned int                  ulFileNO;/**< file no */
    signed int                   lLineNO;  /**< line no */
    unsigned int                  ulP1;    /**< p1 */
    unsigned int                  ulP2;    /**< p2 */
}INNER_LOG_RECORD_STRU;
/**
 * @brief socp inner log data struct
 */
typedef struct
{
    unsigned int                  ulCnt;/**< count */
    INNER_LOG_RECORD_STRU       astLogData[INNER_LOG_DATA_MAX];/**< log data */
}INNER_LOG_DATA_STRU;
/**
 * @brief socp dest ind channel, log report mode
 */
typedef  enum
{
   SOCP_IND_MODE_DIRECT, /**< direct report mode */
   SOCP_IND_MODE_DELAY,  /**< delay report mode, buffers for buffer*/
   SOCP_IND_MODE_CYCLE   /**< cycle report mode, buffer override */
}SOCP_IND_MODE_ENUM;
typedef SOCP_IND_MODE_ENUM socp_ind_mode_e;
/**
 * @brief deflate ind compress state
 */
typedef  enum
{
   DEFLATE_IND_NO_COMPRESS, /**< not compress */
   DEFLATE_IND_COMPRESS     /**< compress mode*/
}DEFLATE_IND_COMPRESSS_ENUM;
typedef DEFLATE_IND_COMPRESSS_ENUM deflate_ind_compress_e;
/**
 * @brief deflate compress enable state
 */
enum DEFLATE_COMPRESS_STATE_ENUM
{
    COMPRESS_DISABLE_STATE       = 0, /**< compress disable*/
    COMPRESS_ENABLE_STATE,            /**< compress enable*/
};
/**
 * @brief socp debug info 
 */
typedef struct
{
    unsigned int u32SocpEncDstIsrOvfIntCnt[SOCP_CODER_DST_BUTT];         /**< ISR中进入编码目的通道buf 溢出中断次数 */
    unsigned int u32SocpEncDstIsrThresholdOvfIntCnt[SOCP_CODER_DST_BUTT];/**< ISR中进入编码目的通道buf阈值溢出中断次数 */
    unsigned int u32SocpEncDstOvfCnt[SOCP_CODER_DST_BUTT];               /**< 编码目的通道溢出总次数 */
    unsigned int u32SocpEncDstPartOvfCnt[SOCP_CODER_DST_BUTT];           /**< 编码目的通道80%溢出总次数 */
}SOCP_DEBUG_INFO_STRU;

/**************************************************************************
  函数声明
**************************************************************************/
/**
 * @brief 本接口用于初始化socp
 *
 * @par 描述:
 * 本接口用于初始化socp
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 0:  操作成功。
 * @retval 非0：操作失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_INIT(void);

/**
 * @brief 本接口用于控制SOCP目的端数据上报管理状态机
 *
 * @par 描述:
 * 本接口用于控制SOCP目的端数据上报管理状态机
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dest_chan_id 编码目的通道号
 * @param[in]  enable    是否打开socp目的端中断
 
 * @retval 0:  操作成功。
 * @retval 非0：操作失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_socp_send_data_manager(unsigned int dest_chan_id, unsigned int enable);

/**
 * @brief 本接口用于设置socp源通道属性
 *
 * @par 描述:
 * 本接口用于设置socp源通道属性
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 * @param[in]  src_attr    源通道属性
 *
 * @retval 0:  源通道属性设置成功。
 * @retval 非0：源通道属性设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_corder_set_src_chan(SOCP_CODER_SRC_ENUM_U32 src_chan_id, socp_src_chan_cfg_s *src_attr);

/**
 * @brief 本接口用于设置socp目的通道属性
 *
 * @par 描述:
 * 本接口用于设置socp目的通道属性
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id 目的通道ID
 * @param[in]  dst_attr    目的通道属性
 *
 * @retval 0:  目的通道属性设置成功。
 * @retval 非0：目的通道属性设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_coder_set_dest_chan_attr(unsigned int dst_chan_id, SOCP_CODER_DEST_CHAN_S *dst_attr);

/**
 * @brief 本接口用于完成SOCP解码器目标通道的分配
 *
 * @par 描述:
 * 本接口用于完成SOCP解码器目标通道的分配，根据解码目标通道参数设置通道属性，并连接源通道，返回函数执行结果。
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id 目的通道ID
 * @param[in]  dst_attr    目的通道属性
 *
 * @retval SOCP_OK:  目的通道属性设置成功。
 * @retval 非SOCP_OK：目的通道属性设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_DECODER_SET_DEST_CHAN(SOCP_DECODER_DST_ENUM_U32 dst_chan_id, SOCP_DECODER_DEST_CHAN_STRU *dst_attr);

/**
 * @brief 本接口用于完成SOCP解码器源通道属性设置
 *
 * @par 描述:
 * 本接口用于完成SOCP解码器源通道属性设置
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 * @param[in]  src_attr    源通道属性
 *
 * @retval SOCP_OK:  目的通道属性设置成功。
 * @retval 非SOCP_OK：目的通道属性设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_DECODER_SET_SRC_CHAN_ATTR ( unsigned int src_chan_id,SOCP_DECODER_SRC_CHAN_STRU *src_attr);

/**
 * @brief 本接口用于给出解码通道中四种异常情况的计数值。
 *
 * @par 描述:
 * 本接口用于给出解码通道中四种异常情况的计数值。
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  chan_id   解码通道ID
 * @param[in]  err_cnt   异常统计信息
 *
 * @retval SOCP_OK:  获取异常统计信息成功。
 * @retval 非SOCP_OK：获取异常统计信息失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_DECODER_GET_ERR_CNT (unsigned int chan_id, SOCP_DECODER_ERROR_CNT_STRU *err_cnt);

/**
 * @brief 本接口用于释放分配的编解码通道
 *
 * @par 描述:
 * 本接口用于释放分配的编解码通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  chan_id 通道ID
 *
 * @retval 0:  操作成功。
 * @retval 非0：操作失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_FREE_CHANNEL(unsigned int chan_id);

/**
 * @brief 本接口用于清空编码源通道
 *
 * @par 描述:
 * 本接口用于清空编码源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 *
 * @retval 0:  操作成功。
 * @retval 非0：操作失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
unsigned int DRV_SOCP_CLEAN_ENCSRC_CHAN(SOCP_CODER_SRC_ENUM_U32 src_chan_id);
#define SOCP_CleanEncSrcChan(enSrcChanID)  DRV_SOCP_CLEAN_ENCSRC_CHAN(enSrcChanID)
/**
 * @brief 本接口用于给定通道注册事件回调函数
 *
 * @par 描述:
 * 本接口用于给定通道注册事件回调函数
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 * @param[in]  event_cb 事件回调函数
 *
 * @retval 0:  启动源通道成功。
 * @retval 非0：启动源通道失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_REGISTER_EVENT_CB(unsigned int src_chan_id, socp_event_cb event_cb);

/**
 * @brief 本接口用于启动socp源通道
 *
 * @par 描述:
 * 本接口用于启动socp源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 *
 * @retval 0:  启动源通道成功。
 * @retval 非0：启动源通道失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_start(unsigned int src_chan_id);

/**
 * @brief 本接口用于停止socp源通道
 *
 * @par 描述:
 * 本接口用于停止socp源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 *
 * @retval 0:  停止源通道成功。
 * @retval 非0：停止源通道失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_stop(unsigned int src_chan_id);
static inline void SOCP_Stop1SrcChan(unsigned int u32SrcChanID)
{
    (void)mdrv_socp_stop(u32SrcChanID);
}

/**
 * @brief 本接口用于设置超时阈值
 *
 * @par 描述:
 * 本接口用于设置超时阈值
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  timeout_en 超时使能
 * @param[in]  time_out 超时阈值
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_SET_TIMEOUT (SOCP_TIMEOUT_EN_ENUM_UIN32 timeout_en, unsigned int time_out);

/**
 * @brief 本接口用于设置解码包长度极限值
 *
 * @par 描述:
 * 本接口用于设置解码包长度极限值
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  pkt_len 包长信息
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_SET_DEC_PKT_LGTH(SOCP_DEC_PKTLGTH_STRU *pkt_len);

/**
 * @brief 本接口用于设置编码源通道的debug模式
 *
 * @par 描述:
 * 本接口用于设置编码源通道的debug模式
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 * @param[in]  debug_en     debug使能
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_SET_DEBUG(unsigned int src_chan_id, unsigned int debug_en);

/**
 * @brief 本接口用于源通道软复位
 *
 * @par 描述:
 * 本接口用于源通道软复位
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 *
 * @retval 0:  软复位成功。
 * @retval 非0：软复位失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_CHAN_SOFT_RESET(unsigned int src_chan_id);

/**
 * @brief 本接口用于获取写数据buffer
 *
 * @par 描述:
 * 本接口用于获取写数据buffer
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 * @param[in]  pBuff  写入buffer大小
 *
 * @retval 0:  获取可用buffer成功。
 * @retval 非0：获取可用buffer失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_get_write_buff( unsigned int src_chan_id, SOCP_BUFFER_RW_STRU *pBuff);

/**
 * @brief 本接口用于表明写入socp源通道数据完成
 *
 * @par 描述:
 * 本接口用于表明写入socp源通道数据完成及大小
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id 源通道ID
 * @param[in]  write_size  写入buffer大小
 *
 * @retval 0:  获取可用buffer成功。
 * @retval 非0：获取可用buffer失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_write_done(unsigned int src_chan_id, unsigned int write_size);

/**
 * @brief 本接口用于注册RD缓冲区读数据的回调函数
 *
 * @par 描述:
 * 本接口用于注册RD缓冲区读数据的回调函数
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id  源通道ID
 * @param[out]  rd_read_cb      RD数据读取回调函数
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_REGISTER_RD_CB(unsigned int src_chan_id, socp_rd_cb rd_read_cb);

/**
 * @brief 本接口用于获取RD buffer的数据指针。。
 *
 * @par 描述:
 * 本接口用于获取RD buffer的数据指针。
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id  源通道ID
 * @param[out]  pBuff      rd buffer大小
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_get_rd_buffer( unsigned int src_chan_id,SOCP_BUFFER_RW_STRU *pBuff);

/**
 * @brief 本接口用于上层通知SOCP驱动，从RD buffer中实际读取的数据
 *
 * @par 描述:
 * 本接口用于上层通知SOCP驱动，从RD buffer中实际读取的数据。
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  src_chan_id  源通道ID
 * @param[out]  rd_szie      rd buffer大小
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_read_rd_done(unsigned int src_chan_id, unsigned int rd_szie);

/**
 * @brief 本接口用于注册读数据的回调函数
 *
 * @par 描述:
 * 本接口用于注册读数据的回调函数
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id  目的通道ID
 * @param[out]  read_cb      数据读取回调函数
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_REGISTER_READ_CB( unsigned int dst_chan_id, socp_read_cb read_cb);
#define BSP_SOCP_RegisterReadCB(u32DestChanID,ReadCB)  \
    DRV_SOCP_REGISTER_READ_CB(u32DestChanID,ReadCB)

/**
 * @brief 本接口用于获取读数据缓冲区指针。
 *
 * @par 描述:
 * 本接口用于获取读数据缓冲区指针。
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id  目的通道ID
 * @param[out]  pBuffer   读取大小
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_GET_READ_BUFF(unsigned int dst_chan_id,SOCP_BUFFER_RW_STRU *pBuffer);
#define BSP_SOCP_GetReadBuff(u32SrcChanID,pBuff)  \
        DRV_SOCP_GET_READ_BUFF(u32SrcChanID,pBuff)

/**
 * @brief 本接口用于上层告诉SOCP驱动，从目标通道中读走的实际数据。
 *
 * @par 描述:
 * 本接口用于上层告诉SOCP驱动，从目标通道中读走的实际数据。
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id  目的通道ID
 * @param[in]  read_size   读取大小
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_READ_DATA_DONE(unsigned int dst_chan_id,unsigned int read_size);
#define BSP_SOCP_ReadDataDone(u32DestChanID,u32ReadSize) \
        DRV_SOCP_READ_DATA_DONE(u32DestChanID,u32ReadSize)

/**
 * @brief 本接口用于设置BBP的SOCP源通道状态
 *
 * @par 描述:
 * 本接口用于设置BBP的SOCP源通道状态
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  enable   状态
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_SET_BBP_ENABLE(int enable);

/**
 * @brief 本接口用于设置BBP DS通道数据溢出处理模式
 *
 * @par 描述:
 * 本接口用于设置BBP DS通道数据溢出处理模式
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  ds_mode   DS模式
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_SET_BBP_DS_MODE(SOCP_BBP_DS_MODE_ENUM_UIN32 ds_mode);

/**
 * @brief 本接口用于使能dsp 的socp源通道
 *
 * @par 描述:
 * 本接口用于使能dsp 的socp源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void  DRV_SOCP_DSPCHN_START(void);

/**
 * @brief 本接口用于去使能dsp 的socp源通道
 *
 * @par 描述:
 * 本接口用于去使能dsp 的socp源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void  DRV_SOCP_DSPCHN_STOP(void);

/**
 * @brief 本接口用于获取socp状态
 *
 * @par 描述:
 * 本接口用于获取socp状态
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval SOCP_IDLE:  空闲
 * @retval SOCP_BUSY： 忙碌
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
SOCP_STATE_ENUM_UINT32  DRV_SOCP_GET_STATE(void);

/**
 * @brief 本接口用于备份BBPDMA寄存器
 *
 * @par 描述:
 * 本接口用于备份BBPDMA寄存器
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 0:  备份成功。
 * @retval 非0：备份失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int  DRV_BBPDMA_DRX_BAK_REG(void);

/**
 * @brief 本接口用于恢复BBPDMA寄存器
 *
 * @par 描述:
 * 本接口用于恢复BBPDMA寄存器
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 0:  恢复成功。
 * @retval 非0：恢复失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int  DRV_BBPDMA_DRX_RESTORE_REG(void);
/**
 * @brief 本接口用于设置ind目的端上报模式
 *
 * @par 描述:
 * 本接口用于设置ind目的端上报模式
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[out]  mode   上报模式
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_set_ind_mode(SOCP_IND_MODE_ENUM mode);
/**
 * @brief 本接口用于查询刷新后LOG2.0 SOCP超时配置信息
 *
 * @par 描述:
 * 本接口用于查询刷新后LOG2.0 SOCP超时配置信息
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[out]  cfg   log配置的信息
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
unsigned int mdrv_socp_get_sd_logcfg(SOCP_ENC_DST_BUF_LOG_CFG_STRU* cfg);
/**
 * @brief 本接口用于设置LTE DSP的SOCP源通道
 *
 * @par 描述:
 * 本接口用于设置LTE DSP的SOCP源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  chan_id   源通道ID
 * @param[in]  phy_addr   buffer物理地址
 * @param[in]  size   buffer大小
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
unsigned int  DRV_SOCP_INIT_LTE_DSP(unsigned int chan_id,unsigned int phy_addr,unsigned int size);
/**
 * @brief 本接口用于设置BBP LOG的SOCP源通道
 *
 * @par 描述:
 * 本接口用于设置BBP LOG的SOCP源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  chan_id   源通道ID
 * @param[in]  phy_addr   buffer物理地址
 * @param[in]  size   buffer大小
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
unsigned int  DRV_SOCP_INIT_LTE_BBP_LOG(unsigned int chan_id,unsigned int phy_addr,unsigned int size);
/**
 * @brief 本接口用于设置BBP DS的SOCP源通道
 *
 * @par 描述:
 * 本接口用于设置BBP DS的SOCP源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  chan_id   源通道ID
 * @param[in]  phy_addr   buffer物理地址
 * @param[in]  size   buffer大小
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
unsigned int  DRV_SOCP_INIT_LTE_BBP_DS(unsigned int chan_id,unsigned int phy_addr,unsigned int size);
/**
 * @brief 本接口用于设置BBP DSP的SOCP源通道
 *
 * @par 描述:
 * 本接口用于设置BBP DSP的SOCP源通道
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  chan_id   源通道ID
 * @param[in]  state   状态（0：去使能,1：使能）
 *
 * @retval 0:  设置成功。
 * @retval 非0：设置失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void DRV_SOCP_ENABLE_LTE_BBP_DSP(unsigned int chan_id, unsigned char state);
/**
 * @brief 本接口用于刷新SDLOG，待删除
 *
 * @par 描述:
 * 本接口用于刷新SDLOG，待删除
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  timer_len   定时器时长
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void BSP_SOCP_RefreshSDLogCfg(unsigned int timer_len);

/**
 * @brief 本接口用于使能SOCP的自动降频
 *
 * @par 描述:
 * 本接口用于使能SOCP的自动降频
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_socp_enalbe_dfs(void);

/**
 * @brief 本接口用于去使能SOCP的自动降频
 *
 * @par 描述:
 * 本接口用于去使能SOCP的自动降频
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_socp_disalbe_dfs(void);

/**
 * @brief 本接口用于SOCP投票
 *
 * @par 描述:
 * 本接口用于SOCP投票，根据投票结果决定SOCP是否睡眠，该接口只在A核提供
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  id   投票组件
 * @param[in]  type   投票类型
 *
 * @retval 0:  投票成功。
 * @retval 非0：投票失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_VOTE(SOCP_VOTE_ID_ENUM_U32 id, SOCP_VOTE_TYPE_ENUM_U32 type);

/**
 * @brief 本接口用于SOCP投票
 *
 * @par 描述:
 * 本接口用于SOCP投票，该接口只在C核提供，适配LDSP首次加载的SOCP上电需求
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  type   投票类型
 *
 * @retval 0:  投票成功。
 * @retval 非0：投票失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int DRV_SOCP_VOTE_TO_MCORE(SOCP_VOTE_TYPE_ENUM_U32 type);
/**
 * @brief 清除socp目的端中断信息
 *
 * @par 描述:
 * 清除socp目的端中断信息
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 无
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_clear_socp_buff(unsigned int u32SrcChanID);


#ifdef __cplusplus
}
#endif
#endif //__MDRV_SOCP_COMMON_H__
