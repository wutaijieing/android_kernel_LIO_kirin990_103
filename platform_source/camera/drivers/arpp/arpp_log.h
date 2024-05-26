/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _ARPP_LOG_H_
#define _ARPP_LOG_H_

#define LOG_TAG         "HISI_ARPP"

#define LOG_DEBUG       0
#define LOG_INFO        1
#define LOG_WARN        1
#define LOG_ERR         1

#if LOG_DEBUG
#define hiar_logd(fmt, ...) \
	printk(KERN_INFO "[" LOG_TAG "]" "[D] %s:" fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define hiar_logd(fmt, ...) ((void)0)
#endif

#if LOG_INFO
#define hiar_logi(fmt, ...) \
	printk(KERN_INFO "[" LOG_TAG "]" "[I] %s:" fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define hiar_logi(fmt, ...) ((void)0)
#endif

#if LOG_WARN
#define hiar_logw(fmt, ...) \
	printk(KERN_WARNING "[" LOG_TAG "]" "[W] %s:" fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define hiar_logw(fmt, ...) ((void)0)
#endif

#if LOG_ERR
#define hiar_loge(fmt, ...) \
	printk(KERN_ERR "[" LOG_TAG "]" "[E] %s:" fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define hiar_loge(fmt, ...) ((void)0)
#endif

#endif /* _ARPP_LOG_H_ */
