/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __DRV_NV_DEF_H__
#define __DRV_NV_DEF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "nv_id_drv.h"

#ifndef __KERNEL__

#ifndef __u8_defined
#define __u8_defined
typedef signed char          s8;//lint !e761
typedef unsigned char        u8;//lint !e761
typedef signed short         s16;//lint !e761
typedef unsigned short       u16;//lint !e761
typedef signed int           s32;//lint !e761
typedef unsigned int         u32;//lint !e761
typedef signed long long     s64;//lint !e761
typedef unsigned long long   u64;//lint !e761
#endif

#endif
/*lint --e{959}*/
/*
 * ½á¹¹Ãû    : nv_protocol_base_type
 * ½á¹¹ËµÃ÷  : nv_protocol_base_type½á¹¹ ID= en_NV_Item_Modem_Log_Path 148
            MBBĞÎÌ¬ modem logÂ·¾¶Ãû£¬ÓÃÓÚÇø·ÖĞÂÀÏĞÎÌ¬µÄmodem logÂ·¾¶¡£
 */
/*NV ID = 50018*/
#pragma pack(push)
#pragma pack(1)
/* ½á¹¹ËµÃ÷  : Èí¼ş°æ±¾ºÅ¡£ */
    typedef struct
    {
        s32        nvStatus;  /* µ±Ç°NVÊÇ·ñÓĞĞ§¡£ */
        s8         nv_version_info[30];  /* Èí¼ş°æ±¾ºÅĞÅÏ¢¡£ */
    }NV_SW_VER_STRU;
#pragma pack(pop)

/* NV ID  = 0xd10b */
/* ½á¹¹ËµÃ÷  : ¿ØÖÆhotplugÌØĞÔ¿ª¹Ø£¬µÚÒ»¸ö32bitµÄºó±ßreservedµÄ8¸öbitÖĞ£¬Ê¹ÓÃÒ»¸öbit£¬Ê¹ÓÃºóreservedµÄbit¼õÉÙÎª7¡£ */
typedef struct ST_PWC_SWITCH_STRU_S {

    /*ÒÔÏÂNVÓÃÓÚµÍ¹¦ºÄµÄÕûÌå¿ØÖÆ£¬ÆäÖĞÓĞĞ©BITÔİÊ±Î´ÓÃ£¬×öËûÓÃÊ±£¬Çë¸üÕıÎª×¼È·µÄÃû³Æ*/
    u32 deepsleep  :1; /* bit0 */
    u32 lightsleep :1; /* bit1 */
    u32 dfs        :1; /* bit2 */
    u32 hifi       :1; /* bit3 */
    u32 drxAbb     :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxZspCore :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxZspPll  :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxWLBbpPll  :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxGBbpPll   :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxRf      :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxPa      :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxGuBbpPd   :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxDspPd     :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxLBbpPd    :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxPmuEco    :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxPeriPd    :1; /* Î´Ê¹ÓÃ¡£ */
    u32 l2cache_mntn  :1; /* Î´Ê¹ÓÃ¡£ */
    u32 bugChk     :1; /* Î´Ê¹ÓÃ¡£ */
    u32 pmuSwitch     :1; /* Î´Ê¹ÓÃ¡£ */
    u32 drxLdsp      :1;  /* Î´Ê¹ÓÃ¡£ */
    u32 matserTDSpd  :1; /* Î´Ê¹ÓÃ¡£ */
    u32 tdsClk    :1;  /* Î´Ê¹ÓÃ¡£ */
    u32 slaveTDSpd   :1; /* Î´Ê¹ÓÃ¡£ */
    u32 slow         :1;/*bit23*/
    u32 ccpu_hotplug_suspend      :1;/*bit24 ´Ë·½°¸°ÎºËÊ±£¬±»°Î³öµÄcpu×ösuspend²Ù×÷£¬ºÄÊ±½Ï³¤*/
    u32 ccpu_hotplug_crg      :1;  /* CcpuÈÈ²å°Î·½°¸2¿ª¹Ø.°ÎµôcpuÊ±£¬±»°Î³öµÄcpu½ö×ö¹ØÖÓºÍË¢cache²Ù×÷£¬½Ï½ÚÊ¡Ê±¼ä¡£µ«ÊÇµÍ¹¦ºÄË¯ÃßÊ±ĞèÒªÏÈ°Ñ±»°Î³öµÄcpu²åÈë */
    u32 ccpu_tickless          :1;  /* CcpuµÄtickless¿ª¹Ø¡£ */
    u32 rsv  :4;
    u32 ddrdfs    :1; /* Ddrdfs¿ØÖÆ¿ª¹Ø¡£ */

    /*ÒÔÏÂNVÓÃÓÚDEBUGÉÏÏÂµçºÍ¿ª¹ØÖÓ*/
    u32 drx_pa_pd        :1; /* bit0 ÓÃÓÚ¿ØÖÆPAµÄÉÏÏÂµç*/
    u32 drx_rfic_pd      :1; /* bit1 ÓÃÓÚ¿ØÖÆRFICµÄÉÏÏÂµç*/
    u32 drx_rfclk_pd     :1; /* bit2 ÓÃÓÚ¿ØÖÆRFIC CLKµÄÉÏÏÂµç*/
    u32 drx_fem_pd       :1; /* bit3 ÓÃÓÚ¿ØÖÆFEMµÄÉÏÏÂµç*/
    u32 drx_cbbe16_pd    :1; /* bit4 ÓÃÓÚ¿ØÖÆCBBE16µÄÉÏÏÂµç*/
    u32 drx_bbe16_pd     :1; /* bit5 ÓÃÓÚ¿ØÖÆBBE16µÄÉÏÏÂµç*/
    u32 drx_abb_pd       :1; /* bit6 ÓÃÓÚ¿ØÖÆABBµÄÉÏÏÂµç*/
    u32 drx_bbp_init     :1; /* bit7 ÓÃÓÚ¿ØÖÆBBPÄ¬ÈÏ½«ÖÓµçÈ«²¿¿ªÆô*/
    u32 drx_bbppwr_pd    :1; /* bit8 ÓÃÓÚ¿ØÖÆBBPµçÔ´ãĞµÄÉÏÏÂµç*/
    u32 drx_bbpclk_pd    :1; /* bit9 ÓÃÓÚ¿ØÖÆBBPÊ±ÖÓãĞµÄ¿ª¹ØÖÓ*/
    u32 drx_bbp_pll      :1; /* bit10 ÓÃÓÚ¿ØÖÆBBP_PLLµÄ¿ª¹ØÖÓ*/
    u32 drx_cbbe16_pll   :1; /* bit11 ÓÃÓÚ¿ØÖÆCBBE16_PLLµÄ¿ª¹ØÖÓ*/
    u32 drx_bbe16_pll    :1; /* bit12 ÓÃÓÚ¿ØÖÆBBE16_PLLµÄ¿ª¹ØÖÓ*/
    u32 drx_bbp_reserved :1; /* bit13 bbpÔ¤Áô*/
    u32 drx_abb_gpll     :1; /* bit14 ÓÃÓÚ¿ØÖÆABB_GPLLµÄ¿ª¹ØÖÓ*/
    u32 drx_abb_scpll    :1; /* bit15 ÓÃÓÚ¿ØÖÆABB_SCPLLµÄ¿ª¹ØÖÓ*/
    u32 drx_abb_reserved1:1; /* abbÔ¤Áô£¬Î´Ê¹ÓÃ¡£ */
    u32 drx_abb_reserved2:1; /* abbÔ¤Áô£¬Î´Ê¹ÓÃ¡£ */
    u32 reserved2        :14; /* bit18-31 Î´ÓÃ*/
}ST_PWC_SWITCH_STRU;

