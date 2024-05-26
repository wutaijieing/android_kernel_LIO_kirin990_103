

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

#define OAL_HI_TIMER_FREE_MODE              0         /* 1101�������� */
#define OAL_HI_TIMER_CYCLE_MODE             1
#define OAL_HI_TIMER_SIZE_32_BIT            1
#define OAL_HI_TIMER_WRAPPING               0
#define OAL_HI_TIMER_INT_OCCUR              1
#define OAL_HI_TIMER_INT_VALID              0x01
#define OAL_HI_TIMER_NO_DIV_FREQ            0x0

#define OAL_HI_SC_REG_BASE                  0x10100000
#define OAL_HI_SC_CTRL                      (OAL_HI_SC_REG_BASE + 0x0000)

#define OAL_IRQ_ENABLE                      1  /* �����ж� */
#define OAL_IRQ_FORBIDDEN                   0  /* ��ֹ�ж� */
#if (HW_LITEOS_OPEN_VERSION_NUM < KERNEL_VERSION(3,1,3))
#define IRQF_SHARED    0x00000080
#endif
#define OAL_SA_SHIRQ    IRQF_SHARED       /* �ж����� */
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
/* timer���ƼĴ��� */
typedef union {
    volatile uint32_t ul_value;
    struct {
        volatile uint32_t ul_oneshot : 1;                 /* ѡ�����ģʽ 0���ؾ���� 1��һ���Լ��� */
        volatile uint32_t ul_timersize : 1;               /* 16bit|32bit��������ģʽ 0��16bit 1��32bit */
        volatile uint32_t ul_timerpre : 2;                /* Ԥ��Ƶ���� 00������Ƶ 01��4����Ƶ 10��8����Ƶ 11��δ���壬�����൱�ڷ�Ƶ����10 */
        volatile uint32_t ul_reserved0 : 1;               /* ����λ */
        volatile uint32_t ul_intenable : 1;               /* �ж�����λ 0������ 1�������� */
        volatile uint32_t ul_timermode : 1;               /* ����ģʽ 0������ģʽ 1������ģʽ */
        volatile uint32_t ul_timeren : 1;                 /* ��ʱ��ʹ��λ 0����ֹ 1��ʹ�� */
        volatile uint32_t ul_reserved1 : 24;              /* ����λ */
    } bits_stru;
} oal_hi_timer_control_union;

/* timer2_3�Ĵ��� */
typedef struct {
    oal_hi_timerx_reg_stru ast_timer[2]; // timer2����3�Ĵ�������Ҫ2����Ӧ��Ա
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
    uint8_t is_bind;    /* �Զ����ָ��,�豣֤��������λ */
    uint8_t irq_cpu;    /* �û���Ӳ�ж����� */
    uint8_t thread_cmd; /* �û����߳����� */
    oal_bool_enum_uint8 is_userctl; /* �Ƿ�����û������� */
}oal_pcie_bindcpu_cfg;

/* �ж��豸�ṹ�� */
typedef struct {
    uint32_t              irq;                  /* �жϺ� */
    int32_t               l_irq_type;             /* �ж����ͱ�־ */
    void               *p_drv_arg;              /* �жϴ��������� */
    int8_t               *pc_name;                /* �ж��豸���� ֻΪ�����Ѻ� */
    oal_irq_intr_func       p_irq_intr_func;        /* �жϴ�������ַ */
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
