/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: implement Crypto Core autotest functions
 * Create: 2020-06-09
 */
#include "crypto_core_autotest.h"
#include <linux/compiler.h>
#include <linux/dma-mapping.h>
#include <linux/fd.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <securec.h>
#include "crypto_core_internal.h"
#include "crypto_core_fs.h"
#include "crypto_core_smc.h"

static struct mspc_autotest_work_struct g_autotest_result;

static int _get_test_fullname(const char *path, char *fullname,
			      unsigned int name_size)
{
	int i = 0;

	if (name_size < MAX_PATH_NAME_LEN) {
		pr_err("%s() fullname size is invalid\n", __func__);
		return MSPC_INVALID_PARAMS_ERROR;
	}

	do {
		if (path[i] == MSPC_CHAR_NEWLINE || path[i] == MSPC_CHAR_SPACE)
			break;
		fullname[i] = path[i];
		i++;
	} while (i < MAX_PATH_NAME_LEN);
	if (i == 0) {
		pr_err("%s() filename is invalid\n", __func__);
		return MSPC_CHANNEL_TEST_PATH_ABSENT_ERROR;
	}
	return MSPC_OK;
}

static int _get_test_image_size(const char *fullname, size_t *img_size)
{
	int fd = -1;
	mm_segment_t fs;
	int result;

	fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/
	fd = (int)crypto_core_sys_open(fullname, O_RDONLY, MSPC_FILESYS_DEFAULT_MODE);
	if (fd < 0) {
		pr_err("%s(): open %s failed, fd=%d\n", __func__, fullname, fd);
		set_fs(fs);
		return MSPC_CHANNEL_TEST_PATH_ABSENT_ERROR;
	}
	result = crypto_core_sys_lseek((unsigned int)fd, 0, SEEK_END);
	if (result < 0) {
		pr_err("%s(): sys_lseek failed from set.\n", __func__);
		crypto_core_sys_close((unsigned int)fd);
		set_fs(fs);
		return MSPC_LSEEK_FILE_ERROR;
	}

	*img_size = (size_t)result + MSPC_ATF_MESSAGE_HEADER_LEN; /*lint !e571*/
	pr_err("%s() file size is 0x%lx\n", __func__, *img_size);
	crypto_core_sys_close((unsigned int)fd);

	crypto_core_sys_unlink(TEST_SUCCESS_FILE);
	crypto_core_sys_unlink(TEST_FAIL_FILE);
	crypto_core_sys_unlink(TEST_RESULT_FILE);
	set_fs(fs);

	return MSPC_OK;
}

static int _mspc_test_result_handle(char *buff, int result)
{
	int fd = -1;
	int ret;
	struct mspc_atf_msg_header *msg_header = NULL;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/

	msg_header = (struct mspc_atf_msg_header *)buff;
	fd = (int)crypto_core_sys_mkdir(TEST_DIRECTORY_PATH, MSPC_FILESYS_DEFAULT_MODE);
	/* EEXIST(File exists), don't return error */
	if (fd < 0 && (fd != -EEXIST)) {
		pr_err("create dir %s fail,ret:%d.\n", TEST_DIRECTORY_PATH, fd);
		ret = fd;
		goto error;
	}

	if (result == MSPC_OK) {
		/* create file for test flag */
		pr_err("%s(): rcv result size is 0x%x\r\n",
		       __func__, msg_header->result_size);
		if ((uint32_t)g_autotest_result.phy == msg_header->result_addr &&
		    g_autotest_result.size >= msg_header->result_size) {
			fd = (int)crypto_core_sys_open(TEST_RESULT_FILE, O_RDWR | O_CREAT, 0);
			if (fd < 0) {
				pr_err("sys_open %s fail, fd: %d.\n",
				       TEST_RESULT_FILE, fd);
				ret = fd;
				goto error;
			}
			(void)crypto_core_sys_write((unsigned int)fd, g_autotest_result.buffer,
						    (size_t)msg_header->result_size);
			crypto_core_sys_close((unsigned int)fd);
			fd = (int)crypto_core_sys_open(TEST_SUCCESS_FILE, O_RDWR | O_CREAT, 0);
			if (fd < 0) {
				pr_err("sys_open %s fail, fd: %d.\n",
				       TEST_SUCCESS_FILE, fd);
				ret = fd;
				goto error;
			}
			crypto_core_sys_close((unsigned int)fd);
			ret = MSPC_OK;
		} else {
			fd = (int)crypto_core_sys_open(TEST_FAIL_FILE, O_RDWR | O_CREAT, 0);
			if (fd < 0) {
				pr_err("sys_open %s fail, fd: %d.\n",
				       TEST_FAIL_FILE, fd);
				ret = fd;
				goto error;
			}
			crypto_core_sys_close((unsigned int)fd);
			ret = MSPC_CHANNEL_TEST_WRITE_RESULT_ERROR;
		}
	} else {
		fd = (int)crypto_core_sys_open(TEST_FAIL_FILE, O_RDWR | O_CREAT, 0);
		if (fd < 0) {
			pr_err("sys_open %s fail, fd: %d.\n", TEST_FAIL_FILE, fd);
			ret = fd;
			goto error;
		}
		crypto_core_sys_close((unsigned int)fd);
		ret = MSPC_CHANNEL_TEST_WRITE_RESULT_ERROR;
	}
error:
	set_fs(fs);
	return ret;
}

