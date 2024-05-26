/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#ifndef __SLIMBUS_DRV_LOG_H__

#define __SLIMBUS_DRV_LOG_H__

#include <stdio.h>
#include <assert.h>

#if DEBUG
#define CFP_DBG_MSG 1
#endif

/**
 * Modules definitions
 */
#define CLIENT_MSG         0x01000000

#define DBG_GEN_MSG        0xFFFFFFFF

/**
 * Log level:
 * DBG_CRIT - critical
 * DBG_WARN - warning
 * DBG_FYI - fyi
 * DBG_HIVERB - highly verbose
 * DBG_INFLOOP - infinite loop debug
 */
#define DBG_CRIT 0
#define DBG_WARN 5
#define DBG_FYI 10
#define DBG_HIVERB 100
#define DBG_INFLOOP 200

/* module mask: */
#ifdef _HAVE_DBG_LOG_INT_
unsigned int g_dbg_enable_log  = 0;
#else
extern unsigned int g_dbg_enable_log;
#endif

/* level, counter, state: */
#ifdef _HAVE_DBG_LOG_INT_
unsigned int g_dbg_log_lvl = DBG_CRIT;
unsigned int g_dbg_log_cnt = 0;
unsigned int g_dbg_state = 0;
#else
extern unsigned int g_dbg_log_lvl;
extern unsigned int g_dbg_log_cnt;
extern unsigned int g_dbg_state;
#endif

#define c_dbg_msg( _t, _x, ...) ( ((_x)==  0) || \
								(((_t) & g_dbg_enable_log) && ((_x) <= g_dbg_log_lvl)) ? \
								printf( __VA_ARGS__): 0 )


#ifdef CFP_DBG_MSG
#define dbg_msg( t, x, ...)	c_dbg_msg( t, x, __VA_ARGS__ )
#else
#define dbg_msg( t, x, ...)
#endif
#ifdef CONFIG_AUDIO_DEBUG
#define v_dbg_msg( l, m, n, ...) dbg_msg( l, m, "[%-20.20s %4d %4d]-" n, __func__,\
									__LINE__, g_dbg_log_cnt++, __VA_ARGS__)
#define cv_dbg_msg( l, m, n, ...) c_dbg_msg( l, m, "[%-20.20s %4d %4d]-" n, __func__,\
									__LINE__, g_dbg_log_cnt++, __VA_ARGS__)
#define ev_dbg_msg( l, m, n, ...) { c_dbg_msg( l, m, "[%-20.20s %4d %4d]-" n, __func__,\
									__LINE__, g_dbg_log_cnt++, __VA_ARGS__); \
									assert(0); }
#else
#define v_dbg_msg( l, m, n, ...) dbg_msg( l, m, "[%4d %4d]-" n,\
									__LINE__, g_dbg_log_cnt++, __VA_ARGS__)
#define cv_dbg_msg( l, m, n, ...) c_dbg_msg( l, m, "[%4d %4d]-" n,\
									__LINE__, g_dbg_log_cnt++, __VA_ARGS__)
#define ev_dbg_msg( l, m, n, ...) { c_dbg_msg( l, m, "[%4d %4d]-" n,\
									__LINE__, g_dbg_log_cnt++, __VA_ARGS__); \
									assert(0); }
#endif

#define dbg_msg_set_lvl( x ) (g_dbg_log_lvl = x)
#define dbg_msg_enable_module( x ) (g_dbg_enable_log |= (x) )
#define dbg_msg_disable_module( x ) (g_dbg_enable_log &= ~( (unsigned int) (x) ))
#define dbg_msg_clear_all( _x ) ( g_dbg_enable_log = _x )

#define set_dbg_state( _x ) (g_dbg_state = _x )
#define get_dbg_state 	  (g_dbg_state)

#endif
