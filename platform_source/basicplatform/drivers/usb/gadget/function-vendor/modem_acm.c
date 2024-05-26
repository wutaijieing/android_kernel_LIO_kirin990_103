/*
 * USB modem function driverUSB modem function driver
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "modem_acm.h"

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/module.h>

#include <platform_include/basicplatform/linux/usb/drv_acm.h>
#include <platform_include/basicplatform/linux/usb/drv_udi.h>
#include <securec.h>

#include "usb_vendor.h"

#define gs_acm_evt_push(port, evt) __gs_acm_evt_push(port, evt)

#define DMA_ADDR_INVALID		(~(dma_addr_t)0)
#define ACM_CDEV_DFT_RD_BUF_SIZE	2048
#define ACM_CDEV_DFT_RD_REQ_NUM		8
#define ACM_CDEV_DFT_WT_REQ_NUM		16
#define ACM_CDEV_AT_WT_REQ_NUM		256
#define ACM_CDEV_MODEM_WT_REQ_NUM	512
#define ACM_CDEV_NAME_MAX		64
#define ACM_CDEV_MAX_XFER_SIZE		(0x1000000 - 1)
#define DOMAIN_NAME_LEN			8
#define WAIT_TIME			300
#define DTERATE				115200
#define CHARFORMAT			8
#define DELAYTIME			50

struct acm_name_type_tbl g_acm_cdev_type_table[ACM_CDEV_COUNT] = {
	/* name		type(protocol id) */
	/* PC UI interface, sh */
	{ "acm_at",		USB_IF_PROTOCOL_PCUI },
	/* 3G Application interface, bj */
	{ "acm_3g_diag",		USB_IF_PROTOCOL_3G_DIAG },
	/* 3G gps inerface, unused */
	{ "acm_a_shell",		USB_IF_PROTOCOL_3G_GPS },
	/* Bluetooth interface, mntn */
	{ "acm_c_shell",		USB_IF_PROTOCOL_BLUETOOTH },
	/* Control interface, sh */
	{ "acm_ctrl",		USB_IF_PROTOCOL_CTRL },
	/* Application interface, bj dl log */
	{ "acm_4g_diag",		USB_IF_PROTOCOL_DIAG },
	/* GPS interface, hids control */
	{ "acm_gps",		USB_IF_PROTOCOL_GPS },
	/* CDMA log interface, maybe unused */
	{ "acm_cdma_log",	USB_IF_PROTOCOL_CDMA_LOG },
	/* sh */
	{ "acm_skytone",		USB_IF_PROTOCOL_SKYTONE },
	/* sh */
	{ "acm_modem",		USB_IF_PROTOCOL_MODEM },
};

int is_modem_port(unsigned int port_num)
{
	if (port_num >= ACM_CDEV_COUNT)
		return 0;
	return g_acm_cdev_type_table[port_num].type == USB_IF_PROTOCOL_MODEM;
}

/* function name should be verb + noun */
unsigned char get_acm_protocol_type(unsigned int port_num)
{
	if (port_num >= ACM_CDEV_COUNT)
		return 0;
	return (unsigned char)g_acm_cdev_type_table[port_num].type;
}

char *get_acm_cdev_name(unsigned int index)
{
	if ((index >= ACM_CDEV_COUNT) ||
		(g_acm_cdev_type_table[index].name == NULL))
		return "unknown";
	else
		return g_acm_cdev_type_table[index].name;
}


/* Global work queue for all ports of modem_acm. */
static struct workqueue_struct *acm_work_queue;

struct gs_acm_cdev_rw_priv {
	struct list_head list;
	bool is_sync;
	bool is_copy;
	int copy_pos;
	struct usb_request *req;
	struct kiocb *iocb;
	void *user_p_skb;
	struct timespec64 tx_start_time;
};

static struct acm_cdev_port_manager {
	char name_domain[DOMAIN_NAME_LEN];
	struct mutex open_close_lock; /* protect open/close */
	struct gs_acm_cdev_port *port;
	ACM_EVENT_CB_T event_cb; /* when usb is remove */
} gs_acm_cdev_ports[ACM_CDEV_COUNT];

static struct notifier_block gs_acm_nb;
static struct notifier_block *gs_acm_nb_ptr;

static unsigned int gs_acm_cdev_n_ports;

static struct gs_acm_evt_manage gs_acm_write_evt_manage;
static struct gs_acm_evt_manage gs_acm_read_evt_manage;
static struct gs_acm_evt_manage gs_acm_sig_stat_evt_manage;


static void gs_acm_cdev_free_request(struct usb_ep *ep,
		struct usb_request  *req);
static struct usb_request *gs_acm_cdev_alloc_request(struct usb_ep *ep,
		unsigned int buf_size);
static void gs_acm_cdev_free_requests(struct usb_ep *ep,
		struct list_head *head, int *allocated);
static void gs_acm_cdev_write_complete(struct usb_ep *ep,
		struct usb_request *req);
static void gs_acm_cdev_read_complete(struct usb_ep *ep,
		struct usb_request *req);

static void gs_acm_evt_init(struct gs_acm_evt_manage *evt, const char *name)
{
	spin_lock_init(&evt->evt_lock);
	evt->port_evt_pos = 0;
	evt->name = name;
	memset(evt->port_evt_array, 0, sizeof(evt->port_evt_array));
}

static void __gs_acm_evt_push(struct gs_acm_cdev_port *port, struct gs_acm_evt_manage *evt)
{
	unsigned long flags;
	int add_new = 1;
	int i;

	spin_lock_irqsave(&evt->evt_lock, flags);
	for (i = 0; i <= evt->port_evt_pos; i++) {
		if (evt->port_evt_array[i] == port) {
			add_new = 0;
			break;
		}
	}
	if (add_new) {
		if (evt->port_evt_pos >= ACM_CDEV_COUNT + 1) {
			pr_err("[MODEM_USB][%s]port_evt_pos exceeds\n", __func__);
			spin_unlock_irqrestore(&evt->evt_lock, flags);
			return;
		}
		evt->port_evt_array[evt->port_evt_pos] =  port;
		evt->port_evt_pos++;
	}
	spin_unlock_irqrestore(&evt->evt_lock, flags);
}

static void *gs_acm_evt_get(struct gs_acm_evt_manage *evt)
{
	unsigned long flags;
	struct gs_acm_cdev_port *ret_port = NULL;

	spin_lock_irqsave(&evt->evt_lock, flags);
	if (evt->port_evt_pos > 0) {
		ret_port = evt->port_evt_array[evt->port_evt_pos - 1];
		evt->port_evt_array[evt->port_evt_pos - 1] = NULL;
		evt->port_evt_pos--;
	}
	spin_unlock_irqrestore(&evt->evt_lock, flags);

	return ret_port;
}

static unsigned int gs_acm_cdev_get_tx_buf_num(unsigned int index)
{
	switch (get_acm_protocol_type(index)) {
	case USB_IF_PROTOCOL_PCUI:
		return ACM_CDEV_AT_WT_REQ_NUM;
	case USB_IF_PROTOCOL_MODEM:
		return ACM_CDEV_MODEM_WT_REQ_NUM;
	default:
		break;
	}
	return ACM_CDEV_DFT_WT_REQ_NUM;
}

static struct usb_request *get_write_req(struct gs_acm_cdev_port *port)
{
	struct usb_request *req = NULL;
	struct list_head *pool = &port->write_pool;

	acm_dbg("port_num %u\n", port->port_num);

	if (unlikely(list_empty(pool))) {
		if (!is_modem_port(port->port_num))
			pr_info("acm[%s]%s: no req\n", __func__,
				get_acm_cdev_name(port->port_num));
		port->stat_tx_no_req++;
		return NULL;
	}

	/* get a write req from the write pool */
	req = list_entry(pool->next, struct usb_request, list);
	list_del_init(&req->list);
	port->write_started++;

	return req;
}

static int do_copy_data(struct gs_acm_cdev_port *port, const char *buf,
			struct usb_request *req,  unsigned int len)
{
	acm_dbg("do copy\n");
	/* alloc a new buffer first time or the room is not enough */
	if (req->length == 0 || req->length < len) {
		kfree(req->buf);
		req->buf = NULL;
		req->length = 0;
		req->buf = kzalloc(len, GFP_KERNEL);
		if (req->buf == NULL) {
			pr_err("gs_acm_cdev_start_tx:malloc req->buf error\n");
			port->stat_tx_alloc_fail++;
			return -ENOMEM;
		}
		req->length = len;
	}
	/* we don't need to free req->buf, if fail */
	if (!buf || copy_from_user(req->buf, buf, len)) {
		port->stat_tx_copy_fail++;
		return -ENOMEM;
	}
	return 0;
}

/*
 * gs_acm_cdev_start_tx
 *
 * This function finds available write requests, calls
 * usb_ep_queue to send the data.
 *
 */
int gs_acm_cdev_start_tx(struct gs_acm_cdev_port *port,
			char *buf, char *paddr, unsigned int len, bool is_sync)
{
	int status;
	struct usb_request *req = NULL;
	struct gs_acm_cdev_rw_priv *write_priv = NULL;
	unsigned long flags;
	struct sk_buff *send_skb = NULL;
	bool is_do_copy = false;

	if (port == NULL)
		return -ENODEV;
	is_do_copy = port->is_do_copy;
	spin_lock_irqsave(&port->port_lock, flags);
	if (!port->is_connect) {
		spin_unlock_irqrestore(&port->port_lock, flags);
		return -ESHUTDOWN;
	}
	req = get_write_req(port);
	spin_unlock_irqrestore(&port->port_lock, flags);

	if (req == NULL)
		return -EFAULT;

	if (is_modem_port(port->port_num)) {
		send_skb = (struct sk_buff *)buf;
		buf = send_skb->data;
		len = send_skb->len;
	}
	/* check whether copy the data */
	if (is_do_copy) {
		status = do_copy_data(port, buf, req, len);
		if (status < 0) {
			spin_lock_irqsave(&port->port_lock, flags);
			list_add_tail(&req->list, &port->write_pool);
			port->write_started--;
			spin_unlock_irqrestore(&port->port_lock, flags);
			return status;
		}
	} else {
		acm_dbg("no copy\n");
		req->buf = buf;
		if (paddr != NULL)
			req->dma = (dma_addr_t)(uintptr_t)paddr;
		else
			req->dma = DMA_ADDR_INVALID;
	}

	req->length = len;
	write_priv = req->context;
	write_priv->user_p_skb = send_skb;
	write_priv->is_sync = is_sync;
	write_priv->is_copy = is_do_copy;
	ktime_get_real_ts64(&(write_priv->tx_start_time));

	spin_lock_irqsave(&port->port_lock, flags);
	if (port->port_usb == NULL) {
		status = -ENODEV;
		port->stat_tx_disconnect++;
		pr_err("[%s] no usb port\n", __func__);
		goto tx_fail_restore;
	}

	/* wait the write req complete */
	if (is_sync)
		port->write_blocked = 1;

	if (len % port->port_usb->in->maxpacket == 0)
		req->zero = 1;

	status = usb_ep_queue(port->port_usb->in, req, GFP_ATOMIC);
	if (status) {
		pr_err("%s: usb_ep_queue fail, status %d!\n", __func__, status);
		port->stat_tx_submit_fail++;
		goto tx_fail_restore;
	}
	port->stat_tx_submit_bytes += len;
	port->stat_tx_submit++;
	spin_unlock_irqrestore(&port->port_lock, flags);

	return 0;

tx_fail_restore:
	list_add_tail(&req->list, &port->write_pool);
	port->write_started--;
	port->write_blocked = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);
	return status;
}