/* ½á¹¹ËµÃ÷  : ¸ÃNVÊµÏÖ¶ÔnrµÍ¹¦ºÄÌØĞÔµÄ¿ª¹Ø¿ØÖÆ¡£ */
typedef struct ST_PWC_NR_POWERCTRL_STRU_S {

    u8 nrcpudeepsleep;  /* ¿ØÖÆNR R8 CPUµÍ¹¦ºÄÌØĞÔ¿ª¹Ø */
    u8 l2hacdeepsleep;  /* ¿ØÖÆL2HAC R8 CPUµÍ¹¦ºÄÌØĞÔ¿ª¹Ø */
    u8 cpubusdfs;  /* µ÷Æµµ÷Ñ¹¿ª¹Ø */
    u8 hotplug;  /* CPU ÈÈ²å°Î£¬µ±Ç°ÌØĞÔ²»ÓÃ´ò¿ª */
    u8 tickless;  /* Tickless£¬µ±Ç°ÌØĞÔ²»´ò¿ª */
    u8 drxnrbbpinit;  /* nrbbp»ØÆ¬³õÊ¼»¯´ò×® */
    u8 dxrbbpaxidfs;  /* nrbbp×Ô¶¯½µÆµ´ò×® */
    u8 drxnrbbpclk;  /* ±£Áô */
    u8 drxnrbbppll;  /* bbppll¿ª¹Ø´ò×® */
    u8 drxnrbbppmu;  /* nrbbpÉÏÏÂµç´ò×® */
    u8 drxbbainit;  /* ±£Áô */
    u8 drxbbapwr;  /* bbaÉÏµç´ò×® */
    u8 drxbbaclk;  /* ±£Áô */
    u8 drxbbapll;  /* ±£Áô */
    u8 drxbbapmu;  /* ±£Áô */
    u8 drxl1c;  /* ±£Áô */
    u8 drxl1cpll;  /* ±£Áô */
    u8 reserved;  /* ±£Áô */
}ST_PWC_NR_POWERCTRL_STRU;

/* ½á¹¹ËµÃ÷£ºDDRÎÂ±£NV */
typedef struct ST_DDR_TMON_PROTECT_STRU_S{
	u32 ddr_tmon_protect_on :1; /* DDRÎÂ±£¿ª¹Ø£¬1:¿ª£¬0:¹Ø */
	u32 ddr_vote_dvfs_alone  :1; /* DDRµ÷ÆµÊÇ·ñÊÜÎÂ±£Ó°Ïì£¬1:ÎÂ±£²»Ó°Ïì£¬0:µ÷Æµ¿¼ÂÇÎÂ±£ÒòËØ */
	u32 reserved1 : 6;
	u32 ddr_tmon_protect_enter_threshold :3; //ÎÂ±£½øÈëµÄthreshold
	u32 ddr_tmon_protect_exit_threshold :3; //ÎÂ±£ÍË³öthreshold
	u32 ddr_tmon_protect_freq_threshold:2;//½øÈëÎÂ±£ºóDDRµÄ×î´ó¹¤×÷Æµµã
	u32 ddr_tmon_protect_upper:3; //ddr ¸ßÎÂ¸´Î»ÃÅÏŞ
	u32 ddr_tmon_protect_downer:3; //ddrµÍÎÂ¸´Î»ÃÅÏŞ
	u32 reserved2:10;
}ST_DDR_TMON_PROTECT_STRU;

/* ½á¹¹ËµÃ÷£ºDRV PM CHRÉÏ±¨ÌØĞÔ¿ª¹ØNV */
typedef struct ST_CHR_REPORT_STRU_S {
    /*
     * 0£ºDRV PM CHRÉÏ±¨ÌØĞÔ¹Ø±Õ£»
     * ÆäËû£º²»Ë¯ÃßÉÏ±¨Òì³£ãĞÖµ£¬µ¥Î»Îª·ÖÖÓ¡£
     */
    u32 pm_monitor_time; /*pm monitor time,Unit:minute,1 means if cp not sleep ,CHR will report the pm state*/
    u32 reserved1;
    u32 reserved2;
    u32 reserved3;
}ST_CHR_REPORT_STRU;

/* NV ID = 0xd10c */
/* ½á¹¹ËµÃ÷  : ÓÃÓÚµÍ¹¦ºÄ¶¯Ì¬µ÷Æµ¡£ */
typedef struct ST_PWC_DFS_STRU_S {
    u32 CcpuUpLimit;  /* CºË¸ºÔØÉÏµ÷ÏŞÖµ£¬Ä¬ÈÏ80 £¬³¬¹ıËüĞèÒª°ÑÆµÂÊÉÏµ÷¡£ */
    u32 CcpuDownLimit;  /* CºË¸ºÔØÏÂµ÷ÏŞÖµ£¬Ä¬ÈÏ55 £¬µÍÓÚËüĞèÒª°ÑÆµÂÊÏÂµ÷¡£ */
    u32 CcpuUpNum;  /* CºËÁ¬Ğø¸ßÓÚ¸ºÔØÉÏÏŞ´ÎÊı£¬Ä¬ÈÏ1,³¬¹ıËüĞèÒª°ÑÆµÂÊÉÏµ÷¡£ */
    u32 CcpuDownNum;  /* CºËÁ¬ĞøµÍÓÚ¸ºÔØÏÂÏŞ´ÎÊı£¬Ä¬ÈÏ3,³¬¹ıËüĞèÒª°ÑÆµÂÊÏÂµ÷¡£ */
    u32 AcpuUpLimit;  /* AºË¸ºÔØÉÏµ÷ÏŞÖµ£¬Ä¬ÈÏ80 £¬³¬¹ıËüĞèÒª°ÑÆµÂÊÉÏµ÷¡£ */
    u32 AcpuDownLimit;  /* AºË¸ºÔØÏÂµ÷ÏŞÖµ£¬Ä¬ÈÏ55 £¬³¬¹ıËüĞèÒª°ÑÆµÂÊÏÂµ÷¡£ */
    u32 AcpuUpNum;  /* AºËÁ¬Ğø¸ßÓÚ¸ºÔØÉÏÏŞ´ÎÊı£¬Ä¬ÈÏ1£¬³¬¹ıËüĞèÒª°ÑÆµÂÊÉÏµ÷¡£ */
    u32 AcpuDownNum;  /* AºËÁ¬ĞøµÍÓÚ¸ºÔØÏÂÏŞ´ÎÊı£¬Ä¬ÈÏ3£¬µÍÓÚËüĞèÒª°ÑÆµÂÊÏÂµ÷¡£ */
    u32 DFSTimerLen;  /* ¶¨Ê±²éÑ¯¸ºÔØÊ±¼ä£¬Ä¬ÈÏ400 ¸ötick£¬1tick=10ms¡£*/
    u32 DFSHifiLoad; /* ±£Áô */
    /*
     * DfsÊ¹ÓÃÄÄÖÖ²ßÂÔ£¬½öÔÚPhoneĞÎÌ¬Ê¹ÓÃ¡£
     * bit0:1Ê¹ÓÃ200msµ÷Æµ¼ì²â²ßÂÔ£»bit0:0Ê¹ÓÃ4sµ÷Æµ¼ì²â²ßÂÔ£»
     * bit1:0£º¹Ø±ÕDDRµ÷Æµ£»1£º´ò¿ªDDRµ÷Æµ¡£
     */
    u32 Strategy;
    u32 DFSDdrUpLimit;  /* DDRµ÷Æµ¸ºÔØÉÏÏŞ£¬½öÔÚPhoneĞÎÌ¬Ê¹ÓÃ¡£ */
    u32 DFSDdrDownLimit;  /* DDRµ÷Æµ¸ºÔØÏÂÏŞ£¬½öÔÚPhoneĞÎÌ¬Ê¹ÓÃ¡£ */
    u32 DFSDdrprofile;  /* DDRµ÷Æµ×î´óprofileÖµ£¬½öÔÚPhoneĞÎÌ¬Ê¹ÓÃ¡£ */
    u32 reserved;  /* ±£ÁôÀ©Õ¹NV£¬ÏÖÓÃÓÚ±íÊ¾Ë¯Ãß»½ĞÑºóµÄÉèÖÃµÄprofileÖµ£¬½öÔÚMBBÏà¹Ø²úÆ·Ê¹ÓÃ¡£ */
}ST_PWC_DFS_STRU;

