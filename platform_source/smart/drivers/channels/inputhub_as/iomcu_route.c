/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description: route channel interface implement.
 * Create: 2019/11/05
 */
#include "iomcu_route.h"

#include <securec.h>

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/suspend.h>
#include <linux/fb.h>
#include <linux/rtc.h>
#include <linux/pm_wakeup.h>
#include <linux/time64.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include "iomcu_log.h"
#include <platform_include/smart/linux/base/ap/protocol.h>

#define ROUTE_BUFFER_MAX_SIZE (1024 * 128)
#ifdef TIMESTAMP_SIZE
#undef TIMESTAMP_SIZE
#define TIMESTAMP_SIZE 8
#endif

static int check_param(const struct inputhub_route_table *route_item)
{
	if (!route_item) {
		ctxhub_err("%s, input pointer is null\n", __func__);
		return RET_FAIL;
	}

	if (!route_item->channel_name) {
		ctxhub_err("%s, channel name is null\n", __func__);
		return RET_FAIL;
	}

	if (!route_item->p_head.pos) {
		ctxhub_err("%s, head is null\n", __func__);
		return RET_FAIL;
	}

	if (!route_item->p_read.pos) {
		ctxhub_err("%s, read is null\n", __func__);
		return RET_FAIL;
	}

	if (!route_item->p_write.pos) {
		ctxhub_err("%s, write is null\n", __func__);
		return RET_FAIL;
	}

	return RET_SUCC;
}

/*
 * Function    : inputhub_route_open
 * Description : alloc route item kernel space
 * Input       : [route_item] manager for kernel buf
 * Output      : none
 * Return      : 0 OK, other error
 */
int inputhub_route_open(struct inputhub_route_table *route_item)
{
	char *pos = NULL;

	if (!route_item || !route_item->channel_name) {
		ctxhub_err("[%s], route item or channel name is NULL!\n",
			__func__);
		return -EINVAL;
	}

	pos = vzalloc(ROUTE_BUFFER_MAX_SIZE);
	if (!pos)
		return -ENOMEM;

	spin_lock_init(&route_item->buffer_spin_lock);
	route_item->is_reading = false;
	route_item->p_head.pos = pos;
	route_item->p_write.pos = pos;
	route_item->p_read.pos = pos;
	route_item->p_head.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->p_write.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->p_read.buffer_size = 0;
	return 0;
}

/*
 * Function    : inputhub_route_close
 * Description : free route item kernel space
 * Input       : [route_item] manager for kernel buf
 * Output      : none
 * Return      : 0 OK, other error
 */
void inputhub_route_close(struct inputhub_route_table *route_item)
{
	if (!route_item) {
		ctxhub_err("[%s], route item is NULL!\n", __func__);
		return;
	}
	if (route_item->p_head.pos)
		vfree(route_item->p_head.pos);

	route_item->p_head.pos = NULL;
	route_item->p_write.pos = NULL;
	route_item->p_read.pos = NULL;
}

void inputhub_route_clean_buffer(struct inputhub_route_table *route_item)
{
	unsigned long flags;

	if (!route_item) {
		ctxhub_err("[%s], route item is NULL!\n", __func__);
		return;
	}

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->p_read.pos = route_item->p_write.pos;
	route_item->p_write.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->p_read.buffer_size = 0;
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
}

static bool data_ready(struct inputhub_route_table *route_item,
		       struct inputhub_buffer_pos *reader)
{
	unsigned long flags;

	if (!route_item || !reader) {
		ctxhub_err("[%s], input is NULL!\n", __func__);
		return false;
	}

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	*reader = route_item->p_read;
	if (reader->buffer_size > 0) {
		if (route_item->is_reading) {
			spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
			return false;
		}
		route_item->is_reading = true;
	}
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return reader->buffer_size > 0;
}

