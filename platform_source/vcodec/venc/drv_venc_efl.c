/*
 * drv_venc_efl.c
 *
 * This is for venc drv.
 *
 * Copyright (c) 2009-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "drv_venc_efl.h"
#include "drv_mem.h"
#include "venc_regulator.h"

#define d_venc_check_cfg_reg_addr_ret(map_addr, cfg_reg_addr, map_buffer_size) \
	do { \
		if (((cfg_reg_addr) < (map_addr)) || ((cfg_reg_addr) > ((map_addr) + (map_buffer_size)))) \
			return VCODEC_FAILURE; \
	} while (0)

#define d_venc_check_cfg_reg_endaddr_ret(map_addr, cfg_reg_addr, map_buffer_size) \
	do { \
		if (((cfg_reg_addr) < ((map_addr) -1)) || ((cfg_reg_addr) > ((map_addr) + (map_buffer_size)))) \
			return VCODEC_FAILURE; \
	} while (0)

#define d_venc_check_cfg_reg_streamaddr_ret(map_addr, cfg_reg_addr, map_buffer_size, shared_fd) \
	do { \
		if ((((cfg_reg_addr) < (map_addr))  || ((cfg_reg_addr) > ((map_addr) + (map_buffer_size)))) && \
			(((shared_fd) != 0) && (((cfg_reg_addr) != 0) && ((cfg_reg_addr) != 0xffffffff)))) \
			return VCODEC_FAILURE; \
	} while (0)

#define d_venc_check_cfg_reg_streamendaddr_ret(map_addr, cfg_reg_addr, map_buffer_size, shared_fd) \
	do { \
		if ((((cfg_reg_addr) < ((map_addr) - 1))  || ((cfg_reg_addr) > ((map_addr) + (map_buffer_size)))) && \
			(((shared_fd) != 0) && (((cfg_reg_addr) != 0) && ((cfg_reg_addr) != 0xffffffff)))) \
			return VCODEC_FAILURE; \
	} while (0)

#define d_venc_check_smrx_reg_addr_ret(map_addr, cfg_reg_addr, map_buffer_size) \
	do { \
	if (((cfg_reg_addr) < ((map_addr) >> 15)) || ((cfg_reg_addr) > (((map_addr) + (map_buffer_size)) >> 15))) \
		return VCODEC_FAILURE; \
	} while (0)

#define d_venc_check_smrx_reg_endaddr_ret(map_addr, cfg_reg_addr, map_buffer_size) \
	do { \
		if (((cfg_reg_addr) < (((map_addr) - 1) >> 15)) || ((cfg_reg_addr) > (((map_addr) + (map_buffer_size)) >> 15))) \
			return VCODEC_FAILURE; \
	} while (0)

static struct venc_fifo_buffer *g_encode_done_info[MAX_OPEN_COUNT];

static int32_t wait_buffer_filled(struct venc_fifo_buffer *buffer)
{
	int32_t ret;
	uint32_t timeout = venc_regulator_is_fpga() ? FPGA_WAIT_EVENT_TIME_MS : ENCODE_DONE_TIMEOUT_MS;

	ret = OSAL_WAIT_EVENT_TIMEOUT(&buffer->event, !queue_is_empty(buffer), timeout); /*lint !e578 !e666*/
	if (ret != 0)
		VCODEC_ERR_VENC("wait buffer filled timeout");

	return ret;
}

static int32_t get_mem_info_by_fd(int32_t fd, mem_buffer_t *buffer)
{
	int32_t ret;

	if (fd < 0) {
		VCODEC_FATAL_VENC("invalid param, share fd is NULL");
		return VCODEC_FAILURE;
	}

	ret = drv_mem_get_map_info(fd, buffer);
	if (ret) {
		VCODEC_FATAL_VENC("share fd map failed");
		return VCODEC_FAILURE;
	}

	drv_mem_put_map_info(buffer);

	return 0;
}

