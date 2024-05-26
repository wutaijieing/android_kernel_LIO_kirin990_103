/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: als para table rohm source file
 * Author: linjianpeng <linjianpeng1@huawei.com>
 * Create: 2020-05-25
 */

#include "als_para_table_rohm.h"

#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <securec.h>

#include "tp_color.h"
#include "contexthub_boot.h"
#include "contexthub_route.h"

/*
 * Although the GRAY and Black TP's RGB ink is same ,but some product may has both the GRAY
 * and Black TP,so must set the als para for  GRAY and Black TP
 * Although the CAFE_2 and BROWN TP's RGB ink is same ,but some product may has both the CAFE_2
 * and BROWN TP,so must set the als para for  CAFE_2 and BROWN TP
 */
static bh1745_als_para_table als_para_diff_tp_color_table[] = {
	{ OTHER, OTHER, DEFAULT_TPLCD, OTHER,
	{ 278, 247, 180, 1450, 1443, 278, 732, 2299, 100, -2489, 10630, -4085,
	 8811, 1349, 1734, 5421, 2020, 0, 0, 0, 0, 0, 0, 2000, 1000 } },
	{ ALPS, V1, DEFAULT_TPLCD, GOLD,
	{ 278, 247, 180, 1450, 1443, 278, 732, 2299, 100, -2489, 10630, -4085,
	 8811, 1349, 1734, 5421, 2020, 3624, 4206, 1331, 435, 3496, 8119, 6000, 150 } },
	{ ALPS, V1, DEFAULT_TPLCD, BLACK,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 6000, 150 } },
	{ ALPS, V1, DEFAULT_TPLCD, GRAY,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 6000, 150 } },
	{ ALPS, V1, DEFAULT_TPLCD, WHITE,
	{ 201, 63, 68, 306, 264, 201, 454, 1347, 100, -2316, 6708, -3320,
	 5631, 1048, 1662, 2476, 1956, 10342, 15975, 8151, 1556, 2769, 8115, 6000, 150 } },
	 { ALPS, V3, DEFAULT_TPLCD, GOLD,
	{ 227, 357, 307, 1938, 1831, 227, 698, 2044, 100, -2166, 9692, -3951,
	 8560, 1062, 1827, 4010, 1947, 3194, 3787, 1203, 385, 3496, 8119, 6000, 150 } },
	{ ALPS, V3, DEFAULT_TPLCD, WHITE,
	{ 202, 92, 109, 422, 355, 202, 455, 1269, 100, -2100, 6412, -3539,
	 5768, 907, 1742, 2433, 1883, 9740, 14171, 7196, 1442, 3769, 8115, 5000, 200 } },
	{ ALPS, V3, DEFAULT_TPLCD, BLACK,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 6000, 150 } },
	{ ALPS, V3, DEFAULT_TPLCD, GRAY,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 6000, 150 } },
	{ ALPS, V3_D, DEFAULT_TPLCD, GOLD,
	{ 169, 269, 332, 1211, 989, 169, 416, 1178, 100, -2082, 6273, -3328,
	 5220, 889, 1742, 2182, 1905, 2831, 4745, 2482, 445, 3253, 8191, 6000, 150 } },
	{ ALPS, V3, DEFAULT_TPLCD, BROWN,
	{ 247, 180, 168, 1305, 1175, 247, 609, 1959, 100, -2390, 8743, -3767,
	 7190, 1396, 1819, 4486, 2044, 4717, 4679, 1852, 532, 3467, 4521, 6000, 150 } },
	{ ALPS, V3_D, DEFAULT_TPLCD, BLACK,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 6000, 150 } },
	{ ALPS, V3_D, DEFAULT_TPLCD, GRAY,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 6000, 150 } },
	{ ALPS, V3_D, DEFAULT_TPLCD, BROWN,
	{ 247, 180, 168, 1305, 1175, 247, 609, 1959, 100, -2390, 8743, -3767,
	 7190, 1396, 1819, 4486, 2044, 4717, 4679, 1852, 532, 3467, 4521, 6000, 150 } },
	{ ALPS, VN1, DEFAULT_TPLCD, GOLD,
	{ 169, 269, 332, 1211, 989, 169, 416, 1178, 100, -2082, 6273, -3328,
	 5220, 889, 1742, 2182, 1905, 2831, 4745, 2482, 445, 3253, 8191, 5000, 200 } },
	{ ALPS, VN1, DEFAULT_TPLCD, BROWN,
	{ 247, 180, 168, 1305, 1175, 247, 609, 1959, 100, -2390, 8743, -3767,
	 7190, 1396, 1819, 4486, 2044, 4717, 4679, 1852, 532, 3467, 4521, 5000, 200 } },
	{ ALPS, VN1, DEFAULT_TPLCD, BLACK,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 5000, 200 } },
	{ ALPS, VN1, DEFAULT_TPLCD, BLACK2,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 5000, 200 } },
	{ ALPS, VN1, DEFAULT_TPLCD, GRAY,
	{ 272, 212, 205, 1188, 1009, 272, 631, 1899, 100, -2580, 9747, -4087,
	 7817, 1376, 1668, 4914, 1970, 4443, 5334, 1963, 553, 3870, 8506, 5000, 200 } },
	{ ALPS, VN1, DEFAULT_TPLCD, PINK_GOLD,
	{ 269, 53, 41, 356, 351, 269, 774, 2483, 100, -2532, 11739, -4033,
	 8865, 1514, 1786, 5431, 2045, 14636, 13294, 4475, 1652, 3171, 6991, 4000, 250 } },
	{ BLANC, V3, DEFAULT_TPLCD, PINK_GOLD,
	{ 247, 316, 265, 2630, 2328, 247, 465, 1635, 100, -2288, 6127, -3330,
	  5313, 1340, 1833, 3350, 2115, 2525, 2343, 1342, 296, 3171, 6991, 4000, 250 } },
	{ BLANC, V3, DEFAULT_TPLCD, BROWN,
	{ 239, 187, 147, 1030, 916, 239, 512, 1639, 100, -2272, 7173, -3419,
	 6110, 1105, 1734, 3254, 2026, 4971, 6059, 2992, 648, 3171, 6991, 4000, 250 } },
	{ BLANC, V3, DEFAULT_TPLCD, BLUE,
	{ 202, 439, 167, 3249, 1427, 202, 954, 4211, 100, -2389, 8069, -2132,
	 10123, 1398, 1811, 1930, 2288, 3192, 2048, 932, 405, 3171, 6991, 4000, 250 } },
	{ BLANC, V3, DEFAULT_TPLCD, GOLD,
	{ 160, 166, 155, 739, 676, 160, 420, 1259, 100, -2177, 6303, -3241,
	 5158, 926, 1693, 2153, 1963, 4733, 7681, 3809, 739, 3171, 6991, 4000, 250 } },
	{ BLANC, VN1, DEFAULT_TPLCD, BLUE,
	{ 405, 441, 166, 3348, 3505, 405, 1158, 3969, 100, -2220, 12642, -5075,
	 13359, 1314, 1953, 14578, 1863, 1878, 1630, 412, 201, 3171, 6991, 4000, 250 } },
	{ BLANC, VN1, DEFAULT_TPLCD, BLACK,
	{ 305, 227, 152, 1431, 1517, 305, 829, 2706, 100, -2119, 9795, -4237,
	 9731, 1111, 1896, 6029, 1798, 3611, 3802, 1202, 432, 3171, 6991, 4000, 250 } },
	{ BLANC, VN1, DEFAULT_TPLCD, BLACK2,
	{ 305, 227, 152, 1431, 1517, 305, 829, 2706, 100, -2119, 9795, -4237,
	 9731, 1111, 1896, 6330, 1888, 3611, 3802, 1202, 432, 3171, 6991, 4000, 250 } },
	{ BLANC, VN1, DEFAULT_TPLCD, GRAY,
	{ 305, 261, 159, 1643, 1579, 305, 829, 2706, 100, -2119, 9795, -4237,
	 9731, 1111, 1896, 6872, 2050, 3611, 3802, 1202, 432, 3171, 6991, 4000, 250 } },
	{ BLANC, VN1, DEFAULT_TPLCD, BROWN,
	{ 308, 138, 102, 1018, 993, 308, 714, 2464, 100, -2199, 8517, -3912,
	 8224, 1245, 1899, 5437, 1966, 5629, 5288, 1998, 627, 3171, 6991, 4000, 250 } },
	{ BLANC, VN1, DEFAULT_TPLCD, PINK_GOLD,
	{ 264, 65, 47, 489, 470, 264, 859, 2859, 100, -2421, 11443, -4167,
	 9788, 1489, 1866, 6699, 2092, 10810, 10131, 3162, 1186, 3171, 6991, 4000, 250 } },

	{ BKL, V3, DEFAULT_TPLCD, BLACK,
	{ 600, 1513, 119, 8644, 7443, 836, 1086, 3890, 100, -2470, 6320, -3633,
	8942, 1229, 1620, 6955, 2213, 839, 535, 343, 88, 2714, 5545, 30000, 0 } },
	{ BKL, V3, DEFAULT_TPLCD, WHITE,
	{ 202, 222, 112, 928, 837, 202, 927, 2330, 100, -2571, 9630, -2872, 8819,
	1144, 1582, 2352, 1972, 6284, 6431, 2317, 687, 3698, 6921, 30000, 0 } },
	{ BKL, V3, DEFAULT_TPLCD, BLUE,
	{ 360, 2538, 886, 2280, 1478, 360, 169, 304, 100, -6216, 4030, -2196,
	2228, 616, 458, 556, 1413, 525, 2802, 5825, 301, 3500, 6000, 30000, 0 } },
	{ NEO, V3, DEFAULT_TPLCD, GOLD,
	{ 405, 530, 196, 4019, 4133, 405, 1158, 3969, 100, -2220, 12642, -5075,
	 13359, 1314, 1953, 16379, 2093, 1878, 1630, 412, 201, 3171, 6991, 4000, 250 } },
	{ NEO, V3, DEFAULT_TPLCD, BLUE,
	{ 405, 530, 196, 4019, 4133, 405, 1158, 3969, 100, -2220, 12642, -5075,
	 13359, 1314, 1953, 16379, 2093, 1878, 1630, 412, 201, 3171, 6991, 4000, 250 } },
	{ NEO, V3, DEFAULT_TPLCD, BLACK,
	{ 305, 261, 159, 1643, 1579, 305, 829, 2706, 100, -2119, 9795, -4237,
	 9731, 1111, 1896, 6872, 2050, 5391, 3722, 1116, 524, 3761, 6000, 4000, 250 } },
	{ NEO, V3, DEFAULT_TPLCD, BLACK2,
	{ 305, 261, 159, 1643, 1579, 305, 829, 2706, 100, -2119, 9795, -4237,
	 9731, 1111, 1896, 6872, 2050, 5391, 3722, 1116, 524, 3761, 6000, 4000, 250 } },
	{ NEO, V3, DEFAULT_TPLCD, BROWN,
	{ 308, 170, 121, 1257, 1175, 308, 714, 2464, 100, -2199, 8517, -3912,
	 8224, 1245, 1899, 5784, 2091, 5629, 5288, 1998, 627, 3171, 6991, 4000, 250 } },
	{ NEO, V3, DEFAULT_TPLCD, PINK_GOLD,
	{ 247, 316, 265, 2630, 2328, 247, 465, 1635, 100, -2288, 6127, -3330,
	 5313, 1340, 1833, 3350, 2115, 2525, 2343, 1342, 296, 3171, 6991, 4000, 250 } },
	{ NEO, V3_A, DEFAULT_TPLCD, BLACK,
	{ 293, 205, 213, 1289, 1103, 293, 660, 1966, 100, -2249, 9038, -4130,
	 8013, 1195, 1830, 5021, 1979, 5026, 5231, 1846, 619, 3886, 6000, 4000, 250 } },
	{ NEO, V3_A, DEFAULT_TPLCD, BLACK2,
	{ 293, 205, 213, 1289, 1103, 293, 660, 1966, 100, -2249, 9038, -4130,
	 8013, 1195, 1830, 5021, 1979, 5026, 5231, 1846, 619, 3886, 6000, 4000, 250 } },
	{ NEO, V3_A, DEFAULT_TPLCD, GRAY,
	{ 293, 205, 213, 1289, 1103, 293, 660, 1966, 100, -2249, 9038, -4130,
	 8013, 1195, 1830, 5021, 1979, 5026, 5231, 1846, 619, 3886, 6000, 4000, 250 } },
	{ NEO, V3_A, DEFAULT_TPLCD, WHITE,
	{ 293, 205, 213, 1289, 1103, 293, 660, 1966, 100, -2249, 9038, -4130,
	 8013, 1195, 1830, 5021, 1979, 5026, 5231, 1846, 619, 3886, 6000, 4000, 250 } },
	{ NEO, V4, DEFAULT_TPLCD, WHITE,
	{ 345, 231, 266, 1472, 1295, 345, 678, 2340, 100, -2246, 8320, -3870,
	 7732, 1132, 1732, 4618, 1952, 3781, 4360, 1743, 477, 3213, 7902, 4000, 250 } },
	{ NEO, V4, DEFAULT_TPLCD, BLACK,
	{ 345, 231, 266, 1472, 1295, 345, 678, 2340, 100, -2246, 8320, -3870,
	 7732, 1132, 1732, 4618, 1952, 3781, 4360, 1743, 477, 3213, 7902, 4000, 250 } },
	{ NEO, V4, DEFAULT_TPLCD, BLACK2,
	{ 345, 231, 266, 1472, 1295, 345, 678, 2340, 100, -2246, 8320, -3870,
	 7732, 1132, 1732, 4618, 1952, 3781, 4360, 1743, 477, 3213, 7902, 4000, 250 } },
	{ NEO, V4, DEFAULT_TPLCD, GRAY,
	{ 345, 231, 266, 1472, 1295, 345, 678, 2340, 100, -2246, 8320, -3870,
	 7732, 1132, 1732, 4618, 1952, 3781, 4360, 1743, 477, 3213, 7902, 4000, 250 } },
};

