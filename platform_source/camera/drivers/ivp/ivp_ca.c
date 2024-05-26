/*
 * This file define interfaces for dev-ivp secure ca
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

#include "ivp_ca.h"
#include <teek_client_api.h>
#include <teek_client_id.h>
#include "ivp_log.h"

#define TEE_SECIVP_SERVICE_ID      TEE_SERVICE_DRM_GRALLOC
#define TEE_SECIVP_PACKAGE_NAME    "/vendor/bin/CameraDaemon"
#define TEE_SECIVP_UID             1013

#define INVALID_IOVA               0
#define IVP_TA_TRUE                1
#define IVP_TA_FALSE               0

#define IOMMU_READ   (1 << 0)
#define IOMMU_WRITE  (1 << 1)
#define IOMMU_CACHE  (1 << 2)
#define IOMMU_NOEXEC (1 << 3)
#define IOMMU_DEVICE (1 << 7)
#define IOMMU_SEC    (1 << 8)
#define IOMMU_EXEC   (1 << 9)

#define get_subparam_type(param_types, index) (0x0F & ((param_types) >> ((index) * 4)))
#define cmd_paramtype_fill(_cmd, _type) {.cmd = (_cmd), .param_type = (_type)}

enum {
	TEE_SECIVP_CMD_MAP_DRM_BUFFER = 0,
	TEE_SECIVP_CMD_CORE_MEM_CFG,
	TEE_SECIVP_CMD_CORE_MEM_UNCFG,
	TEE_SECIVP_CMD_SEC_SMMU_INIT,
	TEE_SECIVP_CMD_SEC_SMMU_DEINIT,
	TEE_SECIVP_CMD_ALGO_SEC_MAP,
	TEE_SECIVP_CMD_ALGO_SEC_UNMAP,
	TEE_SECIVP_CMD_ALGO_NONSEC_MAP,
	TEE_SECIVP_CMD_ALGO_NONSEC_UNMAP,
	TEE_SECIVP_CMD_CAMERA_BUFFER_CFG,
	TEE_SECIVP_CMD_CAMERA_BUFFER_UNCFG,
	TEE_SECIVP_CMD_IVP_CLEAR,
	TEE_SECIVP_CMD_OPEN,
	TEE_SECIVP_CMD_MAX,
};

enum {
	TEE_PARAM_INDEX0 = 0,
	TEE_PARAM_INDEX1,
	TEE_PARAM_INDEX2,
	TEE_PARAM_INDEX3
};

enum {
	SECURE = 0,
	NON_SECURE
};

typedef struct {
	int sharefd;           /* memory fd */
	unsigned int size;     /* memory size */
	unsigned int type;     /* memory type */
	unsigned int da;       /* device address */
	unsigned int prot;     /* operation right */
	unsigned int sec_flag; /* secure flag */
	unsigned long long pa; /* physical address */
} secivp_mem_info;

typedef struct {
	unsigned int cmd;
	unsigned int param_type;
} secivp_cmd_and_paramtype;

