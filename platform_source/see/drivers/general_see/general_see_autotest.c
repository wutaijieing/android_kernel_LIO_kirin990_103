/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: implement general see autotest functions
 * Create: 2020-02-17
 */
#include "general_see_autotest.h"
#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/fd.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <securec.h>
#include "general_see.h"
#include "general_see_fs.h"

/* part 1: channel test function */
#ifdef CONFIG_DFX_DEBUG_FS
static int _get_test_fullname(const char *path, char *fullname)
{
	int i = 0;

	do {
		if (path[i] == HISEE_CHAR_NEWLINE || path[i] == HISEE_CHAR_SPACE)
			break;
		fullname[i] = path[i];
		i++;
	} while (i < MAX_PATH_NAME_LEN);
	if (i == 0) {
		pr_err("%s() filename is invalid\n", __func__);
		return HISEE_CHANNEL_TEST_PATH_ABSENT_ERROR;
	}
	return HISEE_OK;
}

static int _get_test_image_size(const char *fullname, size_t *img_size)
{
	int fd = -1;
	mm_segment_t fs;
	int ret;

	fs = get_fs();
	set_fs(KERNEL_DS);
	fd = (int)hisee_sys_open(fullname, O_RDONLY, HISEE_FILESYS_DEFAULT_MODE);
	if (fd < 0) {
		pr_err("%s(): open %s failed, fd=%d\n", __func__, fullname, fd);
		set_fs(fs);
		return HISEE_CHANNEL_TEST_PATH_ABSENT_ERROR;
	}
	ret = (int)hisee_sys_lseek((unsigned int)fd, 0, SEEK_END);
	if (ret < 0) {
		pr_err("%s(): sys_lseek failed from set.\n", __func__);
		hisee_sys_close((unsigned int)fd);
		set_fs(fs);
		return HISEE_LSEEK_FILE_ERROR;
	}

	*img_size = (size_t)ret + (size_t)HISEE_ATF_MESSAGE_HEADER_LEN;
	pr_err("%s() file size is 0x%x\n", __func__, (unsigned int)(*img_size));
	hisee_sys_close((unsigned int)fd);

	hisee_sys_unlink(TEST_SUCCESS_FILE);
	hisee_sys_unlink(TEST_FAIL_FILE);
	hisee_sys_unlink(TEST_RESULT_FILE);
	set_fs(fs);

	return HISEE_OK;
}

static int _hisee_test_result_handle(char *buff, int result)
{
	int fd = -1;
	struct atf_message_header *p_message_header = NULL;
	mm_segment_t fs;
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	fs = get_fs();
	set_fs(KERNEL_DS);

	p_message_header = (struct atf_message_header *)buff;
	fd = (int)hisee_sys_mkdir(TEST_DIRECTORY_PATH, HISEE_FILESYS_DEFAULT_MODE);
	/* EEXIST(File exists), don't return error */
	if (fd < 0 && (fd != -EEXIST)) {
		pr_err("create dir %s fail, ret: %d.\n", TEST_DIRECTORY_PATH, fd);
		ret = fd;
		goto error;
	}

	if (result == HISEE_OK) {
		/* create file for test flag */
		pr_err("%s(): rcv result size is 0x%x\r\n",
		       __func__, p_message_header->test_result_size);
		if (hisee_data_ptr->channel_test_item_result.phy ==
			p_message_header->test_result_phy &&
		    hisee_data_ptr->channel_test_item_result.size >=
			(long)p_message_header->test_result_size) {
			fd = (int)hisee_sys_open(TEST_RESULT_FILE, O_RDWR | O_CREAT, 0);
			if (fd < 0) {
				pr_err("sys_open %s fail, fd: %d.\n",
				       TEST_RESULT_FILE, fd);
				ret = fd;
				goto error;
			}
			(void)hisee_sys_write((unsigned int)fd,
					hisee_data_ptr->channel_test_item_result.buffer,
					(unsigned long)p_message_header->test_result_size);
			hisee_sys_close((unsigned int)fd);
			fd = (int)hisee_sys_open(TEST_SUCCESS_FILE, O_RDWR | O_CREAT, 0);
			if (fd < 0) {
				pr_err("sys_open %s fail, fd: %d.\n",
				       TEST_SUCCESS_FILE, fd);
				ret = fd;
				goto error;
			}
			hisee_sys_close((unsigned int)fd);
			ret = HISEE_OK;
		} else {
			fd = (int)hisee_sys_open(TEST_FAIL_FILE, O_RDWR | O_CREAT, 0);
			if (fd < 0) {
				pr_err("sys_open %s fail, fd: %d.\n",
				       TEST_FAIL_FILE, fd);
				ret = fd;
				goto error;
			}
			hisee_sys_close((unsigned int)fd);
			ret = HISEE_CHANNEL_TEST_WRITE_RESULT_ERROR;
		}
	} else {
		fd = (int)hisee_sys_open(TEST_FAIL_FILE, O_RDWR | O_CREAT, 0);
		if (fd < 0) {
			pr_err("sys_open %s fail, fd: %d.\n", TEST_FAIL_FILE, fd);
			ret = fd;
			goto error;
		}
		hisee_sys_close((unsigned int)fd);
		ret = HISEE_CHANNEL_TEST_WRITE_RESULT_ERROR;
	}
error:
	set_fs(fs);
	return ret;
}

