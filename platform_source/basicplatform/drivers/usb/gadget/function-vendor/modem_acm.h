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

#ifndef __MODEM_ACM_H
#define __MODEM_ACM_H

#include "usb_vendor.h"
#include <platform_include/basicplatform/linux/usb/drv_acm.h>
#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>
#include <platform_include/basicplatform/linux/usb/hw_usb_pnp21.h>

#define U_ACM_CTRL_DTR  (1 << 0)
#define U_ACM_CTRL_RTS  (1 << 1)
#define U_ACM_CTRL_RING (1 << 3)
#define DOMAIN_NAME_LEN		8

#ifdef DEBUG
#define acm_dbg(format, arg...) pr_info("[%s]" format, __func__, ##arg)
#else
#define acm_dbg(format, arg...) do {} while (0)
#endif

/*
 * The port structure holds info for each port, one for each minor number
 * (and thus for each /dev/ node).
 */
struct gs_acm_cdev_port {
	spinlock_t port_lock;               /* guard port_* access */

	struct gserial *port_usb;

	unsigned int open_count;
	unsigned int is_connect;
	u8 port_num;
	wait_queue_head_t close_wait;       /* wait for last close */
	ACM_EVENT_CB_T event_notify_cb;
	u16 cur_evt;
	u16 line_state_on;
	u16 line_state_change;
	char *in_name;
	char *out_name;

	MODEM_MSC_STRU flow_msg;
	ACM_MODEM_MSC_READ_CB_T read_sig_cb;
	ACM_MODEM_REL_IND_CB_T	rel_ind_cb;

	char read_domain[DOMAIN_NAME_LEN];
	struct list_head read_pool;         /* free read req list */
	struct list_head read_done_queue;        /* done read req list */
	struct list_head read_using_queue;  /* req in using list */
	struct list_head read_queue_in_usb;
	int read_started;
	int read_allocated;
	int read_req_enqueued;
	int read_completed;
	unsigned int read_req_num;
	unsigned int read_buf_size;
	unsigned int reading_pos;
	struct usb_request  *reading_req;
	int read_blocked;
	wait_queue_head_t read_wait;
	struct mutex read_lock;
	ACM_READ_DONE_CB_T read_done_cb;

	char write_domain[DOMAIN_NAME_LEN];
	struct list_head write_pool;        /* free write req list */
	struct list_head write_queue;       /* done write req list */
	/* free write req list after disconnect */
	void **free_queue;
	unsigned int free_queue_n;
	unsigned int write_req_num;
	int write_started;
	int write_allocated;
	int write_blocked;
	int write_block_status;
	int write_completed;
	bool is_do_copy;
	struct mutex write_lock;
	wait_queue_head_t write_wait;
	ACM_WRITE_DONE_CB_T write_done_cb;
	ACM_FREE_CB_T write_done_free_cb;

	struct delayed_work rw_work;
	struct usb_cdc_line_coding port_line_coding;    /* 8-N-1 etc */
	unsigned int is_realloc;
	wait_queue_head_t realloc_wait;

	char debug_tx_domain[DOMAIN_NAME_LEN];
	unsigned int stat_write_async_call;
	unsigned int stat_write_param_err;
	unsigned int stat_sync_tx_submit;
	unsigned int stat_sync_tx_done;
	unsigned int stat_sync_tx_fail;
	unsigned int stat_sync_tx_wait_fail;
	unsigned int stat_tx_submit;
	unsigned int stat_tx_submit_fail;
	unsigned int stat_tx_submit_bytes;
	unsigned int stat_tx_done;
	unsigned int stat_tx_done_fail;
	unsigned int stat_tx_done_bytes;
	unsigned int stat_tx_done_schdule;
	unsigned int stat_tx_done_disconnect;
	unsigned int stat_tx_wakeup_block;
	unsigned int stat_tx_callback;
	unsigned int stat_tx_no_req;
	unsigned int stat_tx_copy_fail;
	unsigned int stat_tx_alloc_fail;
	unsigned int stat_tx_disconnect;
	unsigned int stat_tx_disconnect_free;
	long stat_tx_done_maxtime;
	long stat_tx_done_alltime;

