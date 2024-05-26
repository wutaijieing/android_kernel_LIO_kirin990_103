#ifndef _MEMORY_CS_H_INCLUDED
#define _MEMORY_CS_H_INCLUDED

#include "ipp.h"
#include "segment_common.h"
#include "cfg_table_gf.h"
#include "cfg_table_cpe_top.h"
#include "cfg_table_cvdr.h"
#include "cfg_table_arfeature.h"
#include "cfg_table_reorder.h"
#include "cfg_table_compare.h"
#include "cfg_table_mc.h"
#include "cfg_table_hiof.h"
#include "cfg_table_orb_enh.h"
#include "cfg_table_matcher.h"

enum cpe_mem_id {
	MEM_ID_CMDLST_BUF_ORB_ENH       = 0,
	MEM_ID_CMDLST_ENTRY_ORB_ENH,
	MEM_ID_CMDLST_PARA_ORB_ENH,
	MEM_ID_CMDLST_BUF_GF,
	MEM_ID_CMDLST_ENTRY_GF,
	MEM_ID_CMDLST_PARA_GF,
	MEM_ID_GF_CFG_TAB,
	MEM_ID_ARF_CFG_TAB,
	MEM_ID_CMDLST_BUF_ARF,
	MEM_ID_CMDLST_ENTRY_ARF,
	MEM_ID_CMDLST_PARA_ARF,
	MEM_ID_HIOF_CFG_TAB,
	MEM_ID_CMDLST_BUF_HIOF,
	MEM_ID_CMDLST_ENTRY_HIOF,
	MEM_ID_CMDLST_PARA_HIOF,
	MEM_ID_REORDER_CFG_TAB,
	MEM_ID_CMDLST_BUF_REORDER,
	MEM_ID_CMDLST_ENTRY_REORDER,
	MEM_ID_CMDLST_PARA_REORDER,
	MEM_ID_COMPARE_CFG_TAB,
	MEM_ID_CMDLST_BUF_COMPARE,
	MEM_ID_CMDLST_ENTRY_COMPARE,
	MEM_ID_CMDLST_PARA_COMPARE,
	MEM_ID_CFG_TAB_MC,
	MEM_ID_CMDLST_BUF_MC,
	MEM_ID_CMDLST_ENTRY_MC,
	MEM_ID_CMDLST_PARA_MC,
	MEM_ID_ORB_ENH_CFG_TAB,
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

int hispipp_cfg_check(unsigned long args);

#endif /* _MEMORY_CS_H_INCLUDED */