static const secivp_cmd_and_paramtype g_cmd_paramtype_vector[TEE_SECIVP_CMD_MAX] = {
	cmd_paramtype_fill(TEE_SECIVP_CMD_MAP_DRM_BUFFER,
		TEEC_PARAM_TYPES(TEEC_ION_INPUT, TEEC_VALUE_OUTPUT, TEEC_NONE,
			TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_CORE_MEM_CFG,
		TEEC_PARAM_TYPES(TEEC_ION_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_CORE_MEM_UNCFG,
		TEEC_PARAM_TYPES(TEEC_ION_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_SEC_SMMU_INIT,
		TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_SEC_SMMU_DEINIT,
		TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_ALGO_SEC_MAP,
		TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
			TEEC_VALUE_OUTPUT, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_ALGO_SEC_UNMAP,
		TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
			TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_ALGO_NONSEC_MAP,
		TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT,
			TEEC_VALUE_OUTPUT, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_ALGO_NONSEC_UNMAP,
		TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE,
			TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_CAMERA_BUFFER_CFG,
		TEEC_PARAM_TYPES(TEEC_ION_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_CAMERA_BUFFER_UNCFG,
		TEEC_PARAM_TYPES(TEEC_ION_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_IVP_CLEAR,
		TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE)),
	cmd_paramtype_fill(TEE_SECIVP_CMD_OPEN,
		TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_MEMREF_TEMP_INPUT,
			TEEC_MEMREF_TEMP_INPUT)),
};

static TEEC_Session g_secivp_session;
static TEEC_Context g_secivp_context;
static bool g_secivp_creat_teec = false;

static unsigned int find_param_type(unsigned int cmd)
{
	unsigned int idx;
	for (idx = 0; idx < TEE_SECIVP_CMD_MAX; idx++) {
		if (cmd == g_cmd_paramtype_vector[idx].cmd) {
			return g_cmd_paramtype_vector[idx].param_type;
		}
	}
	return 0;
}

static void secivp_fill_operation_basic(TEEC_Operation *operation, unsigned int cmd)
{
	(void)memset_s(operation, sizeof(TEEC_Operation), 0, sizeof(TEEC_Operation));
	operation->started = IVP_TA_TRUE;
	operation->cancel_flag = IVP_TA_FALSE;
	operation->paramTypes = find_param_type(cmd);
}

static void secivp_fill_buffer(secivp_mem_info *buffer, unsigned int cmd,
	int sharefd, unsigned int size)
{
	if (buffer == NULL)
		return;
	(void)memset_s(buffer, sizeof(secivp_mem_info), 0, sizeof(secivp_mem_info));
	buffer->sharefd = sharefd;
	buffer->size = size;
	if (cmd == TEE_SECIVP_CMD_ALGO_SEC_MAP ||
		cmd == TEE_SECIVP_CMD_ALGO_SEC_UNMAP) {
		buffer->sec_flag = SECURE;
		buffer->prot = IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE | IOMMU_SEC;
	} else {
		buffer->sec_flag = NON_SECURE;
		buffer->prot = IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE;
	}
}

static void secivp_fill_operation_param(TEEC_Operation *operation,
	unsigned int index, void *addr, unsigned int size)
{
	unsigned int param_type;
	param_type = get_subparam_type(operation->paramTypes, index);
	switch (param_type) {
	case TEEC_VALUE_INPUT:
		operation->params[index].value.a = *((int *)addr);
		operation->params[index].value.b = size;
		break;
	case TEEC_ION_INPUT:
		operation->params[index].ionref.ion_share_fd = *((int *)addr);
		operation->params[index].ionref.ion_size = size;
		break;
	case TEEC_MEMREF_TEMP_INPUT:
		operation->params[index].tmpref.buffer = addr;
		operation->params[index].tmpref.size = size;
		break;
	case TEEC_VALUE_OUTPUT:
	case TEEC_NONE:
		break;
	default:
		ivp_err("param_type is inavild");
		return;
	}
}

static int teek_secivp_open(void)
{
	TEEC_Result result;
	TEEC_UUID svc_uuid = TEE_SECIVP_SERVICE_ID; /* Tzdriver id for secivp */
	TEEC_Operation operation = {0};
	unsigned int root_id = TEE_SECIVP_UID;
	char package_name[] = TEE_SECIVP_PACKAGE_NAME;

	ivp_info("enter");
	result = TEEK_InitializeContext(NULL, &g_secivp_context);
	if (result != TEEC_SUCCESS) {
		ivp_err("initialize context failed, result = 0x%x", result);
		return -EPERM;
	}

	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_OPEN);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX2,
		(void *)(&root_id), sizeof(root_id));
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX3,
		(void *)(package_name), strlen(package_name) + 1);

	result = TEEK_OpenSession(&g_secivp_context, &g_secivp_session, &svc_uuid,
		TEEC_LOGIN_IDENTIFY, NULL, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		ivp_err("open session failed, result = 0x%x", result);
		TEEK_FinalizeContext(&g_secivp_context);
		return -EPERM;
	}
	g_secivp_creat_teec = true;
	return 0;
}

static void teek_secivp_close(void)
{
	ivp_info("enter");
	TEEK_CloseSession(&g_secivp_session);
	TEEK_FinalizeContext(&g_secivp_context);
	g_secivp_creat_teec = false;
}

int teek_secivp_sec_cfg(int sharefd, unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation;
	unsigned int origin = 0;

	if (!g_secivp_creat_teec) {
		ivp_err("secivp teec not create");
		return -EPERM;
	}

	if (size == 0) {
		ivp_err("ISP buffer no need set sec cfg");
		return EOK;
	}

	if (sharefd < 0) {
		ivp_err("sharefd invalid %d", sharefd);
		return -EINVAL;
	}

	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_CAMERA_BUFFER_CFG);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX0,
		(void *)(&sharefd), size);

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_CAMERA_BUFFER_CFG, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		ivp_err("sec cfg failed, result[0x%x] origin[0x%x]", result, origin);
		return -EPERM;
	}
	return EOK;
}

