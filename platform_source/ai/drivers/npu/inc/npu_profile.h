/*
 * npu_profile.h
 *
 * about npu profile
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef _DRV_PROFILE_H_
#define _DRV_PROFILE_H_

/* profiling channel id */
#define CHANNEL_HBM  1
#define CHANNEL_BUS  2
#define CHANNEL_PCIE  3
#define CHANNEL_NIC  4
#define CHANNEL_DMA  5
#define CHANNEL_DVPP  6
#define CHANNEL_PROF  7
#define CHANNEL_TSCPU  10
#define CHANNEL_AICORE  43

#define SAMPLE_MASK    0x01
#define SAMPLE_ONLY_DATA  0x0
#define SAMPLE_WITH_HEADER  0x1

#define SAMPLE_NAME_MAX    16

/* prof_sample_register: Sampling Registration Function
 * Callback function registered by the int (*sample_fun)(void *arg, void *buf, int len, int flag):
 * Parameters of each module of the arg:
 * The buf: uses the buffer.
 * The len: uses the buffer length.
 * flag: identifier
 * flag.bit0 = 1: sends sampled "header"+ sampling data.
 * The flag.bit0 = 0: sends only collected data.
 * Length of the data written to the buffer by the return: >=0: . The <0: is abnormal.
 * The pointer returned by the arg: to the callback function
 * channel: enum prof_cmd_type type
 * The return: 0: is registered successfully, and a non-0: exception occurs.
 */
int prof_sample_register(int (*sample_fun)(void *, void *, int, int),
	void *arg, int channel);

/* prof_sample_unregister: Sampling deregistration function
 * Callback function registered by the int (*sample_fun)(void *arg, void *buf, int len):
 * channel: enum prof_cmd_type type
 * The return: 0: is deregistered successfully, and the non-0: is abnormal.
 */
int prof_sample_unregister(int (*sample_fun)(void *, void *, int, int),
	int channel);

#endif /* _DRV_PROFILE_H_ */