static int32_t get_mem_info(venc_fd_info_t *fd_info, venc_buffer_info_t *mem_info)
{
	int32_t ret;
	int32_t i;

	if (!mem_info) {
		VCODEC_FATAL_VENC("pMemMap parameter error");
		return VCODEC_FAILURE;
	}

	ret = get_mem_info_by_fd(fd_info->interal_shared_fd, &mem_info->internal_buffer);
	if (ret) {
		VCODEC_FATAL_VENC("Interal buffer map failed");
		return VCODEC_FAILURE;
	}

	ret = get_mem_info_by_fd(fd_info->image_shared_fd, &mem_info->image_buffer);
	if (ret) {
		VCODEC_FATAL_VENC("Image buffer map failed");
		return VCODEC_FAILURE;
	}

	ret = get_mem_info_by_fd(fd_info->stream_head_shared_fd, &mem_info->stream_head_buffer);
	if (ret) {
		VCODEC_FATAL_VENC("streamhead buffer map failed");
		return VCODEC_FAILURE;
	}

	for (i = 0; i < MAX_SLICE_NUM; i++) {
		if (fd_info->stream_shared_fd[i] == 0) {
			continue;
		}

		ret = get_mem_info_by_fd(fd_info->stream_shared_fd[i], &mem_info->stream_buffer[i]);
		if (ret) {
			VCODEC_FATAL_VENC("Stream buffer map failed");
			return VCODEC_FAILURE;
		}
	}

	return 0;
}

static int32_t check_internal_buffer_addr(const S_HEVC_AVC_REGS_TYPE_CFG *reg, const mem_buffer_t *buffer)
{
#ifndef VCODECV500
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_YADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_CADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_YH_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_CH_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFY_L0_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFC_L0_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFYH_L0_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFCH_L0_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMELD_L0_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFY_L1_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFC_L1_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFYH_L1_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFCH_L1_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMELD_L1_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEST_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_NBI_MVST_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_NBI_MVLD_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEINFO_ST_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEINFO_LD0_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEINFO_LD1_ADDR, buffer->size);

#ifndef VCODEC_PLATFORM_ECONOMIC
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_QPGLD_INF_ADDR, buffer->size);
#endif

#ifndef VCODECV320
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_NBI_MVST_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_NBI_MVST_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_NBI_MVLD_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_NBI_MVLD_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEST_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEST_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMELD_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMELD_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEINFOST_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEINFOST_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEINFOLD0_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEINFOLD0_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEINFOLD1_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_PMEINFOLD1_ADDR_END, buffer->size);
#ifndef VCODEC_PLATFORM_ECONOMIC
	/* QPMAP register, not used in v210 v310 */
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_QPGLD_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_QPGLD_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_YH_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_YH_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_CH_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_CH_ADDR_END, buffer->size);
#endif
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_YADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_YADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_CADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REC_CADDR_END, buffer->size);
#ifndef VCODEC_PLATFORM_ECONOMIC
	/* QPMAP register, not used in v210 v310 */
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_YH_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_YH_ADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_CH_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_CH_ADDR_END, buffer->size);
#endif
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_YADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_YADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_CADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_REF_CADDR_END, buffer->size);
#ifdef VCODEC_PLATFORM_ECONOMIC
	/* secure in v200, not check */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[12].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[12].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[13].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[13].bits.va_end, buffer->size);
	/* image addr when not in v210 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[15].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[15].bits.va_end, buffer->size);
#endif
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[16].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[16].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[17].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[17].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[18].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[18].bits.va_end, buffer->size);
#ifndef VCODEC_PLATFORM_ECONOMIC
	/* image addr in v200 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[19].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[19].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[20].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[20].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[21].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[21].bits.va_end, buffer->size);
	/* secure in v210, not check */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[22].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[22].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[23].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[23].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[24].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[24].bits.va_end, buffer->size);
