

#ifndef __OAL_LITEOS_TIME_H__
#define __OAL_LITEOS_TIME_H__

#include <los_sys.h>
#include <linux/kernel.h>
#include <oal_types.h>
#include <linux/hrtimer.h>
#include <linux/rtc.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_CHIP_BITS_MIPS32 == _PRE_CHIP_BITS)
/* 32位寄存器最大长度 */
#define OAL_TIME_US_MAX_LEN  (0xFFFFFFFF - 1)

#elif (_PRE_CHIP_BITS_MIPS64 == _PRE_CHIP_BITS)
/* 64位寄存器最大长度 */
#define OAL_TIME_US_MAX_LEN  (0xFFFFFFFFFFFFFFFF - 1)

#endif


#define oal_time_get_stamp_ms() LOS_Tick2MS(OAL_TIME_JIFFY)


/* 获取高精度毫秒时间戳,精度1ms */
#define OAL_TIME_GET_HIGH_PRECISION_MS()  oal_get_time_stamp_from_timeval()

#define OAL_ENABLE_CYCLE_COUNT()
#define OAL_DISABLE_CYCLE_COUNT()
#define OAL_GET_CYCLE_COUNT() 0

/* 寄存器反转模块运行时间计算 */
#define oal_time_calc_runtime(_ul_start, _ul_end) \
    ((((OAL_TIME_US_MAX_LEN) / HZ) * 1000) + ((OAL_TIME_US_MAX_LEN) % HZ) * (1000 / HZ) - (_ul_start) + (_ul_end))

#define OAL_TIME_JIFFY    LOS_TickCountGet()
#define oal_read_once(_p) READ_ONCE(_p)
#define OAL_TIME_HZ       HZ
#define oal_msecs_to_jiffies(_msecs)    LOS_MS2Tick(_msecs)
#define oal_jiffies_to_msecs(_jiffies)      LOS_Tick2MS(_jiffies)

/* 获取从_ul_start到_ul_end的时间差 */
#define OAL_TIME_GET_RUNTIME(_ul_start, _ul_end) \
    (((_ul_start) > (_ul_end)) ? (oal_time_calc_runtime((_ul_start), (_ul_end))) : ((_ul_end) - (_ul_start)))

typedef struct {
    int32_t i_sec;
    int32_t i_usec;
} oal_time_us_stru;

typedef union ktime oal_time_t_stru;

#ifndef ktime_t
#define ktime_t union ktime
#endif
typedef struct timeval oal_timeval_stru;
typedef struct rtc_time oal_rtctime_stru;


OAL_STATIC OAL_INLINE void  oal_time_get_stamp_us(oal_time_us_stru *pst_usec)
{
    oal_timeval_stru tv;
    do_gettimeofday(&tv);
    pst_usec->i_sec     = tv.tv_sec;
    pst_usec->i_usec    = tv.tv_usec;
}


OAL_STATIC OAL_INLINE oal_time_t_stru oal_ktime_get(void)
{
    oal_time_t_stru time;
    time.tv64 = LOS_TickCountGet();
    return time;
}

OAL_STATIC OAL_INLINE oal_time_t_stru oal_ktime_sub(const oal_time_t_stru lhs, const oal_time_t_stru rhs)
{
    oal_time_t_stru res;
    res.tv64 = lhs.tv64 - rhs.tv64;
    return res;
}



OAL_STATIC OAL_INLINE uint64_t oal_get_time_stamp_from_timeval(void)
{
    oal_timeval_stru tv;
    do_gettimeofday(&tv);
    return ((uint64_t)tv.tv_usec / 1000 + (uint64_t)tv.tv_sec * 1000); // 乘以1000转换us单位
}

OAL_STATIC OAL_INLINE void oal_do_gettimeofday(oal_timeval_stru *tv)
{
    do_gettimeofday(tv);
}

OAL_STATIC OAL_INLINE void oal_rtc_time_to_tm(uint32_t time, oal_rtctime_stru *tm)
{
    rtc_time_to_tm(time, tm);
}


OAL_STATIC OAL_INLINE uint32_t oal_time_after(uint32_t ul_time_a, uint32_t ul_time_b)
{
    return (uint32_t)((int32_t)((ul_time_b) - (ul_time_a)) < 0);
}

OAL_STATIC OAL_INLINE uint32_t oal_time_is_before(uint32_t ui_time)
{
    return oal_time_after(OAL_TIME_JIFFY, ui_time);
}

OAL_STATIC OAL_INLINE uint32_t oal_ktime_to_us(const oal_time_t_stru kt)
{
    return(oal_jiffies_to_msecs(kt.tv64) * 1000); // 转换成ms，乘以1000
}

OAL_STATIC OAL_INLINE uint32_t oal_ktime_to_ms(const oal_time_t_stru kt)
{
    return(oal_jiffies_to_msecs(kt.tv64));
}

OAL_STATIC OAL_INLINE uint32_t oal_time_before_eq(uint32_t ul_time_a, uint32_t ul_time_b)
{
    return (uint32_t)((int32_t)((ul_time_a) - (ul_time_b)) <= 0);
}

OAL_STATIC OAL_INLINE uint32_t oal_time_before(uint32_t ul_time_a, uint32_t ul_time_b)
{
    return (uint32_t)((int32_t)((ul_time_a) - (ul_time_b)) < 0);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_time.h */
