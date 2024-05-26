#ifndef _MODEM_STRATEGY_ACORE_H
#define _MODEM_STRATEGY_ACORE_H

#include "dsp_om.h"

#define OM_CHUNK_NUM (5)
#define OM_CHUNK_SIZE (64 * 1024)

enum work_id_proxy {
	HIFI_OM_WORK_PROXY = 0,
	HIFI_PCIE_REOPEN,
	HIFI_PCIE_CLOSE
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
struct socdsp_voice_om_work_info {
	enum work_id_proxy work_id;
	char *work_name;
	work_func_t func;
	struct socdsp_om_work_ctl ctl;
};
#endif

void proxy_rev_data_handle(enum socdsp_om_work_id work_id, const unsigned char *addr, unsigned int len);
void modem_om_acore_init(void);
void modem_om_acore_deinit(void);
#endif