#endif
#ifdef VCODEC_PLATFORM_ECONOMIC
	/* internal addr in v210 v310 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[29].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[29].bits.va_end, buffer->size);
	/* secure when not in v210 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[30].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[30].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[31].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[31].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[32].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[32].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[33].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[33].bits.va_end, buffer->size);
#endif
#ifndef VCODEC_PLATFORM_ECONOMIC
	/* value not setted in v210 v310 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[39].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[39].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[40].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[40].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[41].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[41].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[42].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[42].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[43].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[43].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[44].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[44].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[45].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[45].bits.va_end, buffer->size);
#endif
#endif
#else
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_YADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_CADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_YH_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REC_CH_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFY_L0_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFC_L0_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFYH_L0_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFCH_L0_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMELD_L0_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFY_L1_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFC_L1_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFYH_L1_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_REFCH_L1_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMELD_L1_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEST_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_NBI_MVST_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_NBI_MVLD_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEINFO_ST_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEINFO_LD0_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_PMEINFO_LD1_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_QPGLD_INF_ADDR_L, buffer->size);

#endif

	return 0;
}

static int32_t check_image_buffer_addr(const S_HEVC_AVC_REGS_TYPE_CFG *reg, const mem_buffer_t *buffer)
{
#ifndef VCODECV500
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_TUNLCELL_ADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_SRC_YADDR, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_SRC_CADDR, buffer->size);

#ifndef VCODECV320
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_YADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_YADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_CADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_CADDR_END, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_LOWDLY_ADDR_STR, buffer->size);

	d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_LOWDLY_ADDR_END, buffer->size);
#ifdef VCODEC_PLATFORM_ECONOMIC
	/* secure in v200 v310 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[11].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[11].bits.va_end, buffer->size);
#endif
#ifndef VCODEC_PLATFORM_ECONOMIC
	/* internal addr in v210 v310 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[15].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[15].bits.va_end, buffer->size);
	/* secure in v210 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[27].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[27].bits.va_end, buffer->size);
	/* value not setted in v210 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[28].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[28].bits.va_end, buffer->size);
#endif
#ifdef VCODEC_PLATFORM_ECONOMIC
	/* internal addr in v200, image addr in v210 v310 */
	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[19].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[19].bits.va_end, buffer->size);

	d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[20].bits.va_str, buffer->size);

	d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[20].bits.va_end, buffer->size);
#endif
#endif
	if (reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV420_PLANAR ||
		reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV422_PLANAR) {
		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_SRC_VADDR, buffer->size);

#ifndef VCODECV320
		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_VADDR_STR, buffer->size);

		d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_VADDR_END, buffer->size);
#ifndef VCODEC_PLATFORM_ECONOMIC
		/* internal addr in v210 v310 */
		d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[29].bits.va_str, buffer->size);

		d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[29].bits.va_end, buffer->size);
#endif
#ifdef VCODEC_PLATFORM_ECONOMIC
		/* VEDU_SRC_VADDR, need in this condition */
		d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[21].bits.va_str, buffer->size);

		d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[21].bits.va_end, buffer->size);
#endif
#endif
	}

	if (reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV420_SEMIPLANAR_CMP) {
		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_YH_ADDR, buffer->size);

		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_CH_ADDR, buffer->size);

#ifndef VCODECV320
		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_YHADDR_STR, buffer->size);

		d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_YHADDR_END, buffer->size);

		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_CHADDR_STR, buffer->size);

		d_venc_check_cfg_reg_endaddr_ret(buffer->start_phys_addr, reg->MMU_PRE_SRC_CHADDR_END, buffer->size);

		d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[25].bits.va_str, buffer->size);

		d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[25].bits.va_end, buffer->size);

		d_venc_check_smrx_reg_addr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_1[26].bits.va_str, buffer->size);

		d_venc_check_smrx_reg_endaddr_ret(buffer->start_phys_addr, reg->SMMU_MSTR_SMRX_2[26].bits.va_end, buffer->size);
#endif
	}
#else
	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_TUNLCELL_ADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_SRC_YADDR_L, buffer->size);

	d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_SRC_CADDR_L, buffer->size);

	if (reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV420_PLANAR ||
		reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV422_PLANAR) {
		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_SRC_VADDR_L, buffer->size);
	}

	if (reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV420_SEMIPLANAR_CMP) {
		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_YH_ADDR_L, buffer->size);

		d_venc_check_cfg_reg_addr_ret(buffer->start_phys_addr, reg->VEDU_VCPI_CH_ADDR_L, buffer->size);
	}
#endif
	return 0;
}