/*lint -e530*/
static int get_pkt_len(const struct inputhub_route_table *route_item,
		       struct inputhub_buffer_pos *reader, unsigned int *full_pkg_length,
		       size_t count)
{
	char *buffer_end = NULL;
	unsigned int tail_half_len;

	if (!route_item || !reader || !full_pkg_length)
		return RET_FAIL;

	if (reader->buffer_size > ROUTE_BUFFER_MAX_SIZE) {
		if (route_item->channel_name)
			ctxhub_err("error reader->buffer_size = %d in channel %s!\n",
				(int)reader->buffer_size,
				route_item->channel_name);
		return RET_FAIL;
	}

	buffer_end = route_item->p_head.pos + route_item->p_head.buffer_size;

	if (buffer_end - reader->pos >= LENGTH_SIZE) { //lint !e574
		// copy 4 byte from buf end directly
		*full_pkg_length = *((unsigned int *)reader->pos);
		reader->pos += LENGTH_SIZE;
		if (reader->pos == buffer_end)
			reader->pos = route_item->p_head.pos;
	} else {
		tail_half_len = buffer_end - reader->pos;
		// copy 4 byte from buf end and buf start
		if (memcpy_s(full_pkg_length, LENGTH_SIZE, reader->pos,
			     tail_half_len) != EOK) {
			ctxhub_err("[%s], memcpy failed!\n", __func__);
			return RET_FAIL;
		}
		if (memcpy_s((char *)full_pkg_length + tail_half_len,
				LENGTH_SIZE - tail_half_len,
				route_item->p_head.pos,
				LENGTH_SIZE - tail_half_len) != EOK) {
			ctxhub_err("[%s], 2:memcpy failed!\n", __func__);
			return RET_FAIL;
		}

		reader->pos = route_item->p_head.pos +
			(LENGTH_SIZE - tail_half_len);
	}

	if (*full_pkg_length + LENGTH_SIZE > reader->buffer_size ||
	    *full_pkg_length > count) {
		if (route_item->channel_name)
			ctxhub_err("full_pkg_length = %u is too large in channel %s!\nreader buffer size = %u input buffer size is %lu\n",
				*full_pkg_length,
				route_item->channel_name,
				reader->buffer_size,
				count);
		return RET_FAIL;
	}
	return RET_SUCC;
}
/*lint +e530*/

static int update_buffer_info(struct inputhub_route_table *route_item,
			      const struct inputhub_buffer_pos *reader, unsigned int full_pkg_length)
{
	unsigned long flags;
	int ret = RET_SUCC;

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->p_read.pos = reader->pos; //lint !e644
	route_item->p_read.buffer_size -= (full_pkg_length + LENGTH_SIZE);
	if ((route_item->p_write.buffer_size > ROUTE_BUFFER_MAX_SIZE) ||
	    (route_item->p_write.buffer_size +
		(full_pkg_length + LENGTH_SIZE) > ROUTE_BUFFER_MAX_SIZE))
		ret = RET_FAIL;
	else
		route_item->p_write.buffer_size +=
			(full_pkg_length + LENGTH_SIZE);

	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);

	if (ret == RET_FAIL)
		ctxhub_err("%s:%d write buffer error buffer_size=%u pkg_len=%u\n",
			__func__, __LINE__, route_item->p_write.buffer_size,
			full_pkg_length);

	return ret;
}

static void clean_buffer(struct inputhub_route_table *route_item)
{
	unsigned long flags;

	if (route_item->channel_name != NULL)
		ctxhub_err("now we will clear the receive buffer in channel %s!\n",
			route_item->channel_name);

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->p_read.pos = route_item->p_write.pos;
	route_item->p_write.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->p_read.buffer_size = 0;
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
}

/*lint -e644 */
static int copy_buffer_to_user(const struct inputhub_route_table *route_item,
			       struct inputhub_buffer_pos *reader, char __user *buf,
			unsigned int full_pkg_length)
{
	unsigned int tail_half_len;
	char *buffer_end = route_item->p_head.pos + route_item->p_head.buffer_size;
	/* buffer end big enough, copy once, otherwise twice */
	if (buffer_end - reader->pos >= full_pkg_length) {
		if (copy_to_user(buf, reader->pos, full_pkg_length) == 0) {
			reader->pos += full_pkg_length;
			if (reader->pos == buffer_end)
				reader->pos = route_item->p_head.pos;
		} else {
			ctxhub_err("copy to user failed\n");
			return RET_FAIL;
		}
	} else {
		tail_half_len = buffer_end - reader->pos;

		if ((copy_to_user(buf, reader->pos, tail_half_len) == 0) &&
		    (copy_to_user(buf + tail_half_len,
			route_item->p_head.pos,
			(full_pkg_length - tail_half_len)) == 0)) {
			reader->pos = route_item->p_head.pos +
				(full_pkg_length - tail_half_len);
		} else {
			ctxhub_err("copy to user failed\n");
			return RET_FAIL;
		}
	}
	return RET_SUCC;
}
/*lint +e644 */

