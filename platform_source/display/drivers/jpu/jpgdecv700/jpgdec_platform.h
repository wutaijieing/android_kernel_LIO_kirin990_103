/*
 * jpegdec paltform
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef JPGDEC_PLATFORM_H
#define JPGDEC_PLATFORM_H

#include <soc_jpgdec_interface.h>

/* define cvdr reg */
#define JPGDEC_CVDR_AXI_WR_CFG1             JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8
#define JPGDEC_CVDR_AXI_WR_CFG2             JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_7
#define JPGDEC_CVDR_AXI_LIMITER_RD_CFG1     JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_8
#define JPGDEC_CVDR_AXI_LIMITER_RD_CFG2     JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_7
#define JPGDEC_CVDR_AXI_RD_CFG1             JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8
#define JPGDEC_CVDR_AXI_RD_CFG2             JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_7

/* define cvdr reg val */
#define JPGDEC_CVDR_AXI_JPEG_CVDR_CFG_VAL   0x070f2000
#define AXI_JPEG_CVDR_NR_WR_CFG_VAL         0x80040000
#define AXI_JPEG_CVDR_NR_RD_CFG_VAL         0x80040080
#define JPGDEC_NRRD_ACCESS_LIMITER1_VAL     0xf002222
#define JPGDEC_NRRD_ACCESS_LIMITER2_VAL     0xf003333

/* define isr state bit */
#define DEC_DONE_ISR_BIT                    0
#define DEC_ERR_ISR_BIT                     1
#define DEC_OVERTIME_ISR_BIT                2

#define JPGDEC_IRQ_NUM                      1 /* 1 means using merged irq */

#endif /* JPGDEC_PLATFORM_H */