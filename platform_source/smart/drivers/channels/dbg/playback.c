/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: Contexthub data playback driver. Test only, not enable in USER build.
 * Create: 2017-04-22
 */

#include "playback.h"
#include <iomcu_ddr_map.h>
#include <linux/completion.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_syscounter_interface.h>
#include <securec.h>
#include "common/common.h"

#define PLAYBACK_SHARE_MEM_BASEADDR  DDR_PLAYBACK_PHYMEM_BASE_AP
#define PLAYBACK_SHARE_MEM_SIZE      DDR_PLAYBACK_PHYMEM_SIZE

#define PLAYBACK_FILE_OPER_MODE      (0660)
#define PLAYBACK_FILE_WRITE_MODE     (0666)
#define PLAYBACK_FILE_READ_MODE      (0644)
#define PLAYBACK_UINT_SIZE           (4)
#define PLAYBACK_USHORT_SIZE         (2)
#define PLAYBACK_SIZE_1K             (1024)

#if PLAYBACK_SHARE_MEM_SIZE > DDR_PLAYBACK_PHYMEM_SIZE
#error "PLAYBACK_SHARE_MEM_SIZE overflow"
#endif

#define PLAYBACK_PATH  "/data/playback"  // Modify this path while debug

struct playback_dev_t g_playback_dev;

struct linux_dirent {
	unsigned long d_ino;
	unsigned long d_off;
	unsigned short d_reclen;
	char d_name[1];
};

static unsigned int atoi(char *s)
{
	char *p = s;
	char c;
	unsigned long ret = 0;

	if (s == NULL)
		return 0;
	while ((c = *p++) != '\0') {
		if (c >= '0' && c <= '9') {
			ret *= 10;
			ret += (unsigned long)(unsigned int)((unsigned char)c - '0');
			if (ret > U32_MAX)
				return 0;
		} else {
			break;
		}
	}
	return (unsigned int) ret;
}

/*
 * Search for files under the folder, return the number of searched and tag
 */
static void search_file_in_dir(const char *path, unsigned int *count, unsigned short tag[])
{
	mm_segment_t old_fs;
	long fd = -1;
	long nread = 0;
	long bpos = 0;
	char *buf = NULL;
	struct linux_dirent *d = NULL;
	char d_type;
	char *pstr = NULL;
	unsigned int i = 0;

	if (path == NULL) {
		pr_err("[%s]: path is NULL\n", __func__);
		return;
	}
	pr_info("playback: %s enter\n", __func__);
	buf = kmalloc((size_t)PLAYBACK_MAX_PATH_LEN, GFP_KERNEL);
	if (buf == NULL) {
		pr_err("playback:fail to kmalloc\n");
		return;
	}
	(void)memset_s(buf, (size_t)PLAYBACK_MAX_PATH_LEN, 0, (size_t)PLAYBACK_MAX_PATH_LEN);
	old_fs = get_fs(); /*lint !e501*/
	set_fs(KERNEL_DS); /*lint !e501*/
	/* check path , if path isnt exist, return. */
#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	fd = ksys_access(path, 0);
#else
	fd = sys_access(path, 0);
#endif
	if (fd != 0) {
		pr_err("playback:dir doesn't exist %s\n", path);
		goto oper_over2;
	}

#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	fd = ksys_open(path, O_RDONLY, PLAYBACK_FILE_OPER_MODE);
#else
	fd = sys_open(path, O_RDONLY, PLAYBACK_FILE_OPER_MODE);
#endif
	if (fd < 0) {
		pr_err("playback:fail to open dir path %s,  fd %ld\n", path, fd);
		goto oper_over2;
	}

#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	nread = ksys_getdents((unsigned int)fd, (struct linux_dirent *)buf, PLAYBACK_MAX_PATH_LEN);
#else
	nread = sys_getdents((unsigned int)fd, (struct linux_dirent *)buf, PLAYBACK_MAX_PATH_LEN);
#endif
	if (nread == -1) {
		pr_err("playback:fail to getdents\n");
		goto oper_over1;
	}

	for (bpos = 0; bpos < nread;) {
		d = (struct linux_dirent *)(buf + bpos);
		d_type = *(buf + bpos + d->d_reclen - 1);
		if (!strncmp((const char *)d->d_name, (const char *)"..", sizeof("..")) ||
			!strncmp((const char *)d->d_name, (const char *)".", sizeof("."))) {
			bpos += d->d_reclen;
			continue;
		}

		if (d_type != DT_DIR)  {
			pstr = strchr(d->d_name, '_');
			if (pstr != NULL) {
				tag[i] = (unsigned short)atoi(pstr + 1);
				pr_err("playback:file %s:%d\n", d->d_name, tag[i]);
				i++;
				if (i >= MAX_SUPPORT_TAG_COUNT) {
					pr_err("playback:Surpport tag count overflow\n");
					goto oper_over1;
				}
			}
		}
		bpos += d->d_reclen;
	}

oper_over1:
#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	ksys_close(fd);
#else
	sys_close(fd);
#endif
oper_over2:
	kfree(buf);
	set_fs(old_fs);
	*count = i;
}

