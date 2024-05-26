/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description: plat_func.h.
 * Create: 2019/11/05
 */

#ifndef __PLAT_FUNC_H
#define __PLAT_FUNC_H

#include <platform_include/smart/linux/iomcu_log.h>
#include <platform_include/smart/linux/iomcu_boot.h>
#include <platform_include/smart/linux/itf/ddr_config.h>

struct config_on_ddr *get_config_on_ddr(void);

static inline bool is_space_ch(char ch)
{
	return (ch == ' ') || (ch == '\t');
}

static inline bool end_of_string(char ch)
{
	return (ch == '\0') || (ch == '\r') || (ch == '\n');
}

/*
 * Function    : get_sensor_mcu_mode
 * Description : return 1 contexthub boot succ, otherwise fail
 * Input       : none
 * Output      : none
 * Return      : 0 OK, other error
 */
int get_sensor_mcu_mode(void);

/*
 * Function    : get_tag_str
 * Description : get string of tag
 * Input       : [tag]
 * Output      : none
 * Return      : null is error, other ok
 */
char *get_tag_str(int tag);

/*
 * Function    : get_str_begin
 * Description : get string of begin, skip spaces and '\t'
 * Input       : [cmd_buf] command string
 * Output      : none
 * Return      : null is no valid string, other return valid string
 *               start position
 */
const char *get_str_begin(const char *cmd_buf);

/*
 * Function    : get_str_end
 * Description : get string of end, skip spaces and '\t'
 * Input       : [cmd_buf] command string
 * Output      : none
 * Return      : null is no valid string or input is null, other return
 *               valid string last position
 */
const char *get_str_end(const char *cmd_buf);

/*
 * Function    : get_arg
 * Description : get args of string, support digit arg or hex-digit arg
 * Input       : [str] args string
 *             : [arg] save int value which get from str
 * Output      : none
 * Return      : input args is not digit or hex-digit, return false;
 *               Otherwise return true.
 */
bool get_arg(const char *str, int *arg);

#endif /* __PLAT_FUNC_H */
