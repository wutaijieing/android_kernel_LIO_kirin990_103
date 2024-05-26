

#ifndef __OAL_PM_QOS_H__
#define __OAL_PM_QOS_H__

/* 其他头文件包含 */
#include "oal_types.h"
#include "oal_mm.h"
#include "arch/oal_util.h"
#include "securec.h"
#include "platform_oneimage_define.h"

/* 宏定义 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
typedef enum {
    PM_QOS_RESERVED = 0,
    PM_QOS_CPU_DMA_LATENCY,
    PM_QOS_NETWORK_LATENCY,
    PM_QOS_NETWORK_THROUGHPUT,
    PM_QOS_MEMORY_BANDWIDTH,
#ifdef CONFIG_DEVFREQ_GOV_PM_QOS
    PM_QOS_MEMORY_LATENCY,
    PM_QOS_MEMORY_THROUGHPUT,
    PM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD,
#ifdef CONFIG_DEVFREQ_L1BUS_LATENCY
    PM_QOS_L1BUS_LATENCY,
#endif
#endif

#ifdef CONFIG_NPUFREQ_PM_QOS
    PM_QOS_NPU_FREQ_DNLIMIT,
    PM_QOS_NPU_FREQ_UPLIMIT,
#endif
#ifdef CONFIG_GPUTOP_FREQ
    PM_QOS_GPUTOP_FREQ_DNLIMIT,
    PM_QOS_GPUTOP_FREQ_UPLIMIT,
#endif
    /* insert new class ID */
    PM_QOS_NUM_CLASSES,
} oal_qos_type;

#define oal_pm_qos_add_request(req, type, val) cpu_latency_qos_add_request(req, val)
#define oal_pm_qos_remove_request(req) cpu_latency_qos_remove_request(req)
#define oal_pm_qos_update_request(req, val) cpu_latency_qos_update_request(req, val)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#define oal_pm_qos_add_request(req, type, val)
#define oal_pm_qos_remove_request(req)
#define oal_pm_qos_update_request(req, val)
#else
#define oal_pm_qos_add_request(req, type, val) pm_qos_add_request(req, type, val)
#define oal_pm_qos_remove_request(req) pm_qos_remove_request(req)
#define oal_pm_qos_update_request(req, val) pm_qos_update_request(req, val)
#endif

#endif /* end of oal_pm_qos.h */
