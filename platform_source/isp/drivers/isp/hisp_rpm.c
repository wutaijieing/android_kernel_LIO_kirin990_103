/*
 * isp rpmsg client driver
 *
 * Copyright (c) 2013- ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/rpmsg.h>
#include <linux/completion.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/mm_iommu.h>
#include <linux/iommu.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <uapi/linux/isp.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <platform_include/isp/linux/hisp_mempool.h>
#include <securec.h>
#include <hisp_internel.h>

#define DBG_BIT                  (1 << 2)
#define INF_BIT                  (1 << 1)
#define ERR_BIT                  (1 << 0)
#define HISP_NAME_LENGTH		 (64)
#define HISP_LOG_SIZE   		 128

#define CORE_RPMSG_DEBUG_NAME   (1 << 31)

static unsigned int debug_mask = (INF_BIT | ERR_BIT);

#ifdef DEBUG_HISP
module_param_named(debug_mask, debug_mask, int, 0644);
#endif

#ifdef DEBUG_HISP
#define RPMSG_MSG_SIZE          (496)

enum hisp_rpmsg_state {
	RPMSG_UNCONNECTED,
	RPMSG_CONNECTED,
};
#endif

enum hisp_msg_rdr_e {
	RPMSG_RDR_LOW                   = 0,
	/* RECV RPMSG INFO */
	RPMSG_CAMERA_SEND_MSG           = 1,
	RPMSG_SEND_MSG_TO_MAILBOX       = 2,
	RPMSG_RECV_MAILBOX_FROM_ISPCPU  = 3,
	RPMSG_ISP_THREAD_RECVED         = 4,
	RPMSG_RECV_SINGLE_MSG           = 5,
	RPMSG_SINGLE_MSG_TO_CAMERA      = 6,
	RPMSG_CAMERA_MSG_RECVED         = 7,
	RPMSG_RDR_MAX,
};
enum hisp_msg_err_e {
	RPMSG_SEND_ERR                  = 0,
	RPMSG_SEND_MAILBOX_LOST,
	RPMSG_RECV_MAILBOX_LOST,
	RPMSG_THREAD_RECV_LOST,
	RPMSG_RECV_SINGLE_LOST,
	RPMSG_SINGLE_MSG_TO_CAMERA_LOST,
	RPMSG_CAMERA_MSG_RECVED_LOST,
	RPMSG_RECV_THRED_TIMEOUT,
	RPMSG_CAMERA_GET_TIMEOUT,
	RPMSG_ERR_MAX,
};

/**
 * struct rpmsg_hdr - common header for all rpmsg messages
 * @src: source address
 * @dst: destination address
 * @reserved: reserved for future use
 * @len: length of payload (in bytes)
 * @flags: message flags
 * @data: @len bytes of message payload data
 *
 * Every message sent(/received) on the rpmsg bus begins with this header.
 */
struct rpmsg_hdr {
	u32 src;
	u32 dst;
	u32 reserved;
	u16 len;
	u16 flags;
	u8 data[0];
} __packed;

struct hisp_rpmsg_service {
	struct rpmsg_device *rpdev;
#ifdef DEBUG_HISP
	struct miscdevice mdev;
	int enable_rpmsg;
	int bypass_powerdn;
	struct rpmsg_endpoint *ept;
	struct sk_buff_head queue;
	struct mutex lock;
	wait_queue_head_t readq;
	struct list_head buffer_nodes;
	struct mutex buffer_lock;
	struct mutex ctrl_lock;
	u32 dst;
	int state;
#endif
};

#ifdef DEBUG_HISP
#define RPMSG_RDR_INFO_MAX_NUM  242     /* total : 250*16+0x20 = 4k bytes */
#define RPMSG_RDR_1_MS          1000000 /* 1ms = 1000000 */
#define RPMSG_RDR_MSG_INVALID   0xFF00FF00
#define MEM_ALIGN_SIZE          0x10000

struct hisp_msg_head_t {
	unsigned int message_size;
	unsigned int api_name;
	unsigned int message_id;
};

struct hisp_msg_rdr_s {
	unsigned int message_id;
	unsigned int type;
	unsigned long long time;
};

struct hisp_buffer_node {
	int fd;
	struct sg_table *sgt;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct list_head list;
};

#endif

struct hisp_rpmsgrefs_s {
	atomic_t sendin_refs;
	atomic_t sendx_refs;
	atomic_t recvtask_refs;
	atomic_t recvthread_refs;
	atomic_t recvin_refs;
	atomic_t recvcb_refs;
	atomic_t recvdone_refs;
	atomic_t rdr_refs;
#ifdef DEBUG_HISP
	atomic_t start_refs;
	struct mutex isp_msg_mutex;
	struct hisp_msg_rdr_s isp_msg_rdr[RPMSG_RDR_INFO_MAX_NUM];
	unsigned int debug_message_id;
#endif
};

struct hisp_rpmsgrefs_s *hisp_rpmsgrefs;

#ifdef DEBUG_HISP
static atomic_t instances;
static DEFINE_MUTEX(isp_rpmsg_service_mutex);
static DEFINE_MUTEX(isp_rpmsg_send_mutex);
#endif

static struct hisp_rpmsg_service hisp_serv;
struct completion channel_sync;

struct rpmsg_channel_info chinfo = {
	.src = RPMSG_ADDR_ANY,
};

static void hisplog(unsigned int on, const char *tag,
		const char *func, const char *fmt, ...)
{
	va_list args;
	char pbuf[HISP_LOG_SIZE] = {0};
	int size;

	if ((debug_mask & on) == 0 || fmt == NULL)
		return;

	va_start(args, fmt);
	size = vsnprintf_s(pbuf, HISP_LOG_SIZE,
		HISP_LOG_SIZE - 1, fmt, args);/*lint !e592 */
	va_end(args);

	if ((size < 0) || (size > HISP_LOG_SIZE - 1))
		return;

	pbuf[size] = '\0';
	pr_info("Rpmsg ISP [%s][%s] %s", tag, func, pbuf);
}

#define rpmsg_dbg(fmt, ...) hisplog(DBG_BIT, "D", __func__, fmt, ##__VA_ARGS__)
#define rpmsg_info(fmt, ...) hisplog(INF_BIT, "I", __func__, fmt, ##__VA_ARGS__)
#define rpmsg_err(fmt, ...) hisplog(ERR_BIT, "E", __func__, fmt, ##__VA_ARGS__)

#ifdef DEBUG_HISP
static char *hisp_msg_rdr_sources[RPMSG_RDR_MAX] = {
	[RPMSG_RECV_MAILBOX_FROM_ISPCPU] = "ispcpu mailbox come",
	[RPMSG_ISP_THREAD_RECVED]        = "call rpmsg to get",
	[RPMSG_RECV_SINGLE_MSG]          = "rpmsg get one",
	[RPMSG_SINGLE_MSG_TO_CAMERA]     = "camera get one",
	[RPMSG_CAMERA_MSG_RECVED]        = "camera recv one",
	[RPMSG_CAMERA_SEND_MSG]          = "camera send rpmsg",
	[RPMSG_SEND_MSG_TO_MAILBOX]      = "mailbox send ispcpu",
};

static char *hisp_msg_err_sources[RPMSG_ERR_MAX] = {
	[RPMSG_SEND_ERR]                    = "camera send rpmsg lost",
	/* may delay */
	[RPMSG_SEND_MAILBOX_LOST]           = "rpmsg send mailbox lost",
	/* can get more msgs */
	[RPMSG_RECV_MAILBOX_LOST]           = "recv mailbox lost",
	[RPMSG_THREAD_RECV_LOST]            = "thread recv rpmsg lost",
	/* may delay */
	[RPMSG_RECV_SINGLE_LOST]            = "rpmsg recv single lost",
	[RPMSG_SINGLE_MSG_TO_CAMERA_LOST]   = "camera recv single rpmsg lost",
	[RPMSG_CAMERA_MSG_RECVED_LOST]      = "camera get rpmsg lost",
	[RPMSG_RECV_THRED_TIMEOUT]          = "recv thread timeout, question may be kernel schedul",
	[RPMSG_CAMERA_GET_TIMEOUT]          = "camera get msg tiomout",
};

