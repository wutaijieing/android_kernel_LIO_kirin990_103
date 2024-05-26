/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the rproc.h for root processes list checking
 * Create: 2016-06-18
 */

#ifndef _RPROC_H_
#define _RPROC_H_

void rprocs_strip_trustlist(char *rprocs, ssize_t rprocs_len);
int get_root_procs(char *out, size_t outlen);
bool init_rprocs_trustlist(const char *trustlist);

void rproc_init(void);
void rproc_deinit(void);

#endif

