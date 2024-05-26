#ifndef __CVDR__CS_H__
#define __CVDR__CS_H__

enum ipp_vp_rd_id_e {
	IPP_VP_RD_CMDLST = 0,
	IPP_VP_RD_ORB_ENH_Y_HIST,
	IPP_VP_RD_ORB_ENH_Y_IMAGE,
	IPP_VP_RD_ORB,
	IPP_VP_RD_RDR,
	IPP_VP_RD_CMP,
	IPP_VP_RD_VBK_SRC_Y,
	IPP_VP_RD_VBK_SRC_UV,
	IPP_VP_RD_VBK_SRC_Y_DS16,
	IPP_VP_RD_VBK_SRC_UV_DS16,
	IPP_VP_RD_VBK_SIGMA,
	IPP_VP_RD_VBK_MASK_TF,
	IPP_VP_RD_GF_SRC_P,
	IPP_VP_RD_GF_GUI_I,
	IPP_VP_RD_ANF_CURR_I0,
	IPP_VP_RD_ANF_CURR_Q0,
	IPP_VP_RD_ANF_PREV_I0,
	IPP_VP_RD_ANF_PREV_Q0,
	IPP_VP_RD_ANF_CURR_I1,
	IPP_VP_RD_ANF_CURR_Q1,
	IPP_VP_RD_ANF_PREV_I1,
	IPP_VP_RD_ANF_PREV_Q1,
	IPP_VP_RD_MAX,
};

enum ipp_vp_wr_id_e {
	IPP_VP_WR_CMDLST = 0,
	IPP_VP_WR_ORB_ENH_Y,
	IPP_VP_WR_SLAM_Y,
	IPP_VP_WR_ORB_STAT,
	IPP_VP_WR_VBK_Y,
	IPP_VP_WR_VBK_UV,
	IPP_VP_WR_VBK_MASK_TF,
	IPP_VP_WR_GF_LF_A,
	IPP_VP_WR_GF_HF_B,
	IPP_VP_WR_ANF_Q0,
	IPP_VP_WR_ANF_I0,
	IPP_VP_WR_ANF_Q1,
	IPP_VP_WR_ANF_I1,
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
#endif/*__CVDR__CS_H__ */
