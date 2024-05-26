/*
 *
 * NoC. (NoC Mntn Module.)
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DFX_NOC_DBG_H
#define __DFX_NOC_DBG_H

#include "dfx_noc.h"

struct noc_reg_dbg {
	char *p_reg_name;
	char *node_name;
	uint reg_len;		 /* reg length, 8bit/16bit/32bit */
	uint offset;
	uint rw_flag;		 /* bit0 rd, bit1 wr */
};
#define NOC_B_R              0x01
#define NOC_B_W              0x02
#define NOC_B_RW            (NOC_B_R | NOC_B_W)

#define NOC_CMD_CELL_SIZE    64
#define DBG_RET_SUCCESS      0x0
#define DBG_RET_FAIL         -1
#define DBG_INIT_VALUE       0x0

#define DBG_FILE_RIGHT       0660
#define DBG_REG_ARRAY_SIZE   0x4
#define NOC_DBG_CMD_SIZE     0x6
#define NOC_REG_LEN_32       32
#define NOC_DBG_REG_NAME     128
#define NOC_DBG_NOD_NAME     50


enum e_noc_dbg_cmd {
	E_NOC_DBG_CMD_RD_REG = 0,
	E_NOC_DBG_CMD_WR_REG,
	E_NOC_DBG_CMD_PKT_ENABLE,
	E_NOC_DBG_CMD_PKT_DISABLE,
	E_NOC_DBG_CMD_TRANS_ENABLE,
	E_NOC_DBG_CMD_TRANS_DISABLE,
	E_NOC_DBG_CMD_INVALID
};
struct noc_dbg_cmd_lookup {
	const char *str;
	enum e_noc_dbg_cmd cmd;
};
#ifdef CONFIG_DFX_DEBUG_FS
/*
 * Function: noc_dbg_creat_node
 * Description: noc debug function nodes creat
 * input: none
 * output: none
 * return: 0->success -1->failed
 */
extern int noc_dbg_creat_node(void);
#endif
#endif
