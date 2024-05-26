/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/file.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "mdfx_priv.h"
#include "mdfx_utils.h"
#include "mdfx_file.h"
#include "mdfx_logger.h"

extern struct mdfx_pri_data *g_mdfx_data;

/* human readable text of the record */
static char *msg_text(const struct log_node *msg)
{
	if (IS_ERR_OR_NULL(msg))
		return NULL;

	return (char *)msg + sizeof(struct log_node);
}

/* get record by index; idx must point to valid msg */
static struct log_node *msg_from_idx(char *buf, uint32_t idx)
{
	struct log_node *msg = NULL;

	if (IS_ERR_OR_NULL(buf)) {
		mdfx_pr_err("buf is null, idx=%u", idx);
		return NULL;
	}

	msg = (struct log_node *)(buf + idx);

	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer.
	 */
	if (!msg->len)
		return (struct log_node *)buf;

	return msg;
}

/* get next record; idx must point to valid msg */
static uint32_t msg_next(char *buf, uint32_t idx)
{
	struct log_node *msg = NULL;

	if (IS_ERR_OR_NULL(buf)) {
		mdfx_pr_err("buf is null, idx=%u", idx);
		return 0;
	}

	msg = (struct log_node *)(buf + idx);
	/* length == 0 indicates the end of the buffer; wrap */
	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer as *this* one, and
	 * return the one after that.
	 */
	if (!msg->len) {
		mdfx_pr_info("have arrive at the buffer END, idx=%u", idx);
		msg = (struct log_node *)buf;
		return msg->len;
	}

	return idx + msg->len;
}

static char* mdfx_logger_alloc_log_buf()
{
	return kzalloc(LOGGER_BUF_LEN, GFP_ATOMIC);
}

static struct mdfx_log_buf_t* mdfx_get_using_log_buf(struct mdfx_logger_t *logger)
{
	struct mdfx_log_buf_t* log_buf = NULL;

	if (IS_ERR_OR_NULL(logger))
		return NULL;

	log_buf = &(logger->log_bufs[LOGGER_GET_BUF_INDEX(logger->using_index)]);
	if (IS_ERR_OR_NULL(log_buf->log_buf)) {
		log_buf->first_idx = 0;
		log_buf->next_idx = 0;
		log_buf->visitor_id = logger->visitor_id;
		log_buf->log_buf = mdfx_logger_alloc_log_buf();
	}

	return log_buf;
}

static struct mdfx_log_buf_t *mdfx_get_log_buf(struct mdfx_logger_t *logger, uint32_t index)
{
	if (IS_ERR_OR_NULL(logger))
		return NULL;

	if (index >= LOGGER_BUF_MAX_COUNT) {
		mdfx_pr_err("index is over, index=%u", index);
		return NULL;
	}

	return &(logger->log_bufs[index]);
}

static void mdfx_logger_build_event_name(struct mdfx_log_buf_t *log_buf, const char *event_name)
{
	int ret;
	if (IS_ERR_OR_NULL(log_buf) || IS_ERR_OR_NULL(event_name))
		return;

	ret = snprintf(log_buf->event_name, MDFX_EVENT_NAME_MAX, "%s", event_name);
	if (ret < 0)
		log_buf->event_name[0] = '\0';
	log_buf->event_name[MDFX_EVENT_NAME_MAX - 1] = '\0';
}

static void mdfx_logger_clear_event_name(struct mdfx_log_buf_t *log_buf)
{
	if (IS_ERR_OR_NULL(log_buf))
		return;

	log_buf->event_name[0] = '\0';
}


/*
 * Check whether there is enough free space for the given message.
 *
 * The same values of first_idx and next_idx mean that the buffer
 * is either empty or full.
 *
 * If the buffer is empty, we must respect the position of the indexes.
 * They cannot be reset to the beginning of the buffer.
 */
static int logger_buf_has_space(struct mdfx_log_buf_t *using_buf, uint32_t msg_size)
{
	uint32_t free;

	if (IS_ERR_OR_NULL(using_buf))
		return 0;

	if (using_buf->next_idx >= using_buf->first_idx)
		free = max(LOGGER_BUF_LEN - using_buf->next_idx, using_buf->first_idx);
	else
		free = using_buf->first_idx - using_buf->next_idx;

	/*
	 * We need space also for an empty header that signalizes wrapping
	 * of the buffer.
	 */
	return free >= msg_size + sizeof(struct log_node);
}

