

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/buffer_head.h>
#include <linux/backing-dev.h>
#include <linux/kthread.h>
#include <linux/parser.h>
#include <linux/mount.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/exportfs.h>
#include <linux/blkdev.h>
#include <linux/f2fs_fs.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#ifdef CONFIG_MAS_BLK
#include <linux/workqueue.h>
#endif
#endif

#include "f2fs.h"
#include "segment.h"

/* Display on console */
#define DISP(fmt, ptr, member)				\
	do {						\
		printk("F2FS-fs:%-30s\t\t" fmt, #member, ((ptr)->member));	\
	} while (0)

#define DISP_u16(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8x : %u]\n",		\
			#member, le16_to_cpu((ptr)->member),            \
			le16_to_cpu((ptr)->member));	\
	} while (0)


#define DISP_u32(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8x : %u]\n",		\
			#member, le32_to_cpu((ptr)->member),            \
			le32_to_cpu((ptr)->member));	\
	} while (0)

#define DISP_u64(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8llx : %llu]\n",		\
			#member, le64_to_cpu((ptr)->member),            \
			le64_to_cpu((ptr)->member));	\
	} while (0)


/* print the f2fs superblock infomation to the kernel message,
 * it will be saved by DMD or panic log
 * simplified info of f2fs tool: fsck.f2fs
 * f2fs superblock struct locate in kernel/include/linux/f2fs_fs.h
 */
void f2fs_print_raw_sb_info(struct f2fs_sb_info *sbi)
{
	struct f2fs_super_block *sb = NULL;

	if (sbi == NULL) {
		return;
	}

	sb = F2FS_RAW_SUPER(sbi);

	if (sb == NULL) {
		return;
	}

	printk("\n");
	printk("+--------------------------------------------------------+\n");
	printk("| Super block                                            |\n");
	printk("+--------------------------------------------------------+\n");

	DISP_u32(sb, magic);
	DISP_u16(sb, major_ver);
	DISP_u16(sb, minor_ver);
	DISP_u32(sb, log_sectorsize);
	DISP_u32(sb, log_sectors_per_block);

	DISP_u32(sb, log_blocksize);
	DISP_u32(sb, log_blocks_per_seg);
	DISP_u32(sb, segs_per_sec);
	DISP_u32(sb, secs_per_zone);
	DISP_u32(sb, checksum_offset);
	DISP_u64(sb, block_count);

	DISP_u32(sb, section_count);
	DISP_u32(sb, segment_count);
	DISP_u32(sb, segment_count_ckpt);
	DISP_u32(sb, segment_count_sit);
	DISP_u32(sb, segment_count_nat);

	DISP_u32(sb, segment_count_ssa);
	DISP_u32(sb, segment_count_main);
	DISP_u32(sb, segment0_blkaddr);

	DISP_u32(sb, cp_blkaddr);
	DISP_u32(sb, sit_blkaddr);
	DISP_u32(sb, nat_blkaddr);
	DISP_u32(sb, ssa_blkaddr);
	DISP_u32(sb, main_blkaddr);

	DISP_u32(sb, root_ino);
	DISP_u32(sb, node_ino);
	DISP_u32(sb, meta_ino);
	DISP_u32(sb, cp_payload);
	DISP_u32(sb, feature);
	DISP("%s", sb, version);
	printk("\n\n");
}

void f2fs_print_ckpt_info(struct f2fs_sb_info *sbi)
{
	struct f2fs_checkpoint *cp = NULL;

	if (sbi == NULL) {
		return;
	}

	cp = F2FS_CKPT(sbi);

	if (cp == NULL) {
		return;
	}

	printk("\n");
	printk("+--------------------------------------------------------+\n");
	printk("| Checkpoint                                             |\n");
	printk("+--------------------------------------------------------+\n");

	DISP_u64(cp, checkpoint_ver);
	DISP_u64(cp, user_block_count);
	DISP_u64(cp, valid_block_count);
	DISP_u32(cp, rsvd_segment_count);
	DISP_u32(cp, overprov_segment_count);
	DISP_u32(cp, free_segment_count);

	DISP_u32(cp, alloc_type[CURSEG_HOT_NODE]);
	DISP_u32(cp, alloc_type[CURSEG_WARM_NODE]);
	DISP_u32(cp, alloc_type[CURSEG_COLD_NODE]);
	DISP_u32(cp, cur_node_segno[0]);
	DISP_u32(cp, cur_node_segno[1]);
	DISP_u32(cp, cur_node_segno[2]);

	DISP_u16(cp, cur_node_blkoff[0]);
	DISP_u16(cp, cur_node_blkoff[1]);
	DISP_u16(cp, cur_node_blkoff[2]);

	DISP_u32(cp, alloc_type[CURSEG_HOT_DATA]);
	DISP_u32(cp, alloc_type[CURSEG_WARM_DATA]);
	DISP_u32(cp, alloc_type[CURSEG_COLD_DATA]);
	DISP_u32(cp, cur_data_segno[0]);
	DISP_u32(cp, cur_data_segno[1]);
	DISP_u32(cp, cur_data_segno[2]);

	DISP_u16(cp, cur_data_blkoff[0]);
	DISP_u16(cp, cur_data_blkoff[1]);
	DISP_u16(cp, cur_data_blkoff[2]);

	DISP_u32(cp, ckpt_flags);
	DISP_u32(cp, cp_pack_total_block_count);
	DISP_u32(cp, cp_pack_start_sum);
	DISP_u32(cp, valid_node_count);
	DISP_u32(cp, valid_inode_count);
	DISP_u32(cp, next_free_nid);
	DISP_u32(cp, sit_ver_bitmap_bytesize);
	DISP_u32(cp, nat_ver_bitmap_bytesize);
	DISP_u32(cp, checksum_offset);
	DISP_u64(cp, elapsed_time);

	DISP_u32(cp, sit_nat_version_bitmap[0]);
	printk("\n\n");
}

