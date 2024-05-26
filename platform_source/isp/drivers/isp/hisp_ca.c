/*
 * TEEC for secisp
 *
 * Copyright (c) 2019 ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include <securec.h>
#include <dynamic_mem.h>
#include <hisp_internel.h>

/* info */
#define hisp_ca_err(fmt, ...)     (printk(KERN_ERR "[sec]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define hisp_ca_info(fmt, ...)    (printk(KERN_INFO "[sec]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define hisp_ca_dbg(fmt, ...)     (printk(KERN_DEBUG "[sec]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
/* CA info */
#define TEE_SERVICE_SECISP_ID           TEE_SERVICE_SECISP
#define TEE_SECISP_PACKAGE_NAME         "/vendor/bin/CameraDaemon"
#define TEE_SECISP_UID                  1013

enum secisp_mem_type {
	SECISP_TEXT = 0,
	SECISP_DATA,
	SECISP_SEC_POOL,
	SECISP_ISPSEC_POOL,
	SECISP_DYNAMIC_POOL,
	SECISP_RDR,
	SECISP_SHRD,
	SECISP_VQ,
	SECISP_VR0,
	SECISP_VR1,
	SECISP_MAX_TYPE
};

/* CA cmd */
enum hisp_ca_cmd_id {
	TEE_SECISP_CMD_IMG_DISRESET        = 0,
	TEE_SECISP_CMD_RESET               = 1,
	TEE_SECISP_SEC_MEM_CFG_AND_MAP     = 2,
	TEE_SECISP_SEC_MEM_CFG_AND_UNMAP   = 3,
	TEE_SECISP_NONSEC_MEM_MAP_SEC      = 4,
	TEE_SECISP_NONSEC_MEM_UNMAP_SEC    = 5,
	TEE_SECISP_BOOT_MEM_CFG_AND_MAP    = 6,
	TEE_SECISP_BOOT_MEM_CFG_AND_UNMAP  = 7,
	TEE_SECISP_IMG_MEM_CFG_AND_MAP     = 8,
	TEE_SECISP_IMG_MEM_CFG_AND_UNMAP   = 9,
	TEE_SECISP_RSV_MEM_CFG_AND_MAP     = 10,
	TEE_SECISP_RSV_MEM_CFG_AND_UNMAP   = 11,
	TEE_SECISP_CMD_MAX
};

struct hisp_ca_imgmem_info {
	struct hisp_ca_meminfo info;
	unsigned int sfd;
};

struct hisp_ca_img_cfginfo {
	struct hisp_ca_imgmem_info img_info[HISP_SEC_BOOT_MAX_TYPE];
};

struct hisp_cs_rsv_cfginfo {
	struct hisp_ca_meminfo rsv_info[HISP_SEC_RSV_MAX_TYPE];
};

struct hisp_ca_dynmem_info {
	struct dma_buf *dmabuf;
	struct sg_table *table;
	struct dma_buf_attachment *attach;
	unsigned int nents;
};

/* teec info */
static TEEC_Session g_secisp_session;
static TEEC_Context g_secisp_context;
static int g_secisp_creat_teec;
static struct mutex g_secisp_tee_lock;
static struct hisp_ca_dynmem_info g_info;
static struct sglist *g_sgl;
static struct hisp_ca_img_cfginfo *g_img_buf;
static struct hisp_cs_rsv_cfginfo *g_rsv_buf;
static u64 g_pool_sfd[SECISP_MAX_TYPE];