/* Save data to file */
static long save_data_to_file(char *name, const char *databuf, size_t count)
{
	int flags;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t file_size;
	long ret;
	long fd = -1;

	if ((name == NULL) || (databuf == NULL) || (count == 0))
		return 0;
	pr_info("playback %s():name:%s count:%ld.\n", __func__, name, count);
	old_fs = get_fs(); /*lint !e501*/
	set_fs(KERNEL_DS); /*lint !e501*/

#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	fd = ksys_access(g_playback_dev.current_path, 0);
#else
	fd = sys_access(g_playback_dev.current_path, 0);
#endif
	if (fd != 0) {
#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
		fd  = ksys_mkdir(g_playback_dev.current_path, PLAYBACK_FILE_WRITE_MODE);
#else
		fd  = sys_mkdir(g_playback_dev.current_path, PLAYBACK_FILE_WRITE_MODE);
#endif
		if (fd < 0) {
			pr_err("playback %s():create dir err:%s, ret=%ld\n", __func__, g_playback_dev.current_path, fd);
			set_fs(old_fs);
			return -1;
		}
	}

	flags = O_CREAT | O_RDWR | O_APPEND;
	fp = filp_open(name, flags, PLAYBACK_FILE_WRITE_MODE);
	if (fp == NULL || IS_ERR(fp)) {
		set_fs(old_fs);
		pr_err("playback:%s():create %s err.\n", __func__, name);
		return -1;
	}
	file_size = vfs_llseek(fp, 0L, SEEK_END);
	if (file_size > MAX_FILE_SIZE)
		pr_err("%s():file size is overflow.\n", __func__);

	ret = vfs_write(fp, databuf, count, &(fp->f_pos));
	if (ret != count)
		pr_err("playback:%s():write file exception with ret %ld.\n", __func__, ret);

	filp_close(fp, NULL);
	set_fs(old_fs);
	return ret;
}

/*
 * Read data from file
 */
