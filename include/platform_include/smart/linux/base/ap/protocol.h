/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: protocol header file
 * Create: 2021/12/05
 */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#ifdef CONFIG_INPUTHUB_30
// inputhub_30 + inputhub_as
#ifdef CONFIG_INPUTHUB_AS
	#include "as/protocol_base.h"
	#include "as/protocol_ext.h"
#endif

// input_30 + inputhub_20
#ifdef CONFIG_INPUTHUB_20
	#include "30/protocol_base.h"
	#include "30/protocol_ext.h"
#endif
#else
#if defined CONFIG_INPUTHUB_20_710
	#include "20_710/protocol_base.h"
	#include "20_710/protocol_ext.h"
#elif defined CONFIG_INPUTHUB_20_970
	#include "20_970/protocol_base.h"
	#include "20_970/protocol_ext.h"
#else
	#include "20/protocol_base.h"
	#include "20/protocol_ext.h"
#endif
#endif

#endif