/*
 * submit_read_req
 * This function can only be called by gs_acm_cdev_start_rx
 */
static int submit_read_req(struct gs_acm_cdev_port *port, struct list_head *pool)
{
	int status = 0;
	struct usb_request  *req = NULL;
	struct usb_ep  *out = NULL;
	struct gs_acm_cdev_rw_priv *rw_priv = NULL;

	out = port->port_usb->out;
	while (!list_empty(pool)) {
		/* revise the pool length to smaller */
		if ((unsigned int)port->read_started >= port->read_req_num) {
			pr_emerg("%s: try to shrink the read buff num to %u at port %u\n",
				__func__, port->read_req_num, port->port_num);

			gs_acm_cdev_free_requests(out, pool, &port->read_allocated);
			break;
		}

		req = list_first_entry(pool, struct usb_request, list);
		list_del_init(&req->list);
		port->read_started++;

		if (req->length < port->read_buf_size) {
			acm_dbg("realloc req\n");
			gs_acm_cdev_free_request(out, req);
			req = gs_acm_cdev_alloc_request(out, port->read_buf_size);
			if (req == NULL) {
				pr_err("[%s]gs_acm_cdev_alloc_request is fail\n", __func__);
				return -1;
			}
			req->complete = gs_acm_cdev_read_complete;
		}

		status = usb_ep_queue(out, req, GFP_ATOMIC);
		if (status) {
			pr_err("[%s]ep queue failed, status %d!\n", __func__, status);
			list_add(&req->list, pool);
			port->read_started--;
			port->stat_rx_submit_fail++;
			return -1;
		}

		rw_priv = req->context;
		list_add_tail(&rw_priv->list, &port->read_queue_in_usb);

		port->read_req_enqueued++;
		port->stat_rx_submit++;

		/* abort immediately after disconnect */
		if (!port->port_usb) {
			pr_err("[%s]no usb port\n", __func__);
			port->stat_rx_disconnect++;
			return -1;
		}
	}
	return status;
}


/* Context: caller owns port_lock, and port_usb is set */
static int gs_acm_cdev_start_rx(struct gs_acm_cdev_port *port)
{
	int status;
	struct list_head  *pool = &port->read_pool;
	struct usb_request  *req = NULL;

	acm_dbg("port_num %u\n", port->port_num);

	if (!port->is_connect)
		return -ESHUTDOWN;

	if (!port->port_usb) {
		pr_err("[%s]port_usb NULL!\n", __func__);
		return port->read_started;
	}

start_rx_beg:
	status = submit_read_req(port, pool);
	if (status < 0)
		goto start_rx_ret;

	/*
	 * if there are no read req in usb core,
	 * get the read done req and submit to usb core.
	 * this mechamism may cause loosing packets.
	 */

	if (port->port_usb != NULL && port->read_req_enqueued == 0) {
		struct list_head *queue = &port->read_done_queue;

		if (!list_empty(queue)) {
			req = list_entry(queue->prev, struct usb_request, list);
			list_move(&req->list, pool);
			port->read_started--;

			/*
			 * go to beginning of the function,
			 * re-submit the read req
			 */
			port->stat_rx_no_req++;
			pr_info("get read done req for rx, drop a packet\n");
			goto start_rx_beg;
		}
	}
start_rx_ret:
	return port->read_started;
}

/* Context: caller owns port_lock, and port_usb is set */
static void gs_acm_cdev_stop_rx(struct gs_acm_cdev_port *port)
{
	struct gs_acm_cdev_rw_priv *rw_priv = NULL;
	struct usb_ep *out = port->port_usb->out;

	acm_dbg("port_num %u\n", port->port_num);

	while (!list_empty(&port->read_queue_in_usb)) {
		rw_priv = list_first_entry(&port->read_queue_in_usb,
					struct gs_acm_cdev_rw_priv, list);
		port->stat_rx_dequeue++;
		list_del_init(&rw_priv->list);
		spin_unlock(&port->port_lock);
		usb_ep_dequeue(out, rw_priv->req);
		spin_lock(&port->port_lock);
	}
}

static int read_info_proc(struct gs_acm_cdev_port *port,
		struct usb_request  *req, ACM_WR_ASYNC_INFO *read_info)
{
	int status;
	struct sk_buff *skb = NULL;
	unsigned int actual_len;
	unsigned long flags;

	actual_len = req->actual;
	if (is_modem_port(port->port_num)) {
		skb = dev_alloc_skb(actual_len);
		if (skb == NULL) {
			status = -EAGAIN;
		} else {
			memcpy((void *)skb->data, (void *)req->buf, actual_len);
			skb_put(skb, actual_len);
			read_info->pVirAddr = (char *)skb;
			read_info->pPhyAddr = (char *)(~0);
			read_info->u32Size = actual_len;
			status = 0;
		}

		spin_lock_irqsave(&port->port_lock, flags);
		list_move(&req->list, &port->read_pool);
		port->read_started--;
		spin_unlock_irqrestore(&port->port_lock, flags);
	} else {
		read_info->pVirAddr = (char *)req->buf;
		read_info->pPhyAddr = (char *)(~0);
		read_info->u32Size = actual_len;
		status = 0;
	}

	return status;
}

static int gs_acm_cdev_get_read_buf(struct gs_acm_cdev_port *port,
				ACM_WR_ASYNC_INFO *read_info)
{
	struct list_head *queue = &port->read_done_queue;
	struct usb_request  *req = NULL;
	int status;
	unsigned long flags;

	acm_dbg("port_num %u\n", port->port_num);

	mutex_lock(&port->read_lock);

	spin_lock_irqsave(&port->port_lock, flags);
	if (!list_empty(queue)) {
		req = list_first_entry(queue, struct usb_request, list);
		list_move_tail(&req->list, &port->read_using_queue);
	}
	spin_unlock_irqrestore(&port->port_lock, flags);

	if (req == NULL) {
		read_info->pVirAddr = NULL;
		read_info->pPhyAddr = (char *)(~0);
		read_info->u32Size = 0;
		status = -EAGAIN;
		pr_err("[%s]no req\n", __func__);
		goto out;
	}

	if (req->status)
		pr_warn("%s: port %u, req status %d\n", __func__,
			port->port_num, req->status);

	status = read_info_proc(port, req, read_info);

out:
	mutex_unlock(&port->read_lock);
	return status;
}

static int gs_acm_cdev_ret_read_buf(struct gs_acm_cdev_port *port,
				ACM_WR_ASYNC_INFO *read_info)
{
	struct usb_request *cur_req = NULL;
	struct usb_request *next_req = NULL;
	unsigned long flags;

	acm_dbg("port_num %u\n", port->port_num);

	spin_lock_irqsave(&port->port_lock, flags);
	if (is_modem_port(port->port_num)) {
		dev_kfree_skb_any((struct sk_buff *)read_info->pVirAddr);
		read_info->pVirAddr = NULL;
		spin_unlock_irqrestore(&port->port_lock, flags);
		return 0;
	}

	list_for_each_entry_safe(cur_req, next_req,
		&port->read_using_queue, list) {
		if ((uintptr_t)cur_req->buf == (uintptr_t)read_info->pVirAddr) {
			list_move(&cur_req->list, &port->read_pool);
			port->read_started--;
			spin_unlock_irqrestore(&port->port_lock, flags);

			return 0;
		}
	}
	spin_unlock_irqrestore(&port->port_lock, flags);

	return -EFAULT;
}

static void gs_acm_cdev_notify_cb(struct gs_acm_cdev_port *port)
{
	ACM_EVENT_CB_T event_cb = NULL;
	u16 line_state = 0;
	unsigned long flags;
	MODEM_MSC_STRU *flow_msg = &port->flow_msg;
	ACM_MODEM_MSC_READ_CB_T read_sig_cb = NULL;

	acm_dbg("port_num %u\n", port->port_num);

	spin_lock_irqsave(&port->port_lock, flags);
	if (port->line_state_change) {
		/* event_cb is for common serial port  */
		event_cb = port->event_notify_cb;
		line_state = port->line_state_on;
		port->line_state_change = 0;

		/* read_sig_cb is for modem port */
		read_sig_cb = port->read_sig_cb;
		flow_msg->OP_Dtr = SIGNALCH;
		flow_msg->ucDtr = line_state;
	}
	spin_unlock_irqrestore(&port->port_lock, flags);

	if (event_cb) {
		if (line_state)
			port->stat_notify_on_cnt++;
		else
			port->stat_notify_off_cnt++;

		event_cb((ACM_EVT_E)line_state);
	}

	if (read_sig_cb) {
		if (line_state)
			port->stat_notify_on_cnt++;
		else
			port->stat_notify_off_cnt++;

		pr_info("acm[%s] read_sig_cb OP_Dtr=%u ucDtr=%u\n",
			get_acm_cdev_name(port->port_num),
			flow_msg->OP_Dtr,
			flow_msg->ucDtr);
		read_sig_cb(flow_msg);
		flow_msg->OP_Dtr = SIGNALNOCH;
	}
}

static void gs_acm_cdev_read_cb(struct gs_acm_cdev_port *port)
{
	struct list_head *queue = &port->read_done_queue;
	struct usb_request  *req = NULL;
	ACM_READ_DONE_CB_T read_cb = NULL;
	unsigned long flags;
	int status = 0;

	acm_dbg("port_num %u\n", port->port_num);

	for (;;) {
		spin_lock_irqsave(&port->port_lock, flags);
		if (port->read_completed <= 0) {
			spin_unlock_irqrestore(&port->port_lock, flags);
			break;
		}
		port->read_completed--;

		read_cb = NULL;
		if (!list_empty(queue)) {
			req = list_first_entry(queue, struct usb_request, list);
			status = req->status;

			/*
			 * if there are data in queue,
			 * prepare the read callback
			 */
			if (!req->status && port->open_count) {
				read_cb = port->read_done_cb;
			} else {
				list_move(&req->list, &port->read_pool);
				port->read_started--;
			}
		}

		/* submit the next read req */
		if (status != -ESHUTDOWN && port->port_usb != NULL)
			gs_acm_cdev_start_rx(port);
		else
			port->stat_rx_cb_not_start++;

		spin_unlock_irqrestore(&port->port_lock, flags);

		if (read_cb != NULL) {
			port->stat_rx_callback++;
			read_cb();
		}
	}
}