static int hisp_ca_get_sgl_info(int fd, struct device *dev,
	struct scatterlist **sgl, struct hisp_ca_dynmem_info *info)
{
	info->dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(info->dmabuf)) {
		hisp_ca_err("dma_buf_get fail, fd=%d, dma_buf=%pK\n",
			fd, info->dmabuf);
		return -EINVAL;
	}

	info->attach = dma_buf_attach(info->dmabuf, dev);
	if (IS_ERR_OR_NULL(info->attach)) {
		hisp_ca_err("dma_buf_attach error, attach=%pK\n", info->attach);
		dma_buf_put(info->dmabuf);
		return -EINVAL;
	}

	info->table = dma_buf_map_attachment(info->attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(info->table)) {
		hisp_ca_err("dma_buf_map_attachment error\n");
		dma_buf_detach(info->dmabuf, info->attach);
		dma_buf_put(info->dmabuf);
		return -EINVAL;
	}

	if (info->table->sgl == NULL) {
		hisp_ca_err("Failed : sgl.NULL\n");
		dma_buf_unmap_attachment(info->attach, info->table,
			DMA_BIDIRECTIONAL);
		dma_buf_detach(info->dmabuf, info->attach);
		dma_buf_put(info->dmabuf);
		return -EINVAL;
	}

	*sgl = info->table->sgl;
	info->nents = info->table->nents;

	return 0;
}

static void hisp_ca_sgl_info_put(struct hisp_ca_dynmem_info *info)
{
	if (info == NULL) {
		hisp_ca_err("Failed : info.NULL\n");
		return;
	}

	dma_buf_unmap_attachment(info->attach, info->table, DMA_BIDIRECTIONAL);
	dma_buf_detach(info->dmabuf, info->attach);
	dma_buf_put(info->dmabuf);
}

static int hisp_ca_get_ionmem_info(int fd, struct sglist **sglist,
		struct device *dev)
{
	struct scatterlist *sg = NULL;
	struct scatterlist *sgl = NULL;
	struct sglist *tmp_sgl = NULL;
	unsigned int sgl_size;
	unsigned int i = 0;
	int ret;

	ret = hisp_ca_get_sgl_info(fd, dev, &sgl, &g_info);
	if (ret) {
		hisp_ca_err("Failed : hisp_ca_get_sgl_info\n");
		return -EFAULT;
	}

	sgl_size = sizeof(struct sglist) +
		g_info.nents * sizeof(struct ion_page_info);
	tmp_sgl = vmalloc(sgl_size);
	if (tmp_sgl == NULL) {
		hisp_ca_err("alloc tmp_sgl mem failed!\n");
		hisp_ca_sgl_info_put(&g_info);
		return -ENOMEM;
	}

	tmp_sgl->sglist_size = sgl_size;
	tmp_sgl->info_length = g_info.nents;

	for_each_sg(sgl, sg, tmp_sgl->info_length, i) {
		tmp_sgl->page_info[i].phys_addr = page_to_phys(sg_page(sg));
		tmp_sgl->page_info[i].npages = sg->length / PAGE_SIZE;
	}

	*sglist = tmp_sgl;

	return 0;
}

static int hisp_ca_rsvmem_cfg(
	struct hisp_cs_rsv_cfginfo *buffer,
	struct hisp_ca_meminfo *info, int info_size)
{
	int i, ret;

	if (info_size != HISP_SEC_RSV_MAX_TYPE) {
		hisp_ca_err("rsv meminfo size fail.%d\n", info_size);
		return -EINVAL;
	}

	for (i = 0; i < info_size; i++) {
		ret = memcpy_s(&buffer->rsv_info[i],
			sizeof(struct hisp_ca_meminfo),
			&info[i], sizeof(struct hisp_ca_meminfo));
		if (ret != 0) {
			hisp_ca_err("memcpy_s rsv memory fail, index.%d!\n", i);
			return ret;
		}
	}

	return 0;
}

static int hisp_ca_imgmem_cfg(
	struct hisp_ca_img_cfginfo *buffer,
	struct hisp_ca_meminfo *info, int info_size)
{
	struct sg_table *table = NULL;
	enum SEC_SVC type = 0;
	u64 sfd = 0;
	int i, ret;

	if (info_size != HISP_SEC_BOOT_MAX_TYPE) {
		hisp_ca_err("boot meminfo size fail.%d\n", info_size);
		return -EINVAL;
	}