/*ID=0xd111 begin */
typedef struct
{
    /*
     * DumpÄ£Ê½¡£
     * 00: excdump£»
     * 01: usbdump£»
     * 1x: no dump¡£
     */
    u32 dump_switch    : 2; 
    u32 ARMexc         : 1; /* ARMÒì³£¼ì²â¿ª¹Ø¡£ */
    u32 stackFlow      : 1; /* ¶ÑÕ»Òç³ö¼ì²â¿ª¹Ø¡£ */
    u32 taskSwitch     : 1; /* ÈÎÎñÇĞ»»¼ÇÂ¼¿ª¹Ø¡£ */
    u32 intSwitch      : 1; /* ÖĞ¶Ï¼ÇÂ¼¿ª¹Ø¡£ */
    u32 intLock        : 1; /* ËøÖĞ¶Ï¼ÇÂ¼¿ª¹Ø¡£ */
    u32 appRegSave1    : 1; /* ACORE¼Ä´æÆ÷×é1¼ÇÂ¼¿ª¹Ø¡£ */
    u32 appRegSave2    : 1; /* ACORE¼Ä´æÆ÷×é2¼ÇÂ¼¿ª¹Ø¡£ */
    u32 appRegSave3    : 1; /* ACORE¼Ä´æÆ÷×é3¼ÇÂ¼¿ª¹Ø¡£ */
    u32 commRegSave1   : 1; /* CCORE¼Ä´æÆ÷×é1¼ÇÂ¼¿ª¹Ø¡£ */
    u32 commRegSave2   : 1; /* CCORE¼Ä´æÆ÷×é2¼ÇÂ¼¿ª¹Ø¡£ */
    u32 commRegSave3   : 1; /* CCORE¼Ä´æÆ÷×é3¼ÇÂ¼¿ª¹Ø¡£ */
    u32 sysErrReboot   : 1; /* SystemError¸´Î»¿ª¹Ø¡£ */
    u32 reset_log      : 1; /* Ç¿ÖÆ¼ÇÂ¼¿ª¹Ø£¬ÔİÎ´Ê¹ÓÃ¡£ */
    u32 fetal_err      : 1; /* Ç¿ÖÆ¼ÇÂ¼¿ª¹Ø£¬ÔİÎ´Ê¹ÓÃ¡£ */
    u32 log_ctrl       : 2; /* log¿ª¹Ø¡£ */
    u32 dumpTextClip   : 1; /* ddr±£´æÊ±text¶Î²Ã¼ôÌØĞÔ*/
    u32 secDump        : 1;
    u32 mccnt : 1;
    u32 pccnt : 1;
    u32 rrt : 1;
    u32 reserved1      : 9;  /* ±£Áô¡£ */
} DUMP_CFG_STRU;

/*
 * ½á¹¹Ãû    : NV_DCXO_C1C2_CAL_RESULT_STRU
 * ½á¹¹ËµÃ÷  : NVID DCXO C1C2Ğ£×¼½á¹û
 */
typedef struct
{
    u16 shwC2fix;  /* Ï¸µ÷µçÈİÖµ£¬0-255ÓĞĞ§£¬Ô¼0.3ppm/Âë×Ö */
    u16 shwC1fix;  /* ´Öµ÷µçÈİÖµ£¬0-15ÓĞĞ§£¬Ô¼3ppm/Âë×Ö */
}NV_FAC_DCXO_C1C2_CAL_RESULT_STRU;

/* ¿¿¿¿  : ¿¿¿¿¿¿¿¿¿¿¿ */
typedef struct {
    u32 thread0_channel : 1;
    u32 thread1_channel : 1;
    u32 thread2_channel : 1;
    u32 thread3_channel : 1;
    u32 thread4_channel : 1;
    u32 thread5_channel : 1;
    u32 thread6_channel : 1;
    u32 thread7_channel : 1;
    u32 thread8_channel : 1;
    u32 thread9_channel : 1;
    u32 thread10_channel : 1;
    u32 thread11_channel : 1;
    u32 thread12_channel : 1;
    u32 thread13_channel : 1;
    u32 thread14_channel : 1;
    u32 thread15_channel : 1;
    u32 reserved : 16; /* ¿¿¿ */
} DUMP_SCHDULE_CTRL_STRU;