static void hisp_rpmsg_rdr_save(unsigned int num, unsigned int type, void *data)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
	unsigned int rdr_num = (num % RPMSG_RDR_INFO_MAX_NUM);
	struct hisp_msg_rdr_s *msg_rdr = NULL;
	struct hisp_msg_head_t *msg_info = NULL;

	msg_rdr = &dev->isp_msg_rdr[rdr_num];
	if (msg_rdr == NULL) {
		rpmsg_err("Failed: msg_rdr is null\n");
		return;
	}

	if (data != NULL) {
		msg_info = (struct hisp_msg_head_t *)data;
		msg_rdr->message_id = msg_info->message_id;
	} else {
		msg_rdr->message_id = 0;
	}

	msg_rdr->type       = type;
	msg_rdr->time       = dfx_getcurtime();
	rpmsg_dbg("type.%d -- id.0x%x\n", msg_rdr->type, msg_rdr->message_id);
}
#else
static void hisp_rpmsg_rdr_save(unsigned int num, unsigned int type, void *data)
{
}
#endif

int hisp_rpmsg_rdr_init(void)
{
	struct hisp_rpmsgrefs_s *dev = NULL;

	dev = kzalloc(sizeof(struct hisp_rpmsgrefs_s), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	rpmsg_info("kzalloc size.0x%lx\n", sizeof(struct hisp_rpmsgrefs_s));
#ifdef DEBUG_HISP
	mutex_init(&dev->isp_msg_mutex);
#endif
	hisp_rpmsgrefs = dev;

	return 0;
}

int hisp_rpmsg_rdr_deinit(void)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return -ENOMEM;
	}

#ifdef DEBUG_HISP
	mutex_destroy(&dev->isp_msg_mutex);
#endif
	kfree(dev);
	hisp_rpmsgrefs = NULL;

	return 0;
}

void hisp_sendin(void *data)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->sendin_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_CAMERA_SEND_MSG, data);
}

void hisp_sendx(void)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->sendx_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_SEND_MSG_TO_MAILBOX, NULL);
}

void hisp_recvtask(void)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->recvtask_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_RECV_MAILBOX_FROM_ISPCPU, NULL);
}

void hisp_recvthread(void)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->recvthread_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_ISP_THREAD_RECVED, NULL);
}

void hisp_recvin(void *data)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->recvin_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_RECV_SINGLE_MSG, data);
}

void hisp_recvx(void *data)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->recvcb_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_SINGLE_MSG_TO_CAMERA, data);
}

void hisp_recvdone(void *data)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_inc(&dev->recvdone_refs);
	atomic_inc(&dev->rdr_refs);
	hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),
				RPMSG_CAMERA_MSG_RECVED, data);
}

void hisp_rpmsgrefs_dump(void)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	rpmsg_info("cam send info: sendin_refs.0x%x\n",
			atomic_read(&dev->sendin_refs));
	rpmsg_info("cam send info: sendx_refs.0x%x\n",
			atomic_read(&dev->sendx_refs));
	rpmsg_info("Rpg recv info: recvtask_refs.0x%x\n",
			atomic_read(&dev->recvtask_refs));
	rpmsg_info("Rpg recv info: recvthread_refs.0x%x\n",
			atomic_read(&dev->recvthread_refs));
	rpmsg_info("Rpg recv info: recvin_refs.0x%x\n",
			atomic_read(&dev->recvin_refs));
	rpmsg_info("cam recv info: recvcb_refs.0x%x\n",
			atomic_read(&dev->recvcb_refs));
	rpmsg_info("cam recv info: recvdone_refs.0x%x\n",
			atomic_read(&dev->recvdone_refs));
	rpmsg_info("cam recv info: total: rdr_refs.0x%x\n",
			atomic_read(&dev->rdr_refs));
}

void hisp_rpmsgrefs_reset(void)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	atomic_set(&dev->sendin_refs, 0);
	atomic_set(&dev->sendx_refs, 0);
	atomic_set(&dev->recvtask_refs, 0);
	atomic_set(&dev->recvthread_refs, 0);
	atomic_set(&dev->recvin_refs, 0);
	atomic_set(&dev->recvcb_refs, 0);
	atomic_set(&dev->recvdone_refs, 0);
	atomic_set(&dev->rdr_refs, 0);
}
#ifdef DEBUG_HISP
static void hisp_dump_rpmsg_rdr(const unsigned int index)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
	struct hisp_msg_rdr_s *msg    = NULL;
	char time_log[80]            = "";
	char *ptime_log              = NULL;
	unsigned int num             = 0;
	unsigned int cycles          = 0;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	rpmsg_info("~~~~~~~ START TO DUMP RAW MESSAGE INFO ~~~~~~~~~~~\n");

	if (index >= RPMSG_RDR_INFO_MAX_NUM)
		return;

	num = index;/* from index msg */
	ptime_log = time_log;

	while (cycles < RPMSG_RDR_INFO_MAX_NUM) {
		if (num == RPMSG_RDR_INFO_MAX_NUM)
			num = 0;

		msg = &dev->isp_msg_rdr[num];
		print_time(msg->time, ptime_log);
		rpmsg_info("index.%-3d, message_id.0x%-8x, type.%d, info:%-20s, time:%s\n",
				num, msg->message_id, msg->type,
				hisp_msg_rdr_sources[msg->type], ptime_log);
		num++;
		cycles++;
	}

	rpmsg_info("~~~~~~~~~~~~ RAW MESSAGE DUMP END ~~~~~~~~~~~~~~~~~\n");
}

static enum hisp_msg_rdr_e hisprpm_get_index_by_type(unsigned int id,
				unsigned int message_id, unsigned int type)
{
	unsigned int index = RPMSG_RDR_LOW;

	if ((id == message_id) || (message_id == 0x0)) {
		switch (type) {
		case RPMSG_CAMERA_SEND_MSG:
		case RPMSG_RECV_MAILBOX_FROM_ISPCPU:
		case RPMSG_ISP_THREAD_RECVED:
			index = RPMSG_RDR_LOW;
			break;
		case RPMSG_SEND_MSG_TO_MAILBOX:
		case RPMSG_RECV_SINGLE_MSG:
		case RPMSG_SINGLE_MSG_TO_CAMERA:
		case RPMSG_CAMERA_MSG_RECVED:
			if (message_id != 0)
				index = type;
			break;
		default:
			break;
		}
	}
	return index;/* the sencond time not record */
}

static unsigned int hisp_check_rpmsg_lost(unsigned int message_flag)
{
	if (message_flag == 0)
		return 1;
	else
		return 0;
}

static unsigned int hisp_find_rpmsg_err(unsigned long long message_time[],
			const unsigned int message_flag[], unsigned int type)
{
	unsigned int err = 0;

	switch (type) {
	case RPMSG_SEND_ERR:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_CAMERA_SEND_MSG]);
		break;
	case RPMSG_SEND_MAILBOX_LOST:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_SEND_MSG_TO_MAILBOX]);
		break;
	case RPMSG_RECV_MAILBOX_LOST:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_RECV_MAILBOX_FROM_ISPCPU]);
		break;
	case RPMSG_THREAD_RECV_LOST:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_ISP_THREAD_RECVED]);
		break;
	case RPMSG_RECV_SINGLE_LOST:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_RECV_SINGLE_MSG]);
		break;
	case RPMSG_SINGLE_MSG_TO_CAMERA_LOST:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_SINGLE_MSG_TO_CAMERA]);
		break;
	case RPMSG_CAMERA_MSG_RECVED_LOST:
		err = hisp_check_rpmsg_lost(
			message_flag[RPMSG_CAMERA_MSG_RECVED]);
		break;
	case RPMSG_RECV_THRED_TIMEOUT:
		if ((message_time[RPMSG_ISP_THREAD_RECVED] -
		     message_time[RPMSG_RECV_MAILBOX_FROM_ISPCPU]) >
		     10 * RPMSG_RDR_1_MS)
			err = 1;/* time > 10ms */
		break;
	case RPMSG_CAMERA_GET_TIMEOUT:
		if ((message_time[RPMSG_CAMERA_MSG_RECVED] -
		     message_time[RPMSG_SINGLE_MSG_TO_CAMERA]) >
		     10 * RPMSG_RDR_1_MS)
			err = 1;/* time > 10ms */
		break;
	default:
		break;
	}
	return err;
}

