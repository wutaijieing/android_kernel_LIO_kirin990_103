/*
 * drv_tele_mntn_gut.c
 *
 * tele mntn module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#include <drv_tele_mntn_gut.h>
#include <linux/tele_mntn.h>
#include <securec.h>
#define PC_UT_TEST_BUF	64
#define TELE_MNTN_PROTECT_CNT	2
#define TELE_MNTN_CONTROLLABLE_HEAD_SIZE	(2 * sizeof(unsigned int))

/* control handle */
TELE_MNTN_DATA_SECTION struct tele_mntn_handle g_tele_mntn_handle = {0};
static struct tele_mntn_cfg g_tele_mntn_cfg_tbl = {
	TELE_MNTN_AREA_ADDR,
	TELE_MNTN_AREA_SIZE,
	TELE_MNTN_AREA_SIZE * 3 / 4, /* wake threshold */
	600, /* wake interval threshold */
	64, /* user data threshold */
	0xFFFFFFFF, /* unused */
	24, /* IPC_SEM_TELE_MNTN */
};

static int tele_mntn_init_cfg(void);
static int tele_mntn_write_buf(const struct tele_mntn_data *head_data,
			       const void *data, unsigned int len,
			       enum tele_mntn_buf_type buf_type);

int tele_mntn_init(void)
{
	int ret;
	unsigned int pro_size, queue_size, pro_align, queue_align;

	if (g_tele_mntn_handle.init_flag == TELE_MNTN_INIT_MAGIC) {
		ret = TELE_MNTN_ALREADY_INIT;
		return ret;
	}
	ret = memset_s((void *)&g_tele_mntn_handle,
		       sizeof(struct tele_mntn_handle),
		       0x0,
		       sizeof(struct tele_mntn_handle));
	if (ret != EOK) {
		pr_err("%s:%d, memset_s err:%d\n",
		       __func__, __LINE__, ret);
		return ret;
	}
	ret = tele_mntn_common_init();
	if (ret != TELE_MNTN_OK) {
		pr_err("%s: ret=%u, para0=%u\r\n", __func__,
		       (unsigned int)ret, 0U);
		return ret;
	}
	ret = tele_mntn_init_cfg();
	if (ret != TELE_MNTN_OK) {
		pr_err("%s: ret=%u, para0=%u\r\n", __func__,
		       (unsigned int)ret, 0U);
		return ret;
	}
	pro_size = TELE_MNTN_PROTECT_CNT * TELE_MNTN_PROTECT_LEN;
	pro_align = TELE_MNTN_ALIGN(pro_size, TELE_MNTN_ALIGN_SIZE);
	queue_size = TELE_MNTN_HEAD_SIZE + pro_size;
	queue_align = TELE_MNTN_ALIGN(queue_size, TELE_MNTN_ALIGN_SIZE);
	g_tele_mntn_handle.queue.base = g_tele_mntn_handle.data_virt_addr + queue_align;
	g_tele_mntn_handle.queue.length =
		g_tele_mntn_cfg_tbl.data_size - queue_align - pro_align;
	spin_lock_init(&g_tele_mntn_handle.local_lock);
	g_tele_mntn_handle.global_lock =
		(unsigned char *)hwspin_lock_request_specific(g_tele_mntn_cfg_tbl.global_lock_num);
	if (g_tele_mntn_handle.global_lock == NULL)
		return TELE_MNTN_ERRO;
	g_tele_mntn_handle.init_flag = TELE_MNTN_INIT_MAGIC;
	return ret;
}

void tele_mntn_exit(void)
{
	if (g_tele_mntn_handle.data_virt_addr != NULL) {
		iounmap(g_tele_mntn_handle.data_virt_addr);
		g_tele_mntn_handle.data_virt_addr = NULL;
	}
	tele_mntn_common_exit();
}

static int tele_mntn_init_cfg(void)
{
	int ret = TELE_MNTN_OK;

	g_tele_mntn_handle.data_virt_addr =
		(unsigned char *)ioremap(g_tele_mntn_cfg_tbl.data_addr, g_tele_mntn_cfg_tbl.data_size);
	if (g_tele_mntn_handle.data_virt_addr == NULL) {
		ret = TELE_MNTN_MUTEX_VIRT_ADDR_NULL;
		return ret;
	}
	return ret;
}