int teek_secivp_sec_uncfg(int sharefd, unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation;
	u32 origin = 0;

	if (!g_secivp_creat_teec) {
		ivp_err("secivp teec not create");
		return -EPERM;
	}

	if (size == 0) {
		ivp_err("ISP buffer no need set sec uncfg");
		return EOK;
	}

	if (sharefd < 0) {
		ivp_err("sharefd invalid %d", sharefd);
		return -EINVAL;
	}

	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_CAMERA_BUFFER_UNCFG);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX0,
		(void *)(&sharefd), size);

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_CAMERA_BUFFER_UNCFG, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		ivp_err("sec uncfg failed, result[0x%x] origin[0x%x]", result, origin);
		return -EPERM;
	}
	return EOK;
}

unsigned int teek_secivp_secmem_map(unsigned int sharefd, unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation;
	secivp_mem_info buffer;
	unsigned int origin = 0;

	secivp_fill_buffer(&buffer, TEE_SECIVP_CMD_ALGO_SEC_MAP, sharefd, size);
	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_ALGO_SEC_MAP);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX0,
		(void *)(&sharefd), size);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX1,
		(void *)(&buffer), sizeof(buffer));

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_ALGO_SEC_MAP, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		ivp_err("ivp map failed, result[0x%x] origin[0x%x]", result, origin);
		return INVALID_IOVA;
	}

	return operation.params[TEE_PARAM_INDEX2].value.a;
}

int teek_secivp_secmem_unmap(unsigned int sharefd, unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation;
	secivp_mem_info buffer;
	unsigned int origin = 0;

	secivp_fill_buffer(&buffer, TEE_SECIVP_CMD_ALGO_SEC_UNMAP,
		sharefd, size);
	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_ALGO_SEC_UNMAP);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX0,
		(void *)(&sharefd), size);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX1,
		(void *)(&buffer), sizeof(buffer));

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_ALGO_SEC_UNMAP, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		ivp_err("ivp ummap failed, result[0x%x] origin[0x%x]", result, origin);
		return -EPERM;
	}
	return EOK;
}

unsigned int teek_secivp_nonsecmem_map(int sharefd, unsigned int size,
	struct sglist *sgl)
{
	TEEC_Result result;
	TEEC_Operation operation;
	secivp_mem_info buffer;
	unsigned int origin = 0;

	if (sharefd < 0 || sgl == NULL) {
		ivp_err("invalid input");
		return INVALID_IOVA;
	}

	sgl->ion_size = size;
	secivp_fill_buffer(&buffer, TEE_SECIVP_CMD_ALGO_NONSEC_MAP, sharefd, size);
	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_ALGO_NONSEC_MAP);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX0, (void *)sgl,
		sgl->sglist_size);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX1,
		(void *)(&buffer), sizeof(buffer));

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_ALGO_NONSEC_MAP, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		ivp_err("ivp map failed, result[0x%x] origin[0x%x]", result, origin);
		return INVALID_IOVA;
	}
	return operation.params[TEE_PARAM_INDEX2].value.a;
}

int teek_secivp_nonsecmem_unmap(int sharefd, unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation;
	secivp_mem_info buffer;
	unsigned int origin = 0;

	if (sharefd < 0) {
		ivp_err("invalid input");
		return -EINVAL;
	}

	secivp_fill_buffer(&buffer, TEE_SECIVP_CMD_ALGO_NONSEC_UNMAP, sharefd, size);
	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_ALGO_NONSEC_UNMAP);
	secivp_fill_operation_param(&operation, TEE_PARAM_INDEX0, (void *)(&buffer),
		sizeof(buffer));

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_ALGO_NONSEC_UNMAP, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		ivp_err("ivp ummap failed, result[0x%x] origin[0x%x]", result, origin);
		return -EPERM;
	}

	return EOK;
}

void teek_secivp_clear(void)
{
	TEEC_Result result;
	TEEC_Operation operation;
	unsigned int origin = 0;

	if (!g_secivp_creat_teec) {
		ivp_err("secivp teec not create");
		return;
	}

	secivp_fill_operation_basic(&operation, TEE_SECIVP_CMD_IVP_CLEAR);

	result = TEEK_InvokeCommand(&g_secivp_session,
		TEE_SECIVP_CMD_IVP_CLEAR, &operation, &origin);
	if (result != TEEC_SUCCESS)
		ivp_err("clear failed, result[0x%x] origin[0x%x]", result, origin);
	return;
}

int teek_secivp_ca_probe(void)
{
	if (g_secivp_creat_teec) {
		ivp_warn("ta has opened");
		return EOK;
	}
	return teek_secivp_open();
}

void teek_secivp_ca_remove(void)
{
	if (!g_secivp_creat_teec) {
		ivp_warn("ta has closed");
		return;
	}
	teek_secivp_close();
}