/*
 * @brief      : basic process for mspc autotest
 * @param[in]  : path, the string of device node path for mspc test
 * @param[out] : result_phy, the physical address for test result buffer
 * @param[out] : result_size, the size for test result buffer
 * @return     : 0 on success, other value on failure.
 * @notes      : echo command should add "new line" character(0xa) to the
 *               end of string. so path should discard this character
 */
static int mspc_test(const char *path, phys_addr_t result_phy,
		     size_t result_size)
{
	phys_addr_t buff_phy = 0;
	char fullname[MAX_PATH_NAME_LEN + 1] = {0};
	struct mspc_atf_msg_header *msg_header = NULL;
	char *buff_virt = NULL;
	struct device *mspc_device = NULL;
	size_t image_size = 0;
	int ret = MSPC_ERROR;

	mspc_device = mspc_get_device();
	if (!mspc_device) {
		pr_err("%s:get devices failed!\n", __func__);
		return ret;
	}
	ret = _get_test_fullname(path, fullname, MAX_PATH_NAME_LEN);
	if (ret != MSPC_OK)
		return ret;

	ret = _get_test_image_size(fullname, &image_size);
	if (ret != MSPC_OK)
		return ret;

	buff_virt = (char *)dma_alloc_coherent(mspc_device,
					       ALIGN_UP_4KB(image_size),
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return MSPC_SECUREC_ERROR;
	}
	(void)memset_s(buff_virt, ALIGN_UP_4KB(image_size),
		       0, ALIGN_UP_4KB(image_size));

	msg_header = (struct mspc_atf_msg_header *)buff_virt;
	mspc_set_atf_msg_header(msg_header, MSPC_SMC_CHANNEL_AUTOTEST);
	msg_header->result_addr = (uint32_t)result_phy;
	msg_header->result_size = (uint32_t)result_size;
	msg_header->cos_id = MSPC_NATIVE_COS_ID;

	ret = mspc_read_input_test_file(fullname,
			buff_virt + MSPC_ATF_MESSAGE_HEADER_LEN, 0UL,
			image_size - MSPC_ATF_MESSAGE_HEADER_LEN);
	if (ret < MSPC_OK) {
		pr_err("%s(): mspc_read_file failed, ret=%d\n", __func__, ret);
		goto end_process;
	}

	ret = mspc_send_smc_process(msg_header, buff_phy, image_size,
				    MSPC_ATF_SMC_TIMEOUT,
				    MSPC_SMC_CHANNEL_AUTOTEST);
	if (ret != MSPC_OK) {
		pr_err("%s:send image to ATF fail, ret = %d\n", __func__, ret);
		(void)_mspc_test_result_handle(buff_virt, ret);
		goto end_process;
	}

	/* build up the test result info */
	ret = _mspc_test_result_handle(buff_virt, ret);
	if (ret != MSPC_OK)
		pr_err("%s:test result handle fail, ret = %d\n", __func__, ret);

end_process:
	dma_free_coherent(mspc_device, ALIGN_UP_4KB(image_size),
			  buff_virt, buff_phy);
	return ret;
}