static void hisp_rpmsg_err_check(const unsigned int message_err[],
					const unsigned int len)
{
	unsigned int index = 0;

	if (message_err[RPMSG_SEND_ERR]) {
		rpmsg_err("Wrong : %s\n", hisp_msg_err_sources[RPMSG_SEND_ERR]);
		return;
	}

	if (message_err[RPMSG_RECV_SINGLE_LOST]) {
		rpmsg_err("Wrong : %s\n",
			hisp_msg_err_sources[RPMSG_RECV_SINGLE_LOST]);
		return;
	}

	for (index = RPMSG_SEND_ERR; index < len; index++) {
		if (message_err[index])
			rpmsg_err("Wrong : %s\n", hisp_msg_err_sources[index]);
	}
}

static void hisp_analyse_rpmsg_rdr(const unsigned int message_info[],
					const unsigned int info_len,
					const unsigned int message_flag[],
					const unsigned int flag_len)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
	struct hisp_msg_rdr_s *msg    = NULL;
	unsigned long long message_time[RPMSG_RDR_MAX] = { 0 };
	unsigned int message_err[RPMSG_ERR_MAX]        = { 0 };
	char time_log[80]            = "";
	unsigned int index = 0;
	unsigned int len = info_len < flag_len ? info_len : flag_len;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	/* get all time info */
	for (index = RPMSG_CAMERA_SEND_MSG; index < len; index++) {
		if (message_flag[index]) {
			msg = &dev->isp_msg_rdr[message_info[index]];
			message_time[index] = msg->time;
		}
	}

	/* check */
	for (index = RPMSG_SEND_ERR; index < RPMSG_ERR_MAX; index++) {
		message_err[index] = hisp_find_rpmsg_err(message_time,
						message_flag, index);
		rpmsg_dbg("message_err.%d = %d", index, message_err[index]);
	}

	hisp_rpmsg_err_check(message_err, RPMSG_ERR_MAX);
	msg = &dev->isp_msg_rdr[message_info[RPMSG_CAMERA_SEND_MSG]];
	rpmsg_info("******** START TO DUMP MESSAGE_ID : 0x%x INFO ***********",
		msg->message_id);
	for (index = RPMSG_CAMERA_SEND_MSG; index < len; index++) {
		if (message_flag[index] == 0)
			continue;

		msg = &dev->isp_msg_rdr[message_info[index]];
		print_time(message_time[index], time_log);
		rpmsg_info("index.%-3d, message_id.0x%-8x, type.%d, info:%-20s, time:%s\n",
				message_info[index], msg->message_id, msg->type,
				hisp_msg_rdr_sources[msg->type], time_log);
	}

	rpmsg_info("******************* DUMP END ***************************");
}

static void hisp_find_msg_index(unsigned int message_id,
		unsigned int message_info[], unsigned int message_flag[])
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
	struct hisp_msg_rdr_s *msg    = NULL;
	unsigned int num             = 0;
	unsigned int index_num       = 0;
	unsigned long long old_time  = 0;
	unsigned int last_num        = 0;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	/* find the first send message_id index */
	for (num = 0; num < RPMSG_RDR_INFO_MAX_NUM; num++) {
		msg = &dev->isp_msg_rdr[num];
		/* only test : index should be message_id */
		if ((message_id == msg->message_id) &&
		    (msg->type == RPMSG_CAMERA_SEND_MSG)) {
			message_info[RPMSG_CAMERA_SEND_MSG] = num;
			message_flag[RPMSG_CAMERA_SEND_MSG] = 1;
			break;
		}
	}

	if (message_flag[RPMSG_CAMERA_SEND_MSG] != 1) {
		rpmsg_info("can't find send msg id.0x%x,flga.%d\n",
			message_id, message_flag[RPMSG_CAMERA_SEND_MSG]);
		num = 0;/* may be ispcpu send only */
	}

	/* find all message_id info */
	while (num <= RPMSG_RDR_INFO_MAX_NUM) {
		num = num % RPMSG_RDR_INFO_MAX_NUM;
		last_num = num + RPMSG_RDR_INFO_MAX_NUM - 1;
		last_num %= RPMSG_RDR_INFO_MAX_NUM;
		old_time = dev->isp_msg_rdr[last_num].time;
		msg = &dev->isp_msg_rdr[num];
		if (msg->time < old_time)
			break;/* the latest one */

		if ((hisprpm_get_index_by_type(message_id,
				msg->message_id, msg->type) &&
		    (message_flag[msg->type] != 1) != RPMSG_RDR_LOW)) {
			message_info[msg->type] = num;
			message_flag[msg->type] = 1;
		}

		if ((message_flag[RPMSG_SEND_MSG_TO_MAILBOX] &
		     message_flag[RPMSG_CAMERA_MSG_RECVED]) == 1)
			break;

		num++;
		index_num++;

		if (index_num >= RPMSG_RDR_INFO_MAX_NUM)
			break;
	}
}

void hisp_dump_rpmsg_with_id(const unsigned int message_id)
{
	struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
	struct hisp_msg_rdr_s *msg    = NULL;
	unsigned int message_info[RPMSG_RDR_MAX] = { 0 };
	unsigned int message_flag[RPMSG_RDR_MAX] = { 0 };
	unsigned int num             = 0;
	unsigned int index_num       = 0;
	unsigned long long old_time  = 0;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs_s is null\n");
		return;
	}

	mutex_lock(&dev->isp_msg_mutex);
	rpmsg_info("message_id.0x%x\n", message_id);
	/* find message_id info */
	hisp_find_msg_index(message_id, message_info, message_flag);

	/* check some flag */
	if ((message_flag[RPMSG_RECV_SINGLE_MSG] == 0) ||
	    (message_flag[RPMSG_CAMERA_SEND_MSG] == 0))
		goto analyse;

	/* find recv message_id info */
	num = message_info[RPMSG_RECV_SINGLE_MSG];
	while (1) {/*lint !e568 !e685 */
		msg = &dev->isp_msg_rdr[(num + 1) % RPMSG_RDR_INFO_MAX_NUM];
		old_time = msg->time;
		msg = &dev->isp_msg_rdr[num];
		if (msg->time > old_time)
			break;/* the latest one */

		if ((message_flag[msg->type] != 1) &&
		   ((msg->type == RPMSG_RECV_MAILBOX_FROM_ISPCPU) ||
		    (msg->type == RPMSG_ISP_THREAD_RECVED))) {
			message_info[msg->type] = num;
			message_flag[msg->type] = 1;
		}

		if ((message_flag[RPMSG_RECV_MAILBOX_FROM_ISPCPU] &
		     message_flag[RPMSG_ISP_THREAD_RECVED]) == 1)
			break;

		if (num == 0)
			num = RPMSG_RDR_INFO_MAX_NUM - 1;
		else
			num--;

		index_num++;

		if (index_num >= RPMSG_RDR_INFO_MAX_NUM)
			break;
	}
analyse:
	/* analyse the info */
	hisp_analyse_rpmsg_rdr(message_info, RPMSG_RDR_MAX,
			message_flag, RPMSG_RDR_MAX);

	/* dump the raw info */
	hisp_dump_rpmsg_rdr(message_info[RPMSG_CAMERA_SEND_MSG]);
	mutex_unlock(&dev->isp_msg_mutex);
}
#else
void hisp_dump_rpmsg_with_id(const unsigned int message_id)
{
}
#endif

