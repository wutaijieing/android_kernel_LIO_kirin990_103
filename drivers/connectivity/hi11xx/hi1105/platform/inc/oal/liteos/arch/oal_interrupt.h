

#ifndef __OAL_LITEOS_INTERRUPT_H__
#define __OAL_LITEOS_INTERRUPT_H__

#include <linux/interrupt.h>
#include <asm/hal_platform_ints.h>
#include <gpio.h>
#include <hisoc/gpio.h>
#include <oal_types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef IRQF_NO_SUSPEND
#define IRQF_NO_SUSPEND 0x0000
#endif

#ifndef IRQF_DISABLED
#define IRQF_DISABLED 0x0000
#endif

#define GPIO_TO_IRQ(group, bit) ((group) * (GPIO_BIT_NUM) + (bit) + (OS_USER_HWI_MAX))
#define IRQ_TO_GPIO_GROUP(irq) (((irq) - (OS_USER_HWI_MAX)) / (GPIO_BIT_NUM))
#define IRQ_TO_GPIO_BIT(irq) (((irq) - (OS_USER_HWI_MAX)) % (GPIO_BIT_NUM))
OAL_STATIC OAL_INLINE int32_t oal_request_irq(uint32_t irq, irq_handler_t handler, uint32_t flags,
                                              const int8_t *name, void *dev)
{
    if (irq <= OS_USER_HWI_MAX) {
        return request_irq(irq, handler, flags, name, dev);
    } else {
        gpio_groupbit_info st_gpio_info = {0};
        st_gpio_info.groupnumber     = IRQ_TO_GPIO_GROUP(irq);
        st_gpio_info.bitnumber       = IRQ_TO_GPIO_BIT(irq);
        st_gpio_info.irq_handler     = (irq_func)handler;
        st_gpio_info.irq_type        = 0;
        st_gpio_info.data            = dev;

        return gpio_irq_register(&st_gpio_info);
    }
}

OAL_STATIC OAL_INLINE void oal_free_irq(uint32_t irq, void *dev)
{
    free_irq(irq, dev);
}

OAL_STATIC OAL_INLINE void oal_enable_irq(uint32_t irq)
{
    enable_irq(irq);
}

OAL_STATIC OAL_INLINE void oal_disable_irq(uint32_t irq)
{
    disable_irq(irq);
}

OAL_STATIC OAL_INLINE void oal_disable_irq_nosync(uint32_t irq)
{
    disable_irq(irq);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_interrupt.h */