static int32_t check_image_stream_addr(const S_HEVC_AVC_REGS_TYPE_CFG *reg,
		const mem_buffer_t *stream_buffer, const mem_buffer_t *head_buffer)
{
#ifdef VCODECV700
	uint32_t i;
	uint64_t start_addr, end_addr;

	/* according to <VcodecV700 IT Environment Constrain> 4.1 */
	for (i = 0; i < MAX_SLICE_NUM; i++) {
		start_addr = (((uint64_t)reg->VEDU_VCPI_STRMADDR_H_NEW[i] << 32) & 0xFFFFFFFF00000000) |
			(uint64_t)reg->VEDU_VCPI_STRMADDR_L_NEW[i];
		end_addr = start_addr + (uint64_t)reg->VEDU_VCPI_STRMBUFLEN_NEW[i];

		d_venc_check_cfg_reg_streamaddr_ret(stream_buffer[i].start_phys_addr,
				start_addr, stream_buffer[i].size, stream_buffer[i].share_fd);

		d_venc_check_cfg_reg_streamendaddr_ret(stream_buffer[i].start_phys_addr,
				end_addr, stream_buffer[i].size, stream_buffer[i].share_fd);
	}
#elif defined(VCODECV320)
	uint32_t i;
	uint32_t *start_addr = (uint32_t *)&reg->VEDU_VLCST_STRMADDR0;

	for (i = 0; i < MAX_SLICE_NUM; i++) {
		d_venc_check_cfg_reg_streamaddr_ret(stream_buffer[i].start_phys_addr,
				start_addr[i], stream_buffer[i].size, stream_buffer[i].share_fd);
	}
#elif !defined(VCODECV500)
	uint32_t i;
	uint32_t *start_addr = &reg->MMU_PRE_STRMADDR0_STR;
	uint32_t *end_addr = &reg->MMU_PRE_STRMADDR0_END;

	if (reg->VEDU_VCPI_STRFMT.bits.vcpi_str_fmt == YUV420_SEMIPLANAR_CMP) {
		d_venc_check_cfg_reg_streamaddr_ret(head_buffer->start_phys_addr,
				reg->MMU_PRE_PPS_ADDR_STR, head_buffer->size, head_buffer->share_fd);
		d_venc_check_cfg_reg_streamendaddr_ret(head_buffer->start_phys_addr,
				reg->MMU_PRE_PPS_ADDR_END, head_buffer->size, head_buffer->share_fd);
	}

	for (i = 0; i < MAX_SLICE_NUM; i++) {
		d_venc_check_cfg_reg_streamaddr_ret(stream_buffer[i].start_phys_addr,
				start_addr[i * 2], stream_buffer[i].size, stream_buffer[i].share_fd);

		d_venc_check_cfg_reg_streamendaddr_ret(stream_buffer[i].start_phys_addr,
				end_addr[i * 2], stream_buffer[i].size, stream_buffer[i].share_fd);
	}

	start_addr = &reg->VEDU_VLCST_STRMADDR0;
	for (i = 0; i < MAX_SLICE_NUM; i++) {
		d_venc_check_cfg_reg_streamaddr_ret(stream_buffer[i].start_phys_addr,
				start_addr[i], stream_buffer[i].size, stream_buffer[i].share_fd);
	}
#else

	d_venc_check_cfg_reg_addr_ret(stream_buffer->start_phys_addr, reg->VEDU_VCPI_SWPTRADDR_L, stream_buffer->size);

	d_venc_check_cfg_reg_addr_ret(stream_buffer->start_phys_addr, reg->VEDU_VCPI_SRPTRADDR_L, stream_buffer->size);

	d_venc_check_cfg_reg_addr_ret(stream_buffer->start_phys_addr, reg->VEDU_VCPI_STRMADDR_L, stream_buffer->size);

#endif
	return 0;
}

static int32_t check_buffer_addr(const S_HEVC_AVC_REGS_TYPE_CFG *reg, venc_fd_info_t *fd_info)
{
	int32_t ret;
	venc_buffer_info_t mem_info;

	(void)memset_s((void *)&mem_info, sizeof(mem_info), 0, sizeof(mem_info));

	ret = get_mem_info(fd_info, &mem_info);
	if (ret) {
		VCODEC_FATAL_VENC("mem shared error");
		return VCODEC_FAILURE;
	}

	ret = check_internal_buffer_addr(reg, &mem_info.internal_buffer);
	if (ret) {
		VCODEC_FATAL_VENC("check internal buffer addr failed");
		return VCODEC_FAILURE;
	}

	ret = check_image_buffer_addr(reg, &mem_info.image_buffer);
	if (ret) {
		VCODEC_FATAL_VENC("check image buffer addr failed");
		return VCODEC_FAILURE;
	}

	ret = check_image_stream_addr(reg, &mem_info.stream_buffer[0], &mem_info.stream_head_buffer);
	if (ret) {
		VCODEC_FATAL_VENC("check stream buffer addr failed");
		return VCODEC_FAILURE;
	}

	return 0;
}

