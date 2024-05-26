

#ifndef __BFGX_GLE_H__
#define __BFGX_GLE_H__

/* ����ͷ�ļ����� */
#include <linux/miscdevice.h>
#include "hw_bfg_ps.h"

/* �궨�� */
#define DTS_COMP_HI110X_PS_GLE_NAME "hisilicon,hisi_gle"

/* �������� */
struct platform_device *get_gle_platform_device(void);
int32_t hw_ps_gle_init(void);
void hw_ps_gle_exit(void);

#endif /* __BFGX_GLE_H__ */