static int hisp_rpmsg_isp_probe(struct rpmsg_device *rpdev)
{
	struct hisp_rpmsg_service *isp_serv = &hisp_serv;

	isp_serv->rpdev = rpdev;

	rpmsg_info("new HISP connection srv channel: %u -> %u!\n",
			rpdev->src, rpdev->dst);
	complete(&channel_sync);
	rpmsg_dbg("Exit ...\n");
	return 0;
}

static void hisp_rpmsg_isp_remove(struct rpmsg_device *rpdev)
{
	rpmsg_info("Exit ...\n");
}

static int hisp_rpmsg_driver_cb(struct rpmsg_device *rpdev,
				void *data, int len, void *priv, u32 src)
{
	rpmsg_dbg("Enter ...\n");
	dev_warn(&rpdev->dev, "uhm, unexpected message\n");

	print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
			data, len, true);
	rpmsg_dbg("Exit ...\n");
	return 0;
}

static struct rpmsg_device_id rpmsg_isp_id_table[] = {
	{ .name    = "rpmsg-isp-debug" },
	{ },
};
MODULE_DEVICE_TABLE(platform, rpmsg_isp_id_table);
/*lint -save -e485*/
static struct rpmsg_driver hisp_rpmsg_driver = {
	.drv.name   = KBUILD_MODNAME,
	.drv.owner  = THIS_MODULE,
	.id_table   = rpmsg_isp_id_table,
	.probe      = hisp_rpmsg_isp_probe,
	.callback   = hisp_rpmsg_driver_cb,
	.remove     = hisp_rpmsg_isp_remove,
};
/*lint -restore */

#ifdef DEBUG_HISP
#define RPM_MAX_SIZE 1024
static ssize_t hispdbg_version_show(struct device *pdev,
			struct device_attribute *attr, char *buf)
{
	char *s = buf;

	s += snprintf_s(s, RPM_MAX_SIZE, RPM_MAX_SIZE-1, "[%s]\n", __func__);
	return (s - buf);
}

static ssize_t hispdbg_ctrl_show(struct device *pdev,
			struct device_attribute *attr, char *buf)
{
	char *s = buf;

	s += snprintf_s(s, RPM_MAX_SIZE, RPM_MAX_SIZE-1, "rpmsg device %s\n",
		(hisp_serv.enable_rpmsg == 0 ? "Disabled" : "Enabled"));

	return (s - buf);
}

static ssize_t hispdbg_ctrl_store(struct device *pdev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	char *p = NULL;
	unsigned int len = 0;

	p = memchr(buf, '\n', count);
	len = p ? (unsigned int)(p - buf) : count;
	if (len == 0)
		return -EINVAL;

	if (!strncmp(buf, "enable", len))
		hisp_serv.enable_rpmsg = 1;
	else if (!strncmp(buf, "disable", len))
		hisp_serv.enable_rpmsg = 0;
	else if (!strncmp(buf, "bypasspwrdn", len))
		hisp_serv.bypass_powerdn = 1;
	else if (!strncmp(buf, "usepwrdn", len))
		hisp_serv.bypass_powerdn = 0;
	else if (!strncmp(buf, "dumpregs", len))
		hisp_boot_stat_dump();
	else
		pr_info("<Usage: > echo enable/disable/dumpregs > ctrl\n");

	return count;
}

static ssize_t hispdbg_ipcrefs_show(struct device *pdev,
			struct device_attribute *attr, char *buf)
{
	struct hisp_rpmsgrefs_s *dev =
		(struct hisp_rpmsgrefs_s *)hisp_rpmsgrefs;
	char *s = buf;

	if (dev == NULL) {
		rpmsg_err("Failed: hisp_rpmsgrefs is null\n");
		return 0;
	}
	s += snprintf_s(s, RPM_MAX_SIZE, RPM_MAX_SIZE - 1,
	"%s send.(0x%x,0x%x,recv.(0x%x, 0x%x, 0x%x, 0x%x, 0x%x),total:0x%x\n",
	"hisp_rpmsgrefs", atomic_read(&dev->sendin_refs),
	atomic_read(&dev->sendx_refs), atomic_read(&dev->recvtask_refs),
	atomic_read(&dev->recvthread_refs), atomic_read(&dev->recvin_refs),
	atomic_read(&dev->recvcb_refs), atomic_read(&dev->recvdone_refs),
	atomic_read(&dev->rdr_refs));/*lint !e421 */

	return (s - buf);
}

static DEVICE_ATTR_RO(hispdbg_version);
static DEVICE_ATTR_RW(hispdbg_ctrl);
static DEVICE_ATTR_RO(hispdbg_ipcrefs);
static DEVICE_ATTR_RW(hisp_rdr);
static DEVICE_ATTR_RW(hisp_clk);
static DEVICE_ATTR_RW(hisp_power);
static DEVICE_ATTR_RO(hisp_sec_test_regs);
static DEVICE_ATTR_RW(hisp_boot_mode);
static DEVICE_ATTR_RO(hisp_loadbin_update);

static struct attribute *hisp_rpm_attrs[] = {
	&dev_attr_hispdbg_version.attr,
	&dev_attr_hispdbg_ctrl.attr,
	&dev_attr_hispdbg_ipcrefs.attr,
	&dev_attr_hisp_rdr.attr,
	&dev_attr_hisp_clk.attr,
	&dev_attr_hisp_power.attr,
	&dev_attr_hisp_sec_test_regs.attr,
	&dev_attr_hisp_boot_mode.attr,
	&dev_attr_hisp_loadbin_update.attr,
	NULL
};
ATTRIBUTE_GROUPS(hisp_rpm);


unsigned int __weak a7_mmu_map(struct scatterlist *sgl, unsigned int size,
					unsigned int prot, unsigned int flag)
{
	return 0;
}

void __weak a7_mmu_unmap(unsigned int va, unsigned int size)
{
}

#define HISP_MAX_RPMSG_BUF_SIZE	(512)
static int hispdbg_rpmsg_isp_cb(struct rpmsg_device *rpdev, void *data,
				int len, void *priv, u32 src)
{
	struct sk_buff *skb = NULL;
	char *skbdata = NULL;
	struct rpmsg_hdr *rpmsg_msg = NULL;
	int ret = 0;

	rpmsg_dbg("Enter ...\n");

	if (hisp_serv.state != RPMSG_CONNECTED) {
		/* get the remote address through the rpmsg_hdr */
		rpmsg_msg = container_of(data, struct rpmsg_hdr, data);
		rpmsg_dbg("msg src.%u, msg dst.%u\n",
				rpmsg_msg->src, rpmsg_msg->dst);

		/* add instance dst and modify the instance state */
		hisp_serv.dst = rpmsg_msg->src;
		hisp_serv.state = RPMSG_CONNECTED;
	}

	skb = alloc_skb(len, GFP_KERNEL);
	if (skb == NULL)
		return -ENOMEM;

	skbdata = skb_put(skb, len);/*lint !e64 */
	ret = memcpy_s(skbdata, HISP_MAX_RPMSG_BUF_SIZE, data, len);
	if (ret != 0) {
		rpmsg_err("Failed: memcpy_s skbdata fail, ret.%d", ret);
		kfree_skb(skb);
		return ret;
	}

	/* add skb to skb_queue */
	mutex_lock(&hisp_serv.lock);
	skb_queue_tail(&hisp_serv.queue, skb);
	mutex_unlock(&hisp_serv.lock);

	/* wake up any blocking processes, waiting for new data */
	wake_up_interruptible(&hisp_serv.readq);

	rpmsg_dbg("Exit ...\n");
	return 0;
}