static sy3133_als_para_table sy3133_als_para_diff_tp_color_table[] = {
	{ BAH4, V3, DEFAULT_TPLCD, WHITE,
	{ 100, 120, 9113, 192, 7994, 12255, 1414, 940, 0, 120, 9113, 192,
	  7994, 12255, 1414, 940, 0, 21709, 18803, 10135, 11112, 18803, 25, 2048, 4, 0,
	  0, 0, 0 } },

	{ BAH4, V3, DEFAULT_TPLCD, BLACK,
	{ 100, 120, 9113, 192, 7994, 21592, 1289, 2350, 0, 120, 9113, 192,
	  7994, 21592, 1289, 2350, 0, 13253, 20354, 3624, 6911, 0, 50, 2048, 5, 0,
	  0, 0, 0 } },
};

static bu27006_als_para_table bu27006_als_para_diff_tp_color_table[] = {
	{ BAH4, V3, DEFAULT_TPLCD, BLACK,
	{ 3000, 4782, 3494, 14639, -105, 9501, -8533, 16867, -3359, 17423, -4278, 4301, 27670, -5700, 11769,
	   -5611, 1324, -1281, -7174, 2962, 3329, 1200, 30, 4000, 6000, 10000, 100 } },
	{ BAH4, V3, DEFAULT_TPLCD, WHITE,
	{ 3000, -306, 4643, -4519, 267, 3876, -2662, 10936, -2336, 13190, -2792, 3016, 18537, 14589, -24944,
	   11185, -4078, 3560, 28982, 7708, 11965, 5784, 72, 4000, 6000, 10000, 100 } },
};