static int prepare_write_callback(struct gs_acm_cdev_port *port,
			struct sk_buff **send_skb, char **buf, char **phy)
{
	struct usb_request  *req = NULL;
	struct gs_acm_cdev_rw_priv *write_priv = NULL;
	int actual_size;

	req = list_first_entry(&port->write_queue, struct usb_request, list);
	/* if there is data in queue, prepare the write callback */
	*buf = req->buf;
	*phy = (char *)(uintptr_t)req->dma;
	req->dma = DMA_ADDR_INVALID;
	actual_size = (!req->status) ? (int)req->actual : (int)req->status;

	write_priv = (struct gs_acm_cdev_rw_priv *)req->context;

	if (write_priv != NULL)
		*send_skb = write_priv->user_p_skb;

	list_move(&req->list, &port->write_pool);

	port->write_started--;
	port->write_completed--;

	return actual_size;
}


static void gs_acm_cdev_write_cb(struct gs_acm_cdev_port *port)
{
	struct list_head *queue = &port->write_queue;
	char *buf = NULL;
	char *phy = (char *)(uintptr_t)(~0);
	int actual_size;
	ACM_WRITE_DONE_CB_T write_cb = NULL;
	ACM_FREE_CB_T free_cb = NULL;
	struct sk_buff *send_skb = NULL;
	unsigned long flags;
	unsigned int free_queue_n = 0;

	acm_dbg("port_num %u\n", port->port_num);

	for (;;) {
		spin_lock_irqsave(&port->port_lock, flags);

		if (unlikely(port->free_queue_n)) {
			buf = port->free_queue[free_queue_n];
			actual_size = 0;
			free_queue_n++;
			port->free_queue_n--;
			port->stat_tx_disconnect_free++;
		} else {
			if (port->write_completed <= 0 || list_empty(queue)) {
				spin_unlock_irqrestore(&port->port_lock, flags);
				return;
			}
			write_cb = port->write_done_cb;
			free_cb = port->write_done_free_cb;

			actual_size = prepare_write_callback(port,
					&send_skb, &buf, &phy);
		}
		spin_unlock_irqrestore(&port->port_lock, flags);

		if (write_cb != NULL) {
			port->stat_tx_callback++;
			write_cb(buf, phy, actual_size);
		} else if (free_cb != NULL) {
			if (is_modem_port(port->port_num) && send_skb != NULL) {
				port->stat_tx_callback++;
				free_cb((char *)send_skb);
			}
		}
	}
}

/*
 * rw workqueue takes data out of the RX queue and hands it up to the TTY
 * layer until it refuses to take any more data (or is throttled back).
 * Then it issues reads for any further data.
 */
static void gs_acm_cdev_rw_push(struct work_struct *work)
{
	struct gs_acm_cdev_port *port = NULL;

	/* notify callback */
	port = gs_acm_evt_get(&gs_acm_sig_stat_evt_manage);
	while (port != NULL) {
		gs_acm_cdev_notify_cb(port);
		port = gs_acm_evt_get(&gs_acm_sig_stat_evt_manage);
	}
	/* read callback */
	port = gs_acm_evt_get(&gs_acm_read_evt_manage);
	while (port != NULL) {
		gs_acm_cdev_read_cb(port);
		port = gs_acm_evt_get(&gs_acm_read_evt_manage);
	}
	/* write callback */
	port = gs_acm_evt_get(&gs_acm_write_evt_manage);
	while (port != NULL) {
		gs_acm_cdev_write_cb(port);
		port = gs_acm_evt_get(&gs_acm_write_evt_manage);
	}
	/* other callback ... */
}

static void gs_acm_cdev_read_complete(struct usb_ep *ep,
				struct usb_request *req)
{
	struct gs_acm_cdev_port *port = ep->driver_data;
	struct gs_acm_cdev_rw_priv *rw_priv = NULL;

	acm_dbg("port_num %u, actual %u\n", port->port_num, req->actual);

	/* Queue all received data until the tty layer is ready for it. */
	spin_lock(&port->port_lock);

	if (!req->status) {
		port->stat_rx_done++;
		port->stat_rx_done_bytes += req->actual;
	} else {
		port->stat_rx_done_fail++;
	}

	rw_priv = req->context;
	list_del_init(&rw_priv->list);
	port->read_req_enqueued--;

	if (port->port_usb != NULL && !port->is_realloc && req->actual) {
		list_add_tail(&req->list, &port->read_done_queue);
		port->stat_rx_done_schdule++;
		port->read_completed++;
		gs_acm_evt_push(port, &gs_acm_read_evt_manage);
		queue_delayed_work(acm_work_queue, &port->rw_work, 0);
	} else {
		list_add_tail(&req->list, &port->read_pool);
		port->read_started--;
		port->stat_rx_done_disconnect++;
	}
	spin_unlock(&port->port_lock);

	/* if there is blocked read, wake up it */
	if (port->read_blocked) {
		port->read_blocked = 0;
		port->stat_rx_wakeup_block++;
		wake_up_interruptible(&port->read_wait);
	}

	/* if clean up all read reqs, wake up the realloc task */
	if (port->is_realloc && !port->read_started) {
		port->stat_rx_wakeup_realloc++;
		wake_up(&port->realloc_wait);
	}
}

static void gs_acm_cdev_write_complete(struct usb_ep *ep,
				struct usb_request *req)
{
	struct gs_acm_cdev_port *port = ep->driver_data;
	struct gs_acm_cdev_rw_priv *write_priv = NULL;
	struct timespec64 tx_end_time;
	long write_time;

	acm_dbg("port_num %u\n", port->port_num);

	spin_lock(&port->port_lock);

	write_priv = req->context;

	if (!req->status) {
		port->stat_tx_done++;
		port->stat_tx_done_bytes += req->actual;
		ktime_get_real_ts64(&tx_end_time);
		write_time = (tx_end_time.tv_sec -
			write_priv->tx_start_time.tv_sec) * 1000000000 +
			(tx_end_time.tv_nsec - write_priv->tx_start_time.tv_nsec);
		port->stat_tx_done_alltime += write_time;
		if (write_time > port->stat_tx_done_maxtime)
			port->stat_tx_done_maxtime = write_time;
	} else {
		pr_err("%s: req->status %d is fail\n", __func__, req->status);
		port->stat_tx_done_fail++;
	}

	/* sync read wake up the blocked task */
	if (write_priv->is_sync) {
		list_add_tail(&req->list, &port->write_pool);
		port->write_started--;
		port->write_block_status = req->status;
		port->write_blocked = 0;
		write_priv->is_sync = 0;
		port->stat_tx_wakeup_block++;
		wake_up_interruptible(&port->write_wait);
	} else {
	/* async read schedule the workqueue to call the callback */
		if (port->port_usb) {
			list_add_tail(&req->list, &port->write_queue);
			port->stat_tx_done_schdule++;
			port->write_completed++;
			gs_acm_evt_push(port, &gs_acm_write_evt_manage);

			queue_delayed_work(acm_work_queue,
				&port->rw_work, 0);
		} else {
			list_add_tail(&req->list, &port->write_pool);
			port->write_started--;
			port->stat_tx_done_disconnect++;
		}
	}
	spin_unlock(&port->port_lock);
}

static void gs_acm_cdev_free_request(struct usb_ep *ep,
				struct usb_request  *req)
{
	struct gs_acm_cdev_rw_priv *write_priv = NULL;

	write_priv = req->context;

	if (write_priv == NULL) {
		pr_err("%s: write_priv NULL\n", __func__);
		return;
	}
	/*
	 * if copy_data flag is ture,
	 * the data buffer is belong to usr, don't free it
	 */
	if (!write_priv->is_copy)
		req->buf = NULL;

	kfree(req->context);
	req->context = NULL;

	gs_acm_cdev_free_req(ep, req);
}

static void gs_acm_cdev_free_requests(struct usb_ep *ep,
		struct list_head *head, int *allocated)
{
	struct usb_request  *req = NULL;

	while (!list_empty(head)) {
		req = list_entry(head->next, struct usb_request, list);
		list_del_init(&req->list);
		gs_acm_cdev_free_request(ep, req);
		if (allocated != NULL)
			(*allocated)--;
	}
}

static void gs_acm_cdev_free_queue(struct usb_ep *ep,
				struct gs_acm_cdev_port *port)
{
	struct list_head *head = &port->write_queue;
	struct usb_request  *req = NULL;
	struct gs_acm_cdev_rw_priv *write_priv = NULL;
	unsigned int i = 0;

	while (!list_empty(head)) {
		req = list_entry(head->next, struct usb_request, list);
		write_priv = (struct gs_acm_cdev_rw_priv *)req->context;
		if (!write_priv->is_copy && req->buf)
			port->free_queue[i++] = req->buf;
		list_del_init(&req->list);
		gs_acm_cdev_free_request(ep, req);
	}
	port->free_queue_n = i;
}

struct usb_request *gs_acm_cdev_alloc_req(struct usb_ep *ep,
			unsigned int len, gfp_t kmalloc_flags)
{
	struct usb_request *req = NULL;

	req = usb_ep_alloc_request(ep, kmalloc_flags);
	if (req != NULL) {
		req->length = len;
		if (len == 0) {
			req->buf = NULL;
		} else {
			req->buf = kmalloc(len, kmalloc_flags);
			if (req->buf == NULL) {
				usb_ep_free_request(ep, req);
				return NULL;
			}
		}
	}

	return req;
}
EXPORT_SYMBOL_GPL(gs_acm_cdev_alloc_req);

void gs_acm_cdev_free_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	req->buf = NULL;
	usb_ep_free_request(ep, req);
}
EXPORT_SYMBOL_GPL(gs_acm_cdev_free_req);

static struct usb_request *gs_acm_cdev_alloc_request(
			struct usb_ep *ep, unsigned int buf_size)
{
	struct gs_acm_cdev_rw_priv *rw_priv = NULL;
	struct usb_request *req = NULL;

	req = gs_acm_cdev_alloc_req(ep, buf_size, GFP_ATOMIC);
	if (req == NULL) {
		pr_err("[%s]gs_acm_cdev_alloc_req fail\n", __func__);
		return NULL;
	}

	rw_priv = (struct gs_acm_cdev_rw_priv *)
			kzalloc(sizeof(struct gs_acm_cdev_rw_priv), GFP_ATOMIC);
	if (rw_priv == NULL) {
		pr_err("[%s]kzalloc fail\n", __func__);
		gs_acm_cdev_free_req(ep, req);
		return NULL;
	}
	req->context = (void *)rw_priv;
	req->dma = DMA_ADDR_INVALID;
	rw_priv->req = req;

	if (buf_size)
		rw_priv->is_copy = 1;

