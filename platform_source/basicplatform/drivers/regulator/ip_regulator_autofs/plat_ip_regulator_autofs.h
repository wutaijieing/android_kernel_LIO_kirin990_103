#ifndef __PLAT_IP_REGULATOR_AUTOFS_H__
#define __PLAT_IP_REGULATOR_AUTOFS_H__

struct ip_regulator_autofs {
	int ip_regulator_id;
	char *ip_regulator_name;
};

#if defined CONFIG_MIA_IP_PLATFORM
#include "mia_autofs.h"
#elif defined CONFIG_BOST_IP_PLATFORM
#include "bost_autofs.h"
#elif defined CONFIG_ATLA_IP_PLATFORM
#include "alta_autofs.h"
#elif defined CONFIG_PHOE_IP_PLATFORM
#include "phoe_autofs.h"
#elif defined CONFIG_BALT_IP_PLATFORM
#include "balt_autofs.h"
#elif defined CONFIG_CHAR_IP_PLATFORM
#include "char_autofs.h"
#elif defined CONFIG_CHARPRO_IP_PLATFORM
#include "charpro_autofs.h"
#elif defined CONFIG_IP_PLATFORM_COMMON
#include "common_autofs.h"
#else
#include "other_autofs.h"
#endif

#endif