/* ½á¹¹ËµÃ÷  : ÓÃÀ´¿ØÖÆ¿ÉÎ¬¿É²â¹¦ÄÜ¡£ */
typedef struct
{
    union
    {
        u32         uintValue;
        DUMP_CFG_STRU   Bits;
    } dump_cfg;
    union {
        u32 thread_cfg;
        DUMP_SCHDULE_CTRL_STRU thread_bits;
    } schudule_cfg;

    u32 appRegAddr1;    /* ACORE±£´æ¼Ä´æÆ÷×éµØÖ·1*/
    u32 appRegSize1;    /* ACORE±£´æ¼Ä´æÆ÷×é³¤¶È1*/
    u32 appRegAddr2;    /* ACORE±£´æ¼Ä´æÆ÷×éµØÖ·2*/
    u32 appRegSize2;    /* ACORE±£´æ¼Ä´æÆ÷×é³¤¶È2*/
    u32 appRegAddr3;    /* ACORE±£´æ¼Ä´æÆ÷×éµØÖ·3*/
    u32 appRegSize3;    /* ACORE±£´æ¼Ä´æÆ÷×é³¤¶È3*/

    u32 commRegAddr1;   /* CCORE±£´æ¼Ä´æÆ÷×éµØÖ·1*/
    u32 commRegSize1;   /* CCORE±£´æ¼Ä´æÆ÷×é³¤¶È1*/
    u32 commRegAddr2;   /* CCORE±£´æ¼Ä´æÆ÷×éµØÖ·2*/
    u32 commRegSize2;   /* CCORE±£´æ¼Ä´æÆ÷×é³¤¶È2*/
    u32 commRegAddr3;   /* CCORE±£´æ¼Ä´æÆ÷×éµØÖ·3*/
    u32 commRegSize3;   /* CCORE±£´æ¼Ä´æÆ÷×é³¤¶È3*/

    /*
     * Trace²É¼¯¿ª¹Ø¡£
     * 1:¿ª»úÆô¶¯Trace£»
     * ·Ç1:¿ª»ú²»Æô¶¯Trace¡£
     */
    u32 traceOnstartFlag;
    /*
     * Trace²É¼¯ÅäÖÃ¡£
     * bit1£º²É¼¯appºË£»
     * bit2£º²É¼¯modemºË¡£
     */
    u32 traceCoreSet;               
    /*
     * ·À×ÜÏß¹ÒËÄ¹¦ÄÜ¿ª¹Ø¡£
     * 0£º¿ª»ú²»Æô¶¯·À×ÜÏß¹ÒËÀ¹¦ÄÜ£»
     * ·Ç0£º¿ª»úÆô¶¯·À×ÜÏß¹ÒËÀ¹¦ÄÜ¡£
     */
    u32 busErrFlagSet;
} NV_DUMP_STRU;
/*ID=0xd111 end */

/*NV ID = 0xd115 start*/
/* ½á¹¹ËµÃ÷  : ¿ØÖÆÓ²¼ş°æ±¾ºÅ¡£ */
typedef struct {
    u32 index;           /*Ó²¼ş°æ±¾ºÅÊıÖµ(´ó°æ±¾ºÅ1+´ó°æ±¾ºÅ2)£¬Çø·Ö²»Í¬²úÆ·*/
    u32 hwIdSub;        /*Ó²¼ş×Ó°æ±¾ºÅ£¬Çø·Ö²úÆ·µÄ²»Í¬µÄ°æ±¾*/
    char  name[32];           /*ÄÚ²¿²úÆ·Ãû*/
    char    namePlus[32];       /*ÄÚ²¿²úÆ·ÃûPLUS*/
    char    hwVer[32];          /*Ó²¼ş°æ±¾Ãû³Æ*/
    char    dloadId[32];        /*Éı¼¶ÖĞÊ¹ÓÃµÄÃû³Æ*/
    char    productId[32];      /*Íâ²¿²úÆ·Ãû*/
}PRODUCT_INFO_NV_STRU;

/* product support module nv define */
/* ½á¹¹ËµÃ÷  : ÓÃÀ´±êÊ¶²úÆ·°å¸÷Ó²¼şÄ£¿éÊÇ·ñÖ§³Ö£¬1£ºÖ§³Ö£¬0²»Ö§³Ö¡£
 * ¸÷²úÆ·ĞÎÌ¬¿ÉÒÔÔÚ¸Ã²úÆ·µÄNV DIFFÎÄ¼şÖĞ£¬¸ù¾İÓ²¼şIDºÅÀ´¶¨ÖÆÊÇ·ñÖ§³ÖÄ³¸öÄ£¿é£¬Ò²¿É¾İ´ËÀ©Õ¹¶ÔÓ¦²úÆ·ĞÎÌ¬ÌØÓĞµÄÆäËûÄ£¿é¡£
 */
typedef struct
{
    /*
     * ÊÇ·ñÖ§³ÖSD¿¨¡£
     * 1£ºÖ§³Ö£»0£º²»Ö§³Ö¡£
     */
    u32 sdcard      : 1;
    u32 charge      : 1;  /* ÊÇ·ñÖ§³Ö³äµç¡£ */
    u32 wifi        : 1;  /* ÊÇ·ñÖ§³ÖWi-Fi¡£ */
    u32 oled        : 1;  /* ÊÇ·ñÖ§³ÖLCD¡¢OLEDµÈÆÁ¡£ */
    u32 hifi        : 1;  /* ÊÇ·ñÖ§³ÖHi-FiÓïÒôÄ£¿é¡£ */
    u32 onoff       : 1;  /* ÊÇ·ñÖ§³Ö¿ª¹Ø»ú²Ù×÷¡£ */
    u32 hsic        : 1;  /* ÊÇ·ñHSIC½Ó¿Ú¡£ */
    u32 localflash  : 1;  /* ÊÇ·ñÖ§³Ö±¾µØFlash´æ´¢¡£ */
    u32 reserved    : 24;  /* ±£ÁôbitÎ»£¬¹©À©Õ¹Ê¹ÓÃ¡£ */
} DRV_MODULE_SUPPORT_STRU;

/*½á¹¹ËµÃ÷£º¿âÂØ¼ÆµçÑ¹£¬µçÁ÷Ğ£×¼²ÎÊı*/
typedef struct
{
    u32 v_offset_a;         /* µçÑ¹Ğ£×¼ÏßĞÔ²ÎÊı */
    s32 v_offset_b;         /* µçÑ¹Ğ£×¼ÏßĞÔÆ«ÒÆ*/
    u32 c_offset_a;         /* µçÁ÷Ğ£×¼ÏßĞÔ²ÎÊı */
    s32 c_offset_b;         /* µçÁ÷Ğ£×¼ÏßĞÔÆ«ÒÆ */
}COUL_CALI_NV_TYPE;

/* E5´®¿Ú¸´ÓÃ */
/* ½á¹¹ËµÃ÷  : ¸÷ºË´®¿ÚºÅ·ÖÅä£¬ĞéÄâ´®¿ÚºÍÎïÀíAT¿Ú¿ª¹Ø±êÖ¾¡£ */
typedef struct
{
    u32 wait_usr_sele_uart : 1;  /* [bit 0-0]1: await user's command for a moment; 0: do not wait */
    u32 a_core_uart_num    : 2;  /* ·ÖÅä¸øAºËµÄÎïÀí´®¿ÚºÅ¡£ */
    u32 c_core_uart_num    : 2;  /* ·ÖÅä¸øCºËµÄÎïÀí´®¿ÚºÅ¡£ */
    u32 m_core_uart_num    : 2;  /* ·ÖÅä¸øMºËµÄÎïÀí´®¿ÚºÅ¡£ */
    /*
     * A-Shell¿ª¹Ø±êÖ¾¡£
     * 0£º¹Ø£»1£º¿ª¡£
     */
    u32 a_shell            : 1;
    /*
     * C-Shell¿ª¹Ø±êÖ¾¡£
     * 0£º¹Ø£»1£º¿ª¡£
     */
    u32 c_shell            : 1;
    /*
     * ÎïÀíAT¿Ú¿ª¹Ø±êÖ¾¡£
     * 0£º¹Ø£»1£º¿ª¡£
     */
    u32 uart_at            : 1;
    /*
     * ¿ØÖÆÊÇ·ñÊä³öcshellµÄlogµ½AºËÎïÀí´®¿Ú¡£
     * 0£º¹Ø±Õcshellµ½auartµÄÂ·¾¶£»
     * 1£º´ò¿ªcshellµ½auartÂ·¾¶¡£
     */
    u32 extendedbits       : 22;
}DRV_UART_SHELL_FLAG;

