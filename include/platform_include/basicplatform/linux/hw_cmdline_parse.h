/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: hw_cmdline_parse
 * Create: 2021-03-29
 */
#ifndef _HW_COMDLINE_PARSE_H
#define _HW_COMDLINE_PARSE_H

/* Format of hex string: 0x12345678 */
#define HEX_STRING_MAX   (10)
#define TRANSFER_BASE    (16)

#define RUNMODE_FLAG_NORMAL  0
#define RUNMODE_FLAG_FACTORY 1

#if defined (CONFIG_CMDLINE_PARSE)
int get_logctl_value(void);
unsigned int runmode_is_factory(void);
int get_uart0_config(void);
#else
static inline int get_logctl_value(void) { return 0; }
static inline int runmode_is_factory(void) { return 0; }
static inline int get_uart0_config(void) { return 0; }
#endif

#endif
