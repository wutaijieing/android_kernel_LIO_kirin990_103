

#ifndef __EXTERNAL_FEATURE_H__
#define __EXTERNAL_FEATURE_H__

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/ctype.h>
#include <linux/cpumask.h>
#endif

#if defined(_PRE_FEATURE_PLAT_LOCK_CPUFREQ) && !defined(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT)
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_ARCH_PLATFORM
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/platform_drivers/lpcpu_cpufreq_req.h>
#include <linux/platform_drivers/hisi_core_ctl.h>
#else
#include <linux/hisi/lpcpu_cpufreq_req.h>
#include <linux/core_ctl.h>
#endif
#define external_cpufreq_init_req     lpcpu_cpufreq_init_req
#define external_cpufreq_exit_req     lpcpu_cpufreq_exit_req
#define external_cpufreq_update_req   lpcpu_cpufreq_update_req
#else
#ifdef CONFIG_HISI_CPUFREQ_DT
#include <linux/hisi/hisi_cpufreq_req.h>
#include <linux/hisi/hisi_core_ctl.h>
#define external_cpufreq_init_req     hisi_cpufreq_init_req
#define external_cpufreq_exit_req     hisi_cpufreq_exit_req
#define external_cpufreq_update_req   hisi_cpufreq_update_req
#else
#include <linux/notifier.h>
struct cpufreq_req {
    struct notifier_block nb;
    int cpu;
    unsigned int freq;
};
// 该接口不存在的时候，打桩
static inline void core_ctl_check(void) {}
static inline void core_ctl_set_boost(unsigned int timeout) {}
static inline void core_ctl_spread_affinity(cpumask_t *allowed_mask) {}
static inline int hisi_cpufreq_init_req(struct cpufreq_req *req, int cpu)
{
    return 0;
}
static inline void hisi_cpufreq_update_req(struct cpufreq_req *req, unsigned int freq)
{
}
static inline void hisi_cpufreq_exit_req(struct cpufreq_req *req){}
static inline int external_cpufreq_init_req(struct cpufreq_req *req, int cpu)
{
    return 0;
}
static inline void external_cpufreq_exit_req(struct cpufreq_req *req) {}
static inline void external_cpufreq_update_req(struct cpufreq_req *req, unsigned int freq) {}
#endif
#endif
#endif
#endif

#if defined(CONFIG_ARCH_HISI)
#if defined(CONFIG_ARCH_PLATFORM)
extern void get_slow_cpus(struct cpumask *cpumask);
extern void get_fast_cpus(struct cpumask *cpumask);

#define external_get_slow_cpus  get_slow_cpus
#define external_get_fast_cpus  get_fast_cpus
#else
extern void hisi_get_slow_cpus(struct cpumask *cpumask);
extern void hisi_get_fast_cpus(struct cpumask *cpumask);

#define external_get_slow_cpus  hisi_get_slow_cpus
#define external_get_fast_cpus  hisi_get_fast_cpus
#endif /* endif for  CONFIG_ARCH_PLATFORM */
#endif /* endif for CONFIG_ARCH_HISI */

/* 函数定义 */
int32_t gps_ilde_sleep_vote(uint32_t val);
void wlan_pm_idle_sleep_vote(uint8_t uc_allow);

#endif // __EXTERNAL_FEATURE_H__