static tcs3707_als_para_table tcs3707_als_para_diff_tp_color_table[] = {
	{ BAH4, V3, DEFAULT_TPLCD, BLACK,
	{ 100, 850, 1135, -504, -837, 312, 0, 850, 1135, -504, -837, 312,
	  0, 0, 12447, 0, 0, 3130, 12477, 0, 0, 3130, 30, 8582, 5202, 2504,
	  1619, 10000, 100 } },
	{ BAH4, V3, DEFAULT_TPLCD, WHITE,
	{ 100, 850, 144, -50, 157, -111, 0, 850, 144, -50, 157, -111,
	  0, 0, 8175, 0, 0, 2604, 8175, 0, 0, 2604, 8693, 29110, 15121, 9271,
	  7243, 10000, 100 } },
	{ PRS, V3, DEFAULT_TPLCD, BLACK,
	{ 100, 849, 449, -573, 911, -647, 0, 850, -91, -274, 1695, -405,
	  0, 8448, 7987, 0, 0, 2454, -232, 0, 0, 6623, 8448, 15710, 8299, 5748,
	  3335, 10000, 100 } },
	{ PRS, V3, DEFAULT_TPLCD, WHITE,
	{ 100, 849, 449, -573, 911, -647, 0, 850, -91, -274, 1695, -405,
	  0, 8448, 7987, 0, 0, 2454, -232, 0, 0, 6623, 8448, 15710, 8299, 5748,
	  3335, 10000, 100 } },
	{ LDN, V3, DEFAULT_TPLCD, BLACK,
	{ 100, 850, -147, -797, 2692, -204, 0, 850, 112, 102, 63, 58,
	  0, 12800, 15900, 0, 0, -18, 164, 0, 0, 2839, 12800, 11042, 5532, 4203,
	  2620, 10000, 100 } },
	{ LDN, V3, DEFAULT_TPLCD, WHITE,
	{ 100, 850, -147, -797, 2692, -204, 0, 850, 112, 102, 63, 58,
	  0, 12800, 15900, 0, 0, -18, 164, 0, 0, 2839, 12800, 11042, 5532, 4203,
	  2620, 10000, 100 } },
};