	char debug_rx_domain[DOMAIN_NAME_LEN];
	unsigned int stat_read_call;
	unsigned int stat_get_buf_call;
	unsigned int stat_ret_buf_call;
	unsigned int stat_read_param_err;
	unsigned int stat_sync_rx_submit;
	unsigned int stat_sync_rx_done;
	unsigned int stat_sync_rx_done_fail;
	unsigned int stat_sync_rx_done_bytes;
	unsigned int stat_sync_rx_copy_fail;
	unsigned int stat_sync_rx_disconnect;
	unsigned int stat_sync_rx_wait_fail;
	unsigned int stat_rx_submit;
	unsigned int stat_rx_submit_fail;
	unsigned int stat_rx_disconnect;
	unsigned int stat_rx_no_req;
	unsigned int stat_rx_done;
	unsigned int stat_rx_done_fail;
	unsigned int stat_rx_done_bytes;
	unsigned int stat_rx_done_disconnect;
	unsigned int stat_rx_done_schdule;
	unsigned int stat_rx_wakeup_block;
	unsigned int stat_rx_wakeup_realloc;
	unsigned int stat_rx_callback;
	unsigned int stat_rx_cb_not_start;
	unsigned int stat_rx_dequeue;

	char debug_port_domain[DOMAIN_NAME_LEN];
	unsigned int stat_port_is_connect;
	unsigned int stat_port_connect;
	unsigned int stat_port_disconnect;
	unsigned int stat_enable_in_fail;
	unsigned int stat_enable_out_fail;
	unsigned int stat_notify_sched;
	unsigned int stat_notify_on_cnt;
	unsigned int stat_notify_off_cnt;

	unsigned int stat_send_cts_stat;
	unsigned int stat_send_cts_enable;
	unsigned int stat_send_cts_disable;
	unsigned int stat_send_ri;
	unsigned int stat_send_dsr;
	unsigned int stat_send_dcd;
	unsigned int stat_send_capability;
};


struct modem_acm_instance {
	struct usb_function_instance func_inst;
	u8 port_num;
};


/*
 * One non-multiplexed "serial" I/O port ... there can be several of these
 * on any given USB peripheral device, if it provides enough endpoints.
 *
 * The "u_serial" utility component exists to do one thing:  manage TTY
 * style I/O using the USB peripheral endpoints listed here, including
 * hookups to sysfs and /dev for each logical "tty" device.
 *
 * REVISIT at least ACM could support tiocmget() if needed.
 *
 * REVISIT someday, allow multiplexing several TTYs over these endpoints.
 */
struct gserial {
	struct usb_function     func;

	void                    *ioport;

	struct usb_ep           *in;
	struct usb_ep           *out;

	/* REVISIT avoid this CDC-ACM support harder ... */
	struct usb_cdc_line_coding port_line_coding;    /* 9600-8-N-1 etc */

	/* notification callbacks */
	void (*connect)(struct gserial *p);
	void (*disconnect)(struct gserial *p);

	void (*notify_state)(struct gserial *p, u16 state);
	void (*flow_control)(struct gserial *p, u32 rx_is_on, u32 tx_is_on);

	int (*send_break)(struct gserial *p, int duration);
};

struct acm_name_type_tbl {
	char *name;
	enum USB_PID_UNIFY_IF_PROT_T type;
};

/* utilities to allocate/free request and buffer */
struct usb_request *gs_acm_cdev_alloc_req(struct usb_ep *ep,
				unsigned int len, gfp_t flags);
void gs_acm_cdev_free_req(struct usb_ep *ep, struct usb_request *req);

/* management of individual TTY ports */
int gs_acm_cdev_alloc_line(unsigned char *port_line);
void gs_acm_cdev_free_line(unsigned char port_line);

/* connect/disconnect is handled by individual functions */
int gacm_cdev_connect(struct gserial *gser, u8 port_num);
void gacm_cdev_disconnect(struct gserial *gser);

int gacm_cdev_line_state(struct gserial *gser, u8 port_num, u32 state);

void gacm_cdev_suspend_notify(struct gserial *gser);
void gacm_cdev_resume_notify(struct gserial *gser);

/* ---------------------------------------------- */
char *get_acm_cdev_name(unsigned int index);
int is_modem_port(unsigned int port_num);
unsigned char get_acm_protocol_type(unsigned int port_num);

int gs_acm_open(unsigned int port_num);
int gs_acm_close(unsigned int port_num);
ssize_t gs_acm_read(unsigned int port_num, char *buf,
				size_t count, loff_t *ppos);
ssize_t gs_acm_write(unsigned int port_num, const char *buf,
				size_t count, loff_t *ppos);
int gs_acm_ioctl(unsigned int port_num, unsigned int cmd, unsigned long arg);

/* -------------- acm support cap --------------- */
/* max dev driver support */
#define ACM_CDEV_COUNT      10

struct gs_acm_evt_manage {
	void *port_evt_array[ACM_CDEV_COUNT + 1];
	int port_evt_pos;
	const char *name;
	spinlock_t evt_lock;
};

void hiusb_do_acm_cdev_dump(struct seq_file *s, unsigned int port_num);
void modem_acm_debugfs_init(void);
void modem_acm_debugfs_exit(void);

#endif /* __MODEM_ACM_H */
