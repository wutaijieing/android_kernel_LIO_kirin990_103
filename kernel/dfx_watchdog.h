/*
 * huawei_watchdog.h
 *
 * implement the ioctl for user space to get memory usage information,
 * and also provider control command
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#ifdef CONFIG_HW_LOCKUP_REPORT
void dfx_watchdog_check_hung(int duration);
int dfx_watchdog_lockup_init(void);
void dfx_watchdog_lockup_init_work(void);
#else
void dfx_watchdog_check_hung(int duration)
{
	return -EINVAL;
}

int dfx_watchdog_lockup_init(void)
{
	return -EINVAL;
}

void dfx_watchdog_lockup_init_work(void)
{
	return -EINVAL;
}
#endif