static long read_data_form_file(char *name, char *databuf, unsigned int maxcount, size_t offset)
{
	int flags;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	size_t size;
	char *data = NULL;
	long fd = -1;
	unsigned int count = 0;
	long ret;

	if ((name == NULL) || (databuf == NULL) || (maxcount == 0))
		return 0;
	pr_info("playback %s():name:%s count:%d.\n", __func__, name, maxcount);
	old_fs = get_fs(); /*lint !e501*/
	set_fs(KERNEL_DS); /*lint !e501*/

#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	fd = ksys_access(name, 0);
#else
	fd = sys_access(name, 0);
#endif
	if (fd != 0) {
		pr_err("playback:file not exist:%s.%ld\n", name, fd);
		set_fs(old_fs);
		return -1;
	}

	flags = O_RDONLY;
	fp = filp_open(name, flags, PLAYBACK_FILE_READ_MODE);
	if (fp == NULL || IS_ERR(fp)) {
		set_fs(old_fs);
		pr_err("playback:open file err.%s.%pK\n", name, fp);
		return -1;
	}

	if (vfs_llseek(fp, (loff_t)offset, SEEK_SET) < 0) {
		pr_err("playback:large than file size %ld\n", offset);
		goto err;
	}
	data = databuf;
	do {
		size = PLAYBACK_UINT_SIZE;
		if (count + size > maxcount)
			break;
		ret = vfs_read(fp, data, size, &fp->f_pos);
		if (ret != size) {
			pr_err("playback:%s():read file exception with ret %ld:%lld.\n", __func__, ret, fp->f_pos);
			goto err;
		}

		size = *(unsigned short *)(data + PLAYBACK_USHORT_SIZE);
		if (count + size + PLAYBACK_UINT_SIZE > maxcount)
			break;
		data += PLAYBACK_UINT_SIZE;
		ret = vfs_read(fp, data, size, &fp->f_pos);
		if (ret != size) {
			pr_err("playback:%s():read file exception with ret %ld:%lld.\n", __func__, ret, fp->f_pos);
			goto err;
		}
		data += size;
		count += size + PLAYBACK_UINT_SIZE;
	} while (1);

err:
	filp_close(fp, NULL);
	set_fs(old_fs);
	return count;
}

/*
 * Driver initialization IOCTL processing function
 */