	for (i = 0; i < info_size; i++) {
		ret = memcpy_s(&buffer->img_info[i].info,
			sizeof(struct hisp_ca_meminfo),
			&info[i], sizeof(struct hisp_ca_meminfo));
		if (ret != 0) {
			hisp_ca_err("memcpy_s boot memory fail, index.%d!\n", i);
			return ret;
		}

		ret = secmem_get_buffer(buffer->img_info[i].info.sharefd,
			&table, &sfd, &type);
		if (ret != 0) {
			hisp_ca_err("secmem_get_buffer fail. ret.%d!\n", ret);
			return -EFAULT;
		}
		hisp_ca_info("i = %d, sfd = %llu", i, sfd);
		buffer->img_info[i].sfd = (unsigned int)sfd;
	}

	return 0;
}

/*
 * Function name:hisp_ca_ta_open.
 * Discription:Init the TEEC and get the context
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ context: context.
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_ta_open(void)
{
	TEEC_Result result;
	TEEC_UUID svc_uuid = TEE_SERVICE_SECISP_ID;/* Tzdriver id for secisp */
	TEEC_Operation operation = {0};
	char package_name[] = TEE_SECISP_PACKAGE_NAME;
	u32 root_id = TEE_SECISP_UID;
	int ret = 0;

	mutex_lock(&g_secisp_tee_lock);
	result = TEEK_InitializeContext(NULL, &g_secisp_context);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InitializeContext failed.0x%x!\n", result);
		ret = -EPERM;
		goto error;
	}

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_NONE,
					TEEC_NONE,
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_MEMREF_TEMP_INPUT);

	operation.params[2].tmpref.buffer = (void *)(&root_id);
	operation.params[2].tmpref.size   = sizeof(root_id);
	operation.params[3].tmpref.buffer = (void *)(package_name);
	operation.params[3].tmpref.size   = strlen(package_name) + 1;

	result = TEEK_OpenSession(
			&g_secisp_context,
			&g_secisp_session,
			&svc_uuid,
			TEEC_LOGIN_IDENTIFY,
			NULL,
			&operation,
			NULL);

	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_OpenSession failed, result=0x%x!\n", result);
		ret = -EPERM;
		TEEK_FinalizeContext(&g_secisp_context);
	}
	g_secisp_creat_teec = 1;
error:
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_ta_close
 * Discription: close secisp ta
 * Parameters:
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_ta_close(void)
{
	int ret = 0;

	mutex_lock(&g_secisp_tee_lock);
	TEEK_CloseSession(&g_secisp_session);
	TEEK_FinalizeContext(&g_secisp_context);
	g_secisp_creat_teec = 0;
	mutex_unlock(&g_secisp_tee_lock);

	return ret;
}

/*
 * Function name:hisp_ca_ispcpu_disreset
 * Discription: load isp img and ispcpu disreset
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_ispcpu_disreset(void)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	int ret = 0;
	u32 origin = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE);

	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_CMD_IMG_DISRESET,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_CMD_IMG_DISRESET, result);
		ret = -EPERM;
	}

	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_ispcpu_reset
 * Discription: unmap sec mem and reset
 * Parameters:
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_ispcpu_reset(void)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE);

	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_CMD_RESET,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_CMD_RESET, result);
		ret = -EPERM;
	}

	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_sfdmem_map
 * Discription: mem cfg and map
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ buffer:  isp mem info
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_sfdmem_map(struct hisp_ca_meminfo *buffer)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	struct sg_table *table = NULL;
	enum SEC_SVC type = 0;
	u64 sfd = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buffer == NULL) {
		hisp_ca_err("hisp_ca_meminfo is NULL\n");
		return -EPERM;
	}

	ret = secmem_get_buffer(buffer->sharefd, &table, &sfd, &type);
	if (ret != 0) {
		hisp_ca_err("secmem_get_buffer fail. ret.%d!\n", ret);
		return -EFAULT;
	}

	g_pool_sfd[buffer->type] = sfd;
	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_VALUE_INPUT,
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_NONE,
					TEEC_NONE);

	operation.params[0].value.a       = (unsigned int)sfd;
	operation.params[0].value.b       = buffer->size;
	operation.params[1].tmpref.buffer = buffer;
	operation.params[1].tmpref.size   = sizeof(struct hisp_ca_meminfo);

	hisp_ca_info("do map +");
	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_SEC_MEM_CFG_AND_MAP,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_SEC_MEM_CFG_AND_MAP, result);
		ret = -EPERM;
	}

	hisp_ca_info("do map -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_sfdmem_unmap
 * Discription: mem cfg and unmap
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ buffer:  isp mem info
 * return value:
 *      @ 0-->success, others-->failed.
 */