int tele_mntn_write_log(enum tele_mntn_type_id type_id, unsigned int len,
			const void *data)
{
	int ret = TELE_MNTN_ERRO;
	unsigned int data_len;
	spinlock_t *lock = NULL;
	unsigned long flag;
	struct tele_mntn_data *cur_data = &g_tele_mntn_handle.data;
	struct tele_mntn_handle *cur_handle = &g_tele_mntn_handle;
	struct tele_mntn_cfg *cur_table = &g_tele_mntn_cfg_tbl;
	enum tele_mntn_buf_type buf_type = TELE_MNTN_BUF_INCONTROLLABLE;

	lock = &cur_handle->local_lock;
	flag = cur_handle->local_flag;
	if (cur_handle->init_flag != TELE_MNTN_INIT_MAGIC) {
		ret = TELE_MNTN_NOT_INIT;
		return ret;
	}
	if (!tele_mntn_is_func_on(type_id))
		return TELE_MNTN_FUNC_OFF;
	if (tele_mntn_is_func_on(TELE_MNTN_NVME_LOGCAT))
		buf_type = TELE_MNTN_BUF_CONTROLLABLE;
	if (len > cur_table->user_data_threshold) {
		ret = TELE_MNTN_USER_DATA_OVERFLOW;
		return ret;
	}
	ret = memset_s((void *)cur_data, sizeof(struct tele_mntn_data), 0,
		       sizeof(struct tele_mntn_data));
	if (ret != EOK) {
		pr_err("%s:%d memset_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	spin_lock_irqsave(lock, flag);
	ret = hwspin_lock_timeout((struct hwspinlock *)(cur_handle->global_lock),
				  TELE_MNTN_LOCK_TIMEOUT);
	if (ret != 0) {
		spin_unlock_irqrestore((spinlock_t *)(uintptr_t)lock, flag);
		return TELE_MNTN_LOCK_FAIL;
	}
	data_len = TELE_MNTN_ALIGN(len, TELE_MNTN_ALIGN_SIZE);
	cur_data->protect = TELE_MNTN_PROTECT;
	cur_data->type_id = (unsigned int)type_id;
	cur_data->len = data_len;
	cur_data->idex = cur_handle->idex++;
	cur_data->cpu_id = TELE_MNTN_CUR_CPUID;
	cur_data->rtc = get_rtc_time();
	cur_data->slice = get_slice_time();
	ret = tele_mntn_write_buf(cur_data, data, data_len, buf_type);
	if (ret != TELE_MNTN_OK) {
		hwspin_unlock((struct hwspinlock *)(cur_handle->global_lock));
		spin_unlock_irqrestore(lock, flag);
		return ret;
	}
	hwspin_unlock((struct hwspinlock *)(cur_handle->global_lock));
	spin_unlock_irqrestore(lock, flag);

	return TELE_MNTN_OK;
}

static int tele_mntn_queue_set(const struct tele_mntn_data *head_data,
			       struct tele_mntn_head *head,
			       unsigned int len,
			       const void *data,
			       enum tele_mntn_buf_type buf_type)
{
	struct tele_mntn_queue *cur_queue = &g_tele_mntn_handle.queue;
	unsigned int size_to_bottom = 0;
	unsigned int offset;
	int ret;

	offset = head->front * TELE_MNTN_ALIGN_SIZE;
	if (cur_queue->base + offset < cur_queue->base ||
	    (unsigned long)(uintptr_t)cur_queue->base + offset < offset)
		return -EINVAL;
	cur_queue->front = cur_queue->base + offset;
	if (buf_type == TELE_MNTN_BUF_CONTROLLABLE) {
		offset = head->rear * TELE_MNTN_ALIGN_SIZE;
		if (offset < head->rear ||
		    cur_queue->base + offset < cur_queue->base ||
		    (unsigned long)(uintptr_t)cur_queue->base + offset < offset)
			return -EINVAL;
		cur_queue->rear = cur_queue->base + offset;
	} else {
		cur_queue->rear = cur_queue->front;
	}
	if (cur_queue->front >= cur_queue->rear &&
	    cur_queue->base + cur_queue->length >= cur_queue->base &&
	    (unsigned long)(uintptr_t)cur_queue->base + cur_queue->length >= cur_queue->length &&
	    cur_queue->base + cur_queue->length >= cur_queue->front) {
		size_to_bottom =
			(unsigned int)(cur_queue->base + cur_queue->length - cur_queue->front);
		if (size_to_bottom <= len + TELE_MNTN_DATA_SIZE)
			cur_queue->front = cur_queue->base;
	}
	if (cur_queue->front < cur_queue->rear &&
	    (unsigned long)(uintptr_t)(cur_queue->rear - cur_queue->front) >= TELE_MNTN_ALIGN_SIZE)
		size_to_bottom =
			(unsigned int)(cur_queue->rear - cur_queue->front) - TELE_MNTN_ALIGN_SIZE;
	if (size_to_bottom <= len + TELE_MNTN_DATA_SIZE)
		return TELE_MNTN_BUF_FULL;
	ret = memcpy_s((void *)cur_queue->front, TELE_MNTN_DATA_SIZE,
		       (void *)head_data, TELE_MNTN_DATA_SIZE);
	if (ret != EOK) {
		pr_err("%s:%d memcpy_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (cur_queue->front + TELE_MNTN_DATA_SIZE > cur_queue->front &&
	    (unsigned long)(uintptr_t)cur_queue->front + TELE_MNTN_DATA_SIZE > TELE_MNTN_DATA_SIZE)
		cur_queue->front += TELE_MNTN_DATA_SIZE;
	ret = memcpy_s((void *)cur_queue->front, len, (void *)data, len);
	if (ret != EOK) {
		pr_err("%s:%d memcpy_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	return ret;
}

static int tele_mntn_write_buf(const struct tele_mntn_data *head_data,
			       const void *data, unsigned int len,
			       enum tele_mntn_buf_type buf_type)
{
	struct tele_mntn_queue *cur_queue = &g_tele_mntn_handle.queue;
	struct tele_mntn_head *head =
		(struct tele_mntn_head *)g_tele_mntn_handle.data_virt_addr;
	struct tele_mntn_head *head_tmp = &g_tele_mntn_handle.head;
	int ret;

	if (TELE_MNTN_CHECK_DDR_SELF_REFRESH != 0)
		return TELE_MNTN_DDR_SELF_REFRESH;
	ret = memset_s((void *)head_tmp, sizeof(struct tele_mntn_head),
		       0x0, sizeof(struct tele_mntn_head));
	if (ret != EOK) {
		pr_err("%s:%d memset_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	ret = memcpy_s((void *)head_tmp, sizeof(struct tele_mntn_head),
		       (void *)head, TELE_MNTN_HEAD_SIZE);
	if (ret != EOK) {
		pr_err("%s:%d memcpy_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (head_tmp->protect_word1 != TELE_MNTN_PROTECT1 ||
	    head_tmp->protect_word2 != TELE_MNTN_PROTECT2)
		return TELE_MNTN_PROTECT_ERR;
	if (head_tmp->front * TELE_MNTN_ALIGN_SIZE < head_tmp->front)
		return -EINVAL;
	ret = tele_mntn_queue_set(head_data, head_tmp, len, data, buf_type);
	if (ret != 0) {
		pr_err("%s:%d set err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (cur_queue->front + len >= cur_queue->front &&
	    (unsigned long)(uintptr_t)cur_queue->front + len >= len &&
	    cur_queue->front >= cur_queue->base) {
		cur_queue->front += len;
		head_tmp->front =
			((unsigned int)(cur_queue->front - cur_queue->base)) / TELE_MNTN_ALIGN_SIZE;
	}
	if (buf_type != TELE_MNTN_BUF_CONTROLLABLE)
		ret = memcpy_s((void *)head, sizeof(struct tele_mntn_head),
			       (void *)head_tmp, TELE_MNTN_HEAD_SIZE);
	else
		ret = memcpy_s((void *)head, sizeof(struct tele_mntn_head),
			       (void *)head_tmp,
			       TELE_MNTN_CONTROLLABLE_HEAD_SIZE);
	if (ret != EOK) {
		pr_err("%s:%d memcpy_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	return TELE_MNTN_OK;
}

int tele_mntn_get_head(struct tele_mntn_queue *queue)
{
	int ret;
	struct tele_mntn_queue *cur_queue = &g_tele_mntn_handle.queue;
	struct tele_mntn_head head_tmp = {0};

	if (queue == NULL) {
		ret = TELE_MNTN_PARA_INVALID;
		return ret;
	}
	if (g_tele_mntn_handle.init_flag != TELE_MNTN_INIT_MAGIC) {
		ret = TELE_MNTN_NOT_INIT;
		return ret;
	}
	ret = memcpy_s((void *)&head_tmp, sizeof(struct tele_mntn_head),
		       (void *)g_tele_mntn_handle.data_virt_addr,
		       TELE_MNTN_HEAD_SIZE);
	if (ret != EOK) {
		pr_err("%s:%d memcpy_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (head_tmp.protect_word1 != TELE_MNTN_PROTECT1 ||
	    head_tmp.protect_word2 != TELE_MNTN_PROTECT2) {
		ret = TELE_MNTN_PROTECT_ERR;
		return ret;
	}
	queue->base = cur_queue->base;
	queue->length = cur_queue->length;
	queue->front = cur_queue->base + head_tmp.front * TELE_MNTN_ALIGN_SIZE;
	queue->rear = cur_queue->base + head_tmp.rear * TELE_MNTN_ALIGN_SIZE;
	return ret;
}

int tele_mntn_set_head(const struct tele_mntn_queue *queue)
{
	int ret;
	struct tele_mntn_head *head =
		(struct tele_mntn_head *)g_tele_mntn_handle.data_virt_addr;
	struct tele_mntn_head head_tmp = {0};

	if (queue == NULL) {
		ret = TELE_MNTN_PARA_INVALID;
		return ret;
	}
	if (g_tele_mntn_handle.init_flag != TELE_MNTN_INIT_MAGIC) {
		ret = TELE_MNTN_NOT_INIT;
		return ret;
	}
	ret = memcpy_s((void *)&head_tmp, sizeof(struct tele_mntn_head),
		       (void *)head, TELE_MNTN_HEAD_SIZE);
	if (ret != EOK) {
		pr_err("%s:%d memcpy_s err:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (head_tmp.protect_word1 != TELE_MNTN_PROTECT1 ||
	    head_tmp.protect_word2 != TELE_MNTN_PROTECT2) {
		ret = TELE_MNTN_PROTECT_ERR;
		return ret;
	}
	head_tmp.rear =
		((unsigned int)(queue->rear - queue->base)) / TELE_MNTN_ALIGN_SIZE;
	ret = memcpy_s((void *)&head->rear, sizeof(head->rear),
		       (void *)&head_tmp.rear, sizeof(head_tmp.rear));

	return ret;
}
