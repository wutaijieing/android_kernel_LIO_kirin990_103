
#ifndef _HEALTH_GRADE_H_
#define _HEALTH_GRADE_H_

#include <linux/types.h>

#define HEALTH_GRADE_PROFILE_MAX    20

#define HEALTH_GRADE_CPU_NVE_NUM    255
#define HEALTH_GRADE_CPU_NVE_NAME   "HG-CPU"
#define HEALTH_GRADE_OTHER_NVE_NUM  256
#define HEALTH_GRADE_OTHER_NVE_NAME "HG-OTH"

#define HEALTH_INFO_VOL_OFFSET_MAX  64

#define HEALTH_INFO_INPUT_MAX       20
#define HEALTH_GRADE_INPUT_MAX      10

#define HEALTH_INFO_TAG    0
#define HEALTH_GRADE_TAG   1

/*
 * HEALTH INFO FORMAT
 * bit[7:7] 1:valid, 0:invalid
 * bit[6:6] 1:all test passed, 0:fail happened
 * bit[5:0] voltage offset passed
 */
#define HEALTH_INFO_NVEDATA_VOL_START     0
#define HEALTH_INFO_NVEDATA_VOL_MASK      0x3F
#define HEALTH_INFO_NVEDATA_PASS_START    6
#define HEALTH_INFO_NVEDATA_PASS_MASK     0x1
#define HEALTH_INFO_NVEDATA_VALID_START   7
#define HEALTH_INFO_NVEDATA_VALID_MASK    0x1
#define health_info_pack(valid, vol, pass) \
	(((valid & HEALTH_INFO_NVEDATA_VALID_MASK) << HEALTH_INFO_NVEDATA_VALID_START) | \
	((pass & HEALTH_INFO_NVEDATA_PASS_MASK) << HEALTH_INFO_NVEDATA_PASS_START) | \
	((vol & HEALTH_INFO_NVEDATA_VOL_MASK) << HEALTH_INFO_NVEDATA_VOL_START))

/*
 * HEALTH GRADE FORMAT
 * bit[7:7] 1:valid, 0:invalid
 * bit[6:3] reserved
 * bit[2:0] grade(A:0x3 B:0x2 C:0x1 D(abnormal):0x0)
 */
#define HEALTH_GRADE_NVEDATA_GRADE_START  0
#define HEALTH_GRADE_NVEDATA_GRADE_MASK   0x7
#define HEALTH_GRADE_NVEDATA_VALID_START  7
#define HEALTH_GRADE_NVEDATA_VALID_MASK   0x1
#define health_grade_pack(valid, grade) \
	(((valid & HEALTH_GRADE_NVEDATA_VALID_MASK) << HEALTH_GRADE_NVEDATA_VALID_START) | \
	((grade & HEALTH_GRADE_NVEDATA_GRADE_MASK) << HEALTH_GRADE_NVEDATA_GRADE_START))

enum health_grade_core {
	HG_LITTLE_CORE = 0,
	HG_MIDDLE_CORE = 1,
	HG_BIG_CORE = 2,
	HG_L3 = 3,
	HG_TYPE_MAX,
};

enum health_grade_level {
	HG_GRADE_A = 0,
	HG_GRADE_B = 1,
	HG_GRADE_C = 2,
	HG_GRADE_D = 3,
	HG_GRADE_MAX,
};

struct health_grade_nve_data {
	u8 info[HG_TYPE_MAX][HEALTH_GRADE_PROFILE_MAX];
	u8 grade[HG_TYPE_MAX];
};

#endif /* _HEALTH_GRADE_H_ */
