#include <linux/err.h>
#include <linux/printk.h>
#include "memory.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

static unsigned int       g_mem_used[MEM_ID_MAX]  = {0};
static unsigned long long g_mem_va[MEM_ID_MAX] = {0};
static unsigned int       g_mem_da[MEM_ID_MAX] = {0};

unsigned int g_mem_offset[MEM_ID_MAX] = {
	[MEM_ID_CMDLST_BUF_GF] =
		ALIGN_UP(CMDLST_BUFFER_SIZE * MAX_GF_STRIPE_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_GF] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t) *
			MAX_GF_STRIPE_NUM,
				CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_GF] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_GF_CFG_TAB] =
		ALIGN_UP(sizeof(struct gf_config_table_t) * MAX_GF_STRIPE_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CVDR_CFG_TAB_GF] =
		ALIGN_UP(sizeof(struct cfg_tab_cvdr_t) * MAX_GF_STRIPE_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_ORB_CFG_TAB] =
		ALIGN_UP(sizeof(struct enh_orb_cfg_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_ORB] =
		ALIGN_UP(CMDLST_BUFFER_SIZE * CMDLST_STRIPE_MAX_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_ORB] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t) *
			CMDLST_STRIPE_MAX_NUM,
				CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_ORB] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_VBK_CFG_TAB] =
		ALIGN_UP(sizeof(struct vbk_config_table_t) * MAX_VBK_STRIPE_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_VBK] =
		ALIGN_UP(CMDLST_BUFFER_SIZE * MAX_VBK_STRIPE_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_VBK] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t) *
			MAX_VBK_STRIPE_NUM,
				CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_VBK] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CVDR_CFG_TAB_VBK] =
		ALIGN_UP(sizeof(struct cfg_tab_cvdr_t) * MAX_VBK_STRIPE_NUM,
			CVDR_ALIGN_BYTES),
	[MEM_ID_REORDER_CFG_TAB] =
		ALIGN_UP(sizeof(struct cfg_tab_reorder_t) * ORB_LAYER_MAX,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_REORDER] =
		ALIGN_UP(CMDLST_BUFFER_SIZE * ORB_LAYER_MAX,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_REORDER] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t) * ORB_LAYER_MAX,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_REORDER] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CVDR_CFG_TAB_REORDER] =
		ALIGN_UP(sizeof(struct cfg_tab_cvdr_t) * ORB_LAYER_MAX,
			CVDR_ALIGN_BYTES),
	[MEM_ID_COMPARE_CFG_TAB] =
		ALIGN_UP(sizeof(struct cfg_tab_compare_t) * ORB_LAYER_MAX,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_COMPARE] =
		ALIGN_UP(CMDLST_BUFFER_SIZE * ORB_LAYER_MAX * 3,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_COMPARE] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t) *
			ORB_LAYER_MAX * 3,
				CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_COMPARE] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CVDR_CFG_TAB_COMPARE] =
		ALIGN_UP(sizeof(struct cfg_tab_cvdr_t) * ORB_LAYER_MAX * 3,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CFG_TAB_MC] =
		ALIGN_UP(sizeof(struct cfg_tab_mc_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CVDR_CFG_TAB_MC] =
		ALIGN_UP(sizeof(struct cfg_tab_cvdr_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_MC] =
		ALIGN_UP(CMDLST_BUFFER_SIZE * ORB_LAYER_MAX,
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_MC] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t) *
			ORB_LAYER_MAX, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_MC] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_ORB_ENH_CFG_TAB] =
		ALIGN_UP(sizeof(struct cfg_tab_orb_enh_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_ORB_ENH] =
		ALIGN_UP(CMDLST_BUFFER_SIZE, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_ORB_ENH] =
		ALIGN_UP(sizeof(struct schedule_cmdlst_link_t),
			CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_ORB_ENH] =
		ALIGN_UP(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CVDR_CFG_TAB_ORB_ENH] =
		ALIGN_UP(sizeof(struct cfg_tab_cvdr_t), CVDR_ALIGN_BYTES),
};

int cpe_mem_init(unsigned long long va,
	unsigned int da, unsigned int size)
{
	int i = 0;

	for (i = 0; i < MEM_ID_MAX; i++) {
		g_mem_used[i]  = 0;

		if (i == 0) {
			g_mem_va[i] = va;
			g_mem_da[i] = da;
		} else {
			g_mem_va[i] = g_mem_va[i - 1] + g_mem_offset[i - 1];
			g_mem_da[i] = g_mem_da[i - 1] + g_mem_offset[i - 1];
		}
	}

	if (g_mem_va[MEM_ID_MAX - 1] - va > size) {
		E(" Failed : vaddr overflow");
		return -ENOMEM;
	}

	if (g_mem_da[MEM_ID_MAX - 1] - da > size) {
		E("Failed : daddr overflow\n");
		return -ENOMEM;
	}		

	return 0;
}

int cpe_init_memory(void)
{
	unsigned int da;
	unsigned long long va;
	int ret;

	da = get_cpe_addr_da();

	if (da == 0) {
		E(" Failed : CPE Device da false");
		return -ENOMEM;
	}

	va = (unsigned long long)(uintptr_t)get_cpe_addr_va();

	if (va == 0) {
		E(" Failed : CPE Device va false");
		return -ENOMEM;
	}

	ret = cpe_mem_init(va, da, MEM_HISPCPE_SIZE);
	if (ret != 0) {
		E(" Failed : cpe_mem_init%d", ret);
		return -ENOMEM;
	}

	return 0;
}

int cpe_mem_get(enum cpe_mem_id mem_id,
	unsigned long long *va, unsigned int *da)
{
	if (va == NULL || da == NULL) {
		E(" Failed : va.%pK, da.%pK", va, da);
		return -ENOMEM;
	}

	if (mem_id >= MEM_ID_MAX) {
		E(" Failed : mem_id.(%u >= %u)",
			mem_id, MEM_ID_MAX);
		return -ENOMEM;
	}

	if (g_mem_used[mem_id] == 1) {
		E(" Failed : g_mem_used[%u].%u",
			mem_id, g_mem_used[mem_id]);
		return -ENOMEM;
	}
	*da = (unsigned int)g_mem_da[mem_id];
	*va = (unsigned long long)g_mem_va[mem_id];

	if ((*da == 0) || (*va == 0)) {
		E(" Failed : g_mem_da, g_mem_va");
		return -ENOMEM;
	}

	g_mem_used[mem_id] = 1;
	return 0;
}

void cpe_mem_free(enum cpe_mem_id mem_id)
{
	if (mem_id >= MEM_ID_MAX) {
		E("Failed : Invalid parameter! mem_id = %u\n", mem_id);
		return;
	}

	if (g_mem_used[mem_id] == 0)
		E("Failed : Unable to free, g_mem_used[%u].%u",
			mem_id, g_mem_used[mem_id]);

	g_mem_used[mem_id] = 0;
}

#pragma GCC diagnostic pop

