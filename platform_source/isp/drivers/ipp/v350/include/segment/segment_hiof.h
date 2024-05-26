#ifndef _HIOF_CS_H__
#define _HIOF_CS_H__

#include "ipp.h"
#include "cfg_table_hiof.h"

#define HIOF_MAX_IMG_W_SIZE (768)
#define HIOF_MAX_IMG_H_SIZE (432)
#define HIOF_MIN_IMG_W_SIZE (64)
#define HIOF_MIN_IMG_H_SIZE (36)
#define HIOF_MODE_MD (1)
#define HIOF_MODE_TNR (2)
#define HIOF_MODE_MD_TNR (3)
#define ALIGN_4_PIX (4)

enum hiof_buf_usage_e {
	BI_REF_Y = 0,
	BI_CUR_Y,
	BO_SPARSE_MV_CONFI, // for MD
	BO_DENSE_MV_CONFI, // for TNR
	HIOF_STREAM_MAX,
};

typedef struct _hiof_req_ctrl_cfg_t {
	unsigned int mode;
	unsigned int spena;
	unsigned int iter_num;
	unsigned int iter_check_en;
} hiof_req_ctrl_cfg_t;

typedef struct _hiof_req_size_cfg_t {
	unsigned int       width;
	unsigned int       height;
} hiof_req_size_cfg_t;

typedef struct _hiof_req_coeff_cfg_t {
	unsigned int coeff0;
	unsigned int coeff1;
} hiof_req_coeff_cfg_t;

typedef struct _hiof_req_threshold_cfg_t {
	unsigned int flat_thr;
	unsigned int ismotion_thr;
	unsigned int confilv_thr;
	unsigned int att_max;
} hiof_req_threshold_cfg_t;


typedef struct _req_hiof_cfg_t {
	hiof_req_ctrl_cfg_t    hiof_ctrl_cfg;
	hiof_req_size_cfg_t    hiof_size_cfg;
	hiof_req_coeff_cfg_t    hiof_coeff_cfg;
	hiof_req_threshold_cfg_t hiof_threshold_cfg;
} req_hiof_cfg_t;


struct msg_req_hiof_request_t {
	unsigned int frame_number;
	unsigned int hiof_rate_value;
	req_hiof_cfg_t hiof_cfg;
	struct ipp_stream_t streams[HIOF_STREAM_MAX];
};

int hiof_request_handler(struct msg_req_hiof_request_t *hiof_req);
#endif /* _HIOF_CS_H__ */