/*
 * ½á¹¹ËµÃ÷  : ¿´ÃÅ¹·³õÊ¼»¯ÅäÖÃNV¡£
 * ¸ÃNVÔÚB5000Æ½Ì¨ĞŞ¸ÄÎª¿É¶¨ÖÆÊôĞÔ£¬ÖÕ¶Ë°´ĞèĞŞ¸Ä¶¨ÖÆ£»
 */
typedef struct
{
    /*
     * ¿ØÖÆWDTÊÇ·ñ´ò¿ª¡£
     * 1£º´ò¿ª£»0£º¹Ø±Õ¡£
     */
    u32 wdt_enable;
    u32 wdt_timeout;  /* WDT³¬Ê±Ê±¼ä¡£Ä¬ÈÏ30s¡£ */
    u32 wdt_keepalive_ctime;  /* WDTÎ¹¹·Ê±¼ä¼ä¸ô£¬Ä¬ÈÏ15s¡£ */
    u32 wdt_suspend_timerout;  /* SuspendÖĞWDT³¬Ê±Ê±¼äÄ¬ÈÏ150s¡£ */
    u32 wdt_reserve;  /* WDT±£ÁôÏî¡£ */
}DRV_WDT_INIT_PARA_STRU;

/* pm om¿ØÖÆ NVÏî = 0xd145 */
/*
 * ½á¹¹ËµÃ÷  : 1¡¢ÅäÖÃ¿ØÖÆµÍ¹¦ºÄ¿ÉÎ¬¿É²âÊ¹ÓÃµÄmemoryÀàĞÍ£»
 * 2¡¢µÍ¹¦ºÄ¿ÉÎ¬¿É²â¹¦ÄÜÊÇ·ñ¿ªÆô
 * 3¡¢ÅäÖÃ¸÷¸öºËµÄlog´¥·¢ÎÄ¼ş¼ÇÂ¼µÄ·§Öµ¡£
 */
typedef struct
{
    /*
     * ¿ØÖÆ»º´æÊÇÊ¹ÓÃ¹²ÏíÄÚ´æ»¹ÊÇÆäËûÀàĞÍµÄmemory¡£
     * 0£ºÊ¹ÓÃÔ¤ÏÈ·ÖÅäµÄ256KB¹²ÏíDDR£»
     * 1£º¸´ÓÃÆäËû¿ÉÎ¬¿É²âÄÚ´æ£¨ÈçsocpÊ¹ÓÃµÄDDR¿Õ¼ä£©¡£
     */
    u32 mem_ctrl:1;
    u32 reserved:31;      /* ¿ØÖÆbitÎ»:±£Áô */
    u8  log_threshold[4]; /* ¸÷¸öºËµÄlog´¥·¢ÎÄ¼ş¼ÇÂ¼µÄ·§Öµ£¬ÆäÖĞÊı×éindex¶ÔÓ¦¸÷¸öºËµÄipc±àºÅ£¬·§ÖµÒÔ°Ù·Ö±È±íÊ¾¡£ */
    /*
     * ¿ØÖÆ¸÷Ä£¿éÊÇ·ñ¿ªÆôµÍ¹¦ºÄlog¼ÇÂ¼¹¦ÄÜ£¬Ã¿¸öbit±íÊ¾¸ÃÄ£¿é×´Ì¬¡£
     * 0£º¹Ø±Õ
     * 1£º´ò¿ª
     */
    u8  mod_sw[8];        
}DRV_PM_OM_CFG_STRU;

/* watchpoint = 0xd148 begin */
typedef struct
{
    u32 enable;     /* Ê¹ÄÜ±êÖ¾£¬0£¬È¥Ê¹ÄÜ£»1£¬Ê¹ÄÜ£¬ Èç¹û¸Ã±êÖ¾ÎªÈ¥Ê¹ÄÜ£¬ÅäÖÃ½«Ö±½ÓºöÂÔ */
    u32 type;       /* ¼à¿ØÀàĞÍ: 1£¬¶Á£»2£¬Ğ´£»3£¬¶ÁĞ´ */
    u32 start_addr; /* ¼à¿ØÆğÊ¼µØÖ· */
    u32 end_addr;   /* ¼à¿Ø½áÊøµØÖ· */
}WATCHPOINT_CFG;
typedef struct
{
    u32 enable;     /* Ê¹ÄÜ±êÖ¾£¬0£¬È¥Ê¹ÄÜ£»1£¬Ê¹ÄÜ£¬ Èç¹û¸Ã±êÖ¾ÎªÈ¥Ê¹ÄÜ£¬ÅäÖÃ½«Ö±½ÓºöÂÔ */
    u32 type;       /* ¼à¿ØÀàĞÍ: 1£¬¶Á£»2£¬Ğ´£»3£¬¶ÁĞ´ */
    u32 start_addr_low; /* ¼à¿ØÆğÊ¼µØÖ·µÍ32bit */
    u32 start_addr_high; /* ¼à¿ØÆğÊ¼µØÖ·¸ß32bit */
    u32 end_addr_low;   /* ¼à¿Ø½áÊøµØÖ·µÍ32bit */
    u32 end_addr_high; /* ¼à¿Ø½áÊøµØÖ·¸ß32bit */
}WATCHPOINT_CFG64;
/* ½á¹¹ËµÃ÷£ºÅäÖÃwatchpoint¡£±£ÁôNVÏî½ûÖ¹ĞŞ¸Ä */
typedef struct
{
    WATCHPOINT_CFG64  ap_cfg[4];  /* AºËÅäÖÃ£¬×î¶àÖ§³Ö4¸öwatchpoint */
    WATCHPOINT_CFG  cp_cfg[4];  /* CºËÅäÖÃ£¬×î¶àÖ§³Ö4¸öwatchpoint */
}DRV_WATCHPOINT_CFG_STRU;
/* watchpoint = 0xd148 end */

/* 0xD194, for tsensor start*/
struct DRV_TSENSOR_NV_TEMP_UNIT {
    s32 high_temp;
    s32 low_temp;
};