	INIT_LIST_HEAD(&rw_priv->list);

	return req;
}

static int gs_acm_cdev_alloc_requests(struct usb_ep *ep,
		struct list_head *head,
		void (*fn)(struct usb_ep *, struct usb_request *),
		int *allocated, unsigned int buf_size, unsigned int buf_num)
{
	unsigned int i;
	struct usb_request *req = NULL;
	unsigned int n = allocated ? (buf_num - (unsigned int)*allocated) : buf_num;

	for (i = 0; i < n; i++) {
		req = gs_acm_cdev_alloc_request(ep, buf_size);
		if (req == NULL)
			return list_empty(head) ? -ENOMEM : 0;
		req->complete = fn;
		list_add_tail(&req->list, head);
		if (allocated != NULL)
			(*allocated)++;
	}
	return 0;
}

/* Context: holding port_lock */
static int gs_acm_cdev_prepare_io(struct gs_acm_cdev_port *port)
{
	struct list_head *head = &port->read_pool;
	struct usb_ep *ep_out = port->port_usb->out;
	struct usb_ep *ep_in = port->port_usb->in;
	int status;

	/* alloc requests for bulk out ep */
	if (port->read_buf_size > ACM_CDEV_MAX_XFER_SIZE) {
		pr_err("[%s]port %u, read_buf_size too large\n",
			__func__, port->port_num);
		port->read_buf_size = ACM_CDEV_MAX_XFER_SIZE;
	}
	status = gs_acm_cdev_alloc_requests(ep_out, head, gs_acm_cdev_read_complete,
			&port->read_allocated, port->read_buf_size,
			port->read_req_num);
	if (status) {
		pr_err("port%u alloc request for bulk out ep failed!\n",
			port->port_num);
		return status;
	}
	acm_dbg("port%u: alloc %u reqs for bulk out ep, buf_size 0x%x, allocated %d\n",
			port->port_num, port->read_req_num, port->read_buf_size,
			port->read_allocated);

	/* alloc request for bulk in ep */
	status = gs_acm_cdev_alloc_requests(ep_in,
			&port->write_pool, gs_acm_cdev_write_complete,
			&port->write_allocated, 0, port->write_req_num);
	if (status) {
		pr_err("port%u alloc request for bulk out ep failed!\n",
			port->port_num);
		gs_acm_cdev_free_requests(ep_out, head, &port->read_allocated);
		return status;
	}
	acm_dbg("port%u: alloc %u reqs for bulk in ep, buf_size 0x%x, allocated %d\n",
			port->port_num, port->write_req_num, 0,
			port->write_allocated);

	return 0;
}

static int gs_acm_cdev_write_base(struct gs_acm_cdev_port *port,
				char *buf, char *paddr,
				unsigned int len, bool is_sync)
{
	int status = 0;

	acm_dbg("+\n");

	if (len == 0) {
		pr_err("%s: zero length packet to send\n", __func__);
		return status;
	}

	if (len > ACM_CDEV_MAX_XFER_SIZE) {
		pr_err("[%s]port %u, len too large\n",
			__func__, port->port_num);
		return -EINVAL;
	}

	if (buf == NULL) {
		pr_err("[%s]port %u, buf is NULL\n", __func__, port->port_num);
		return -EINVAL;
	}

	mutex_lock(&port->write_lock);

	status = gs_acm_cdev_start_tx(port, buf, paddr, len, is_sync);
	if (status)
		goto write_mutex_exit;

	/* async write don't need to wait write complete */
	if (!is_sync)
		goto write_mutex_exit;

	status = wait_event_interruptible(port->write_wait,
			(port->write_blocked == 0));
	if (status) {
		port->stat_sync_tx_wait_fail++;
		goto write_mutex_exit;
	}

	/* check status */
	if (port->write_block_status) {
		status = port->write_block_status;
		port->write_block_status = 0;
	} else {
		status = (int)len;
	}

write_mutex_exit:
	mutex_unlock(&port->write_lock);
	acm_dbg("-\n");
	return status;
}

static struct usb_request *gs_acm_cdev_get_reading_req(
					struct gs_acm_cdev_port *port)
{
	struct usb_request *reading_req = NULL;

	if (port->reading_req == NULL) {
		if (!list_empty(&port->read_done_queue)) {
			reading_req = list_first_entry(&port->read_done_queue,
						struct usb_request, list);
			list_del_init(&reading_req->list);
		}
	} else {
		reading_req = port->reading_req;
	}

	return reading_req;
}

static void gs_acm_cdev_ret_reading_req(struct gs_acm_cdev_port *port,
				struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&port->port_lock, flags);
	list_add_tail(&req->list, &port->read_pool);
	port->read_started--;
	spin_unlock_irqrestore(&port->port_lock, flags);
}

static int gs_acm_cdev_realloc_read_buf(struct gs_acm_cdev_port *port,
				ACM_READ_BUFF_INFO *buf_info)
{
	int status;
	int ret;
	struct usb_ep *out = NULL;
	struct list_head *head = &port->read_pool;
	unsigned long flags;

	/* get the read lock to stop usr use the read interface */
	mutex_lock(&port->read_lock);

	/* 1. dequeue all read req in usb core */
	spin_lock_irqsave(&port->port_lock, flags);
	if (unlikely(port->port_usb == NULL)) {
		status = -ENODEV;
		goto realloc_exit;
	}
	port->is_realloc = 1;

	out = port->port_usb->out;
	gs_acm_cdev_stop_rx(port);
	spin_unlock_irqrestore(&port->port_lock, flags);

	/* 2. wait for all read req complete */
	(void)wait_event_timeout(port->realloc_wait,
		(!port->read_started), WAIT_TIME);

	spin_lock_irqsave(&port->port_lock, flags);
	if (unlikely(port->port_usb == NULL)) {
		status = -ENODEV;
		pr_err("%s:no usb port\n", __func__);
		goto realloc_exit;
	}

	/* make sure the read reqs have been clean up */
	/* 3. free the old req pool */
	gs_acm_cdev_free_requests(out, head, &port->read_allocated);

	/* 4. alloc the new req pool */
	port->read_req_num = buf_info->u32BuffNum;
	port->read_buf_size = buf_info->u32BuffSize;
	if (port->read_buf_size > ACM_CDEV_MAX_XFER_SIZE) {
		pr_err("[%s]port %u, read_buf_size too large\n",
			__func__, port->port_num);
		port->read_buf_size = ACM_CDEV_MAX_XFER_SIZE;
	}

	status = gs_acm_cdev_alloc_requests(out, head,
			gs_acm_cdev_read_complete, &port->read_allocated,
			port->read_buf_size, port->read_req_num);

	acm_dbg("port%u: realloc %u reqs for bulk out ep, buf_size 0x%x, allocated %d\n",
			port->port_num, port->read_req_num, port->read_buf_size,
			port->read_allocated);

	/* 5. restart the rx */
	ret = gs_acm_cdev_start_rx(port);
	if (ret < 0)
		pr_err("[%s] start rx err\n", __func__);

realloc_exit:
	port->is_realloc = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);
	mutex_unlock(&port->read_lock);

	return status;
}

static void op_cts_proc(struct gs_acm_cdev_port *port,
			MODEM_MSC_STRU *flow_msg, struct gserial *gser)
{
	if (flow_msg->ucCts == SIGNALNOCH) {
		if (gser->flow_control)
			gser->flow_control(gser, RECV_DISABLE, SEND_ENABLE);
		port->stat_send_cts_stat = RECV_DISABLE;
		port->stat_send_cts_disable++;
	} else {
		if (gser->flow_control)
			gser->flow_control(gser, RECV_ENABLE, SEND_ENABLE);
		port->stat_send_cts_stat = RECV_ENABLE;
		port->stat_send_cts_enable++;
	}
	pr_info("acm[%s] flow_control: %s\n", get_acm_cdev_name(port->port_num),
		(flow_msg->ucCts == SIGNALNOCH) ? "disable recv" : "enable recv");
}

static int gs_acm_cdev_write_cmd(struct gs_acm_cdev_port *port,
				MODEM_MSC_STRU *flow_msg)
{
	u16 capability = 0;
	struct gserial *gser = NULL;

	if (port->port_usb != NULL) {
		gser = port->port_usb;
	} else {
		pr_err("%s: write cmd when disconnected\n", __func__);
		return -ENODEV;
	}

	if (flow_msg->OP_Cts == SIGNALCH)
		op_cts_proc(port, flow_msg, gser);

	if ((flow_msg->OP_Ri == SIGNALCH) || (flow_msg->OP_Dsr == SIGNALCH)
				|| (flow_msg->OP_Dcd == SIGNALCH)) {
		if ((flow_msg->OP_Ri == SIGNALCH) && (flow_msg->ucRi == HIGHLEVEL)) {
			capability |= MODEM_CTRL_RI;
			pr_info("acm[%s] OP_Ri CHANGE capability=0x%x\n",
				get_acm_cdev_name(port->port_num), capability);
			port->stat_send_ri++;
		};

		/* DSR SIGNAL CHANGE */
		if ((flow_msg->OP_Dsr == SIGNALCH) && (flow_msg->ucDsr == HIGHLEVEL)) {
			capability |= MODEM_CTRL_DSR;
			pr_info("acm[%s] OP_Dsr CHANGE capability=0x%x\n",
				get_acm_cdev_name(port->port_num), capability);
			port->stat_send_dsr++;
		};

		/* DCD SIGNAL CHANGE */
		if ((flow_msg->OP_Dcd == SIGNALCH) && (flow_msg->ucDcd == HIGHLEVEL)) {
			capability |= MODEM_CTRL_DCD;
			pr_info("acm[%s] OP_Dcd CHANGE capability=0x%x\n",
				get_acm_cdev_name(port->port_num), capability);
			port->stat_send_dcd++;
		};

		if (gser->notify_state)
			gser->notify_state(gser, capability);

		port->stat_send_capability = capability;
	}

	return 0;
}

int gs_acm_open(unsigned int port_num)
{
	struct gs_acm_cdev_port *port = NULL;
	unsigned long flags;
	int ret = 0;

	pr_info("%s: port_num %u\n", __func__, port_num);

	if (port_num >= ACM_CDEV_COUNT) {
		pr_err("%s: invalid port_num %u\n", __func__, port_num);
		return -EINVAL;
	}

	mutex_lock(&gs_acm_cdev_ports[port_num].open_close_lock);
	port = gs_acm_cdev_ports[port_num].port;
	if (port == NULL) {
		ret = -ENODEV;
		goto enodev_exit;
	}

	spin_lock_irqsave(&port->port_lock, flags);
	if (port->open_count != 0) {
		ret = -EBUSY;
		goto ebusy_exit;
	}
	port->open_count = 1;

	/* if connected, start the I/O stream */
	if (port->port_usb != NULL) {
		struct gserial *gser = port->port_usb;

		if (gser->connect != NULL)
			gser->connect(gser);
	}

ebusy_exit:
	spin_unlock_irqrestore(&port->port_lock, flags);
enodev_exit:
	mutex_unlock(&gs_acm_cdev_ports[port_num].open_close_lock);

	return ret;
}