/*
 * @brief      : basic process for hisee autotest
 * @param[in]  : path, the string of device node path for hisee test
 * @param[out] : result_phy, the physical address for test result buffer
 * @param[out] : result_size, the size for test result buffer
 * @return     : 0 on success, other value on failure.
 * @notes      : echo command should add "new line" character(0xa) to the
 *               end of string. so path should discard this character
 */
static int hisee_test(const char *path, phys_addr_t result_phy,
		      size_t result_size)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	char fullname[MAX_PATH_NAME_LEN + 1] = {0};
	struct atf_message_header *p_message_header = NULL;
	size_t image_size = 0;
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	ret = _get_test_fullname(path, fullname);
	if (ret != HISEE_OK)
		return set_errno_then_exit(ret);

	ret = _get_test_image_size(fullname, &image_size);
	if (ret != HISEE_OK)
		return set_errno_then_exit(ret);

	buff_virt = (char *)dma_alloc_coherent(
					hisee_data_ptr->cma_device,
					ALIGN_UP_4KB(image_size),
					&buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	ret = memset_s(buff_virt, ALIGN_UP_4KB(image_size), 0,
		       ALIGN_UP_4KB(image_size));
	if (ret != EOK) {
		pr_err("%s(): memset err.\n", __func__);
		ret = HISEE_SECUREC_ERR;
		goto end_process;
	}
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_HISEE_CHANNEL_TEST);
	p_message_header->test_result_phy = (unsigned int)result_phy;
	p_message_header->test_result_size = (unsigned int)result_size;

	ret = hisee_read_file(fullname,
			      buff_virt + HISEE_ATF_MESSAGE_HEADER_LEN, 0UL,
			      image_size - HISEE_ATF_MESSAGE_HEADER_LEN);
	if (ret < HISEE_OK) {
		pr_err("%s(): hisee_read_file failed, ret=%d\n", __func__, ret);
		goto end_process;
	}

	ret = send_smc_process(p_message_header, buff_phy, image_size,
			       HISEE_ATF_GENERAL_TIMEOUT,
			       CMD_HISEE_CHANNEL_TEST);
	/* build up the test result info */
	ret = _hisee_test_result_handle(buff_virt, ret);