static rpr531_als_para_table rpr531_als_para_diff_tp_color_table[] = {
	/* Here use PRA as an example */
	{ PRA, V4, TS_PANEL_UNKNOWN,
		{ 6897, 4300, 2780, 4716, 4208, 1985, 1053, 1992, 1168, 1629, 2062, 2367, 746, 104, 10000, 200 } },
	{ COL, V4, TS_PANEL_LENS,
		{ 7221, 4300, 3329, 3329, 4727, 1985, 1498, 1498, 900, 1400, 1629, 2367, 645, 94, 15000, 0 } },
	{ COL, V4, TS_PANEL_TRULY,
		{ 7222, 4300, 3329, 3329, 4727, 1985, 1498, 1498, 900, 1400, 1629, 2367, 645, 94, 15000, 0 } },
	{ COL, V4, TS_PANEL_OFILIM,
		{ 7223, 4300, 3329, 3329, 4727, 1985, 1498, 1498, 900, 1400, 1629, 2367, 645, 94, 15000, 0 } },
	{ COL, V4, TS_PANEL_UNKNOWN,
		{ 7224, 4300, 3329, 3329, 4727, 1985, 1498, 1498, 900, 1400, 1629, 2367, 645, 94, 15000, 0 } },

	{ COR, V4, TS_PANEL_UNKNOWN,
		{ 8696, 3949, 2765, 2765, 6067, 1877, 1115, 1115, 700, 1554, 2200, 2500, 732, 106, 15000, 100 } },

	{ PAR, V4, TS_PANEL_UNKNOWN,
		{ 16618, 10495, 3570, 2380, 9648, 5500, 1382, 921, 800, 1150, 1900, 2500, 469, 97, 15000, 100 } },

};