int32_t venc_drv_create_queue(void)
{
	int32_t i;
	int32_t j;

	for (i = 0; i < MAX_OPEN_COUNT; i++) {
		g_encode_done_info[i] = create_queue(struct venc_fifo_buffer);
		if (g_encode_done_info[i] == NULL)
			break;
	}

	if (i == MAX_OPEN_COUNT)
		return 0;

	for (j = 0; j < i; j++) {
		destory_queue(g_encode_done_info[j]);
		g_encode_done_info[j] = NULL;
	}

	VCODEC_FATAL_VENC("create queue failed");
	return VCODEC_FAILURE;
}

void venc_drv_destroy_queue(void)
{
	int32_t i;

	for (i = 0; i < MAX_OPEN_COUNT; i++) {
		if (g_encode_done_info[i] != NULL) {
			destory_queue(g_encode_done_info[i]);
			g_encode_done_info[i] = NULL;
		}
	}
}

int32_t venc_drv_alloc_encode_done_info_buffer(struct file *file)
{
	int32_t i;
	int32_t ret;

	if (file == NULL) {
		VCODEC_ERR_VENC("input file is NULL");
		return VCODEC_FAILURE;
	}

	for (i = 0; i < MAX_OPEN_COUNT; i++) {
		if (queue_is_null(g_encode_done_info[i]))
			break;
	}

	if (i == MAX_OPEN_COUNT) {
		VCODEC_FATAL_VENC("failed to find idle kfifo");
		return VCODEC_FAILURE;
	}

	ret = alloc_queue(g_encode_done_info[i], MAX_RING_BUFFER_SIZE);
	if (ret != 0) {
		VCODEC_FATAL_VENC("failed to allocate kfifo");
		return VCODEC_FAILURE;
	}

	file->private_data = g_encode_done_info[i];

	return 0;
}

int32_t venc_drv_free_encode_done_info_buffer(const struct file *file)
{
	int32_t i;

	if (file == NULL) {
		VCODEC_ERR_VENC("input file is NULL");
		return VCODEC_FAILURE;
	}

	for (i = 0; i < MAX_OPEN_COUNT; i++) {
		if (file->private_data == g_encode_done_info[i]) {
			free_queue(g_encode_done_info[i]);
			return 0;
		}
	}

	VCODEC_ERR_VENC("not find buffer info, free buffer fail");

	return VCODEC_FAILURE;
}

int32_t venc_drv_get_encode_done_info(struct venc_fifo_buffer *buffer, struct encode_done_info *encode_done_info)
{
	int32_t ret;

	if (buffer == NULL || encode_done_info == NULL) {
		VCODEC_ERR_VENC("input is invalid");
		return VCODEC_FAILURE;
	}

	ret = wait_buffer_filled(buffer);
	if (ret != 0) {
		VCODEC_ERR_VENC("wait buffer filled failed");
		return VCODEC_FAILURE;
	}

	ret = pop(buffer, encode_done_info);
	if (ret != 0) {
		VCODEC_ERR_VENC("read encode done info failed");
		return VCODEC_FAILURE;
	}

	return 0;
}

int32_t venc_drv_encode(struct encode_info *encode_info, struct venc_fifo_buffer *buffer)
{
	int32_t ret;
	uint32_t i;
	uint32_t core_num = venc_get_core_num();
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	if (encode_info == NULL || buffer == NULL) {
		VCODEC_FATAL_VENC("input is invalid");
		return VCODEC_FAILURE;
	}

	if (encode_info->clock_info.core_num <= 0 ||
		encode_info->clock_info.core_num > core_num ||
		encode_info->clock_info.clock_type >= VENC_CLK_BUTT) {
		VCODEC_FATAL_VENC("check clock info param is invalid");
		return VCODEC_FAILURE;
	}

	ret = check_buffer_addr(&encode_info->all_reg, &encode_info->mem_info);
	if (ret) {
		VCODEC_FATAL_VENC("check buffer addr error");
		return VCODEC_FAILURE;
	}

	for (i = 0; i < MAX_SUPPORT_CORE_NUM && i < core_num; i++)
		pm_update_account(&venc->ctx[i].pm);

	return venc->ops.encode(encode_info, buffer);
}