struct DRV_TSENSOR_NV_TEMP_INFO {
    u32 temp_unit_nums;                                     /*¸ßÎÂÎÂ¶ÈãĞÖµÊµ¼Êµ¥ÔªÊı*/
    struct DRV_TSENSOR_NV_TEMP_UNIT temp_threshold_init[5]; /*×î¶àÖ§³Ö5×é¸ßÎÂ¶ÈãĞÖµ*/
};
/* ½á¹¹ËµÃ÷£ºÎÂ±£ÌØĞÔNV¿ØÖÆ */
typedef struct
{
    u32 enable_reset_hreshold;                                         /*Ê¹ÄÜ¸´Î»ÃÅÏŞ±êÖ¾,0,È¥Ê¹ÄÜ£»1,Ê¹ÄÜ*/
    int reset_threshold_temp;                                          /*¸´Î»ÎÂ¶ÈãĞÖµ*/
    u32 timer_value_s;                                                 /*¶¨Ê±Æ÷³¬Ê±Öµ*/
    u32 ltemp_out_time;                                                /*µÍÎÂÎÂ±£ÍË³öÊ±¼ä*/
    u32 enable_print_temp;                                             /*Ê¹ÄÜ10s´òÓ¡Ò»´ÎÎÂ¶È*/
    u32 enable_htemp_protect;                                          /*¸ßÎÂÎÂ±£Ê¹ÄÜ±êÖ¾, 0, È¥Ê¹ÄÜ;  1, Ê¹ÄÜ*/
    struct DRV_TSENSOR_NV_TEMP_INFO htemp_threshold;   /*¸ßÎÂÎÂ±£ãĞÖµ*/
    u32 enable_ltemp_protect;                                          /*µÍÎÂÎÂ±£Ê¹ÄÜ±êÖ¾, 0, È¥Ê¹ÄÜ;  1, Ê¹ÄÜ*/
    struct DRV_TSENSOR_NV_TEMP_INFO ltemp_threshold;   /*µÍÎÂÎÂ±£ãĞÖµ*/
}DRV_TSENSOR_TEMP_PROTECT_CONTROL_NV_STRU;
/* 0xD194, for tsensor end*/

/* 0xd19a, for pcie speed ctrl  start*/
/* ½á¹¹ËµÃ÷£ºPCIEµ÷ËÙºÍµÍ¹¦ºÄ¿ØÖÆµÄNV */
typedef struct
{
    u32 pcie_speed_ctrl_flag;    /*PCIE ËÙ¶È¿ØÖÆ±êÖ¾,0,È¥Ê¹ÄÜ£»1,Ê¹ÄÜ*/
    u32 pcie_pm_ctrl_flag;       /*PCIE µÍ¹¦ºÄ¿ØÖÆ±êÖ¾,0,È¥Ê¹ÄÜ£»1,Ê¹ÄÜ*/
    u32 pcie_init_speed_mode;    /*³õÊ¼»¯Ê±µÄPCIE ËÙ¶È*/
    u32 pcie_pm_speed_mode;      /*µÍ¹¦ºÄ»Ö¸´Ê±PCIEÅäÖÃµÄËÙ¶È*/
    u32 pcie_speed_work_delay;   /* µ±½µµÍPCIE ËÙ¶ÈµÄÑÓÊ±work µÄÑÓÊ±Ê±¼äÊ±¼äÊÇms */

    u32 gen1_volume;    /*PCIE GEN1Ä£Ê½µÄÁ÷Á¿ãĞÖµ´óĞ¡*/
    u32 gen2_volume;    /*PCIE GEN2Ä£Ê½µÄÁ÷Á¿ãĞÖµ´óĞ¡*/
    u32 gen3_volume;    /*PCIE GEN3Ä£Ê½µÄÁ÷Á¿ãĞÖµ´óĞ¡*/
    u32 pcie_l1ss_enable_flag;          /*PCIE L1SS Enable ,  0,È¥Ê¹ÄÜ£»1,Ê¹ÄÜ*/

}DRV_PCIE_SPEED_CTRL_STRU;
/* 0xd19a, for pcie speed ctrl  end*/

/* ½á¹¹ËµÃ÷£º ¿ØÖÆ¸ßËÙuart²¦ºÅÌØĞÔÊÇ·ñ´ò¿ª£¬Êı¾İ¶Ë¿Ú×ª³Éashell»òcshell¡£Ö÷Ïß²»ÔÙÊ¹ÓÃ */
typedef struct
{
    /*
     * ¿ØÖÆ¸ßËÙuart²¦ºÅÌØĞÔÊÇ·ñ´ò¿ª¡£
     * 0£º¹Ø±Õ£»
     * 1£º´ò¿ª¡£
     */
    u32 DialupEnableCFG;
    /*
    * ¸ÃNVÏîÓÃÓÚ711 hsuart²¦ºÅÒµÎñ£¬NAS½«ÃüÁî²ÎÊıĞ´Èëµ½NVÏîÖĞ£¬Èí¸´Î»ºó
    * hsuart²ã»á¶ÁÈ¡NVÖµ£¬½«hsuart¶Ë¿ÚÇĞ»»³ÉÏàÓ¦¶Ë¿Ú¡£
    * 0£ºhsuartÎªÊı´«¶Ë¿Ú£»
    * 0x5A5A5A5A£ºhsuartÎªAShell¶Ë¿Ú£»
    * 0xA5A5A5A5£ºhsuartÎªCShell¶Ë¿Ú¡£
    */
    u32 DialupACShellCFG;
}DRV_DIALUP_HSUART_STRU;

/* TEST support module nv define */
/* ½á¹¹ËµÃ÷  : ¸ÃNVÊµÏÖ¿ØÖÆ¸÷¸öÄ£¿éÊÇ·ñ´ò¿ªµÄ¹¦ÄÜ£¬1£º´ò¿ª£»0£º¹Ø±Õ¡£ */
typedef struct
{
    u32 lcd         : 1;  /* ÊÇ·ñÖ§³ÖlcdÄ£¿é£¬1: support; 0: not support */
    u32 emi         : 1;  /* ÊÇ·ñÖ§³ÖemiÄ£¿é */
    u32 pwm         : 1; /* ÊÇ·ñÖ§³ÖpwmÄ£¿é */
    u32 i2c         : 1; /* ÊÇ·ñÖ§³Öi2cÄ£¿é */
    u32 leds        : 1;  /* ÊÇ·ñÖ§³ÖledsÄ£¿é */
    u32 reserved    : 27;
} DRV_MODULE_TEST_STRU;

/* ½á¹¹ËµÃ÷  : NV×Ô¹ÜÀí¡£ */
typedef struct
{
    /*
     * ÔÚNVÊı¾İ³öÏÖ´íÎóÊ±µÄ´¦Àí·½Ê½¡£
     * 1£º²úÏßÄ£Ê½£»
     * 2£ºÓÃ»§Ä£Ê½¡£
     */
    u32 ulResumeMode;
}NV_SELF_CTRL_STRU;