static long playback_init_cmd(unsigned long arg)
{
	struct app_init_cmd_t init_cmd;
	unsigned int count;
	struct playback_info_t *pinfo = NULL;
	unsigned int size;
	unsigned short tag[MAX_SUPPORT_TAG_COUNT] = {0};
	char *pbuf = NULL;
	struct read_info rd;
	int ret;

	if (arg == 0) {
		pr_err("%s invalid user pointer\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&init_cmd, (void *)((uintptr_t)arg), (size_t)sizeof(struct app_init_cmd_t))) {
		pr_err("%s flp_ioctl copy_from_user error\n", __func__);
		return -EFAULT;
	}
	if (init_cmd.path == NULL) {
		pr_err("%s init_cmd.path is null\n", __func__);
		return -EFAULT;
	}

	ret = strcpy_s(g_playback_dev.current_path, sizeof(g_playback_dev.current_path) - 1, PLAYBACK_PATH);
	if (ret != EOK) {
		pr_err("%s flp_ioctl copy path error\n", __func__);
		return -EFAULT;
	}
	if (strnlen(g_playback_dev.current_path, (size_t)PLAYBACK_MAX_PATH_LEN) == PLAYBACK_MAX_PATH_LEN)  {
		pr_err("%s path in overflow\n", __func__);
		return -EFAULT;
	}
	if (init_cmd.mode != 0) {
		g_playback_dev.current_count = MAX_SUPPORT_TAG_COUNT;
	} else {
		search_file_in_dir(g_playback_dev.current_path, &g_playback_dev.current_count, tag);
		if (!g_playback_dev.current_count) {
			pr_err("playback:%s no file\n", __func__);
			return -EFAULT;
		}
	}
	/* if dispose IOCTL_PLAYBACK_INIT cmd repeatly, just free memory that malloc in last time */
	if (g_playback_dev.info != NULL) {
		kfree(g_playback_dev.info);
		pr_err("%s reinit without release\n", __func__);
	}
	pinfo = kmalloc(g_playback_dev.current_count*sizeof(struct playback_info_t), GFP_KERNEL);
	if (pinfo == NULL) {
		pr_err("%s no memory\n", __func__);
		return -ENOMEM;
	}
	(void)memset_s(pinfo, g_playback_dev.current_count*sizeof(struct playback_info_t), 0, g_playback_dev.current_count*sizeof(struct playback_info_t));
	pbuf = kmalloc((size_t)MAX_PKT_LENGTH, GFP_KERNEL);
	if (pbuf == NULL) {
		pr_err("%s no memory\n", __func__);
		kfree(pinfo);
		return -ENOMEM;
	}
	(void)memset_s(pbuf, (size_t)MAX_PKT_LENGTH, 0, (size_t)MAX_PKT_LENGTH);
	g_playback_dev.info = pinfo;
	g_playback_dev.current_mode = init_cmd.mode;
	*pbuf = (char)init_cmd.mode;
	size = (PLAYBACK_SHARE_MEM_SIZE/g_playback_dev.current_count) & ~(PLAYBACK_SIZE_1K - 1);
	for (count = 0; count < g_playback_dev.current_count; count++) {
		if (init_cmd.mode != 0)
			pinfo->tag_id = count;
		else
			pinfo->tag_id = tag[count];

		pinfo->buf1_addr = PLAYBACK_SHARE_MEM_BASEADDR + count * size;
		pinfo->buf2_addr = PLAYBACK_SHARE_MEM_BASEADDR + count * size + (size >> 1);
		pinfo->buf_size = size >> 1;
		pinfo->file_offset = 0;
		ret = memcpy_s((pbuf + 1 + count * sizeof(struct sm_info_t)),
				(MAX_PKT_LENGTH - 1 - count * sizeof(struct sm_info_t)), pinfo, sizeof(struct sm_info_t));
		if (ret != EOK)
			pr_err("%s memcpy buf fail, ret[%d]\n", __func__, ret);
		pinfo++;
	}
	ret = send_cmd_from_kernel_response(TAG_DATA_PLAYBACK, CMD_CMN_OPEN_REQ,
		0, (char *)pbuf, count * sizeof(struct sm_info_t) + 1, &rd);
	if (ret == 0)
		g_playback_dev.status |= FUNCTION_INIT;
	kfree(pbuf);

	reinit_completion(&g_playback_dev.done);
	return ret;
}

/* Start data recording or playback */
static long playback_start_cmd(void)
{
	unsigned int count;
	struct data_status_t *pbuf = NULL;
	struct buf_status_t *pstatus = NULL;
	struct playback_info_t *info = g_playback_dev.info;
	char buf[MAX_PKT_LENGTH] = {0};
	int ret;

	if (FUNCTION_START == (g_playback_dev.status & FUNCTION_START)) {
		pr_err("%s data playback start aready\n", __func__);
		return -EFAULT;
	}

	/* 1:record 0:playback */
	if (g_playback_dev.current_mode) {
		pstatus = (struct buf_status_t *)buf;
		for (count = 0; count < g_playback_dev.current_count; count++) {
			pstatus->tag_id = info->tag_id;
			pstatus->buf_status = BUFFER_ALL_READY;
			pstatus++;
			info++;
		}
		return send_cmd_from_kernel(TAG_DATA_PLAYBACK, CMD_CMN_CONFIG_REQ,
			CMD_DATA_PLAYBACK_BUF_READY_REQ, (char *)buf, count * sizeof(struct buf_status_t));
	} else {
		pbuf = (struct data_status_t *)buf;
		for (count = 0; count < g_playback_dev.current_count; count++) {
			pbuf->tag_id = info->tag_id;
			pbuf->buf_status = BUFFER_ALL_READY;
			ret = snprintf_s(g_playback_dev.filename, PLAYBACK_MAX_PATH_LEN, (size_t)(PLAYBACK_MAX_PATH_LEN - 1),
							"%s/data_%d.bin", g_playback_dev.current_path, info->tag_id);
			if (ret < 0)  {
				pr_err("%s path in overflow\n", __func__);
				return -EFAULT;
			}
			pbuf->data_size[0] = (unsigned int)read_data_form_file(g_playback_dev.filename,
								((char *)g_playback_dev.vaddr + info->buf1_addr - g_playback_dev.phy_addr),
								info->buf_size, (size_t)info->file_offset);
			info->file_offset += pbuf->data_size[0];
			pbuf->data_size[1] = (unsigned int)read_data_form_file(g_playback_dev.filename,
											 ((char *)g_playback_dev.vaddr + info->buf2_addr - g_playback_dev.phy_addr),
											info->buf_size, (size_t)info->file_offset);
			info->file_offset += pbuf->data_size[1];
			pr_info("playback pbuf->data_size[0]:%d, pbuf->data_size[1]:%d, file_offset:%d\n",
					pbuf->data_size[0], pbuf->data_size[1], info->file_offset);
			pbuf++;
			info++;
		}
		return send_cmd_from_kernel(TAG_DATA_PLAYBACK, CMD_CMN_CONFIG_REQ,
			CMD_DATA_PLAYBACK_DATA_READY_REQ, (char *)buf, (size_t)(count * sizeof(struct data_status_t)));
	}
}

/*
 * Data recording or playback workqueue
 */
static void playback_work(struct work_struct *wk)
{
	mutex_lock(&g_playback_dev.mutex_lock);
	if (g_playback_dev.data == IOCTL_PLAYBACK_START) {
		playback_start_cmd();
		g_playback_dev.status |= FUNCTION_START;
		g_playback_dev.data = 0;
	}
	mutex_unlock(&g_playback_dev.mutex_lock);
}

/*
 * Process the playback completion message sent by the contexthub side, read new data from the file and put it on the DDR. Only one buf will be empty at a time
 */
static int handle_notify_from_mcu_playback(const struct pkt_header *head)
{
	struct data_status_t *pstatus = (struct data_status_t  *) (head + 1);
	struct playback_info_t *pinfo = NULL;
	char buf[MAX_PKT_LENGTH] = {0};
	struct data_status_t *pbuf = (struct data_status_t *)buf;
	unsigned int count;
	int ret;

	pr_info("%s mode:%d length:%d tag:%d\n", __func__, g_playback_dev.current_count,
			head->length, pstatus->tag_id);
	pinfo = g_playback_dev.info;
	for (count = 0; count < g_playback_dev.current_count; count++) {
		if (pinfo->tag_id == pstatus->tag_id)
			break;
		pinfo++;
	}

	if (count == g_playback_dev.current_count) {
		pr_err("%s tag_id error for mcu\n", __func__);
		return -EFAULT;
	}
	pbuf->tag_id = pstatus->tag_id;
	/* 0:playback; handle playback */
	/* if IOCTL_PLAYBACK_STOP, just close */
	if (FUNCTION_START != (g_playback_dev.status & FUNCTION_START)) {
		pr_err("%s data playback stop aready\n", __func__);
		return -EFAULT;
	}
	if ((completion_done(&g_playback_dev.done) == 0) &&
		((!pstatus->data_size[0] && BUFFER1_READY == pstatus->buf_status) ||
		(!pstatus->data_size[1] && BUFFER2_READY == pstatus->buf_status))) {
		g_playback_dev.complete_status = COMPLETE_REPLAY_DONE;
		complete(&g_playback_dev.done);
	}
	ret = snprintf_s(g_playback_dev.filename, PLAYBACK_MAX_PATH_LEN, (PLAYBACK_MAX_PATH_LEN - 1),
					"%s/data_%d.bin", g_playback_dev.current_path, pstatus->tag_id);
	if (ret < 0)  {
		pr_err("%s path in overflow\n", __func__);
		return -EFAULT;
	}
	if (pstatus->buf_status == BUFFER1_READY) {
		count = (unsigned int)read_data_form_file(g_playback_dev.filename,
										((char *)g_playback_dev.vaddr + pinfo->buf1_addr - g_playback_dev.phy_addr),
										pinfo->buf_size, (size_t)pinfo->file_offset);
		pbuf->data_size[0] = count;
		pinfo->file_offset += count;
		pbuf->buf_status = BUFFER1_READY;
	} else if (pstatus->buf_status == BUFFER2_READY) {
		count = (unsigned int)read_data_form_file(g_playback_dev.filename,
										((char *)g_playback_dev.vaddr + pinfo->buf2_addr - g_playback_dev.phy_addr),
										pinfo->buf_size, (size_t)pinfo->file_offset);
		pbuf->data_size[1] = count;
		pinfo->file_offset += count;
		pbuf->buf_status = BUFFER2_READY;
	} else {
		pr_err("%s Buf Ready at the same time, not support\n", __func__);
		return -EFAULT;
	}
	return send_cmd_from_kernel(TAG_DATA_PLAYBACK, CMD_CMN_CONFIG_REQ,
					CMD_DATA_PLAYBACK_DATA_READY_REQ, (char *)pbuf, sizeof(struct data_status_t));
}

/*
 * Process the buf full message on the DDR sent by the contexthub side, and save the data to a file. Only one buf needs to be processed at a time
 */
static int handle_notify_from_mcu_record(const struct pkt_header *head)
{
	struct data_status_t *pstatus = (struct data_status_t  *) (head + 1);
	char *data = NULL;
	struct playback_info_t *pinfo = NULL;
	char buf[MAX_PKT_LENGTH] = {0};
	struct data_status_t *pbuf = (struct data_status_t *)buf;
	unsigned int count;
	unsigned int data_len;

	int ret;

	pr_err("%s mode:%d length:%d tag:%d\n", __func__, g_playback_dev.current_count,
			head->length, pstatus->tag_id);
	pinfo = g_playback_dev.info;
	for (count = 0; count < g_playback_dev.current_count; count++) {
		if (pinfo->tag_id == pstatus->tag_id)
			break;
		pinfo++;
	}

	if (count == g_playback_dev.current_count) {
		pr_err("%s tag_id error for mcu\n", __func__);
		return -EFAULT;
	}
	pbuf->tag_id = pstatus->tag_id;
	/* 1:record; handle record */
	if (pstatus->buf_status == BUFFER1_READY) {
		data = (char *)g_playback_dev.vaddr + pinfo->buf1_addr - g_playback_dev.phy_addr;
		data_len = pstatus->data_size[0];
		pbuf->buf_status = BUFFER1_READY;
	} else if (pstatus->buf_status == BUFFER2_READY) {
		data = (char *)g_playback_dev.vaddr + pinfo->buf2_addr - g_playback_dev.phy_addr;
		data_len = pstatus->data_size[1];
		pbuf->buf_status = BUFFER2_READY;
	} else {
		pr_err("%s Buf Ready at the same time, not support\n", __func__);
		return -EFAULT;
	}
	ret = snprintf_s(g_playback_dev.filename, PLAYBACK_MAX_PATH_LEN, (PLAYBACK_MAX_PATH_LEN - 1),
					"%s/data_%d.bin", g_playback_dev.current_path, *((unsigned short *)data));
	if (ret < 0)  {
		pr_err("%s path in overflow\n", __func__);
		return -EFAULT;
	}
	save_data_to_file(g_playback_dev.filename, data, (size_t)data_len);
	return send_cmd_from_kernel(TAG_DATA_PLAYBACK, CMD_CMN_CONFIG_REQ,
					CMD_DATA_PLAYBACK_BUF_READY_REQ, (char *)pbuf, sizeof(struct buf_status_t));
}

/*
 * Process messages sent from the contexthub side
 */
static int playback_notify_from_mcu(const struct pkt_header *head)
{
	int ret;

	if (head == NULL) {
		pr_err("%s head is null\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&g_playback_dev.mutex_lock);

	/* if release, just return */
	if (g_playback_dev.info == NULL) {
		mutex_unlock(&g_playback_dev.mutex_lock);
		pr_err("%s dev have closed\n", __func__);
		return -EFAULT;
	}

	/* 1:record 0:playback */
	if (g_playback_dev.current_mode)
		ret = handle_notify_from_mcu_record(head);
	else
		ret = handle_notify_from_mcu_playback(head);

	mutex_unlock(&g_playback_dev.mutex_lock);
	return ret;
}

/*
 * Process IOCTL messages sent by the AP side
 */
static long playback_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	pr_info("%s : 0X%x\n", __func__, cmd);
	switch (cmd) {
	case IOCTL_PLAYBACK_INIT:
		if (arg == 0) {
			pr_err("%s arg is null\n", __func__);
			return -EFAULT;
		}
		mutex_lock(&g_playback_dev.mutex_lock);
		ret = playback_init_cmd(arg);
		mutex_unlock(&g_playback_dev.mutex_lock);
		break;
	case IOCTL_PLAYBACK_START:
		mutex_lock(&g_playback_dev.mutex_lock);
		if ((FUNCTION_INIT != (g_playback_dev.status & FUNCTION_INIT)) ||
			(FUNCTION_START == (g_playback_dev.status & FUNCTION_START))) {
			mutex_unlock(&g_playback_dev.mutex_lock);
			pr_err("%s data playback START aready\n", __func__);
			return -EFAULT;
		}
		g_playback_dev.data = IOCTL_PLAYBACK_START;
		queue_work(system_power_efficient_wq, &g_playback_dev.work);
		mutex_unlock(&g_playback_dev.mutex_lock);
		break;
	case IOCTL_PLAYBACK_STOP:
		mutex_lock(&g_playback_dev.mutex_lock);
		if (g_playback_dev.status & FUNCTION_START) {
			ret = (long)send_cmd_from_kernel(TAG_DATA_PLAYBACK, CMD_CMN_CLOSE_REQ, 0, NULL, 0UL);
			g_playback_dev.status &= ~FUNCTION_START;
		}
		mutex_unlock(&g_playback_dev.mutex_lock);
		break;
	case IOCTL_PLAYBACK_REPLAY_COMPLETE:
		ret = wait_for_completion_interruptible(&g_playback_dev.done);
		if (ret != 0) {
			pr_err("%s  wait_for_completion_interruptible error\n", __func__);
			return -EFAULT;
		}
		if (arg == 0) {
			pr_err("%s arg is null\n", __func__);
			return -EFAULT;
		}
		mutex_lock(&g_playback_dev.mutex_lock);
		if (copy_to_user((void *)((uintptr_t)arg), &g_playback_dev.complete_status, (size_t)sizeof(int))) {
			mutex_unlock(&g_playback_dev.mutex_lock);
			pr_err("%s playback: copy_from_user error\n", __func__);
			return -EFAULT;
		}
		mutex_unlock(&g_playback_dev.mutex_lock);
		pr_info("%s  replay complete\n", __func__);
		break;
	default:
		pr_err("%s  cmd not surpport\n", __func__);
		return -EFAULT;
	}
	return ret;
}

/*
 * Device node open processing function
 */
static int playback_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&g_playback_dev.mutex_lock);
	g_playback_dev.status = FUNCTION_OPEN;
	g_playback_dev.info = NULL;
	mutex_unlock(&g_playback_dev.mutex_lock);
	pr_info("%s %d: enter\n", __func__, __LINE__);
	return 0;
}