irqreturn_t venc_drv_encode_done(int32_t irq, void *dev_id)
{
	int32_t i = 0;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	VCODEC_DBG_VENC("enter %s ()", __func__);

	for (i = 0; i < MAX_SUPPORT_CORE_NUM; i++) {
		if (venc->ctx[i].irq_num.normal == (uint32_t)irq)
			break;
	}

	if (i == MAX_SUPPORT_CORE_NUM) {
		VCODEC_FATAL_VENC("isr not register");
		return IRQ_HANDLED;
	}

	if (venc->ctx[i].status != VENC_BUSY)
		VCODEC_FATAL_VENC("cord %d: isr invalid core status, status is %d",
				i, venc->ctx[i].status);

	venc->ops.encode_done(&venc->ctx[i]);
	VCODEC_DBG_VENC("out %s ()", __func__);
	return IRQ_HANDLED;
}

int32_t venc_drv_open_vedu(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	int32_t ret;

	VCODEC_DBG_VENC("enter %s()", __func__);

	(void)memset_s(&venc->backup_info.info, sizeof(struct clock_info), 0, sizeof(struct clock_info));
	ret = venc->ops.init();

	VCODEC_DBG_VENC("exit %s()", __func__);
	return ret;
}

int32_t venc_drv_close_vedu(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	VCODEC_DBG_VENC("enter %s()", __func__);

	venc_regulator_wait_hardware_idle(&venc->event);
	venc->ops.deinit();

	VCODEC_DBG_VENC("exit %s()", __func__);

	return 0;
}

int32_t venc_drv_suspend_vedu(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	VCODEC_INFO_VENC("enter %s()", __func__);

	venc_regulator_wait_hardware_idle(&venc->event);

	VCODEC_INFO_VENC("exit %s()", __func__);

	return 0;
}

int32_t venc_drv_resume_vedu(void)
{
	VCODEC_INFO_VENC("enter %s()", __func__);
	VCODEC_INFO_VENC("exit %s()", __func__);
	return 0;
}

inline int32_t venc_check_coreid(int32_t core_id)
{
	if (core_id < VENC_CORE_0 || (uint32_t)core_id >= venc_get_core_num())
		return VCODEC_FAILURE;

	return 0;
}

void venc_drv_init_pm(void)
{
	uint32_t i;
	uint32_t core_num = venc_get_core_num();
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

#ifdef VENC_DEBUG_ENABLE
	if (venc->debug_flag & (1LL << LOW_POWER_DISABLE))
		return;
#endif

#ifdef SUPPORT_SWITCH_POWER_OFF_PER_FRAME
	if (venc->popf_switch)
		return;
#endif

	if (!is_support_power_control_per_frame())
		return;

	for (i = 0; i < MAX_SUPPORT_CORE_NUM && i < core_num; i++)
		pm_init_account(&venc->ctx[i].pm);
}

void venc_drv_deinit_pm(void)
{
	uint32_t i;
	uint32_t core_num = venc_get_core_num();
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	for (i = 0; i < MAX_SUPPORT_CORE_NUM && i < core_num; i++)
		pm_deinit_account(&venc->ctx[i].pm);
}

void venc_drv_print_pm_info(void)
{
	char buf[PM_INFO_BUFFER_SIZE];
	int32_t ret;
	uint32_t i;
	uint32_t core_num = venc_get_core_num();
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	for (i = 0; i < MAX_SUPPORT_CORE_NUM && i < core_num; i++) {
		ret = pm_show_total_account(&venc->ctx[i].pm, buf, sizeof(buf));
		if (ret > 0)
			VCODEC_INFO_VENC("core id %d: %s", i, buf);
	}
}