static bh1726_als_para_table bh1726_als_para_diff_tp_color_table[] = {
	{ HRY, V4, TS_PANEL_UNKNOWN,
	 { 881, 930, 515, 515, 436, 550, 144, 144, 768, 931, 3775, 3775, 2199, 299, 5000, 300 } },

	{ POT, V4, TS_PANEL_UNKNOWN,
	 { 496, 600, 296, 296, 275, 390, 85, 85, 694, 923, 3254, 3254, 4197, 563, 5000, 300 } },

	{ MAR, V4, TS_PANEL_UNKNOWN,
	 { 1094, 1070, 680, 680, 406, 383, 107, 107, 1014, 1413, 6345, 6345, 1764, 271, 7000, 100 } },

	{ MAR, VN1, BOE_TPLCD,
	{ 208, 164, 112, 112, 118, 66, 29, 29,
	900, 1700, 3862, 3862, 16256, 2301, 7000, 100 } },
	{ MAR, VN1, CMI_TPLCD,
	{ 333, 112, 58, 58, 293, 53, 15, 15,
	900, 1500, 3866, 3866, 14587, 1957, 7000, 100 } },
	{ MAR, VN1, JDI_TPLCD,
	{ 163, 107, 75, 75, 102, 43, 20, 20,
	900, 1600, 3750, 3750, 20864, 2880, 7000, 100 } },
	{ MAR, VN1, TM_TPLCD,
	{ 268, 273, 96, 96, 163, 177, 26, 26,
	900, 1100, 3161, 3161, 13502, 2135, 7000, 100 } },

	{ STK, V4, BOE_TPLCD,
	{ 790, 601, 515, 515, 511, 177, 103, 103,
	749, 1158, 5000, 5000, 2876, 386, 5000, 300 } },
	{ STK, V4, INX_TPLCD,
	{ 833, 557, 505, 505, 613, 194, 142, 142,
	660, 983, 3559, 3559, 2876, 386, 5000, 300 } },
	{ STK, V4, TCL_TPLCD,
	 { 800, 708, 526, 526, 495, 340, 125, 125,
		592, 845, 4207, 4207, 2876, 386, 5000, 300 } },
	{ STK, V4, TM_TPLCD,
	{ 800, 708, 526, 526, 495, 340, 125, 125,
		592, 845, 4207, 4207, 2876, 386, 5000, 300 } },
	{ STK, V4, TS_PANEL_UNKNOWN,
	{ 800, 708, 526, 526, 495, 340, 125, 125,
	592, 845, 4207, 4207, 2876, 386, 5000, 300 } },
	{ ASK, V4, BOE_TPLCD,
	{ 1192, 1394, 696, 696, 709, 974, 208, 208,
	760, 911, 3434, 3434, 1998, 327, 5000, 300 } },
	{ ASK, V4, TM_TPLCD,
	{ 1188, 1381, 675, 675, 688, 959, 191, 191,
	711, 919, 3503, 3503, 1998, 327, 5000, 300 } },
	{ ASK, V4, LG_TPLCD,
	{ 1181, 1322, 684, 684, 714, 960, 248, 248,
	500, 860, 2756, 2756, 1998, 327, 5000, 300 } },
	{ ASK, V4, INX_TPLCD, // LTPS TP
	{ 1084, 1016, 582, 582, 535, 571, 136, 136,
	728, 962, 4250, 10000, 1998, 327, 5000, 300 } },
	{ ASK, V4, INX_TPLCD2, // A-SI TP
	{ 1067, 1237, 591, 591, 471, 751, 138, 138,
	500, 1055, 4254, 10000, 1998, 327, 5000, 300 } },
	{ ASK, V4, TS_PANEL_UNKNOWN,
	{ 1188, 1381, 675, 675, 688, 959, 191, 191,
	711, 919, 3503, 3503, 1998, 327, 5000, 300 } },

	{ GLK, V3, TS_PANEL_UNKNOWN,
	{ 268, 273, 96, 96, 163, 177, 30, 30,
	900, 1120, 3161, 3161, 13502, 2135, 30000, 100 } },
};