static int hispdbg_set_rpmsg_debug_name(void)
{
	struct hisp_shared_para *share_para = NULL;

	hisp_lock_sharedbuf();
	share_para = hisp_share_get_para();
	if (share_para == NULL) {
		pr_err("%s:hisp_share_get_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return -EINVAL;
	}

	share_para->exc_flag |= (u32)CORE_RPMSG_DEBUG_NAME;

	pr_info("%s: exc flag = 0x%x\n", __func__, share_para->exc_flag);
	hisp_unlock_sharedbuf();

	return 0;
}

static int hispdbg_rproc_power_up(void)
{
	int ret = 0;
	unsigned long timeleft = 0;

	rpmsg_info("Enter...\n");

	ret = hisp_rproc_enable();
	if (ret != 0) {
		rpmsg_err("Failed : hisp_rproc_enable.%d\n", ret);
		return ret;
	}

	ret = hispdbg_set_rpmsg_debug_name();
	if (ret != 0) {
		rpmsg_err("Failed : hispdbg_set_rpmsg_debug_name.%d\n", ret);
		return ret;
	}

	timeleft  = wait_for_completion_timeout(&channel_sync,
			msecs_to_jiffies(300000000));
	if (!timeleft) {
		rpmsg_err("Failed: wait_for_completion_timeout timed out\n");
		hisp_rproc_disable();
		return -ETIMEDOUT;
	}
	rpmsg_info("Exit...\n");
	return 0;
}

static int hispdbg_rpmsg_isp_serv_init(void)
{
	struct hisp_rpmsg_service *isp_serv = &hisp_serv;
	char name[64];
	int ret;

	isp_serv->state = RPMSG_UNCONNECTED;
	/* assign a new, unique, local address and associate instance with it */
	isp_serv->ept = rpmsg_create_ept(isp_serv->rpdev,
				hispdbg_rpmsg_isp_cb, isp_serv, chinfo);
	if (isp_serv->ept == NULL) {
		rpmsg_err("create ept failed\n");
		return -ENOMEM;
	}

	/* create ion client for map */
	ret = snprintf_s(name, HISP_NAME_LENGTH, HISP_NAME_LENGTH - 1,
			"%u", task_pid_nr(current->group_leader));
	if (ret < 0) {
		rpmsg_err("Failed : snprintf_s name.%d\n", ret);
		goto create_failure;
	}

	/* the local addr */
	rpmsg_dbg("local addr assigned: %d\n", isp_serv->ept->addr);

	mutex_init(&isp_serv->lock);
	skb_queue_head_init(&isp_serv->queue);
	init_waitqueue_head(&isp_serv->readq);

	rpmsg_dbg("isp_serv->rpdev:src.%d, dst.%d, ept.dst.%d\n",
			isp_serv->rpdev->src, isp_serv->rpdev->dst,
			isp_serv->ept->rpdev->dst);

	return 0;

create_failure:
	rpmsg_destroy_ept(isp_serv->ept);
	isp_serv->ept = NULL;

	return -ENOMEM;
}

static struct hisp_buffer_node *hispdbg_create_buffer_node(int fd)
{
	struct hisp_buffer_node *node = NULL;

	node = kzalloc(sizeof(struct hisp_buffer_node), GFP_KERNEL);
	if (!node) {
		rpmsg_err("%s: fail to alloc sgtable_node", __func__);
		return ERR_PTR(-ENOMEM);
	}

	node->fd = fd;
	node->buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(node->buf)) {
		rpmsg_err("dma_buf_get fail, fd=%d\n", fd);
		goto fail_get_node_buf;
	}

	node->attachment = dma_buf_attach(node->buf, hisp_serv.mdev.this_device);
	if (IS_ERR_OR_NULL(node->attachment)) {
		rpmsg_err("dma_buf_attach error\n");
		goto fail_attach_buf;
	}

	node->sgt = dma_buf_map_attachment(node->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(node->sgt)) {
		rpmsg_err("dma_buf_map_attachment error\n");
		goto fail_map_buf;
	}

	return node;

fail_map_buf:
	dma_buf_detach(node->buf, node->attachment);
fail_attach_buf:
	dma_buf_put(node->buf);
fail_get_node_buf:
	kfree(node);
	return ERR_PTR(-ENOENT);
}

static struct hisp_buffer_node *hispdbg_find_buffer_node_by_fd(int fd)
{
	struct hisp_buffer_node *node = NULL;
	struct hisp_buffer_node *ret_node = ERR_PTR(-ENOENT);

	list_for_each_entry(node, &hisp_serv.buffer_nodes, list) {
		if (node->fd == fd) {
			ret_node = node;
			break;
		}
	}

	return ret_node;
}

static struct hisp_buffer_node *hispdbg_get_buffer_node_by_fd(int fd)
{
	struct hisp_buffer_node *node = NULL;

	mutex_lock(&hisp_serv.buffer_lock);
	node = hispdbg_find_buffer_node_by_fd(fd);
	if (!IS_ERR(node)) {
		mutex_unlock(&hisp_serv.buffer_lock);
		return node;
	}

	node = hispdbg_create_buffer_node(fd);
	if (!IS_ERR(node)) {
		list_add(&node->list, &hisp_serv.buffer_nodes);
		mutex_unlock(&hisp_serv.buffer_lock);
		return node;
	}
	mutex_unlock(&hisp_serv.buffer_lock);
	return ERR_PTR(-ENODEV);
}