/*
 * Device node close processing function
 */
static int playback_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	mutex_lock(&g_playback_dev.mutex_lock);
	if (g_playback_dev.status == 0) {
		mutex_unlock(&g_playback_dev.mutex_lock);
		return 0;
	} else if (g_playback_dev.status & FUNCTION_START) {
		ret = send_cmd_from_kernel(TAG_DATA_PLAYBACK, CMD_CMN_CLOSE_REQ, 0, NULL, 0UL);
	}
	g_playback_dev.status = 0;
	if (g_playback_dev.info != NULL) {
		kfree(g_playback_dev.info);
		g_playback_dev.info = NULL;
	}
	if (completion_done(&g_playback_dev.done) == 0) {
		g_playback_dev.complete_status = COMPLETE_CLOSE;
		complete(&g_playback_dev.done);
	}
	pr_info("%s %d: close\n", __func__, __LINE__);
	mutex_unlock(&g_playback_dev.mutex_lock);
	return ret;
}

static const struct file_operations playback_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = playback_ioctl,
	.open =	playback_open,
	.release = playback_release,
};

/* Description:miscdevice to data playback function */
static struct miscdevice playback_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name =	 "playback",
	.fops =	 &playback_fops,
	.mode =	 0660,
};

/*
 * Drive probe function. Initialize internal resources, register for contexthub message notification
 */