bh1745_als_para_table *als_get_bh1745_table_by_id(uint32_t id)
{
	if (id >= ARRAY_SIZE(als_para_diff_tp_color_table))
		return NULL;
	return &(als_para_diff_tp_color_table[id]);
}

sy3133_als_para_table *als_get_sy3133_table_by_id(uint32_t id)
{
	if (id >= ARRAY_SIZE(sy3133_als_para_diff_tp_color_table))
		return NULL;
	return &(sy3133_als_para_diff_tp_color_table[id]);
}

bu27006_als_para_table *als_get_bu27006_table_by_id(uint32_t id)
{
	if (id >= ARRAY_SIZE(bu27006_als_para_diff_tp_color_table))
		return NULL;
	return &(bu27006_als_para_diff_tp_color_table[id]);
}

tcs3707_als_para_table *als_get_tcs3707_table_by_id(uint32_t id)
{
	if (id >= ARRAY_SIZE(tcs3707_als_para_diff_tp_color_table))
		return NULL;
	return &(tcs3707_als_para_diff_tp_color_table[id]);
}

bh1745_als_para_table *als_get_bh1745_first_table(void)
{
	return &(als_para_diff_tp_color_table[0]);
}

sy3133_als_para_table *als_get_sy3133_first_table(void)
{
	return &(sy3133_als_para_diff_tp_color_table[0]);
}