/* 0xd158 */
/* ½á¹¹ËµÃ÷  : ¿ØÖÆpmu½Ó¿ÚÀàĞÍÊÇssi½Ó¿Ú»¹ÊÇspmi½Ó¿Ú¡£ */
typedef struct
{
    u32 pmu_enable_cfg;     /* pmu ´ò¿ª¹Ø±Õ¿ØÖÆNV¡£ */
    /*
     * pmu ssi spmiĞ­ÒéÑ¡Ôñ¡£
     * 0£ºssi½Ó¿Ú£»1£ºspmi½Ó¿Ú
     */
    u32 protocol_sel;       
    u32 reserved;  /* ±£Áô¡£ */
}DRV_PMU_CFG_STRU;


/* 0xd159 */
/* ½á¹¹ËµÃ÷  : ¸ÃNVÊµÏÖÔÚmodem Æô¶¯½×¶ÎÅäÖÃ serdes Ê±ÖÓµÄ²¨ĞÎÓëÇı¶¯ÄÜÁ¦¹¦ÄÜ¡£ */
typedef struct
{
    /*
     * Bit 7 ×î¸ßÎ» 0£ºÍ¨¹ınv ÅäÖÃ serdes Ê±ÖÓ²¨ĞÎ mask£¬Èç¹ûÒªĞŞ¸Ä ²¨ĞÎ£¬ĞèÒªÍ¬Ê±ĞŞ¸Ä mask Îª 1£»
     * Bit 0 ×îµÍÎ» 0£ºserdesÅäÖÃÎª·½²¨£»1£º serdesÅäÖÃÎªÕıÏÒ²¨
     */
    u8 clk_type;           
    /*
     * Bit 7 ×î¸ßÎ» 0£ºÍ¨¹ınv ÅäÖÃ serdes Ê±ÖÓÇı¶¯ÄÜÁ¦ mask£¬Èç¹ûÒªĞŞ¸Ä ²¨ĞÎ£¬ĞèÒªÍ¬Ê±ĞŞ¸Ä mask 1£»
     * Bit 0-bit6 ÅäÖÃserdes Ê±ÖÓÇı¶¯ÄÜÁ¦µÄÖµ
     */
    u8 serdes_clk_drv;      
    u8 reserved1;
    u8 reserved2;
}DRV_PMU_CLK_STRU;

/* 0xd183 */
typedef struct
{
	u32 sim_volt_flag;
	u32 reserved;
}DRV_NV_PMU_TYPE;

/* 0xd168 */
/* ½á¹¹ËµÃ÷  : £¨C10/C20  £©¿ØÖÆË«ºËcoresight¹¦ÄÜµÄÊ¹ÄÜÓëETBÊı¾İ±£´æÊÇ·ñÊ¹ÄÜ¡£ */
typedef struct
{
    u32 ap_enable:1;        /* Ap coresightÊ¹ÄÜ¿ØÖÆ 1£ºÊ¹ÄÜ£¬0£º²»Ê¹ÄÜ */
    u32 cp_enable:1;        /* CP coersightÊ¹ÄÜ¿ØÖÆ1£ºÊ¹ÄÜ£¬0£º²»Ê¹ÄÜ */
    u32 ap_store :1;        /*AP coresight store flag*/
    u32 cp_store :1;        /* Cp etbÊı¾İ±£´æ¿ØÖÆ1£ºÊ¹ÄÜ£¬0£º²»Ê¹ÄÜ */
    u32 reserved :28;
}DRV_CORESIGHT_CFG_STRU;

/* ½á¹¹ËµÃ÷  : M3ÉÏ¿ØÖÆµÍ¹¦ºÄµÄNV¡£ */
typedef struct
{
    /*
     * M3ÊÇ·ñ½øNormal wfi¡£
     * 0±íÊ¾²»½ø£»
     * 1±íÊ¾½ø¡£
     */
    u8 normalwfi_flag;
    /*
     * M3ÊÇ·ñ½øÉîË¯µÄ±ê¼Ç¡£
     * 0±íÊ¾²»½ø£»
     * 1±íÊ¾½ø¡£
     */
    u8 deepsleep_flag;
    /*
     * M3 ÊÇ·ñ×ßBuck3µôµçµÄÁ÷³Ì¡£
     * 0±íÊ¾Buck3²»µôµç£»
     * 1±íÊ¾Buck3µôµç¡£
     */
    u8 buck3off_flag;
    /*
     * M3 Èç¹ûBuck3²»µôµçµÄ»°£¬ÍâÉèÊÇ·ñµôµç¡£
     * 0±íÊ¾ÍâÉè²»µôµç£»
     * 1±íÊ¾ÍâÉèµôµç£¨×¢Òâ£¬Èç¹ûBuck3µôµçµÄ»°£¬´ËNVÅäÖÃ²»Æğ×÷ÓÃ£¬Buck3µôµç£¬ÍâÉèÒ»¶¨µôµç£©¡£
     */
    u8 peridown_flag;
    u32 deepsleep_Tth;  /* CºËÉîË¯Ê±¼äãĞÖµ¡£ */
    u32 TLbbp_Tth;  /* CºËÉîË¯£¬BBP¼Ä´æÆ÷µÄãĞÖµ¡£ */
}DRV_NV_PM_TYPE;

/* ½á¹¹ËµÃ÷  : ¸ÃNVÊµÏÖ¶ÔR8 mem retentionÌØĞÔµÄ¿ª¹Ø¿ØÖÆ¡£ */
typedef struct
{
    u32 lr_retention_enable;  /* 4G R8 MEM retention¿ª¹Ø¿ØÖÆ */
    u32 nr_retention_enable;  /* 5G  R8 MEM retention¿ª¹Ø¿ØÖÆ */
    u32 l2hac_retention_enable;  /* L2HAC  R8 MEM retention¿ª¹Ø¿ØÖÆ,µ±Ç°ÔİÎ´ÆôÓÃ£¬HAC memÓëNR µÄÊ¹ÓÃnr_retention_enableÍ³Ò»¿ØÖÆ */
	u32 default_mode;  /* NvÎ´´ò¿ªÇé¿öÏÂ£¬Ä¬ÈÏÄ£Ê½£¬Èç¹ûÎª1£¬ÔòÄ¬ÈÏÎªshut downÄ£Ê½£»Èç¹ûÎª1£¬ÔòÄ¬ÈÏÎªretentionÄ£Ê½ */
    u32 reserved1;
}ST_PWC_RETENTION_STRU;

/* ½á¹¹ËµÃ÷  : ¸ÃNVÊµÏÖ¶ÔµÍ¹¦ºÄÎ¬²âdebugÌØĞÔµÄ¿ª¹Ø¿ØÖÆ¡£ */
typedef struct
{
    u32 lpmcu_lockirq_syserr:1; /* if lockirq time length greater than threshold,this cfg decide whether system error*/
    u32 lrcpu_pminfo_report2hids:1; /*if set,report lr pm info to hids,include dpm,pm,wakelock,rsracc etc*/
    u32 nrcpu_pminfo_report2hids:1; /*if set,report nr pm info to hids,include dpm,pm,wakelock,rsracc etc*/
    u32 l2hac_pminfo_report2hids:1; /*if set,report l2hac pm info to hids,include dpm,pm,wakelock,rsracc etc*/
    u32 rsv:28;
    u32 lpmcu_irq_monitor_threshold; /*record lpmcu lockirq time length threshold,slice number*/
    u32 pm_stay_wake_time_threshold; /* ccpu±£³Ö»½ĞÑÊ±¼äãĞÖµ£»³¬¹ı¸ÃãĞÖµ£¬Íùhids´òÓ¡Î¬²âĞÅÏ¢ */
    u32 ccpu_sleep_check_time; /*M3 check if response ccpu pd irq late,unit is slice*/
    u32 nrwakeup_time_threshold; /*M3 check if nrwakeup time over threshold,unit is slice*/
    u32 lrwakeup_time_threshold; /*M3 check if lrwakeup time over threshold,unit is slice*/
    u32 rsv5;
    u32 rsv6;
    u32 rsv7;
    u32 rsv8;
}ST_PMOM_DEBUG_STRU;

