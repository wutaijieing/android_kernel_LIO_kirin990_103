/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: secboot driver head file
 * Create: 2022/7/7
 */

#ifndef __SECBOOT_H__
#define __SECBOOT_H__

#ifdef CONFIG_SECBOOT_AB_SLOT_TRIGGER_UPDATE_NVCNT
/*
 * trigger update nv counter
 */
void seb_trigger_update_version(void);
#else
static inline void seb_trigger_update_version(void){}
#endif

#endif /* __SECBOOT_H__ */
