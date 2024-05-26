
#ifndef _DMSS_PT_H
#define _DMSS_PT_H

#define DMSSPT_UNIT_SIZE		128

#ifdef CONFIG_DFX_MNTN_PC
#define DMSSPT_ROOT "/var/log/hisi/bbox/running_trace/dmss_pt/"
#else
#define DMSSPT_ROOT "/data/hisi_logs/running_trace/dmss_pt/"
#endif
#define DMSSPT_BUF_DTS_NODE	"/reserved-memory/dmss_pt"

#define DMSSPT_DUMP_TIMEOUT	5
#define DMSSPT_DIR_LIMIT		0770
#define DMSSPT_FILE_LIMIT		0660
#define DMSSPT_PATH_MAXLEN         128UL
#define DMSSPT_NUM_OFFSET		1

#define DMSSPT_RT_PRIORITY		90

#define DMSSPT_FILE_MAX_LEN			0x40000000UL

#define DMSSPT_NOINIT_MAGIC		0xC5C5C5C5U
#define DMSSPT_FS_INIT_MAGIC		0x1FFFFFFF

#define SAVE_AGAIN	1
#define SAVE_CONTINUE	(-1)
#define SAVE_FAIL	(-2)
#define SIZE_1KB	1024
#define PT_REGION_PART_NUM	2
#define TOTAL_FILE_INDEX_MAX	20

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

typedef enum {
	TRACE_MODE0 = 0, /* power consumption mode */
	TRACE_MODE1 = 1, /* performance mode */
	TRACE_MODE_MAX,
}
INTLV_MODE;

typedef enum {
	DMSS_TRACE0 = 0, /* hardware trace module 0 */
	DMSS_TRACE1 = 1, /* hardware trace module 1 */
	DMSS_TRACE_MAX,
}
TRACE_MOD;

struct dmsspt_region {
	phys_addr_t			base;
	phys_addr_t			size;
	phys_addr_t			fama_offset;
};

struct pattern_trace_info {
	u32 intlv;
	u32 intlv_mode;
	u32 dmi_num;
	u64 pt_wptr[DMSS_TRACE_MAX];
	u64 pt_rptr[DMSS_TRACE_MAX];/* physical address */
};

struct pattern_trace_stat {
	u64 trace_begin_time;
	u64 trace_end_time;
	u32 trace_cur_address[DMSS_TRACE_MAX];
	u32 trace_pattern_cnt[DMSS_TRACE_MAX];
	u32 trace_roll_cnt[DMSS_TRACE_MAX];
	u32 trace_unaligned_mode;
};

struct pattern_trace_header {
	u64 trace_begin_time;
	u64 pad0;
	u64 trace_end_time;
	u64 pad1;
	u32 trace_cur_address;  /* GLB_TRACE_STAT0 */
	u32 trace_pattern_cnt; /* GLB_TRACE_STAT1 */
	u32 trace_roll_cnt; /* GLB_TRACE_STAT2 */
	u32 trace_unaligned_mode; /* GLB_TRACE_CTRL1[28] */
};

#ifdef CONFIG_DDR_DMSS_PT
int dmsspt_save_trace(struct pattern_trace_info *pinfo,
		      struct pattern_trace_stat *pstat,
		      bool is_tracing_stop);
struct dmsspt_region  get_dmsspt_buffer_region(void);
int dmsspt_is_finished(void);
#else
static inline int dmsspt_save_trace(struct pattern_trace_info *pinfo,
				    struct pattern_trace_stat *pstat,
				    bool is_tracing_stop)
{ return 0; }
static inline struct dmsspt_region  get_dmsspt_buffer_region(void)
{
	struct dmsspt_region region;
	region.base = 0;
	region.size = 0;
	return region;
}
#endif

#endif