bu27006_als_para_table *als_get_bu27006_first_table(void)
{
	return &(bu27006_als_para_diff_tp_color_table[0]);
}

tcs3707_als_para_table *als_get_tcs3707_first_table(void)
{
	return &(tcs3707_als_para_diff_tp_color_table[0]);
}

uint32_t als_get_bh1745_table_count(void)
{
	return ARRAY_SIZE(als_para_diff_tp_color_table);
}

uint32_t als_get_sy3133_table_count(void)
{
	return ARRAY_SIZE(sy3133_als_para_diff_tp_color_table);
}

uint32_t als_get_bu27006_table_count(void)
{
	return ARRAY_SIZE(bu27006_als_para_diff_tp_color_table);
}

uint32_t als_get_tcs3707_table_count(void)
{
	return ARRAY_SIZE(tcs3707_als_para_diff_tp_color_table);
}

rpr531_als_para_table *als_get_rpr531_table_by_id(uint32_t id)
{
	if (id >= ARRAY_SIZE(rpr531_als_para_diff_tp_color_table))
		return NULL;
	return &(rpr531_als_para_diff_tp_color_table[id]);
}

rpr531_als_para_table *als_get_rpr531_first_table(void)
{
	return &(rpr531_als_para_diff_tp_color_table[0]);
}

uint32_t als_get_rpr531_table_count(void)
{
	return ARRAY_SIZE(rpr531_als_para_diff_tp_color_table);
}

bh1726_als_para_table *als_get_bh1726_table_by_id(uint32_t id)
{
	if (id >= ARRAY_SIZE(bh1726_als_para_diff_tp_color_table))
		return NULL;
	return &(bh1726_als_para_diff_tp_color_table[id]);
}

bh1726_als_para_table *als_get_bh1726_first_table(void)
{
	return &(bh1726_als_para_diff_tp_color_table[0]);
}

uint32_t als_get_bh1726_table_count(void)
{
	return ARRAY_SIZE(bh1726_als_para_diff_tp_color_table);
}