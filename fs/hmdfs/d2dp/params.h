/* SPDX-License-Identifier: GPL-2.0 */
/*
 * D2D protocol customization parameters
 */

#ifndef D2D_PARAMS_H
#define D2D_PARAMS_H

#include <linux/time.h>
#include <linux/types.h>

/* Number of RTO retries which leads to connection drop */
#define D2D_RETRY_LIMIT_MAX 5

/* Packet format */
#define MIN_DATAGRAM_SIZE 1472U
#define MAX_DATAGRAM_SIZE 14792U
#define SACK_PAIRS_MAX    100

/* Buffer size params */
#define D2D_BUFFER_SIZE (6 * (1UL << 20))  /* 6MiB */

#define D2D_PACKET_ID_RCV_WINDOW (D2D_BUFFER_SIZE / MIN_DATAGRAM_SIZE)

/* RX buffer allowed fill percentage */
#define D2D_BUFFER_ALLOWED_FILL_PERCENTAGE 90

/* Timer periods */
#define D2D_ACK_PERIOD_MSEC   8U                   /* 5ms */
#define D2D_RTO_PERIOD_MSEC   (1U * MSEC_PER_SEC)  /* 1s  */
#define D2D_BWEST_PERIOD_MSEC 200U                 /* 200ms  */

/* Bandwidth estimator limits */
#define D2D_BWEST_LO_WATERLINE_MBPS 50  /*  50 Mbps */
#define D2D_BWEST_HI_WATERLINE_MBPS 150 /* 150 Mbps */

/* How much time we wait until protocol can be forcefully destroyed */
#define D2D_DESTROY_TIMEOUT_MSEC (10U * MSEC_PER_SEC)

/* Reserved bits in protocol header */
#define D2D_PROTOCOL_RESERVED_BITS_MASK	0xF000  /* first 4 flag bits */

#endif /* D2D_PARAMS_H */
