#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/printk.h>
#include "memory.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#define PLATFORM_VERSION 350
#define MEM_HISPCPE_SIZE 0x02000000 /*  the cpe iova size */

struct ipp_cfg_s {
	uint32_t platform_version;
	uint32_t mapbuffer;
	uint32_t mapbuffer_sec;
};

static unsigned int       g_mem_used[MEM_ID_MAX]  = {0};
static unsigned long long g_mem_va[MEM_ID_MAX] = {0};
static unsigned int       g_mem_da[MEM_ID_MAX] = {0};

unsigned int g_mem_offset[MEM_ID_MAX] = {
	[MEM_ID_GF_CFG_TAB] =
	align_up(sizeof(struct seg_gf_cfg_t) * MAX_GF_STRIPE_NUM, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_GF] =
	align_up(CMDLST_BUFFER_SIZE * MAX_GF_STRIPE_NUM, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_GF] =
	align_up(sizeof(struct schedule_cmdlst_link_t) * MAX_GF_STRIPE_NUM, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_GF] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),

	[MEM_ID_ARF_CFG_TAB] =
	align_up(sizeof(struct cfg_enh_arf_t), CVDR_ALIGN_BYTES), // ipp path:orb+enh use one cfg table
	[MEM_ID_CMDLST_BUF_ARF] =
	align_up(CMDLST_BUFFER_SIZE * CMDLST_STRIPE_MAX_NUM, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_ARF] =
	align_up(sizeof(struct schedule_cmdlst_link_t) * CMDLST_STRIPE_MAX_NUM, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_ARF] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),

	[MEM_ID_HIOF_CFG_TAB] =
	align_up(sizeof(struct seg_hiof_cfg_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_HIOF] =
	align_up(CMDLST_BUFFER_SIZE, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_HIOF] =
	align_up(sizeof(struct schedule_cmdlst_link_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_HIOF] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),

	[MEM_ID_REORDER_CFG_TAB] =
	align_up(sizeof(struct seg_matcher_rdr_cfg_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_REORDER] =
	align_up(CMDLST_BUFFER_SIZE * MATCHER_LAYER_MAX, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_REORDER] =
	align_up(sizeof(struct schedule_cmdlst_link_t) * MATCHER_LAYER_MAX, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_REORDER] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),

	[MEM_ID_COMPARE_CFG_TAB] =
	align_up(sizeof(struct seg_matcher_cmp_cfg_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_COMPARE] =
	align_up(CMDLST_BUFFER_SIZE *MATCHER_LAYER_MAX * 3, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_COMPARE] =
	align_up(sizeof(struct schedule_cmdlst_link_t) * MATCHER_LAYER_MAX * 3, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_COMPARE] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),

	[MEM_ID_CFG_TAB_MC] =
	align_up(sizeof(struct seg_mc_cfg_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_MC] =
	align_up(CMDLST_BUFFER_SIZE * MATCHER_LAYER_MAX, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_MC] =
	align_up(sizeof(struct schedule_cmdlst_link_t) * MATCHER_LAYER_MAX, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_MC] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),

	[MEM_ID_ORB_ENH_CFG_TAB] =
	align_up(sizeof(struct seg_orb_enh_cfg_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_BUF_ORB_ENH] =
	align_up(CMDLST_BUFFER_SIZE, CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_ENTRY_ORB_ENH] =
	align_up(sizeof(struct schedule_cmdlst_link_t), CVDR_ALIGN_BYTES),
	[MEM_ID_CMDLST_PARA_ORB_ENH] =
	align_up(sizeof(struct cmdlst_para_t), CVDR_ALIGN_BYTES),
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
		loge("Failed : vaddr overflow");
		return -ENOMEM;
	}

	if (g_mem_da[MEM_ID_MAX - 1] - da > size) {
		loge("Failed : daddr overflow");
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
		loge(" Failed : CPE Device da false");
		return -ENOMEM;
	}

	va = (unsigned long long)(uintptr_t)get_cpe_addr_va();
	if (va == 0) {
		loge(" Failed : CPE Device va false");
		return -ENOMEM;
	}

	ret = cpe_mem_init(va, da, MEM_HISPCPE_SIZE);
	if (ret != 0) {
		loge(" Failed : cpe_mem_init%d", ret);
		return -ENOMEM;
	}

	return 0;
}

int cpe_mem_get(enum cpe_mem_id mem_id,
				unsigned long long *va, unsigned int *da)
{
	if (va == NULL || da == NULL) {
		loge(" Failed : va.%pK, da.%pK", va, da);
		return -ENOMEM;
	}

	if (mem_id >= MEM_ID_MAX) {
		loge(" Failed : mem_id.(%u >= %u)",
			 mem_id, MEM_ID_MAX);
		return -ENOMEM;
	}

	if (g_mem_used[mem_id] == 1) {
		loge(" Failed : g_mem_used[%u].%u",
			 mem_id, g_mem_used[mem_id]);
		return -ENOMEM;
	}

	*da = (unsigned int)g_mem_da[mem_id];
	*va = (unsigned long long)g_mem_va[mem_id];
	if ((*da == 0) || (*va == 0)) {
		loge(" Failed : g_mem_da, g_mem_va");
		return -ENOMEM;
	}

	g_mem_used[mem_id] = 1;
	return 0;
}

void cpe_mem_free(enum cpe_mem_id mem_id)
{
	if (mem_id >= MEM_ID_MAX) {
		loge("Failed : Invalid parameter! mem_id = %u", mem_id);
		return;
	}

	if (g_mem_used[mem_id] == 0)
		loge("Failed : Unable to free, g_mem_used[%u].%u",
			 mem_id, g_mem_used[mem_id]);

	g_mem_used[mem_id] = 0;
}

int hispipp_cfg_check(unsigned long args)
{
	int ret;
	struct ipp_cfg_s ipp_cfg = { 0 };
	void __user *args_cfgcheck = (void __user *)(uintptr_t) args;

	pr_info("[%s] +\n", __func__);

	if (args_cfgcheck == NULL) {
		pr_err("[%s] args_cfgcheck.%pK\n", __func__, args_cfgcheck);
		return -EINVAL;
	}

	ipp_cfg.platform_version = PLATFORM_VERSION;
	ipp_cfg.mapbuffer = MEM_HISPCPE_SIZE;

	ret = copy_to_user(args_cfgcheck, &ipp_cfg, sizeof(struct ipp_cfg_s));
	if (ret != 0) {
		pr_err("[%s] copy_to_user.%d\n", __func__, ret);
		return -EFAULT;
	}

	pr_info("[%s] -\n", __func__);
	return ISP_IPP_OK;
}


#pragma GCC diagnostic pop