static int generic_playback_probe(struct platform_device *pdev)
{
	int ret;
	char *status = NULL;
	struct device_node *np = pdev->dev.of_node;

	ret = get_contexthub_dts_status();
	if (ret)
		return ret;
	dev_info(&pdev->dev, "generic playback probe\n");
	ret = of_property_read_string(np, "status", (const char **)&status);
	if (ret == 0) {
		if (!strncmp("disabled", status, strlen("disabled"))) {
			pr_err("cannot register playback driver, dts status setting is disabled\n");
			return -EPERM;
		}
	}
	ret = of_property_read_u32(np, "playback_mem_addr", &g_playback_dev.phy_addr);
	if (ret < 0)
		g_playback_dev.phy_addr = PLAYBACK_SHARE_MEM_BASEADDR;
	ret = of_property_read_u32(np, "playback_mem_size", &g_playback_dev.size);
	if (ret < 0)
		g_playback_dev.size = PLAYBACK_SHARE_MEM_SIZE;

	g_playback_dev.vaddr = ioremap_wc((size_t)g_playback_dev.phy_addr, g_playback_dev.size);
	if (g_playback_dev.vaddr == NULL) {
		pr_err("[%s] ioremap err\n", __func__);
		return -ENOMEM;
	}
	mutex_init(&g_playback_dev.mutex_lock);
	init_completion(&g_playback_dev.done);
	INIT_WORK(&g_playback_dev.work, playback_work);
	register_mcu_event_notifier(TAG_DATA_PLAYBACK, CMD_DATA_PLAYBACK_DATA_READY_RESP, playback_notify_from_mcu);
	register_mcu_event_notifier(TAG_DATA_PLAYBACK, CMD_DATA_PLAYBACK_BUF_READY_RESP, playback_notify_from_mcu);
	ret = misc_register(&playback_miscdev);
	if (ret != 0)
		pr_err("cannot register playback err=%d\n", ret);
	return ret;
}