/* ¿¿¿¿  : ¿NV¿¿¿¿HKADC¿¿¿¿¿¿¿ */
typedef struct {
    u8 version; /* ¿¿¿¿¿1¿¿hkadc¿¿¿¿¿¿¿¿ */
    u8 rsv1;
    u8 rsv2;
    u8 rsv3;
} DRV_HKADC_CAL_VERSION;


/* ½á¹¹ËµÃ÷  : ÓÃÓÚÖ¸Ê¾Î¬²â£¨DIAG£©Ê¹ÓÃµÄÄ¬ÈÏÍ¨µÀÀàĞÍ¡£¡£ */
typedef struct
{
    /*
     * 0£ºUSBÍ¨µÀ
     * 1£ºVCOM
     */
    u32 enPortNum;         
}DIAG_CHANNLE_PORT_CFG_STRU;
/* ½á¹¹ËµÃ÷£ºdeflate ÌØĞÔ¿ª¹Ø */
typedef struct {
u32 deflate_enable:1;      /*0 deflateÌØĞÔ¹Ø±Õ 1 deflateÌØĞÔ´ò¿ª*/
u32 reservd:31;
}DRV_DEFLATE_CFG_STRU;

/* ½á¹¹ËµÃ÷£º·ÏÆú²»ÓÃ */
typedef struct {
    u32 iqi_enable:1;      /*0 iqiÌØĞÔ¹Ø±Õ 1 iqiÌØĞÔ´ò¿ª*/
    u32 serial_enable:1;   /*serial ¿ª¹Ø 1´ò¿ª 0 ¹Ø±Õ*/
    //u32 debug_enable:1;    /*debug¹¦ÄÜÊ¹ÄÜ*/
    u32 reservd:30;
}DRV_IQI_CFG_STRU;

/*
 * ½á¹¹ËµÃ÷  : ¸ÃNVÊµÏÖ¶Ô¸÷×é¼ş½øĞĞ¿ª»úlogÍ¨ÖªºÍ¸÷×é¼ş¿ª»úlog¼¶±ğÅäÖÃµÄ¹¦ÄÜ
 */
typedef struct
{
    /*
     * 0£º¹Ø±Õ¿ª»úLOGÌØĞÔ£»
     * 1£º´ò¿ª¿ª»úLOGÌØĞÔ¡£
     */
    u8  cMasterSwitch;  
    /*
     * 0£º¿ª»úLOGÄÚ´æ²»¿ÉÓÃ£»
     * 1£º¿ª»úLOGÄÚ´æ¿ÉÓÃ¡£
     */
    u8  cBufUsable;
    /*
     * 0£º¿ª»úLOGÄÚ´æ²»Ê¹ÄÜ£»
     * 1£º¿ª»úLOGÄÚ´æÊ¹ÄÜ¡£
     */
    u8  cBufEnable;     
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswACPUBsp;     
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswACPUDiag;
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswACPUHifi;
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswLRMBsp;
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswLRMDiag;
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswLRMTLPhy;
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswLRMGUPhy;    
    u8  cswLRMCPhy;     /* Range:[0,3] 4G Modem CPHY¿ª»úlog profile */
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswLRMEasyRf;   
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswNRMBsp;      
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswNRMDiag;     
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswNRMHAC;      
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswNRMPhy;      
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswNRMHL1C;     
    /*
     * 0£º¿ª»úLOG¹Ø±Õ
     * 1£º¾«¼ò¼¶±ğLOG£»
     * 2£ºÕı³£¼¶±ğLOG
     * 3£ºÈ«LOG
     */
    u8  cswNRMPDE;      
    u8  cReserved[6];   /* ±£Áô×Ö¶Î */
}DRV_NV_POWER_ON_LOG_SWITCH_STRU;


typedef struct
{
    s32 temperature;
    u16 code;
    u16 reserved;
} DRV_CONVERT_TABLE_TYPE;
#define XO_TABLE_SIZE 166
/* ½á¹¹ËµÃ÷£ºÄÚ²¿Ê¹ÓÃ */
typedef struct
{
    DRV_CONVERT_TABLE_TYPE convert_table[XO_TABLE_SIZE];
} DRV_XO_CONVERT_TABLE;
#define PA_TABLE_SIZE 32
/* ½á¹¹ËµÃ÷£ºÄÚ²¿Ê¹ÓÃ */
typedef struct
{
    DRV_CONVERT_TABLE_TYPE convert_table[PA_TABLE_SIZE];
} DRV_PA_CONVERT_TABLE;


#ifndef LPHY_UT_MODE//lint !e553
typedef struct convert_table
{
    s32 temperature;
    u16 code;
    u16 reserved;
} convert_table;
#define XO_TBL_SIZE 166
typedef struct xo_convert_table_array
{
    convert_table convert_table[XO_TBL_SIZE];
} xo_convert_table_array;
#define PA_TBL_SIZE 32
typedef struct pa_convert_table_array
{
    convert_table convert_table[PA_TBL_SIZE];
} pa_convert_table_array;
#endif
/* NVID 50019 for HiBurn tool update */
typedef struct {
    /*
    0 : fastboot¿¿¿¿¿kernel
    1 : ¿¿¿¿¿fastboot
    */
    u8 update_mode_flag;
    u8 reserved1;
    u8 reserved2;
    u8 reserved3;
} NV_HIBURN_CONFIG_STRU;

/* ¿¿¿¿¿¿ NV ¿¿¿¿ digital power monitor ¿¿¿¿¿¿¿¿¿¿ */
typedef struct {
    /* ¿¿¿¿¿¿,0¿¿,1¿¿,¿¿¿¿ */
    u32 enable;
    /* ¿¿¿¿¿¿¿¿¿¿¿,¿¿¿ms,¿¿¿1000 */
    u32 task_delay_ms;
    u32 reserved1;
    u32 reserved2;
} DRV_POWER_MONITOR_CFG;

/*
 * ¿¿¿¿¿NV¿¿¿¿
 */
typedef struct {
    unsigned char factory_reset_flag; /* ¿¿¿¿¿¿ 1:¿¿¿¿ 0:¿¿¿¿¿¿ */
    unsigned char reserve[0x3];         /* ¿¿¿¿ */
} NVM_OPT_MODE_STRU;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
