/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: HKIP general communication and test driver
 * Date: 2020/05/04
 */

#ifndef HKIP_TEST_HARNESS_H
#define HKIP_TEST_HARNESS_H

int hkip_sysreg_set_wrapper(void *data, u64 val);

extern unsigned char hkip_text[], hkip_rodata[], hkip_data[];

extern int hkip_mem_readable(const u32 *mem);
extern int hkip_mem_writable(const u32 *mem);
extern int hkip_mem_executable(const void *mem);
extern int hkip_mem_user_readable(const u32 *mem);
extern int hkip_mem_user_writable(const u32 *mem);
extern int hkip_mem_user_executable(const void *mem);

extern u32 hkip_literal_read(u64 *);
extern u32 hkip_literal_read_user(u64 *);

typedef u32 (*literal_reader)(u64 *);

#endif
