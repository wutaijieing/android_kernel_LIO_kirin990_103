/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the sescan.h for selinux status checking
 * Create: 2016-06-18
 */

#ifndef _SESCAN_H_
#define _SESCAN_H_

#include <asm/sections.h>
#include <crypto/hash.h>
#include <crypto/hash_info.h>
#include <linux/crypto.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/lsm_hooks.h>
#include <linux/scatterlist.h>
#include <linux/security.h>
#include <linux/string.h>
#include <linux/version.h>

#include "./include/hw_rscan_utils.h"

int get_selinux_enforcing(void);
int sescan_hookhash(uint8_t *hash, size_t hash_len);

#endif

