/*
 *               (C) COPYRIGHT 2014 - 2016 SYNOPSYS, INC.
 *                         ALL RIGHTS RESERVED.
 *
 * This software and the associated documentation are confidential and
 * proprietary to Synopsys, Inc. Your use or disclosure of this software
 * is subject to the terms and conditions of a written license agreement
 * between you, or your company, and Synopsys, Inc.
 *
 * The entire notice above must be reproduced on all authorized copies.
 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 */

#ifndef HOST_LIB_DRIVER_LINUX_IF_H
#define HOST_LIB_DRIVER_LINUX_IF_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif
#include <linux/proc_fs.h>
#include "elliptic_std_def.h"
#include "elliptic_system_types.h"
#include "host_lib_driver_errors.h"

/**
 * \addtogroup HL_Driver
 * @{
 */
/** Host Library Driver instruct kernel driver to create dynamic memory. */
#define HL_DRIVER_ALLOCATE_DYNAMIC_MEM 0xffffffff
/** @}
 */

#define ESM_STATUS ELP_STATUS

/* IOCTL commands */
#define DPU_HDCP_IOCTL_MAGIC 'H'

/* 635+2+8 */
#define SHA1_MAX_LENGTH 650

/* ESM_HLD_IOCTL_LOAD_CODE */
typedef struct {
	uint8_t *code;
	uint32_t code_size;
	ESM_STATUS returned_status;
} esm_hld_ioctl_load_code;

/* ESM_HLD_IOCTL_GET_CODE_PHYS_ADDR */
typedef struct {
	uint32_t returned_phys_addr;
	ESM_STATUS returned_status;
} esm_hld_ioctl_get_code_phys_addr;

/* ESM_HLD_IOCTL_GET_DATA_PHYS_ADDR */
typedef struct {
	uint32_t returned_phys_addr;
	ESM_STATUS returned_status;
} esm_hld_ioctl_get_data_phys_addr;

/* ESM_HLD_IOCTL_GET_DATA_SIZE */
typedef struct {
	uint32_t returned_data_size;
	ESM_STATUS returned_status;
} esm_hld_ioctl_get_data_size;

/* ESM_HLD_IOCTL_HPI_READ */
typedef struct {
	uint32_t offset;
	uint32_t returned_data;
	ESM_STATUS returned_status;
} esm_hld_ioctl_hpi_read;

/* ESM_HLD_IOCTL_HPI_WRITE */
typedef struct {
	uint32_t offset;
	uint32_t data;
	ESM_STATUS returned_status;
} esm_hld_ioctl_hpi_write;

/* ESM_HLD_IOCTL_DATA_READ */
typedef struct {
	uint32_t offset;
	uint32_t nbytes;
	uint8_t *dest_buf;
	ESM_STATUS returned_status;
} esm_hld_ioctl_data_read;

/* ESM_HLD_IOCTL_DATA_WRITE */
typedef struct {
	uint32_t offset;
	uint32_t nbytes;
	uint8_t *src_buf;
	ESM_STATUS returned_status;
} esm_hld_ioctl_data_write;

/* ESM_HLD_IOCTL_DATA_SET */
typedef struct {
	uint32_t offset;
	uint32_t nbytes;
	uint8_t data;
	ESM_STATUS returned_status;
} esm_hld_ioctl_data_set;

/* ESM_HLD_IOCTL_ESM_OPEN */
typedef struct {
	uint32_t hpi_base;
	uint32_t code_base;
	uint32_t code_size;
	uint32_t data_base;
	uint32_t data_size;
	ESM_STATUS returned_status;
} esm_hld_ioctl_esm_open;

typedef struct {
	uint32_t type;
	uint32_t value;
	ESM_STATUS returned_status;
} esm_hld_ioctl_esm_start;

typedef struct {
	uint8_t type;
	uint8_t bcaps;
	uint32_t bksv_msb;
	uint32_t bksv_lsb;
	uint8_t sha1_buffer[SHA1_MAX_LENGTH];
	uint32_t buffer_length;
	uint8_t v_prime[20];
	ESM_STATUS returned_status;
} esm_hld_ioctl_get_te_info;

#define ESM_HLD_IOCTL_LOAD_CODE          _IOW(DPU_HDCP_IOCTL_MAGIC, 0x1, esm_hld_ioctl_load_code)          /* 1000 */
#define ESM_HLD_IOCTL_GET_CODE_PHYS_ADDR _IOW(DPU_HDCP_IOCTL_MAGIC, 0x2, esm_hld_ioctl_get_code_phys_addr) /* 1001 */
#define ESM_HLD_IOCTL_GET_DATA_PHYS_ADDR _IOW(DPU_HDCP_IOCTL_MAGIC, 0x3, esm_hld_ioctl_get_data_phys_addr) /* 1002 */
#define ESM_HLD_IOCTL_GET_DATA_SIZE      _IOW(DPU_HDCP_IOCTL_MAGIC, 0x4, esm_hld_ioctl_get_data_size)      /* 1003 */
#define ESM_HLD_IOCTL_HPI_READ           _IOW(DPU_HDCP_IOCTL_MAGIC, 0x5, esm_hld_ioctl_hpi_read)           /* 1004 */
#define ESM_HLD_IOCTL_HPI_WRITE          _IOW(DPU_HDCP_IOCTL_MAGIC, 0x6, esm_hld_ioctl_hpi_write)          /* 1005 */
#define ESM_HLD_IOCTL_DATA_READ          _IOW(DPU_HDCP_IOCTL_MAGIC, 0x7, esm_hld_ioctl_data_read)          /* 1006 */
#define ESM_HLD_IOCTL_DATA_WRITE         _IOW(DPU_HDCP_IOCTL_MAGIC, 0x8, esm_hld_ioctl_data_write)         /* 1007 */
#define ESM_HLD_IOCTL_DATA_SET           _IOW(DPU_HDCP_IOCTL_MAGIC, 0x9, esm_hld_ioctl_data_set)           /* 1008 */
#define ESM_HLD_IOCTL_ESM_OPEN           _IOW(DPU_HDCP_IOCTL_MAGIC, 0xa, esm_hld_ioctl_esm_open)           /* 1009 */

#define ESM_HLD_IOCTL_ESM_START          _IOW(DPU_HDCP_IOCTL_MAGIC, 0xb, esm_hld_ioctl_esm_start)
#define ESM_HLD_IOCTL_GET_TE_INFO        _IOW(DPU_HDCP_IOCTL_MAGIC, 0xc, esm_hld_ioctl_get_te_info)

long esm_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
void esm_end_device(void);
void esm_hld_init(void);
void esm_driver_enable(int en);

#endif /* HOST_LIB_DRIVER_LINUX_IF_H */