extern int f2fs_fill_super_done;
void f2fs_print_sbi_info(struct f2fs_sb_info *sbi)
{
	if (!f2fs_fill_super_done)
		return;
	if (sbi == NULL)
		return;
	f2fs_err(sbi, "\n");
	f2fs_err(sbi, "+--------------------------------------------------------+\n");
	f2fs_err(sbi, "|       SBI(Real time dirty nodes/segments info)         |\n");
	f2fs_err(sbi, "+--------------------------------------------------------+\n");

	f2fs_err(sbi, "ndirty_node: %d\n", get_pages(sbi, F2FS_DIRTY_NODES));
	f2fs_err(sbi, "ndirty_dent: %d\n", get_pages(sbi, F2FS_DIRTY_DENTS));
	f2fs_err(sbi, "ndirty_meta: %d\n", get_pages(sbi, F2FS_DIRTY_META));
	f2fs_err(sbi, "ndirty_data: %d\n", get_pages(sbi, F2FS_DIRTY_DATA));
#ifdef CONFIG_F2FS_STAT_FS
	f2fs_err(sbi, "ndirty_files: %d\n", sbi->ndirty_inode[DIR_INODE]);
#endif
	f2fs_err(sbi, "inmem_pages: %d\n", get_pages(sbi, F2FS_INMEM_PAGES));
	f2fs_err(sbi, "total_count: %d\n", ((unsigned int)sbi->user_block_count)/((unsigned int)sbi->blocks_per_seg));
	f2fs_err(sbi, "rsvd_segs: %d\n", reserved_segments(sbi));
	f2fs_err(sbi, "overp_segs: %d\n", overprovision_segments(sbi));
	f2fs_err(sbi, "valid_count: %d\n", valid_user_blocks(sbi));
#ifdef CONFIG_F2FS_STAT_FS
	/*lint -save -e529 -e438*/
	f2fs_err(sbi, "inline_attr: %d\n", atomic_read(&sbi->inline_xattr));
	f2fs_err(sbi, "inline_inode: %d\n", atomic_read(&sbi->inline_inode));
	f2fs_err(sbi, "inline_dir: %d\n", atomic_read(&sbi->inline_dir));
	/*lint -restore*/
#endif
	f2fs_err(sbi, "utilization: %d\n", utilization(sbi));

	f2fs_err(sbi, "free_segs: %d\n", free_segments(sbi));
	f2fs_err(sbi, "free_secs: %d\n", free_sections(sbi));
#ifdef CONFIG_F2FS_TURBO_ZONE
	if (is_tz_existed(sbi))
		f2fs_err(sbi, "normal_free_segs: %d\n",
			 get_free_segs_in_normal_zone(sbi));
#endif
	f2fs_err(sbi, "prefree_count: %d\n", prefree_segments(sbi));
	f2fs_err(sbi, "dirty_count: %d\n", dirty_segments(sbi));
	f2fs_err(sbi, "node_pages: %d\n",(int)NODE_MAPPING(sbi)->nrpages);
	f2fs_err(sbi, "meta_pages: %d\n",(int)META_MAPPING(sbi)->nrpages);
	f2fs_err(sbi, "nats: %d\n", NM_I(sbi)->nat_cnt[TOTAL_NAT]);
	f2fs_err(sbi, "dirty_nats: %d\n", SIT_I(sbi)->dirty_sentries);

#ifdef CONFIG_F2FS_STAT_FS
	f2fs_err(sbi, "segment_count[LFS]: %d\n", sbi->segment_count[0]);
	f2fs_err(sbi, "segment_count[SSR]: %d\n", sbi->segment_count[1]);
	f2fs_err(sbi, "block_count[LFS]: %d\n", sbi->block_count[0]);
	f2fs_err(sbi, "block_count[SSR]: %d\n", sbi->block_count[1]);
#endif

	f2fs_err(sbi, "\n\n");
}

#ifdef CONFIG_MAS_BLK
static unsigned int f2fs_free_size;
static inline void __do_f2fs_print_frag_info(struct super_block *sb, void *arg)
{
	struct f2fs_sb_info *sbi = NULL;
	unsigned long long total_size, free_size, undiscard_size;

	if (sb->s_magic != F2FS_SUPER_MAGIC)
		return;

	sbi = F2FS_SB(sb);

	total_size = blks_to_mb(sbi->user_block_count, sbi->blocksize);
	free_size = blks_to_mb(sbi->user_block_count - valid_user_blocks(sbi),
			sbi->blocksize);
	undiscard_size = blks_to_mb(SM_I(sbi)->dcc_info->undiscard_blks,
			sbi->blocksize);

	f2fs_free_size += free_size;

	pr_err("<f2fs> : size = %lluMB, free = %lluMB, undiscard = %lluMB, free_sec = %u\n",
			total_size, free_size, undiscard_size,
			free_sections(sbi));
}

/* get f2fs free_size */
unsigned int f2fs_get_free_size(void)
{
	f2fs_free_size = 0;
	iterate_supers(__do_f2fs_print_frag_info, NULL);
	return f2fs_free_size;
}
EXPORT_SYMBOL(f2fs_get_free_size);

static void do_f2fs_print_frag_info(struct work_struct *work)
{
	iterate_supers(__do_f2fs_print_frag_info, NULL);
}

static DECLARE_WORK(f2fs_print_frag_info_work, &do_f2fs_print_frag_info);

void f2fs_print_frag_info(void)
{
	schedule_work(&f2fs_print_frag_info_work);
}
#endif