int hisp_ca_sfdmem_unmap(struct hisp_ca_meminfo *buffer)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	u64 sfd = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buffer == NULL) {
		hisp_ca_err("hisp_ca_meminfo is NULL\n");
		return -EPERM;
	}

	sfd = g_pool_sfd[buffer->type];
	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_VALUE_INPUT,
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_NONE,
					TEEC_NONE);

	operation.params[0].value.a       = (unsigned int)sfd;
	operation.params[0].value.b       = buffer->size;
	operation.params[1].tmpref.buffer = buffer;
	operation.params[1].tmpref.size   = sizeof(struct hisp_ca_meminfo);

	hisp_ca_info("do unmap +");
	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_SEC_MEM_CFG_AND_UNMAP,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_SEC_MEM_CFG_AND_UNMAP, result);
		ret = -EPERM;
	}

	hisp_ca_info("do unmap -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_dynmem_map
 * Discription: mem cfg and map
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ buffer:  isp mem info
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_dynmem_map(
	struct hisp_ca_meminfo *buffer, struct device *dev)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buffer == NULL || dev == NULL) {
		hisp_ca_err("hisp_ca_meminfo or dev is NULL\n");
		return -EPERM;
	}

	ret = hisp_ca_get_ionmem_info(buffer->sharefd, &g_sgl, dev);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_get_ionmem_info fail\n");
		return -EPERM;
	}

	g_sgl->ion_size = buffer->size; /*lint !e613 */

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
		TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	operation.params[0].tmpref.buffer = g_sgl;
	operation.params[0].tmpref.size   = g_sgl->sglist_size;
	operation.params[1].tmpref.buffer = buffer;
	operation.params[1].tmpref.size	  = sizeof(struct hisp_ca_meminfo);

	hisp_ca_info("do map +");
	result = TEEK_InvokeCommand(&g_secisp_session,
		TEE_SECISP_NONSEC_MEM_MAP_SEC, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_NONSEC_MEM_MAP_SEC, result);
		vfree(g_sgl);
		g_sgl = NULL;
		hisp_ca_sgl_info_put(&g_info);
		ret = -EPERM;
	}

	hisp_ca_info("do map -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_dynmem_unmap
 * Discription: mem cfg and unmap
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ buffer:  isp mem info
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_dynmem_unmap(
	struct hisp_ca_meminfo *buffer, struct device *dev)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buffer == NULL || dev == NULL) {
		hisp_ca_err("hisp_ca_meminfo or dev is NULL\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
		TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	operation.params[0].tmpref.buffer = g_sgl;
	operation.params[0].tmpref.size   = g_sgl->sglist_size;
	operation.params[1].tmpref.buffer = buffer;
	operation.params[1].tmpref.size   = sizeof(struct hisp_ca_meminfo);

	hisp_ca_info("do unmap +");
	result = TEEK_InvokeCommand(&g_secisp_session,
		TEE_SECISP_NONSEC_MEM_UNMAP_SEC, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_NONSEC_MEM_UNMAP_SEC, result);
		ret = -EPERM;
	}

	vfree(g_sgl);
	g_sgl = NULL;
	hisp_ca_sgl_info_put(&g_info);
	hisp_ca_info("do unmap -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_imgmem_map
 * Discription: boot mem map
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ buffer:  boot img mem info
 * return value:
 *      @ 0-->success, others-->failed.
 */
static int hisp_ca_imgmem_map(struct hisp_ca_img_cfginfo *buffer)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buffer == NULL) {
		hisp_ca_err("hisp_ca_img_cfginfo is NULL\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE);

	operation.params[0].tmpref.buffer = buffer;
	operation.params[0].tmpref.size   = sizeof(struct hisp_ca_img_cfginfo);

	hisp_ca_info("do map +");
	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_IMG_MEM_CFG_AND_MAP,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_IMG_MEM_CFG_AND_MAP, result);
		ret = -EPERM;
	}

	hisp_ca_info("do map -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_imgmem_unmap
 * Discription: boot img mem unmap
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ buffer:  boot mem info
 * return value:
 *      @ 0-->success, others-->failed.
 */
static int hisp_ca_imgmem_unmap(struct hisp_ca_img_cfginfo *buffer)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buffer == NULL) {
		hisp_ca_err("hisp_ca_img_cfginfo is NULL\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE);

	operation.params[0].tmpref.buffer = buffer;
	operation.params[0].tmpref.size   = sizeof(struct hisp_ca_img_cfginfo);

	hisp_ca_info("do unmap +");
	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_IMG_MEM_CFG_AND_UNMAP,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_IMG_MEM_CFG_AND_UNMAP, result);
		ret = -EPERM;
	}

	hisp_ca_info("do unmap -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_rsvmem_map
 * Discription: isp reserve mem map
 * return value:
 *      @ 0-->success, others-->failed.
 */
static int hisp_ca_rsvmem_map(struct hisp_cs_rsv_cfginfo *buf)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	int ret = 0;
	u32 origin = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buf == NULL) {
		hisp_ca_err("hisp_cs_rsv_cfginfo is NULL\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE);

	operation.params[0].tmpref.buffer = buf;
	operation.params[0].tmpref.size   = sizeof(struct hisp_cs_rsv_cfginfo);

	hisp_ca_info("do map +");
	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_RSV_MEM_CFG_AND_MAP,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_RSV_MEM_CFG_AND_MAP, result);
		ret = -EPERM;
	}

	hisp_ca_info("do map -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_rsvmem_unmap
 * Discription: isp rsv mem unmap
 * Parameters:
 * return value:
 *      @ 0-->success, others-->failed.
 */
static int hisp_ca_rsvmem_unmap(struct hisp_cs_rsv_cfginfo *buf)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	int ret = 0;

	if (g_secisp_creat_teec == 0) {
		hisp_ca_err("secisp teec not create\n");
		return -EPERM;
	}

	if (buf == NULL) {
		hisp_ca_err("hisp_cs_rsv_cfginfo is NULL\n");
		return -EPERM;
	}

	mutex_lock(&g_secisp_tee_lock);
	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
					TEEC_MEMREF_TEMP_INPUT,
					TEEC_NONE,
					TEEC_NONE,
					TEEC_NONE);

	operation.params[0].tmpref.buffer = buf;
	operation.params[0].tmpref.size   = sizeof(struct hisp_cs_rsv_cfginfo);

	hisp_ca_info("do unmap +");
	result = TEEK_InvokeCommand(
			&g_secisp_session,
			TEE_SECISP_RSV_MEM_CFG_AND_UNMAP,
			&operation,
			&origin);
	if (result != TEEC_SUCCESS) {
		hisp_ca_err("TEEK_InvokeCommand .%d failed, result is 0x%x!\n",
			TEE_SECISP_RSV_MEM_CFG_AND_MAP, result);
		ret = -EPERM;
	}

	hisp_ca_info("do unmap -");
	mutex_unlock(&g_secisp_tee_lock);
	return ret;
}