/* drop old messages until we have enough contiguous space */
static void logger_make_free_space(struct mdfx_log_buf_t *log_buf, uint32_t msg_size)
{
	if (IS_ERR_OR_NULL(log_buf))
		return;

	while (!logger_buf_has_space(log_buf, msg_size))
		log_buf->first_idx = msg_next(log_buf->log_buf, log_buf->first_idx);
}

static uint32_t logger_get_aligned_size(uint32_t text_len)
{
	return LOGGER_ALIGN_UP(LOGGER_LOG_NODE_LEN + text_len);
}

static struct file* mdfx_logger_open_file(bool need_new_file,
	const char* event_name, uint32_t name_max, int *out_file_size, int64_t id)
{
	static char filename[FILENAME_LEN] = {0};
	char event_filename[FILENAME_LEN] = {0};
	char logger_dir[FILENAME_LEN / 2] = {0};
	char *filename_ptr = NULL;
	char *include_str = NULL;
	char file_category[MAX_FILE_NAME_LEN / 2] = {0};
	int file_size;
	int file_count;
	int ret;
	int event_name_len;
	bool need_create_new_file = need_new_file;

	if (IS_ERR_OR_NULL(event_name) || IS_ERR_OR_NULL(out_file_size))
		return NULL;

	event_name_len = strlen(event_name);
	if (event_name_len <= 0 || event_name_len >= name_max) //lint !e574
		filename_ptr = filename;
	else
		filename_ptr = event_filename;

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return NULL;

	if (!need_create_new_file) {
		file_size = mdfx_file_get_size(filename_ptr);
		if (file_size < 0 || file_size >= g_mdfx_data->file_spec.file_max_size) //lint !e574
			need_create_new_file = true;

		*out_file_size = file_size;
	}

	/*
	 * if file size is -1, it means there is no such file, just new file;
	 * if the size of current file is great than file_max_size,
	 * new file should be create
	 */
	if (need_create_new_file) {
		if (event_name[0] != '\0')
			ret = snprintf(file_category, MAX_FILE_NAME_LEN / 2 - 1, "event[%s]_id[%lld]", event_name, id);
		else
			ret = snprintf(file_category, MAX_FILE_NAME_LEN / 2 - 1, "%s_id[%lld]", MDFX_NORMAL_NAME_PREFIX, id);
		if (ret < 0) {
			mdfx_pr_err("write to array file_category error");
			return NULL;
		}
		mdfx_file_get_name(filename_ptr, MODULE_NAME_LOGGER, file_category, FILENAME_LEN);
		ret = mdfx_create_dir(filename_ptr);
		if (ret != 0) {
			mdfx_pr_err("create dir failed name=%s\n", filename_ptr);
			return NULL;
		}

		mdfx_file_get_dir_name(logger_dir, MODULE_NAME_LOGGER, FILENAME_LEN / 2);

		include_str = (event_name_len > 0) ? MDFX_EVENT_NAME_PREFIX : MDFX_NORMAL_NAME_PREFIX;
		file_count = (event_name_len > 0) ? g_mdfx_data->var_event_log_file_count : g_mdfx_data->var_log_file_count;
		file_count = mdfx_remove_redundant_files(logger_dir, include_str, file_count);

		mdfx_pr_info("file_count=%d, max_count=%u, filename=%s\n",
					file_count, g_mdfx_data->var_log_file_count, filename_ptr);

		*out_file_size = 0;
	}

	return mdfx_file_open(filename_ptr);
}