static void hispdbg_put_buffer_node_by_fd(int fd)
{
	struct hisp_buffer_node *node = NULL;

	mutex_lock(&hisp_serv.buffer_lock);
	node = hispdbg_find_buffer_node_by_fd(fd);
	if (IS_ERR(node)) {
		rpmsg_err("%s: fail to find node by fd.%d", __func__, fd);
		goto err_out;
	}
	dma_buf_unmap_attachment(node->attachment, node->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(node->buf, node->attachment);
	dma_buf_put(node->buf);

	list_del(&node->list);
	kfree(node);

err_out:
	mutex_unlock(&hisp_serv.buffer_lock);
}

static long hispdbg_buffer_operation(unsigned int cmd,
		map_hisp_cfg_data_t *cfg)
{
	struct hisp_buffer_node *node = NULL;

	if (cfg == NULL) {
		rpmsg_err("Failed : map_hisp_cfg_data is NULL\n");
		return -EFAULT;
	}

	rpmsg_info("cfg sharefd = 0x%x, size = 0x%x, prot = %x, type = 0x%x\n",
				cfg->param.sharefd, cfg->param.size,
				cfg->param.prot, cfg->param.type);

	switch (cmd) {
	case HISP_GET_MAPBUFFER:
		node = hispdbg_get_buffer_node_by_fd(cfg->param.sharefd);
		if (IS_ERR(node) || (IS_ERR(node->sgt))) {
			rpmsg_err("Failed : get buff node\n");
			return -EFAULT;
		}

		if (node->buf->size != cfg->param.size) {
			rpmsg_err("Failed : size not equal\n");
			hispdbg_put_buffer_node_by_fd(cfg->param.sharefd);
			return -EFAULT;
		}

		cfg->param.moduleaddr = a7_mmu_map(node->sgt->sgl, cfg->param.size,
					cfg->param.prot, cfg->param.type);
		break;
	case HISP_UNMAP_BUFFER:
		a7_mmu_unmap(cfg->param.moduleaddr, cfg->param.size);
		hispdbg_put_buffer_node_by_fd(cfg->param.sharefd);
		break;
	default:
		rpmsg_err("Failed : not supported cmd.0x%x\n", cmd);
		break;
	}

	return 0;
}

static long hispdbg_pool_operation(unsigned int cmd,
		map_hisp_cfg_data_t *cfg)
{
	struct hisp_buffer_node *node = NULL;
	int ret;

	if (cfg == NULL) {
		rpmsg_err("Failed : map_hisp_cfg_data is NULL\n");
		return -EFAULT;
	}

	switch (cmd) {
	case HISP_POOL_STEUP:
		node = hispdbg_get_buffer_node_by_fd(cfg->param.sharefd);
		if (IS_ERR(node) || (IS_ERR(node->sgt))) {
			rpmsg_err("Failed : get buff node\n");
			return -EFAULT;
		}

		if (node->buf->size != cfg->param.pool_size) {
			rpmsg_err("Failed : size not equal\n");
			hispdbg_put_buffer_node_by_fd(cfg->param.sharefd);
			return -EFAULT;
		}

		cfg->param.pool_addr = hisp_mem_map_setup(node->sgt->sgl, cfg->param.iova,
				cfg->param.pool_size, cfg->param.prot,
				cfg->param.pool_num, cfg->param.type, 0x100000);
		if (cfg->param.pool_addr == 0) {
			rpmsg_err("Failed : hisp_mem_map_setup\n");
			hispdbg_put_buffer_node_by_fd(cfg->param.sharefd);
			return -EINVAL;
		}
		rpmsg_dbg("mem_iova = 0x%x, mem_pool_addr = 0x%x, size = 0x%x,",
			cfg->param.iova, cfg->param.pool_addr,
			cfg->param.pool_size);
		rpmsg_dbg(" prot = %x, pool_num = %d\n",
			cfg->param.prot, cfg->param.pool_num);
		break;
	case HISP_POOL_DESTROY:
		hisp_mem_pool_destroy(cfg->param.pool_num);
		hispdbg_put_buffer_node_by_fd(cfg->param.sharefd);
		break;
	case HISP_POOL_ALLOC:
		cfg->param.moduleaddr = hisp_mem_pool_alloc_iova(
				cfg->param.size, cfg->param.pool_num);
		rpmsg_dbg("hisp_mem_pool_alloc_iova(0x%x, 0x%x, 0x%x)\n",
			cfg->param.moduleaddr, cfg->param.size,
			cfg->param.pool_num);
		break;
	case HISP_POOL_FREE:
		ret = hisp_mem_pool_free_iova(cfg->param.pool_num,
				cfg->param.moduleaddr, cfg->param.size);
		if (ret != 0)
			rpmsg_err("Failed : hisp_mem_pool_free_iova.%d\n", ret);

		rpmsg_dbg("hisp_mem_pool_free_iova(0x%x, 0x%x, 0x%x)\n",
			cfg->param.moduleaddr, cfg->param.size,
			cfg->param.pool_num);
		break;
	default:
		rpmsg_err("Failed : not supported cmd.0x%x\n", cmd);
		break;
	}

	return 0;
}

static long hispdbg_isp_mem_operation(unsigned int cmd,
		unsigned long arg)
{
	map_hisp_cfg_data_t cfg;
	int ret = 0;

	if (arg == 0){
		rpmsg_err("argp is null\n");
		return -EINVAL;
	}

	if (_IOC_SIZE(cmd) > sizeof(cfg)) {
		rpmsg_err("cmd arg size too large");
		return -EINVAL;
	}

	ret = (int)copy_from_user(&cfg, (void __user *)arg, sizeof(cfg));
	if (ret != 0) {
		rpmsg_err("Failed : copy_from_user.%d\n", ret);
		return -EFAULT;
	}

	switch (cmd) {
	case HISP_GET_MAPBUFFER:
	case HISP_UNMAP_BUFFER:
		ret = hispdbg_buffer_operation(cmd, &cfg);
		if (ret != 0) {
			rpmsg_err("hispdbg_buffer_operation fail\n");
			return -EFAULT;
		}
		break;
	case HISP_POOL_STEUP:
	case HISP_POOL_DESTROY:
	case HISP_POOL_ALLOC:
	case HISP_POOL_FREE:
		ret = hispdbg_pool_operation(cmd, &cfg);
		if (ret != 0) {
			rpmsg_err("hispdbg_pool_operation fail\n");
			return -EFAULT;
		}
		break;
	default:
		rpmsg_err("Failed : not supported cmd.0x%x\n", cmd);
		break;
	}

	ret = (int)copy_to_user((void __user *)arg,
				(const void *)&cfg, sizeof(cfg));
	if (ret != 0) {
		rpmsg_err("Failed : copy_to_user.%d.(0x%lx, 0x%x, 0x%x)\n",
			ret, arg, cfg.param.moduleaddr, cfg.param.size);
		return -EFAULT;
	}

	return 0;
}

static long hispdbg_sensor_power_operation(unsigned int cmd,
		unsigned long arg)
{
	rpmsg_ioctl_arg_t info;
	char sensor_name[SENSOR_NAME_SIZE];
	int ret;

	if (_IOC_SIZE(cmd) > sizeof(info)) {
		rpmsg_err("cmd arg size too large");
		return -EINVAL;
	}

	ret = (int)copy_from_user(&info,
		(void __user *)(uintptr_t)arg, sizeof(info));
	if (ret != 0) {
		rpmsg_err("Failed : copy_from_user.%d\n", ret);
		return -EFAULT;
	}

	ret = memset_s(sensor_name, SENSOR_NAME_SIZE, '\0', SENSOR_NAME_SIZE);
	if (ret != 0)
		pr_err("%s: memset_s sensor_name to 0 fail.%d!\n",
			__func__, ret);

	info.name[SENSOR_NAME_SIZE - 1] = '\0';
	ret = memcpy_s(sensor_name, SENSOR_NAME_SIZE, info.name,
			((strlen(info.name) > SENSOR_NAME_SIZE) ?
			SENSOR_NAME_SIZE : strlen(info.name)));
	if (ret != 0) {
		rpmsg_err("Failed : memcpy_s sensor_name.%d\n", ret);
		return -EFAULT;
	}

	sensor_name[SENSOR_NAME_SIZE - 1] = '\0';
	rpmsg_info("info.index = %d, info.name = %s\n",
			info.index, sensor_name);
	ret = rpmsg_sensor_ioctl(cmd, info.index, sensor_name);
	if (ret < 0) {
		rpmsg_info("Failed : hisprpm_sensor_ioctl.%d\n", ret);
		return ret;
	}

	return 0;
}

static int hispdbg_map_iommu(struct memory_block_s *buf)
{
	unsigned int iova;
	unsigned long size;
	struct device *dev = NULL;
	struct hisp_buffer_node *node = NULL;

	rpmsg_info("+\n");

	if (buf->shared_fd < 0) {
		rpmsg_err("Failed : hare_fd.%d\n", buf->shared_fd);
		return -EINVAL;
	}

	dev = hisp_get_device();
	if (dev == NULL) {
		rpmsg_err("Failed : dev.%pK\n", dev);
		return -EFAULT;
	}

	node = hispdbg_get_buffer_node_by_fd(buf->shared_fd);
	if (IS_ERR(node) || (IS_ERR_OR_NULL(node->buf))) {
		rpmsg_err("Failed : get buff node\n");
		return -EFAULT;
	}

	if (node->buf->size != (unsigned int)buf->size) {
		rpmsg_err("Failed : size not equal\n");
		goto err_dma_buf_get;
	}

	iova = kernel_iommu_map_dmabuf(dev, node->buf, buf->prot, &size);
	if (iova == 0) {
		rpmsg_err("Failed : isp_iommu_map_dmabuf\n");
		goto err_dma_buf_get;
	}

	if (buf->size != size) {
		rpmsg_err("isp_iommu_map_range failed: %d len 0x%lx\n",
				buf->size, size);
		goto err_dma_buf_get;
	}

	buf->da = iova;
	rpmsg_info("[after map iommu]da.(0x%x)", buf->da);/*lint !e626 */
	return 0;

err_dma_buf_get:
	hispdbg_put_buffer_node_by_fd(buf->shared_fd);
	return -ENOMEM;
}

static void hispdbg_unmap_iommu(struct memory_block_s *buf)
{
	int ret = 0;
	struct device *dev = NULL;
	struct hisp_buffer_node *node = NULL;

	rpmsg_info("+\n");

	dev = hisp_get_device();
	if (dev == NULL) {
		rpmsg_err("Failed : dev.%pK\n", dev);
		return;
	}

	node = hispdbg_get_buffer_node_by_fd(buf->shared_fd);
	if (IS_ERR(node) || (IS_ERR_OR_NULL(node->buf))) {
		rpmsg_err("Failed : get buff node\n");
		return;
	}

	ret = kernel_iommu_unmap_dmabuf(dev, node->buf, buf->da);
	if (ret < 0)
		rpmsg_err("Failed : iommu_unmap_dmabuf\n");

	hispdbg_put_buffer_node_by_fd(buf->shared_fd);
	rpmsg_info("-\n");
}

static long hispdbg_isp_iommu_operation(unsigned int cmd,
		unsigned long arg)
{
	struct hisp_buffer_node *node = NULL;
	struct memory_block_s buf = { 0 };
	int ret;

	if (arg == 0){
		rpmsg_err("argp is null\n");
		return -EINVAL;
	}

	if (_IOC_SIZE(cmd) > sizeof(struct memory_block_s)) {
		rpmsg_err("cmd arg size too large\n");
		return -EINVAL;
	}

	ret = (int)copy_from_user(&buf, (void __user *)arg,
					sizeof(struct memory_block_s));
	if (ret != 0) {
		rpmsg_err("copy_from_user.%d\n", ret);
		return -EFAULT;
	}

	switch (cmd) {
	case HISP_ALLOC_CPU_MEM:
		node = hispdbg_get_buffer_node_by_fd(buf.shared_fd);
		if (IS_ERR(node) || (IS_ERR(node->sgt))) {
			rpmsg_err("Failed : get buff node\n");
			return -EFAULT;
		}

		if (node->buf->size != (unsigned int)buf.size) {
			rpmsg_err("Failed : size not equal\n");
			hispdbg_put_buffer_node_by_fd(buf.shared_fd);
			return -EFAULT;
		}

		buf.da = hisp_alloc_cpu_map_addr(node->sgt->sgl,
						buf.prot, buf.size, MEM_ALIGN_SIZE);
		if (!buf.da) {
			rpmsg_err("Failed : alloc cpu addr\n");
			hispdbg_put_buffer_node_by_fd(buf.shared_fd);
			}
		break;
	case HISP_FREE_CPU_MEM:
		ret = hisp_free_cpu_map_addr(buf.da, buf.size);
		if (ret != 0)
			rpmsg_err("%s: hisp_free_cpu_map_addr failed", __func__);
		hispdbg_put_buffer_node_by_fd(buf.shared_fd);
		break;
	/* warining: the da not control, this func only for test */
	case HISP_MAP_IOMMU:
		rpmsg_info("cmd.HISP_MAP_IOMMU\n");
		ret = hispdbg_map_iommu(&buf);
			if (ret != 0) {
				rpmsg_err("Failed: rpm_map_iommu.%d\n", ret);
				return ret;
			}

		break;
	case HISP_UNMAP_IOMMU:
		hispdbg_unmap_iommu(&buf);
		break;
	default:
		rpmsg_err("Failed : not supported cmd.0x%x\n", cmd);
		break;
	}
	ret = (int)copy_to_user((void __user *)arg, &buf,
				sizeof(struct memory_block_s));
	if (ret != 0) {
		rpmsg_err("copy_to_user.%d\n", ret);
		return -EFAULT;
	}

	return 0;
}

static int hispdbg_rpmsg_isp_open(struct inode *inode, struct file *filp)
{
	struct hisp_rpmsg_service *isp_serv = &hisp_serv;
	int ret;

	rpmsg_info("Enter...\n");

	if (!isp_serv->enable_rpmsg) {
		rpmsg_err("Failed : enable_rpmsg.%d\n",
				isp_serv->enable_rpmsg);
		return -ENODEV;
	}

	mutex_lock(&isp_rpmsg_service_mutex);
	if ((atomic_read(&instances) > 0) || (hisp_rproc_enable_status() > 0)) {
		rpmsg_err("Failed : rpmsg had been started, ins_count.%d\n",
				atomic_read(&instances));
		mutex_unlock(&isp_rpmsg_service_mutex);
		return -ENODEV;
	}

	ret = hispdbg_rproc_power_up();
	if (ret) {
		rpmsg_err("power up failed.\n");
		goto pwrup_failure;
	}

	ret = hispdbg_rpmsg_isp_serv_init();
	if (ret) {
		rpmsg_err("hisp serv init failed.\n");
		goto serv_init_failure;
	}
	/* associate filp with the new instance */
	filp->private_data = isp_serv;

	atomic_set(&instances, 1);
	rpmsg_info("ins_count = %d\n", atomic_read(&instances));

	mutex_unlock(&isp_rpmsg_service_mutex);

	rpmsg_info("Exit ...\n");
	return 0;

serv_init_failure:
	hisp_rproc_disable();
pwrup_failure:
	mutex_unlock(&isp_rpmsg_service_mutex);

	rpmsg_err("open failed.\n");
	return -ENOMEM;
}

static int hispdbg_rpmsg_isp_release(struct inode *inode, struct file *filp)
{
	struct hisp_rpmsg_service *isp_serv = &hisp_serv;
	int ret = 0;

	rpmsg_info("Enter ...\n");
	if (isp_serv == NULL) {
		rpmsg_err("rpmsg_send failed: %d\n", ret);
		return -EINVAL;
	}

	mutex_lock(&isp_rpmsg_service_mutex);
	if (!atomic_read(&instances)) {
		rpmsg_err("Failed : rpmsg not start, instances.%d\n",
				atomic_read(&instances));
		mutex_unlock(&isp_rpmsg_service_mutex);
		return -ENODEV;
	}

	/* destroy ept */
	if (isp_serv->ept != NULL) {
		rpmsg_destroy_ept(isp_serv->ept);
		isp_serv->ept = NULL;
	}

	if (!isp_serv->bypass_powerdn) {
		rpmsg_info("enter rproc disable.\n");

		hisp_rproc_disable();
	}

	atomic_set(&instances, 0);
	rpmsg_info("ins_count = %d\n", atomic_read(&instances));
	mutex_unlock(&isp_rpmsg_service_mutex);

	rpmsg_info("Exit ...\n");
	return 0;
}

//lint -save -e30 -e142 -e838 -e774 -e712
static long hispdbg_rpmsg_isp_ioctl(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	int ret;

	rpmsg_info("+ cmd.0x%x\n", cmd);
	if ((void __user *)arg == NULL) {
		rpmsg_err("arg is NULL\n");
		return -EINVAL;
	}

	mutex_lock(&hisp_serv.ctrl_lock);
	switch (cmd) {
	case HISP_GET_MAPBUFFER:
	case HISP_UNMAP_BUFFER:
	case HISP_POOL_STEUP:
	case HISP_POOL_DESTROY:
	case HISP_POOL_ALLOC:
	case HISP_POOL_FREE:
	case HISP_SECISP_TA_MAP:
	case HISP_SECISP_TA_UNMAP:
		ret = hispdbg_isp_mem_operation(cmd, arg);
		if (ret) {
			rpmsg_err("hisp memory operation fail\n");
			goto fail_ioctrl_cmd;
		}
		break;
	case HWSENSOR_IOCTL_POWER_UP:
	case HWSENSOR_IOCTL_POWER_DOWN:
		ret = hispdbg_sensor_power_operation(cmd, arg);
		if (ret) {
			rpmsg_err("sensor power operation fail\n");
			goto fail_ioctrl_cmd;
		}
		break;
	case HISP_ALLOC_CPU_MEM:
	case HISP_FREE_CPU_MEM:
	case HISP_MAP_IOMMU:
	case HISP_UNMAP_IOMMU:
		ret = hispdbg_isp_iommu_operation(cmd, arg);
		if (ret) {
			rpmsg_err("isp iommu operation fail\n");
			goto fail_ioctrl_cmd;
		}
		break;
	default:
		rpmsg_err("Failed : not supported cmd.0x%x\n", cmd);
		goto fail_ioctrl_cmd;
	}

	mutex_unlock(&hisp_serv.ctrl_lock);
	rpmsg_info("- cmd.0x%x\n", cmd);

	return 0;

fail_ioctrl_cmd:
	mutex_unlock(&hisp_serv.ctrl_lock);
	return -EFAULT;
}
//lint -restore

static ssize_t hispdbg_rpmsg_isp_read(struct file *filp, char __user *buf,
		size_t len, loff_t *offp)
{
	struct sk_buff *skb = NULL;
	int use;
	int empty;

	rpmsg_dbg("Enter ...\n");
	if (buf == NULL) {
		rpmsg_err("buf is NULL\n");
		return -ERESTARTSYS;
	}
	if (mutex_lock_interruptible(&hisp_serv.lock))
		return -ERESTARTSYS;

	/* check if skb_queue is NULL ?*/
	if (skb_queue_empty(&hisp_serv.queue)) {
		mutex_unlock(&hisp_serv.lock);/*lint !e455 */
		/* non-blocking requested ? return now */
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		/* otherwise block, and wait for data */
		empty = skb_queue_empty(&hisp_serv.queue);
		if (wait_event_interruptible(hisp_serv.readq, !empty))
			return -ERESTARTSYS;

		if (mutex_lock_interruptible(&hisp_serv.lock))
			return -ERESTARTSYS;
	}

	skb = skb_dequeue(&hisp_serv.queue);
	if (skb == NULL) {
		mutex_unlock(&hisp_serv.lock);/*lint !e455 */
		rpmsg_err("skb_dequeue failed.\n");
		return -EIO;
	}

	mutex_unlock(&hisp_serv.lock);/*lint !e455 */

	use = min_t(unsigned int, len, skb->len);
	if (copy_to_user(buf, skb->data, use)) {
		rpmsg_err("copy_to_user fail.\n");
		use = -EFAULT;
	}
	rpmsg_dbg("api_name.%d, message_id.0x%x\n",
			((isp_msg_t *)skb->data)->api_name,
			((isp_msg_t *)skb->data)->message_id);

	kfree_skb(skb);

	rpmsg_dbg("Exit ...\n");
	return use;
}

static int rpmsg_send_single_to_connect(
	struct hisp_rpmsg_service *priv, void *data, unsigned int len)
{
	struct hisp_rpmsg_service *isp_serv = priv;
	int ret = 0;

	if (isp_serv == NULL) {
		rpmsg_err("rpmsg_send failed: %d\n", ret);
		return -EINVAL;
	}

	rpmsg_dbg("Enter...\n");
	rpmsg_dbg("before single send, src.%d, dst.%d n",
		isp_serv->ept->addr, isp_serv->rpdev->dst);

	ret = rpmsg_send_offchannel(hisp_serv.ept, isp_serv->ept->addr,
					isp_serv->rpdev->dst, data, len);
	if (ret) {
		rpmsg_err("rpmsg_send failed: %d\n", ret);
		return ret;
	}

	rpmsg_dbg("Exit...\n");
	return 0;
}

static ssize_t hispdbg_rpmsg_isp_write(struct file *filp,
		const char __user *ubuf, size_t len, loff_t *offp)
{
	struct hisp_rpmsg_service *isp_serv = filp->private_data;
	char msg_buf[RPMSG_MSG_SIZE] = { 0 };
	int use = 0;
	int ret = 0;

	rpmsg_dbg("Enter ...\n");

	if (isp_serv == NULL) {
		rpmsg_err("rpmsg_send failed: %d\n", ret);
		return -EINVAL;
	}

	if (ubuf == NULL)
		return -EINVAL;

	/* for now, limit msg size to 512 bytes */
	use = min_t(unsigned int, RPMSG_MSG_SIZE, len);
	if (copy_from_user(msg_buf, ubuf, use))
		return -EMSGSIZE;

	rpmsg_dbg("api_name.%d, message_id.0x%x\n",
		((isp_msg_t *)msg_buf)->api_name,
		((isp_msg_t *)msg_buf)->message_id);

	mutex_lock(&isp_rpmsg_send_mutex);

	/*
	 * if the msg is first msg, we will send it
	 * by the special function to connect
	 */
	if (isp_serv->state != RPMSG_CONNECTED) {
		ret = rpmsg_send_single_to_connect(isp_serv, msg_buf, use);
		if (ret) {
			rpmsg_err("rpmsg send single failed: %d\n", ret);
			mutex_unlock(&isp_rpmsg_send_mutex);
			return ret;
		}
		mutex_unlock(&isp_rpmsg_send_mutex);
		return use;
	}
	rpmsg_dbg("before send, src.%d, dst.%d\n",
		isp_serv->ept->addr, isp_serv->dst);
	/* here, in debug stage, we need to do something for the msg */
	ret = rpmsg_send_offchannel(isp_serv->ept, isp_serv->ept->addr,
					isp_serv->dst, msg_buf, use);
	if (ret) {
		rpmsg_err("rpmsg_send failed: %d\n", ret);
		mutex_unlock(&isp_rpmsg_send_mutex);
		return ret;
	}

	mutex_unlock(&isp_rpmsg_send_mutex);
	rpmsg_dbg("Exit ...\n");
	return use;
}

static unsigned int hispdbg_rpmsg_isp_poll(
		struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	rpmsg_dbg("Enter ...\n");
	if (mutex_lock_interruptible(&hisp_serv.lock))
		return -ERESTARTSYS;/*lint !e570 */

	poll_wait(filp, &hisp_serv.readq, wait);
	if (!skb_queue_empty(&hisp_serv.queue))
		mask |= POLLIN | POLLRDNORM;

	if (true)
		mask |= POLLOUT | POLLWRNORM;

	mutex_unlock(&hisp_serv.lock);/*lint !e455 */

	rpmsg_dbg("Exit ...\n");
	return mask;
}

static const struct file_operations hisp_rpm_fops = {
	.open           = hispdbg_rpmsg_isp_open,
	.release        = hispdbg_rpmsg_isp_release,
	.unlocked_ioctl = hispdbg_rpmsg_isp_ioctl,
	.compat_ioctl   = hispdbg_rpmsg_isp_ioctl,
	.read           = hispdbg_rpmsg_isp_read,
	.write          = hispdbg_rpmsg_isp_write,
	.poll           = hispdbg_rpmsg_isp_poll,
	.owner          = THIS_MODULE,
};

static int hispdbg_attach_misc(struct miscdevice *dev,
				const struct file_operations *fops,
				const struct attribute_group **groups)
{
	rpmsg_dbg("Enter ...\n");
	if (dev == NULL) {
		rpmsg_err("Failed : dev.%pK\n", dev);
		return -ENOMEM;
	}

	if (!fops || !groups)
		rpmsg_err("Warnning : fops.%pK, groups.%pK\n", fops, groups);

	dev->minor     = MISC_DYNAMIC_MINOR;
	dev->name      = KBUILD_MODNAME;
	dev->groups    = groups;
	dev->fops      = fops;
	rpmsg_dbg("Exit ...\n");

	return 0;
}
#endif

static int __init hisp_rpmsg_init(void)
{
#ifdef DEBUG_HISP
	int ret;

	rpmsg_dbg("Enter ...\n");
	atomic_set(&instances, 0);
	ret = hispdbg_attach_misc(&hisp_serv.mdev,
			&hisp_rpm_fops, hisp_rpm_groups);
	if (ret != 0)
		rpmsg_err("Failed : hispdbg_attach_misc.%d\n", ret);

	ret = misc_register(&hisp_serv.mdev);
	if (ret != 0)
		rpmsg_err("Failed : misc_register.%d\n", ret);
	hisp_serv.enable_rpmsg = 0;
	hisp_serv.bypass_powerdn = 0;
	INIT_LIST_HEAD(&hisp_serv.buffer_nodes);
	mutex_init(&hisp_serv.buffer_lock);
	mutex_init(&hisp_serv.ctrl_lock);
#endif
	init_completion(&channel_sync);
	return register_rpmsg_driver(&hisp_rpmsg_driver);
}
module_init(hisp_rpmsg_init);

static void __exit hisp_rpmsg_exit(void)
{
	rpmsg_dbg("Enter ...\n");

	unregister_rpmsg_driver(&hisp_rpmsg_driver);

#ifdef DEBUG_HISP
	misc_deregister(&hisp_serv.mdev);
	mutex_destroy(&hisp_serv.buffer_lock);
	mutex_destroy(&hisp_serv.ctrl_lock);
#endif

	rpmsg_dbg("Exit ...\n");
}
module_exit(hisp_rpmsg_exit);

MODULE_DESCRIPTION("ISP offloading rpmsg driver");
MODULE_LICENSE("GPL v2");