/*
 * Function name:hisp_ca_imgmem_config
 * Discription: boot img mem cfg
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ info: boot img mem info
 *      @ info_size: info array num
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_imgmem_config(struct hisp_ca_meminfo *info, int info_size)
{
	int ret;

	if (info == NULL) {
		hisp_ca_err("hisp_ca_meminfo is NULL\n");
		return -EPERM;
	}

	g_img_buf = kzalloc(sizeof(struct hisp_ca_img_cfginfo), GFP_KERNEL);
	if (g_img_buf == NULL)
		return -ENOMEM;

	ret = hisp_ca_imgmem_cfg(g_img_buf, info, info_size);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_imgmem_cfg fail, ret.%d\n", ret);
		kfree(g_img_buf);
		g_img_buf = NULL;
		return ret;
	}

	ret = hisp_ca_imgmem_map(g_img_buf);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_imgmem_map fail, ret.%d\n", ret);
		kfree(g_img_buf);
		g_img_buf = NULL;
		return ret;
	}

	return 0;
}

/*
 * Function name:hisp_ca_imgmem_deconfig
 * Discription: boot mem decfg
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_imgmem_deconfig(void)
{
	int ret;

	ret = hisp_ca_imgmem_unmap(g_img_buf);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_imgmem_unmap fail, ret.%d\n", ret);
		kfree(g_img_buf);
		g_img_buf = NULL;
		return ret;
	}

	kfree(g_img_buf);
	g_img_buf = NULL;
	return 0;
}

/*
 * Function name:hisp_ca_rsvmem_config
 * Discription: isp rsv mem config
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ info: reserve meminfo
 *      @ info_size: info array size
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_rsvmem_config(struct hisp_ca_meminfo *info, int info_size)
{
	int ret;

	if (info == NULL) {
		hisp_ca_err("hisp_ca_meminfo is NULL\n");
		return -EPERM;
	}

	g_rsv_buf = kzalloc(sizeof(struct hisp_cs_rsv_cfginfo), GFP_KERNEL);
	if (g_rsv_buf == NULL)
		return -ENOMEM;

	ret = hisp_ca_rsvmem_cfg(g_rsv_buf, info, info_size);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_rsvmem_cfg fail, ret.%d\n", ret);
		kfree(g_rsv_buf);
		g_rsv_buf = NULL;
		return ret;
	}

	ret = hisp_ca_rsvmem_map(g_rsv_buf);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_rsvmem_map fail, ret.%d\n", ret);
		kfree(g_rsv_buf);
		g_rsv_buf = NULL;
		return ret;
	}

	return 0;
}

/*
 * Function name:hisp_ca_rsvmem_deconfig
 * Discription: isp rsv mem config
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 * return value:
 *      @ 0-->success, others-->failed.
 */
int hisp_ca_rsvmem_deconfig(void)
{
	int ret;

	ret = hisp_ca_rsvmem_unmap(g_rsv_buf);
	if (ret < 0) {
		hisp_ca_err("hisp_ca_rsvmem_unmap fail, ret.%d\n", ret);
		kfree(g_rsv_buf);
		g_rsv_buf = NULL;
		return ret;
	}

	kfree(g_rsv_buf);
	g_rsv_buf = NULL;
	return 0;
}

/*
 * Function name:hisp_ca_probe
 * Discription: sec isp ca init
 * Parameters:
 * return value:
 */
void hisp_ca_probe(void)
{
	mutex_init(&g_secisp_tee_lock);
	g_secisp_creat_teec = 0;
}
/*
 * Function name:hisp_ca_remove
 * Discription: sec isp ca remove
 * Parameters:
 * return value:
 */
void hisp_ca_remove(void)
{
	g_secisp_creat_teec = 0;
	mutex_destroy(&g_secisp_tee_lock);
}