end_process:
	dma_free_coherent(hisee_data_ptr->cma_device,
			  ALIGN_UP_4KB(image_size),
			  buff_virt, buff_phy);
	return set_errno_then_exit(ret);
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
	while (buff[run_offset + i] != HISEE_CHAR_SPACE)
		i++;

	if (i == 0 || i > HISEE_INT_NUM_HEX_LEN) {
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
			bit_width += 4; /* 4 is the char bit width in Hexadecimal */
		}
	}

	get_hisee_data_ptr()->channel_test_item_result.size = size;
	*offset = run_offset + total_char;
}

static int channel_test_check_buffer_size(const char *buff, int offset)
{
	int updated_offset = offset;

	if (strncmp(buff + updated_offset, "result_size:0x",
		    sizeof("result_size:0x") - 1) == 0)
		_get_test_buffer_size(buff, &updated_offset);
	else
		get_hisee_data_ptr()->channel_test_item_result.size =
			TEST_RESULT_SIZE_DEFAULT;

	return updated_offset;
}
#endif /* CONFIG_DFX_DEBUG_FS */

/*
 * @brief      : entry for hisee autotest
 * @param[in]  : buf, test command string buffer
 * @param[in]  : para, parameters
 * @return     : 0 on success, other value on failure.
 */
int hisee_channel_test_func(const void *buf, int para)
{
#ifdef CONFIG_DFX_DEBUG_FS
	const char *buff = buf;
	int ret;
	int offset = 0;
	size_t size;
	struct hisee_module_data *hisee_data_ptr = NULL;

	if (!buf) {
		pr_err("%s(): input buf is NULL.\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}

	hisee_data_ptr = get_hisee_data_ptr();
	bypass_space_char();
	offset = channel_test_check_buffer_size(buff, offset);

	size = (size_t)hisee_data_ptr->channel_test_item_result.size;
	pr_err("result size is 0x%x.\n", (unsigned int)size);
	if (size == 0) {
		pr_err("result size is bad.\r\n");
		return set_errno_then_exit(HISEE_CHANNEL_TEST_CMD_ERROR);
	}

	bypass_space_char();
	if (buff[offset] == 0) {
		pr_err("test file path is bad.\n");
		return set_errno_then_exit(HISEE_CHANNEL_TEST_CMD_ERROR);
	}

	if (hisee_data_ptr->channel_test_item_result.buffer)
		dma_free_coherent(hisee_data_ptr->cma_device, ALIGN_UP_4KB(size),
				  hisee_data_ptr->channel_test_item_result.buffer,
				  hisee_data_ptr->channel_test_item_result.phy);

	hisee_data_ptr->channel_test_item_result.buffer =
		(char *)dma_alloc_coherent(hisee_data_ptr->cma_device, ALIGN_UP_4KB(size),
				(dma_addr_t *)&hisee_data_ptr->channel_test_item_result.phy,
				GFP_KERNEL);
	if (!hisee_data_ptr->channel_test_item_result.buffer) {
		pr_err("%s(): alloc 0x%x fail.\r\n", __func__, (unsigned int)ALIGN_UP_4KB(size));
		return set_errno_then_exit(HISEE_CHANNEL_TEST_RESULT_MALLOC_ERROR);
	}

	ret = hisee_test(buff + offset,
			 hisee_data_ptr->channel_test_item_result.phy, size);
	if (ret != HISEE_OK)
		pr_err("%s(): hisee_test fail, ret = %d\n", __func__, ret);

	dma_free_coherent(hisee_data_ptr->cma_device, ALIGN_UP_4KB(size),
			  hisee_data_ptr->channel_test_item_result.buffer,
			  hisee_data_ptr->channel_test_item_result.phy);
	hisee_data_ptr->channel_test_item_result.buffer = NULL;
	hisee_data_ptr->channel_test_item_result.phy = 0;
	hisee_data_ptr->channel_test_item_result.size = 0;
	check_and_print_result();
	return set_errno_then_exit(ret);
#else
	int ret = HISEE_OK;

	check_and_print_result();
	return set_errno_then_exit(ret);
#endif
}
