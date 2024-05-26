
#ifndef __FACTORY_H__
#define __FACTORY_H__
#include "oal_util.h"
#include "oal_workqueue.h"

int32_t memcheck_is_working(void);
int32_t device_mem_check(unsigned long long *time);
int32_t wlan_device_mem_check(int32_t l_runing_test_mode);
int32_t wlan_device_mem_check_result(unsigned long long *time);
void wlan_device_mem_check_work(oal_work_stru *pst_worker);

#endif