int gs_acm_close(unsigned int port_num)
{
	struct gs_acm_cdev_port *port = NULL;
	unsigned long flags;

	pr_info("%s: port_num %u\n", __func__, port_num);

	if (port_num >= ACM_CDEV_COUNT) {
		pr_err("%s: invalid port_num %u\n", __func__, port_num);
		return -EINVAL;
	}

	port = gs_acm_cdev_ports[port_num].port;
	if (port == NULL) {
		pr_err("%s: port %u is not allocated\n", __func__, port_num);
		return -ENODEV;
	}

	spin_lock_irqsave(&port->port_lock, flags);
	if (port->open_count != 1) {
		WARN_ON(1);
		goto exit;
	}

	port->open_count = 0;
	port->line_state_on = 0;
	port->line_state_change = 0;

	if (port->port_usb != NULL) {
		struct gserial  *gser = port->port_usb;

		if (gser->disconnect != NULL)
			gser->disconnect(gser);
	}

	wake_up_interruptible(&port->close_wait);
exit:
	spin_unlock_irqrestore(&port->port_lock, flags);

	return 0;
}

static struct usb_request *port_read_req_proc(struct gs_acm_cdev_port *port)
{
	int status;
	unsigned long flags;
	struct usb_request *reading_req = NULL;

	do {
		spin_lock_irqsave(&port->port_lock, flags);
		if (port->port_usb == NULL) {
			port->stat_sync_rx_disconnect++;
			spin_unlock_irqrestore(&port->port_lock, flags);
			return NULL;
		}

		reading_req = gs_acm_cdev_get_reading_req(port);
		if (reading_req == NULL) {
			acm_dbg("no reading_req\n");
			/* if no req, wait for reading complete */
			port->read_blocked = 1;
			spin_unlock_irqrestore(&port->port_lock, flags);
			status = wait_event_interruptible(port->read_wait,
					(port->read_blocked == 0));
			if (status) {
				pr_err("[%s]interrupted by signal, status %d\n",
					__func__, status);
				port->stat_sync_rx_wait_fail++;
				return NULL;
			}
		} else {
			spin_unlock_irqrestore(&port->port_lock, flags);
		}
	} while (reading_req == NULL);

	return reading_req;
}

static int read_buf_cpy(struct gs_acm_cdev_port *port,
		struct usb_request *reading_req, char *buf, size_t count)
{
	unsigned int left_size;
	unsigned int copy_size;
	unsigned int need_size = (unsigned int)count;
	struct gs_acm_cdev_rw_priv *read_priv = NULL;
	char *copy_addr = NULL;
	int status;

	/* prepare copy address and copy size */
	read_priv = reading_req->context;
	copy_addr = (char *)reading_req->buf + read_priv->copy_pos;
	left_size = reading_req->actual - (unsigned int)read_priv->copy_pos;
	if (left_size > need_size) {
		copy_size = need_size;
		read_priv->copy_pos += (int)copy_size;
		port->reading_req = reading_req;
	} else {
		copy_size = left_size;
		read_priv->copy_pos = 0;
		port->reading_req = NULL;
	}

	if (copy_addr != NULL && copy_size != 0)
		memcpy((void *)buf, (void *)copy_addr, copy_size);

	status = (int)copy_size;
	if (port->reading_req == NULL)
		gs_acm_cdev_ret_reading_req(port, reading_req);
	port->stat_sync_rx_done++;
	port->stat_sync_rx_done_bytes += copy_size;

	return status;
}

ssize_t gs_acm_read(unsigned int port_num, char *buf,
				size_t count, loff_t *ppos)
{
	struct gs_acm_cdev_port *port = NULL;
	struct usb_request *reading_req = NULL;
	int status = 0;

	acm_dbg("+\n");
	acm_dbg("port_num %u\n", port_num);

	if (port_num >= ACM_CDEV_COUNT)
		return -EINVAL;

	port = gs_acm_cdev_ports[port_num].port;
	if (port == NULL) {
		pr_err("[%s]port %u is not allocated\n", __func__, port_num);
		return -ENODEV;
	}

	port->stat_read_call++;
	if (unlikely(buf == NULL || count == 0)) {
		pr_err("%s: param err, buf %pK, count %lu\n",
			__func__, buf, count);
		port->stat_read_param_err++;
		return -EFAULT;
	}

	mutex_lock(&port->read_lock);
	reading_req = port_read_req_proc(port);
	if (reading_req == NULL)
		goto read_exit;

	if (reading_req->status) {
		port->stat_sync_rx_done_fail++;
		port->reading_req = NULL;
		gs_acm_cdev_ret_reading_req(port, reading_req);
		goto read_exit;
	}

	status = read_buf_cpy(port, reading_req, buf, count);

read_exit:
	mutex_unlock(&port->read_lock);

	acm_dbg("-\n");
	return (ssize_t)status;
}

ssize_t gs_acm_write(unsigned int port_num, const char *buf,
				size_t count, loff_t *ppos)
{
	struct gs_acm_cdev_port *port = NULL;
	unsigned int len = (unsigned int)count;
	int status;

	acm_dbg("+\n");
	acm_dbg("port_num %u\n", port_num);

	if (unlikely(buf == NULL || count == 0)) {
		pr_err("%s invalid param buf:%pK, count:%zu\n",
			__func__, buf, count);
		return -EFAULT;
	}

	if (port_num >= ACM_CDEV_COUNT)
		return -EINVAL;

	port = gs_acm_cdev_ports[port_num].port;
	if (port == NULL) {
		pr_err("[%s]port %u is not allocated\n", __func__, port_num);
		return -ENODEV;
	}

	port->stat_sync_tx_submit++;
	status = gs_acm_cdev_write_base(port, (char *)buf, NULL, len, true);
	if (status > 0)
		port->stat_sync_tx_done++;
	else
		port->stat_sync_tx_fail++;

	acm_dbg("-\n");
	return (ssize_t)status;
}

static struct gs_acm_cdev_port *acm_get_port(unsigned int port_num)
{
	struct gs_acm_cdev_port *port = NULL;

	if (port_num >= ACM_CDEV_COUNT) {
		pr_err("invalid port_num %u\n", port_num);
		return NULL;
	}

	port = gs_acm_cdev_ports[port_num].port;
	if (port == NULL) {
		pr_err("port %u is not allocated\n", port_num);
		return NULL;
	}
	return port;
}

static int acm_ioctl_write_async_proc(struct gs_acm_cdev_port *port,
		unsigned int port_num, unsigned long arg)
{
	int status;
	ACM_WR_ASYNC_INFO *rw_info = NULL;
	struct sk_buff *send_skb = NULL;

	if (arg == 0) {
		pr_err("%s: %u: ACM_IOCTL_WRITE_ASYNC invalid param\n",
			__func__, port_num);
		return -EFAULT;
	}

	rw_info = (ACM_WR_ASYNC_INFO *)(uintptr_t)arg;
	port->stat_write_async_call++;

	/* For modem port, user may not init rw_info->u32Size */
	if (is_modem_port(port->port_num)) {
		send_skb = (struct sk_buff *)rw_info->pVirAddr;
		rw_info->u32Size = send_skb->len;
	}

	status = gs_acm_cdev_write_base(port, rw_info->pVirAddr,
			rw_info->pPhyAddr, rw_info->u32Size, false);
	return status;
}

int gs_acm_ioctl(unsigned int port_num, unsigned int cmd, unsigned long arg)
{
	struct gs_acm_cdev_port *port = NULL;
	int status = 0;

	acm_dbg("+\n");

	port = acm_get_port(port_num);
	if (port == NULL)
		return -ENODEV;

	switch (cmd) {
	case ACM_IOCTL_SET_READ_CB:
	case UDI_IOCTL_SET_READ_CB:
		port->read_done_cb = (ACM_READ_DONE_CB_T)(uintptr_t)arg;
		break;

	case ACM_IOCTL_SET_WRITE_CB:
	case UDI_IOCTL_SET_WRITE_CB:
		port->write_done_cb = (ACM_WRITE_DONE_CB_T)(uintptr_t)arg;
		break;

	case ACM_IOCTL_SET_EVT_CB:
		port->event_notify_cb = (ACM_EVENT_CB_T)(uintptr_t)arg;
		if (port->port_num < ACM_CDEV_COUNT)
			gs_acm_cdev_ports[port->port_num].event_cb
					= (ACM_EVENT_CB_T)(uintptr_t)arg;
		break;

	case ACM_IOCTL_SET_FREE_CB:
		if (is_modem_port(port->port_num)) {
			port->write_done_free_cb
				= (ACM_FREE_CB_T)(uintptr_t)arg;
		} else {
			pr_err("ACM_IOCTL_SET_FREE_CB only for modem port\n");
			status = -EINVAL;
		}
		break;

	case ACM_IOCTL_WRITE_ASYNC:
		status = acm_ioctl_write_async_proc(port, port_num, arg);
		if (status < 0)
			return status;
		break;

	case ACM_IOCTL_GET_RD_BUFF:
		if (arg == 0) {
			pr_err("%s: %u: ACM_IOCTL_GET_RD_BUFF invalid param\n",
							__func__, port_num);
			return -EFAULT;
		}
		port->stat_get_buf_call++;
		status = gs_acm_cdev_get_read_buf(port, (ACM_WR_ASYNC_INFO *)(uintptr_t)arg);
		break;

	case ACM_IOCTL_RETURN_BUFF:
		if (arg == 0) {
			pr_err("%s: %u: ACM_IOCTL_RETURN_BUFF invalid param\n",
							__func__, port_num);
			return -EFAULT;
		}
		port->stat_ret_buf_call++;
		status = gs_acm_cdev_ret_read_buf(port,
				(ACM_WR_ASYNC_INFO *)(uintptr_t)arg);
		break;

	case ACM_IOCTL_RELLOC_READ_BUFF:
		if (arg == 0) {
			pr_err("%s: %u: ACM_IOCTL_RELLOC_READ_BUFF invalid param\n",
							__func__, port_num);
			return -EFAULT;
		}
		status = gs_acm_cdev_realloc_read_buf(port,
				(ACM_READ_BUFF_INFO *)(uintptr_t)arg);
		break;

	case ACM_IOCTL_GET_WRITE_REQ_NUM:
		if (arg == 0) {
			pr_err("%s: %u: ACM_IOCTL_GET_WRITE_REQ_NUM invalid param\n",
							__func__, port_num);
			return -EINVAL;
		}
		unsigned int *pval = (unsigned int *)arg;
		*pval = port->write_req_num;
		break;

	case ACM_IOCTL_WRITE_DO_COPY:
		port->is_do_copy = (bool)arg;
		break;
	case ACM_MODEM_IOCTL_SET_MSC_READ_CB:
		if (is_modem_port(port->port_num))
			port->read_sig_cb =
				(ACM_MODEM_MSC_READ_CB_T)(uintptr_t)arg;
		break;
	case ACM_MODEM_IOCTL_MSC_WRITE_CMD:
		if (is_modem_port(port->port_num))
			gs_acm_cdev_write_cmd(port,
				(MODEM_MSC_STRU *)(uintptr_t)arg);
		break;
	case ACM_MODEM_IOCTL_SET_REL_IND_CB:
		if (!is_modem_port(port->port_num))
			break;
		port->rel_ind_cb = (ACM_MODEM_REL_IND_CB_T)(uintptr_t)arg;
		/*
		 * if enumeration has done andrel_ind_cb
		 * has been registered,
		 * then call rel_ind_cb for modem port
		 */
		if (port->rel_ind_cb && (port->cur_evt == ACM_EVT_DEV_READY)) {
			pr_info("acm[%s] rel_ind_cb %d\n",
				get_acm_cdev_name(port->port_num),
				ACM_EVT_DEV_READY);
			port->rel_ind_cb(ACM_EVT_DEV_READY);
		}
		break;
	default:
		pr_err("%s: unknown cmd 0x%x!\n", __func__, cmd);
		status = -1;
		break;
	}
	acm_dbg("-\n");

	return status;
}