static void mdfx_logger_gen_file(void *data)
{
	struct mdfx_log_buf_t *log_buf = NULL;
	struct log_node *msg = NULL;
	struct file* saver_file = NULL;
	int32_t file_size;
	mm_segment_t old_fs;
	uint32_t current_index;
	uint32_t end_index;
	int ret;

	if (IS_ERR_OR_NULL(g_mdfx_data)) {
		mdfx_pr_err("g_mdfx_data is null");
		return;
	}
	if (IS_ERR_OR_NULL(data)) {
		mdfx_pr_err("data is null");
		return;
	}
	log_buf = (struct mdfx_log_buf_t *)data;

	raw_spin_lock(&log_buf->buf_lock);
	current_index = log_buf->first_idx;
	end_index = log_buf->next_idx;
	raw_spin_unlock(&log_buf->buf_lock);

	saver_file = mdfx_logger_open_file(false, log_buf->event_name, MDFX_EVENT_NAME_MAX, &file_size, log_buf->visitor_id);
	if (IS_ERR_OR_NULL(saver_file)) {
		mdfx_pr_err("saver file is null");
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	while (current_index != end_index) {
		msg = msg_from_idx(log_buf->log_buf, current_index);
		if (IS_ERR_OR_NULL(msg)) {
			mdfx_pr_err("msg is null, log_buf=%pK, current_index=%u", log_buf->log_buf, current_index);

			set_fs(old_fs);
			filp_close(saver_file, NULL);
			return;
		}

		/*
		 * if the size of current file is great than file_max_size,
		 * new file should be create
		 */
		if (msg->text_len + file_size > g_mdfx_data->file_spec.file_max_size) {
			set_fs(old_fs);
			filp_close(saver_file, NULL);

			// create new file
			saver_file = mdfx_logger_open_file(true, log_buf->event_name, MDFX_EVENT_NAME_MAX, &file_size, log_buf->visitor_id);
			if (IS_ERR_OR_NULL(saver_file))
				return;

			old_fs = get_fs();
			set_fs(KERNEL_DS); //lint !e501
		}

		ret = mdfx_file_write(saver_file, msg_text(msg), msg->text_len);
		if (ret < 0)
			mdfx_pr_err("mdfx_file_write error");
		/* write the kmsg per idx, and update current_index, until equal to next idx */
		current_index = msg_next(log_buf->log_buf, current_index);
		file_size += msg->text_len;
	}

	set_fs(old_fs);
	filp_close(saver_file, NULL);
}

static void mdfx_logger_clear_last_node(struct mdfx_log_buf_t *using_log_buf, uint32_t msg_size)
{
	if (IS_ERR_OR_NULL(using_log_buf)) {
		mdfx_pr_err("using_log_buf is null");
		return;
	}
	/*
	 * The kmsg + an additional empty header does not fit
	 * at the end of the buffer. Add an empty header with len == 0
	 * to signify a wrap around.
	 * The kmsg entry cannot be cut, the address must be consecutive.
	 */
	if (using_log_buf->next_idx + msg_size + LOGGER_LOG_NODE_LEN > LOGGER_BUF_LEN) {
		memset(using_log_buf->log_buf + using_log_buf->next_idx, 0, LOGGER_LOG_NODE_LEN);
		using_log_buf->next_idx = 0;

		mdfx_pr_warn("no avaliable space\n");
	}
}

static void mdfx_append_null_node_to_tail(struct mdfx_log_buf_t *using_log_buf, uint32_t msg_size)
{
	if (IS_ERR_OR_NULL(using_log_buf)) {
		mdfx_pr_err("using_log_buf is null");
		return;
	}
	/*
	 * if buffer have no space, we need append a null node to assign that a wrapper
	 */
	if (using_log_buf->next_idx + msg_size + LOGGER_LOG_NODE_LEN >= LOGGER_BUF_LEN) {
		if (using_log_buf->next_idx + LOGGER_LOG_NODE_LEN <= LOGGER_BUF_LEN) {
			memset(using_log_buf->log_buf + using_log_buf->next_idx, 0, LOGGER_LOG_NODE_LEN);
			return;
		}

		mdfx_pr_warn("append null node fail, next_idx=%u\n", using_log_buf->next_idx);
	}
}

/*
 * if in user version, it should be a cycle buffer.
 * when buffer is full, should drop old idx.
 * otherwise, when buffer is full, should save it to file.
 */
static struct mdfx_log_buf_t* mdfx_logger_get_available_buf(struct mdfx_logger_t *logger,
		uint32_t msg_size, bool be_cycle)
{
	struct mdfx_log_buf_t *using_log_buf = NULL;
	struct mdfx_log_buf_t *saving_log_buf = NULL;

	if (IS_ERR_OR_NULL(logger)) {
		mdfx_pr_err("logger is null");
		return NULL;
	}

	using_log_buf = mdfx_get_using_log_buf(logger);

	if (be_cycle) {
		logger_make_free_space(using_log_buf, msg_size);
		mdfx_logger_clear_last_node(using_log_buf, msg_size);
		return using_log_buf;
	}

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return NULL;

	/* current log buf is full, save to file, switch to another buf */
	if (!logger_buf_has_space(using_log_buf, msg_size)) {
		mdfx_append_null_node_to_tail(using_log_buf, msg_size);
		logger->saving_index = logger->using_index;
		saving_log_buf = mdfx_get_log_buf(logger, logger->saving_index);
		mdfx_logger_clear_event_name(saving_log_buf);

		/* start the saver thread to save log to file */
		mdfx_saver_triggle_thread(&g_mdfx_data->log_saving_thread, saving_log_buf, mdfx_logger_gen_file);

		mdfx_pr_info("current log buf is full, triggle log_saving_thread");
		/* get a new log buff, start from 0 */
		logger->using_index = LOGGER_GET_BUF_INDEX(logger->using_index + 1);
		using_log_buf = mdfx_get_using_log_buf(logger);
		using_log_buf->next_idx = 0;
		using_log_buf->first_idx = 0;
	}

	return using_log_buf;
}

static void mdfx_logger_fill_text(struct mdfx_log_buf_t *log_buf, uint32_t size,
	const char *text, uint32_t text_len, const char *header_buf, uint32_t header_len)
{
	struct log_node *msg = NULL;

	if (IS_ERR_OR_NULL(log_buf) || IS_ERR_OR_NULL(text) || IS_ERR_OR_NULL(header_buf))
		return;

	raw_spin_lock(&log_buf->buf_lock);

	/* fill message */
	msg = (struct log_node *)(log_buf->log_buf + log_buf->next_idx);

	if ((msg_text(msg) + header_len) >= (log_buf->log_buf + LOGGER_BUF_LEN)) {
		mdfx_pr_info("header_len=%u", header_len);
		raw_spin_unlock(&log_buf->buf_lock);
		return;
	}

	if ((msg_text(msg) + header_len + text_len) >= (log_buf->log_buf + LOGGER_BUF_LEN)) {
		mdfx_pr_info("header_len=%u, text_len=%u", header_len, text_len);
		raw_spin_unlock(&log_buf->buf_lock);
		return;
	}

	memcpy(msg_text(msg), header_buf, header_len);
	memcpy(msg_text(msg) + header_len, text, text_len);

	msg->text_len = text_len + header_len;
	*(msg_text(msg) + msg->text_len) = '\0';
	msg->len = size;

	log_buf->next_idx += msg->len;

	raw_spin_unlock(&log_buf->buf_lock);
}

/* insert record into the buffer, discard old ones, update heads
 * ________________________________________________________________
 * |          |            |      |        |                      |
 * | node_len | header_msg | text | padding| .....next log ...    |
 * |__________|____________|______|________|______________________|
 * |<-----  aligned_up(struct log_node) -->|
 */
static int mdfx_logger_store_log(struct mdfx_logger_t *logger, const char *text, uint32_t text_len, bool cycle)
{
	struct mdfx_log_buf_t *log_buf = NULL;
	char header_buf[LOGGER_HEADER_MAX_LEN] = {'\0'};
	uint16_t header_len;
	uint32_t size;

	if (IS_ERR_OR_NULL(logger) || IS_ERR_OR_NULL(text) || (text_len == 0))
		return -1;

	header_len = mdfx_get_msg_header(header_buf, sizeof(header_buf) - 1);
	size = text_len + header_len;

	/* number of '\0' padding bytes to next message */
	size = logger_get_aligned_size(size);
	raw_spin_lock(&logger->logger_lock);
	log_buf = mdfx_logger_get_available_buf(logger, size, cycle);
	mdfx_logger_fill_text(log_buf, size, text, text_len, header_buf, header_len);
	raw_spin_unlock(&logger->logger_lock);

	return 0;
}

int mdfx_logger_emit(int64_t visitor_id, const char *fmt, ...)
{
	char textbuf[LOGGER_EACH_TEXT_MAX_LEN] = {0};
	size_t text_len;
	struct mdfx_logger_t *logger = NULL;
	va_list args;
	int ret;

	if (IS_ERR_OR_NULL(fmt))
		return -1;

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return -1;

	if (!MDFX_HAS_CAPABILITY(g_mdfx_data->mdfx_caps, MDFX_CAP_LOGGER))
		return 0;

	va_start(args, fmt);

	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(textbuf, sizeof(textbuf) - 1, fmt, args);
	if (text_len <= 0) {
		va_end(args);
		return 0;
	}

	textbuf[(--text_len) % LOGGER_EACH_TEXT_MAX_LEN] = '\n';
	textbuf[(++text_len) % LOGGER_EACH_TEXT_MAX_LEN] = '\0';

	logger = mfx_get_visitor_logger(visitor_id);
	if (IS_ERR_OR_NULL(logger) || logger->visitor_id == 0) {
		mdfx_pr_err("logger is null or logger not inited");
		va_end(args);
		return -1;
	}

	ret = mdfx_logger_store_log(logger, textbuf, text_len, mdfx_user_mode());
	va_end(args);

	return ret;
}

void mdfx_logger_init(struct mdfx_logger_t *logger, int64_t id)
{
	struct mdfx_log_buf_t *log_buf = NULL;
	uint32_t i;

	if (IS_ERR_OR_NULL(logger))
		return;

	for (i = 0; i < LOGGER_BUF_MAX_COUNT; i++) {
		log_buf = &logger->log_bufs[i];
		log_buf->first_idx = 0;
		log_buf->next_idx = 0;
		log_buf->visitor_id = id;
		raw_spin_lock_init(&log_buf->buf_lock);
		log_buf->log_buf = mdfx_logger_alloc_log_buf();
	}

	logger->using_index = 0;
	logger->saving_index = 0;
	logger->visitor_id = id;
	logger->using_log_buf = mdfx_get_using_log_buf(logger);

	raw_spin_lock_init(&logger->logger_lock);
}

void mdfx_logger_deinit(struct mdfx_logger_t *logger)
{
	struct mdfx_log_buf_t *log_buf = NULL;
	uint32_t i;

	if (IS_ERR_OR_NULL(logger))
		return;

	for (i = 0; i < LOGGER_BUF_MAX_COUNT; i++) {
		log_buf = &logger->log_bufs[i];
		log_buf->first_idx = 0;
		log_buf->next_idx = 0;
		log_buf->visitor_id = INVALID_VISITOR_ID;

		if (log_buf->log_buf) {
			kfree(log_buf->log_buf);
			log_buf->log_buf = NULL;
		}
	}

	logger->using_index = 0;
	logger->saving_index = 0;
	logger->visitor_id = INVALID_VISITOR_ID;
	logger->using_log_buf = NULL;
}

static int mdfx_logger_act(struct mdfx_pri_data *mdfx_data, struct mdfx_event_desc *desc)
{
	struct mdfx_logger_t *logger = NULL;
	uint64_t detail;
	uint64_t visitor_types;
	uint32_t type_bit;
	int64_t visitor_id;
	struct mdfx_log_buf_t *log_buf = NULL;

	mdfx_pr_info("enter mdfx_logger_act");

	if (IS_ERR_OR_NULL(mdfx_data) || IS_ERR_OR_NULL(desc))
		return -1;

	if (mdfx_event_check_parameter(desc))
		return -1;

	if (!MDFX_HAS_CAPABILITY(mdfx_data->mdfx_caps, MDFX_CAP_LOGGER))
		return 0;

	detail = mdfx_event_get_action_detail(desc, ACTOR_LOGGER);
	if (!ENABLE_BITS_FIELD(detail, LOGGER_PRINT_DRIVER_LOG))
		return 0;

	visitor_types = desc->relevance_visitor_types;
	for (type_bit = 0;  visitor_types != 0; type_bit++) {
		if (!ENABLE_BIT64(visitor_types, type_bit))
			continue;

		mdfx_pr_info("call mdfx_get_visitor_id, type:%llu", BIT64(type_bit));
		// triggle all visitor logger to save to file
		visitor_id = mdfx_get_visitor_id(&mdfx_data->visitors, BIT64(type_bit));
		if (visitor_id != INVALID_VISITOR_ID) {
			logger = mfx_get_visitor_logger(visitor_id);
			if (logger && logger->visitor_id > 0) {
				raw_spin_lock(&logger->logger_lock);

				log_buf = mdfx_get_log_buf(logger, logger->using_index);
				mdfx_logger_build_event_name(log_buf, desc->event_name);

				mdfx_saver_triggle_thread(&mdfx_data->log_saving_thread, log_buf,
						mdfx_logger_gen_file);

				mdfx_pr_info("triggle log_saving_thread");
				raw_spin_unlock(&logger->logger_lock);
			}
		}

		DISABLE_BIT64(visitor_types, type_bit);
	}

	return 0;
}

struct mdfx_actor_ops_t mdfx_logger_ops = {
	.act = mdfx_logger_act,
	.do_ioctl = NULL,
};

void mdfx_logger_init_actor(struct mdfx_actor_t *actor)
{
	if (IS_ERR_OR_NULL(actor))
		return;

	actor->actor_type = ACTOR_LOGGER;
	actor->ops = &mdfx_logger_ops;
}


