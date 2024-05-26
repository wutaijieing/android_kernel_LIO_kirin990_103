
#ifndef __PMU_RTC_INTERFACE_H__
#define __PMU_RTC_INTERFACE_H__

#ifdef CONFIG_PMU_RTC_READCOUNT
extern unsigned long pmu_rtc_readcount(void);
#define external_pmu_rtc_readcount pmu_rtc_readcount
#elif defined(CONFIG_HISI_PMU_RTC_READCOUNT)
extern unsigned long hisi_pmu_rtc_readcount(void);
#define external_pmu_rtc_readcount hisi_pmu_rtc_readcount
#endif

#endif /* end for __PMU_RTC_INTERFACE_H__ */