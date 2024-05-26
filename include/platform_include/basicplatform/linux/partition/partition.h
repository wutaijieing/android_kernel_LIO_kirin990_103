/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: partition table header
 */

#ifndef _PARTITION_H_
#define _PARTITION_H_

#include "partition_macro.h"
#include "partition_def.h"

#ifdef CONFIG_PLAT_PARTITION_CS2

#ifdef CONFIG_PRODUCT_ARMPC
#include <armpc_cs2_plat_partition.h>
#elif defined(CONFIG_PRODUCT_CDC)
#include <cdc_cs2_plat_partition.h>
#elif defined(CONFIG_PRODUCT_CDC_ACE)
#include <cdc_ace_cs2_plat_partition.h>
#else
#include <cs2_plat_partition.h> /* not pc */
#endif

#else /* not CS2 */

#ifdef CONFIG_PRODUCT_ARMPC
#include <armpc_plat_partition.h>
#elif defined(CONFIG_PRODUCT_CDC)
#include <cdc_plat_partition.h>
#elif defined(CONFIG_PARTITION_PLAT_UNLEGACY)
#ifdef CONFIG_RECOVERY_AB_PARTITION
#include <plat_partition_recovery_ab.h>
#else
#include <plat_partition_unlegacy.h>
#endif
#elif defined(CONFIG_PARTITION_A_PLUS_B_EXTERNAL_MODEM)
#include <a_plus_b_plat_partition.h>
#elif defined(CONFIG_PLAT_PARTITION_BASE)
#include <plat_a_partition.h>
#else
#include <plat_partition.h>
#endif

#endif /* end of CONFIG_PLAT_PARTITION_CS2 */

#endif
