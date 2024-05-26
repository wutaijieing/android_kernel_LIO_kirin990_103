#ifndef _MEMORY_CS_H_INCLUDED
#define _MEMORY_CS_H_INCLUDED

#include "ipp.h"
#include "adapter_common.h"
#include "cfg_table_gf.h"
#include "config_table_cvdr.h"
#include "orb_common.h"
#include "cfg_table_orb.h"

#define MEM_HISPCPE_SIZE   	0x900000 /*  the cpe iova size */
#define MEM_HIPPTOF_SEC_SIZE         0x01800000

enum cpe_mem_id {
	MEM_ID_CMDLST_BUF_GF       = 0,
	MEM_ID_CMDLST_ENTRY_GF,
	MEM_ID_CMDLST_PARA_GF,
	MEM_ID_GF_CFG_TAB,
	MEM_ID_CVDR_CFG_TAB_GF,
	MEM_ID_ORB_CFG_TAB,
	MEM_ID_CMDLST_BUF_ORB,
	MEM_ID_CMDLST_ENTRY_ORB,
	MEM_ID_CMDLST_PARA_ORB,
	MEM_ID_MAX
};

struct cpe_va_da {
	unsigned long long va;
	unsigned int       da;
};

extern int cpe_mem_init(unsigned long long va,
	unsigned int da, unsigned int size);
extern int cpe_mem_get(enum cpe_mem_id mem_id,
	unsigned long long *va, unsigned int *da);
extern void cpe_mem_free(enum cpe_mem_id mem_id);

extern int cpe_init_memory(void);
extern void *get_cpe_addr_va(void);
extern unsigned int get_cpe_addr_da(void);

#endif /*_MEMORY_CS_H_INCLUDED*/