/*
 * Function    : inputhub_route_read
 * Description : read report data from kernel buffer, copy to user buffer
 * Input       : [route_item] manager for kernel buf
 *             : [buf] user buffer
 *             : [count] user buffer size
 * Output      : none
 * Return      : read buffer bytes actually
 */
ssize_t inputhub_route_read(struct inputhub_route_table *route_item,
			    char __user *buf, size_t count)
{
	struct inputhub_buffer_pos reader;
	unsigned int full_pkg_length;
	int ret;
	unsigned long flags;

	if (!buf) {
		ctxhub_err("[%s], input is NULL!\n", __func__);
		return -EINVAL;
	}

	ret = check_param(route_item);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], check param failed!\n", __func__);
		return -EINVAL;
	}

	/* woke up by signal */
	if (wait_event_interruptible(route_item->read_wait, data_ready(route_item, &reader)) != 0) //lint !e666 !e578
		return 0;

	ret = get_pkt_len(route_item, &reader, &full_pkg_length, count);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], get pkg length failed!\n", __func__);
		goto clean_buffer;
	}

	ret = copy_buffer_to_user(route_item, &reader, buf, full_pkg_length);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], copy to user failed!\n", __func__);
		goto clean_buffer;
	}

	ret = update_buffer_info(route_item, &reader, full_pkg_length);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], update buffer info failed!\n", __func__);
		goto clean_buffer;
	}

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->is_reading = false;
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return full_pkg_length;

clean_buffer:
	clean_buffer(route_item);
	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->is_reading = false;
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return 0;
}
EXPORT_SYMBOL_GPL(inputhub_route_read); //lint !e546 !e580

