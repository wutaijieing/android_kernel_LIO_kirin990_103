/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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
#include "at_device_phy_cmd_tbl.h"

#include "AtParse.h"
#include "at_cmd_proc.h"
#include "at_parse_core.h"
#include "at_test_para_cmd.h"
#include "at_device_cmd.h"
#include "at_lte_common.h"

#include "at_device_phy_set_cmd_proc.h"
#include "at_device_phy_qry_cmd_proc.h"


#define THIS_FILE_ID PS_FILE_ID_AT_DEVICE_PAM_CMD_TBL_C

static const AT_ParCmdElement g_atDevicePhyCmdTbl[] = {
    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 查询产品形态
     * [说明]: 该命令用来查询产品形态。
     * [语法]:
     *     [命令]: ^PRODTYPE?
     *     [结果]: <CR><LF>^PRODTYPE: <result>
     *             <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>ERROR<CR><LF>
     * [参数]:
     *     <result>: 终端产品形态，取值范围为0～255。
     *             0：数据卡；
     *             1：模块；
     *             2：E5；
     *             3：CPE；
     *             其他：无意义。
     */
    { AT_CMD_PRODTYPE,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryProdTypePara, AT_QRY_PARA_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^PRODTYPE", VOS_NULL_PTR },

    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 车载模块RFIC_DIE_ID读取
     * [说明]: 该命令用于读取车载模块RFIC_DIE_ID；
     *         BalongV7R22C60新增。
     *         该命令暂不支持。
     * [语法]:
     *     [命令]: ^RFICID?
     *     [结果]: 执行成功时：
     *             <CR><LF>^RFICID: (list of supported<DIEID>s)<CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             错误情况时返回：
     *             <CR><LF>+CME ERROR: <err_code><CR><LF>
     * [参数]:
     *     <DIEID>: 字符串类型，RFIC_DIE_ID码流，16进制输出 32字节256bit
     *             最多两条，可以没有。
     * [示例]:
     *     · 执行测试命令 底层获取到两条RFICID的情况
     *       AT^RFICID?
     *       ^RFICID: 0, "0000000000000000000000000000000000000000000000000000000000000000"
     *       ^RFICID: 1, "0000000000000000000000000000000000000000000000000000000000000000"
     *       OK
     *     · 执行测试命令 底层获取到一条RFICID的情况
     *       AT^RFICID?
     *       ^RFICID: 0, "0000000000000000000000000000000000000000000000000000000000000000"
     *       OK
     *     · 执行测试命令 底层没有获取到RFICID的情况
     *       AT^RFICID?
     *       OK
     */
    { AT_CMD_RFICID,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryRficDieIDPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^RFICID", VOS_NULL_PTR },

    { AT_CMD_RFFEID,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryRffeDieIDPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^RFFEID", VOS_NULL_PTR },

    { AT_CMD_LTCOMMCMD,
      AT_SetLTCommCmdPara, AT_SET_PARA_TIME, atQryLTCommCmdPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^LTCOMMCMD", (VOS_UINT8 *)"(0-65535),(0-2000),(0-65535),(0-65535),(@data),(@data),(@data),(@data)" },

    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 设置非信令下的波形
     * [说明]: 在非信令下发出指定波形信号。若产品不支持直接返回错误。频点设置使用^FCHAN。
     *         该命令需要在^FCHAN后边执行。
     *         如果参数不带，则默认为0。
     * [语法]:
     *     [命令]: ^FWAVE=<type>,<amplitue_dbm_percent>[,<slot>]
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>ERROR<CR><LF>
     * [参数]:
     *     <type>: 发射的波形类型:
     *             0: BPSK
     *             1: PI2_BPSK
     *             2: QPSK
     *             3: 16QAM
     *             4: 64QAM
     *             5: 256QAM
     *             6: GMSK
     *             7: 8PSK
     *             8: 单音：
     *             G模支持6、7、8三种波形；G模默认按1时隙测试；（G模不测调制）
     *             W模支持2、8两种波形；
     *             C模支持2、8两种波形；
     *             T模支持8一种波形；
     *             L模支持2、3、4、5、8五种波形；
     *             NR模支持0~5和8七种波形；
     *             目前NR只要求采用CP-OFDM测试；
     *     <amplitue_dbm_percent>: 发射波形的功率大小，以0.01dBm为单位
     *     <slot>: GSM调制信号的时隙：
     *             0：8时隙发射；
     *             1：1时隙发射；
     *             2：2时隙发射；
     *             3：3时隙发射；
     *             4：4时隙发射；
     * [示例]:
     *       非信令下波形设置成功
     *     · W:
     *       AT^FWAVE=0,100
     *       OK
     *     · G:
     *       AT^FWAVE=0,1000
     *       OK
     */
    { AT_CMD_FWAVE,
      AT_SetFwavePara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^FWAVE", (VOS_UINT8 *)FWAVE_CMD_PARA_STRING },

    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 设置非信令的信道
     * [说明]: 在调试状态下设置某一频段UL或DL信道。设置后，单板自动按照设置的信道同步设置相应的上行和下行信道。若产品不支持可直接返回ERROR。该指令一次性有效，设置内容重启（包括软、硬重启）不保留（即下电不保存）。
     *         此命令在非信令模式（AT^TMODE=1）下使用，其它模式下返回错误码。
     *         如果参数不带，则默认为0。
     *         每次下发这个命令，都会将原有的设置清除，也就是一旦设置了这个命令，之前的所有信息都会清除；
     * [语法]:
     *     [命令]: ^FCHAN=<mode>,<band>,<channel>,<band_width>,[<sub_carrier_space>]
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     *     [命令]: ^FCHAN?
     *     [结果]: <CR><LF>^FCHAN: <mode>,<band>,<ul_channel>,<dl_channel>,<band_width>,[<sub_carrier_space>]<CR><LF>
     *             <CR><LF>OK<CR><LF>
     *     [命令]: ^FCHAN=?
     *     [结果]: <CR><LF>^FCHAN: (list of supported <mode>s),(list of supported <band>s),(list of supported < ul_channel >s) , (list of supported < dl_channel >s) <CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <mode>: 单板模式。
     *             0：WCDMA；
     *             2：TD-SCDMA(目前带NR的芯片不支持TD)；
     *             3：GSM；
     *             4：EDGE；
     *             5：AWS；
     *             6：FDD LTE；
     *             7：TDD LTE。
     *             8：WIFI
     *             9：NR
     *             10：LTE-V
     *     <band>: 频段频率。
     *             不管以前GUCTL模的band是怎么转换的，但这次统一按照按协议band设置；
     *             G模的Band对应关系如下：
     *             GSM850对应band5;
     *             GSM900对应Band8；
     *             GSM1800对应band3;
     *             GSM1900对应Band2;
     *             如果<mode>=WIFI的Band, 这个参数应该填为15（与以前兼容）；
     *     <channel>: 信道号，取值范围为0～4294967295。
     *             X模取值范围为[1,799]和[991,1323]。
     *             鉴于有些band的上下行信道是不对称的，所以请测试的兄弟特别注意。如果需要测试不对称的band，在打开TX或者RX前需单独设置上下行的信道：
     *             如LTE的Band: 66, 70;
     *             如NR的Band: 66, 70, 75, 76;
     *     <ul_channel>: 上行信道（如果上行和下行一样也需要上报）。
     *     <dl_channel>: 下行信道（如果上行和下行一样也需要上报）。
     *     <band_width>: 带宽值（这个参数开放给所有模，由产品测试时自己决定各个模的带宽）：
     *             0: 200K
     *             1: 1.2288M
     *             2: 1.28M
     *             3: 1.4M
     *             4: 3M
     *             5: 5M
     *             6: 10M
     *             7: 15M
     *             8: 20M
     *             9: 25M
     *             10: 30M
     *             11: 40M
     *             12: 50M
     *             13: 60M
     *             14: 80M
     *             15: 90M
     *             16: 100M
     *             17: 200M
     *             18: 400M
     *             19: 800M
     *             20: 1G
     *     <sub_carrier_space>: NR子载波间隔（NR必带参数）：
     *             0: 15K
     *             1: 30K
     *             2: 60K
     *             3: 120K
     *             4: 240K
     *             注：NR sub6G只支持30K
     *     <err>: 错误码:
     *             0：单板模式错误；
     *             1：输入频段信息无法对应；
     *             2：设置信道信息失败；
     *             3：输入频段和信道信息无法组合；
     *             4：其它错误（包括参数错误，参数个数不正确等）。
     *             5：WIFI的band设置错误；
     *             6：不支持WIFI；
     *             7：WIFI没有找开；
     *             8：接入模式不正确；
     *             9：band width错误；
     *             10：子载波间隔参数错误；
     *             11：发送加载DSP消息失败；
     *             12：加载DSP失败；
     *             13: 没有配置SCS参数
     *             50：参数错误；
     *             536：band和channel 不匹配；
     * [示例]:
     *     · 单板已经进入非信令模式，设置band41
     *       AT^FCHAN=10,28,27210,8
     *       OK
     */
    { AT_CMD_FCHAN,
      At_SetFChanPara, AT_SET_PARA_TIME, At_QryFChanPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^FCHAN", (VOS_UINT8 *)FCHAN_CMD_PARA_STRING },

    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 选择射频通路
     * [说明]: 该命令用于设置单板的射频通路，该命令仅一次有效，掉电后不保存。
     *         如果参数不带，则默认为0。
     * [语法]:
     *     [命令]: ^TSELRF=<path>,<tx_or_rx>[,<mimo_type>,<group>]
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>ERROR<CR><LF>
     *     [命令]: ^TSELRF?
     *     [结果]: <CR><LF>^TSELRF: <number><CR><LF>
     *             <CR><LF>^TSELRF: <path>
     *             <CR><LF>^TSELRF: <path><CR><LF>
     *             ……
     *             <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>ERROR<CR><LF>
     *     [命令]: ^TSELRF=?
     *     [结果]: 不支持5G时：
     *             <CR><LF>^TSELRF: (list of supported <path>s), (list of supported <tx_or_rx>s) <CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             支持5G时：
     *             <CR><LF>^TSELRF: (list of supported <path>s), (list of supported <tx_or_rx>s), (list of supported <mimo_type>s), (list of supported <group>s) <CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <path>: 具体的射频通路编号。
     *             1：GSM通路（包括EDGE/GPRS/EGSM等的全集）；
     *             2：WCDMA主集；
     *             3：WCDMA分集（分集工作时发射通路仍然为主集，仅接收通路为分集）；
     *             6：TD通道；
     *             7：WIFI通道；
     *             8：WiMax通路（产品线定义值，HISI不支持）；
     *             9：FDD LTE主集；
     *             10：FDD LTE 分集；
     *             11：FDD LTE MIMO；
     *             12：TDD LTE/LTE-V 主集；
     *             13：TDD LTE/LTE-V 分集；
     *             14：TDD LTE/ MIMO；
     *             15：Navigation通路（产品线定义值，HISI不支持）；
     *             16：NR 主集；
     *             17：NR 分集；
     *             18：NR MIMO；
     *     <number>: 单板支持的总的通路数，同一类<path>包括多个<group>，只记录一次。
     *     <group>: 保留(后续可能测试MIMO)。范围（0~255）
     *     <mimo_type>: 表示MIMO类型，范围（1-3）
     *             1：2个TX或者2个RX；
     *             2：4个RX；
     *             3：8个RX；
     *     <group>: 表示测试那根天线，根据<mimo_type>参数不同，取值范围不同：
     *             若<mimo_type> = 1；则取值如下：
     *             1：第1根天线；
     *             2：第2根天线；
     *             3：同时测试天线1和天线2；（只支持单音）
     *             若<mimo_type> = 2；则取值如下：
     *             1：第1根天线；
     *             2：第2根天线；
     *             3：第3根天线；
     *             4：第4根天线；
     *             若<mimo_type> = 3；则取值如下：
     *             1：第1根天线；
     *             2：第2根天线；
     *             3：第3根天线；
     *             4：第4根天线；
     *             5：第5根天线；
     *             6：第6根天线；
     *             7：第7根天线；
     *             8：第8根天线；
     *     <tx_or_rx>: 表示是TX还是RX：
     *             0：表示TX；
     *             1：表示RX；
     *     <err>: 错误情况说明：
     *             0：当前模式错误；
     *             1：Path 错误；
     *             2：TX 或者RX 参数错误；
     *             6：不支持WIFI；
     *             7：没有打开wifi;
     *             50：参数错误；
     * [示例]:
     *     · RX选择射频通路为NR MIMO(4R)的第三根天线
     *       AT^TSELRF=18,1,2,3
     *       OK
     */
    { AT_CMD_TSELRF,
      AT_SetTSelRfPara, AT_SET_PARA_TIME, AT_QryTSelRfPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_SET_PARA_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (TAF_UINT8 *)"^TSELRF", (VOS_UINT8 *)TSELRF_CMD_PARA_STRING },

#if (FEATURE_UE_MODE_NR == FEATURE_OFF)
    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 设置天线闭环调谐
     * [说明]: 该命令用于非信令模式下，设置天线闭环调谐是否使能。这个命令只针对命令^FCHAN命令中MODE为WCDMA、AWS、FDD LTE、TDD LTE有效；
     *         注：
     *         1、这个命令需要在^FTXON前下发。
     *         2、这个命令在支持NR的版本上所有制式暂不支持，后续根据产品线需求实现。
     * [语法]:
     *     [命令]: ^CLT=<enable>
     *     [结果]: 执行成功时：
     *             <CR><LF>OK<CR><LF>
     *             有MT 相关错误时：
     *             <CR><LF>+CME ERROR: <err_code><CR><LF>
     *     [命令]: ^CLT=?
     *     [结果]: <CR><LF>^CLT: (list of supported <enable>s)<CR><LF><CR><LF>OK<CR><LF>
     * [参数]:
     *     <enable>: 天线调谐闭环是否使能：
     *             0：关闭闭环
     *             1：使能闭环
     * [示例]:
     *     · 天线调谐闭环使能
     *       AT^CLT=1
     *       OK
     *     · 执行测试命令
     *       AT^CLT=?
     *       ^CLT: (0,1)
     *       OK
     */
    { AT_CMD_CLT,
      At_SetCltPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^CLT", (VOS_UINT8 *)"(0,1)" },
#endif

#if (FEATURE_UE_MODE_NR == FEATURE_OFF)
    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: CLT（天线闭环调谐）状态信息查询
     * [说明]: 查询当前MT工位CLT状态信息(只有WPHY和LPHY才会有状态信息)。
     *         注：
     *         1、CLT INFO信息只支持查询一次，每当查询后，modem记录的CLT INFO信息就会清掉。如果再次查询，就只返回OK，直到物理层有新的消息报上来。
     *         2、这个命令在支持NR的版本上所有制式暂不支持，后续根据产品线需求实现。
     * [语法]:
     *     [命令]: ^CLTINFO?
     *     [结果]: 若当前modem记录有CLTINFO信息：
     *             <CR><LF>^CLTINTO: <GammaReal>, <GammaImag>, <GammaAmpUc0>, <GammaAmpUc1>, <GammaAmpUc2>, <GammaAntCoarseTune>, <FomcoarseTune>, <CltAlgState>, <CltDetectCount>, <Dac0>, <Dac1>, <Dac2>, <Dac3> <CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             若modem没有记录有CLTINFO信息：
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <GammaReal>: 有符号整型值，反射系数实部
     *     <GammaImag>: 有符号整型值，反射系数虚部
     *     <GammaAmpUc0>: 无符号整型值，驻波检测场景0反射系数幅度
     *     <GammaAmpUc1>: 无符号整型值，驻波检测场景1反射系数幅度
     *     <GammaAmpUc2>: 无符号整型值，驻波检测场景2反射系数幅度
     *     <GammaAntCoarseTune>: 无符号整型值，粗调格点位置
     *     <FomcoarseTune>: 无符号整型值，粗调FOM值
     *     <CltAlgState>: 无符号整型值，闭环算法收敛状态
     *     <CltDetectCount>: 无符号整型值，闭环收敛总步数
     *     <Dac0>: 无符号整型值，Dac0
     *     <Dac1>: 无符号整型值，Dac1
     *     <Dac2>: 无符号整型值，Dac2
     *     <Dac3>: 无符号整型值，Dac3
     * [示例]:
     *     · 查询上报
     *       AT^CLTINFO?
     *       ^CLTINFO: 3150,0,4465,2214,3150,22536,185,17,3,106,11,0,0
     *       OK
     */
    { AT_CMD_CLTINFO,
      VOS_NULL_PTR, AT_NOT_SET_TIME, At_QryCltInfo, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^CLTINFO", VOS_NULL_PTR },
#endif

#if (FEATURE_UE_MODE_NR == FEATURE_OFF)
    { AT_CMD_MIPIRD,
      AT_SetMipiRdPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (TAF_UINT8 *)"^MIPIRD", (TAF_UINT8 *)"(0-9),(0-1),(0-15),(0-255)" },
#endif

    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 非信令下打开发射机
     * [说明]: 在非信令下打开或关闭发射机。若产品不支持可直接返回ERROR。
     *         注：
     *         1.     此命令在非信令模式（AT^TMODE=1）下使用，其它模式下返回错误码0。
     *         2.     此命令需要在设置非信令信道（^FCHAN）和选择射频通道（^TSELRF）后执行。
     *         3.     如果参数不带，则默认为0。
     *         4.     非信令无线模式，如果使用At命令配置上行，需要先通过At^FRXON命令配置下行，借用下行配置流程把Tx Tuner配置成功，再配置上行。
     * [语法]:
     *     [命令]: ^FTXON=<switch>
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>+CME ERROR:<err><CR><LF>
     *     [命令]: ^FTXON?
     *     [结果]: <CR><LF>^FTXON: <switch><CR><LF>
     *             <CR><LF>OK<CR><LF>
     *     [命令]: ^FTXON=?
     *     [结果]: <CR><LF>^FTXON: (list of supported <switch>s) <CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <switch>: 打开/关闭发射机，支持5G时取值为0-1。
     *             0：关闭发射机；
     *             1：打开发射机。包括基带调制和RF发射通道；
     *             2：打开发射机。只打开RFIC Transmitter和PA。
     *     <err>: 错误码。
     *             0：单板模式错误；
     *             1：没有设置相关信道；
     *             2：打开发射机失败；
     *             3：其它错误。
     */
    { AT_CMD_FTXON,
      At_SetFTxonPara, AT_SET_PARA_TIME, At_QryFTxonPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_FTXON_OTHER_ERR, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^FTXON", (VOS_UINT8 *)FTXON_CMD_PARA_STRING },

    /*
     * [类别]: 协议AT-测试类私有
     * [含义]: 设置DPDT测试开关
     * [说明]: 该命令只用于测试模式，设置天线算法打开/关闭。如果进入测试模式，想恢复原来默认态，请上下电重启单板。
     *         注：这个命令在带NR的版本上MT工位不支持，物理层和BBIC都不支持这个。AT模块会直接返回OK；
     * [语法]:
     *     [命令]: ^DPDTTEST=<RatMode>,<Flag>
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>+CME ERROR <err><CR><LF>
     *     [命令]: ^DPDTTEST=?
     *     [结果]: <CR><LF>^DPDTTEST: (list of supported <RatMode>s),(list of supported <Flag>s)<CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <RatMode>: 整型值，接入模式。
     *             0：GSM；
     *             1：WCDMA；
     *             2：LTE；
     *             3：TDSCDMA；
     *             4：CDMA
     *             5：NR
     *     <Flag>: 整型值，DPDT测试开关。
     *             0：关闭双天线算法；
     *             1：打开双天线算法；
     *             2：恢复默认状态。
     *             注：当前仅(0-2)的取值生效，为保留扩展性Flag取值范围定为(0-65535)。
     * [示例]:
     *     · G模下打开双天线算法
     *       AT^DPDTTEST=0,1
     *       OK
     */
    { AT_CMD_DPDTTEST,
      At_SetDpdtTestFlagPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (VOS_UINT8 *)"^DPDTTEST", (VOS_UINT8 *)DPDT_TEST_CMD_PATA_STRING },

    /*
     * [类别]: 协议AT-测试类私有
     * [含义]: 设置DPDT状态
     * [说明]: 设置天线切换状态。该命令在出d后需要重新设置双天线控制字，出d表示挂电话或者停止数传业务。
     *         这条命令回复成功，只代表软件流程是OK的，但并不代表硬件设置成功；
     *         目前不支持毫米波。
     *         注：
     *         1、如果参数不带，则默认为0；
     *         2、当前HAL状态没有实现。
     *         该命令仅限产线模式下使用。
     * [语法]:
     *     [命令]: ^DPDT=<RatMode>,<DpdtValue>,<WorkType>
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     *     [命令]: ^DPDT=?
     *     [结果]: 不支持5G时：
     *             <CR><LF>^DPDT: (list of supported <RatMode>s),(list of supported <DpdtValue>s)<CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             支持5G时：
     *             <CR><LF>^DPDT: (list of supported <RatMode>s),(list of supported <DpdtValue>s), (list of supported <WorkType>s)<CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <RatMode>: 整型值，接入模式。
     *             0：GSM；
     *             1：WCDMA；
     *             2：LTE；
     *             3：TDSCDMA；
     *             4：CDMA。
     *             5：NR
     *     <DpdtValue>: 整型值，目前只用到低8位。
     *             低4bit表示TAS/MAS状态：
     *             0：TAS直通MAS直通
     *             1：TAS交叉MAS直通
     *             2：TAS直通MAS交叉
     *             3：TAS交叉MAS交叉
     *             高4bit表示Hall状态：
     *             Bit4：Hall有效标志位，0表示无需配置Hall态，1表示根据Bit5设置Hall态
     *             Bit5：Hall状态位，0表示非Hall态，1表示Hall态
     *             注：当前仅(0,1,2,3,16,17,18,19,48,49,50,51)值生效，为保留扩展性DpdtValue取值范围定为(0-65535)。
     *     <WorkType>: 工作状态：
     *             0：业务状态；
     *             1：侦听状态；
     * [示例]:
     *     · W模下设置天线为直通态
     *       AT^DPDT=1,0,1
     *       OK
     *     · W模下设置天线为交叉态和Hall态
     *       AT^DPDT=1,49,1
     *       OK
     *     · W模下设置天线为交叉态和非Hall态
     *       AT^DPDT=1,17,1
     *       OK
     *     · L模下设置天线为交叉态
     *       AT^DPDT=2,1,1
     *       OK
     *     · L模下设置Hall态
     *       AT^DPDT=2,48,1
     *       OK
     *     · L模下设置非Hall态
     *       AT^DPDT=2,16,1
     *       OK
     *       注：TD-SCDMA不支持设置Hall态，LTE不支持在一条AT命令同时配置Hall和Dpdt状态。
     */
    { AT_CMD_DPDT,
      At_SetDpdtPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (VOS_UINT8 *)"^DPDT", (VOS_UINT8 *)DPDT_CMD_PARA_STRING },

    /*
     * [类别]: 协议AT-测试类私有
     * [含义]: 查询DPDT Value值
     * [说明]: 查询天线当前状态,返回当前天线DPDT值。
     *         说明：修改天线后，如果天线算法仍在运转，则该状态可能会被改变；建议使用该命令切换DPDT后，再次使用[AT^DPDT=RatMode,0]恢复默认状态。
     *         说明:
     *         如果参数不带，则默认为0。
     *         该命令仅限产线模式下使用。
     * [语法]:
     *     [命令]: ^DPDTQRY=<RatMode>,<WorkType>
     *     [结果]: <CR><LF>^DPDTQRY: <DpdtValue><CR><LF>
     *             <CR><LF>OK<CR><LF>
     *     [命令]: ^DPDTQRY=?
     *     [结果]: 不支持5G时：
     *             <CR><LF>^DPDTQRY: (list of supported <RatMode>s)<CR><LF>
     *             <CR><LF>OK<CR><LF
     *             支持5G时：
     *             <CR><LF>^DPDTQRY: (list of supported <RatMode>s), (list of supported <WorkType>s)<CR><LF>
     *             <CR><LF>OK<CR><LF
     * [参数]:
     *     <RatMode>: 整型值，接入模式。
     *             0：GSM；
     *             1：WCDMA；
     *             2：LTE；
     *             3：TDSCDMA；
     *             4：CDMA
     *             5：NR
     *     <DpdtValue>: 整型值。
     *             0：TAS直通MAS直通
     *             1：TAS交叉MAS直通
     *             2：TAS直通MAS交叉
     *             3：TAS交叉MAS交叉
     *             注：当前仅(0,1,2,3)值生效，为保留扩展性DpdtValue取值范围定为(0-65535)。
     *     <WorkType>: 工作状态：
     *             0：业务状态；
     *             1：侦听状态；
     * [示例]:
     *     · W模下查询天线状态
     *       AT^DPDTQRY=1,1
     *       ^DPDTQRY :0
     *       OK
     */
    { AT_CMD_DPDTQRY,
      At_SetQryDpdtPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (VOS_UINT8 *)"^DPDTQRY", (VOS_UINT8 *)DPDT_QRY_PARA_STRING },

    /*
     * [类别]: 协议AT-测试类私有
     * [含义]: 设置侦听通道测试模式
     * [说明]: 此命令用于设置侦听通道的测试模式，便于产线测试。设置成功直接返回OK，设置失败返回错误信息。
     * [语法]:
     *     [命令]: ^RXTESTMODE=<cmd>
     *     [结果]: 设置执行成功时：
     *             <CR><LF>OK<CR><LF>
     *             设置执行错误情况：
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     * [参数]:
     *     <cmd>: 侦听通道测试模式，整数型，取值0，1。默认值为0。
     *             0：退出侦听测试模式；
     *             1：进入侦听测试模式；
     * [示例]:
     *     · 设置进入侦听测试模式成功
     *       AT^RXTESTMODE=1
     *       OK
     *     · 执行测试命令
     *       AT^RXTESTMODE=?
     *       ^RXTESTMODE: (0,1)
     *       OK
     */
    { AT_CMD_RXTESTMODE,
      AT_SetRxTestModePara, AT_SET_PARA_TIME, TAF_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (TAF_UINT8 *)"^RXTESTMODE", (TAF_UINT8 *)"(0,1)" },

#if (FEATURE_LTE == FEATURE_ON)
    /*
     * [类别]: 协议AT-LTE相关
     * [含义]: 查询LTE小区CA状态信息
     * [说明]: 用于查询LTE小区上行、下行CA配置状态、CA激活状态、Laa小区标识、Band频段、占用带宽及频点信息。
     *         此命令仅在LTE为主模，且驻留在LTE网络时查询有效。
     * [语法]:
     *     [命令]: ^LCACELLEX?
     *     [结果]: 执行成功时：
     *             [<CR><LF>^LCACELLEX: <cell_index>,<ul_cfg>, <dl_cfg>,<act_flg>,<laa_flg>,<band>,<band_width>, <earfcn>,<CR><LF> [<CR><LF>^LCACELLEX: <cell_index>,<ul_cfg>,<dl_cfg>,<act_flg>,<laa_flg>, <band>,<band_width>,<earfcn>,<CR><LF>[…]]] <CR><LF>OK<CR><LF>
     *             有MT 相关错误时：
     *             <CR><LF>+CME ERROR: <err_code><CR><LF>
     *     [命令]: ^LCACELLEX=?
     *     [结果]: <CR><LF>OK<CR><LF>
     * [参数]:
     *     <cell_index>: 整型值，LTE小区索引，0表示PCell，其余为SCell。
     *     <ul_cfg>: 整型值，该小区上行CA是否被配置：
     *             0：未配置；
     *             1：已配置。
     *     <dl_cfg>: 整型值，该小区下行CA是否被配置：
     *             0：未配置；
     *             1：已配置。
     *     <act_flg>: 整型值，该小区 CA是否被激活：
     *             0：未激活；
     *             1：已激活。
     *     <laa_flg>: 整型值，该小区是否为Laa小区：
     *             0：不是Laa小区；
     *             1：是Laa小区。
     *     <band>: 整型值，该小区的Band频段，如7代表Band VII。
     *     <band_width>: 整型值，该小区占用带宽：
     *             0：带宽为1.4MHz；
     *             1：带宽为3MHz；
     *             2：带宽为5MHz；
     *             3：带宽为10MHz；
     *             4：带宽为15MHz；
     *             5：带宽为20MHz。
     *     <earfcn>: 整型值，该小区频点。
     * [示例]:
     *     · UE驻留在LTE网络时查询小区CA状态信息
     *       AT^LCACELLEX?
     *       ^LCACELLEX: 0,1,1,1,0,7,2,21120
     *       ^LCACELLEX: 1,1,1,1,1,34,1,36230
     *       OK
     *     · 执行测试命令
     *       AT^LCACELLEX=?
     *       OK
     */
    { AT_CMD_LCACELLEX,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryLCaCellExPara, AT_QRY_PARA_TIME, At_CmdTestProcOK, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (TAF_UINT8 *)"^LCACELLEX", VOS_NULL_PTR },

    /*
     * [类别]: 协议AT-LTE相关
     * [含义]: 控制LTE小区CA信息主动上报
     * [说明]: 该命令用于控制UE在驻留LTE网络时，SCell激活状态变化、CA添加/释放等场景主动上报^LCACELLINFO命令，通知AP当前LTE模式下小区CA状态信息。
     * [语法]:
     *     [命令]: ^LCACELLRPTCFG=<enable>
     *     [结果]: 执行成功时：
     *             <CR><LF>OK<CR><LF>
     *             有MT 相关错误时：
     *             <CR><LF>+CME ERROR: <err_code><CR><LF>
     *     [命令]: ^LCACELLRPTCFG?
     *     [结果]: 执行成功时：
     *             <CR><LF>^LCACELLRPTCFG: <enable><CR><LF> <CR><LF>OK<CR><LF>
     *             有MT 相关错误时：
     *             <CR><LF>+CME ERROR: <err_code><CR><LF>
     *     [命令]: ^LCACELLRPTCFG=?
     *     [结果]: <CR><LF>^LCACELLRPTCFG:(0,1)<CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [参数]:
     *     <enable>: 整型值，主动上报^LCACELLINFO命令的开关：
     *             0：关闭^LCACELLINFO命令的主动上报；
     *             1：打开^LCACELLINFO命令的主动上报。
     * [示例]:
     *     · 打开^LCACELLINFO命令的主动上报
     *       AT^LCACELLRPTCFG=1
     *       OK
     *     · 查询^LCACELLINFO命令的主动上报开关状态
     *       AT^LCACELLRPTCFG?
     *       ^LCACELLRPTCFG: 1
     *       OK
     *     · 执行测试命令
     *       AT^LCACELLRPTCFG=?
     *       ^LCACELLRPTCFG: (0,1)
     *       OK
     */
    { AT_CMD_LCACELLRPTCFG,
      AT_SetLcaCellRptCfgPara, AT_SET_PARA_TIME, AT_QryLcaCellRptCfgPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (TAF_UINT8 *)"^LCACELLRPTCFG", (TAF_UINT8 *)"(0,1)" },
#endif

    /*
     * [类别]: 协议AT-LTE相关
     * [含义]: LTE小区CA状态查询
     * [说明]: 用于查询LTE小区上行、下行CA配置状态和CA激活状态。
     *         同时显示主小区和多个辅小区的CA状态。
     *         终端应该保证在LTE模下使用该命令，在其他模下，不要下发该查询。
     *         如果在2G/3G模式下或LTE未驻留时，Modem收到该查询命令，则返回所有CA小区均未配置、未激活状态。
     * [语法]:
     *     [命令]: ^LCACELL?
     *     [结果]: <CR><LF>^ LCACELL: "<cell_id> <ul_cfg> <dl_cfg> <act>"[, "<cell_id> <ul_cfg> <dl_cfg> <act>"[,......]]<CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>ERROR<CR><LF>
     * [参数]:
     *     <cell_id>: 整型值，cell id，0表示Pcell，其他表示Scell。
     *     <ul_cfg>: 整型值，本cell上行CA是否被配置，0表示未配置，1表示已配置。
     *     <dl_cfg>: 整型值，本cell下行CA是否被配置，0表示未配置，1表示已配置。
     *     <act>: 整型值，本cell CA是否被激活，0表示未激活，1表示已激活。
     * [示例]:
     *     · 查询LTE小区CA配置状态
     *       AT^LCACELL?
     *       ^LCACELL: "0 0 0 0", "1 1 1 1", "2 1 1 1", "3 0 0 0", "4 0 0 0", "5 0 0 0", "6 0 0 0", "7 0 0 0"
     *       OK
     */
    { AT_CMD_LCACELL,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryLcacellPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (VOS_UINT8 *)"^LCACELL", VOS_NULL_PTR },

    /*
     * [类别]: 协议AT-LTE相关
     * [含义]: 设置UE版本号
     * [说明]: 设置UE协议版本号，把版本号信息写入NV项，在下一次开机后生效。
     * [语法]:
     *     [命令]: ^RADVER=<mod>,<ver>
     *     [结果]: <CR><LF>OK<CR><LF>
     *             错误情况：
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     *     [命令]: ^RADVER=?
     *     [结果]: <CR><LF>^RADVER: (list of supported <mod>s),(list of supported < ver>s)<CR><LF><CR><LF>OK<CR><LF>
     * [参数]:
     *     <mod>: 整型值，目前只支持2(LTE)。取值范围为(0-3)，未描述的值都是预留值。
     *             0：GSM；
     *             1：WCDMA；
     *             2：LTE。
     *     <ver>: 整型值，协议版本号，目前只支持如下描述值。取值范围为(4-16)，未描述的值都是预留值。
     *             9：release version 9;
     *             10：release version 10;
     *             11：release version 11;
     *             12：release version 12;
     *             13：release version 13;
     *             14：release version 14;
     *             15：release version 15。
     * [示例]:
     *     · 设置LTE协议版本号为version 9
     *       AT^RADVER=2,9
     *       OK
     */
    { AT_CMD_RADVER,
      AT_SetRadverPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (VOS_UINT8 *)"^RADVER", (VOS_UINT8 *)"(0-3),(4-16)" },

    /*
     * [类别]: 协议AT-测试类私有
     * [含义]: 设置脉冲下发数量配置信息
     * [说明]: 本命令用于产线校准场景下，通知LPHY强制输出n个脉冲。
     *         此命令在非信令模式（AT^TMODE=1）下使用，其它模式下返回错误码0。并且必须在LTE主模下发此命令，否则返回ERROR。
     * [语法]:
     *     [命令]: ^FORCESYNC=n
     *     [结果]: 执行设置成功时：
     *             <CR><LF>OK<CR><LF>
     *             执行错误时：
     *             <CR><LF>+CME ERROR: <err_code><CR><LF>
     * [参数]:
     *     <n>: LPHY需要下发的脉冲个数，取值范围1~256。
     * [示例]:
     *     · 设置脉冲下发数量配置信息
     *       AT^FORCESYNC=100
     *       OK
     */
    { AT_CMD_FORCESYNC,
      AT_SetForceSyncPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_NO_LIMITED,
      (TAF_UINT8 *)"^FORCESYNC", (VOS_UINT8 *)"(1-256)" },





    /*
     * [类别]: 装备AT-GUC装备
     * [含义]: 读取寄存器PDEG的值
     * [说明]: 该命令用于读取PDEG的值。
     * [语法]:
     *     [命令]: ^FPOWDET?
     *     [结果]: 执行成功时：
     *             <CR><LF>^FPOWDET: <value><CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             错误情况时返回：
     *             <CR><LF>ERROR<CR><LF>
     *             若是GSM下的调制信号：
     *             <CR><LF>^FPOWDET: <value1>[,<value2>[,<value3>[,<value4>[,<value5>,<value6>,<value7>,<value8>]]]]<CR><LF>
     *             <CR><LF>OK<CR><LF>
     *             错误情况时返回：
     *             <CR><LF>ERROR<CR><LF>
     * [参数]:
     *     <value>: PDEG的值，
     * [示例]:
     *     · 执行查询命令
     *       AT^FPOWDET?
     *       ^FPOWDET: 10
     *       OK
     */
    { AT_CMD_FPOWDET,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryFpowdetTPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^FPOWDET", VOS_NULL_PTR },


};

/* 注册PHY装备AT命令表 */
VOS_UINT32 AT_RegisterDevicePhyCmdTable(VOS_VOID)
{
    return AT_RegisterCmdTable(g_atDevicePhyCmdTbl, sizeof(g_atDevicePhyCmdTbl) / sizeof(g_atDevicePhyCmdTbl[0]));
}

