#ifndef __CVDR__CS_H__
#define __CVDR__CS_H__

enum ipp_vp_rd_id_e {
	IPP_VP_RD_CMDLST = 0,
	IPP_VP_RD_ORB_ENH_Y_HIST,
	IPP_VP_RD_ORB_ENH_Y_IMAGE,
	IPP_VP_RD_ARF_0,
	IPP_VP_RD_ARF_1,
	IPP_VP_RD_ARF_2,
	IPP_VP_RD_ARF_3,
	IPP_VP_RD_HIOF_CUR_Y, // 7
	IPP_VP_RD_HIOF_REF_Y, // 8
	IPP_VP_RD_RDR,
	IPP_VP_RD_CMP,
	IPP_VP_RD_GF_SRC_P,
	IPP_VP_RD_GF_GUI_I,
	IPP_VP_RD_MAX,
};

enum ipp_vp_wr_id_e {
	IPP_VP_WR_CMDLST = 0,
	IPP_VP_WR_ORB_ENH_Y,
	IPP_VP_WR_ARF_PRY_1,
	IPP_VP_WR_ARF_PRY_2,
	IPP_VP_WR_ARF_DOG_0,
	IPP_VP_WR_ARF_DOG_1,
	IPP_VP_WR_ARF_DOG_2,
	IPP_VP_WR_ARF_DOG_3,
	IPP_VP_WR_HIOF_SPARSE_MD,
	IPP_VP_WR_HIOF_DENSE_TNR,
	IPP_VP_WR_GF_LF_A,
	IPP_VP_WR_GF_HF_B,
	IPP_VP_WR_MAX,
};

enum ipp_nr_rd_id_e {
	IPP_NR_RD_CMP,
	IPP_NR_RD_MC,
	IPP_NR_RD_MAX,
};

enum ipp_nr_wr_id_e {
	IPP_NR_WR_RDR = 0,
	IPP_NR_WR_MAX,
};

extern unsigned int g_cvdr_vp_wr_id[IPP_VP_WR_MAX];
extern unsigned int g_cvdr_vp_rd_id[IPP_VP_RD_MAX];
extern unsigned int g_cvdr_nr_wr_id[IPP_NR_WR_MAX];
extern unsigned int g_cvdr_nr_rd_id[IPP_NR_RD_MAX];

int ippdev_lock(void);
int ippdev_unlock(void);

#define get_cvdr_vp_wr_port_num(x) g_cvdr_vp_wr_id[x]
#define get_cvdr_vp_rd_port_num(x) g_cvdr_vp_rd_id[x]
#define get_cvdr_nr_wr_port_num(x) g_cvdr_nr_wr_id[x]
#define get_cvdr_nr_rd_port_num(x) g_cvdr_nr_rd_id[x]
#endif /* __CVDR__CS_H__  */