static int64_t get_timestamps(void)
{
	struct timespec ts;

	get_monotonic_boottime(&ts);
	/* timevalToNano */
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static uint32_t write_to_fifo(struct inputhub_buffer_pos *pwriter,
			char * const buffer_begin, const char * const buffer_end,
			const char * const buf, uint32_t count)
{
	uint32_t cur_to_tail_len = buffer_end - pwriter->pos;
	uint32_t buffer_size = buffer_end - buffer_begin;
	int ret;

	if (cur_to_tail_len >= count) {
		/* now write buffer end is enough copy directly */
		ret = memcpy_s(pwriter->pos, cur_to_tail_len, buf, count);
		if (ret != EOK)
			return 0;

		pwriter->pos += count;
		if (buffer_end == pwriter->pos)
			pwriter->pos = buffer_begin;
	} else {
		/*lint -e662 */
		/* now write buffer end is not enough, so copy twice */
		ret = memcpy_s(pwriter->pos, cur_to_tail_len, buf, cur_to_tail_len);
		if (ret != EOK)
			return 0;

		ret = memcpy_s(buffer_begin, buffer_size,
			       buf + cur_to_tail_len,
			count - cur_to_tail_len);
		if (ret != EOK)
			return 0;
		/*lint +e662 */

		pwriter->pos = buffer_begin + (count - cur_to_tail_len);
	}
	return count;
}

/*
 * Function    : inputhub_route_write_batch
 * Description : write report data to kernel buffer, use external timestamp
 * Input       : [route_item] manager for kernel buf
 *             : [buf] data buffer
 *             : [count] data buffer size
 *             : [timestamp] external timestamp
 * Output      : none
 * Return      : write buffer bytes actually
 */
ssize_t inputhub_route_write_batch(struct inputhub_route_table *route_item,
				   const char *buf, size_t count, int64_t timestamp)
{
	struct inputhub_buffer_pos writer;
	char *buffer_begin = NULL;
	char *buffer_end = NULL;
	struct t_head header;
	unsigned long flags;
	uint32_t write_count;

	if (!buf || check_param(route_item) != RET_SUCC)
		return -EINVAL;

	header.timestamp = timestamp;

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	writer = route_item->p_write;
	if (writer.buffer_size < count + HEAD_SIZE)
		goto out;

	buffer_begin = route_item->p_head.pos;
	buffer_end = route_item->p_head.pos + route_item->p_head.buffer_size;
	if (UINT_MAX - count < sizeof(int64_t)) {
		ctxhub_err("[%s] %s count is too large :%lu\n", __func__, route_item->channel_name, count);
		goto out;
	}

	header.pkg_length = count + sizeof(int64_t);
	write_count = write_to_fifo(&writer, buffer_begin, buffer_end, header.effect_addr, HEAD_SIZE);
	if (write_count != HEAD_SIZE) {
		ctxhub_err("[%s] %s write head failed!n", __func__, route_item->channel_name);
		goto out;
	}

	if (count != 0) {
		write_count = write_to_fifo(&writer, buffer_begin, buffer_end, buf, count);
		if (write_count != count) {
			ctxhub_err("[%s] %s write data failed!n", __func__, route_item->channel_name);
			goto out;
		}
	}

	route_item->p_write.pos = writer.pos;
	route_item->p_write.buffer_size -= (count + HEAD_SIZE);
	if ((UINT_MAX - route_item->p_read.buffer_size) < (count + HEAD_SIZE)) {
		ctxhub_err("%s:%s p_read :count is too large :%lu!\n", __func__, route_item->channel_name, count);
		goto out;
	}
	route_item->p_read.buffer_size += (count + HEAD_SIZE);
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	wake_up_interruptible(&route_item->read_wait);

	return (count + HEAD_SIZE);

out:
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return 0;
}

/*
 * Function    : inputhub_route_write
 * Description : write report data to kernel buffer, use internal timestamp
 * Input       : [route_item] manager for kernel buf
 *             : [buf] data buffer
 *             : [count] data buffer size
 * Output      : none
 * Return      : write buffer bytes actually
 */
ssize_t inputhub_route_write(struct inputhub_route_table *route_item,
			     const char *buf, size_t count)
{
	struct inputhub_buffer_pos writer;
	char *buffer_begin = NULL;
	char *buffer_end = NULL;
	struct t_head header;
	unsigned long flags;
	int ret;
	uint32_t write_count;

	if (!buf) {
		ctxhub_err("[%s], input is NULL!\n", __func__);
		return -EINVAL;
	}

	ret = check_param(route_item);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], check param failed!\n", __func__);
		return -EINVAL;
	}

	header.timestamp = get_timestamps();

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	writer = route_item->p_write;

	if (writer.buffer_size < count + HEAD_SIZE) {
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		return 0;
	}

	buffer_begin = route_item->p_head.pos;
	buffer_end = route_item->p_head.pos + route_item->p_head.buffer_size;
	header.pkg_length = count + sizeof(int64_t);

	write_count = write_to_fifo(&writer, buffer_begin, buffer_end,
		header.effect_addr, HEAD_SIZE);
	if (write_count != HEAD_SIZE) {
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		ctxhub_err("%s, write head failed!n", __func__);
		return 0;
	}

	if (count != 0) {
		write_count = write_to_fifo(&writer, buffer_begin, buffer_end,
			buf, count);
		if (write_count != count) {
			spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
			ctxhub_err("%s, write package failed!n", __func__);
			return 0;
		}
	}

	route_item->p_write.pos = writer.pos;
	route_item->p_write.buffer_size -= (count + HEAD_SIZE);
	route_item->p_read.buffer_size += (count + HEAD_SIZE);
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	wake_up_interruptible(&route_item->read_wait);
	return (count + HEAD_SIZE);
}
EXPORT_SYMBOL_GPL(inputhub_route_write); //lint !e546 !e580