static int gs_acm_cdev_port_alloc(unsigned int port_num,
				struct usb_cdc_line_coding *coding)
{
	struct gs_acm_cdev_port *port = NULL;
	int ret = 0;
	unsigned int free_queue_n;

	mutex_lock(&gs_acm_cdev_ports[port_num].open_close_lock);

	if (gs_acm_cdev_ports[port_num].port != NULL) {
		ret = -EBUSY;
		goto out;
	}

	port = kzalloc(sizeof(*port), GFP_KERNEL);
	if (port == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	spin_lock_init(&port->port_lock);
	init_waitqueue_head(&port->close_wait);

	INIT_DELAYED_WORK(&port->rw_work, gs_acm_cdev_rw_push);

	INIT_LIST_HEAD(&port->read_pool);
	INIT_LIST_HEAD(&port->read_done_queue);
	INIT_LIST_HEAD(&port->read_using_queue);
	INIT_LIST_HEAD(&port->read_queue_in_usb);
	INIT_LIST_HEAD(&port->write_pool);
	INIT_LIST_HEAD(&port->write_queue);
	free_queue_n = gs_acm_cdev_get_tx_buf_num(port_num);
	port->free_queue = kzalloc((free_queue_n * sizeof(void *)), GFP_KERNEL);
	if (port->free_queue == NULL) {
		kfree(port);
		ret = -ENOMEM;
		goto out;
	}

	init_waitqueue_head(&port->write_wait);
	init_waitqueue_head(&port->read_wait);
	init_waitqueue_head(&port->realloc_wait);

	mutex_init(&port->write_lock);
	mutex_init(&port->read_lock);

	port->port_num = port_num;
	port->port_line_coding = *coding;

	port->read_buf_size = ACM_CDEV_DFT_RD_BUF_SIZE;
	port->read_req_num = ACM_CDEV_DFT_RD_REQ_NUM;
	port->write_req_num = gs_acm_cdev_get_tx_buf_num(port_num);
	port->is_do_copy = 1;

	scnprintf(port->read_domain, DOMAIN_NAME_LEN, "%d_rd", port_num);
	scnprintf(port->write_domain, DOMAIN_NAME_LEN, "%d_wt", port_num);
	scnprintf(port->debug_tx_domain, DOMAIN_NAME_LEN, "dtx%d", port_num);
	scnprintf(port->debug_rx_domain, DOMAIN_NAME_LEN, "drx%d", port_num);
	scnprintf(port->debug_port_domain, DOMAIN_NAME_LEN, "dpt%d", port_num);

	/* mark the asic string for debug */
	scnprintf(gs_acm_cdev_ports[port_num].name_domain,
		DOMAIN_NAME_LEN, "acm%u", port_num);

	gs_acm_cdev_ports[port_num].port = port;

	acm_dbg("alloc port %u done!\n", port_num);

out:
	mutex_unlock(&gs_acm_cdev_ports[port_num].open_close_lock);

	return ret;
}

static int gs_acm_cdev_closed(struct gs_acm_cdev_port *port)
{
	int cond;
	unsigned long flags;

	spin_lock_irqsave(&port->port_lock, flags);
	cond = (port->open_count == 0);
	spin_unlock_irqrestore(&port->port_lock, flags);

	return cond;
}

static void gs_acm_cdev_port_free(unsigned int port_num)
{
	struct gs_acm_cdev_port *port = NULL;
	int ret;

	acm_dbg("+\n");

	if (port_num >= ACM_CDEV_COUNT)
		return;

	/* prevent new opens */
	mutex_lock(&gs_acm_cdev_ports[port_num].open_close_lock);

	port = gs_acm_cdev_ports[port_num].port;

	/* wait for old opens to finish */
	ret = wait_event_interruptible(port->close_wait,
			gs_acm_cdev_closed(port));
	if (ret)
		pr_err("[%s]wait for port closed failed\n", __func__);

	gs_acm_cdev_ports[port_num].port = NULL;

	mutex_unlock(&gs_acm_cdev_ports[port_num].open_close_lock);

	WARN_ON(port->port_usb != NULL);

	mutex_destroy(&port->write_lock);
	mutex_destroy(&port->read_lock);

	kfree(port->free_queue);
	kfree(port);

	acm_dbg("-\n");
}

static void device_disable_port_process(struct gs_acm_cdev_port *port,
						MODEM_MSC_STRU *flow_msg)
{
	if (port != NULL && port->line_state_on) {
		if (port->read_sig_cb != NULL) {
			flow_msg = &port->flow_msg;
			flow_msg->OP_Dtr = SIGNALCH;
			flow_msg->ucDtr = SIGNALNOCH;
			pr_info("acm[%s] read_sig_cb\n",
				get_acm_cdev_name(port->port_num));
			port->read_sig_cb(flow_msg);
			flow_msg->OP_Dtr = SIGNALNOCH;
			}
		port->line_state_on = 0;
	}

	if (port != NULL && port->rel_ind_cb &&
				(port->cur_evt == ACM_EVT_DEV_READY)) {
		pr_info("acm[%s] rel_ind_cb %d\n",
			get_acm_cdev_name(port->port_num), ACM_EVT_DEV_SUSPEND);
		port->rel_ind_cb(ACM_EVT_DEV_SUSPEND);
		port->cur_evt = ACM_EVT_DEV_SUSPEND;
	}
}

static int gs_acm_usb_notifier_cb(struct notifier_block *nb,
				unsigned long event, void *priv)
{
	int i;
	struct gs_acm_cdev_port *port = NULL;
	MODEM_MSC_STRU *flow_msg = NULL;

	acm_dbg("+\n");
	pr_info("%s: event:%lu\n", __func__, event);

	/* event <= 0 means: USB_MODEM_DEVICE_DISABLE or USB_MODEM_DEVICE_REMOVE */
	if (event == USB_MODEM_DEVICE_DISABLE ||
			event == USB_MODEM_DEVICE_REMOVE) {
		for (i = 0; i < ACM_CDEV_COUNT; i++) {
			if (gs_acm_cdev_ports[i].event_cb)
				gs_acm_cdev_ports[i].event_cb(ACM_EVT_DEV_SUSPEND);
			port = gs_acm_cdev_ports[i].port;
			device_disable_port_process(port, flow_msg);
		}
	}

	acm_dbg("-\n");
	return 0;
}

/* called by free instance */
void gs_acm_cdev_free_line(unsigned char port_num)
{
	pr_info("%s: +\n", __func__);
	gs_acm_cdev_port_free(port_num);
	pr_info("%s: -\n", __func__);
}

/* called by alloc instance */
int gs_acm_cdev_alloc_line(unsigned char *line_num)
{
	struct usb_cdc_line_coding  coding;
	unsigned int port_num;
	int ret;

	if (line_num == NULL || *line_num >= ACM_CDEV_COUNT)
		return -EFAULT;
	pr_info("%s: +\n", __func__);
	coding.dwDTERate = cpu_to_le32(DTERATE);
	coding.bCharFormat = CHARFORMAT;
	coding.bParityType = USB_CDC_NO_PARITY;
	coding.bDataBits = USB_CDC_1_STOP_BITS;

	/* alloc and init each port */
	for (port_num = 0; port_num < ACM_CDEV_COUNT; port_num++) {
		ret = gs_acm_cdev_port_alloc(port_num, &coding);
		if (ret == -EBUSY)
			continue;
		if (ret)
			return ret;
		break;
	}
	if (ret || port_num >= ACM_CDEV_COUNT)
		return ret;

	acm_dbg("got a free port, port_num: %u\n", port_num);

	*line_num = (unsigned char)port_num;

	pr_info("%s: -\n", __func__);

	return ret;
}

static int modem_acm_init(void)
{
	unsigned int i;

	acm_dbg("+\n");

	gs_acm_evt_init(&gs_acm_write_evt_manage, "write_evt");
	gs_acm_evt_init(&gs_acm_read_evt_manage, "read_evt");
	gs_acm_evt_init(&gs_acm_sig_stat_evt_manage, "sig_stat_evt");

	gs_acm_cdev_n_ports = ACM_CDEV_COUNT;

	for (i = 0; i < ACM_CDEV_COUNT; i++)
		mutex_init(&gs_acm_cdev_ports[i].open_close_lock);

	acm_work_queue = create_singlethread_workqueue("acm_cdev");
	if (acm_work_queue == NULL) {
		pr_err("[%s]no memory\n", __func__);
		return -ENOMEM;
	}

	/* we just regist once, and don't unregist any more */
	if (gs_acm_nb_ptr == NULL) {
		gs_acm_nb_ptr = &gs_acm_nb;
		gs_acm_nb.priority = USB_NOTIF_PRIO_FD;
		gs_acm_nb.notifier_call = gs_acm_usb_notifier_cb;
		bsp_usb_register_notify(gs_acm_nb_ptr);
	}

	modem_acm_debugfs_init();

	acm_dbg("-\n");

	return 0;
}
module_init(modem_acm_init);

static void modem_acm_exit(void)
{
	acm_dbg("+\n");
	modem_acm_debugfs_exit();

	gs_acm_cdev_n_ports = 0;

	if (acm_work_queue != NULL) {
		destroy_workqueue(acm_work_queue);
		acm_work_queue = NULL;
	}
	acm_dbg("-\n");
}
module_exit(modem_acm_exit);

/* called by acm_setup f_modem_acm.c */
int gacm_cdev_line_state(struct gserial *gser, u8 port_num, u32 state)
{
	struct gs_acm_cdev_port *port = NULL;
	unsigned long flags;
	u16 line_state;

	acm_dbg("+\n");

	if (port_num >= gs_acm_cdev_n_ports) {
		pr_emerg("port_num:%u, n_ports:%u\n",
			port_num, gs_acm_cdev_n_ports);
		return -ENXIO;
	}

	port = gs_acm_cdev_ports[port_num].port;
	acm_dbg("port_num %u, state 0x%x\n", port->port_num, state);

	spin_lock_irqsave(&port->port_lock, flags);
	line_state = port->line_state_on;

	/* if line state is change notify the callback */
	if (line_state != (u16)(state & U_ACM_CTRL_DTR)) {
		port->line_state_on = (u16)(state & U_ACM_CTRL_DTR);
		port->line_state_change = 1;

		if (port->line_state_on) {
			port->stat_tx_done_maxtime = 0;
			port->stat_tx_done_alltime = 0;
		}

		pr_info("acm[%u] old_DTR_value = %u, ucDtr = %u\n",
			port_num, line_state, port->line_state_on);
	}
	spin_unlock_irqrestore(&port->port_lock, flags);

	/*
	 * host may change the state in a short time,
	 * delay it, use the last state
	 */
	if (port->line_state_change) {
		gs_acm_evt_push(port, &gs_acm_sig_stat_evt_manage);
		queue_delayed_work(acm_work_queue,
			&port->rw_work, DELAYTIME); /* gs_acm_cdev_rw_push */
		port->stat_notify_sched++;
	}

	acm_dbg("-\n");

	return 0;
}

static void notify_serial_state(struct gserial *gser,
				struct gs_acm_cdev_port *port)
{
	if (port->open_count) {
		if (gser->connect != NULL)
			gser->connect(gser);
	} else {
		if (gser->disconnect != NULL)
			gser->disconnect(gser);
	}
}

/*
 * gacm_cdev_connect - notify TTY I/O glue that USB link is active
 * @gser: the function, set up with endpoints and descriptors
 * @port_num: which port is active
 * Context: any (usually from irq)
 *
 * This is called activate endpoints and let the TTY layer know that
 * the connection is active ... not unlike "carrier detect".  It won't
 * necessarily start I/O queues; unless the TTY is held open by any
 * task, there would be no point.  However, the endpoints will be
 * activated so the USB host can perform I/O, subject to basic USB
 * hardware flow control.
 *
 * Caller needs to have set up the endpoints and USB function in @dev
 * before calling this, as well as the appropriate (speed-specific)
 * endpoint descriptors, and also have set up the TTY driver by calling
 * @gserial_setup().
 *
 * Returns negative errno or zero.
 * On success, ep->driver_data will be overwritten.
 */
int gacm_cdev_connect(struct gserial *gser, u8 port_num)
{
	struct gs_acm_cdev_port *port = NULL;
	unsigned long flags;
	int status;

	acm_dbg("+\n");
	pr_info("[%s] port_num: %u\n", __func__, port_num);

	if (port_num >= gs_acm_cdev_n_ports) {
		pr_emerg("port_num:%u, n_ports:%u\n", port_num, gs_acm_cdev_n_ports);
		return -ENXIO;
	}
	if (gser == NULL)
		return -ENXIO;

	/* we "know" gserial_cleanup() hasn't been called */
	port = gs_acm_cdev_ports[port_num].port;

	/* activate the endpoints */
	pr_info("[%s] enable ep in %pK\n", __func__, gser->in);
	status = usb_ep_enable(gser->in);
	if (status < 0) {
		port->stat_enable_in_fail++;
		pr_err("[%s] in fail, status %d\n", __func__, status);
		return status;
	}

	pr_info("[%s] enable ep out %pK\n", __func__, gser->out);
	status = usb_ep_enable(gser->out);
	if (status < 0) {
		port->stat_enable_out_fail++;
		pr_err("[%s] out fail, status %d\n", __func__, status);
		goto fail_out;
	}

	/* then tell the tty glue that I/O can work */
	spin_lock_irqsave(&port->port_lock, flags);

	gser->in->driver_data = port;
	gser->out->driver_data = port;

	gser->ioport = (void *)port;
	port->port_usb = gser;
	port->cur_evt = ACM_EVT_DEV_READY;
	/*
	 * REVISIT unclear how best to handle this state...
	 * we don't really couple it with the Linux TTY.
	 */
	gser->port_line_coding = port->port_line_coding;
	port->is_connect = 1;
	/* prepare requests */
	gs_acm_cdev_prepare_io(port);

	/*
	 * if it's already open, start I/O ... and notify the serial
	 * protocol about open/close status (connect/disconnect).
	 * don't need to notify host now ...
	 */
	notify_serial_state(gser, port);

	/* start read requests */
	acm_dbg("port %u strat rx\n", port_num);
	status = gs_acm_cdev_start_rx(port);

	spin_unlock_irqrestore(&port->port_lock, flags);

	port->in_name = (char *)gser->in->name;
	port->out_name = (char *)gser->out->name;
	port->stat_port_connect++;
	port->stat_port_is_connect = 1;

	acm_dbg("-\n");
	return status;

fail_out:
	usb_ep_disable(gser->in);
	gser->in->driver_data = NULL;
	port->stat_port_is_connect = 0;
	return status;
}

/*
 * gacm_cdev_disconnect - notify TTY I/O glue that USB link is inactive
 * @gser: the function, on which gserial_connect() was called
 * Context: any (usually from irq)
 *
 * This is called to deactivate endpoints and let the TTY layer know
 * that the connection went inactive ... not unlike "hangup".
 *
 * On return, the state is as if gserial_connect() had never been called;
 * there is no active USB I/O on these endpoints.
 */
void gacm_cdev_disconnect(struct gserial *gser)
{
	struct gs_acm_cdev_port *port = NULL;
	unsigned long flags;

	acm_dbg("+\n");

	if (gser == NULL)
		return;
	port = gser->ioport;
	if (port != NULL) {
		pr_info("[%s]: port_num %u\n", __func__, port->port_num);
		spin_lock_irqsave(&port->port_lock, flags);
		port->is_connect = 0;
		spin_unlock_irqrestore(&port->port_lock, flags);
	}
	/* disable endpoints, aborting down any active I/O */
	pr_info("[%s] disable ep out %pK\n", __func__, gser->out);
	usb_ep_disable(gser->out);

	pr_info("[%s] disable ep in %pK\n", __func__, gser->in);
	usb_ep_disable(gser->in);

	if (port == NULL) {
		WARN_ON(1);
		return;
	}

	gs_acm_cdev_write_cb(port);

	spin_lock_irqsave(&port->port_lock, flags);

	port->port_usb = NULL;
	gser->ioport = NULL;
	gser->out->driver_data = NULL;
	gser->in->driver_data = NULL;

	gs_acm_cdev_free_requests(gser->out, &port->read_pool, NULL);
	gs_acm_cdev_free_requests(gser->out, &port->read_done_queue, NULL);
	gs_acm_cdev_free_requests(gser->out, &port->read_using_queue, NULL);
	gs_acm_cdev_free_requests(gser->in, &port->write_pool, NULL);
	gs_acm_cdev_free_queue(gser->in, port);

	port->read_allocated = 0;
	port->write_allocated = 0;

	spin_unlock_irqrestore(&port->port_lock, flags);

	port->stat_port_disconnect++;
	port->stat_port_is_connect = 0;

	if (port->read_blocked) {
		port->read_blocked = 0;

		port->stat_rx_wakeup_block++;

		wake_up_interruptible(&port->read_wait);
	}
	acm_dbg("-\n");
}

/* called by f_modem_acm.c acm_resume */
void gacm_cdev_resume_notify(struct gserial *gser)
{
	struct gs_acm_cdev_port *port = NULL;

	if (gser == NULL)
		return;
	port = gser->ioport;
	if (port == NULL) {
		pr_err("acm cdev port is null\n");
		return;
	}
	pr_info("acm[%s] resume\n", get_acm_cdev_name(port->port_num));

	if (port->rel_ind_cb != NULL && (port->cur_evt == ACM_EVT_DEV_SUSPEND)) {
		pr_info("acm[%s] rel_ind_cb %d\n",
			get_acm_cdev_name(port->port_num), ACM_EVT_DEV_READY);
		port->rel_ind_cb(ACM_EVT_DEV_READY);
		port->cur_evt = ACM_EVT_DEV_READY;
	}
}

/* called by f_modem_acm.c acm_suspend */
void gacm_cdev_suspend_notify(struct gserial *gser)
{
	struct gs_acm_cdev_port *port = NULL;
	MODEM_MSC_STRU *flow_msg = NULL;

	if (gser == NULL)
		return;
	port = gser->ioport;
	if (port == NULL) {
		pr_err("acm cdev port is null\n");
		return;
	}
	flow_msg = &port->flow_msg;

	pr_info("acm[%s] suspend\n", get_acm_cdev_name(port->port_num));

	if (port->line_state_on) {
		if (port->read_sig_cb != NULL) {
			flow_msg->OP_Dtr = SIGNALCH;
			flow_msg->ucDtr = SIGNALNOCH;
			pr_info("acm[%s] read_sig_cb\n",
				get_acm_cdev_name(port->port_num));
			port->read_sig_cb(flow_msg);
			flow_msg->OP_Dtr = SIGNALNOCH;
		}

		if (port->event_notify_cb) {
			pr_info("acm[%s] event_notify_cb\n\n",
				get_acm_cdev_name(port->port_num));
			port->event_notify_cb(ACM_EVT_DEV_SUSPEND);
		}
		port->line_state_on = 0;
	}

	if (port->rel_ind_cb && (port->cur_evt == ACM_EVT_DEV_READY)) {
		pr_info("acm[%s] rel_ind_cb %d \n",
			get_acm_cdev_name(port->port_num), ACM_EVT_DEV_SUSPEND);
		port->rel_ind_cb(ACM_EVT_DEV_SUSPEND);
		port->cur_evt = ACM_EVT_DEV_SUSPEND;
	}
}

static void acm_cdev_dump_ep_info(struct seq_file *s,
				struct gs_acm_cdev_port *port)
{
	char *find = NULL;
	unsigned long ep_num = 0;
	int ret;