/*
 * Drive the remove function. Release internal resources and register for contexthub message notification
 */
static int generic_playback_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s %d:\n", __func__, __LINE__);
	unregister_mcu_event_notifier(TAG_DATA_PLAYBACK, CMD_DATA_PLAYBACK_DATA_READY_RESP, playback_notify_from_mcu);
	unregister_mcu_event_notifier(TAG_DATA_PLAYBACK, CMD_DATA_PLAYBACK_BUF_READY_RESP, playback_notify_from_mcu);
	misc_deregister(&playback_miscdev);
	cancel_work_sync(&g_playback_dev.work);
	iounmap(g_playback_dev.vaddr);
	return 0;
}

static const struct of_device_id generic_playback[] = {
	{ .compatible = "hisilicon,Contexthub-playback" },
	{},
};
MODULE_DEVICE_TABLE(of, generic_playback);
static struct platform_driver generic_playback_platdrv = {
	.driver = {
		.name   = "playback",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(generic_playback),
	},
	.probe = generic_playback_probe,
	.remove	= generic_playback_remove,
};

static int __init playback_init(void)
{
	return platform_driver_register(&generic_playback_platdrv);
}

static void __exit playback_exit(void)
{
	platform_driver_unregister(&generic_playback_platdrv);
}

late_initcall_sync(playback_init);
module_exit(playback_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("Generic smart playback driver via DT");
MODULE_LICENSE("GPL");

