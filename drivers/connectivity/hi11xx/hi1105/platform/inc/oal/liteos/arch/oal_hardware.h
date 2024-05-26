

#ifndef __OAL_LITEOS_HARDWARE_H__
#define __OAL_LITEOS_HARDWARE_H__

#include <linux/kernel.h>
#include <linux/interrupt.h>

#include "oal_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef irq_handler_t oal_irq_handler_t;
typedef struct resource oal_resource;

/* BEGIN:Added by zhouqingsong/2012/2/15 for SD5115V100 */
#define OAL_HI_TIMER_REG_BASE               0x10105000

#define OAL_HI_TIMER_NUM                    2
#define OAL_HI_TIMER_ENABLE                 1
#define OAL_HI_TIMER_DISABLE                0
#define OAL_HI_TIMER_INT_DISABLE            1
#define OAL_HI_TIMER_INT_CLEAR              0
#define OAL_HI_TIMER_DEFAULT_PERIOD         1

#define OAL_HI_TIMER_IRQ_NO                 80            /* 5113 : 5   5115:80 */

#define OAL_HI_TIMER_FREE_MODE              0         /* 1101测试新增 */
#define OAL_HI_TIMER_CYCLE_MODE             1
#define OAL_HI_TIMER_SIZE_32_BIT            1
#define OAL_HI_TIMER_WRAPPING               0
#define OAL_HI_TIMER_INT_OCCUR              1
#define OAL_HI_TIMER_INT_VALID              0x01
#define OAL_HI_TIMER_NO_DIV_FREQ            0x0

#define OAL_HI_SC_REG_BASE                  0x10100000
#define OAL_HI_SC_CTRL                      (OAL_HI_SC_REG_BASE + 0x0000)

#define OAL_IRQ_ENABLE                      1  /* 可以中断 */
#define OAL_IRQ_FORBIDDEN                   0  /* 禁止中断 */
#if (HW_LITEOS_OPEN_VERSION_NUM < KERNEL_VERSION(3,1,3))
#define IRQF_SHARED    0x00000080
#endif
#define OAL_SA_SHIRQ    IRQF_SHARED       /* 中断类型 */
typedef pm_message_t oal_pm_message_t;

typedef uint32_t        (*oal_irq_intr_func)(void *);

typedef struct {
    volatile uint32_t ul_timerx_load;
    volatile uint32_t ul_timerx_value;
    volatile uint32_t ul_timerx_control;
    volatile uint32_t ul_timerx_intclr;
    volatile uint32_t ul_timerx_ris;
    volatile uint32_t ul_timerx_mis;
    volatile uint32_t ul_timerx_bgload;
    volatile uint32_t ul_reserve;
} oal_hi_timerx_reg_stru;
/* timer控制寄存器 */
typedef union {
    volatile uint32_t ul_value;
    struct {
        volatile uint32_t ul_oneshot : 1;                 /* 选择计数模式 0：回卷计数 1：一次性计数 */
        volatile uint32_t ul_timersize : 1;               /* 16bit|32bit计数操作模式 0：16bit 1：32bit */
        volatile uint32_t ul_timerpre : 2;                /* 预分频因子 00：不分频 01：4级分频 10：8级分频 11：未定义，设置相当于分频因子10 */
        volatile uint32_t ul_reserved0 : 1;               /* 保留位 */
        volatile uint32_t ul_intenable : 1;               /* 中断屏蔽位 0：屏蔽 1：不屏蔽 */
        volatile uint32_t ul_timermode : 1;               /* 计数模式 0：自由模式 1：周期模式 */
        volatile uint32_t ul_timeren : 1;                 /* 定时器使能位 0：禁止 1：使能 */
        volatile uint32_t ul_reserved1 : 24;              /* 保留位 */
    } bits_stru;
} oal_hi_timer_control_union;

/* timer2_3寄存器 */
typedef struct {
    oal_hi_timerx_reg_stru ast_timer[2]; // timer2——3寄存器，需要2个对应成员
} oal_hi_timer_reg_stru;
typedef struct {
    oal_hi_timer_control_union  u_timerx_config;
} oal_hi_timerx_config_stru;
typedef struct {
    uint16_t  vendor;
    uint16_t  device;
    uint32_t  irq;
} oal_pci_dev_stru;

typedef struct _oal_pcie_bindcpu_cfg_ {
    uint8_t is_bind;    /* 自动绑核指令,需保证存放在最低位 */
    uint8_t irq_cpu;    /* 用户绑定硬中断命令 */
    uint8_t thread_cmd; /* 用户绑定线程命令 */
    oal_bool_enum_uint8 is_userctl; /* 是否根据用户命令绑核 */
}oal_pcie_bindcpu_cfg;

/* 中断设备结构体 */
typedef struct {
    uint32_t              irq;                  /* 中断号 */
    int32_t               l_irq_type;             /* 中断类型标志 */
    void               *p_drv_arg;              /* 中断处理函数参数 */
    int8_t               *pc_name;                /* 中断设备名字 只为界面友好 */
    oal_irq_intr_func       p_irq_intr_func;        /* 中断处理函数地址 */
} oal_irq_dev_stru;

typedef uint8_t   oal_hi_timerx_index_enum_uint8;
OAL_STATIC OAL_INLINE void  oal_irq_enable(void)
{
    LOS_IntUnLock();
}

OAL_STATIC OAL_INLINE void  oal_irq_disable(void)
{
    LOS_IntLock();
}

OAL_STATIC OAL_INLINE void  oal_irq_trigger(uint8_t uc_cpuid)
{
}

OAL_INLINE int32_t  oal_irq_setup(oal_irq_dev_stru *st_osdev)
{
    return 0;
}

OAL_INLINE int32_t  oal_irq_free(oal_irq_dev_stru *st_osdev)
{
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_hardware.h */