	if (s == NULL) {
		pr_err("dump ep info, seq file is null\n");
		return;
	}

	if (port->stat_port_is_connect) {
		seq_printf(s, "in ep name:\t\t <%s>\n", port->in_name);
		find = strstr(port->in_name, "ep");
		if (find != NULL) {
			/* skip "ep" char */
			find += 2;
			ret = kstrtoul(find, 0, &ep_num);
			if (ret)
				return;
			/* in ep_num on odd position */
			seq_printf(s, "in ep num:\t\t <%ld>\n", ep_num * 2 + 1);
		}
		seq_printf(s, "out ep name:\t\t <%s>\n", port->out_name);
		find = strstr(port->out_name, "ep");
		if (find != NULL) {
			/* skip "ep" char */
			find += 2;
			ret = kstrtoul(find, 0, &ep_num);
			if (ret)
				return;
			/* out ep_num on even position */
			seq_printf(s, "out ep num:\t\t <%ld>\n", ep_num * 2);
		}
	} else {
		seq_puts(s, "the acm dev is not connect\n");
	}
}

static void print_dump_stat_devctx_info(struct seq_file *s,
			struct gs_acm_cdev_port *port,  unsigned int port_num)
{
	seq_puts(s, "=== dump stat dev ctx info ===\n");
	seq_printf(s, "port_usb                  %pK\n", port->port_usb);
	seq_printf(s, "build version:            %s\n", __VERSION__);
	seq_printf(s, "dev name                  %s\n", get_acm_cdev_name(port_num));
	seq_printf(s, "open_count                %u\n", port->open_count);
	seq_printf(s, "is_do_copy                %d\n", port->is_do_copy);
	seq_printf(s, "port_num                  %u\n", port->port_num);
	seq_printf(s, "line_state_on             %u\n", port->line_state_on);
	seq_printf(s, "line_state_change         %u\n", port->line_state_change);
}

static void print_dump_stat_read_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump stat read info ===\n");
	seq_printf(s, "read_started              %d\n", port->read_started);
	seq_printf(s, "read_allocated            %d\n", port->read_allocated);
	seq_printf(s, "read_req_enqueued         %d\n", port->read_req_enqueued);
	seq_printf(s, "read_req_num              %u\n", port->read_req_num);
	seq_printf(s, "read_buf_size             %u\n", port->read_buf_size);
	seq_printf(s, "read_completed            %d\n", port->read_completed);
	seq_printf(s, "reading_pos               %u\n", port->reading_pos);
}