static void _get_test_buffer_size(const char *buff, int *offset)
{
	int run_offset;
	unsigned int value, size;
	int total_char;
	int i;
	unsigned int bit_width;
	char curr_char;

	run_offset = *offset + (int)sizeof("result_size:0x") - 1;
	/* find last size char */
	i = 0;
	while (buff[run_offset + i] != MSPC_CHAR_SPACE)
		i++;

	if (i == 0 || i > MSPC_INT_NUM_HEX_LEN) {
		pr_err("result size is bad, use default size.\n");
		total_char = 0;
		size = CHANNEL_TEST_RESULT_SIZE_DEFAULT;
	} else {
		total_char = i;
		size = 0;
		i--;
		bit_width = 0;
		while (i >= 0) {
			curr_char = buff[run_offset + i];
			if (curr_char >= '0' && curr_char <= '9') {
				value = curr_char - '0';
			} else if (curr_char >= 'a' && curr_char <= 'f') {
				value = (curr_char - 'a') + 0xa;
			} else if (curr_char >= 'A' && curr_char <= 'F') {
				value = (curr_char - 'A') + 0xA;
			} else {
				pr_err("result size is bad, use default size.\n");
				size = TEST_RESULT_SIZE_DEFAULT;
				break;
			}
			size += (value << bit_width);
			i--;
			bit_width += 4; /* 4 is the char bit width in Hex */
		}
	}

	g_autotest_result.size = size;
	*offset = run_offset + total_char;
}

static int channel_test_check_buffer_size(const char *buff, int offset)
{
	int updated_offset = offset;

	if (strncmp(buff + updated_offset, "result_size:0x",
		    sizeof("result_size:0x") - 1) == 0)
		_get_test_buffer_size(buff, &updated_offset);
	else
		g_autotest_result.size = TEST_RESULT_SIZE_DEFAULT;

	return updated_offset;
}

/*
 * @brief      : entry for mspc autotest
 * @param[in]  : buf, test command string buffer
 * @param[in]  : para, parameters, not used now
 * @return     : 0 on success, other value on failure.
 */
int mspc_channel_test_func(const void *buf, int para)
{
	const char *buff = buf;
	int ret;
	int offset = 0;
	size_t size;
	struct device *mspc_device = NULL;

	(void)para;
	if (!buf) {
		pr_err("%s(): input buf is NULL.\n", __func__);
		ret = MSPC_NO_RESOURCES_ERROR;
		goto exit_process;
	}

	mspc_device = mspc_get_device();
	if (!mspc_device) {
		pr_err("%s:get devices failed!\n", __func__);
		ret = MSPC_NO_RESOURCES_ERROR;
		goto exit_process;
	}
	bypass_space_char();
	offset = channel_test_check_buffer_size(buff, offset);

	size = (size_t)g_autotest_result.size;
	pr_err("result size is 0x%x.\n", (unsigned int)size);
	if (size == 0) {
		pr_err("result size is bad.\r\n");
		ret = MSPC_CHANNEL_TEST_CMD_ERROR;
		goto exit_process;
	}

	bypass_space_char();
	if (buff[offset] == 0) {
		pr_err("test file path is bad.\n");
		ret = MSPC_CHANNEL_TEST_CMD_ERROR;
		goto exit_process;
	}

	if (g_autotest_result.buffer)
		dma_free_coherent(mspc_device, ALIGN_UP_4KB(size),
				  g_autotest_result.buffer,
				  g_autotest_result.phy);

	g_autotest_result.buffer =
		(int8_t*)dma_alloc_coherent(mspc_device, ALIGN_UP_4KB(size),
			(dma_addr_t *)&g_autotest_result.phy, GFP_KERNEL);
	if (!g_autotest_result.buffer) {
		pr_err("%s(): alloc 0x%x fail.\r\n",
		       __func__, (unsigned int)ALIGN_UP_4KB(size));
		ret = MSPC_CHANNEL_TEST_RESULT_MALLOC_ERROR;
		goto exit_process;
	}

	ret = mspc_test(buff + offset, g_autotest_result.phy, size);
	if (ret != MSPC_OK)
		pr_err("%s(): mspc_test fail, ret = %d\n", __func__, ret);

	dma_free_coherent(mspc_device, ALIGN_UP_4KB(size),
			  g_autotest_result.buffer, g_autotest_result.phy);
	g_autotest_result.buffer = NULL;
	g_autotest_result.phy = 0;
	g_autotest_result.size = 0;

exit_process:
	pr_err("mspc:%s() run %s\n", __func__,
			(ret == MSPC_OK) ? "success" :  "failed");

	mspc_record_errno(ret);
	return ret;
}
