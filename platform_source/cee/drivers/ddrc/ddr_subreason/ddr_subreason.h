/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: dmss intr subreason
 * Author: caodongyi
 * Create: 2020-5-27
 */

#ifndef _DDR_SUBREASON_H_
#define _DDR_SUBREASON_H_

#ifdef CONFIG_DDR_MONOCEROS
#include "ddr_subreason_monoceros.h"
#elif defined(CONFIG_DDR_PERSEUS)
#include "ddr_subreason_perseus.h"
#elif defined(CONFIG_DDR_CASSIOPEIA)
#include "ddr_subreason_cassiopeia.h"
#elif defined(CONFIG_DDR_PISCES)
#include "ddr_subreason_pisces.h"
#elif defined(CONFIG_DDR_CHAMAELEON)
#include "ddr_subreason_chamaeleon.h"
#endif

#endif