static void print_dump_rx_status_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump rx status info ===\n");
	seq_printf(s, "stat_read_call            %u\n", port->stat_read_call);
	seq_printf(s, "stat_get_buf_call         %u\n", port->stat_get_buf_call);
	seq_printf(s, "stat_ret_buf_call         %u\n", port->stat_ret_buf_call);
	seq_printf(s, "stat_read_param_err       %u\n", port->stat_read_param_err);
	seq_printf(s, "read_blocked              %d\n", port->read_blocked);
	seq_printf(s, "stat_sync_rx_submit       %u\n", port->stat_sync_rx_submit);
	seq_printf(s, "stat_sync_rx_done         %u\n", port->stat_sync_rx_done);
	seq_printf(s, "stat_sync_rx_done_fail    %u\n", port->stat_sync_rx_done_fail);
	seq_printf(s, "stat_sync_rx_done_bytes   %u\n", port->stat_sync_rx_done_bytes);
	seq_printf(s, "stat_sync_rx_copy_fail    %u\n", port->stat_sync_rx_copy_fail);
	seq_printf(s, "stat_sync_rx_disconnect   %u\n", port->stat_sync_rx_disconnect);
	seq_printf(s, "stat_sync_rx_wait_fail    %u\n", port->stat_sync_rx_wait_fail);
	seq_printf(s, "stat_rx_submit            %u\n", port->stat_rx_submit);
	seq_printf(s, "stat_rx_submit_fail       %u\n", port->stat_rx_submit_fail);
	seq_printf(s, "stat_rx_disconnect        %u\n", port->stat_rx_disconnect);
	seq_printf(s, "stat_rx_no_req            %u\n", port->stat_rx_no_req);
	seq_printf(s, "stat_rx_done              %u\n", port->stat_rx_done);
	seq_printf(s, "stat_rx_done_fail         %u\n", port->stat_rx_done_fail);
	seq_printf(s, "stat_rx_done_bytes        %u\n", port->stat_rx_done_bytes);
	seq_printf(s, "stat_rx_done_disconnect   %u\n", port->stat_rx_done_disconnect);
	seq_printf(s, "stat_rx_done_schdule      %u\n", port->stat_rx_done_schdule);
	seq_printf(s, "stat_rx_wakeup_block      %u\n", port->stat_rx_wakeup_block);
	seq_printf(s, "stat_rx_wakeup_realloc    %u\n", port->stat_rx_wakeup_realloc);
	seq_printf(s, "stat_rx_callback          %u\n", port->stat_rx_callback);
	seq_printf(s, "stat_rx_cb_not_start      %u\n", port->stat_rx_cb_not_start);
	seq_printf(s, "stat_rx_dequeue           %u\n", port->stat_rx_dequeue);
}

static void print_dump_stat_write_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump stat write info ===\n");
	seq_printf(s, "write_req_num             %u\n", port->write_req_num);
	seq_printf(s, "write_started             %d\n", port->write_started);
	seq_printf(s, "write_completed           %d\n", port->write_completed);
	seq_printf(s, "write_allocated           %d\n", port->write_allocated);
	seq_printf(s, "write_blocked             %d\n", port->write_blocked);
	seq_printf(s, "write_block_status        %d\n", port->write_block_status);
}

static void print_dump_tx_status_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump tx status info ===\n");
	seq_printf(s, "stat_write_async_call     %u\n", port->stat_write_async_call);
	seq_printf(s, "stat_write_param_err      %u\n", port->stat_write_param_err);
	seq_printf(s, "stat_sync_tx_submit       %u\n", port->stat_sync_tx_submit);
	seq_printf(s, "stat_sync_tx_done         %u\n", port->stat_sync_tx_done);
	seq_printf(s, "stat_sync_tx_fail         %u\n", port->stat_sync_tx_fail);
	seq_printf(s, "stat_sync_tx_wait_fail    %u\n", port->stat_sync_tx_wait_fail);
	seq_printf(s, "stat_tx_submit            %u\n", port->stat_tx_submit);
	seq_printf(s, "stat_tx_submit_fail       %u\n", port->stat_tx_submit_fail);
	seq_printf(s, "stat_tx_submit_bytes      %u\n", port->stat_tx_submit_bytes);
	seq_printf(s, "stat_tx_done              %u\n", port->stat_tx_done);
	seq_printf(s, "stat_tx_done_fail         %u\n", port->stat_tx_done_fail);
	seq_printf(s, "stat_tx_done_bytes        %u\n", port->stat_tx_done_bytes);
	seq_printf(s, "stat_tx_done_schdule      %u\n", port->stat_tx_done_schdule);
	seq_printf(s, "stat_tx_done_disconnect   %u\n", port->stat_tx_done_disconnect);
	seq_printf(s, "stat_tx_wakeup_block      %u\n", port->stat_tx_wakeup_block);
	seq_printf(s, "stat_tx_callback          %u\n", port->stat_tx_callback);
	seq_printf(s, "stat_tx_no_req            %u\n", port->stat_tx_no_req);
	seq_printf(s, "stat_tx_copy_fail         %u\n", port->stat_tx_copy_fail);
	seq_printf(s, "stat_tx_alloc_fail        %u\n", port->stat_tx_alloc_fail);
	seq_printf(s, "stat_tx_disconnect        %u\n", port->stat_tx_disconnect);
	seq_printf(s, "stat_tx_disconnect_free   %u\n", port->stat_tx_disconnect_free);
	seq_printf(s, "stat_tx_done_maxtime      %ld\n", port->stat_tx_done_maxtime);
	if (port->stat_tx_done)
		seq_printf(s, "stat_tx_done_avgtime      %ld\n",
			port->stat_tx_done_alltime / (long)port->stat_tx_done);
	seq_printf(s, "stat_tx_done_alltime      %ld\n", port->stat_tx_done_alltime);
}

static void print_dump_port_status_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump port status info ===\n");
	seq_printf(s, "stat_port_connect         %u\n", port->stat_port_connect);
	seq_printf(s, "stat_port_disconnect      %u\n", port->stat_port_disconnect);
	seq_printf(s, "stat_enable_in_fail       %u\n", port->stat_enable_in_fail);
	seq_printf(s, "stat_enable_out_fail      %u\n", port->stat_enable_out_fail);
	seq_printf(s, "stat_notify_sched         %u\n", port->stat_notify_sched);
	seq_printf(s, "stat_notify_on_cnt        %u\n", port->stat_notify_on_cnt);
	seq_printf(s, "stat_notify_off_cnt       %u\n", port->stat_notify_off_cnt);
	seq_printf(s, "cur_evt                   %u\n", port->cur_evt);
}

static void print_dump_call_back_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump call back info ===\n");
	seq_printf(s, "read_done_cb              %pK\n", port->read_done_cb);
	seq_printf(s, "write_done_cb             %pK\n", port->write_done_cb);
	seq_printf(s, "write_done_free_cb        %pK\n", port->write_done_free_cb);
	seq_printf(s, "event_notify_cb           %pK\n", port->event_notify_cb);
	seq_printf(s, "read_sig_cb               %pK\n", port->read_sig_cb);
	seq_printf(s, "rel_ind_cb                %pK\n", port->rel_ind_cb);
}

static void print_dump_send_signal_info(struct seq_file *s,
					struct gs_acm_cdev_port *port)
{
	seq_puts(s, "\n=== dump send signal info ===\n");
	seq_printf(s, "stat_send_cts_stat        %u\n", port->stat_send_cts_stat);
	seq_printf(s, "stat_send_cts_enable      %u\n", port->stat_send_cts_enable);
	seq_printf(s, "stat_send_cts_disable     %u\n", port->stat_send_cts_disable);
	seq_printf(s, "stat_send_ri              %u\n", port->stat_send_ri);
	seq_printf(s, "stat_send_dsr             %u\n", port->stat_send_dsr);
	seq_printf(s, "stat_send_dcd             %u\n", port->stat_send_dcd);
	seq_printf(s, "stat_send_capability    0x%x\n", port->stat_send_capability);
}

void hiusb_do_acm_cdev_dump(struct seq_file *s, unsigned int port_num)
{
	struct gs_acm_cdev_port *port = NULL;

	if (s == NULL) {
		pr_err("%s err, seq file is null\n", __func__);
		return;
	}

	if (port_num >= gs_acm_cdev_n_ports) {
		seq_printf(s, "port_num:%u, n_ports:%u\n",
			port_num, gs_acm_cdev_n_ports);
		return;
	}

	port = gs_acm_cdev_ports[port_num].port;

	print_dump_stat_devctx_info(s, port, port_num);
	acm_cdev_dump_ep_info(s, port);

	mdelay(10);
	print_dump_stat_read_info(s, port);
	print_dump_rx_status_info(s, port);

	mdelay(10);
	print_dump_stat_write_info(s, port);
	print_dump_tx_status_info(s, port);

	mdelay(10);
	print_dump_port_status_info(s, port);

	mdelay(10);
	print_dump_call_back_info(s, port);

	if (is_modem_port(port_num))
		print_dump_send_signal_info(s, port);
}
