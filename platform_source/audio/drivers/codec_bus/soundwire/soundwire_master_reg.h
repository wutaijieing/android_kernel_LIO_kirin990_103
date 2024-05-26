/*
 * soundwire_master_reg.h -- soundwire master reg
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/

#ifndef __SOUNDWIRE_MASTER_REG_H__
#define __SOUNDWIRE_MASTER_REG_H__

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif



/*****************************************************************************
  2 宏定义
*****************************************************************************/

/****************************************************************************
                     (1/1) SOUNDWIRE_HOST_REG
 ****************************************************************************/
/* 寄存器说明：DP0中断状态寄存器。
   位域定义UNION结构:  MST_DP0_INTSTAT_UNION */
#define MST_DP0_INTSTAT_OFFSET                        0x00

/* 寄存器说明：DP0中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP0_INTMASK_UNION */
#define MST_DP0_INTMASK_OFFSET                        0x01

/* 寄存器说明：DP0的端口控制寄存器。
   位域定义UNION结构:  MST_DP0_PORTCTRL_UNION */
#define MST_DP0_PORTCTRL_OFFSET                       0x02

/* 寄存器说明：DP0 Block控制寄存器。
   位域定义UNION结构:  MST_DP0_BLOCKCTRL1_UNION */
#define MST_DP0_BLOCKCTRL1_OFFSET                     0x03

/* 寄存器说明：DP0的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP0_PREPARESTATUS_UNION */
#define MST_DP0_PREPARESTATUS_OFFSET                  0x04

/* 寄存器说明：DP0的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP0_PREPARECTRL_UNION */
#define MST_DP0_PREPARECTRL_OFFSET                    0x05

/* 寄存器说明：DP0的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP0_CHANNELEN_BANK0_UNION */
#define MST_DP0_CHANNELEN_BANK0_OFFSET                0x20

/* 寄存器说明：DP0的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP0_SAMPLECTRL1_BANK0_UNION */
#define MST_DP0_SAMPLECTRL1_BANK0_OFFSET              0x22

/* 寄存器说明：DP0的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP0_SAMPLECTRL2_BANK0_UNION */
#define MST_DP0_SAMPLECTRL2_BANK0_OFFSET              0x23

/* 寄存器说明：DP0的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP0_OFFSETCTRL1_BANK0_UNION */
#define MST_DP0_OFFSETCTRL1_BANK0_OFFSET              0x24

/* 寄存器说明：DP0的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP0_OFFSETCTRL2_BANK0_UNION */
#define MST_DP0_OFFSETCTRL2_BANK0_OFFSET              0x25

/* 寄存器说明：DP0的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP0_HCTRL_BANK0_UNION */
#define MST_DP0_HCTRL_BANK0_OFFSET                    0x26

/* 寄存器说明：DP0的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP0_LANECTRL_BANK0_UNION */
#define MST_DP0_LANECTRL_BANK0_OFFSET                 0x28

/* 寄存器说明：DP0的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP0_CHANNELEN_BANK1_UNION */
#define MST_DP0_CHANNELEN_BANK1_OFFSET                0x30

/* 寄存器说明：DP0的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP0_SAMPLECTRL1_BANK1_UNION */
#define MST_DP0_SAMPLECTRL1_BANK1_OFFSET              0x32

/* 寄存器说明：DP0的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP0_SAMPLECTRL2_BANK1_UNION */
#define MST_DP0_SAMPLECTRL2_BANK1_OFFSET              0x33

/* 寄存器说明：DP0的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP0_OFFSETCTRL1_BANK1_UNION */
#define MST_DP0_OFFSETCTRL1_BANK1_OFFSET              0x34

/* 寄存器说明：DP0的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP0_OFFSETCTRL2_BANK1_UNION */
#define MST_DP0_OFFSETCTRL2_BANK1_OFFSET              0x35

/* 寄存器说明：DP0的传输子帧参数控制寄存器(bank1。)
   位域定义UNION结构:  MST_DP0_HCTRL_BANK1_UNION */
#define MST_DP0_HCTRL_BANK1_OFFSET                    0x36

/* 寄存器说明：DP0的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP0_LANECTRL_BANK1_UNION */
#define MST_DP0_LANECTRL_BANK1_OFFSET                 0x38

/* 寄存器说明：data_port屏蔽后中断状态寄存器1。
   位域定义UNION结构:  MST_DP_IntStat1_UNION */
#define MST_DP_IntStat1_OFFSET                        0x40

/* 寄存器说明：data_port屏蔽后中断状态寄存器2。
   位域定义UNION结构:  MST_DP_IntStat2_UNION */
#define MST_DP_IntStat2_OFFSET                        0x42

/* 寄存器说明：data_port屏蔽后中断状态寄存器3。
   位域定义UNION结构:  MST_DP_IntStat3_UNION */
#define MST_DP_IntStat3_OFFSET                        0x43

/* 寄存器说明：Slave devicenumber配置寄存器（已删除）。
   位域定义UNION结构:  MST_SCP_DEVNUMBER_UNION */
#define MST_SCP_DEVNUMBER_OFFSET                      0x46

/* 寄存器说明：帧形状配置寄存器(bank0)。
   位域定义UNION结构:  MST_SCP_FRAMECTRL_BANK0_UNION */
#define MST_SCP_FRAMECTRL_BANK0_OFFSET                0x60

/* 寄存器说明：帧形状配置寄存器(bank1)。
   位域定义UNION结构:  MST_SCP_FRAMECTRL_BANK1_UNION */
#define MST_SCP_FRAMECTRL_BANK1_OFFSET                0x70

/* 寄存器说明：时钟门控配置寄存器1。
   位域定义UNION结构:  MST_CLKEN1_UNION */
#define MST_CLKEN1_OFFSET                   0x80

/* 寄存器说明：时钟门控配置寄存器2。
   位域定义UNION结构:  MST_CLKEN2_UNION */
#define MST_CLKEN2_OFFSET                   0x81

/* 寄存器说明：时钟门控配置寄存器3。
   位域定义UNION结构:  MST_CLKEN3_UNION */
#define MST_CLKEN3_OFFSET                   0x82

/* 寄存器说明：时钟门控配置寄存器4。
   位域定义UNION结构:  MST_CLKEN4_UNION */
#define MST_CLKEN4_OFFSET                   0x83

/* 寄存器说明：时钟门控配置寄存器5(删除)。
   位域定义UNION结构:  MST_CLKEN5_UNION */
#define MST_CLKEN5_OFFSET                   0x84

/* 寄存器说明：时钟门控配置寄存器6。
   位域定义UNION结构:  MST_CLKEN6_UNION */
#define MST_CLKEN6_OFFSET                   0x85

/* 寄存器说明：时钟门控配置寄存器7。
   位域定义UNION结构:  MST_CLKEN7_UNION */
#define MST_CLKEN7_OFFSET                   0x86

/* 寄存器说明：时钟门控配置寄存器8。
   位域定义UNION结构:  MST_CLKEN8_UNION */
#define MST_CLKEN8_OFFSET                   0x87

/* 寄存器说明：软复位配置寄存器1。
   位域定义UNION结构:  MST_RSTEN1_UNION */
#define MST_RSTEN1_OFFSET                   0x88

/* 寄存器说明：软复位配置寄存器2。
   位域定义UNION结构:  MST_RSTEN2_UNION */
#define MST_RSTEN2_OFFSET                   0x89

/* 寄存器说明：软复位配置寄存器3。
   位域定义UNION结构:  MST_RSTEN3_UNION */
#define MST_RSTEN3_OFFSET                   0x8A

/* 寄存器说明：软复位配置寄存器4(删除)。
   位域定义UNION结构:  MST_RSTEN4_UNION */
#define MST_RSTEN4_OFFSET                   0x8B

/* 寄存器说明：软复位配置寄存器5(删除)。
   位域定义UNION结构:  MST_RSTEN5_UNION */
#define MST_RSTEN5_OFFSET                   0x8C

/* 寄存器说明：软复位配置寄存器6。
   位域定义UNION结构:  MST_RSTEN6_UNION */
#define MST_RSTEN6_OFFSET                   0x8D

/* 寄存器说明：软复位配置寄存器7。
   位域定义UNION结构:  MST_RSTEN7_UNION */
#define MST_RSTEN7_OFFSET                   0x8E

/* 寄存器说明：软复位配置寄存器8。
   位域定义UNION结构:  MST_RSTEN8_UNION */
#define MST_RSTEN8_OFFSET                   0x8F

/* 寄存器说明：时钟分频控制寄存器。
   位域定义UNION结构:  MST_CLKDIVCTRL_UNION */
#define MST_CLKDIVCTRL_OFFSET               0x90

/* 寄存器说明：时钟唤醒&SSP配置寄存器。
   位域定义UNION结构:  MST_CLK_WAKEUP_EN_UNION */
#define MST_CLK_WAKEUP_EN_OFFSET            0x91

/* 寄存器说明：SYSCTRL内部状态机指示信号。
   位域定义UNION结构:  MST_TEST_MONITOR_UNION */
#define MST_TEST_MONITOR_OFFSET             0x92

/* 寄存器说明：BANK切换指示状态信号。
   位域定义UNION结构:  MST_BANK_SW_MONITOR_UNION */
#define MST_BANK_SW_MONITOR_OFFSET          0x93

/* 寄存器说明：slave寄存器访问选择。
   位域定义UNION结构:  MST_SLV_REG_SEL_UNION */
#define MST_SLV_REG_SEL_OFFSET              0x9F

/* 寄存器说明：环回通路配置寄存器。
   位域定义UNION结构:  MST_FIFO_LOOP_EN_UNION */
#define MST_FIFO_LOOP_EN_OFFSET             0xA0

/* 寄存器说明：子模块控制配置寄存器1。
   位域定义UNION结构:  MST_CTRL1_UNION */
#define MST_CTRL1_OFFSET                    0xA1

/* 寄存器说明：子模块控制配置寄存器2。
   位域定义UNION结构:  MST_CTRL2_UNION */
#define MST_CTRL2_OFFSET                    0xA2

/* 寄存器说明：DATA_PORT方向配置寄存器2。
   位域定义UNION结构:  MST_SOURCESINK_CFG2_UNION */
#define MST_SOURCESINK_CFG2_OFFSET          0xA3

/* 寄存器说明：DATA_PORT大小端配置寄存器1。
   位域定义UNION结构:  MST_DATAPORT_CFG1_UNION */
#define MST_DATAPORT_CFG1_OFFSET            0xA4

/* 寄存器说明：DATA_PORT大小端配置寄存器2。
   位域定义UNION结构:  MST_DATAPORT_CFG2_UNION */
#define MST_DATAPORT_CFG2_OFFSET            0xA5

/* 寄存器说明：DATA_PORT FIFO环回使能1(可配DP1~9)。
   位域定义UNION结构:  MST_DATAPORT_FIFO_CFG1_UNION */
#define MST_DATAPORT_FIFO_CFG1_OFFSET       0xA6

/* 寄存器说明：DATA_PORT FIFO环回使能2。
   位域定义UNION结构:  MST_DATAPORT_FIFO_CFG2_UNION */
#define MST_DATAPORT_FIFO_CFG2_OFFSET       0xA7

/* 寄存器说明：DATA_PORT 可维可测选择。
   位域定义UNION结构:  MST_DATAPORT_TEST_PROC_SEL_UNION */
#define MST_DATAPORT_TEST_PROC_SEL_OFFSET   0xA8

/* 寄存器说明：SOUNDWIRE同步超时等待时间配置。
   位域定义UNION结构:  MST_SYNC_TIMEOUT_UNION */
#define MST_SYNC_TIMEOUT_OFFSET             0xA9

/* 寄存器说明：SOUNDWIRE枚举超时等待时间配置。
   位域定义UNION结构:  MST_ENUM_TIMEOUT_UNION */
#define MST_ENUM_TIMEOUT_OFFSET             0xAA

/* 寄存器说明：SOUNDWIRE中断状态寄存器1。
   位域定义UNION结构:  MST_INTSTAT1_UNION */
#define MST_INTSTAT1_OFFSET                 0xC0

/* 寄存器说明：SOUNDWIRE中断状态屏蔽寄存器1。
   位域定义UNION结构:  MST_INTMASK1_UNION */
#define MST_INTMASK1_OFFSET                 0xC1

/* 寄存器说明：SOUNDWIRE中断状态屏蔽后状态寄存器1。
   位域定义UNION结构:  MST_INTMSTAT1_UNION */
#define MST_INTMSTAT1_OFFSET                0xC2

/* 寄存器说明：SOUNDWIRE中断状态寄存器2。
   位域定义UNION结构:  MST_INTSTAT2_UNION */
#define MST_INTSTAT2_OFFSET                 0xC3

/* 寄存器说明：SOUNDWIRE中断状态屏蔽寄存器2。
   位域定义UNION结构:  MST_INTMASK2_UNION */
#define MST_INTMASK2_OFFSET                 0xC4

/* 寄存器说明：SOUNDWIRE中断状态屏蔽寄存器2。
   位域定义UNION结构:  MST_INTMSTAT2_UNION */
#define MST_INTMSTAT2_OFFSET                0xC5

/* 寄存器说明：SOUNDWIRE中断状态寄存器3。
   位域定义UNION结构:  MST_INTSTAT3_UNION */
#define MST_INTSTAT3_OFFSET                 0xC6

/* 寄存器说明：SOUNDWIRE中断状态屏蔽寄存器3。
   位域定义UNION结构:  MST_INTMASK3_UNION */
#define MST_INTMASK3_OFFSET                 0xC7

/* 寄存器说明：SOUNDWIRE中断状态寄存器3。
   位域定义UNION结构:  MST_INTMSTAT3_UNION */
#define MST_INTMSTAT3_OFFSET                0xC8

/* 寄存器说明：SOUNDWIRE  Device 状态寄存器0
   位域定义UNION结构:  MST_SLV_STAT0_UNION */
#define MST_SLV_STAT0_OFFSET                0xC9

/* 寄存器说明：SOUNDWIRE  Device 状态寄存器1
   位域定义UNION结构:  MST_SLV_STAT1_UNION */
#define MST_SLV_STAT1_OFFSET                0xCA

/* 寄存器说明：SOUNDWIRE  Device 状态寄存器2
   位域定义UNION结构:  MST_SLV_STAT2_UNION */
#define MST_SLV_STAT2_OFFSET                0xCB

/* 寄存器说明：DP_FIFO清空寄存器1
   位域定义UNION结构:  MST_DP_FIFO_CLR1_UNION */
#define MST_DP_FIFO_CLR1_OFFSET                       0xCC

/* 寄存器说明：DP_FIFO清空寄存器1
   位域定义UNION结构:  MST_DP_FIFO_CLR2_UNION */
#define MST_DP_FIFO_CLR2_OFFSET                       0xCD

/* 寄存器说明：可维可测输出观测只读寄存器0。
   位域定义UNION结构:  MST_DEBUG_OUT_0_UNION */
#define MST_DEBUG_OUT_0_OFFSET                        0xDF

/* 寄存器说明：可维可测输出观测只读寄存器1。
   位域定义UNION结构:  MST_DEBUG_OUT_1_UNION */
#define MST_DEBUG_OUT_1_OFFSET                        0xE0

/* 寄存器说明：可维可测输出观测只读寄存器2。
   位域定义UNION结构:  MST_DEBUG_OUT_2_UNION */
#define MST_DEBUG_OUT_2_OFFSET                        0xE1

/* 寄存器说明：可维可测输出观测只读寄存器3。
   位域定义UNION结构:  MST_DEBUG_OUT_3_UNION */
#define MST_DEBUG_OUT_3_OFFSET                        0xE2

/* 寄存器说明：可维可测输出观测只读寄存器4。
   位域定义UNION结构:  MST_DEBUG_OUT_4_UNION */
#define MST_DEBUG_OUT_4_OFFSET                        0xE3

/* 寄存器说明：可维可测锁存输出选择配置寄存器0。
   位域定义UNION结构:  MST_DEBUG_OUT_SEL0_UNION */
#define MST_DEBUG_OUT_SEL0_OFFSET                     0xE4

/* 寄存器说明：可维可测输出选择配置寄存器1。
   位域定义UNION结构:  MST_DEBUG_OUT_SEL1_UNION */
#define MST_DEBUG_OUT_SEL1_OFFSET                     0xE5

/* 寄存器说明：可维可测输出选择配置寄存器2。
   位域定义UNION结构:  MST_DEBUG_OUT_SEL2_UNION */
#define MST_DEBUG_OUT_SEL2_OFFSET                     0xE6

/* 寄存器说明：DP_FIFO水线配置寄存器。
   位域定义UNION结构:  MST_DP1_FIFO_THRE_CTRL_UNION */
#define MST_DP1_FIFO_THRE_CTRL_OFFSET                 0xE7

/* 寄存器说明：DP_FIFO水线配置寄存器1。
   位域定义UNION结构:  MST_DP_FIFO_THRE_CTRL1_UNION */
#define MST_DP_FIFO_THRE_CTRL1_OFFSET                 0xE8

/* 寄存器说明：DP_FIFO水线配置寄存器2。
   位域定义UNION结构:  MST_DP_FIFO_THRE_CTRL2_UNION */
#define MST_DP_FIFO_THRE_CTRL2_OFFSET                 0xE9

/* 寄存器说明：dp0_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP0_FIFO_STAT_UNION */
#define MST_DP0_FIFO_STAT_OFFSET                      0xF5

/* 寄存器说明：dp12_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP12_FIFO_INTSTAT_UNION */
#define MST_DP12_FIFO_INTSTAT_OFFSET                  0xF6

/* 寄存器说明：dp34_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP34_FIFO_INTSTAT_UNION */
#define MST_DP34_FIFO_INTSTAT_OFFSET                  0xF7

/* 寄存器说明：dp56_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP56_FIFO_INTSTAT_UNION */
#define MST_DP56_FIFO_INTSTAT_OFFSET                  0xF8

/* 寄存器说明：dp78_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP78_FIFO_INTSTAT_UNION */
#define MST_DP78_FIFO_INTSTAT_OFFSET                  0xF9

/* 寄存器说明：dp910_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP910_FIFO_INTSTAT_UNION */
#define MST_DP910_FIFO_INTSTAT_OFFSET                 0xFA

/* 寄存器说明：dp1112_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP1112_FIFO_INTSTAT_UNION */
#define MST_DP1112_FIFO_INTSTAT_OFFSET                0xFB

/* 寄存器说明：dp1314_fifo空满状态寄存器。
   位域定义UNION结构:  MST_DP1314_FIFO_INTSTAT_UNION */
#define MST_DP1314_FIFO_INTSTAT_OFFSET                0xFC

/* 寄存器说明：SOUNDWIRE debug配置寄存器
   位域定义UNION结构:  MST_DBG_UNION */
#define MST_DBG_OFFSET                      0x0FE

/* 寄存器说明：版本寄存器。
   位域定义UNION结构:  MST_VERSION_UNION */
#define MST_VERSION_OFFSET                  0x0FF

/* 寄存器说明：dp1中断状态寄存器。
   位域定义UNION结构:  MST_DP1_INTSTAT_UNION */
#define MST_DP1_INTSTAT_OFFSET                        0x100

/* 寄存器说明：dp1中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP1_INTMASK_UNION */
#define MST_DP1_INTMASK_OFFSET                        0x101

/* 寄存器说明：dp1的端口控制寄存器。
   位域定义UNION结构:  MST_DP1_PORTCTRL_UNION */
#define MST_DP1_PORTCTRL_OFFSET                       0x102

/* 寄存器说明：dp1 Block1控制寄存器。
   位域定义UNION结构:  MST_DP1_BLOCKCTRL1_UNION */
#define MST_DP1_BLOCKCTRL1_OFFSET                     0x103

/* 寄存器说明：dp1的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP1_PREPARESTATUS_UNION */
#define MST_DP1_PREPARESTATUS_OFFSET                  0x104

/* 寄存器说明：dp1的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP1_PREPARECTRL_UNION */
#define MST_DP1_PREPARECTRL_OFFSET                    0x105

/* 寄存器说明：dp1的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_CHANNELEN_BANK0_UNION */
#define MST_DP1_CHANNELEN_BANK0_OFFSET                0x120

/* 寄存器说明：dp1 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_BLOCKCTRL2_BANK0_UNION */
#define MST_DP1_BLOCKCTRL2_BANK0_OFFSET               0x121

/* 寄存器说明：dp1的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_SAMPLECTRL1_BANK0_UNION */
#define MST_DP1_SAMPLECTRL1_BANK0_OFFSET              0x122

/* 寄存器说明：dp1的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_SAMPLECTRL2_BANK0_UNION */
#define MST_DP1_SAMPLECTRL2_BANK0_OFFSET              0x123

/* 寄存器说明：dp1的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP1_OFFSETCTRL1_BANK0_UNION */
#define MST_DP1_OFFSETCTRL1_BANK0_OFFSET              0x124

/* 寄存器说明：dp1的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP1_OFFSETCTRL2_BANK0_UNION */
#define MST_DP1_OFFSETCTRL2_BANK0_OFFSET              0x125

/* 寄存器说明：dp1的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_HCTRL_BANK0_UNION */
#define MST_DP1_HCTRL_BANK0_OFFSET                    0x126

/* 寄存器说明：dp1 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_BLOCKCTRL3_BANK0_UNION */
#define MST_DP1_BLOCKCTRL3_BANK0_OFFSET               0x127

/* 寄存器说明：dp1的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP1_LANECTRL_BANK0_UNION */
#define MST_DP1_LANECTRL_BANK0_OFFSET                 0x128

/* 寄存器说明：dp1的通道使能寄存器(bank1)(仅channel1\2有效，其他勿配，下同)。
   位域定义UNION结构:  MST_DP1_CHANNELEN_BANK1_UNION */
#define MST_DP1_CHANNELEN_BANK1_OFFSET                0x130

/* 寄存器说明：dp1 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP1_BLOCKCTRL2_BANK1_UNION */
#define MST_DP1_BLOCKCTRL2_BANK1_OFFSET               0x131

/* 寄存器说明：dp1的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP1_SAMPLECTRL1_BANK1_UNION */
#define MST_DP1_SAMPLECTRL1_BANK1_OFFSET              0x132

/* 寄存器说明：dp1的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP1_SAMPLECTRL2_BANK1_UNION */
#define MST_DP1_SAMPLECTRL2_BANK1_OFFSET              0x133

/* 寄存器说明：dp1的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP1_OFFSETCTRL1_BANK1_UNION */
#define MST_DP1_OFFSETCTRL1_BANK1_OFFSET              0x134

/* 寄存器说明：dp1的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP1_OFFSETCTRL2_BANK1_UNION */
#define MST_DP1_OFFSETCTRL2_BANK1_OFFSET              0x135

/* 寄存器说明：dp1的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP1_HCTRL_BANK1_UNION */
#define MST_DP1_HCTRL_BANK1_OFFSET                    0x136

/* 寄存器说明：dp1 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP1_BLOCKCTRL3_BANK1_UNION */
#define MST_DP1_BLOCKCTRL3_BANK1_OFFSET               0x137

/* 寄存器说明：dp1的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP1_LANECTRL_BANK1_UNION */
#define MST_DP1_LANECTRL_BANK1_OFFSET                 0x138

/* 寄存器说明：dp2中断状态寄存器。
   位域定义UNION结构:  MST_DP2_INTSTAT_UNION */
#define MST_DP2_INTSTAT_OFFSET                        0x200

/* 寄存器说明：dp2中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP2_INTMASK_UNION */
#define MST_DP2_INTMASK_OFFSET                        0x201

/* 寄存器说明：dp2的端口控制寄存器。
   位域定义UNION结构:  MST_DP2_PORTCTRL_UNION */
#define MST_DP2_PORTCTRL_OFFSET                       0x202

/* 寄存器说明：dp2 Block1控制寄存器。
   位域定义UNION结构:  MST_DP2_BLOCKCTRL1_UNION */
#define MST_DP2_BLOCKCTRL1_OFFSET                     0x203

/* 寄存器说明：dp2的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP2_PREPARESTATUS_UNION */
#define MST_DP2_PREPARESTATUS_OFFSET                  0x204

/* 寄存器说明：dp2的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP2_PREPARECTRL_UNION */
#define MST_DP2_PREPARECTRL_OFFSET                    0x205

/* 寄存器说明：dp2的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_CHANNELEN_BANK0_UNION */
#define MST_DP2_CHANNELEN_BANK0_OFFSET                0x220

/* 寄存器说明：dp2 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_BLOCKCTRL2_BANK0_UNION */
#define MST_DP2_BLOCKCTRL2_BANK0_OFFSET               0x221

/* 寄存器说明：dp2的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_SAMPLECTRL1_BANK0_UNION */
#define MST_DP2_SAMPLECTRL1_BANK0_OFFSET              0x222

/* 寄存器说明：dp2的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_SAMPLECTRL2_BANK0_UNION */
#define MST_DP2_SAMPLECTRL2_BANK0_OFFSET              0x223

/* 寄存器说明：dp2的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP2_OFFSETCTRL1_BANK0_UNION */
#define MST_DP2_OFFSETCTRL1_BANK0_OFFSET              0x224

/* 寄存器说明：dp2的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP2_OFFSETCTRL2_BANK0_UNION */
#define MST_DP2_OFFSETCTRL2_BANK0_OFFSET              0x225

/* 寄存器说明：dp2的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_HCTRL_BANK0_UNION */
#define MST_DP2_HCTRL_BANK0_OFFSET                    0x226

/* 寄存器说明：dp2 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_BLOCKCTRL3_BANK0_UNION */
#define MST_DP2_BLOCKCTRL3_BANK0_OFFSET               0x227

/* 寄存器说明：dp2的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP2_LANECTRL_BANK0_UNION */
#define MST_DP2_LANECTRL_BANK0_OFFSET                 0x228

/* 寄存器说明：dp2的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_CHANNELEN_BANK1_UNION */
#define MST_DP2_CHANNELEN_BANK1_OFFSET                0x230

/* 寄存器说明：dp2 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_BLOCKCTRL2_BANK1_UNION */
#define MST_DP2_BLOCKCTRL2_BANK1_OFFSET               0x231

/* 寄存器说明：dp2的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_SAMPLECTRL1_BANK1_UNION */
#define MST_DP2_SAMPLECTRL1_BANK1_OFFSET              0x232

/* 寄存器说明：dp2的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_SAMPLECTRL2_BANK1_UNION */
#define MST_DP2_SAMPLECTRL2_BANK1_OFFSET              0x233

/* 寄存器说明：dp2的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP2_OFFSETCTRL1_BANK1_UNION */
#define MST_DP2_OFFSETCTRL1_BANK1_OFFSET              0x234

/* 寄存器说明：dp2的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP2_OFFSETCTRL2_BANK1_UNION */
#define MST_DP2_OFFSETCTRL2_BANK1_OFFSET              0x235

/* 寄存器说明：dp2的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_HCTRL_BANK1_UNION */
#define MST_DP2_HCTRL_BANK1_OFFSET                    0x236

/* 寄存器说明：dp2 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_BLOCKCTRL3_BANK1_UNION */
#define MST_DP2_BLOCKCTRL3_BANK1_OFFSET               0x237

/* 寄存器说明：dp2的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP2_LANECTRL_BANK1_UNION */
#define MST_DP2_LANECTRL_BANK1_OFFSET                 0x238

/* 寄存器说明：dp3中断状态寄存器。
   位域定义UNION结构:  MST_DP3_INTSTAT_UNION */
#define MST_DP3_INTSTAT_OFFSET                        0x300

/* 寄存器说明：dp3中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP3_INTMASK_UNION */
#define MST_DP3_INTMASK_OFFSET                        0x301

/* 寄存器说明：dp3的端口控制寄存器。
   位域定义UNION结构:  MST_DP3_PORTCTRL_UNION */
#define MST_DP3_PORTCTRL_OFFSET                       0x302

/* 寄存器说明：dp3 Block1控制寄存器。
   位域定义UNION结构:  MST_DP3_BLOCKCTRL1_UNION */
#define MST_DP3_BLOCKCTRL1_OFFSET                     0x303

/* 寄存器说明：dp3的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP3_PREPARESTATUS_UNION */
#define MST_DP3_PREPARESTATUS_OFFSET                  0x304

/* 寄存器说明：dp3的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP3_PREPARECTRL_UNION */
#define MST_DP3_PREPARECTRL_OFFSET                    0x305

/* 寄存器说明：dp3的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_CHANNELEN_BANK0_UNION */
#define MST_DP3_CHANNELEN_BANK0_OFFSET                0x320

/* 寄存器说明：dp3 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_BLOCKCTRL2_BANK0_UNION */
#define MST_DP3_BLOCKCTRL2_BANK0_OFFSET               0x321

/* 寄存器说明：dp3的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_SAMPLECTRL1_BANK0_UNION */
#define MST_DP3_SAMPLECTRL1_BANK0_OFFSET              0x322

/* 寄存器说明：dp3的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_SAMPLECTRL2_BANK0_UNION */
#define MST_DP3_SAMPLECTRL2_BANK0_OFFSET              0x323

/* 寄存器说明：dp3的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP3_OFFSETCTRL1_BANK0_UNION */
#define MST_DP3_OFFSETCTRL1_BANK0_OFFSET              0x324

/* 寄存器说明：dp3的Offset控制寄存器高8位(bank0)
   位域定义UNION结构:  MST_DP3_OFFSETCTRL2_BANK0_UNION */
#define MST_DP3_OFFSETCTRL2_BANK0_OFFSET              0x325

/* 寄存器说明：dp3的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_HCTRL_BANK0_UNION */
#define MST_DP3_HCTRL_BANK0_OFFSET                    0x326

/* 寄存器说明：dp3 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_BLOCKCTRL3_BANK0_UNION */
#define MST_DP3_BLOCKCTRL3_BANK0_OFFSET               0x327

/* 寄存器说明：dp3的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP3_LANECTRL_BANK0_UNION */
#define MST_DP3_LANECTRL_BANK0_OFFSET                 0x328

/* 寄存器说明：dp3的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_CHANNELEN_BANK1_UNION */
#define MST_DP3_CHANNELEN_BANK1_OFFSET                0x330

/* 寄存器说明：dp3 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_BLOCKCTRL2_BANK1_UNION */
#define MST_DP3_BLOCKCTRL2_BANK1_OFFSET               0x331

/* 寄存器说明：dp3的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_SAMPLECTRL1_BANK1_UNION */
#define MST_DP3_SAMPLECTRL1_BANK1_OFFSET              0x332

/* 寄存器说明：dp3的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_SAMPLECTRL2_BANK1_UNION */
#define MST_DP3_SAMPLECTRL2_BANK1_OFFSET              0x333

/* 寄存器说明：dp3的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP3_OFFSETCTRL1_BANK1_UNION */
#define MST_DP3_OFFSETCTRL1_BANK1_OFFSET              0x334

/* 寄存器说明：dp3的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP3_OFFSETCTRL2_BANK1_UNION */
#define MST_DP3_OFFSETCTRL2_BANK1_OFFSET              0x335

/* 寄存器说明：dp3的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_HCTRL_BANK1_UNION */
#define MST_DP3_HCTRL_BANK1_OFFSET                    0x336

/* 寄存器说明：dp3 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_BLOCKCTRL3_BANK1_UNION */
#define MST_DP3_BLOCKCTRL3_BANK1_OFFSET               0x337

/* 寄存器说明：dp3的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP3_LANECTRL_BANK1_UNION */
#define MST_DP3_LANECTRL_BANK1_OFFSET                 0x338

/* 寄存器说明：dp4中断状态寄存器。
   位域定义UNION结构:  MST_DP4_INTSTAT_UNION */
#define MST_DP4_INTSTAT_OFFSET                        0x400

/* 寄存器说明：dp4中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP4_INTMASK_UNION */
#define MST_DP4_INTMASK_OFFSET                        0x401

/* 寄存器说明：dp4的端口控制寄存器。
   位域定义UNION结构:  MST_DP4_PORTCTRL_UNION */
#define MST_DP4_PORTCTRL_OFFSET                       0x402

/* 寄存器说明：dp4 Block1控制寄存器。
   位域定义UNION结构:  MST_DP4_BLOCKCTRL1_UNION */
#define MST_DP4_BLOCKCTRL1_OFFSET                     0x403

/* 寄存器说明：dp4的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP4_PREPARESTATUS_UNION */
#define MST_DP4_PREPARESTATUS_OFFSET                  0x404

/* 寄存器说明：dp4的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP4_PREPARECTRL_UNION */
#define MST_DP4_PREPARECTRL_OFFSET                    0x405

/* 寄存器说明：dp4的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_CHANNELEN_BANK0_UNION */
#define MST_DP4_CHANNELEN_BANK0_OFFSET                0x420

/* 寄存器说明：dp4 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_BLOCKCTRL2_BANK0_UNION */
#define MST_DP4_BLOCKCTRL2_BANK0_OFFSET               0x421

/* 寄存器说明：dp4的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_SAMPLECTRL1_BANK0_UNION */
#define MST_DP4_SAMPLECTRL1_BANK0_OFFSET              0x422

/* 寄存器说明：dp4的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_SAMPLECTRL2_BANK0_UNION */
#define MST_DP4_SAMPLECTRL2_BANK0_OFFSET              0x423

/* 寄存器说明：dp4的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP4_OFFSETCTRL1_BANK0_UNION */
#define MST_DP4_OFFSETCTRL1_BANK0_OFFSET              0x424

/* 寄存器说明：dp4的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP4_OFFSETCTRL2_BANK0_UNION */
#define MST_DP4_OFFSETCTRL2_BANK0_OFFSET              0x425

/* 寄存器说明：dp4的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_HCTRL_BANK0_UNION */
#define MST_DP4_HCTRL_BANK0_OFFSET                    0x426

/* 寄存器说明：dp4 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_BLOCKCTRL3_BANK0_UNION */
#define MST_DP4_BLOCKCTRL3_BANK0_OFFSET               0x427

/* 寄存器说明：dp4的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP4_LANECTRL_BANK0_UNION */
#define MST_DP4_LANECTRL_BANK0_OFFSET                 0x428

/* 寄存器说明：dp4的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_CHANNELEN_BANK1_UNION */
#define MST_DP4_CHANNELEN_BANK1_OFFSET                0x430

/* 寄存器说明：dp4 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_BLOCKCTRL2_BANK1_UNION */
#define MST_DP4_BLOCKCTRL2_BANK1_OFFSET               0x431

/* 寄存器说明：dp4的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_SAMPLECTRL1_BANK1_UNION */
#define MST_DP4_SAMPLECTRL1_BANK1_OFFSET              0x432

/* 寄存器说明：dp4的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_SAMPLECTRL2_BANK1_UNION */
#define MST_DP4_SAMPLECTRL2_BANK1_OFFSET              0x433

/* 寄存器说明：dp4的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP4_OFFSETCTRL1_BANK1_UNION */
#define MST_DP4_OFFSETCTRL1_BANK1_OFFSET              0x434

/* 寄存器说明：dp4的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP4_OFFSETCTRL2_BANK1_UNION */
#define MST_DP4_OFFSETCTRL2_BANK1_OFFSET              0x435

/* 寄存器说明：dp4的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_HCTRL_BANK1_UNION */
#define MST_DP4_HCTRL_BANK1_OFFSET                    0x436

/* 寄存器说明：dp4 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_BLOCKCTRL3_BANK1_UNION */
#define MST_DP4_BLOCKCTRL3_BANK1_OFFSET               0x437

/* 寄存器说明：dp4的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP4_LANECTRL_BANK1_UNION */
#define MST_DP4_LANECTRL_BANK1_OFFSET                 0x438

/* 寄存器说明：dp5中断状态寄存器。
   位域定义UNION结构:  MST_DP5_INTSTAT_UNION */
#define MST_DP5_INTSTAT_OFFSET                        0x500

/* 寄存器说明：dp5中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP5_INTMASK_UNION */
#define MST_DP5_INTMASK_OFFSET                        0x501

/* 寄存器说明：dp5的端口控制寄存器。
   位域定义UNION结构:  MST_DP5_PORTCTRL_UNION */
#define MST_DP5_PORTCTRL_OFFSET                       0x502

/* 寄存器说明：dp5 Block1控制寄存器。
   位域定义UNION结构:  MST_DP5_BLOCKCTRL1_UNION */
#define MST_DP5_BLOCKCTRL1_OFFSET                     0x503

/* 寄存器说明：dp5的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP5_PREPARESTATUS_UNION */
#define MST_DP5_PREPARESTATUS_OFFSET                  0x504

/* 寄存器说明：dp5的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP5_PREPARECTRL_UNION */
#define MST_DP5_PREPARECTRL_OFFSET                    0x505

/* 寄存器说明：dp5的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_CHANNELEN_BANK0_UNION */
#define MST_DP5_CHANNELEN_BANK0_OFFSET                0x520

/* 寄存器说明：dp5 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_BLOCKCTRL2_BANK0_UNION */
#define MST_DP5_BLOCKCTRL2_BANK0_OFFSET               0x521

/* 寄存器说明：dp5的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_SAMPLECTRL1_BANK0_UNION */
#define MST_DP5_SAMPLECTRL1_BANK0_OFFSET              0x522

/* 寄存器说明：dp5的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_SAMPLECTRL2_BANK0_UNION */
#define MST_DP5_SAMPLECTRL2_BANK0_OFFSET              0x523

/* 寄存器说明：dp5的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP5_OFFSETCTRL1_BANK0_UNION */
#define MST_DP5_OFFSETCTRL1_BANK0_OFFSET              0x524

/* 寄存器说明：dp5的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP5_OFFSETCTRL2_BANK0_UNION */
#define MST_DP5_OFFSETCTRL2_BANK0_OFFSET              0x525

/* 寄存器说明：dp5的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_HCTRL_BANK0_UNION */
#define MST_DP5_HCTRL_BANK0_OFFSET                    0x526

/* 寄存器说明：dp5 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_BLOCKCTRL3_BANK0_UNION */
#define MST_DP5_BLOCKCTRL3_BANK0_OFFSET               0x527

/* 寄存器说明：dp5的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP5_LANECTRL_BANK0_UNION */
#define MST_DP5_LANECTRL_BANK0_OFFSET                 0x528

/* 寄存器说明：dp5的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_CHANNELEN_BANK1_UNION */
#define MST_DP5_CHANNELEN_BANK1_OFFSET                0x530

/* 寄存器说明：dp5 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_BLOCKCTRL2_BANK1_UNION */
#define MST_DP5_BLOCKCTRL2_BANK1_OFFSET               0x531

/* 寄存器说明：dp5的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_SAMPLECTRL1_BANK1_UNION */
#define MST_DP5_SAMPLECTRL1_BANK1_OFFSET              0x532

/* 寄存器说明：dp5的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_SAMPLECTRL2_BANK1_UNION */
#define MST_DP5_SAMPLECTRL2_BANK1_OFFSET              0x533

/* 寄存器说明：dp5的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP5_OFFSETCTRL1_BANK1_UNION */
#define MST_DP5_OFFSETCTRL1_BANK1_OFFSET              0x534

/* 寄存器说明：dp5的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP5_OFFSETCTRL2_BANK1_UNION */
#define MST_DP5_OFFSETCTRL2_BANK1_OFFSET              0x535

/* 寄存器说明：dp5的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_HCTRL_BANK1_UNION */
#define MST_DP5_HCTRL_BANK1_OFFSET                    0x536

/* 寄存器说明：dp5 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_BLOCKCTRL3_BANK1_UNION */
#define MST_DP5_BLOCKCTRL3_BANK1_OFFSET               0x537

/* 寄存器说明：dp5的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP5_LANECTRL_BANK1_UNION */
#define MST_DP5_LANECTRL_BANK1_OFFSET                 0x538

/* 寄存器说明：dp6中断状态寄存器。
   位域定义UNION结构:  MST_DP6_INTSTAT_UNION */
#define MST_DP6_INTSTAT_OFFSET                        0x600

/* 寄存器说明：dp6中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP6_INTMASK_UNION */
#define MST_DP6_INTMASK_OFFSET                        0x601

/* 寄存器说明：dp6的端口控制寄存器。
   位域定义UNION结构:  MST_DP6_PORTCTRL_UNION */
#define MST_DP6_PORTCTRL_OFFSET                       0x602

/* 寄存器说明：dp6 Block1控制寄存器。
   位域定义UNION结构:  MST_DP6_BLOCKCTRL1_UNION */
#define MST_DP6_BLOCKCTRL1_OFFSET                     0x603

/* 寄存器说明：dp6的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP6_PREPARESTATUS_UNION */
#define MST_DP6_PREPARESTATUS_OFFSET                  0x604

/* 寄存器说明：dp6的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP6_PREPARECTRL_UNION */
#define MST_DP6_PREPARECTRL_OFFSET                    0x605

/* 寄存器说明：dp6的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_CHANNELEN_BANK0_UNION */
#define MST_DP6_CHANNELEN_BANK0_OFFSET                0x620

/* 寄存器说明：dp6 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_BLOCKCTRL2_BANK0_UNION */
#define MST_DP6_BLOCKCTRL2_BANK0_OFFSET               0x621

/* 寄存器说明：dp6的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_SAMPLECTRL1_BANK0_UNION */
#define MST_DP6_SAMPLECTRL1_BANK0_OFFSET              0x622

/* 寄存器说明：dp6的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_SAMPLECTRL2_BANK0_UNION */
#define MST_DP6_SAMPLECTRL2_BANK0_OFFSET              0x623

/* 寄存器说明：dp6的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP6_OFFSETCTRL1_BANK0_UNION */
#define MST_DP6_OFFSETCTRL1_BANK0_OFFSET              0x624

/* 寄存器说明：dp6的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP6_OFFSETCTRL2_BANK0_UNION */
#define MST_DP6_OFFSETCTRL2_BANK0_OFFSET              0x625

/* 寄存器说明：dp6的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_HCTRL_BANK0_UNION */
#define MST_DP6_HCTRL_BANK0_OFFSET                    0x626

/* 寄存器说明：dp6 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_BLOCKCTRL3_BANK0_UNION */
#define MST_DP6_BLOCKCTRL3_BANK0_OFFSET               0x627

/* 寄存器说明：dp6的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP6_LANECTRL_BANK0_UNION */
#define MST_DP6_LANECTRL_BANK0_OFFSET                 0x628

/* 寄存器说明：dp6的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_CHANNELEN_BANK1_UNION */
#define MST_DP6_CHANNELEN_BANK1_OFFSET                0x630

/* 寄存器说明：dp6 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_BLOCKCTRL2_BANK1_UNION */
#define MST_DP6_BLOCKCTRL2_BANK1_OFFSET               0x631

/* 寄存器说明：dp6的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_SAMPLECTRL1_BANK1_UNION */
#define MST_DP6_SAMPLECTRL1_BANK1_OFFSET              0x632

/* 寄存器说明：dp6的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_SAMPLECTRL2_BANK1_UNION */
#define MST_DP6_SAMPLECTRL2_BANK1_OFFSET              0x633

/* 寄存器说明：dp6的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP6_OFFSETCTRL1_BANK1_UNION */
#define MST_DP6_OFFSETCTRL1_BANK1_OFFSET              0x634

/* 寄存器说明：dp6的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP6_OFFSETCTRL2_BANK1_UNION */
#define MST_DP6_OFFSETCTRL2_BANK1_OFFSET              0x635

/* 寄存器说明：dp6的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_HCTRL_BANK1_UNION */
#define MST_DP6_HCTRL_BANK1_OFFSET                    0x636

/* 寄存器说明：dp6 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_BLOCKCTRL3_BANK1_UNION */
#define MST_DP6_BLOCKCTRL3_BANK1_OFFSET               0x637

/* 寄存器说明：dp6的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP6_LANECTRL_BANK1_UNION */
#define MST_DP6_LANECTRL_BANK1_OFFSET                 0x638

/* 寄存器说明：dp7中断状态寄存器。
   位域定义UNION结构:  MST_DP7_INTSTAT_UNION */
#define MST_DP7_INTSTAT_OFFSET                        0x700

/* 寄存器说明：dp7中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP7_INTMASK_UNION */
#define MST_DP7_INTMASK_OFFSET                        0x701

/* 寄存器说明：dp7的端口控制寄存器。
   位域定义UNION结构:  MST_DP7_PORTCTRL_UNION */
#define MST_DP7_PORTCTRL_OFFSET                       0x702

/* 寄存器说明：dp7 Block1控制寄存器。
   位域定义UNION结构:  MST_DP7_BLOCKCTRL1_UNION */
#define MST_DP7_BLOCKCTRL1_OFFSET                     0x703

/* 寄存器说明：dp7的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP7_PREPARESTATUS_UNION */
#define MST_DP7_PREPARESTATUS_OFFSET                  0x704

/* 寄存器说明：dp7的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP7_PREPARECTRL_UNION */
#define MST_DP7_PREPARECTRL_OFFSET                    0x705

/* 寄存器说明：dp7的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_CHANNELEN_BANK0_UNION */
#define MST_DP7_CHANNELEN_BANK0_OFFSET                0x720

/* 寄存器说明：dp7 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_BLOCKCTRL2_BANK0_UNION */
#define MST_DP7_BLOCKCTRL2_BANK0_OFFSET               0x721

/* 寄存器说明：dp7的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_SAMPLECTRL1_BANK0_UNION */
#define MST_DP7_SAMPLECTRL1_BANK0_OFFSET              0x722

/* 寄存器说明：dp7的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_SAMPLECTRL2_BANK0_UNION */
#define MST_DP7_SAMPLECTRL2_BANK0_OFFSET              0x723

/* 寄存器说明：dp7的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP7_OFFSETCTRL1_BANK0_UNION */
#define MST_DP7_OFFSETCTRL1_BANK0_OFFSET              0x724

/* 寄存器说明：dp7的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP7_OFFSETCTRL2_BANK0_UNION */
#define MST_DP7_OFFSETCTRL2_BANK0_OFFSET              0x725

/* 寄存器说明：dp7的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_HCTRL_BANK0_UNION */
#define MST_DP7_HCTRL_BANK0_OFFSET                    0x726

/* 寄存器说明：dp7 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_BLOCKCTRL3_BANK0_UNION */
#define MST_DP7_BLOCKCTRL3_BANK0_OFFSET               0x727

/* 寄存器说明：dp7的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP7_LANECTRL_BANK0_UNION */
#define MST_DP7_LANECTRL_BANK0_OFFSET                 0x728

/* 寄存器说明：dp7的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_CHANNELEN_BANK1_UNION */
#define MST_DP7_CHANNELEN_BANK1_OFFSET                0x730

/* 寄存器说明：dp7 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_BLOCKCTRL2_BANK1_UNION */
#define MST_DP7_BLOCKCTRL2_BANK1_OFFSET               0x731

/* 寄存器说明：dp7的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_SAMPLECTRL1_BANK1_UNION */
#define MST_DP7_SAMPLECTRL1_BANK1_OFFSET              0x732

/* 寄存器说明：dp7的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_SAMPLECTRL2_BANK1_UNION */
#define MST_DP7_SAMPLECTRL2_BANK1_OFFSET              0x733

/* 寄存器说明：dp7的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP7_OFFSETCTRL1_BANK1_UNION */
#define MST_DP7_OFFSETCTRL1_BANK1_OFFSET              0x734

/* 寄存器说明：dp7的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP7_OFFSETCTRL2_BANK1_UNION */
#define MST_DP7_OFFSETCTRL2_BANK1_OFFSET              0x735

/* 寄存器说明：dp7的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_HCTRL_BANK1_UNION */
#define MST_DP7_HCTRL_BANK1_OFFSET                    0x736

/* 寄存器说明：dp7 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_BLOCKCTRL3_BANK1_UNION */
#define MST_DP7_BLOCKCTRL3_BANK1_OFFSET               0x737

/* 寄存器说明：dp7的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP7_LANECTRL_BANK1_UNION */
#define MST_DP7_LANECTRL_BANK1_OFFSET                 0x738

/* 寄存器说明：dp8中断状态寄存器。
   位域定义UNION结构:  MST_DP8_INTSTAT_UNION */
#define MST_DP8_INTSTAT_OFFSET                        0x800

/* 寄存器说明：dp8中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP8_INTMASK_UNION */
#define MST_DP8_INTMASK_OFFSET                        0x801

/* 寄存器说明：dp8的端口控制寄存器。
   位域定义UNION结构:  MST_DP8_PORTCTRL_UNION */
#define MST_DP8_PORTCTRL_OFFSET                       0x802

/* 寄存器说明：dp8 Block1控制寄存器。
   位域定义UNION结构:  MST_DP8_BLOCKCTRL1_UNION */
#define MST_DP8_BLOCKCTRL1_OFFSET                     0x803

/* 寄存器说明：dp8的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP8_PREPARESTATUS_UNION */
#define MST_DP8_PREPARESTATUS_OFFSET                  0x804

/* 寄存器说明：dp8的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP8_PREPARECTRL_UNION */
#define MST_DP8_PREPARECTRL_OFFSET                    0x805

/* 寄存器说明：dp8的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_CHANNELEN_BANK0_UNION */
#define MST_DP8_CHANNELEN_BANK0_OFFSET                0x820

/* 寄存器说明：dp8 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_BLOCKCTRL2_BANK0_UNION */
#define MST_DP8_BLOCKCTRL2_BANK0_OFFSET               0x821

/* 寄存器说明：dp8的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_SAMPLECTRL1_BANK0_UNION */
#define MST_DP8_SAMPLECTRL1_BANK0_OFFSET              0x822

/* 寄存器说明：dp8的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_SAMPLECTRL2_BANK0_UNION */
#define MST_DP8_SAMPLECTRL2_BANK0_OFFSET              0x823

/* 寄存器说明：dp8的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP8_OFFSETCTRL1_BANK0_UNION */
#define MST_DP8_OFFSETCTRL1_BANK0_OFFSET              0x824

/* 寄存器说明：dp8的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP8_OFFSETCTRL2_BANK0_UNION */
#define MST_DP8_OFFSETCTRL2_BANK0_OFFSET              0x825

/* 寄存器说明：dp8的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_HCTRL_BANK0_UNION */
#define MST_DP8_HCTRL_BANK0_OFFSET                    0x826

/* 寄存器说明：dp8 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_BLOCKCTRL3_BANK0_UNION */
#define MST_DP8_BLOCKCTRL3_BANK0_OFFSET               0x827

/* 寄存器说明：dp8的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP8_LANECTRL_BANK0_UNION */
#define MST_DP8_LANECTRL_BANK0_OFFSET                 0x828

/* 寄存器说明：dp8的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_CHANNELEN_BANK1_UNION */
#define MST_DP8_CHANNELEN_BANK1_OFFSET                0x830

/* 寄存器说明：dp8 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_BLOCKCTRL2_BANK1_UNION */
#define MST_DP8_BLOCKCTRL2_BANK1_OFFSET               0x831

/* 寄存器说明：dp8的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_SAMPLECTRL1_BANK1_UNION */
#define MST_DP8_SAMPLECTRL1_BANK1_OFFSET              0x832

/* 寄存器说明：dp8的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_SAMPLECTRL2_BANK1_UNION */
#define MST_DP8_SAMPLECTRL2_BANK1_OFFSET              0x833

/* 寄存器说明：dp8的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP8_OFFSETCTRL1_BANK1_UNION */
#define MST_DP8_OFFSETCTRL1_BANK1_OFFSET              0x834

/* 寄存器说明：dp8的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP8_OFFSETCTRL2_BANK1_UNION */
#define MST_DP8_OFFSETCTRL2_BANK1_OFFSET              0x835

/* 寄存器说明：dp8的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_HCTRL_BANK1_UNION */
#define MST_DP8_HCTRL_BANK1_OFFSET                    0x836

/* 寄存器说明：dp8 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_BLOCKCTRL3_BANK1_UNION */
#define MST_DP8_BLOCKCTRL3_BANK1_OFFSET               0x837

/* 寄存器说明：dp8的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP8_LANECTRL_BANK1_UNION */
#define MST_DP8_LANECTRL_BANK1_OFFSET                 0x838

/* 寄存器说明：dp9中断状态寄存器。
   位域定义UNION结构:  MST_DP9_INTSTAT_UNION */
#define MST_DP9_INTSTAT_OFFSET                        0x900

/* 寄存器说明：dp9中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP9_INTMASK_UNION */
#define MST_DP9_INTMASK_OFFSET                        0x901

/* 寄存器说明：dp9的端口控制寄存器。
   位域定义UNION结构:  MST_DP9_PORTCTRL_UNION */
#define MST_DP9_PORTCTRL_OFFSET                       0x902

/* 寄存器说明：dp9 Block1控制寄存器。
   位域定义UNION结构:  MST_DP9_BLOCKCTRL1_UNION */
#define MST_DP9_BLOCKCTRL1_OFFSET                     0x903

/* 寄存器说明：dp9的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP9_PREPARESTATUS_UNION */
#define MST_DP9_PREPARESTATUS_OFFSET                  0x904

/* 寄存器说明：dp9的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP9_PREPARECTRL_UNION */
#define MST_DP9_PREPARECTRL_OFFSET                    0x905

/* 寄存器说明：dp9的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_CHANNELEN_BANK0_UNION */
#define MST_DP9_CHANNELEN_BANK0_OFFSET                0x920

/* 寄存器说明：dp9 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_BLOCKCTRL2_BANK0_UNION */
#define MST_DP9_BLOCKCTRL2_BANK0_OFFSET               0x921

/* 寄存器说明：dp9的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_SAMPLECTRL1_BANK0_UNION */
#define MST_DP9_SAMPLECTRL1_BANK0_OFFSET              0x922

/* 寄存器说明：dp9的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_SAMPLECTRL2_BANK0_UNION */
#define MST_DP9_SAMPLECTRL2_BANK0_OFFSET              0x923

/* 寄存器说明：dp9的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP9_OFFSETCTRL1_BANK0_UNION */
#define MST_DP9_OFFSETCTRL1_BANK0_OFFSET              0x924

/* 寄存器说明：dp9的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP9_OFFSETCTRL2_BANK0_UNION */
#define MST_DP9_OFFSETCTRL2_BANK0_OFFSET              0x925

/* 寄存器说明：dp9的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_HCTRL_BANK0_UNION */
#define MST_DP9_HCTRL_BANK0_OFFSET                    0x926

/* 寄存器说明：dp9 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_BLOCKCTRL3_BANK0_UNION */
#define MST_DP9_BLOCKCTRL3_BANK0_OFFSET               0x927

/* 寄存器说明：dp9的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP9_LANECTRL_BANK0_UNION */
#define MST_DP9_LANECTRL_BANK0_OFFSET                 0x928

/* 寄存器说明：dp9的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_CHANNELEN_BANK1_UNION */
#define MST_DP9_CHANNELEN_BANK1_OFFSET                0x930

/* 寄存器说明：dp9 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_BLOCKCTRL2_BANK1_UNION */
#define MST_DP9_BLOCKCTRL2_BANK1_OFFSET               0x931

/* 寄存器说明：dp9的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_SAMPLECTRL1_BANK1_UNION */
#define MST_DP9_SAMPLECTRL1_BANK1_OFFSET              0x932

/* 寄存器说明：dp9的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_SAMPLECTRL2_BANK1_UNION */
#define MST_DP9_SAMPLECTRL2_BANK1_OFFSET              0x933

/* 寄存器说明：dp9的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP9_OFFSETCTRL1_BANK1_UNION */
#define MST_DP9_OFFSETCTRL1_BANK1_OFFSET              0x934

/* 寄存器说明：dp9的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP9_OFFSETCTRL2_BANK1_UNION */
#define MST_DP9_OFFSETCTRL2_BANK1_OFFSET              0x935

/* 寄存器说明：dp9的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_HCTRL_BANK1_UNION */
#define MST_DP9_HCTRL_BANK1_OFFSET                    0x936

/* 寄存器说明：dp9 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_BLOCKCTRL3_BANK1_UNION */
#define MST_DP9_BLOCKCTRL3_BANK1_OFFSET               0x937

/* 寄存器说明：dp9的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP9_LANECTRL_BANK1_UNION */
#define MST_DP9_LANECTRL_BANK1_OFFSET                 0x938

/* 寄存器说明：dp10中断状态寄存器。
   位域定义UNION结构:  MST_DP10_INTSTAT_UNION */
#define MST_DP10_INTSTAT_OFFSET                       0xA00

/* 寄存器说明：dp10中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP10_INTMASK_UNION */
#define MST_DP10_INTMASK_OFFSET                       0xA01

/* 寄存器说明：dp10的端口控制寄存器。
   位域定义UNION结构:  MST_DP10_PORTCTRL_UNION */
#define MST_DP10_PORTCTRL_OFFSET                      0xA02

/* 寄存器说明：dp10 Block1控制寄存器。
   位域定义UNION结构:  MST_DP10_BLOCKCTRL1_UNION */
#define MST_DP10_BLOCKCTRL1_OFFSET                    0xA03

/* 寄存器说明：dp10的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP10_PREPARESTATUS_UNION */
#define MST_DP10_PREPARESTATUS_OFFSET                 0xA04

/* 寄存器说明：dp10的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP10_PREPARECTRL_UNION */
#define MST_DP10_PREPARECTRL_OFFSET                   0xA05

/* 寄存器说明：dp10的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_CHANNELEN_BANK0_UNION */
#define MST_DP10_CHANNELEN_BANK0_OFFSET               0xA20

/* 寄存器说明：dp10 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_BLOCKCTRL2_BANK0_UNION */
#define MST_DP10_BLOCKCTRL2_BANK0_OFFSET              0xA21

/* 寄存器说明：dp10的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_SAMPLECTRL1_BANK0_UNION */
#define MST_DP10_SAMPLECTRL1_BANK0_OFFSET             0xA22

/* 寄存器说明：dp10的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_SAMPLECTRL2_BANK0_UNION */
#define MST_DP10_SAMPLECTRL2_BANK0_OFFSET             0xA23

/* 寄存器说明：dp10的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP10_OFFSETCTRL1_BANK0_UNION */
#define MST_DP10_OFFSETCTRL1_BANK0_OFFSET             0xA24

/* 寄存器说明：dp10的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP10_OFFSETCTRL2_BANK0_UNION */
#define MST_DP10_OFFSETCTRL2_BANK0_OFFSET             0xA25

/* 寄存器说明：dp10的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_HCTRL_BANK0_UNION */
#define MST_DP10_HCTRL_BANK0_OFFSET                   0xA26

/* 寄存器说明：dp10 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_BLOCKCTRL3_BANK0_UNION */
#define MST_DP10_BLOCKCTRL3_BANK0_OFFSET              0xA27

/* 寄存器说明：dp10的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP10_LANECTRL_BANK0_UNION */
#define MST_DP10_LANECTRL_BANK0_OFFSET                0xA28

/* 寄存器说明：dp10的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_CHANNELEN_BANK1_UNION */
#define MST_DP10_CHANNELEN_BANK1_OFFSET               0xA30

/* 寄存器说明：dp10 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_BLOCKCTRL2_BANK1_UNION */
#define MST_DP10_BLOCKCTRL2_BANK1_OFFSET              0xA31

/* 寄存器说明：dp10的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_SAMPLECTRL1_BANK1_UNION */
#define MST_DP10_SAMPLECTRL1_BANK1_OFFSET             0xA32

/* 寄存器说明：dp10的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_SAMPLECTRL2_BANK1_UNION */
#define MST_DP10_SAMPLECTRL2_BANK1_OFFSET             0xA33

/* 寄存器说明：dp10的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP10_OFFSETCTRL1_BANK1_UNION */
#define MST_DP10_OFFSETCTRL1_BANK1_OFFSET             0xA34

/* 寄存器说明：dp10的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP10_OFFSETCTRL2_BANK1_UNION */
#define MST_DP10_OFFSETCTRL2_BANK1_OFFSET             0xA35

/* 寄存器说明：dp10的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_HCTRL_BANK1_UNION */
#define MST_DP10_HCTRL_BANK1_OFFSET                   0xA36

/* 寄存器说明：dp10 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_BLOCKCTRL3_BANK1_UNION */
#define MST_DP10_BLOCKCTRL3_BANK1_OFFSET              0xA37

/* 寄存器说明：dp10的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP10_LANECTRL_BANK1_UNION */
#define MST_DP10_LANECTRL_BANK1_OFFSET                0xA38

/* 寄存器说明：dp11中断状态寄存器。
   位域定义UNION结构:  MST_DP11_INTSTAT_UNION */
#define MST_DP11_INTSTAT_OFFSET                       0xB00

/* 寄存器说明：dp11中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP11_INTMASK_UNION */
#define MST_DP11_INTMASK_OFFSET                       0xB01

/* 寄存器说明：dp11的端口控制寄存器。
   位域定义UNION结构:  MST_DP11_PORTCTRL_UNION */
#define MST_DP11_PORTCTRL_OFFSET                      0xB02

/* 寄存器说明：dp11 Block1控制寄存器。
   位域定义UNION结构:  MST_DP11_BLOCKCTRL1_UNION */
#define MST_DP11_BLOCKCTRL1_OFFSET                    0xB03

/* 寄存器说明：dp11的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP11_PREPARESTATUS_UNION */
#define MST_DP11_PREPARESTATUS_OFFSET                 0xB04

/* 寄存器说明：dp11的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP11_PREPARECTRL_UNION */
#define MST_DP11_PREPARECTRL_OFFSET                   0xB05

/* 寄存器说明：dp11的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_CHANNELEN_BANK0_UNION */
#define MST_DP11_CHANNELEN_BANK0_OFFSET               0xB20

/* 寄存器说明：dp11 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_BLOCKCTRL2_BANK0_UNION */
#define MST_DP11_BLOCKCTRL2_BANK0_OFFSET              0xB21

/* 寄存器说明：dp11的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_SAMPLECTRL1_BANK0_UNION */
#define MST_DP11_SAMPLECTRL1_BANK0_OFFSET             0xB22

/* 寄存器说明：dp11的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_SAMPLECTRL2_BANK0_UNION */
#define MST_DP11_SAMPLECTRL2_BANK0_OFFSET             0xB23

/* 寄存器说明：dp11的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP11_OFFSETCTRL1_BANK0_UNION */
#define MST_DP11_OFFSETCTRL1_BANK0_OFFSET             0xB24

/* 寄存器说明：dp11的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP11_OFFSETCTRL2_BANK0_UNION */
#define MST_DP11_OFFSETCTRL2_BANK0_OFFSET             0xB25

/* 寄存器说明：dp11的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_HCTRL_BANK0_UNION */
#define MST_DP11_HCTRL_BANK0_OFFSET                   0xB26

/* 寄存器说明：dp11 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_BLOCKCTRL3_BANK0_UNION */
#define MST_DP11_BLOCKCTRL3_BANK0_OFFSET              0xB27

/* 寄存器说明：dp11的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP11_LANECTRL_BANK0_UNION */
#define MST_DP11_LANECTRL_BANK0_OFFSET                0xB28

/* 寄存器说明：dp11的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_CHANNELEN_BANK1_UNION */
#define MST_DP11_CHANNELEN_BANK1_OFFSET               0xB30

/* 寄存器说明：dp11 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_BLOCKCTRL2_BANK1_UNION */
#define MST_DP11_BLOCKCTRL2_BANK1_OFFSET              0xB31

/* 寄存器说明：dp11的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_SAMPLECTRL1_BANK1_UNION */
#define MST_DP11_SAMPLECTRL1_BANK1_OFFSET             0xB32

/* 寄存器说明：dp11的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_SAMPLECTRL2_BANK1_UNION */
#define MST_DP11_SAMPLECTRL2_BANK1_OFFSET             0xB33

/* 寄存器说明：dp11的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP11_OFFSETCTRL1_BANK1_UNION */
#define MST_DP11_OFFSETCTRL1_BANK1_OFFSET             0xB34

/* 寄存器说明：dp11的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP11_OFFSETCTRL2_BANK1_UNION */
#define MST_DP11_OFFSETCTRL2_BANK1_OFFSET             0xB35

/* 寄存器说明：dp11的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_HCTRL_BANK1_UNION */
#define MST_DP11_HCTRL_BANK1_OFFSET                   0xB36

/* 寄存器说明：dp11 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_BLOCKCTRL3_BANK1_UNION */
#define MST_DP11_BLOCKCTRL3_BANK1_OFFSET              0xB37

/* 寄存器说明：dp11的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP11_LANECTRL_BANK1_UNION */
#define MST_DP11_LANECTRL_BANK1_OFFSET                0xB38

/* 寄存器说明：dp12中断状态寄存器。
   位域定义UNION结构:  MST_DP12_INTSTAT_UNION */
#define MST_DP12_INTSTAT_OFFSET                       0xC00

/* 寄存器说明：dp12中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP12_INTMASK_UNION */
#define MST_DP12_INTMASK_OFFSET                       0xC01

/* 寄存器说明：dp12的端口控制寄存器。
   位域定义UNION结构:  MST_DP12_PORTCTRL_UNION */
#define MST_DP12_PORTCTRL_OFFSET                      0xC02

/* 寄存器说明：dp12 Block1控制寄存器。
   位域定义UNION结构:  MST_DP12_BLOCKCTRL1_UNION */
#define MST_DP12_BLOCKCTRL1_OFFSET                    0xC03

/* 寄存器说明：dp12的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP12_PREPARESTATUS_UNION */
#define MST_DP12_PREPARESTATUS_OFFSET                 0xC04

/* 寄存器说明：dp12的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP12_PREPARECTRL_UNION */
#define MST_DP12_PREPARECTRL_OFFSET                   0xC05

/* 寄存器说明：dp12的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_CHANNELEN_BANK0_UNION */
#define MST_DP12_CHANNELEN_BANK0_OFFSET               0xC20

/* 寄存器说明：dp12 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_BLOCKCTRL2_BANK0_UNION */
#define MST_DP12_BLOCKCTRL2_BANK0_OFFSET              0xC21

/* 寄存器说明：dp12的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_SAMPLECTRL1_BANK0_UNION */
#define MST_DP12_SAMPLECTRL1_BANK0_OFFSET             0xC22

/* 寄存器说明：dp12的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_SAMPLECTRL2_BANK0_UNION */
#define MST_DP12_SAMPLECTRL2_BANK0_OFFSET             0xC23

/* 寄存器说明：dp12的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP12_OFFSETCTRL1_BANK0_UNION */
#define MST_DP12_OFFSETCTRL1_BANK0_OFFSET             0xC24

/* 寄存器说明：dp12的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP12_OFFSETCTRL2_BANK0_UNION */
#define MST_DP12_OFFSETCTRL2_BANK0_OFFSET             0xC25

/* 寄存器说明：dp12的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_HCTRL_BANK0_UNION */
#define MST_DP12_HCTRL_BANK0_OFFSET                   0xC26

/* 寄存器说明：dp12 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_BLOCKCTRL3_BANK0_UNION */
#define MST_DP12_BLOCKCTRL3_BANK0_OFFSET              0xC27

/* 寄存器说明：dp12的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP12_LANECTRL_BANK0_UNION */
#define MST_DP12_LANECTRL_BANK0_OFFSET                0xC28

/* 寄存器说明：dp12的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_CHANNELEN_BANK1_UNION */
#define MST_DP12_CHANNELEN_BANK1_OFFSET               0xC30

/* 寄存器说明：dp12 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_BLOCKCTRL2_BANK1_UNION */
#define MST_DP12_BLOCKCTRL2_BANK1_OFFSET              0xC31

/* 寄存器说明：dp12的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_SAMPLECTRL1_BANK1_UNION */
#define MST_DP12_SAMPLECTRL1_BANK1_OFFSET             0xC32

/* 寄存器说明：dp12的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_SAMPLECTRL2_BANK1_UNION */
#define MST_DP12_SAMPLECTRL2_BANK1_OFFSET             0xC33

/* 寄存器说明：dp12的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP12_OFFSETCTRL1_BANK1_UNION */
#define MST_DP12_OFFSETCTRL1_BANK1_OFFSET             0xC34

/* 寄存器说明：dp12的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP12_OFFSETCTRL2_BANK1_UNION */
#define MST_DP12_OFFSETCTRL2_BANK1_OFFSET             0xC35

/* 寄存器说明：dp12的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_HCTRL_BANK1_UNION */
#define MST_DP12_HCTRL_BANK1_OFFSET                   0xC36

/* 寄存器说明：dp12 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_BLOCKCTRL3_BANK1_UNION */
#define MST_DP12_BLOCKCTRL3_BANK1_OFFSET              0xC37

/* 寄存器说明：dp12的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP12_LANECTRL_BANK1_UNION */
#define MST_DP12_LANECTRL_BANK1_OFFSET                0xC38

/* 寄存器说明：dp13中断状态寄存器。
   位域定义UNION结构:  MST_DP13_INTSTAT_UNION */
#define MST_DP13_INTSTAT_OFFSET                       0xD00

/* 寄存器说明：dp13中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP13_INTMASK_UNION */
#define MST_DP13_INTMASK_OFFSET                       0xD01

/* 寄存器说明：dp13的端口控制寄存器。
   位域定义UNION结构:  MST_DP13_PORTCTRL_UNION */
#define MST_DP13_PORTCTRL_OFFSET                      0xD02

/* 寄存器说明：dp13 Block1控制寄存器。
   位域定义UNION结构:  MST_DP13_BLOCKCTRL1_UNION */
#define MST_DP13_BLOCKCTRL1_OFFSET                    0xD03

/* 寄存器说明：dp13的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP13_PREPARESTATUS_UNION */
#define MST_DP13_PREPARESTATUS_OFFSET                 0xD04

/* 寄存器说明：dp13的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP13_PREPARECTRL_UNION */
#define MST_DP13_PREPARECTRL_OFFSET                   0xD05

/* 寄存器说明：dp13的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_CHANNELEN_BANK0_UNION */
#define MST_DP13_CHANNELEN_BANK0_OFFSET               0xD20

/* 寄存器说明：dp13 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_BLOCKCTRL2_BANK0_UNION */
#define MST_DP13_BLOCKCTRL2_BANK0_OFFSET              0xD21

/* 寄存器说明：dp13的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_SAMPLECTRL1_BANK0_UNION */
#define MST_DP13_SAMPLECTRL1_BANK0_OFFSET             0xD22

/* 寄存器说明：dp13的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_SAMPLECTRL2_BANK0_UNION */
#define MST_DP13_SAMPLECTRL2_BANK0_OFFSET             0xD23

/* 寄存器说明：dp13的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP13_OFFSETCTRL1_BANK0_UNION */
#define MST_DP13_OFFSETCTRL1_BANK0_OFFSET             0xD24

/* 寄存器说明：dp13的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP13_OFFSETCTRL2_BANK0_UNION */
#define MST_DP13_OFFSETCTRL2_BANK0_OFFSET             0xD25

/* 寄存器说明：dp13的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_HCTRL_BANK0_UNION */
#define MST_DP13_HCTRL_BANK0_OFFSET                   0xD26

/* 寄存器说明：dp13 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_BLOCKCTRL3_BANK0_UNION */
#define MST_DP13_BLOCKCTRL3_BANK0_OFFSET              0xD27

/* 寄存器说明：dp13的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP13_LANECTRL_BANK0_UNION */
#define MST_DP13_LANECTRL_BANK0_OFFSET                0xD28

/* 寄存器说明：dp13的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_CHANNELEN_BANK1_UNION */
#define MST_DP13_CHANNELEN_BANK1_OFFSET               0xD30

/* 寄存器说明：dp13 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_BLOCKCTRL2_BANK1_UNION */
#define MST_DP13_BLOCKCTRL2_BANK1_OFFSET              0xD31

/* 寄存器说明：dp13的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_SAMPLECTRL1_BANK1_UNION */
#define MST_DP13_SAMPLECTRL1_BANK1_OFFSET             0xD32

/* 寄存器说明：dp13的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_SAMPLECTRL2_BANK1_UNION */
#define MST_DP13_SAMPLECTRL2_BANK1_OFFSET             0xD33

/* 寄存器说明：dp13的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP13_OFFSETCTRL1_BANK1_UNION */
#define MST_DP13_OFFSETCTRL1_BANK1_OFFSET             0xD34

/* 寄存器说明：dp13的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP13_OFFSETCTRL2_BANK1_UNION */
#define MST_DP13_OFFSETCTRL2_BANK1_OFFSET             0xD35

/* 寄存器说明：dp13的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_HCTRL_BANK1_UNION */
#define MST_DP13_HCTRL_BANK1_OFFSET                   0xD36

/* 寄存器说明：dp13 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_BLOCKCTRL3_BANK1_UNION */
#define MST_DP13_BLOCKCTRL3_BANK1_OFFSET              0xD37

/* 寄存器说明：dp13的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP13_LANECTRL_BANK1_UNION */
#define MST_DP13_LANECTRL_BANK1_OFFSET                0xD38

/* 寄存器说明：dp14中断状态寄存器。
   位域定义UNION结构:  MST_DP14_INTSTAT_UNION */
#define MST_DP14_INTSTAT_OFFSET                       0xE00

/* 寄存器说明：dp14中断屏蔽寄存器。
   位域定义UNION结构:  MST_DP14_INTMASK_UNION */
#define MST_DP14_INTMASK_OFFSET                       0xE01

/* 寄存器说明：dp14的端口控制寄存器。
   位域定义UNION结构:  MST_DP14_PORTCTRL_UNION */
#define MST_DP14_PORTCTRL_OFFSET                      0xE02

/* 寄存器说明：dp14 Block1控制寄存器。
   位域定义UNION结构:  MST_DP14_BLOCKCTRL1_UNION */
#define MST_DP14_BLOCKCTRL1_OFFSET                    0xE03

/* 寄存器说明：dp14的通道准备状态寄存器。
   位域定义UNION结构:  MST_DP14_PREPARESTATUS_UNION */
#define MST_DP14_PREPARESTATUS_OFFSET                 0xE04

/* 寄存器说明：dp14的通道准备控制寄存器。
   位域定义UNION结构:  MST_DP14_PREPARECTRL_UNION */
#define MST_DP14_PREPARECTRL_OFFSET                   0xE05

/* 寄存器说明：dp14的通道使能寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_CHANNELEN_BANK0_UNION */
#define MST_DP14_CHANNELEN_BANK0_OFFSET               0xE20

/* 寄存器说明：dp14 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_BLOCKCTRL2_BANK0_UNION */
#define MST_DP14_BLOCKCTRL2_BANK0_OFFSET              0xE21

/* 寄存器说明：dp14的采样间隔低8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_SAMPLECTRL1_BANK0_UNION */
#define MST_DP14_SAMPLECTRL1_BANK0_OFFSET             0xE22

/* 寄存器说明：dp14的采样间隔高8bit控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_SAMPLECTRL2_BANK0_UNION */
#define MST_DP14_SAMPLECTRL2_BANK0_OFFSET             0xE23

/* 寄存器说明：dp14的Offset控制寄存器低8位(bank0)。
   位域定义UNION结构:  MST_DP14_OFFSETCTRL1_BANK0_UNION */
#define MST_DP14_OFFSETCTRL1_BANK0_OFFSET             0xE24

/* 寄存器说明：dp14的Offset控制寄存器高8位(bank0)。
   位域定义UNION结构:  MST_DP14_OFFSETCTRL2_BANK0_UNION */
#define MST_DP14_OFFSETCTRL2_BANK0_OFFSET             0xE25

/* 寄存器说明：dp14的传输子帧参数控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_HCTRL_BANK0_UNION */
#define MST_DP14_HCTRL_BANK0_OFFSET                   0xE26

/* 寄存器说明：dp14 Block2控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_BLOCKCTRL3_BANK0_UNION */
#define MST_DP14_BLOCKCTRL3_BANK0_OFFSET              0xE27

/* 寄存器说明：dp14的数据线控制寄存器(bank0)。
   位域定义UNION结构:  MST_DP14_LANECTRL_BANK0_UNION */
#define MST_DP14_LANECTRL_BANK0_OFFSET                0xE28

/* 寄存器说明：dp14的通道使能寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_CHANNELEN_BANK1_UNION */
#define MST_DP14_CHANNELEN_BANK1_OFFSET               0xE30

/* 寄存器说明：dp14 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_BLOCKCTRL2_BANK1_UNION */
#define MST_DP14_BLOCKCTRL2_BANK1_OFFSET              0xE31

/* 寄存器说明：dp14的采样间隔低8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_SAMPLECTRL1_BANK1_UNION */
#define MST_DP14_SAMPLECTRL1_BANK1_OFFSET             0xE32

/* 寄存器说明：dp14的采样间隔高8bit控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_SAMPLECTRL2_BANK1_UNION */
#define MST_DP14_SAMPLECTRL2_BANK1_OFFSET             0xE33

/* 寄存器说明：dp14的Offset控制寄存器低8位(bank1)。
   位域定义UNION结构:  MST_DP14_OFFSETCTRL1_BANK1_UNION */
#define MST_DP14_OFFSETCTRL1_BANK1_OFFSET             0xE34

/* 寄存器说明：dp14的Offset控制寄存器高8位(bank1)。
   位域定义UNION结构:  MST_DP14_OFFSETCTRL2_BANK1_UNION */
#define MST_DP14_OFFSETCTRL2_BANK1_OFFSET             0xE35

/* 寄存器说明：dp14的传输子帧参数控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_HCTRL_BANK1_UNION */
#define MST_DP14_HCTRL_BANK1_OFFSET                   0xE36

/* 寄存器说明：dp14 Block2控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_BLOCKCTRL3_BANK1_UNION */
#define MST_DP14_BLOCKCTRL3_BANK1_OFFSET              0xE37

/* 寄存器说明：dp14的数据线控制寄存器(bank1)。
   位域定义UNION结构:  MST_DP14_LANECTRL_BANK1_UNION */
#define MST_DP14_LANECTRL_BANK1_OFFSET                0xE38

/*****************************************************************************
  3 枚举定义
*****************************************************************************/



/*****************************************************************************
  4 消息头定义
*****************************************************************************/


/*****************************************************************************
  5 消息定义
*****************************************************************************/



/*****************************************************************************
  6 STRUCT定义
*****************************************************************************/



/*****************************************************************************
  7 UNION定义
*****************************************************************************/

/****************************************************************************
                     (1/1) HOST_REG
 ****************************************************************************/
/*****************************************************************************
 结构名    : MST_DP0_INTSTAT_UNION
 结构说明  : DP0_INTSTAT 寄存器结构定义。地址偏移量:0x00，初值:0x00，宽度:8
 寄存器说明: DP0中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_test_fail_int  : 1;  /* bit[0]  : DP0 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp0_port_ready_int : 1;  /* bit[1]  : DP0 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp0_cfg_error0_int : 1;  /* bit[5]  : Dp0自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp0_cfg_error1_int : 1;  /* bit[6]  : Dp0自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp0_cfg_error2_int : 1;  /* bit[7]  : Dp0自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP0_INTSTAT_UNION;
#endif
#define MST_DP0_INTSTAT_dp0_test_fail_int_START   (0)
#define MST_DP0_INTSTAT_dp0_test_fail_int_END     (0)
#define MST_DP0_INTSTAT_dp0_port_ready_int_START  (1)
#define MST_DP0_INTSTAT_dp0_port_ready_int_END    (1)
#define MST_DP0_INTSTAT_dp0_cfg_error0_int_START  (5)
#define MST_DP0_INTSTAT_dp0_cfg_error0_int_END    (5)
#define MST_DP0_INTSTAT_dp0_cfg_error1_int_START  (6)
#define MST_DP0_INTSTAT_dp0_cfg_error1_int_END    (6)
#define MST_DP0_INTSTAT_dp0_cfg_error2_int_START  (7)
#define MST_DP0_INTSTAT_dp0_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_INTMASK_UNION
 结构说明  : DP0_INTMASK 寄存器结构定义。地址偏移量:0x01，初值:0x00，宽度:8
 寄存器说明: DP0中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_test_fail_int_mask  : 1;  /* bit[0]  : DP0 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp0_port_ready_int_mask : 1;  /* bit[1]  : DP0 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp0_cfg_error0_int_mask : 1;  /* bit[5]  : Dp0自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp0_cfg_error1_int_mask : 1;  /* bit[6]  : Dp0自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp0_cfg_error2_int_mask : 1;  /* bit[7]  : Dp0自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP0_INTMASK_UNION;
#endif
#define MST_DP0_INTMASK_dp0_test_fail_int_mask_START   (0)
#define MST_DP0_INTMASK_dp0_test_fail_int_mask_END     (0)
#define MST_DP0_INTMASK_dp0_port_ready_int_mask_START  (1)
#define MST_DP0_INTMASK_dp0_port_ready_int_mask_END    (1)
#define MST_DP0_INTMASK_dp0_cfg_error0_int_mask_START  (5)
#define MST_DP0_INTMASK_dp0_cfg_error0_int_mask_END    (5)
#define MST_DP0_INTMASK_dp0_cfg_error1_int_mask_START  (6)
#define MST_DP0_INTMASK_dp0_cfg_error1_int_mask_END    (6)
#define MST_DP0_INTMASK_dp0_cfg_error2_int_mask_START  (7)
#define MST_DP0_INTMASK_dp0_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_PORTCTRL_UNION
 结构说明  : DP0_PORTCTRL 寄存器结构定义。地址偏移量:0x02，初值:0x00，宽度:8
 寄存器说明: DP0的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0         : 2;  /* bit[0-1]: 保留; */
        unsigned char  dp0_portdatamode   : 2;  /* bit[2-3]: DP0 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp0_nextinvertbank : 1;  /* bit[4]  : DP0 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_1         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_2         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP0_PORTCTRL_UNION;
#endif
#define MST_DP0_PORTCTRL_dp0_portdatamode_START    (2)
#define MST_DP0_PORTCTRL_dp0_portdatamode_END      (3)
#define MST_DP0_PORTCTRL_dp0_nextinvertbank_START  (4)
#define MST_DP0_PORTCTRL_dp0_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP0_BLOCKCTRL1_UNION
 结构说明  : DP0_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x03，初值:0x1F，宽度:8
 寄存器说明: DP0 Block控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_wordlength : 6;  /* bit[0-5]: DP0 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP0_BLOCKCTRL1_UNION;
#endif
#define MST_DP0_BLOCKCTRL1_dp0_wordlength_START  (0)
#define MST_DP0_BLOCKCTRL1_dp0_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP0_PREPARESTATUS_UNION
 结构说明  : DP0_PREPARESTATUS 寄存器结构定义。地址偏移量:0x04，初值:0x00，宽度:8
 寄存器说明: DP0的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_n_finished_channel1 : 1;  /* bit[0]  : DP0通道状态未完成指示:
                                                                  1'b0:完成;
                                                                  1'b1:未完成; */
        unsigned char  reserved                : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP0_PREPARESTATUS_UNION;
#endif
#define MST_DP0_PREPARESTATUS_dp0_n_finished_channel1_START  (0)
#define MST_DP0_PREPARESTATUS_dp0_n_finished_channel1_END    (0)


/*****************************************************************************
 结构名    : MST_DP0_PREPARECTRL_UNION
 结构说明  : DP0_PREPARECTRL 寄存器结构定义。地址偏移量:0x05，初值:0x00，宽度:8
 寄存器说明: DP0的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_prepare_channel1 : 1;  /* bit[0]  : DP0使能通道准备:
                                                               1'b0:不使能;
                                                               1'b1:使能； */
        unsigned char  reserved             : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP0_PREPARECTRL_UNION;
#endif
#define MST_DP0_PREPARECTRL_dp0_prepare_channel1_START  (0)
#define MST_DP0_PREPARECTRL_dp0_prepare_channel1_END    (0)


/*****************************************************************************
 结构名    : MST_DP0_CHANNELEN_BANK0_UNION
 结构说明  : DP0_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x20，初值:0x00，宽度:8
 寄存器说明: DP0的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_enable_channel1_bank0 : 1;  /* bit[0]  : DP0的Channel1使能:(bank0)
                                                                    1'b0:不使能;
                                                                    1'b1:使能; */
        unsigned char  reserved                  : 7;  /* bit[1-7]: 保留 */
    } reg;
} MST_DP0_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP0_CHANNELEN_BANK0_dp0_enable_channel1_bank0_START  (0)
#define MST_DP0_CHANNELEN_BANK0_dp0_enable_channel1_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP0_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP0_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x22，初值:0x00，宽度:8
 寄存器说明: DP0的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_sampleintervallow_bank0 : 8;  /* bit[0-7]: DP0的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP0_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP0_SAMPLECTRL1_BANK0_dp0_sampleintervallow_bank0_START  (0)
#define MST_DP0_SAMPLECTRL1_BANK0_dp0_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP0_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x23，初值:0x00，宽度:8
 寄存器说明: DP0的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: DP0的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP0_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP0_SAMPLECTRL2_BANK0_dp0_sampleintervalhigh_bank0_START  (0)
#define MST_DP0_SAMPLECTRL2_BANK0_dp0_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP0_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x24，初值:0x00，宽度:8
 寄存器说明: DP0的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_offset1_bank0 : 8;  /* bit[0-7]: DP0的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP0_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP0_OFFSETCTRL1_BANK0_dp0_offset1_bank0_START  (0)
#define MST_DP0_OFFSETCTRL1_BANK0_dp0_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP0_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x25，初值:0x00，宽度:8
 寄存器说明: DP0的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_offset2_bank0 : 8;  /* bit[0-7]: DP0的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP0_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP0_OFFSETCTRL2_BANK0_dp0_offset2_bank0_START  (0)
#define MST_DP0_OFFSETCTRL2_BANK0_dp0_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_HCTRL_BANK0_UNION
 结构说明  : DP0_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x26，初值:0x11，宽度:8
 寄存器说明: DP0的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_hstop_bank0  : 4;  /* bit[0-3]: DP0传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp0_hstart_bank0 : 4;  /* bit[4-7]: DP0传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP0_HCTRL_BANK0_UNION;
#endif
#define MST_DP0_HCTRL_BANK0_dp0_hstop_bank0_START   (0)
#define MST_DP0_HCTRL_BANK0_dp0_hstop_bank0_END     (3)
#define MST_DP0_HCTRL_BANK0_dp0_hstart_bank0_START  (4)
#define MST_DP0_HCTRL_BANK0_dp0_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_LANECTRL_BANK0_UNION
 结构说明  : DP0_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x28，初值:0x00，宽度:8
 寄存器说明: DP0的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_lanecontrol_bank0 : 3;  /* bit[0-2]: DP0的数据线控制，表示DP0在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP0_LANECTRL_BANK0_UNION;
#endif
#define MST_DP0_LANECTRL_BANK0_dp0_lanecontrol_bank0_START  (0)
#define MST_DP0_LANECTRL_BANK0_dp0_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP0_CHANNELEN_BANK1_UNION
 结构说明  : DP0_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x30，初值:0x00，宽度:8
 寄存器说明: DP0的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_enable_channel1_bank1 : 1;  /* bit[0]  : DP0的Channel1使能:(bank1)
                                                                    1'b0:不使能;
                                                                    1'b1:使能; */
        unsigned char  reserved                  : 7;  /* bit[1-7]: 保留 */
    } reg;
} MST_DP0_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP0_CHANNELEN_BANK1_dp0_enable_channel1_bank1_START  (0)
#define MST_DP0_CHANNELEN_BANK1_dp0_enable_channel1_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP0_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP0_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x32，初值:0x00，宽度:8
 寄存器说明: DP0的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_sampleintervallow_bank1 : 8;  /* bit[0-7]: DP0的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP0_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP0_SAMPLECTRL1_BANK1_dp0_sampleintervallow_bank1_START  (0)
#define MST_DP0_SAMPLECTRL1_BANK1_dp0_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP0_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x33，初值:0x00，宽度:8
 寄存器说明: DP0的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: DP0的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP0_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP0_SAMPLECTRL2_BANK1_dp0_sampleintervalhigh_bank1_START  (0)
#define MST_DP0_SAMPLECTRL2_BANK1_dp0_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP0_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x34，初值:0x00，宽度:8
 寄存器说明: DP0的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_offset1_bank1 : 8;  /* bit[0-7]: DP0的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP0_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP0_OFFSETCTRL1_BANK1_dp0_offset1_bank1_START  (0)
#define MST_DP0_OFFSETCTRL1_BANK1_dp0_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP0_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x35，初值:0x00，宽度:8
 寄存器说明: DP0的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_offset2_bank1 : 8;  /* bit[0-7]: DP0的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP0_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP0_OFFSETCTRL2_BANK1_dp0_offset2_bank1_START  (0)
#define MST_DP0_OFFSETCTRL2_BANK1_dp0_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_HCTRL_BANK1_UNION
 结构说明  : DP0_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x36，初值:0x11，宽度:8
 寄存器说明: DP0的传输子帧参数控制寄存器(bank1。)
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_hstop_bank1  : 4;  /* bit[0-3]: DP0传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp0_hstart_bank1 : 4;  /* bit[4-7]: DP0传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP0_HCTRL_BANK1_UNION;
#endif
#define MST_DP0_HCTRL_BANK1_dp0_hstop_bank1_START   (0)
#define MST_DP0_HCTRL_BANK1_dp0_hstop_bank1_END     (3)
#define MST_DP0_HCTRL_BANK1_dp0_hstart_bank1_START  (4)
#define MST_DP0_HCTRL_BANK1_dp0_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP0_LANECTRL_BANK1_UNION
 结构说明  : DP0_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x38，初值:0x00，宽度:8
 寄存器说明: DP0的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_lanecontrol_bank1 : 3;  /* bit[0-2]: DP0的数据线控制，表示DP0在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP0_LANECTRL_BANK1_UNION;
#endif
#define MST_DP0_LANECTRL_BANK1_dp0_lanecontrol_bank1_START  (0)
#define MST_DP0_LANECTRL_BANK1_dp0_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP_IntStat1_UNION
 结构说明  : DP_IntStat1 寄存器结构定义。地址偏移量:0x40，初值:0x00，宽度:8
 寄存器说明: data_port屏蔽后中断状态寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0    : 3;  /* bit[0-2]: 保留。 */
        unsigned char  dp0_int_mstat : 1;  /* bit[3]  : data_port0屏蔽后的相关汇集的中断
                                                        1'b1:有中断；
                                                        1'b0:无中断。 */
        unsigned char  dp1_int_mstat : 1;  /* bit[4]  : data_port1屏蔽后的相关汇集的中断
                                                        1'b1:有中断；
                                                        1'b0:无中断。 */
        unsigned char  dp2_int_mstat : 1;  /* bit[5]  : data_port2屏蔽后的相关汇集的中断
                                                        1'b1:有中断；
                                                        1'b0:无中断。 */
        unsigned char  dp3_int_mstat : 1;  /* bit[6]  : data_port3屏蔽后的相关汇集的中断
                                                        1'b1:有中断；
                                                        1'b0:无中断。 */
        unsigned char  reserved_1    : 1;  /* bit[7]  : 保留。 */
    } reg;
} MST_DP_IntStat1_UNION;
#endif
#define MST_DP_IntStat1_dp0_int_mstat_START  (3)
#define MST_DP_IntStat1_dp0_int_mstat_END    (3)
#define MST_DP_IntStat1_dp1_int_mstat_START  (4)
#define MST_DP_IntStat1_dp1_int_mstat_END    (4)
#define MST_DP_IntStat1_dp2_int_mstat_START  (5)
#define MST_DP_IntStat1_dp2_int_mstat_END    (5)
#define MST_DP_IntStat1_dp3_int_mstat_START  (6)
#define MST_DP_IntStat1_dp3_int_mstat_END    (6)


/*****************************************************************************
 结构名    : MST_DP_IntStat2_UNION
 结构说明  : DP_IntStat2 寄存器结构定义。地址偏移量:0x42，初值:0x00，宽度:8
 寄存器说明: data_port屏蔽后中断状态寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_int_mstat  : 1;  /* bit[0]: data_port4屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  dp5_int_mstat  : 1;  /* bit[1]: data_port5屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  dp6_int_mstat  : 1;  /* bit[2]: data_port6屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  dp7_int_mstat  : 1;  /* bit[3]: data_port7屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  dp8_int_mstat  : 1;  /* bit[4]: data_port8屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  dp9_int_mstat  : 1;  /* bit[5]: data_port9屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  dp10_int_mstat : 1;  /* bit[6]: data_port10屏蔽后的相关汇集的中断
                                                       1'b1:有中断；
                                                       1'b0:无中断。 */
        unsigned char  reserved       : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_DP_IntStat2_UNION;
#endif
#define MST_DP_IntStat2_dp4_int_mstat_START   (0)
#define MST_DP_IntStat2_dp4_int_mstat_END     (0)
#define MST_DP_IntStat2_dp5_int_mstat_START   (1)
#define MST_DP_IntStat2_dp5_int_mstat_END     (1)
#define MST_DP_IntStat2_dp6_int_mstat_START   (2)
#define MST_DP_IntStat2_dp6_int_mstat_END     (2)
#define MST_DP_IntStat2_dp7_int_mstat_START   (3)
#define MST_DP_IntStat2_dp7_int_mstat_END     (3)
#define MST_DP_IntStat2_dp8_int_mstat_START   (4)
#define MST_DP_IntStat2_dp8_int_mstat_END     (4)
#define MST_DP_IntStat2_dp9_int_mstat_START   (5)
#define MST_DP_IntStat2_dp9_int_mstat_END     (5)
#define MST_DP_IntStat2_dp10_int_mstat_START  (6)
#define MST_DP_IntStat2_dp10_int_mstat_END    (6)


/*****************************************************************************
 结构名    : MST_DP_IntStat3_UNION
 结构说明  : DP_IntStat3 寄存器结构定义。地址偏移量:0x43，初值:0x00，宽度:8
 寄存器说明: data_port屏蔽后中断状态寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_int_mstat : 1;  /* bit[0]  : data_port11屏蔽后的相关汇集的中断
                                                         1'b1:有中断；
                                                         1'b0:无中断。 */
        unsigned char  dp12_int_mstat : 1;  /* bit[1]  : data_port12屏蔽后的相关汇集的中断
                                                         1'b1:有中断；
                                                         1'b0:无中断。 */
        unsigned char  dp13_int_mstat : 1;  /* bit[2]  : data_port13屏蔽后的相关汇集的中断
                                                         1'b1:有中断；
                                                         1'b0:无中断。 */
        unsigned char  dp14_int_mstat : 1;  /* bit[3]  : data_port14屏蔽后的相关汇集的中断
                                                         1'b1:有中断；
                                                         1'b0:无中断。 */
        unsigned char  reserved       : 4;  /* bit[4-7]: 保留。 */
    } reg;
} MST_DP_IntStat3_UNION;
#endif
#define MST_DP_IntStat3_dp11_int_mstat_START  (0)
#define MST_DP_IntStat3_dp11_int_mstat_END    (0)
#define MST_DP_IntStat3_dp12_int_mstat_START  (1)
#define MST_DP_IntStat3_dp12_int_mstat_END    (1)
#define MST_DP_IntStat3_dp13_int_mstat_START  (2)
#define MST_DP_IntStat3_dp13_int_mstat_END    (2)
#define MST_DP_IntStat3_dp14_int_mstat_START  (3)
#define MST_DP_IntStat3_dp14_int_mstat_END    (3)


/*****************************************************************************
 结构名    : MST_SCP_DEVNUMBER_UNION
 结构说明  : SCP_DEVNUMBER 寄存器结构定义。地址偏移量:0x46，初值:0x00，宽度:8
 寄存器说明: Slave devicenumber配置寄存器（已删除）。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved : 8;  /* bit[0-7]: 保留; */
    } reg;
} MST_SCP_DEVNUMBER_UNION;
#endif


/*****************************************************************************
 结构名    : MST_SCP_FRAMECTRL_BANK0_UNION
 结构说明  : SCP_FRAMECTRL_BANK0 寄存器结构定义。地址偏移量:0x60，初值:0x00，宽度:8
 寄存器说明: 帧形状配置寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  columncontrol_bank0 : 3;  /* bit[0-2]: 帧形状列数配置:
                                                              3'd0：2列；
                                                              3'd1：4列；
                                                              3'd2：6列；
                                                              3'd3：8列；
                                                              3'd4：10列；
                                                              3'd5：12列；
                                                              3'd6：14列；
                                                              3'd7：16列； */
        unsigned char  rowcontrol_bank0    : 5;  /* bit[3-7]: 帧形状行数配置：
                                                              5'd0：48行；
                                                              5'd1：50行；
                                                              5'd2：60行；
                                                              5'd3：64行；
                                                              5'd4：75行；
                                                              5'd5：80行；
                                                              5'd6：125行；
                                                              5'd7：147行；
                                                              5'd8：96行；
                                                              5'd9：100行；
                                                              5'd10：120行；
                                                              5'd11：128行；
                                                              5'd12：150行；
                                                              5'd13：160行；
                                                              5'd14：250行；
                                                              5'd15：48行；
                                                              5'd16：192行；
                                                              5'd17：200行；
                                                              5'd18：240行；
                                                              5'd19：256行；
                                                              5'd20：72行；
                                                              5'd21：144行；
                                                              5'd22：90行；
                                                              5'd23：180行；
                                                              others：48行 */
    } reg;
} MST_SCP_FRAMECTRL_BANK0_UNION;
#endif
#define MST_SCP_FRAMECTRL_BANK0_columncontrol_bank0_START  (0)
#define MST_SCP_FRAMECTRL_BANK0_columncontrol_bank0_END    (2)
#define MST_SCP_FRAMECTRL_BANK0_rowcontrol_bank0_START     (3)
#define MST_SCP_FRAMECTRL_BANK0_rowcontrol_bank0_END       (7)


/*****************************************************************************
 结构名    : MST_SCP_FRAMECTRL_BANK1_UNION
 结构说明  : SCP_FRAMECTRL_BANK1 寄存器结构定义。地址偏移量:0x70，初值:0x00，宽度:8
 寄存器说明: 帧形状配置寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  columncontrol_bank1 : 3;  /* bit[0-2]: 帧形状列数配置:
                                                              3'd0：2列；
                                                              3'd1：4列；
                                                              3'd2：6列；
                                                              3'd3：8列；
                                                              3'd4：10列；
                                                              3'd5：12列；
                                                              3'd6：14列；
                                                              3'd7：16列； */
        unsigned char  rowcontrol_bank1    : 5;  /* bit[3-7]: 帧形状行数配置：
                                                              5'd0：48行；
                                                              5'd1：50行；
                                                              5'd2：60行；
                                                              5'd3：64行；
                                                              5'd4：75行；
                                                              5'd5：80行；
                                                              5'd6：125行；
                                                              5'd7：147行；
                                                              5'd8：96行；
                                                              5'd9：100行；
                                                              5'd10：120行；
                                                              5'd11：128行；
                                                              5'd12：150行；
                                                              5'd13：160行；
                                                              5'd14：250行；
                                                              5'd15：48行；
                                                              5'd16：192行；
                                                              5'd17：200行；
                                                              5'd18：240行；
                                                              5'd19：256行；
                                                              5'd20：72行；
                                                              5'd21：144行；
                                                              5'd22：90行；
                                                              5'd23：180行；
                                                              others：48行 */
    } reg;
} MST_SCP_FRAMECTRL_BANK1_UNION;
#endif
#define MST_SCP_FRAMECTRL_BANK1_columncontrol_bank1_START  (0)
#define MST_SCP_FRAMECTRL_BANK1_columncontrol_bank1_END    (2)
#define MST_SCP_FRAMECTRL_BANK1_rowcontrol_bank1_START     (3)
#define MST_SCP_FRAMECTRL_BANK1_rowcontrol_bank1_END       (7)


/*****************************************************************************
 结构名    : MST_CLKEN1_UNION
 结构说明  : CLKEN1 寄存器结构定义。地址偏移量:0x80，初值:0x84，宽度:8
 寄存器说明: 时钟门控配置寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  frame_ctrl_clken : 1;  /* bit[0]: frame_ctrl模块时钟使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port_clken  : 1;  /* bit[1]: data_port模块总时钟使能：（注意，还要配置data_port子模块时钟使能）
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  sw_clk_o_div_en  : 1;  /* bit[2]: sw_clk分频器使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  rx0_clken        : 1;  /* bit[3]: rx0模块时钟使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  tx0_clken        : 1;  /* bit[4]: tx0模块时钟使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  rx1_clken        : 1;  /* bit[5]: rx1模块时钟使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  tx1_clken        : 1;  /* bit[6]: tx1模块时钟使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  sysctrl_clken    : 1;  /* bit[7]: sysctrl模块时钟使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
    } reg;
} MST_CLKEN1_UNION;
#endif
#define MST_CLKEN1_frame_ctrl_clken_START  (0)
#define MST_CLKEN1_frame_ctrl_clken_END    (0)
#define MST_CLKEN1_data_port_clken_START   (1)
#define MST_CLKEN1_data_port_clken_END     (1)
#define MST_CLKEN1_sw_clk_o_div_en_START   (2)
#define MST_CLKEN1_sw_clk_o_div_en_END     (2)
#define MST_CLKEN1_rx0_clken_START         (3)
#define MST_CLKEN1_rx0_clken_END           (3)
#define MST_CLKEN1_tx0_clken_START         (4)
#define MST_CLKEN1_tx0_clken_END           (4)
#define MST_CLKEN1_rx1_clken_START         (5)
#define MST_CLKEN1_rx1_clken_END           (5)
#define MST_CLKEN1_tx1_clken_START         (6)
#define MST_CLKEN1_tx1_clken_END           (6)
#define MST_CLKEN1_sysctrl_clken_START     (7)
#define MST_CLKEN1_sysctrl_clken_END       (7)


/*****************************************************************************
 结构名    : MST_CLKEN2_UNION
 结构说明  : CLKEN2 寄存器结构定义。地址偏移量:0x81，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  data_port1_clken : 1;  /* bit[0]: data_port1l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port2_clken : 1;  /* bit[1]: data_port2l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port3_clken : 1;  /* bit[2]: data_port3l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port4_clken : 1;  /* bit[3]: data_port4l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port5_clken : 1;  /* bit[4]: data_port5l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port6_clken : 1;  /* bit[5]: data_port6l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port7_clken : 1;  /* bit[6]: data_port7l模块时钟使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  reserved         : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_CLKEN2_UNION;
#endif
#define MST_CLKEN2_data_port1_clken_START  (0)
#define MST_CLKEN2_data_port1_clken_END    (0)
#define MST_CLKEN2_data_port2_clken_START  (1)
#define MST_CLKEN2_data_port2_clken_END    (1)
#define MST_CLKEN2_data_port3_clken_START  (2)
#define MST_CLKEN2_data_port3_clken_END    (2)
#define MST_CLKEN2_data_port4_clken_START  (3)
#define MST_CLKEN2_data_port4_clken_END    (3)
#define MST_CLKEN2_data_port5_clken_START  (4)
#define MST_CLKEN2_data_port5_clken_END    (4)
#define MST_CLKEN2_data_port6_clken_START  (5)
#define MST_CLKEN2_data_port6_clken_END    (5)
#define MST_CLKEN2_data_port7_clken_START  (6)
#define MST_CLKEN2_data_port7_clken_END    (6)


/*****************************************************************************
 结构名    : MST_CLKEN3_UNION
 结构说明  : CLKEN3 寄存器结构定义。地址偏移量:0x82，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  data_port8_clken  : 1;  /* bit[0]: data_port8l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port9_clken  : 1;  /* bit[1]: data_port9l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port10_clken : 1;  /* bit[2]: data_port10l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port11_clken : 1;  /* bit[3]: data_port11l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port12_clken : 1;  /* bit[4]: data_port12l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port13_clken : 1;  /* bit[5]: data_port13l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port14_clken : 1;  /* bit[6]: data_port14l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  reserved          : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_CLKEN3_UNION;
#endif
#define MST_CLKEN3_data_port8_clken_START   (0)
#define MST_CLKEN3_data_port8_clken_END     (0)
#define MST_CLKEN3_data_port9_clken_START   (1)
#define MST_CLKEN3_data_port9_clken_END     (1)
#define MST_CLKEN3_data_port10_clken_START  (2)
#define MST_CLKEN3_data_port10_clken_END    (2)
#define MST_CLKEN3_data_port11_clken_START  (3)
#define MST_CLKEN3_data_port11_clken_END    (3)
#define MST_CLKEN3_data_port12_clken_START  (4)
#define MST_CLKEN3_data_port12_clken_END    (4)
#define MST_CLKEN3_data_port13_clken_START  (5)
#define MST_CLKEN3_data_port13_clken_END    (5)
#define MST_CLKEN3_data_port14_clken_START  (6)
#define MST_CLKEN3_data_port14_clken_END    (6)


/*****************************************************************************
 结构名    : MST_CLKEN4_UNION
 结构说明  : CLKEN4 寄存器结构定义。地址偏移量:0x83，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器4。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0 : 6;  /* bit[0-5]: 保留。 */
        unsigned char  bus_rst_en : 1;  /* bit[6]  : bus_rst使能：
                                                     1'b1:总线复位；
                                                     1'b0:总线解复位； */
        unsigned char  reserved_1 : 1;  /* bit[7]  : 保留。 */
    } reg;
} MST_CLKEN4_UNION;
#endif
#define MST_CLKEN4_bus_rst_en_START  (6)
#define MST_CLKEN4_bus_rst_en_END    (6)


/*****************************************************************************
 结构名    : MST_CLKEN5_UNION
 结构说明  : CLKEN5 寄存器结构定义。地址偏移量:0x84，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器5(删除)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved : 8;  /* bit[0-7]: 保留。 */
    } reg;
} MST_CLKEN5_UNION;
#endif


/*****************************************************************************
 结构名    : MST_CLKEN6_UNION
 结构说明  : CLKEN6 寄存器结构定义。地址偏移量:0x85，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器6。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp_fifo1_up_clken : 1;  /* bit[0]: dp_fifo1上行l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo2_up_clken : 1;  /* bit[1]: dp_fifo2l上行模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo3_up_clken : 1;  /* bit[2]: dp_fifo3l上行模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo4_up_clken : 1;  /* bit[3]: dp_fifo4l上行模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo5_up_clken : 1;  /* bit[4]: dp_fifo5l上行模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo6_up_clken : 1;  /* bit[5]: dp_fifo6l上行模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo7_up_clken : 1;  /* bit[6]: dp_fifo7上行l模块时钟使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  reserved          : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_CLKEN6_UNION;
#endif
#define MST_CLKEN6_dp_fifo1_up_clken_START  (0)
#define MST_CLKEN6_dp_fifo1_up_clken_END    (0)
#define MST_CLKEN6_dp_fifo2_up_clken_START  (1)
#define MST_CLKEN6_dp_fifo2_up_clken_END    (1)
#define MST_CLKEN6_dp_fifo3_up_clken_START  (2)
#define MST_CLKEN6_dp_fifo3_up_clken_END    (2)
#define MST_CLKEN6_dp_fifo4_up_clken_START  (3)
#define MST_CLKEN6_dp_fifo4_up_clken_END    (3)
#define MST_CLKEN6_dp_fifo5_up_clken_START  (4)
#define MST_CLKEN6_dp_fifo5_up_clken_END    (4)
#define MST_CLKEN6_dp_fifo6_up_clken_START  (5)
#define MST_CLKEN6_dp_fifo6_up_clken_END    (5)
#define MST_CLKEN6_dp_fifo7_up_clken_START  (6)
#define MST_CLKEN6_dp_fifo7_up_clken_END    (6)


/*****************************************************************************
 结构名    : MST_CLKEN7_UNION
 结构说明  : CLKEN7 寄存器结构定义。地址偏移量:0x86，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器7。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0         : 2;  /* bit[0-1]: 保留。 */
        unsigned char  dp_fifo10_dn_clken : 1;  /* bit[2]  : dp_fifo10下行l模块时钟使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo11_dn_clken : 1;  /* bit[3]  : dp_fifo11下行模块时钟使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo12_dn_clken : 1;  /* bit[4]  : dp_fifo12下行模块时钟使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo13_dn_clken : 1;  /* bit[5]  : dp_fifo13下行l模块时钟使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo14_dn_clken : 1;  /* bit[6]  : dp_fifo14下行l模块时钟使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  reserved_1         : 1;  /* bit[7]  : 保留。 */
    } reg;
} MST_CLKEN7_UNION;
#endif
#define MST_CLKEN7_dp_fifo10_dn_clken_START  (2)
#define MST_CLKEN7_dp_fifo10_dn_clken_END    (2)
#define MST_CLKEN7_dp_fifo11_dn_clken_START  (3)
#define MST_CLKEN7_dp_fifo11_dn_clken_END    (3)
#define MST_CLKEN7_dp_fifo12_dn_clken_START  (4)
#define MST_CLKEN7_dp_fifo12_dn_clken_END    (4)
#define MST_CLKEN7_dp_fifo13_dn_clken_START  (5)
#define MST_CLKEN7_dp_fifo13_dn_clken_END    (5)
#define MST_CLKEN7_dp_fifo14_dn_clken_START  (6)
#define MST_CLKEN7_dp_fifo14_dn_clken_END    (6)


/*****************************************************************************
 结构名    : MST_CLKEN8_UNION
 结构说明  : CLKEN8 寄存器结构定义。地址偏移量:0x87，初值:0x00，宽度:8
 寄存器说明: 时钟门控配置寄存器8。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp_fifo8_up_clken : 1;  /* bit[0]  : dp_fifo8上行l模块时钟使能:
                                                            1'b0:不使能;
                                                            1'b1:使能; */
        unsigned char  dp_fifo9_up_clken : 1;  /* bit[1]  : dp_fifo9l上行模块时钟使能:
                                                            1'b0:不使能;
                                                            1'b1:使能; */
        unsigned char  reserved          : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_CLKEN8_UNION;
#endif
#define MST_CLKEN8_dp_fifo8_up_clken_START  (0)
#define MST_CLKEN8_dp_fifo8_up_clken_END    (0)
#define MST_CLKEN8_dp_fifo9_up_clken_START  (1)
#define MST_CLKEN8_dp_fifo9_up_clken_END    (1)


/*****************************************************************************
 结构名    : MST_RSTEN1_UNION
 结构说明  : RSTEN1 寄存器结构定义。地址偏移量:0x88，初值:0x80，宽度:8
 寄存器说明: 软复位配置寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  frame_ctrl_rsten : 1;  /* bit[0]: frame_ctrl模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port_rsten  : 1;  /* bit[1]: data_port模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  reserved         : 1;  /* bit[2]: 保留。 */
        unsigned char  rx0_rsten        : 1;  /* bit[3]: rx0模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  tx0_rsten        : 1;  /* bit[4]: tx0模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  rx1_rsten        : 1;  /* bit[5]: rx1模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  tx1_rsten        : 1;  /* bit[6]: tx1模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  sysctrl_rsten    : 1;  /* bit[7]: sysctrl模块解复位使能：
                                                         1'b0:不使能;
                                                         1'b1:使能; */
    } reg;
} MST_RSTEN1_UNION;
#endif
#define MST_RSTEN1_frame_ctrl_rsten_START  (0)
#define MST_RSTEN1_frame_ctrl_rsten_END    (0)
#define MST_RSTEN1_data_port_rsten_START   (1)
#define MST_RSTEN1_data_port_rsten_END     (1)
#define MST_RSTEN1_rx0_rsten_START         (3)
#define MST_RSTEN1_rx0_rsten_END           (3)
#define MST_RSTEN1_tx0_rsten_START         (4)
#define MST_RSTEN1_tx0_rsten_END           (4)
#define MST_RSTEN1_rx1_rsten_START         (5)
#define MST_RSTEN1_rx1_rsten_END           (5)
#define MST_RSTEN1_tx1_rsten_START         (6)
#define MST_RSTEN1_tx1_rsten_END           (6)
#define MST_RSTEN1_sysctrl_rsten_START     (7)
#define MST_RSTEN1_sysctrl_rsten_END       (7)


/*****************************************************************************
 结构名    : MST_RSTEN2_UNION
 结构说明  : RSTEN2 寄存器结构定义。地址偏移量:0x89，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  data_port1_rsten : 1;  /* bit[0]: data_port1l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port2_rsten : 1;  /* bit[1]: data_port2l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port3_rsten : 1;  /* bit[2]: data_port3l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port4_rsten : 1;  /* bit[3]: data_port4l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port5_rsten : 1;  /* bit[4]: data_port5l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port6_rsten : 1;  /* bit[5]: data_port6l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  data_port7_rsten : 1;  /* bit[6]: data_port7l模块解复位使能:
                                                         1'b0:不使能;
                                                         1'b1:使能; */
        unsigned char  reserved         : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_RSTEN2_UNION;
#endif
#define MST_RSTEN2_data_port1_rsten_START  (0)
#define MST_RSTEN2_data_port1_rsten_END    (0)
#define MST_RSTEN2_data_port2_rsten_START  (1)
#define MST_RSTEN2_data_port2_rsten_END    (1)
#define MST_RSTEN2_data_port3_rsten_START  (2)
#define MST_RSTEN2_data_port3_rsten_END    (2)
#define MST_RSTEN2_data_port4_rsten_START  (3)
#define MST_RSTEN2_data_port4_rsten_END    (3)
#define MST_RSTEN2_data_port5_rsten_START  (4)
#define MST_RSTEN2_data_port5_rsten_END    (4)
#define MST_RSTEN2_data_port6_rsten_START  (5)
#define MST_RSTEN2_data_port6_rsten_END    (5)
#define MST_RSTEN2_data_port7_rsten_START  (6)
#define MST_RSTEN2_data_port7_rsten_END    (6)


/*****************************************************************************
 结构名    : MST_RSTEN3_UNION
 结构说明  : RSTEN3 寄存器结构定义。地址偏移量:0x8A，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  data_port8_rsten  : 1;  /* bit[0]: data_port8l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port9_rsten  : 1;  /* bit[1]: data_port9l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port10_rsten : 1;  /* bit[2]: data_port10l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port11_rsten : 1;  /* bit[3]: data_port11l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port12_rsten : 1;  /* bit[4]: data_port12l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port13_rsten : 1;  /* bit[5]: data_port13l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  data_port14_rsten : 1;  /* bit[6]: data_port14l模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  reserved          : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_RSTEN3_UNION;
#endif
#define MST_RSTEN3_data_port8_rsten_START   (0)
#define MST_RSTEN3_data_port8_rsten_END     (0)
#define MST_RSTEN3_data_port9_rsten_START   (1)
#define MST_RSTEN3_data_port9_rsten_END     (1)
#define MST_RSTEN3_data_port10_rsten_START  (2)
#define MST_RSTEN3_data_port10_rsten_END    (2)
#define MST_RSTEN3_data_port11_rsten_START  (3)
#define MST_RSTEN3_data_port11_rsten_END    (3)
#define MST_RSTEN3_data_port12_rsten_START  (4)
#define MST_RSTEN3_data_port12_rsten_END    (4)
#define MST_RSTEN3_data_port13_rsten_START  (5)
#define MST_RSTEN3_data_port13_rsten_END    (5)
#define MST_RSTEN3_data_port14_rsten_START  (6)
#define MST_RSTEN3_data_port14_rsten_END    (6)


/*****************************************************************************
 结构名    : MST_RSTEN4_UNION
 结构说明  : RSTEN4 寄存器结构定义。地址偏移量:0x8B，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器4(删除)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved : 8;  /* bit[0-7]: 保留。 */
    } reg;
} MST_RSTEN4_UNION;
#endif


/*****************************************************************************
 结构名    : MST_RSTEN5_UNION
 结构说明  : RSTEN5 寄存器结构定义。地址偏移量:0x8C，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器5(删除)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved : 8;  /* bit[0-7]: 保留。 */
    } reg;
} MST_RSTEN5_UNION;
#endif


/*****************************************************************************
 结构名    : MST_RSTEN6_UNION
 结构说明  : RSTEN6 寄存器结构定义。地址偏移量:0x8D，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器6。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp_fifo1_up_rsten : 1;  /* bit[0]: dp_fifo1上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo2_up_rsten : 1;  /* bit[1]: dp_fifo2上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo3_up_rsten : 1;  /* bit[2]: dp_fifo3上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo4_up_rsten : 1;  /* bit[3]: dp_fifo4上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo5_up_rsten : 1;  /* bit[4]: dp_fifo5上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo6_up_rsten : 1;  /* bit[5]: dp_fifo6上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  dp_fifo7_up_rsten : 1;  /* bit[6]: dp_fifo7上行模块解复位使能:
                                                          1'b0:不使能;
                                                          1'b1:使能; */
        unsigned char  reserved          : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_RSTEN6_UNION;
#endif
#define MST_RSTEN6_dp_fifo1_up_rsten_START  (0)
#define MST_RSTEN6_dp_fifo1_up_rsten_END    (0)
#define MST_RSTEN6_dp_fifo2_up_rsten_START  (1)
#define MST_RSTEN6_dp_fifo2_up_rsten_END    (1)
#define MST_RSTEN6_dp_fifo3_up_rsten_START  (2)
#define MST_RSTEN6_dp_fifo3_up_rsten_END    (2)
#define MST_RSTEN6_dp_fifo4_up_rsten_START  (3)
#define MST_RSTEN6_dp_fifo4_up_rsten_END    (3)
#define MST_RSTEN6_dp_fifo5_up_rsten_START  (4)
#define MST_RSTEN6_dp_fifo5_up_rsten_END    (4)
#define MST_RSTEN6_dp_fifo6_up_rsten_START  (5)
#define MST_RSTEN6_dp_fifo6_up_rsten_END    (5)
#define MST_RSTEN6_dp_fifo7_up_rsten_START  (6)
#define MST_RSTEN6_dp_fifo7_up_rsten_END    (6)


/*****************************************************************************
 结构名    : MST_RSTEN7_UNION
 结构说明  : RSTEN7 寄存器结构定义。地址偏移量:0x8E，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器7。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0         : 2;  /* bit[0-1]: 保留。 */
        unsigned char  dp_fifo10_dn_rsten : 1;  /* bit[2]  : dp_fifo10下行模块解复位使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo11_dn_rsten : 1;  /* bit[3]  : dp_fifo11下行模块解复位使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo12_dn_rsten : 1;  /* bit[4]  : dp_fifo12下行模块解复位使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo13_dn_rsten : 1;  /* bit[5]  : dp_fifo13下行模块解复位使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp_fifo14_dn_rsten : 1;  /* bit[6]  : dp_fifo14下行模块解复位使能:
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  reserved_1         : 1;  /* bit[7]  : 保留。 */
    } reg;
} MST_RSTEN7_UNION;
#endif
#define MST_RSTEN7_dp_fifo10_dn_rsten_START  (2)
#define MST_RSTEN7_dp_fifo10_dn_rsten_END    (2)
#define MST_RSTEN7_dp_fifo11_dn_rsten_START  (3)
#define MST_RSTEN7_dp_fifo11_dn_rsten_END    (3)
#define MST_RSTEN7_dp_fifo12_dn_rsten_START  (4)
#define MST_RSTEN7_dp_fifo12_dn_rsten_END    (4)
#define MST_RSTEN7_dp_fifo13_dn_rsten_START  (5)
#define MST_RSTEN7_dp_fifo13_dn_rsten_END    (5)
#define MST_RSTEN7_dp_fifo14_dn_rsten_START  (6)
#define MST_RSTEN7_dp_fifo14_dn_rsten_END    (6)


/*****************************************************************************
 结构名    : MST_RSTEN8_UNION
 结构说明  : RSTEN8 寄存器结构定义。地址偏移量:0x8F，初值:0x00，宽度:8
 寄存器说明: 软复位配置寄存器8。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp_fifo8_up_rsten : 1;  /* bit[0]  : dp_fifo8上行模块解复位使能:
                                                            1'b0:不使能;
                                                            1'b1:使能; */
        unsigned char  dp_fifo9_up_rsten : 1;  /* bit[1]  : dp_fifo9上行模块解复位使能:
                                                            1'b0:不使能;
                                                            1'b1:使能; */
        unsigned char  reserved          : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_RSTEN8_UNION;
#endif
#define MST_RSTEN8_dp_fifo8_up_rsten_START  (0)
#define MST_RSTEN8_dp_fifo8_up_rsten_END    (0)
#define MST_RSTEN8_dp_fifo9_up_rsten_START  (1)
#define MST_RSTEN8_dp_fifo9_up_rsten_END    (1)


/*****************************************************************************
 结构名    : MST_CLKDIVCTRL_UNION
 结构说明  : CLKDIVCTRL 寄存器结构定义。地址偏移量:0x90，初值:0x01，宽度:8
 寄存器说明: 时钟分频控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  sw_clk_div_ratio : 2;  /* bit[0-1]: sw_clk_ini分频比配置：
                                                           2'b00:二分频，12.288MHz;
                                                           2'b01:四分频，6.144MHz;
                                                           2'b10:八分频，3.072MHz;
                                                           2'b11:16分频，1.536MHz */
        unsigned char  reserved         : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_CLKDIVCTRL_UNION;
#endif
#define MST_CLKDIVCTRL_sw_clk_div_ratio_START  (0)
#define MST_CLKDIVCTRL_sw_clk_div_ratio_END    (1)


/*****************************************************************************
 结构名    : MST_CLK_WAKEUP_EN_UNION
 结构说明  : CLK_WAKEUP_EN 寄存器结构定义。地址偏移量:0x91，初值:0x00，宽度:8
 寄存器说明: 时钟唤醒&SSP配置寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  mst_clk_wakeup_en : 1;  /* bit[0]  : master侧时钟唤醒配置使能：(只写)
                                                            1'b0:不使能；
                                                            1'b1:使能 */
        unsigned char  reserved          : 7;  /* bit[1-7]: 保留。 */
    } reg;
} MST_CLK_WAKEUP_EN_UNION;
#endif
#define MST_CLK_WAKEUP_EN_mst_clk_wakeup_en_START  (0)
#define MST_CLK_WAKEUP_EN_mst_clk_wakeup_en_END    (0)


/*****************************************************************************
 结构名    : MST_TEST_MONITOR_UNION
 结构说明  : TEST_MONITOR 寄存器结构定义。地址偏移量:0x92，初值:0x01，宽度:8
 寄存器说明: SYSCTRL内部状态机指示信号。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  clkswitch_cs : 4;  /* bit[0-3]: clockswitch状态机指示。
                                                       4'b0001:M_NORMAL
                                                       4'b0011:M_STOPPING
                                                       4'b0010:M_STOOPED
                                                       4'b0110:M_STOP_RESTARTING
                                                       4'b0111:M_SWITCHING
                                                       4'b0101:M_SWITCHED
                                                       4'b0100:M_SWITCH_RESTARTING
                                                       4'b1100:S_NORAML
                                                       4'b1101:S_STOPPING
                                                       4'b1111:S_STOPPED
                                                       4'b1110:S_STOP_RESTARTING
                                                       4'b1010:S_SWITCHING
                                                       4'b1011:S_SWITCHED
                                                       4'b1001:S_SWITCH_RESTARTING */
        unsigned char  reserved     : 4;  /* bit[4-7]: 保留。 */
    } reg;
} MST_TEST_MONITOR_UNION;
#endif
#define MST_TEST_MONITOR_clkswitch_cs_START  (0)
#define MST_TEST_MONITOR_clkswitch_cs_END    (3)


/*****************************************************************************
 结构名    : MST_BANK_SW_MONITOR_UNION
 结构说明  : BANK_SW_MONITOR 寄存器结构定义。地址偏移量:0x93，初值:0x01，宽度:8
 寄存器说明: BANK切换指示状态信号。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  frame_bank0_sel     : 1;  /* bit[0]  : bank切换指示信号：
                                                              1'b0:非bank0
                                                              1'b1:bank0 */
        unsigned char  frame_bank1_sel     : 1;  /* bit[1]  : bank切换指示信号：
                                                              1'b0:非bank1
                                                              1'b1:bank1 */
        unsigned char  frame_ctrl_state    : 3;  /* bit[2-4]: frame_ctrl状态机跳转指示. */
        unsigned char  rx0_sync_state_curr : 3;  /* bit[5-7]: RX0内部同步状态机状态指示（debug用） */
    } reg;
} MST_BANK_SW_MONITOR_UNION;
#endif
#define MST_BANK_SW_MONITOR_frame_bank0_sel_START      (0)
#define MST_BANK_SW_MONITOR_frame_bank0_sel_END        (0)
#define MST_BANK_SW_MONITOR_frame_bank1_sel_START      (1)
#define MST_BANK_SW_MONITOR_frame_bank1_sel_END        (1)
#define MST_BANK_SW_MONITOR_frame_ctrl_state_START     (2)
#define MST_BANK_SW_MONITOR_frame_ctrl_state_END       (4)
#define MST_BANK_SW_MONITOR_rx0_sync_state_curr_START  (5)
#define MST_BANK_SW_MONITOR_rx0_sync_state_curr_END    (7)


/*****************************************************************************
 结构名    : MST_SLV_REG_SEL_UNION
 结构说明  : SLV_REG_SEL 寄存器结构定义。地址偏移量:0x9F，初值:0x00，宽度:8
 寄存器说明: slave寄存器访问选择。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  page_reg_sel : 1;  /* bit[0]  : 音频codec访问寄存器选择：
                                                       1'b0:访问device内部寄存器;
                                                       1'b1:访问640X内部寄存器 */
        unsigned char  reserved     : 7;  /* bit[1-7]: 保留。 */
    } reg;
} MST_SLV_REG_SEL_UNION;
#endif
#define MST_SLV_REG_SEL_page_reg_sel_START  (0)
#define MST_SLV_REG_SEL_page_reg_sel_END    (0)


/*****************************************************************************
 结构名    : MST_FIFO_LOOP_EN_UNION
 结构说明  : FIFO_LOOP_EN 寄存器结构定义。地址偏移量:0xA0，初值:0x00，宽度:8
 寄存器说明: 环回通路配置寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  fifo_loopback_sel : 4;  /* bit[0-3]: fifo环回通路选择：
                                                            4'b0000:DP0_FIFO通路
                                                            4'b0001:DP1_FIFO通路;
                                                            4'b0010:DP2_FIFO通路;
                                                            ……
                                                            4b'1110:DP14_FIFO通路; 
                                                            其他：无效,禁配.（只可选DP10~14） */
        unsigned char  reserved          : 4;  /* bit[4-7]: 保留。 */
    } reg;
} MST_FIFO_LOOP_EN_UNION;
#endif
#define MST_FIFO_LOOP_EN_fifo_loopback_sel_START  (0)
#define MST_FIFO_LOOP_EN_fifo_loopback_sel_END    (3)


/*****************************************************************************
 结构名    : MST_CTRL1_UNION
 结构说明  : CTRL1 寄存器结构定义。地址偏移量:0xA1，初值:0x11，宽度:8
 寄存器说明: 子模块控制配置寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  sync_timeout_en : 1;  /* bit[0]  : 同步超时检测使能
                                                          1'b0:不使能；
                                                          1'b1:使能 */
        unsigned char  dploopback0_en  : 1;  /* bit[1]  : TX0到RX0环回使能：
                                                          1'b0:不使能；
                                                          1'b1:使能 */
        unsigned char  dploopback1_en  : 1;  /* bit[2]  : TX1到RX1环回使能：
                                                          1'b0:不使能；
                                                          1'b1:使能 */
        unsigned char  frame_ctrl_en   : 1;  /* bit[3]  : frame_ctrl模块使能：
                                                          1'b0:不使能；
                                                          1'b1:使能 */
        unsigned char  dev_OFFSET_ctrl   : 4;  /* bit[4-7]: device_OFFSET地址控制。 */
    } reg;
} MST_CTRL1_UNION;
#endif
#define MST_CTRL1_sync_timeout_en_START  (0)
#define MST_CTRL1_sync_timeout_en_END    (0)
#define MST_CTRL1_dploopback0_en_START   (1)
#define MST_CTRL1_dploopback0_en_END     (1)
#define MST_CTRL1_dploopback1_en_START   (2)
#define MST_CTRL1_dploopback1_en_END     (2)
#define MST_CTRL1_frame_ctrl_en_START    (3)
#define MST_CTRL1_frame_ctrl_en_END      (3)
#define MST_CTRL1_dev_OFFSET_ctrl_START    (4)
#define MST_CTRL1_dev_OFFSET_ctrl_END      (7)


/*****************************************************************************
 结构名    : MST_CTRL2_UNION
 结构说明  : CTRL2 寄存器结构定义。地址偏移量:0xA2，初值:0x01，宽度:8
 寄存器说明: 子模块控制配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  enum_timeout_en : 1;  /* bit[0]  : 枚举超时检测使能
                                                          1'b0:不使能；
                                                          1'b1:使能 */
        unsigned char  reserved        : 7;  /* bit[1-7]: 保留。 */
    } reg;
} MST_CTRL2_UNION;
#endif
#define MST_CTRL2_enum_timeout_en_START  (0)
#define MST_CTRL2_enum_timeout_en_END    (0)


/*****************************************************************************
 结构名    : MST_SOURCESINK_CFG2_UNION
 结构说明  : SOURCESINK_CFG2 寄存器结构定义。地址偏移量:0xA3，初值:0x00，宽度:8
 寄存器说明: DATA_PORT方向配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved : 8;  /* bit[0-7]: 保留。 */
    } reg;
} MST_SOURCESINK_CFG2_UNION;
#endif


/*****************************************************************************
 结构名    : MST_DATAPORT_CFG1_UNION
 结构说明  : DATAPORT_CFG1 寄存器结构定义。地址偏移量:0xA4，初值:0xFF，宽度:8
 寄存器说明: DATA_PORT大小端配置寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_big_ending : 1;  /* bit[0]: DATA_PORT0的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp1_big_ending : 1;  /* bit[1]: DATA_PORT1的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp2_big_ending : 1;  /* bit[2]: DATA_PORT2的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp3_big_ending : 1;  /* bit[3]: DATA_PORT3的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp4_big_ending : 1;  /* bit[4]: DATA_PORT4的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp5_big_ending : 1;  /* bit[5]: DATA_PORT5的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp6_big_ending : 1;  /* bit[6]: DATA_PORT6的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
        unsigned char  dp7_big_ending : 1;  /* bit[7]: DATA_PORT7的数据传输配置:
                                                       1'b0:低位补0;
                                                       1'b1:高位补0; */
    } reg;
} MST_DATAPORT_CFG1_UNION;
#endif
#define MST_DATAPORT_CFG1_dp0_big_ending_START  (0)
#define MST_DATAPORT_CFG1_dp0_big_ending_END    (0)
#define MST_DATAPORT_CFG1_dp1_big_ending_START  (1)
#define MST_DATAPORT_CFG1_dp1_big_ending_END    (1)
#define MST_DATAPORT_CFG1_dp2_big_ending_START  (2)
#define MST_DATAPORT_CFG1_dp2_big_ending_END    (2)
#define MST_DATAPORT_CFG1_dp3_big_ending_START  (3)
#define MST_DATAPORT_CFG1_dp3_big_ending_END    (3)
#define MST_DATAPORT_CFG1_dp4_big_ending_START  (4)
#define MST_DATAPORT_CFG1_dp4_big_ending_END    (4)
#define MST_DATAPORT_CFG1_dp5_big_ending_START  (5)
#define MST_DATAPORT_CFG1_dp5_big_ending_END    (5)
#define MST_DATAPORT_CFG1_dp6_big_ending_START  (6)
#define MST_DATAPORT_CFG1_dp6_big_ending_END    (6)
#define MST_DATAPORT_CFG1_dp7_big_ending_START  (7)
#define MST_DATAPORT_CFG1_dp7_big_ending_END    (7)


/*****************************************************************************
 结构名    : MST_DATAPORT_CFG2_UNION
 结构说明  : DATAPORT_CFG2 寄存器结构定义。地址偏移量:0xA5，初值:0x7F，宽度:8
 寄存器说明: DATA_PORT大小端配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_big_ending  : 1;  /* bit[0]: DATA_PORT8的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  dp9_big_ending  : 1;  /* bit[1]: DATA_PORT9的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  dp10_big_ending : 1;  /* bit[2]: DATA_PORT10的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  dp11_big_ending : 1;  /* bit[3]: DATA_PORT11的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  dp12_big_ending : 1;  /* bit[4]: DATA_PORT12的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  dp13_big_ending : 1;  /* bit[5]: DATA_PORT13的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  dp14_big_ending : 1;  /* bit[6]: DATA_PORT14的数据传输配置:
                                                        1'b0:低位补0;
                                                        1'b1:高位补0; */
        unsigned char  reserved        : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_DATAPORT_CFG2_UNION;
#endif
#define MST_DATAPORT_CFG2_dp8_big_ending_START   (0)
#define MST_DATAPORT_CFG2_dp8_big_ending_END     (0)
#define MST_DATAPORT_CFG2_dp9_big_ending_START   (1)
#define MST_DATAPORT_CFG2_dp9_big_ending_END     (1)
#define MST_DATAPORT_CFG2_dp10_big_ending_START  (2)
#define MST_DATAPORT_CFG2_dp10_big_ending_END    (2)
#define MST_DATAPORT_CFG2_dp11_big_ending_START  (3)
#define MST_DATAPORT_CFG2_dp11_big_ending_END    (3)
#define MST_DATAPORT_CFG2_dp12_big_ending_START  (4)
#define MST_DATAPORT_CFG2_dp12_big_ending_END    (4)
#define MST_DATAPORT_CFG2_dp13_big_ending_START  (5)
#define MST_DATAPORT_CFG2_dp13_big_ending_END    (5)
#define MST_DATAPORT_CFG2_dp14_big_ending_START  (6)
#define MST_DATAPORT_CFG2_dp14_big_ending_END    (6)


/*****************************************************************************
 结构名    : MST_DATAPORT_FIFO_CFG1_UNION
 结构说明  : DATAPORT_FIFO_CFG1 寄存器结构定义。地址偏移量:0xA6，初值:0x00，宽度:8
 寄存器说明: DATA_PORT FIFO环回使能1(可配DP1~9)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_fifo_loopback_en : 1;  /* bit[0]: data_port0 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp1_fifo_loopback_en : 1;  /* bit[1]: data_port1 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp2_fifo_loopback_en : 1;  /* bit[2]: data_port2 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp3_fifo_loopback_en : 1;  /* bit[3]: data_port3 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp4_fifo_loopback_en : 1;  /* bit[4]: data_port4 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp5_fifo_loopback_en : 1;  /* bit[5]: data_port5 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp6_fifo_loopback_en : 1;  /* bit[6]: data_port6 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
        unsigned char  dp7_fifo_loopback_en : 1;  /* bit[7]: data_port7 fifo环回使能
                                                             1'b0:不使能;
                                                             1'b1:使能; */
    } reg;
} MST_DATAPORT_FIFO_CFG1_UNION;
#endif
#define MST_DATAPORT_FIFO_CFG1_dp0_fifo_loopback_en_START  (0)
#define MST_DATAPORT_FIFO_CFG1_dp0_fifo_loopback_en_END    (0)
#define MST_DATAPORT_FIFO_CFG1_dp1_fifo_loopback_en_START  (1)
#define MST_DATAPORT_FIFO_CFG1_dp1_fifo_loopback_en_END    (1)
#define MST_DATAPORT_FIFO_CFG1_dp2_fifo_loopback_en_START  (2)
#define MST_DATAPORT_FIFO_CFG1_dp2_fifo_loopback_en_END    (2)
#define MST_DATAPORT_FIFO_CFG1_dp3_fifo_loopback_en_START  (3)
#define MST_DATAPORT_FIFO_CFG1_dp3_fifo_loopback_en_END    (3)
#define MST_DATAPORT_FIFO_CFG1_dp4_fifo_loopback_en_START  (4)
#define MST_DATAPORT_FIFO_CFG1_dp4_fifo_loopback_en_END    (4)
#define MST_DATAPORT_FIFO_CFG1_dp5_fifo_loopback_en_START  (5)
#define MST_DATAPORT_FIFO_CFG1_dp5_fifo_loopback_en_END    (5)
#define MST_DATAPORT_FIFO_CFG1_dp6_fifo_loopback_en_START  (6)
#define MST_DATAPORT_FIFO_CFG1_dp6_fifo_loopback_en_END    (6)
#define MST_DATAPORT_FIFO_CFG1_dp7_fifo_loopback_en_START  (7)
#define MST_DATAPORT_FIFO_CFG1_dp7_fifo_loopback_en_END    (7)


/*****************************************************************************
 结构名    : MST_DATAPORT_FIFO_CFG2_UNION
 结构说明  : DATAPORT_FIFO_CFG2 寄存器结构定义。地址偏移量:0xA7，初值:0x00，宽度:8
 寄存器说明: DATA_PORT FIFO环回使能2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_fifo_loopback_en : 1;  /* bit[0]  : data_port8 fifo环回使能
                                                               1'b0:不使能;
                                                               1'b1:使能; */
        unsigned char  dp9_fifo_loopback_en : 1;  /* bit[1]  : data_port9 fifo环回使能
                                                               1'b0:不使能;
                                                               1'b1:使能; */
        unsigned char  reserved             : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_DATAPORT_FIFO_CFG2_UNION;
#endif
#define MST_DATAPORT_FIFO_CFG2_dp8_fifo_loopback_en_START  (0)
#define MST_DATAPORT_FIFO_CFG2_dp8_fifo_loopback_en_END    (0)
#define MST_DATAPORT_FIFO_CFG2_dp9_fifo_loopback_en_START  (1)
#define MST_DATAPORT_FIFO_CFG2_dp9_fifo_loopback_en_END    (1)


/*****************************************************************************
 结构名    : MST_DATAPORT_TEST_PROC_SEL_UNION
 结构说明  : DATAPORT_TEST_PROC_SEL 寄存器结构定义。地址偏移量:0xA8，初值:0x00，宽度:8
 寄存器说明: DATA_PORT 可维可测选择。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  data_port_test_proc_sel : 4;  /* bit[0-3]: data_port_test_proc_sel通路选择：
                                                                  4'd0:data_port0;
                                                                  ……
                                                                  4'd14:data_port14; */
        unsigned char  reserved                : 4;  /* bit[4-7]: 保留。 */
    } reg;
} MST_DATAPORT_TEST_PROC_SEL_UNION;
#endif
#define MST_DATAPORT_TEST_PROC_SEL_data_port_test_proc_sel_START  (0)
#define MST_DATAPORT_TEST_PROC_SEL_data_port_test_proc_sel_END    (3)


/*****************************************************************************
 结构名    : MST_SYNC_TIMEOUT_UNION
 结构说明  : SYNC_TIMEOUT 寄存器结构定义。地址偏移量:0xA9，初值:0x01，宽度:8
 寄存器说明: SOUNDWIRE同步超时等待时间配置。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  sync_timeout_frm_num : 8;  /* bit[0-7]: 同步超时等待帧长度时间配置：
                                                               (sync_timeout_frm_num+1)*16帧长度的时间 */
    } reg;
} MST_SYNC_TIMEOUT_UNION;
#endif
#define MST_SYNC_TIMEOUT_sync_timeout_frm_num_START  (0)
#define MST_SYNC_TIMEOUT_sync_timeout_frm_num_END    (7)


/*****************************************************************************
 结构名    : MST_ENUM_TIMEOUT_UNION
 结构说明  : ENUM_TIMEOUT 寄存器结构定义。地址偏移量:0xAA，初值:0xFF，宽度:8
 寄存器说明: SOUNDWIRE枚举超时等待时间配置。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  enum_timeout_frm_num : 8;  /* bit[0-7]: 枚举超时等待帧长度时间配置：
                                                               (sync_timeout_frm_num+1)*16帧长度的时间 */
    } reg;
} MST_ENUM_TIMEOUT_UNION;
#endif
#define MST_ENUM_TIMEOUT_enum_timeout_frm_num_START  (0)
#define MST_ENUM_TIMEOUT_enum_timeout_frm_num_END    (7)


/*****************************************************************************
 结构名    : MST_INTSTAT1_UNION
 结构说明  : INTSTAT1 寄存器结构定义。地址偏移量:0xC0，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved           : 1;  /* bit[0]: 保留。 */
        unsigned char  slv_unattached_int : 1;  /* bit[1]: slave设备unattched中断:
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
        unsigned char  slv_com_int        : 1;  /* bit[2]: slave设备内部异常中断:
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
        unsigned char  command_ignore_int : 1;  /* bit[3]: command_ignored中断:
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
        unsigned char  command_abort_int  : 1;  /* bit[4]: command_abort中断:
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
        unsigned char  msync_lost_int     : 1;  /* bit[5]: master侧检测失同步中断:
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
        unsigned char  mbus_clash_int_0   : 1;  /* bit[6]: master侧检测到bus_clash中断:(lane0)
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
        unsigned char  mbus_clash_int_1   : 1;  /* bit[7]: master侧检测到bus_clash中断:(lane1)
                                                           1：写1时清除中断，查询时表示中断有效；
                                                           0：查询时表示中断无效。 */
    } reg;
} MST_INTSTAT1_UNION;
#endif
#define MST_INTSTAT1_slv_unattached_int_START  (1)
#define MST_INTSTAT1_slv_unattached_int_END    (1)
#define MST_INTSTAT1_slv_com_int_START         (2)
#define MST_INTSTAT1_slv_com_int_END           (2)
#define MST_INTSTAT1_command_ignore_int_START  (3)
#define MST_INTSTAT1_command_ignore_int_END    (3)
#define MST_INTSTAT1_command_abort_int_START   (4)
#define MST_INTSTAT1_command_abort_int_END     (4)
#define MST_INTSTAT1_msync_lost_int_START      (5)
#define MST_INTSTAT1_msync_lost_int_END        (5)
#define MST_INTSTAT1_mbus_clash_int_0_START    (6)
#define MST_INTSTAT1_mbus_clash_int_0_END      (6)
#define MST_INTSTAT1_mbus_clash_int_1_START    (7)
#define MST_INTSTAT1_mbus_clash_int_1_END      (7)


/*****************************************************************************
 结构名    : MST_INTMASK1_UNION
 结构说明  : INTMASK1 寄存器结构定义。地址偏移量:0xC1，初值:0x1E，宽度:8
 寄存器说明: SOUNDWIRE中断状态屏蔽寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved                : 1;  /* bit[0]: 保留。 */
        unsigned char  slv_unattached_int_mask : 1;  /* bit[1]: slave设备unattched中断屏蔽位:
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
        unsigned char  slv_com_int_mask        : 1;  /* bit[2]: slave设备内部异常中断屏蔽位:
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
        unsigned char  command_ignore_int_mask : 1;  /* bit[3]: command_ignored中断屏蔽位:
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
        unsigned char  command_abort_int_mask  : 1;  /* bit[4]: command_abort中断屏蔽位:
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
        unsigned char  msync_lost_int_mask     : 1;  /* bit[5]: master侧检测失同步中断屏蔽位:
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
        unsigned char  mbus_clash_int_0_mask   : 1;  /* bit[6]: master侧检测到bus_clash中断屏蔽位:(lane0)
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
        unsigned char  mbus_clash_int_1_mask   : 1;  /* bit[7]: master侧检测到bus_clash中断屏蔽位:(lane1)
                                                                1'b0：中断屏蔽;
                                                                1'b1：中断不屏蔽; */
    } reg;
} MST_INTMASK1_UNION;
#endif
#define MST_INTMASK1_slv_unattached_int_mask_START  (1)
#define MST_INTMASK1_slv_unattached_int_mask_END    (1)
#define MST_INTMASK1_slv_com_int_mask_START         (2)
#define MST_INTMASK1_slv_com_int_mask_END           (2)
#define MST_INTMASK1_command_ignore_int_mask_START  (3)
#define MST_INTMASK1_command_ignore_int_mask_END    (3)
#define MST_INTMASK1_command_abort_int_mask_START   (4)
#define MST_INTMASK1_command_abort_int_mask_END     (4)
#define MST_INTMASK1_msync_lost_int_mask_START      (5)
#define MST_INTMASK1_msync_lost_int_mask_END        (5)
#define MST_INTMASK1_mbus_clash_int_0_mask_START    (6)
#define MST_INTMASK1_mbus_clash_int_0_mask_END      (6)
#define MST_INTMASK1_mbus_clash_int_1_mask_START    (7)
#define MST_INTMASK1_mbus_clash_int_1_mask_END      (7)


/*****************************************************************************
 结构名    : MST_INTMSTAT1_UNION
 结构说明  : INTMSTAT1 寄存器结构定义。地址偏移量:0xC2，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态屏蔽后状态寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved                 : 1;  /* bit[0]: 保留。 */
        unsigned char  slv_unattached_int_mstat : 1;  /* bit[1]: slave设备unattched屏蔽后的中断状态:
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
        unsigned char  slv_com_int_mstat        : 1;  /* bit[2]: slave设备内部异常屏蔽后的中断状态:
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
        unsigned char  command_ignore_int_mstat : 1;  /* bit[3]: command_ignored屏蔽后的中断状态:
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
        unsigned char  command_abort_int_mstat  : 1;  /* bit[4]: command_abort屏蔽后的中断状态:
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
        unsigned char  msync_lost_int_mstat     : 1;  /* bit[5]: master侧检测失同步屏蔽后的中断状态:
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
        unsigned char  mbus_clash_int_0_mstat   : 1;  /* bit[6]: master侧检测到bus_clash屏蔽后的中断状态:(lane0)
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
        unsigned char  mbus_clash_int_1_mstat   : 1;  /* bit[7]: master侧检测到bus_clash屏蔽后的中断状态:(lane1)
                                                                 1'b1：有中断;
                                                                 1'b0：无中断; */
    } reg;
} MST_INTMSTAT1_UNION;
#endif
#define MST_INTMSTAT1_slv_unattached_int_mstat_START  (1)
#define MST_INTMSTAT1_slv_unattached_int_mstat_END    (1)
#define MST_INTMSTAT1_slv_com_int_mstat_START         (2)
#define MST_INTMSTAT1_slv_com_int_mstat_END           (2)
#define MST_INTMSTAT1_command_ignore_int_mstat_START  (3)
#define MST_INTMSTAT1_command_ignore_int_mstat_END    (3)
#define MST_INTMSTAT1_command_abort_int_mstat_START   (4)
#define MST_INTMSTAT1_command_abort_int_mstat_END     (4)
#define MST_INTMSTAT1_msync_lost_int_mstat_START      (5)
#define MST_INTMSTAT1_msync_lost_int_mstat_END        (5)
#define MST_INTMSTAT1_mbus_clash_int_0_mstat_START    (6)
#define MST_INTMSTAT1_mbus_clash_int_0_mstat_END      (6)
#define MST_INTMSTAT1_mbus_clash_int_1_mstat_START    (7)
#define MST_INTMSTAT1_mbus_clash_int_1_mstat_END      (7)


/*****************************************************************************
 结构名    : MST_INTSTAT2_UNION
 结构说明  : INTSTAT2 寄存器结构定义。地址偏移量:0xC3，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  mpar_err_int              : 1;  /* bit[0]: master侧检测奇偶错误中断:
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  msync_timeout_int         : 1;  /* bit[1]: master侧同步超时中断:
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  reg_req_int               : 1;  /* bit[2]: frame_ctrl读写slave寄存器完成中断：
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  scp_frmend_finished_int   : 1;  /* bit[3]: frame_ctrl配置scp完成中断：
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  slv_attached_int          : 1;  /* bit[4]: slave同步完成完成中断：
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  clock_option_finished_int : 1;  /* bit[5]: clock唤醒完成(包括clockstop clkswitch)中断：
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  enum_timeout_int          : 1;  /* bit[6]: master侧枚举超时中断:
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  enum_finished_int         : 1;  /* bit[7]: 枚举完成中断:
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
    } reg;
} MST_INTSTAT2_UNION;
#endif
#define MST_INTSTAT2_mpar_err_int_START               (0)
#define MST_INTSTAT2_mpar_err_int_END                 (0)
#define MST_INTSTAT2_msync_timeout_int_START          (1)
#define MST_INTSTAT2_msync_timeout_int_END            (1)
#define MST_INTSTAT2_reg_req_int_START                (2)
#define MST_INTSTAT2_reg_req_int_END                  (2)
#define MST_INTSTAT2_scp_frmend_finished_int_START    (3)
#define MST_INTSTAT2_scp_frmend_finished_int_END      (3)
#define MST_INTSTAT2_slv_attached_int_START           (4)
#define MST_INTSTAT2_slv_attached_int_END             (4)
#define MST_INTSTAT2_clock_option_finished_int_START  (5)
#define MST_INTSTAT2_clock_option_finished_int_END    (5)
#define MST_INTSTAT2_enum_timeout_int_START           (6)
#define MST_INTSTAT2_enum_timeout_int_END             (6)
#define MST_INTSTAT2_enum_finished_int_START          (7)
#define MST_INTSTAT2_enum_finished_int_END            (7)


/*****************************************************************************
 结构名    : MST_INTMASK2_UNION
 结构说明  : INTMASK2 寄存器结构定义。地址偏移量:0xC4，初值:0xC4，宽度:8
 寄存器说明: SOUNDWIRE中断状态屏蔽寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  mpar_err_int_mask              : 1;  /* bit[0]: master侧检测奇偶错误中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  msync_timeout_int_mask         : 1;  /* bit[1]: master侧同步超时中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  reg_req_int_mask               : 1;  /* bit[2]: frame_ctrl读写slave寄存器完成中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  scp_frmend_finished_int_mask   : 1;  /* bit[3]: frame_ctrl配置scp完成中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  slv_attached_int_mask          : 1;  /* bit[4]: slave枚举完成完成中断屏蔽位：
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  clock_option_finished_int_mask : 1;  /* bit[5]: clock唤醒完成(包括clockstop clkswitch)中断屏蔽位：
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  enum_timeout_int_mask          : 1;  /* bit[6]: master侧枚举超时中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  enum_finished_int_mask         : 1;  /* bit[7]: 枚举完成中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
    } reg;
} MST_INTMASK2_UNION;
#endif
#define MST_INTMASK2_mpar_err_int_mask_START               (0)
#define MST_INTMASK2_mpar_err_int_mask_END                 (0)
#define MST_INTMASK2_msync_timeout_int_mask_START          (1)
#define MST_INTMASK2_msync_timeout_int_mask_END            (1)
#define MST_INTMASK2_reg_req_int_mask_START                (2)
#define MST_INTMASK2_reg_req_int_mask_END                  (2)
#define MST_INTMASK2_scp_frmend_finished_int_mask_START    (3)
#define MST_INTMASK2_scp_frmend_finished_int_mask_END      (3)
#define MST_INTMASK2_slv_attached_int_mask_START           (4)
#define MST_INTMASK2_slv_attached_int_mask_END             (4)
#define MST_INTMASK2_clock_option_finished_int_mask_START  (5)
#define MST_INTMASK2_clock_option_finished_int_mask_END    (5)
#define MST_INTMASK2_enum_timeout_int_mask_START           (6)
#define MST_INTMASK2_enum_timeout_int_mask_END             (6)
#define MST_INTMASK2_enum_finished_int_mask_START          (7)
#define MST_INTMASK2_enum_finished_int_mask_END            (7)


/*****************************************************************************
 结构名    : MST_INTMSTAT2_UNION
 结构说明  : INTMSTAT2 寄存器结构定义。地址偏移量:0xC5，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态屏蔽寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  mpar_err_int_mstat              : 1;  /* bit[0]: master侧检测奇偶错误屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  msync_timeout_int_mstat         : 1;  /* bit[1]: master侧同步超时屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  reg_req_int_mstat               : 1;  /* bit[2]: 'frame_ctrl读写slave寄存器完成屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  scp_frmend_finished_int_mstat   : 1;  /* bit[3]: frame_ctrl配置scp完成屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  slv_attached_int_mstat          : 1;  /* bit[4]: slave枚举完成完成屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  clock_option_finished_int_mstat : 1;  /* bit[5]: clock唤醒完成(包括clockstop clkswitch)屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  enum_timeout_int_mstat          : 1;  /* bit[6]: master侧枚举超时屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  enum_finished_int_mstat         : 1;  /* bit[7]: 枚举完成屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
    } reg;
} MST_INTMSTAT2_UNION;
#endif
#define MST_INTMSTAT2_mpar_err_int_mstat_START               (0)
#define MST_INTMSTAT2_mpar_err_int_mstat_END                 (0)
#define MST_INTMSTAT2_msync_timeout_int_mstat_START          (1)
#define MST_INTMSTAT2_msync_timeout_int_mstat_END            (1)
#define MST_INTMSTAT2_reg_req_int_mstat_START                (2)
#define MST_INTMSTAT2_reg_req_int_mstat_END                  (2)
#define MST_INTMSTAT2_scp_frmend_finished_int_mstat_START    (3)
#define MST_INTMSTAT2_scp_frmend_finished_int_mstat_END      (3)
#define MST_INTMSTAT2_slv_attached_int_mstat_START           (4)
#define MST_INTMSTAT2_slv_attached_int_mstat_END             (4)
#define MST_INTMSTAT2_clock_option_finished_int_mstat_START  (5)
#define MST_INTMSTAT2_clock_option_finished_int_mstat_END    (5)
#define MST_INTMSTAT2_enum_timeout_int_mstat_START           (6)
#define MST_INTMSTAT2_enum_timeout_int_mstat_END             (6)
#define MST_INTMSTAT2_enum_finished_int_mstat_START          (7)
#define MST_INTMSTAT2_enum_finished_int_mstat_END            (7)


/*****************************************************************************
 结构名    : MST_INTSTAT3_UNION
 结构说明  : INTSTAT3 寄存器结构定义。地址偏移量:0xC6，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  clockoptionnow_cfg_int  : 1;  /* bit[0]  : clockoption配置now完成指示中断:
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  clockoptiondone_cfg_int : 1;  /* bit[1]  : clockoption配置done完成指示中断:
                                                                  1：写1时清除中断，查询时表示中断有效；
                                                                  0：查询时表示中断无效。 */
        unsigned char  reserved                : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_INTSTAT3_UNION;
#endif
#define MST_INTSTAT3_clockoptionnow_cfg_int_START   (0)
#define MST_INTSTAT3_clockoptionnow_cfg_int_END     (0)
#define MST_INTSTAT3_clockoptiondone_cfg_int_START  (1)
#define MST_INTSTAT3_clockoptiondone_cfg_int_END    (1)


/*****************************************************************************
 结构名    : MST_INTMASK3_UNION
 结构说明  : INTMASK3 寄存器结构定义。地址偏移量:0xC7，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态屏蔽寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  clockoptionnow_cfg_int_mask  : 1;  /* bit[0]  : clockoption配置now中断屏蔽位:
                                                                       1'b0：中断屏蔽;
                                                                       1'b1：中断不屏蔽; */
        unsigned char  clockoptiondone_cfg_int_mask : 1;  /* bit[1]  : clockoption配置done奇偶错误中断:
                                                                       1：写1时清除中断，查询时表示中断有效；
                                                                       0：查询时表示中断无效。 */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_INTMASK3_UNION;
#endif
#define MST_INTMASK3_clockoptionnow_cfg_int_mask_START   (0)
#define MST_INTMASK3_clockoptionnow_cfg_int_mask_END     (0)
#define MST_INTMASK3_clockoptiondone_cfg_int_mask_START  (1)
#define MST_INTMASK3_clockoptiondone_cfg_int_mask_END    (1)


/*****************************************************************************
 结构名    : MST_INTMSTAT3_UNION
 结构说明  : INTMSTAT3 寄存器结构定义。地址偏移量:0xC8，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE中断状态寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  clockoptionnow_cfg_int_mstat  : 1;  /* bit[0]  : clockoption配置now屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  clockoptiondone_cfg_int_mstat : 1;  /* bit[1]  : clockoption配置done屏蔽后的中断状态:
                                                                        1'b1：有中断;
                                                                        1'b0：无中断; */
        unsigned char  reserved                      : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_INTMSTAT3_UNION;
#endif
#define MST_INTMSTAT3_clockoptionnow_cfg_int_mstat_START   (0)
#define MST_INTMSTAT3_clockoptionnow_cfg_int_mstat_END     (0)
#define MST_INTMSTAT3_clockoptiondone_cfg_int_mstat_START  (1)
#define MST_INTMSTAT3_clockoptiondone_cfg_int_mstat_END    (1)


/*****************************************************************************
 结构名    : MST_SLV_STAT0_UNION
 结构说明  : SLV_STAT0 寄存器结构定义。地址偏移量:0xC9，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE  Device 状态寄存器0
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  slv_stat_00 : 2;  /* bit[0-1]: slv_stat00的状态:
                                                      2'b00:无设备待枚举
                                                      2'b01:有设备待枚举 */
        unsigned char  slv_stat_01 : 2;  /* bit[2-3]: slv_stat01的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_02 : 2;  /* bit[4-5]: slv_stat02的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_03 : 2;  /* bit[6-7]: slv_stat03的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
    } reg;
} MST_SLV_STAT0_UNION;
#endif
#define MST_SLV_STAT0_slv_stat_00_START  (0)
#define MST_SLV_STAT0_slv_stat_00_END    (1)
#define MST_SLV_STAT0_slv_stat_01_START  (2)
#define MST_SLV_STAT0_slv_stat_01_END    (3)
#define MST_SLV_STAT0_slv_stat_02_START  (4)
#define MST_SLV_STAT0_slv_stat_02_END    (5)
#define MST_SLV_STAT0_slv_stat_03_START  (6)
#define MST_SLV_STAT0_slv_stat_03_END    (7)


/*****************************************************************************
 结构名    : MST_SLV_STAT1_UNION
 结构说明  : SLV_STAT1 寄存器结构定义。地址偏移量:0xCA，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE  Device 状态寄存器1
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  slv_stat_04 : 2;  /* bit[0-1]: slv_stat04的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_05 : 2;  /* bit[2-3]: slv_stat05的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_06 : 2;  /* bit[4-5]: slv_stat06的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_07 : 2;  /* bit[6-7]: slv_stat07的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
    } reg;
} MST_SLV_STAT1_UNION;
#endif
#define MST_SLV_STAT1_slv_stat_04_START  (0)
#define MST_SLV_STAT1_slv_stat_04_END    (1)
#define MST_SLV_STAT1_slv_stat_05_START  (2)
#define MST_SLV_STAT1_slv_stat_05_END    (3)
#define MST_SLV_STAT1_slv_stat_06_START  (4)
#define MST_SLV_STAT1_slv_stat_06_END    (5)
#define MST_SLV_STAT1_slv_stat_07_START  (6)
#define MST_SLV_STAT1_slv_stat_07_END    (7)


/*****************************************************************************
 结构名    : MST_SLV_STAT2_UNION
 结构说明  : SLV_STAT2 寄存器结构定义。地址偏移量:0xCB，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE  Device 状态寄存器2
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  slv_stat_08 : 2;  /* bit[0-1]: slv_stat08的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_09 : 2;  /* bit[2-3]: slv_stat09的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_10 : 2;  /* bit[4-5]: slv_stat10的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
        unsigned char  slv_stat_11 : 2;  /* bit[6-7]: slv_stat11的状态:
                                                      2'b00:掉线
                                                      2'b01:在线
                                                      2'b10:在线有中断告警 */
    } reg;
} MST_SLV_STAT2_UNION;
#endif
#define MST_SLV_STAT2_slv_stat_08_START  (0)
#define MST_SLV_STAT2_slv_stat_08_END    (1)
#define MST_SLV_STAT2_slv_stat_09_START  (2)
#define MST_SLV_STAT2_slv_stat_09_END    (3)
#define MST_SLV_STAT2_slv_stat_10_START  (4)
#define MST_SLV_STAT2_slv_stat_10_END    (5)
#define MST_SLV_STAT2_slv_stat_11_START  (6)
#define MST_SLV_STAT2_slv_stat_11_END    (7)


/*****************************************************************************
 结构名    : MST_DP_FIFO_CLR1_UNION
 结构说明  : DP_FIFO_CLR1 寄存器结构定义。地址偏移量:0xCC，初值:0x00，宽度:8
 寄存器说明: DP_FIFO清空寄存器1
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_fifo_clr : 1;  /* bit[0]: dp1_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp2_fifo_clr : 1;  /* bit[1]: dp2_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp3_fifo_clr : 1;  /* bit[2]: dp3_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp4_fifo_clr : 1;  /* bit[3]: dp4_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp5_fifo_clr : 1;  /* bit[4]: dp5_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp6_fifo_clr : 1;  /* bit[5]: dp6_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp7_fifo_clr : 1;  /* bit[6]: dp7_fifo清空寄存器，1有效;
                                                     当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  reserved     : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_DP_FIFO_CLR1_UNION;
#endif
#define MST_DP_FIFO_CLR1_dp1_fifo_clr_START  (0)
#define MST_DP_FIFO_CLR1_dp1_fifo_clr_END    (0)
#define MST_DP_FIFO_CLR1_dp2_fifo_clr_START  (1)
#define MST_DP_FIFO_CLR1_dp2_fifo_clr_END    (1)
#define MST_DP_FIFO_CLR1_dp3_fifo_clr_START  (2)
#define MST_DP_FIFO_CLR1_dp3_fifo_clr_END    (2)
#define MST_DP_FIFO_CLR1_dp4_fifo_clr_START  (3)
#define MST_DP_FIFO_CLR1_dp4_fifo_clr_END    (3)
#define MST_DP_FIFO_CLR1_dp5_fifo_clr_START  (4)
#define MST_DP_FIFO_CLR1_dp5_fifo_clr_END    (4)
#define MST_DP_FIFO_CLR1_dp6_fifo_clr_START  (5)
#define MST_DP_FIFO_CLR1_dp6_fifo_clr_END    (5)
#define MST_DP_FIFO_CLR1_dp7_fifo_clr_START  (6)
#define MST_DP_FIFO_CLR1_dp7_fifo_clr_END    (6)


/*****************************************************************************
 结构名    : MST_DP_FIFO_CLR2_UNION
 结构说明  : DP_FIFO_CLR2 寄存器结构定义。地址偏移量:0xCD，初值:0x00，宽度:8
 寄存器说明: DP_FIFO清空寄存器1
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_fifo_clr  : 1;  /* bit[0]: dp8_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp9_fifo_clr  : 1;  /* bit[1]: dp9_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp10_fifo_clr : 1;  /* bit[2]: dp10_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp11_fifo_clr : 1;  /* bit[3]: dp11_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp12_fifo_clr : 1;  /* bit[4]: dp12_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp13_fifo_clr : 1;  /* bit[5]: dp13_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  dp14_fifo_clr : 1;  /* bit[6]: dp14_fifo清空寄存器，1有效;
                                                      当dp7_cfg_error0_int有效时，需要配置该寄存器为1，再配置为0 */
        unsigned char  reserved      : 1;  /* bit[7]: 保留。 */
    } reg;
} MST_DP_FIFO_CLR2_UNION;
#endif
#define MST_DP_FIFO_CLR2_dp8_fifo_clr_START   (0)
#define MST_DP_FIFO_CLR2_dp8_fifo_clr_END     (0)
#define MST_DP_FIFO_CLR2_dp9_fifo_clr_START   (1)
#define MST_DP_FIFO_CLR2_dp9_fifo_clr_END     (1)
#define MST_DP_FIFO_CLR2_dp10_fifo_clr_START  (2)
#define MST_DP_FIFO_CLR2_dp10_fifo_clr_END    (2)
#define MST_DP_FIFO_CLR2_dp11_fifo_clr_START  (3)
#define MST_DP_FIFO_CLR2_dp11_fifo_clr_END    (3)
#define MST_DP_FIFO_CLR2_dp12_fifo_clr_START  (4)
#define MST_DP_FIFO_CLR2_dp12_fifo_clr_END    (4)
#define MST_DP_FIFO_CLR2_dp13_fifo_clr_START  (5)
#define MST_DP_FIFO_CLR2_dp13_fifo_clr_END    (5)
#define MST_DP_FIFO_CLR2_dp14_fifo_clr_START  (6)
#define MST_DP_FIFO_CLR2_dp14_fifo_clr_END    (6)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_0_UNION
 结构说明  : DEBUG_OUT_0 寄存器结构定义。地址偏移量:0xDF，初值:0x00，宽度:8
 寄存器说明: 可维可测输出观测只读寄存器0。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_io_proc_out : 7;  /* bit[0-6]: 可维可测io输出观测只读寄存器[6:0] */
        unsigned char  reserved         : 1;  /* bit[7]  : 保留。 */
    } reg;
} MST_DEBUG_OUT_0_UNION;
#endif
#define MST_DEBUG_OUT_0_test_io_proc_out_START  (0)
#define MST_DEBUG_OUT_0_test_io_proc_out_END    (6)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_1_UNION
 结构说明  : DEBUG_OUT_1 寄存器结构定义。地址偏移量:0xE0，初值:0x00，宽度:8
 寄存器说明: 可维可测输出观测只读寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_reg_proc_out_1 : 8;  /* bit[0-7]: 可维可测reg输出观测只读寄存器[31:24] */
    } reg;
} MST_DEBUG_OUT_1_UNION;
#endif
#define MST_DEBUG_OUT_1_test_reg_proc_out_1_START  (0)
#define MST_DEBUG_OUT_1_test_reg_proc_out_1_END    (7)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_2_UNION
 结构说明  : DEBUG_OUT_2 寄存器结构定义。地址偏移量:0xE1，初值:0x00，宽度:8
 寄存器说明: 可维可测输出观测只读寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_reg_proc_out_2 : 8;  /* bit[0-7]: 可维可测reg输出观测只读寄存器[23:16] */
    } reg;
} MST_DEBUG_OUT_2_UNION;
#endif
#define MST_DEBUG_OUT_2_test_reg_proc_out_2_START  (0)
#define MST_DEBUG_OUT_2_test_reg_proc_out_2_END    (7)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_3_UNION
 结构说明  : DEBUG_OUT_3 寄存器结构定义。地址偏移量:0xE2，初值:0x00，宽度:8
 寄存器说明: 可维可测输出观测只读寄存器3。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_reg_proc_out_3 : 8;  /* bit[0-7]: 可维可测reg输出观测只读寄存器[15:8] */
    } reg;
} MST_DEBUG_OUT_3_UNION;
#endif
#define MST_DEBUG_OUT_3_test_reg_proc_out_3_START  (0)
#define MST_DEBUG_OUT_3_test_reg_proc_out_3_END    (7)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_4_UNION
 结构说明  : DEBUG_OUT_4 寄存器结构定义。地址偏移量:0xE3，初值:0x00，宽度:8
 寄存器说明: 可维可测输出观测只读寄存器4。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_reg_proc_out_4 : 8;  /* bit[0-7]: 可维可测reg输出观测只读寄存器[7:0] */
    } reg;
} MST_DEBUG_OUT_4_UNION;
#endif
#define MST_DEBUG_OUT_4_test_reg_proc_out_4_START  (0)
#define MST_DEBUG_OUT_4_test_reg_proc_out_4_END    (7)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_SEL0_UNION
 结构说明  : DEBUG_OUT_SEL0 寄存器结构定义。地址偏移量:0xE4，初值:0x00，宽度:8
 寄存器说明: 可维可测锁存输出选择配置寄存器0。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_proc_vld_sel : 2;  /* bit[0-1]: vld锁存信号选择：
                                                            2'b00:常开;
                                                            2'b01:帧头锁存;
                                                            2'b10:帧尾锁存;
                                                            2'b11:SSP点锁存; */
        unsigned char  reserved          : 6;  /* bit[2-7]: 保留。 */
    } reg;
} MST_DEBUG_OUT_SEL0_UNION;
#endif
#define MST_DEBUG_OUT_SEL0_test_proc_vld_sel_START  (0)
#define MST_DEBUG_OUT_SEL0_test_proc_vld_sel_END    (1)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_SEL1_UNION
 结构说明  : DEBUG_OUT_SEL1 寄存器结构定义。地址偏移量:0xE5，初值:0x00，宽度:8
 寄存器说明: 可维可测输出选择配置寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_io_proc_sel : 8;  /* bit[0-7]: 可维可测选择io输出配置寄存器 */
    } reg;
} MST_DEBUG_OUT_SEL1_UNION;
#endif
#define MST_DEBUG_OUT_SEL1_test_io_proc_sel_START  (0)
#define MST_DEBUG_OUT_SEL1_test_io_proc_sel_END    (7)


/*****************************************************************************
 结构名    : MST_DEBUG_OUT_SEL2_UNION
 结构说明  : DEBUG_OUT_SEL2 寄存器结构定义。地址偏移量:0xE6，初值:0x00，宽度:8
 寄存器说明: 可维可测输出选择配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  test_reg_proc_sel : 8;  /* bit[0-7]: 可维可测选择reg输出配置寄存器 */
    } reg;
} MST_DEBUG_OUT_SEL2_UNION;
#endif
#define MST_DEBUG_OUT_SEL2_test_reg_proc_sel_START  (0)
#define MST_DEBUG_OUT_SEL2_test_reg_proc_sel_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_FIFO_THRE_CTRL_UNION
 结构说明  : DP1_FIFO_THRE_CTRL 寄存器结构定义。地址偏移量:0xE7，初值:0x20，宽度:8
 寄存器说明: DP_FIFO水线配置寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp_fifo_thre_up : 6;  /* bit[0-5]: dp fifo上搬水线配置 配置范围：1~63（dp1~9） */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留。 */
    } reg;
} MST_DP1_FIFO_THRE_CTRL_UNION;
#endif
#define MST_DP1_FIFO_THRE_CTRL_dp_fifo_thre_up_START  (0)
#define MST_DP1_FIFO_THRE_CTRL_dp_fifo_thre_up_END    (5)


/*****************************************************************************
 结构名    : MST_DP_FIFO_THRE_CTRL1_UNION
 结构说明  : DP_FIFO_THRE_CTRL1 寄存器结构定义。地址偏移量:0xE8，初值:0x20，宽度:8
 寄存器说明: DP_FIFO水线配置寄存器1。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp_fifo_thre_dn : 6;  /* bit[0-5]: dp fifo下搬水线配置 配置范围：1~63（dp11~14） */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留 */
    } reg;
} MST_DP_FIFO_THRE_CTRL1_UNION;
#endif
#define MST_DP_FIFO_THRE_CTRL1_dp_fifo_thre_dn_START  (0)
#define MST_DP_FIFO_THRE_CTRL1_dp_fifo_thre_dn_END    (5)


/*****************************************************************************
 结构名    : MST_DP_FIFO_THRE_CTRL2_UNION
 结构说明  : DP_FIFO_THRE_CTRL2 寄存器结构定义。地址偏移量:0xE9，初值:0x40，宽度:8
 寄存器说明: DP_FIFO水线配置寄存器2。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_fifo_thre_dn : 7;  /* bit[0-6]: dp10 fifo下搬水线配置 配置范围：1~128（dp10） */
        unsigned char  reserved          : 1;  /* bit[7]  : 保留 */
    } reg;
} MST_DP_FIFO_THRE_CTRL2_UNION;
#endif
#define MST_DP_FIFO_THRE_CTRL2_dp10_fifo_thre_dn_START  (0)
#define MST_DP_FIFO_THRE_CTRL2_dp10_fifo_thre_dn_END    (6)


/*****************************************************************************
 结构名    : MST_DP0_FIFO_STAT_UNION
 结构说明  : DP0_FIFO_STAT 寄存器结构定义。地址偏移量:0xF5，初值:0x0A，宽度:8
 寄存器说明: dp0_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp0_fifo_up_full  : 1;  /* bit[0]  : dp0_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp0_fifo_up_empty : 1;  /* bit[1]  : dp0_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  dp0_fifo_dn_full  : 1;  /* bit[2]  : dp0_fifo_dn满指示:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp0_fifo_dn_empty : 1;  /* bit[3]  : dp0_fifo_dn空指示:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved          : 4;  /* bit[4-7]: 保留。 */
    } reg;
} MST_DP0_FIFO_STAT_UNION;
#endif
#define MST_DP0_FIFO_STAT_dp0_fifo_up_full_START   (0)
#define MST_DP0_FIFO_STAT_dp0_fifo_up_full_END     (0)
#define MST_DP0_FIFO_STAT_dp0_fifo_up_empty_START  (1)
#define MST_DP0_FIFO_STAT_dp0_fifo_up_empty_END    (1)
#define MST_DP0_FIFO_STAT_dp0_fifo_dn_full_START   (2)
#define MST_DP0_FIFO_STAT_dp0_fifo_dn_full_END     (2)
#define MST_DP0_FIFO_STAT_dp0_fifo_dn_empty_START  (3)
#define MST_DP0_FIFO_STAT_dp0_fifo_dn_empty_END    (3)


/*****************************************************************************
 结构名    : MST_DP12_FIFO_INTSTAT_UNION
 结构说明  : DP12_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xF6，初值:0x22，宽度:8
 寄存器说明: dp12_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_fifo_up_full  : 1;  /* bit[0]  : dp1_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp1_fifo_up_empty : 1;  /* bit[1]  : dp1_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_0        : 2;  /* bit[2-3]: 保留。 */
        unsigned char  dp2_fifo_up_full  : 1;  /* bit[4]  : dp1_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp2_fifo_up_empty : 1;  /* bit[5]  : dp1_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_1        : 2;  /* bit[6-7]: 保留。 */
    } reg;
} MST_DP12_FIFO_INTSTAT_UNION;
#endif
#define MST_DP12_FIFO_INTSTAT_dp1_fifo_up_full_START   (0)
#define MST_DP12_FIFO_INTSTAT_dp1_fifo_up_full_END     (0)
#define MST_DP12_FIFO_INTSTAT_dp1_fifo_up_empty_START  (1)
#define MST_DP12_FIFO_INTSTAT_dp1_fifo_up_empty_END    (1)
#define MST_DP12_FIFO_INTSTAT_dp2_fifo_up_full_START   (4)
#define MST_DP12_FIFO_INTSTAT_dp2_fifo_up_full_END     (4)
#define MST_DP12_FIFO_INTSTAT_dp2_fifo_up_empty_START  (5)
#define MST_DP12_FIFO_INTSTAT_dp2_fifo_up_empty_END    (5)


/*****************************************************************************
 结构名    : MST_DP34_FIFO_INTSTAT_UNION
 结构说明  : DP34_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xF7，初值:0x22，宽度:8
 寄存器说明: dp34_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_fifo_up_full  : 1;  /* bit[0]  : dp3_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp3_fifo_up_empty : 1;  /* bit[1]  : dp3_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_0        : 2;  /* bit[2-3]: 保留。 */
        unsigned char  dp4_fifo_up_full  : 1;  /* bit[4]  : dp4_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp4_fifo_up_empty : 1;  /* bit[5]  : dp4_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_1        : 2;  /* bit[6-7]: 保留。 */
    } reg;
} MST_DP34_FIFO_INTSTAT_UNION;
#endif
#define MST_DP34_FIFO_INTSTAT_dp3_fifo_up_full_START   (0)
#define MST_DP34_FIFO_INTSTAT_dp3_fifo_up_full_END     (0)
#define MST_DP34_FIFO_INTSTAT_dp3_fifo_up_empty_START  (1)
#define MST_DP34_FIFO_INTSTAT_dp3_fifo_up_empty_END    (1)
#define MST_DP34_FIFO_INTSTAT_dp4_fifo_up_full_START   (4)
#define MST_DP34_FIFO_INTSTAT_dp4_fifo_up_full_END     (4)
#define MST_DP34_FIFO_INTSTAT_dp4_fifo_up_empty_START  (5)
#define MST_DP34_FIFO_INTSTAT_dp4_fifo_up_empty_END    (5)


/*****************************************************************************
 结构名    : MST_DP56_FIFO_INTSTAT_UNION
 结构说明  : DP56_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xF8，初值:0x22，宽度:8
 寄存器说明: dp56_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_fifo_up_full  : 1;  /* bit[0]  : dp5_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp5_fifo_up_empty : 1;  /* bit[1]  : dp5_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_0        : 2;  /* bit[2-3]: 保留。 */
        unsigned char  dp6_fifo_up_full  : 1;  /* bit[4]  : dp6_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp6_fifo_up_empty : 1;  /* bit[5]  : dp6_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_1        : 2;  /* bit[6-7]: 保留。 */
    } reg;
} MST_DP56_FIFO_INTSTAT_UNION;
#endif
#define MST_DP56_FIFO_INTSTAT_dp5_fifo_up_full_START   (0)
#define MST_DP56_FIFO_INTSTAT_dp5_fifo_up_full_END     (0)
#define MST_DP56_FIFO_INTSTAT_dp5_fifo_up_empty_START  (1)
#define MST_DP56_FIFO_INTSTAT_dp5_fifo_up_empty_END    (1)
#define MST_DP56_FIFO_INTSTAT_dp6_fifo_up_full_START   (4)
#define MST_DP56_FIFO_INTSTAT_dp6_fifo_up_full_END     (4)
#define MST_DP56_FIFO_INTSTAT_dp6_fifo_up_empty_START  (5)
#define MST_DP56_FIFO_INTSTAT_dp6_fifo_up_empty_END    (5)


/*****************************************************************************
 结构名    : MST_DP78_FIFO_INTSTAT_UNION
 结构说明  : DP78_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xF9，初值:0x22，宽度:8
 寄存器说明: dp78_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_fifo_up_full  : 1;  /* bit[0]  : dp7_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp7_fifo_up_empty : 1;  /* bit[1]  : dp7_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_0        : 2;  /* bit[2-3]: 保留。 */
        unsigned char  dp8_fifo_up_full  : 1;  /* bit[4]  : dp8_fifo_up满中断:
                                                            1：满；
                                                            0：非满。 */
        unsigned char  dp8_fifo_up_empty : 1;  /* bit[5]  : dp8_fifo_up空中断:
                                                            1：空；
                                                            0：非空。 */
        unsigned char  reserved_1        : 2;  /* bit[6-7]: 保留。 */
    } reg;
} MST_DP78_FIFO_INTSTAT_UNION;
#endif
#define MST_DP78_FIFO_INTSTAT_dp7_fifo_up_full_START   (0)
#define MST_DP78_FIFO_INTSTAT_dp7_fifo_up_full_END     (0)
#define MST_DP78_FIFO_INTSTAT_dp7_fifo_up_empty_START  (1)
#define MST_DP78_FIFO_INTSTAT_dp7_fifo_up_empty_END    (1)
#define MST_DP78_FIFO_INTSTAT_dp8_fifo_up_full_START   (4)
#define MST_DP78_FIFO_INTSTAT_dp8_fifo_up_full_END     (4)
#define MST_DP78_FIFO_INTSTAT_dp8_fifo_up_empty_START  (5)
#define MST_DP78_FIFO_INTSTAT_dp8_fifo_up_empty_END    (5)


/*****************************************************************************
 结构名    : MST_DP910_FIFO_INTSTAT_UNION
 结构说明  : DP910_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xFA，初值:0x82，宽度:8
 寄存器说明: dp910_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_fifo_up_full   : 1;  /* bit[0]  : dp9_fifo_up满中断:
                                                             1：满；
                                                             0：非满。 */
        unsigned char  dp9_fifo_up_empty  : 1;  /* bit[1]  : dp9_fifo_up空中断:
                                                             1：空；
                                                             0：非空。 */
        unsigned char  reserved           : 4;  /* bit[2-5]: 保留。 */
        unsigned char  dp10_fifo_dn_full  : 1;  /* bit[6]  : dp10_fifo_dn满指示:
                                                             1：满；
                                                             0：非满。 */
        unsigned char  dp10_fifo_dn_empty : 1;  /* bit[7]  : dp10_fifo_dn空指示:
                                                             1：空；
                                                             0：非空。 */
    } reg;
} MST_DP910_FIFO_INTSTAT_UNION;
#endif
#define MST_DP910_FIFO_INTSTAT_dp9_fifo_up_full_START    (0)
#define MST_DP910_FIFO_INTSTAT_dp9_fifo_up_full_END      (0)
#define MST_DP910_FIFO_INTSTAT_dp9_fifo_up_empty_START   (1)
#define MST_DP910_FIFO_INTSTAT_dp9_fifo_up_empty_END     (1)
#define MST_DP910_FIFO_INTSTAT_dp10_fifo_dn_full_START   (6)
#define MST_DP910_FIFO_INTSTAT_dp10_fifo_dn_full_END     (6)
#define MST_DP910_FIFO_INTSTAT_dp10_fifo_dn_empty_START  (7)
#define MST_DP910_FIFO_INTSTAT_dp10_fifo_dn_empty_END    (7)


/*****************************************************************************
 结构名    : MST_DP1112_FIFO_INTSTAT_UNION
 结构说明  : DP1112_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xFB，初值:0x88，宽度:8
 寄存器说明: dp1112_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0         : 2;  /* bit[0-1]: 保留。 */
        unsigned char  dp11_fifo_dn_full  : 1;  /* bit[2]  : dp11_fifo_dn满指示:
                                                             1：满；
                                                             0：非满。 */
        unsigned char  dp11_fifo_dn_empty : 1;  /* bit[3]  : dp11_fifo_dn空指示:
                                                             1：空；
                                                             0：非空。 */
        unsigned char  reserved_1         : 2;  /* bit[4-5]: 保留。 */
        unsigned char  dp12_fifo_dn_full  : 1;  /* bit[6]  : dp12_fifo_dn满指示:
                                                             1：满；
                                                             0：非满。 */
        unsigned char  dp12_fifo_dn_empty : 1;  /* bit[7]  : dp12_fifo_dn空指示:
                                                             1：空；
                                                             0：非空。 */
    } reg;
} MST_DP1112_FIFO_INTSTAT_UNION;
#endif
#define MST_DP1112_FIFO_INTSTAT_dp11_fifo_dn_full_START   (2)
#define MST_DP1112_FIFO_INTSTAT_dp11_fifo_dn_full_END     (2)
#define MST_DP1112_FIFO_INTSTAT_dp11_fifo_dn_empty_START  (3)
#define MST_DP1112_FIFO_INTSTAT_dp11_fifo_dn_empty_END    (3)
#define MST_DP1112_FIFO_INTSTAT_dp12_fifo_dn_full_START   (6)
#define MST_DP1112_FIFO_INTSTAT_dp12_fifo_dn_full_END     (6)
#define MST_DP1112_FIFO_INTSTAT_dp12_fifo_dn_empty_START  (7)
#define MST_DP1112_FIFO_INTSTAT_dp12_fifo_dn_empty_END    (7)


/*****************************************************************************
 结构名    : MST_DP1314_FIFO_INTSTAT_UNION
 结构说明  : DP1314_FIFO_INTSTAT 寄存器结构定义。地址偏移量:0xFC，初值:0x88，宽度:8
 寄存器说明: dp1314_fifo空满状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  reserved_0         : 2;  /* bit[0-1]: 保留。 */
        unsigned char  dp13_fifo_dn_full  : 1;  /* bit[2]  : dp13_fifo_dn满指示:
                                                             1：满；
                                                             0：非满。 */
        unsigned char  dp13_fifo_dn_empty : 1;  /* bit[3]  : dp13_fifo_dn空指示:
                                                             1：空；
                                                             0：非空。 */
        unsigned char  reserved_1         : 2;  /* bit[4-5]: 保留。 */
        unsigned char  dp14_fifo_dn_full  : 1;  /* bit[6]  : dp14_fifo_dn满指示:
                                                             1：满；
                                                             0：非满。 */
        unsigned char  dp14_fifo_dn_empty : 1;  /* bit[7]  : dp14_fifo_dn空指示:
                                                             1：空；
                                                             0：非空。 */
    } reg;
} MST_DP1314_FIFO_INTSTAT_UNION;
#endif
#define MST_DP1314_FIFO_INTSTAT_dp13_fifo_dn_full_START   (2)
#define MST_DP1314_FIFO_INTSTAT_dp13_fifo_dn_full_END     (2)
#define MST_DP1314_FIFO_INTSTAT_dp13_fifo_dn_empty_START  (3)
#define MST_DP1314_FIFO_INTSTAT_dp13_fifo_dn_empty_END    (3)
#define MST_DP1314_FIFO_INTSTAT_dp14_fifo_dn_full_START   (6)
#define MST_DP1314_FIFO_INTSTAT_dp14_fifo_dn_full_END     (6)
#define MST_DP1314_FIFO_INTSTAT_dp14_fifo_dn_empty_START  (7)
#define MST_DP1314_FIFO_INTSTAT_dp14_fifo_dn_empty_END    (7)


/*****************************************************************************
 结构名    : MST_DBG_UNION
 结构说明  : DBG 寄存器结构定义。地址偏移量:0x0FE，初值:0x00，宽度:8
 寄存器说明: SOUNDWIRE debug配置寄存器
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dbg_clr  : 1;  /* bit[0]  : dbg信号清零使能：
                                                   1'b1:清零；
                                                   1'b0:不清零；
                                                   使用时需要先配1再配0； */
        unsigned char  reserved : 7;  /* bit[1-7]: 保留。 */
    } reg;
} MST_DBG_UNION;
#endif
#define MST_DBG_dbg_clr_START   (0)
#define MST_DBG_dbg_clr_END     (0)


/*****************************************************************************
 结构名    : MST_VERSION_UNION
 结构说明  : VERSION 寄存器结构定义。地址偏移量:0x0FF，初值:0x20，宽度:8
 寄存器说明: 版本寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  version : 8;  /* bit[0-7]: SOUNDWIRE版本寄存器 */
    } reg;
} MST_VERSION_UNION;
#endif
#define MST_VERSION_version_START  (0)
#define MST_VERSION_version_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_INTSTAT_UNION
 结构说明  : DP1_INTSTAT 寄存器结构定义。地址偏移量:0x100，初值:0x00，宽度:8
 寄存器说明: dp1中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_test_fail_int  : 1;  /* bit[0]  : dp1 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp1_port_ready_int : 1;  /* bit[1]  : dp1 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp1_cfg_error0_int : 1;  /* bit[5]  : dp1自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp1_cfg_error1_int : 1;  /* bit[6]  : dp1自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp1_cfg_error2_int : 1;  /* bit[7]  : dp1自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP1_INTSTAT_UNION;
#endif
#define MST_DP1_INTSTAT_dp1_test_fail_int_START   (0)
#define MST_DP1_INTSTAT_dp1_test_fail_int_END     (0)
#define MST_DP1_INTSTAT_dp1_port_ready_int_START  (1)
#define MST_DP1_INTSTAT_dp1_port_ready_int_END    (1)
#define MST_DP1_INTSTAT_dp1_cfg_error0_int_START  (5)
#define MST_DP1_INTSTAT_dp1_cfg_error0_int_END    (5)
#define MST_DP1_INTSTAT_dp1_cfg_error1_int_START  (6)
#define MST_DP1_INTSTAT_dp1_cfg_error1_int_END    (6)
#define MST_DP1_INTSTAT_dp1_cfg_error2_int_START  (7)
#define MST_DP1_INTSTAT_dp1_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_INTMASK_UNION
 结构说明  : DP1_INTMASK 寄存器结构定义。地址偏移量:0x101，初值:0x00，宽度:8
 寄存器说明: dp1中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_test_fail_int_mask  : 1;  /* bit[0]  : dp1 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp1_port_ready_int_mask : 1;  /* bit[1]  : dp1 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp1_cfg_error0_int_mask : 1;  /* bit[5]  : dp1自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp1_cfg_error1_int_mask : 1;  /* bit[6]  : dp1自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp1_cfg_error2_int_mask : 1;  /* bit[7]  : dp1自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP1_INTMASK_UNION;
#endif
#define MST_DP1_INTMASK_dp1_test_fail_int_mask_START   (0)
#define MST_DP1_INTMASK_dp1_test_fail_int_mask_END     (0)
#define MST_DP1_INTMASK_dp1_port_ready_int_mask_START  (1)
#define MST_DP1_INTMASK_dp1_port_ready_int_mask_END    (1)
#define MST_DP1_INTMASK_dp1_cfg_error0_int_mask_START  (5)
#define MST_DP1_INTMASK_dp1_cfg_error0_int_mask_END    (5)
#define MST_DP1_INTMASK_dp1_cfg_error1_int_mask_START  (6)
#define MST_DP1_INTMASK_dp1_cfg_error1_int_mask_END    (6)
#define MST_DP1_INTMASK_dp1_cfg_error2_int_mask_START  (7)
#define MST_DP1_INTMASK_dp1_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_PORTCTRL_UNION
 结构说明  : DP1_PORTCTRL 寄存器结构定义。地址偏移量:0x102，初值:0x00，宽度:8
 寄存器说明: dp1的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_portflowmode   : 2;  /* bit[0-1]: dp1流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp1_portdatamode   : 2;  /* bit[2-3]: dp1 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp1_nextinvertbank : 1;  /* bit[4]  : dp1 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP1_PORTCTRL_UNION;
#endif
#define MST_DP1_PORTCTRL_dp1_portflowmode_START    (0)
#define MST_DP1_PORTCTRL_dp1_portflowmode_END      (1)
#define MST_DP1_PORTCTRL_dp1_portdatamode_START    (2)
#define MST_DP1_PORTCTRL_dp1_portdatamode_END      (3)
#define MST_DP1_PORTCTRL_dp1_nextinvertbank_START  (4)
#define MST_DP1_PORTCTRL_dp1_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP1_BLOCKCTRL1_UNION
 结构说明  : DP1_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x103，初值:0x1F，宽度:8
 寄存器说明: dp1 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_wordlength : 6;  /* bit[0-5]: dp1 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP1_BLOCKCTRL1_UNION;
#endif
#define MST_DP1_BLOCKCTRL1_dp1_wordlength_START  (0)
#define MST_DP1_BLOCKCTRL1_dp1_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP1_PREPARESTATUS_UNION
 结构说明  : DP1_PREPARESTATUS 寄存器结构定义。地址偏移量:0x104，初值:0x00，宽度:8
 寄存器说明: dp1的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_n_finished_channel1 : 1;  /* bit[0]: dp1通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel2 : 1;  /* bit[1]: dp1通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel3 : 1;  /* bit[2]: dp1通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel4 : 1;  /* bit[3]: dp1通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel5 : 1;  /* bit[4]: dp1通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel6 : 1;  /* bit[5]: dp1通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel7 : 1;  /* bit[6]: dp1通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp1_n_finished_channel8 : 1;  /* bit[7]: dp1通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP1_PREPARESTATUS_UNION;
#endif
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel1_START  (0)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel1_END    (0)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel2_START  (1)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel2_END    (1)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel3_START  (2)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel3_END    (2)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel4_START  (3)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel4_END    (3)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel5_START  (4)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel5_END    (4)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel6_START  (5)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel6_END    (5)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel7_START  (6)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel7_END    (6)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel8_START  (7)
#define MST_DP1_PREPARESTATUS_dp1_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_PREPARECTRL_UNION
 结构说明  : DP1_PREPARECTRL 寄存器结构定义。地址偏移量:0x105，初值:0x00，宽度:8
 寄存器说明: dp1的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_prepare_channel1 : 1;  /* bit[0]: dp1使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel2 : 1;  /* bit[1]: dp1使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel3 : 1;  /* bit[2]: dp1使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel4 : 1;  /* bit[3]: dp1使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel5 : 1;  /* bit[4]: dp1使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel6 : 1;  /* bit[5]: dp1使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel7 : 1;  /* bit[6]: dp1使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp1_prepare_channel8 : 1;  /* bit[7]: dp1使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP1_PREPARECTRL_UNION;
#endif
#define MST_DP1_PREPARECTRL_dp1_prepare_channel1_START  (0)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel1_END    (0)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel2_START  (1)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel2_END    (1)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel3_START  (2)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel3_END    (2)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel4_START  (3)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel4_END    (3)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel5_START  (4)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel5_END    (4)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel6_START  (5)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel6_END    (5)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel7_START  (6)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel7_END    (6)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel8_START  (7)
#define MST_DP1_PREPARECTRL_dp1_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_CHANNELEN_BANK0_UNION
 结构说明  : DP1_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x120，初值:0x00，宽度:8
 寄存器说明: dp1的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_enable_channel1_bank0 : 1;  /* bit[0]: dp1的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel2_bank0 : 1;  /* bit[1]: dp1的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel3_bank0 : 1;  /* bit[2]: dp1的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel4_bank0 : 1;  /* bit[3]: dp1的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel5_bank0 : 1;  /* bit[4]: dp1的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel6_bank0 : 1;  /* bit[5]: dp1的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel7_bank0 : 1;  /* bit[6]: dp1的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel8_bank0 : 1;  /* bit[7]: dp1的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP1_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel1_bank0_START  (0)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel1_bank0_END    (0)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel2_bank0_START  (1)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel2_bank0_END    (1)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel3_bank0_START  (2)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel3_bank0_END    (2)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel4_bank0_START  (3)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel4_bank0_END    (3)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel5_bank0_START  (4)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel5_bank0_END    (4)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel6_bank0_START  (5)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel6_bank0_END    (5)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel7_bank0_START  (6)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel7_bank0_END    (6)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel8_bank0_START  (7)
#define MST_DP1_CHANNELEN_BANK0_dp1_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP1_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x121，初值:0x00，宽度:8
 寄存器说明: dp1 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp1 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP1_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP1_BLOCKCTRL2_BANK0_dp1_blockgroupcontrol_bank0_START  (0)
#define MST_DP1_BLOCKCTRL2_BANK0_dp1_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP1_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP1_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x122，初值:0x00，宽度:8
 寄存器说明: dp1的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp1的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP1_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP1_SAMPLECTRL1_BANK0_dp1_sampleintervallow_bank0_START  (0)
#define MST_DP1_SAMPLECTRL1_BANK0_dp1_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP1_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x123，初值:0x00，宽度:8
 寄存器说明: dp1的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp1的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP1_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP1_SAMPLECTRL2_BANK0_dp1_sampleintervalhigh_bank0_START  (0)
#define MST_DP1_SAMPLECTRL2_BANK0_dp1_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP1_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x124，初值:0x00，宽度:8
 寄存器说明: dp1的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_offset1_bank0 : 8;  /* bit[0-7]: dp1的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP1_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP1_OFFSETCTRL1_BANK0_dp1_offset1_bank0_START  (0)
#define MST_DP1_OFFSETCTRL1_BANK0_dp1_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP1_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x125，初值:0x00，宽度:8
 寄存器说明: dp1的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_offset2_bank0 : 8;  /* bit[0-7]: dp1的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP1_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP1_OFFSETCTRL2_BANK0_dp1_offset2_bank0_START  (0)
#define MST_DP1_OFFSETCTRL2_BANK0_dp1_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_HCTRL_BANK0_UNION
 结构说明  : DP1_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x126，初值:0x11，宽度:8
 寄存器说明: dp1的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_hstop_bank0  : 4;  /* bit[0-3]: dp1传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp1_hstart_bank0 : 4;  /* bit[4-7]: dp1传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP1_HCTRL_BANK0_UNION;
#endif
#define MST_DP1_HCTRL_BANK0_dp1_hstop_bank0_START   (0)
#define MST_DP1_HCTRL_BANK0_dp1_hstop_bank0_END     (3)
#define MST_DP1_HCTRL_BANK0_dp1_hstart_bank0_START  (4)
#define MST_DP1_HCTRL_BANK0_dp1_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP1_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x127，初值:0x00，宽度:8
 寄存器说明: dp1 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_blockpackingmode_bank0 : 1;  /* bit[0]  : dp1 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP1_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP1_BLOCKCTRL3_BANK0_dp1_blockpackingmode_bank0_START  (0)
#define MST_DP1_BLOCKCTRL3_BANK0_dp1_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP1_LANECTRL_BANK0_UNION
 结构说明  : DP1_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x128，初值:0x00，宽度:8
 寄存器说明: dp1的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_lanecontrol_bank0 : 3;  /* bit[0-2]: dp1的数据线控制，表示dp1在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP1_LANECTRL_BANK0_UNION;
#endif
#define MST_DP1_LANECTRL_BANK0_dp1_lanecontrol_bank0_START  (0)
#define MST_DP1_LANECTRL_BANK0_dp1_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP1_CHANNELEN_BANK1_UNION
 结构说明  : DP1_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x130，初值:0x00，宽度:8
 寄存器说明: dp1的通道使能寄存器(bank1)(仅channel1\2有效，其他勿配，下同)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_enable_channel1_bank1 : 1;  /* bit[0]: dp1的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel2_bank1 : 1;  /* bit[1]: dp1的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel3_bank1 : 1;  /* bit[2]: dp1的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel4_bank1 : 1;  /* bit[3]: dp1的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel5_bank1 : 1;  /* bit[4]: dp1的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel6_bank1 : 1;  /* bit[5]: dp1的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel7_bank1 : 1;  /* bit[6]: dp1的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp1_enable_channel8_bank1 : 1;  /* bit[7]: dp1的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP1_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel1_bank1_START  (0)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel1_bank1_END    (0)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel2_bank1_START  (1)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel2_bank1_END    (1)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel3_bank1_START  (2)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel3_bank1_END    (2)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel4_bank1_START  (3)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel4_bank1_END    (3)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel5_bank1_START  (4)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel5_bank1_END    (4)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel6_bank1_START  (5)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel6_bank1_END    (5)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel7_bank1_START  (6)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel7_bank1_END    (6)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel8_bank1_START  (7)
#define MST_DP1_CHANNELEN_BANK1_dp1_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP1_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x131，初值:0x00，宽度:8
 寄存器说明: dp1 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp1 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP1_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP1_BLOCKCTRL2_BANK1_dp1_blockgroupcontrol_bank1_START  (0)
#define MST_DP1_BLOCKCTRL2_BANK1_dp1_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP1_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP1_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x132，初值:0x00，宽度:8
 寄存器说明: dp1的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp1的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP1_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP1_SAMPLECTRL1_BANK1_dp1_sampleintervallow_bank1_START  (0)
#define MST_DP1_SAMPLECTRL1_BANK1_dp1_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP1_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x133，初值:0x00，宽度:8
 寄存器说明: dp1的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp1的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP1_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP1_SAMPLECTRL2_BANK1_dp1_sampleintervalhigh_bank1_START  (0)
#define MST_DP1_SAMPLECTRL2_BANK1_dp1_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP1_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x134，初值:0x00，宽度:8
 寄存器说明: dp1的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_offset1_bank1 : 8;  /* bit[0-7]: dp1的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP1_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP1_OFFSETCTRL1_BANK1_dp1_offset1_bank1_START  (0)
#define MST_DP1_OFFSETCTRL1_BANK1_dp1_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP1_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x135，初值:0x00，宽度:8
 寄存器说明: dp1的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_offset2_bank1 : 8;  /* bit[0-7]: dp1的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP1_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP1_OFFSETCTRL2_BANK1_dp1_offset2_bank1_START  (0)
#define MST_DP1_OFFSETCTRL2_BANK1_dp1_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_HCTRL_BANK1_UNION
 结构说明  : DP1_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x136，初值:0x11，宽度:8
 寄存器说明: dp1的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_hstop_bank1  : 4;  /* bit[0-3]: dp1传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp1_hstart_bank1 : 4;  /* bit[4-7]: dp1传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP1_HCTRL_BANK1_UNION;
#endif
#define MST_DP1_HCTRL_BANK1_dp1_hstop_bank1_START   (0)
#define MST_DP1_HCTRL_BANK1_dp1_hstop_bank1_END     (3)
#define MST_DP1_HCTRL_BANK1_dp1_hstart_bank1_START  (4)
#define MST_DP1_HCTRL_BANK1_dp1_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP1_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP1_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x137，初值:0x00，宽度:8
 寄存器说明: dp1 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_blockpackingmode_bank1 : 1;  /* bit[0]  : dp1 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP1_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP1_BLOCKCTRL3_BANK1_dp1_blockpackingmode_bank1_START  (0)
#define MST_DP1_BLOCKCTRL3_BANK1_dp1_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP1_LANECTRL_BANK1_UNION
 结构说明  : DP1_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x138，初值:0x00，宽度:8
 寄存器说明: dp1的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp1_lanecontrol_bank1 : 3;  /* bit[0-2]: dp1的数据线控制，表示dp1在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP1_LANECTRL_BANK1_UNION;
#endif
#define MST_DP1_LANECTRL_BANK1_dp1_lanecontrol_bank1_START  (0)
#define MST_DP1_LANECTRL_BANK1_dp1_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP2_INTSTAT_UNION
 结构说明  : DP2_INTSTAT 寄存器结构定义。地址偏移量:0x200，初值:0x00，宽度:8
 寄存器说明: dp2中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_test_fail_int  : 1;  /* bit[0]  : dp2 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp2_port_ready_int : 1;  /* bit[1]  : dp2 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp2_cfg_error0_int : 1;  /* bit[5]  : dp2自定义中断1:(业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp2_cfg_error1_int : 1;  /* bit[6]  : dp2自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp2_cfg_error2_int : 1;  /* bit[7]  : dp2自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP2_INTSTAT_UNION;
#endif
#define MST_DP2_INTSTAT_dp2_test_fail_int_START   (0)
#define MST_DP2_INTSTAT_dp2_test_fail_int_END     (0)
#define MST_DP2_INTSTAT_dp2_port_ready_int_START  (1)
#define MST_DP2_INTSTAT_dp2_port_ready_int_END    (1)
#define MST_DP2_INTSTAT_dp2_cfg_error0_int_START  (5)
#define MST_DP2_INTSTAT_dp2_cfg_error0_int_END    (5)
#define MST_DP2_INTSTAT_dp2_cfg_error1_int_START  (6)
#define MST_DP2_INTSTAT_dp2_cfg_error1_int_END    (6)
#define MST_DP2_INTSTAT_dp2_cfg_error2_int_START  (7)
#define MST_DP2_INTSTAT_dp2_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_INTMASK_UNION
 结构说明  : DP2_INTMASK 寄存器结构定义。地址偏移量:0x201，初值:0x00，宽度:8
 寄存器说明: dp2中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_test_fail_int_mask  : 1;  /* bit[0]  : dp2 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp2_port_ready_int_mask : 1;  /* bit[1]  : dp2 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp2_cfg_error0_int_mask : 1;  /* bit[5]  : dp2自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp2_cfg_error1_int_mask : 1;  /* bit[6]  : dp2自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp2_cfg_error2_int_mask : 1;  /* bit[7]  : dp2自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP2_INTMASK_UNION;
#endif
#define MST_DP2_INTMASK_dp2_test_fail_int_mask_START   (0)
#define MST_DP2_INTMASK_dp2_test_fail_int_mask_END     (0)
#define MST_DP2_INTMASK_dp2_port_ready_int_mask_START  (1)
#define MST_DP2_INTMASK_dp2_port_ready_int_mask_END    (1)
#define MST_DP2_INTMASK_dp2_cfg_error0_int_mask_START  (5)
#define MST_DP2_INTMASK_dp2_cfg_error0_int_mask_END    (5)
#define MST_DP2_INTMASK_dp2_cfg_error1_int_mask_START  (6)
#define MST_DP2_INTMASK_dp2_cfg_error1_int_mask_END    (6)
#define MST_DP2_INTMASK_dp2_cfg_error2_int_mask_START  (7)
#define MST_DP2_INTMASK_dp2_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_PORTCTRL_UNION
 结构说明  : DP2_PORTCTRL 寄存器结构定义。地址偏移量:0x202，初值:0x00，宽度:8
 寄存器说明: dp2的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_portflowmode   : 2;  /* bit[0-1]: dp2流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp2_portdatamode   : 2;  /* bit[2-3]: dp2 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp2_nextinvertbank : 1;  /* bit[4]  : dp2 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP2_PORTCTRL_UNION;
#endif
#define MST_DP2_PORTCTRL_dp2_portflowmode_START    (0)
#define MST_DP2_PORTCTRL_dp2_portflowmode_END      (1)
#define MST_DP2_PORTCTRL_dp2_portdatamode_START    (2)
#define MST_DP2_PORTCTRL_dp2_portdatamode_END      (3)
#define MST_DP2_PORTCTRL_dp2_nextinvertbank_START  (4)
#define MST_DP2_PORTCTRL_dp2_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP2_BLOCKCTRL1_UNION
 结构说明  : DP2_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x203，初值:0x1F，宽度:8
 寄存器说明: dp2 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_wordlength : 6;  /* bit[0-5]: dp2 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP2_BLOCKCTRL1_UNION;
#endif
#define MST_DP2_BLOCKCTRL1_dp2_wordlength_START  (0)
#define MST_DP2_BLOCKCTRL1_dp2_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP2_PREPARESTATUS_UNION
 结构说明  : DP2_PREPARESTATUS 寄存器结构定义。地址偏移量:0x204，初值:0x00，宽度:8
 寄存器说明: dp2的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_n_finished_channel1 : 1;  /* bit[0]: dp2通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel2 : 1;  /* bit[1]: dp2通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel3 : 1;  /* bit[2]: dp2通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel4 : 1;  /* bit[3]: dp2通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel5 : 1;  /* bit[4]: dp2通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel6 : 1;  /* bit[5]: dp2通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel7 : 1;  /* bit[6]: dp2通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp2_n_finished_channel8 : 1;  /* bit[7]: dp2通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP2_PREPARESTATUS_UNION;
#endif
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel1_START  (0)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel1_END    (0)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel2_START  (1)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel2_END    (1)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel3_START  (2)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel3_END    (2)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel4_START  (3)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel4_END    (3)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel5_START  (4)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel5_END    (4)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel6_START  (5)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel6_END    (5)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel7_START  (6)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel7_END    (6)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel8_START  (7)
#define MST_DP2_PREPARESTATUS_dp2_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_PREPARECTRL_UNION
 结构说明  : DP2_PREPARECTRL 寄存器结构定义。地址偏移量:0x205，初值:0x00，宽度:8
 寄存器说明: dp2的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_prepare_channel1 : 1;  /* bit[0]: dp2使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel2 : 1;  /* bit[1]: dp2使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel3 : 1;  /* bit[2]: dp2使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel4 : 1;  /* bit[3]: dp2使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel5 : 1;  /* bit[4]: dp2使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel6 : 1;  /* bit[5]: dp2使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel7 : 1;  /* bit[6]: dp2使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp2_prepare_channel8 : 1;  /* bit[7]: dp2使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP2_PREPARECTRL_UNION;
#endif
#define MST_DP2_PREPARECTRL_dp2_prepare_channel1_START  (0)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel1_END    (0)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel2_START  (1)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel2_END    (1)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel3_START  (2)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel3_END    (2)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel4_START  (3)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel4_END    (3)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel5_START  (4)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel5_END    (4)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel6_START  (5)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel6_END    (5)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel7_START  (6)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel7_END    (6)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel8_START  (7)
#define MST_DP2_PREPARECTRL_dp2_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_CHANNELEN_BANK0_UNION
 结构说明  : DP2_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x220，初值:0x00，宽度:8
 寄存器说明: dp2的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_enable_channel1_bank0 : 1;  /* bit[0]: dp2的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel2_bank0 : 1;  /* bit[1]: dp2的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel3_bank0 : 1;  /* bit[2]: dp2的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel4_bank0 : 1;  /* bit[3]: dp2的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel5_bank0 : 1;  /* bit[4]: dp2的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel6_bank0 : 1;  /* bit[5]: dp2的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel7_bank0 : 1;  /* bit[6]: dp2的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel8_bank0 : 1;  /* bit[7]: dp2的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP2_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel1_bank0_START  (0)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel1_bank0_END    (0)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel2_bank0_START  (1)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel2_bank0_END    (1)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel3_bank0_START  (2)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel3_bank0_END    (2)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel4_bank0_START  (3)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel4_bank0_END    (3)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel5_bank0_START  (4)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel5_bank0_END    (4)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel6_bank0_START  (5)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel6_bank0_END    (5)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel7_bank0_START  (6)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel7_bank0_END    (6)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel8_bank0_START  (7)
#define MST_DP2_CHANNELEN_BANK0_dp2_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP2_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x221，初值:0x00，宽度:8
 寄存器说明: dp2 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp2 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP2_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP2_BLOCKCTRL2_BANK0_dp2_blockgroupcontrol_bank0_START  (0)
#define MST_DP2_BLOCKCTRL2_BANK0_dp2_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP2_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP2_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x222，初值:0x00，宽度:8
 寄存器说明: dp2的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp2的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP2_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP2_SAMPLECTRL1_BANK0_dp2_sampleintervallow_bank0_START  (0)
#define MST_DP2_SAMPLECTRL1_BANK0_dp2_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP2_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x223，初值:0x00，宽度:8
 寄存器说明: dp2的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp2的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP2_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP2_SAMPLECTRL2_BANK0_dp2_sampleintervalhigh_bank0_START  (0)
#define MST_DP2_SAMPLECTRL2_BANK0_dp2_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP2_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x224，初值:0x00，宽度:8
 寄存器说明: dp2的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_offset1_bank0 : 8;  /* bit[0-7]: dp2的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP2_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP2_OFFSETCTRL1_BANK0_dp2_offset1_bank0_START  (0)
#define MST_DP2_OFFSETCTRL1_BANK0_dp2_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP2_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x225，初值:0x00，宽度:8
 寄存器说明: dp2的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_offset2_bank0 : 8;  /* bit[0-7]: dp2的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP2_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP2_OFFSETCTRL2_BANK0_dp2_offset2_bank0_START  (0)
#define MST_DP2_OFFSETCTRL2_BANK0_dp2_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_HCTRL_BANK0_UNION
 结构说明  : DP2_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x226，初值:0x11，宽度:8
 寄存器说明: dp2的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_hstop_bank0  : 4;  /* bit[0-3]: dp2传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp2_hstart_bank0 : 4;  /* bit[4-7]: dp2传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP2_HCTRL_BANK0_UNION;
#endif
#define MST_DP2_HCTRL_BANK0_dp2_hstop_bank0_START   (0)
#define MST_DP2_HCTRL_BANK0_dp2_hstop_bank0_END     (3)
#define MST_DP2_HCTRL_BANK0_dp2_hstart_bank0_START  (4)
#define MST_DP2_HCTRL_BANK0_dp2_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP2_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x227，初值:0x00，宽度:8
 寄存器说明: dp2 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_blockpackingmode_bank0 : 1;  /* bit[0]  : dp2 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP2_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP2_BLOCKCTRL3_BANK0_dp2_blockpackingmode_bank0_START  (0)
#define MST_DP2_BLOCKCTRL3_BANK0_dp2_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP2_LANECTRL_BANK0_UNION
 结构说明  : DP2_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x228，初值:0x00，宽度:8
 寄存器说明: dp2的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_lanecontrol_bank0 : 3;  /* bit[0-2]: dp2的数据线控制，表示dp2在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP2_LANECTRL_BANK0_UNION;
#endif
#define MST_DP2_LANECTRL_BANK0_dp2_lanecontrol_bank0_START  (0)
#define MST_DP2_LANECTRL_BANK0_dp2_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP2_CHANNELEN_BANK1_UNION
 结构说明  : DP2_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x230，初值:0x00，宽度:8
 寄存器说明: dp2的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_enable_channel1_bank1 : 1;  /* bit[0]: dp2的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel2_bank1 : 1;  /* bit[1]: dp2的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel3_bank1 : 1;  /* bit[2]: dp2的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel4_bank1 : 1;  /* bit[3]: dp2的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel5_bank1 : 1;  /* bit[4]: dp2的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel6_bank1 : 1;  /* bit[5]: dp2的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel7_bank1 : 1;  /* bit[6]: dp2的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp2_enable_channel8_bank1 : 1;  /* bit[7]: dp2的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP2_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel1_bank1_START  (0)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel1_bank1_END    (0)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel2_bank1_START  (1)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel2_bank1_END    (1)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel3_bank1_START  (2)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel3_bank1_END    (2)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel4_bank1_START  (3)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel4_bank1_END    (3)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel5_bank1_START  (4)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel5_bank1_END    (4)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel6_bank1_START  (5)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel6_bank1_END    (5)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel7_bank1_START  (6)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel7_bank1_END    (6)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel8_bank1_START  (7)
#define MST_DP2_CHANNELEN_BANK1_dp2_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP2_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x231，初值:0x00，宽度:8
 寄存器说明: dp2 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp2 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP2_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP2_BLOCKCTRL2_BANK1_dp2_blockgroupcontrol_bank1_START  (0)
#define MST_DP2_BLOCKCTRL2_BANK1_dp2_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP2_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP2_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x232，初值:0x00，宽度:8
 寄存器说明: dp2的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp2的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP2_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP2_SAMPLECTRL1_BANK1_dp2_sampleintervallow_bank1_START  (0)
#define MST_DP2_SAMPLECTRL1_BANK1_dp2_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP2_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x233，初值:0x00，宽度:8
 寄存器说明: dp2的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp2的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP2_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP2_SAMPLECTRL2_BANK1_dp2_sampleintervalhigh_bank1_START  (0)
#define MST_DP2_SAMPLECTRL2_BANK1_dp2_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP2_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x234，初值:0x00，宽度:8
 寄存器说明: dp2的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_offset1_bank1 : 8;  /* bit[0-7]: dp2的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP2_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP2_OFFSETCTRL1_BANK1_dp2_offset1_bank1_START  (0)
#define MST_DP2_OFFSETCTRL1_BANK1_dp2_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP2_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x235，初值:0x00，宽度:8
 寄存器说明: dp2的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_offset2_bank1 : 8;  /* bit[0-7]: dp2的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP2_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP2_OFFSETCTRL2_BANK1_dp2_offset2_bank1_START  (0)
#define MST_DP2_OFFSETCTRL2_BANK1_dp2_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_HCTRL_BANK1_UNION
 结构说明  : DP2_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x236，初值:0x11，宽度:8
 寄存器说明: dp2的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_hstop_bank1  : 4;  /* bit[0-3]: dp2传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp2_hstart_bank1 : 4;  /* bit[4-7]: dp2传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP2_HCTRL_BANK1_UNION;
#endif
#define MST_DP2_HCTRL_BANK1_dp2_hstop_bank1_START   (0)
#define MST_DP2_HCTRL_BANK1_dp2_hstop_bank1_END     (3)
#define MST_DP2_HCTRL_BANK1_dp2_hstart_bank1_START  (4)
#define MST_DP2_HCTRL_BANK1_dp2_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP2_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP2_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x237，初值:0x00，宽度:8
 寄存器说明: dp2 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_blockpackingmode_bank1 : 1;  /* bit[0]  : dp2 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP2_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP2_BLOCKCTRL3_BANK1_dp2_blockpackingmode_bank1_START  (0)
#define MST_DP2_BLOCKCTRL3_BANK1_dp2_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP2_LANECTRL_BANK1_UNION
 结构说明  : DP2_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x238，初值:0x00，宽度:8
 寄存器说明: dp2的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp2_lanecontrol_bank1 : 3;  /* bit[0-2]: dp2的数据线控制，表示dp2在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP2_LANECTRL_BANK1_UNION;
#endif
#define MST_DP2_LANECTRL_BANK1_dp2_lanecontrol_bank1_START  (0)
#define MST_DP2_LANECTRL_BANK1_dp2_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP3_INTSTAT_UNION
 结构说明  : DP3_INTSTAT 寄存器结构定义。地址偏移量:0x300，初值:0x00，宽度:8
 寄存器说明: dp3中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_test_fail_int  : 1;  /* bit[0]  : dp3 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp3_port_ready_int : 1;  /* bit[1]  : dp3 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp3_cfg_error0_int : 1;  /* bit[5]  : dp3自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp3_cfg_error1_int : 1;  /* bit[6]  : dp3自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp3_cfg_error2_int : 1;  /* bit[7]  : dp3自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP3_INTSTAT_UNION;
#endif
#define MST_DP3_INTSTAT_dp3_test_fail_int_START   (0)
#define MST_DP3_INTSTAT_dp3_test_fail_int_END     (0)
#define MST_DP3_INTSTAT_dp3_port_ready_int_START  (1)
#define MST_DP3_INTSTAT_dp3_port_ready_int_END    (1)
#define MST_DP3_INTSTAT_dp3_cfg_error0_int_START  (5)
#define MST_DP3_INTSTAT_dp3_cfg_error0_int_END    (5)
#define MST_DP3_INTSTAT_dp3_cfg_error1_int_START  (6)
#define MST_DP3_INTSTAT_dp3_cfg_error1_int_END    (6)
#define MST_DP3_INTSTAT_dp3_cfg_error2_int_START  (7)
#define MST_DP3_INTSTAT_dp3_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_INTMASK_UNION
 结构说明  : DP3_INTMASK 寄存器结构定义。地址偏移量:0x301，初值:0x00，宽度:8
 寄存器说明: dp3中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_test_fail_int_mask  : 1;  /* bit[0]  : dp3 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp3_port_ready_int_mask : 1;  /* bit[1]  : dp3 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp3_cfg_error0_int_mask : 1;  /* bit[5]  : dp3自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp3_cfg_error1_int_mask : 1;  /* bit[6]  : dp3自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp3_cfg_error2_int_mask : 1;  /* bit[7]  : dp3自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP3_INTMASK_UNION;
#endif
#define MST_DP3_INTMASK_dp3_test_fail_int_mask_START   (0)
#define MST_DP3_INTMASK_dp3_test_fail_int_mask_END     (0)
#define MST_DP3_INTMASK_dp3_port_ready_int_mask_START  (1)
#define MST_DP3_INTMASK_dp3_port_ready_int_mask_END    (1)
#define MST_DP3_INTMASK_dp3_cfg_error0_int_mask_START  (5)
#define MST_DP3_INTMASK_dp3_cfg_error0_int_mask_END    (5)
#define MST_DP3_INTMASK_dp3_cfg_error1_int_mask_START  (6)
#define MST_DP3_INTMASK_dp3_cfg_error1_int_mask_END    (6)
#define MST_DP3_INTMASK_dp3_cfg_error2_int_mask_START  (7)
#define MST_DP3_INTMASK_dp3_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_PORTCTRL_UNION
 结构说明  : DP3_PORTCTRL 寄存器结构定义。地址偏移量:0x302，初值:0x00，宽度:8
 寄存器说明: dp3的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_portflowmode   : 2;  /* bit[0-1]: dp3流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp3_portdatamode   : 2;  /* bit[2-3]: dp3 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp3_nextinvertbank : 1;  /* bit[4]  : dp3 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP3_PORTCTRL_UNION;
#endif
#define MST_DP3_PORTCTRL_dp3_portflowmode_START    (0)
#define MST_DP3_PORTCTRL_dp3_portflowmode_END      (1)
#define MST_DP3_PORTCTRL_dp3_portdatamode_START    (2)
#define MST_DP3_PORTCTRL_dp3_portdatamode_END      (3)
#define MST_DP3_PORTCTRL_dp3_nextinvertbank_START  (4)
#define MST_DP3_PORTCTRL_dp3_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP3_BLOCKCTRL1_UNION
 结构说明  : DP3_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x303，初值:0x1F，宽度:8
 寄存器说明: dp3 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_wordlength : 6;  /* bit[0-5]: dp3 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP3_BLOCKCTRL1_UNION;
#endif
#define MST_DP3_BLOCKCTRL1_dp3_wordlength_START  (0)
#define MST_DP3_BLOCKCTRL1_dp3_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP3_PREPARESTATUS_UNION
 结构说明  : DP3_PREPARESTATUS 寄存器结构定义。地址偏移量:0x304，初值:0x00，宽度:8
 寄存器说明: dp3的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_n_finished_channel1 : 1;  /* bit[0]: dp3通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel2 : 1;  /* bit[1]: dp3通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel3 : 1;  /* bit[2]: dp3通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel4 : 1;  /* bit[3]: dp3通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel5 : 1;  /* bit[4]: dp3通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel6 : 1;  /* bit[5]: dp3通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel7 : 1;  /* bit[6]: dp3通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp3_n_finished_channel8 : 1;  /* bit[7]: dp3通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP3_PREPARESTATUS_UNION;
#endif
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel1_START  (0)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel1_END    (0)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel2_START  (1)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel2_END    (1)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel3_START  (2)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel3_END    (2)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel4_START  (3)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel4_END    (3)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel5_START  (4)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel5_END    (4)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel6_START  (5)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel6_END    (5)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel7_START  (6)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel7_END    (6)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel8_START  (7)
#define MST_DP3_PREPARESTATUS_dp3_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_PREPARECTRL_UNION
 结构说明  : DP3_PREPARECTRL 寄存器结构定义。地址偏移量:0x305，初值:0x00，宽度:8
 寄存器说明: dp3的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_prepare_channel1 : 1;  /* bit[0]: dp3使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel2 : 1;  /* bit[1]: dp3使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel3 : 1;  /* bit[2]: dp3使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel4 : 1;  /* bit[3]: dp3使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel5 : 1;  /* bit[4]: dp3使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel6 : 1;  /* bit[5]: dp3使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel7 : 1;  /* bit[6]: dp3使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp3_prepare_channel8 : 1;  /* bit[7]: dp3使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP3_PREPARECTRL_UNION;
#endif
#define MST_DP3_PREPARECTRL_dp3_prepare_channel1_START  (0)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel1_END    (0)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel2_START  (1)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel2_END    (1)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel3_START  (2)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel3_END    (2)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel4_START  (3)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel4_END    (3)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel5_START  (4)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel5_END    (4)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel6_START  (5)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel6_END    (5)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel7_START  (6)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel7_END    (6)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel8_START  (7)
#define MST_DP3_PREPARECTRL_dp3_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_CHANNELEN_BANK0_UNION
 结构说明  : DP3_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x320，初值:0x00，宽度:8
 寄存器说明: dp3的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_enable_channel1_bank0 : 1;  /* bit[0]: dp3的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel2_bank0 : 1;  /* bit[1]: dp3的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel3_bank0 : 1;  /* bit[2]: dp3的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel4_bank0 : 1;  /* bit[3]: dp3的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel5_bank0 : 1;  /* bit[4]: dp3的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel6_bank0 : 1;  /* bit[5]: dp3的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel7_bank0 : 1;  /* bit[6]: dp3的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel8_bank0 : 1;  /* bit[7]: dp3的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP3_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel1_bank0_START  (0)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel1_bank0_END    (0)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel2_bank0_START  (1)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel2_bank0_END    (1)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel3_bank0_START  (2)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel3_bank0_END    (2)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel4_bank0_START  (3)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel4_bank0_END    (3)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel5_bank0_START  (4)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel5_bank0_END    (4)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel6_bank0_START  (5)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel6_bank0_END    (5)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel7_bank0_START  (6)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel7_bank0_END    (6)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel8_bank0_START  (7)
#define MST_DP3_CHANNELEN_BANK0_dp3_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP3_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x321，初值:0x00，宽度:8
 寄存器说明: dp3 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp3 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP3_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP3_BLOCKCTRL2_BANK0_dp3_blockgroupcontrol_bank0_START  (0)
#define MST_DP3_BLOCKCTRL2_BANK0_dp3_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP3_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP3_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x322，初值:0x00，宽度:8
 寄存器说明: dp3的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp3的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP3_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP3_SAMPLECTRL1_BANK0_dp3_sampleintervallow_bank0_START  (0)
#define MST_DP3_SAMPLECTRL1_BANK0_dp3_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP3_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x323，初值:0x00，宽度:8
 寄存器说明: dp3的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp3的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP3_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP3_SAMPLECTRL2_BANK0_dp3_sampleintervalhigh_bank0_START  (0)
#define MST_DP3_SAMPLECTRL2_BANK0_dp3_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP3_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x324，初值:0x00，宽度:8
 寄存器说明: dp3的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_offset1_bank0 : 8;  /* bit[0-7]: dp3的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP3_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP3_OFFSETCTRL1_BANK0_dp3_offset1_bank0_START  (0)
#define MST_DP3_OFFSETCTRL1_BANK0_dp3_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP3_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x325，初值:0x00，宽度:8
 寄存器说明: dp3的Offset控制寄存器高8位(bank0)
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_offset2_bank0 : 8;  /* bit[0-7]: dp3的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP3_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP3_OFFSETCTRL2_BANK0_dp3_offset2_bank0_START  (0)
#define MST_DP3_OFFSETCTRL2_BANK0_dp3_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_HCTRL_BANK0_UNION
 结构说明  : DP3_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x326，初值:0x11，宽度:8
 寄存器说明: dp3的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_hstop_bank0  : 4;  /* bit[0-3]: dp3传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp3_hstart_bank0 : 4;  /* bit[4-7]: dp3传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP3_HCTRL_BANK0_UNION;
#endif
#define MST_DP3_HCTRL_BANK0_dp3_hstop_bank0_START   (0)
#define MST_DP3_HCTRL_BANK0_dp3_hstop_bank0_END     (3)
#define MST_DP3_HCTRL_BANK0_dp3_hstart_bank0_START  (4)
#define MST_DP3_HCTRL_BANK0_dp3_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP3_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x327，初值:0x00，宽度:8
 寄存器说明: dp3 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_blockpackingmode_bank0 : 1;  /* bit[0]  : dp3 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP3_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP3_BLOCKCTRL3_BANK0_dp3_blockpackingmode_bank0_START  (0)
#define MST_DP3_BLOCKCTRL3_BANK0_dp3_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP3_LANECTRL_BANK0_UNION
 结构说明  : DP3_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x328，初值:0x00，宽度:8
 寄存器说明: dp3的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_lanecontrol_bank0 : 3;  /* bit[0-2]: dp3的数据线控制，表示dp3在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP3_LANECTRL_BANK0_UNION;
#endif
#define MST_DP3_LANECTRL_BANK0_dp3_lanecontrol_bank0_START  (0)
#define MST_DP3_LANECTRL_BANK0_dp3_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP3_CHANNELEN_BANK1_UNION
 结构说明  : DP3_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x330，初值:0x00，宽度:8
 寄存器说明: dp3的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_enable_channel1_bank1 : 1;  /* bit[0]: dp3的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel2_bank1 : 1;  /* bit[1]: dp3的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel3_bank1 : 1;  /* bit[2]: dp3的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel4_bank1 : 1;  /* bit[3]: dp3的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel5_bank1 : 1;  /* bit[4]: dp3的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel6_bank1 : 1;  /* bit[5]: dp3的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel7_bank1 : 1;  /* bit[6]: dp3的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp3_enable_channel8_bank1 : 1;  /* bit[7]: dp3的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP3_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel1_bank1_START  (0)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel1_bank1_END    (0)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel2_bank1_START  (1)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel2_bank1_END    (1)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel3_bank1_START  (2)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel3_bank1_END    (2)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel4_bank1_START  (3)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel4_bank1_END    (3)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel5_bank1_START  (4)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel5_bank1_END    (4)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel6_bank1_START  (5)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel6_bank1_END    (5)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel7_bank1_START  (6)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel7_bank1_END    (6)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel8_bank1_START  (7)
#define MST_DP3_CHANNELEN_BANK1_dp3_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP3_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x331，初值:0x00，宽度:8
 寄存器说明: dp3 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp3 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP3_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP3_BLOCKCTRL2_BANK1_dp3_blockgroupcontrol_bank1_START  (0)
#define MST_DP3_BLOCKCTRL2_BANK1_dp3_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP3_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP3_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x332，初值:0x00，宽度:8
 寄存器说明: dp3的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp3的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP3_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP3_SAMPLECTRL1_BANK1_dp3_sampleintervallow_bank1_START  (0)
#define MST_DP3_SAMPLECTRL1_BANK1_dp3_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP3_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x333，初值:0x00，宽度:8
 寄存器说明: dp3的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp3的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP3_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP3_SAMPLECTRL2_BANK1_dp3_sampleintervalhigh_bank1_START  (0)
#define MST_DP3_SAMPLECTRL2_BANK1_dp3_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP3_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x334，初值:0x00，宽度:8
 寄存器说明: dp3的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_offset1_bank1 : 8;  /* bit[0-7]: dp3的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP3_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP3_OFFSETCTRL1_BANK1_dp3_offset1_bank1_START  (0)
#define MST_DP3_OFFSETCTRL1_BANK1_dp3_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP3_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x335，初值:0x00，宽度:8
 寄存器说明: dp3的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_offset2_bank1 : 8;  /* bit[0-7]: dp3的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP3_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP3_OFFSETCTRL2_BANK1_dp3_offset2_bank1_START  (0)
#define MST_DP3_OFFSETCTRL2_BANK1_dp3_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_HCTRL_BANK1_UNION
 结构说明  : DP3_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x336，初值:0x11，宽度:8
 寄存器说明: dp3的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_hstop_bank1  : 4;  /* bit[0-3]: dp3传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp3_hstart_bank1 : 4;  /* bit[4-7]: dp3传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP3_HCTRL_BANK1_UNION;
#endif
#define MST_DP3_HCTRL_BANK1_dp3_hstop_bank1_START   (0)
#define MST_DP3_HCTRL_BANK1_dp3_hstop_bank1_END     (3)
#define MST_DP3_HCTRL_BANK1_dp3_hstart_bank1_START  (4)
#define MST_DP3_HCTRL_BANK1_dp3_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP3_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP3_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x337，初值:0x00，宽度:8
 寄存器说明: dp3 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_blockpackingmode_bank1 : 1;  /* bit[0]  : dp3 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP3_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP3_BLOCKCTRL3_BANK1_dp3_blockpackingmode_bank1_START  (0)
#define MST_DP3_BLOCKCTRL3_BANK1_dp3_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP3_LANECTRL_BANK1_UNION
 结构说明  : DP3_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x338，初值:0x00，宽度:8
 寄存器说明: dp3的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp3_lanecontrol_bank1 : 3;  /* bit[0-2]: dp3的数据线控制，表示dp3在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP3_LANECTRL_BANK1_UNION;
#endif
#define MST_DP3_LANECTRL_BANK1_dp3_lanecontrol_bank1_START  (0)
#define MST_DP3_LANECTRL_BANK1_dp3_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP4_INTSTAT_UNION
 结构说明  : DP4_INTSTAT 寄存器结构定义。地址偏移量:0x400，初值:0x00，宽度:8
 寄存器说明: dp4中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_test_fail_int  : 1;  /* bit[0]  : dp4 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp4_port_ready_int : 1;  /* bit[1]  : dp4 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp4_cfg_error0_int : 1;  /* bit[5]  : dp4自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp4_cfg_error1_int : 1;  /* bit[6]  : dp4自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp4_cfg_error2_int : 1;  /* bit[7]  : dp4自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP4_INTSTAT_UNION;
#endif
#define MST_DP4_INTSTAT_dp4_test_fail_int_START   (0)
#define MST_DP4_INTSTAT_dp4_test_fail_int_END     (0)
#define MST_DP4_INTSTAT_dp4_port_ready_int_START  (1)
#define MST_DP4_INTSTAT_dp4_port_ready_int_END    (1)
#define MST_DP4_INTSTAT_dp4_cfg_error0_int_START  (5)
#define MST_DP4_INTSTAT_dp4_cfg_error0_int_END    (5)
#define MST_DP4_INTSTAT_dp4_cfg_error1_int_START  (6)
#define MST_DP4_INTSTAT_dp4_cfg_error1_int_END    (6)
#define MST_DP4_INTSTAT_dp4_cfg_error2_int_START  (7)
#define MST_DP4_INTSTAT_dp4_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_INTMASK_UNION
 结构说明  : DP4_INTMASK 寄存器结构定义。地址偏移量:0x401，初值:0x00，宽度:8
 寄存器说明: dp4中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_test_fail_int_mask  : 1;  /* bit[0]  : dp4 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp4_port_ready_int_mask : 1;  /* bit[1]  : dp4 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp4_cfg_error0_int_mask : 1;  /* bit[5]  : dp4自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp4_cfg_error1_int_mask : 1;  /* bit[6]  : dp4自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp4_cfg_error2_int_mask : 1;  /* bit[7]  : dp4自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP4_INTMASK_UNION;
#endif
#define MST_DP4_INTMASK_dp4_test_fail_int_mask_START   (0)
#define MST_DP4_INTMASK_dp4_test_fail_int_mask_END     (0)
#define MST_DP4_INTMASK_dp4_port_ready_int_mask_START  (1)
#define MST_DP4_INTMASK_dp4_port_ready_int_mask_END    (1)
#define MST_DP4_INTMASK_dp4_cfg_error0_int_mask_START  (5)
#define MST_DP4_INTMASK_dp4_cfg_error0_int_mask_END    (5)
#define MST_DP4_INTMASK_dp4_cfg_error1_int_mask_START  (6)
#define MST_DP4_INTMASK_dp4_cfg_error1_int_mask_END    (6)
#define MST_DP4_INTMASK_dp4_cfg_error2_int_mask_START  (7)
#define MST_DP4_INTMASK_dp4_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_PORTCTRL_UNION
 结构说明  : DP4_PORTCTRL 寄存器结构定义。地址偏移量:0x402，初值:0x00，宽度:8
 寄存器说明: dp4的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_portflowmode   : 2;  /* bit[0-1]: dp4流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp4_portdatamode   : 2;  /* bit[2-3]: dp4 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp4_nextinvertbank : 1;  /* bit[4]  : dp4 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP4_PORTCTRL_UNION;
#endif
#define MST_DP4_PORTCTRL_dp4_portflowmode_START    (0)
#define MST_DP4_PORTCTRL_dp4_portflowmode_END      (1)
#define MST_DP4_PORTCTRL_dp4_portdatamode_START    (2)
#define MST_DP4_PORTCTRL_dp4_portdatamode_END      (3)
#define MST_DP4_PORTCTRL_dp4_nextinvertbank_START  (4)
#define MST_DP4_PORTCTRL_dp4_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP4_BLOCKCTRL1_UNION
 结构说明  : DP4_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x403，初值:0x1F，宽度:8
 寄存器说明: dp4 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_wordlength : 6;  /* bit[0-5]: dp4 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP4_BLOCKCTRL1_UNION;
#endif
#define MST_DP4_BLOCKCTRL1_dp4_wordlength_START  (0)
#define MST_DP4_BLOCKCTRL1_dp4_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP4_PREPARESTATUS_UNION
 结构说明  : DP4_PREPARESTATUS 寄存器结构定义。地址偏移量:0x404，初值:0x00，宽度:8
 寄存器说明: dp4的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_n_finished_channel1 : 1;  /* bit[0]: dp4通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel2 : 1;  /* bit[1]: dp4通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel3 : 1;  /* bit[2]: dp4通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel4 : 1;  /* bit[3]: dp4通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel5 : 1;  /* bit[4]: dp4通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel6 : 1;  /* bit[5]: dp4通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel7 : 1;  /* bit[6]: dp4通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp4_n_finished_channel8 : 1;  /* bit[7]: dp4通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP4_PREPARESTATUS_UNION;
#endif
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel1_START  (0)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel1_END    (0)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel2_START  (1)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel2_END    (1)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel3_START  (2)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel3_END    (2)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel4_START  (3)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel4_END    (3)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel5_START  (4)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel5_END    (4)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel6_START  (5)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel6_END    (5)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel7_START  (6)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel7_END    (6)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel8_START  (7)
#define MST_DP4_PREPARESTATUS_dp4_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_PREPARECTRL_UNION
 结构说明  : DP4_PREPARECTRL 寄存器结构定义。地址偏移量:0x405，初值:0x00，宽度:8
 寄存器说明: dp4的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_prepare_channel1 : 1;  /* bit[0]: dp4使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel2 : 1;  /* bit[1]: dp4使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel3 : 1;  /* bit[2]: dp4使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel4 : 1;  /* bit[3]: dp4使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel5 : 1;  /* bit[4]: dp4使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel6 : 1;  /* bit[5]: dp4使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel7 : 1;  /* bit[6]: dp4使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp4_prepare_channel8 : 1;  /* bit[7]: dp4使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP4_PREPARECTRL_UNION;
#endif
#define MST_DP4_PREPARECTRL_dp4_prepare_channel1_START  (0)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel1_END    (0)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel2_START  (1)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel2_END    (1)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel3_START  (2)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel3_END    (2)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel4_START  (3)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel4_END    (3)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel5_START  (4)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel5_END    (4)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel6_START  (5)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel6_END    (5)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel7_START  (6)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel7_END    (6)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel8_START  (7)
#define MST_DP4_PREPARECTRL_dp4_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_CHANNELEN_BANK0_UNION
 结构说明  : DP4_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x420，初值:0x00，宽度:8
 寄存器说明: dp4的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_enable_channel1_bank0 : 1;  /* bit[0]: dp4的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel2_bank0 : 1;  /* bit[1]: dp4的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel3_bank0 : 1;  /* bit[2]: dp4的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel4_bank0 : 1;  /* bit[3]: dp4的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel5_bank0 : 1;  /* bit[4]: dp4的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel6_bank0 : 1;  /* bit[5]: dp4的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel7_bank0 : 1;  /* bit[6]: dp4的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel8_bank0 : 1;  /* bit[7]: dp4的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP4_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel1_bank0_START  (0)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel1_bank0_END    (0)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel2_bank0_START  (1)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel2_bank0_END    (1)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel3_bank0_START  (2)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel3_bank0_END    (2)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel4_bank0_START  (3)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel4_bank0_END    (3)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel5_bank0_START  (4)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel5_bank0_END    (4)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel6_bank0_START  (5)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel6_bank0_END    (5)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel7_bank0_START  (6)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel7_bank0_END    (6)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel8_bank0_START  (7)
#define MST_DP4_CHANNELEN_BANK0_dp4_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP4_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x421，初值:0x00，宽度:8
 寄存器说明: dp4 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp4 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP4_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP4_BLOCKCTRL2_BANK0_dp4_blockgroupcontrol_bank0_START  (0)
#define MST_DP4_BLOCKCTRL2_BANK0_dp4_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP4_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP4_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x422，初值:0x00，宽度:8
 寄存器说明: dp4的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp4的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP4_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP4_SAMPLECTRL1_BANK0_dp4_sampleintervallow_bank0_START  (0)
#define MST_DP4_SAMPLECTRL1_BANK0_dp4_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP4_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x423，初值:0x00，宽度:8
 寄存器说明: dp4的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp4的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP4_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP4_SAMPLECTRL2_BANK0_dp4_sampleintervalhigh_bank0_START  (0)
#define MST_DP4_SAMPLECTRL2_BANK0_dp4_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP4_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x424，初值:0x00，宽度:8
 寄存器说明: dp4的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_offset1_bank0 : 8;  /* bit[0-7]: dp4的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP4_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP4_OFFSETCTRL1_BANK0_dp4_offset1_bank0_START  (0)
#define MST_DP4_OFFSETCTRL1_BANK0_dp4_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP4_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x425，初值:0x00，宽度:8
 寄存器说明: dp4的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_offset2_bank0 : 8;  /* bit[0-7]: dp4的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP4_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP4_OFFSETCTRL2_BANK0_dp4_offset2_bank0_START  (0)
#define MST_DP4_OFFSETCTRL2_BANK0_dp4_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_HCTRL_BANK0_UNION
 结构说明  : DP4_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x426，初值:0x11，宽度:8
 寄存器说明: dp4的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_hstop_bank0  : 4;  /* bit[0-3]: dp4传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp4_hstart_bank0 : 4;  /* bit[4-7]: dp4传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP4_HCTRL_BANK0_UNION;
#endif
#define MST_DP4_HCTRL_BANK0_dp4_hstop_bank0_START   (0)
#define MST_DP4_HCTRL_BANK0_dp4_hstop_bank0_END     (3)
#define MST_DP4_HCTRL_BANK0_dp4_hstart_bank0_START  (4)
#define MST_DP4_HCTRL_BANK0_dp4_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP4_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x427，初值:0x00，宽度:8
 寄存器说明: dp4 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_blockpackingmode_bank0 : 1;  /* bit[0]  : dp4 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP4_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP4_BLOCKCTRL3_BANK0_dp4_blockpackingmode_bank0_START  (0)
#define MST_DP4_BLOCKCTRL3_BANK0_dp4_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP4_LANECTRL_BANK0_UNION
 结构说明  : DP4_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x428，初值:0x00，宽度:8
 寄存器说明: dp4的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_lanecontrol_bank0 : 3;  /* bit[0-2]: dp4的数据线控制，表示dp4在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP4_LANECTRL_BANK0_UNION;
#endif
#define MST_DP4_LANECTRL_BANK0_dp4_lanecontrol_bank0_START  (0)
#define MST_DP4_LANECTRL_BANK0_dp4_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP4_CHANNELEN_BANK1_UNION
 结构说明  : DP4_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x430，初值:0x00，宽度:8
 寄存器说明: dp4的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_enable_channel1_bank1 : 1;  /* bit[0]: dp4的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel2_bank1 : 1;  /* bit[1]: dp4的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel3_bank1 : 1;  /* bit[2]: dp4的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel4_bank1 : 1;  /* bit[3]: dp4的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel5_bank1 : 1;  /* bit[4]: dp4的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel6_bank1 : 1;  /* bit[5]: dp4的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel7_bank1 : 1;  /* bit[6]: dp4的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp4_enable_channel8_bank1 : 1;  /* bit[7]: dp4的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP4_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel1_bank1_START  (0)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel1_bank1_END    (0)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel2_bank1_START  (1)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel2_bank1_END    (1)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel3_bank1_START  (2)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel3_bank1_END    (2)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel4_bank1_START  (3)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel4_bank1_END    (3)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel5_bank1_START  (4)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel5_bank1_END    (4)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel6_bank1_START  (5)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel6_bank1_END    (5)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel7_bank1_START  (6)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel7_bank1_END    (6)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel8_bank1_START  (7)
#define MST_DP4_CHANNELEN_BANK1_dp4_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP4_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x431，初值:0x00，宽度:8
 寄存器说明: dp4 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp4 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP4_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP4_BLOCKCTRL2_BANK1_dp4_blockgroupcontrol_bank1_START  (0)
#define MST_DP4_BLOCKCTRL2_BANK1_dp4_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP4_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP4_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x432，初值:0x00，宽度:8
 寄存器说明: dp4的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp4的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP4_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP4_SAMPLECTRL1_BANK1_dp4_sampleintervallow_bank1_START  (0)
#define MST_DP4_SAMPLECTRL1_BANK1_dp4_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP4_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x433，初值:0x00，宽度:8
 寄存器说明: dp4的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp4的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP4_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP4_SAMPLECTRL2_BANK1_dp4_sampleintervalhigh_bank1_START  (0)
#define MST_DP4_SAMPLECTRL2_BANK1_dp4_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP4_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x434，初值:0x00，宽度:8
 寄存器说明: dp4的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_offset1_bank1 : 8;  /* bit[0-7]: dp4的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP4_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP4_OFFSETCTRL1_BANK1_dp4_offset1_bank1_START  (0)
#define MST_DP4_OFFSETCTRL1_BANK1_dp4_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP4_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x435，初值:0x00，宽度:8
 寄存器说明: dp4的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_offset2_bank1 : 8;  /* bit[0-7]: dp4的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP4_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP4_OFFSETCTRL2_BANK1_dp4_offset2_bank1_START  (0)
#define MST_DP4_OFFSETCTRL2_BANK1_dp4_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_HCTRL_BANK1_UNION
 结构说明  : DP4_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x436，初值:0x11，宽度:8
 寄存器说明: dp4的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_hstop_bank1  : 4;  /* bit[0-3]: dp4传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp4_hstart_bank1 : 4;  /* bit[4-7]: dp4传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP4_HCTRL_BANK1_UNION;
#endif
#define MST_DP4_HCTRL_BANK1_dp4_hstop_bank1_START   (0)
#define MST_DP4_HCTRL_BANK1_dp4_hstop_bank1_END     (3)
#define MST_DP4_HCTRL_BANK1_dp4_hstart_bank1_START  (4)
#define MST_DP4_HCTRL_BANK1_dp4_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP4_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP4_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x437，初值:0x00，宽度:8
 寄存器说明: dp4 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_blockpackingmode_bank1 : 1;  /* bit[0]  : dp4 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP4_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP4_BLOCKCTRL3_BANK1_dp4_blockpackingmode_bank1_START  (0)
#define MST_DP4_BLOCKCTRL3_BANK1_dp4_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP4_LANECTRL_BANK1_UNION
 结构说明  : DP4_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x438，初值:0x00，宽度:8
 寄存器说明: dp4的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp4_lanecontrol_bank1 : 3;  /* bit[0-2]: dp4的数据线控制，表示dp4在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP4_LANECTRL_BANK1_UNION;
#endif
#define MST_DP4_LANECTRL_BANK1_dp4_lanecontrol_bank1_START  (0)
#define MST_DP4_LANECTRL_BANK1_dp4_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP5_INTSTAT_UNION
 结构说明  : DP5_INTSTAT 寄存器结构定义。地址偏移量:0x500，初值:0x00，宽度:8
 寄存器说明: dp5中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_test_fail_int  : 1;  /* bit[0]  : dp5 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp5_port_ready_int : 1;  /* bit[1]  : dp5 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp5_cfg_error0_int : 1;  /* bit[5]  : dp5自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp5_cfg_error1_int : 1;  /* bit[6]  : dp5自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp5_cfg_error2_int : 1;  /* bit[7]  : dp5自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP5_INTSTAT_UNION;
#endif
#define MST_DP5_INTSTAT_dp5_test_fail_int_START   (0)
#define MST_DP5_INTSTAT_dp5_test_fail_int_END     (0)
#define MST_DP5_INTSTAT_dp5_port_ready_int_START  (1)
#define MST_DP5_INTSTAT_dp5_port_ready_int_END    (1)
#define MST_DP5_INTSTAT_dp5_cfg_error0_int_START  (5)
#define MST_DP5_INTSTAT_dp5_cfg_error0_int_END    (5)
#define MST_DP5_INTSTAT_dp5_cfg_error1_int_START  (6)
#define MST_DP5_INTSTAT_dp5_cfg_error1_int_END    (6)
#define MST_DP5_INTSTAT_dp5_cfg_error2_int_START  (7)
#define MST_DP5_INTSTAT_dp5_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_INTMASK_UNION
 结构说明  : DP5_INTMASK 寄存器结构定义。地址偏移量:0x501，初值:0x00，宽度:8
 寄存器说明: dp5中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_test_fail_int_mask  : 1;  /* bit[0]  : dp5 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp5_port_ready_int_mask : 1;  /* bit[1]  : dp5 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp5_cfg_error0_int_mask : 1;  /* bit[5]  : dp5自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp5_cfg_error1_int_mask : 1;  /* bit[6]  : dp5自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp5_cfg_error2_int_mask : 1;  /* bit[7]  : dp5自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP5_INTMASK_UNION;
#endif
#define MST_DP5_INTMASK_dp5_test_fail_int_mask_START   (0)
#define MST_DP5_INTMASK_dp5_test_fail_int_mask_END     (0)
#define MST_DP5_INTMASK_dp5_port_ready_int_mask_START  (1)
#define MST_DP5_INTMASK_dp5_port_ready_int_mask_END    (1)
#define MST_DP5_INTMASK_dp5_cfg_error0_int_mask_START  (5)
#define MST_DP5_INTMASK_dp5_cfg_error0_int_mask_END    (5)
#define MST_DP5_INTMASK_dp5_cfg_error1_int_mask_START  (6)
#define MST_DP5_INTMASK_dp5_cfg_error1_int_mask_END    (6)
#define MST_DP5_INTMASK_dp5_cfg_error2_int_mask_START  (7)
#define MST_DP5_INTMASK_dp5_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_PORTCTRL_UNION
 结构说明  : DP5_PORTCTRL 寄存器结构定义。地址偏移量:0x502，初值:0x00，宽度:8
 寄存器说明: dp5的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_portflowmode   : 2;  /* bit[0-1]: dp5流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp5_portdatamode   : 2;  /* bit[2-3]: dp5 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp5_nextinvertbank : 1;  /* bit[4]  : dp5 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP5_PORTCTRL_UNION;
#endif
#define MST_DP5_PORTCTRL_dp5_portflowmode_START    (0)
#define MST_DP5_PORTCTRL_dp5_portflowmode_END      (1)
#define MST_DP5_PORTCTRL_dp5_portdatamode_START    (2)
#define MST_DP5_PORTCTRL_dp5_portdatamode_END      (3)
#define MST_DP5_PORTCTRL_dp5_nextinvertbank_START  (4)
#define MST_DP5_PORTCTRL_dp5_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP5_BLOCKCTRL1_UNION
 结构说明  : DP5_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x503，初值:0x1F，宽度:8
 寄存器说明: dp5 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_wordlength : 6;  /* bit[0-5]: dp5 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP5_BLOCKCTRL1_UNION;
#endif
#define MST_DP5_BLOCKCTRL1_dp5_wordlength_START  (0)
#define MST_DP5_BLOCKCTRL1_dp5_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP5_PREPARESTATUS_UNION
 结构说明  : DP5_PREPARESTATUS 寄存器结构定义。地址偏移量:0x504，初值:0x00，宽度:8
 寄存器说明: dp5的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_n_finished_channel1 : 1;  /* bit[0]: dp5通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel2 : 1;  /* bit[1]: dp5通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel3 : 1;  /* bit[2]: dp5通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel4 : 1;  /* bit[3]: dp5通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel5 : 1;  /* bit[4]: dp5通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel6 : 1;  /* bit[5]: dp5通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel7 : 1;  /* bit[6]: dp5通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp5_n_finished_channel8 : 1;  /* bit[7]: dp5通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP5_PREPARESTATUS_UNION;
#endif
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel1_START  (0)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel1_END    (0)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel2_START  (1)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel2_END    (1)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel3_START  (2)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel3_END    (2)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel4_START  (3)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel4_END    (3)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel5_START  (4)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel5_END    (4)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel6_START  (5)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel6_END    (5)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel7_START  (6)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel7_END    (6)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel8_START  (7)
#define MST_DP5_PREPARESTATUS_dp5_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_PREPARECTRL_UNION
 结构说明  : DP5_PREPARECTRL 寄存器结构定义。地址偏移量:0x505，初值:0x00，宽度:8
 寄存器说明: dp5的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_prepare_channel1 : 1;  /* bit[0]: dp5使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel2 : 1;  /* bit[1]: dp5使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel3 : 1;  /* bit[2]: dp5使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel4 : 1;  /* bit[3]: dp5使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel5 : 1;  /* bit[4]: dp5使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel6 : 1;  /* bit[5]: dp5使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel7 : 1;  /* bit[6]: dp5使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp5_prepare_channel8 : 1;  /* bit[7]: dp5使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP5_PREPARECTRL_UNION;
#endif
#define MST_DP5_PREPARECTRL_dp5_prepare_channel1_START  (0)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel1_END    (0)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel2_START  (1)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel2_END    (1)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel3_START  (2)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel3_END    (2)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel4_START  (3)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel4_END    (3)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel5_START  (4)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel5_END    (4)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel6_START  (5)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel6_END    (5)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel7_START  (6)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel7_END    (6)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel8_START  (7)
#define MST_DP5_PREPARECTRL_dp5_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_CHANNELEN_BANK0_UNION
 结构说明  : DP5_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x520，初值:0x00，宽度:8
 寄存器说明: dp5的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_enable_channel1_bank0 : 1;  /* bit[0]: dp5的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel2_bank0 : 1;  /* bit[1]: dp5的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel3_bank0 : 1;  /* bit[2]: dp5的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel4_bank0 : 1;  /* bit[3]: dp5的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel5_bank0 : 1;  /* bit[4]: dp5的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel6_bank0 : 1;  /* bit[5]: dp5的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel7_bank0 : 1;  /* bit[6]: dp5的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel8_bank0 : 1;  /* bit[7]: dp5的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP5_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel1_bank0_START  (0)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel1_bank0_END    (0)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel2_bank0_START  (1)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel2_bank0_END    (1)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel3_bank0_START  (2)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel3_bank0_END    (2)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel4_bank0_START  (3)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel4_bank0_END    (3)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel5_bank0_START  (4)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel5_bank0_END    (4)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel6_bank0_START  (5)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel6_bank0_END    (5)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel7_bank0_START  (6)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel7_bank0_END    (6)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel8_bank0_START  (7)
#define MST_DP5_CHANNELEN_BANK0_dp5_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP5_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x521，初值:0x00，宽度:8
 寄存器说明: dp5 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp5 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP5_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP5_BLOCKCTRL2_BANK0_dp5_blockgroupcontrol_bank0_START  (0)
#define MST_DP5_BLOCKCTRL2_BANK0_dp5_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP5_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP5_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x522，初值:0x00，宽度:8
 寄存器说明: dp5的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp5的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP5_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP5_SAMPLECTRL1_BANK0_dp5_sampleintervallow_bank0_START  (0)
#define MST_DP5_SAMPLECTRL1_BANK0_dp5_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP5_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x523，初值:0x00，宽度:8
 寄存器说明: dp5的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp5的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP5_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP5_SAMPLECTRL2_BANK0_dp5_sampleintervalhigh_bank0_START  (0)
#define MST_DP5_SAMPLECTRL2_BANK0_dp5_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP5_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x524，初值:0x00，宽度:8
 寄存器说明: dp5的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_offset1_bank0 : 8;  /* bit[0-7]: dp5的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP5_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP5_OFFSETCTRL1_BANK0_dp5_offset1_bank0_START  (0)
#define MST_DP5_OFFSETCTRL1_BANK0_dp5_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP5_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x525，初值:0x00，宽度:8
 寄存器说明: dp5的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_offset2_bank0 : 8;  /* bit[0-7]: dp5的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP5_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP5_OFFSETCTRL2_BANK0_dp5_offset2_bank0_START  (0)
#define MST_DP5_OFFSETCTRL2_BANK0_dp5_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_HCTRL_BANK0_UNION
 结构说明  : DP5_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x526，初值:0x11，宽度:8
 寄存器说明: dp5的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_hstop_bank0  : 4;  /* bit[0-3]: dp5传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp5_hstart_bank0 : 4;  /* bit[4-7]: dp5传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP5_HCTRL_BANK0_UNION;
#endif
#define MST_DP5_HCTRL_BANK0_dp5_hstop_bank0_START   (0)
#define MST_DP5_HCTRL_BANK0_dp5_hstop_bank0_END     (3)
#define MST_DP5_HCTRL_BANK0_dp5_hstart_bank0_START  (4)
#define MST_DP5_HCTRL_BANK0_dp5_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP5_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x527，初值:0x00，宽度:8
 寄存器说明: dp5 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_blockpackingmode_bank0 : 1;  /* bit[0]  : dp5 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP5_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP5_BLOCKCTRL3_BANK0_dp5_blockpackingmode_bank0_START  (0)
#define MST_DP5_BLOCKCTRL3_BANK0_dp5_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP5_LANECTRL_BANK0_UNION
 结构说明  : DP5_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x528，初值:0x00，宽度:8
 寄存器说明: dp5的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_lanecontrol_bank0 : 3;  /* bit[0-2]: dp5的数据线控制，表示dp5在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP5_LANECTRL_BANK0_UNION;
#endif
#define MST_DP5_LANECTRL_BANK0_dp5_lanecontrol_bank0_START  (0)
#define MST_DP5_LANECTRL_BANK0_dp5_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP5_CHANNELEN_BANK1_UNION
 结构说明  : DP5_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x530，初值:0x00，宽度:8
 寄存器说明: dp5的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_enable_channel1_bank1 : 1;  /* bit[0]: dp5的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel2_bank1 : 1;  /* bit[1]: dp5的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel3_bank1 : 1;  /* bit[2]: dp5的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel4_bank1 : 1;  /* bit[3]: dp5的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel5_bank1 : 1;  /* bit[4]: dp5的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel6_bank1 : 1;  /* bit[5]: dp5的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel7_bank1 : 1;  /* bit[6]: dp5的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp5_enable_channel8_bank1 : 1;  /* bit[7]: dp5的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP5_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel1_bank1_START  (0)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel1_bank1_END    (0)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel2_bank1_START  (1)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel2_bank1_END    (1)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel3_bank1_START  (2)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel3_bank1_END    (2)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel4_bank1_START  (3)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel4_bank1_END    (3)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel5_bank1_START  (4)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel5_bank1_END    (4)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel6_bank1_START  (5)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel6_bank1_END    (5)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel7_bank1_START  (6)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel7_bank1_END    (6)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel8_bank1_START  (7)
#define MST_DP5_CHANNELEN_BANK1_dp5_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP5_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x531，初值:0x00，宽度:8
 寄存器说明: dp5 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp5 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP5_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP5_BLOCKCTRL2_BANK1_dp5_blockgroupcontrol_bank1_START  (0)
#define MST_DP5_BLOCKCTRL2_BANK1_dp5_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP5_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP5_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x532，初值:0x00，宽度:8
 寄存器说明: dp5的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp5的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP5_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP5_SAMPLECTRL1_BANK1_dp5_sampleintervallow_bank1_START  (0)
#define MST_DP5_SAMPLECTRL1_BANK1_dp5_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP5_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x533，初值:0x00，宽度:8
 寄存器说明: dp5的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp5的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP5_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP5_SAMPLECTRL2_BANK1_dp5_sampleintervalhigh_bank1_START  (0)
#define MST_DP5_SAMPLECTRL2_BANK1_dp5_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP5_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x534，初值:0x00，宽度:8
 寄存器说明: dp5的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_offset1_bank1 : 8;  /* bit[0-7]: dp5的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP5_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP5_OFFSETCTRL1_BANK1_dp5_offset1_bank1_START  (0)
#define MST_DP5_OFFSETCTRL1_BANK1_dp5_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP5_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x535，初值:0x00，宽度:8
 寄存器说明: dp5的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_offset2_bank1 : 8;  /* bit[0-7]: dp5的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP5_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP5_OFFSETCTRL2_BANK1_dp5_offset2_bank1_START  (0)
#define MST_DP5_OFFSETCTRL2_BANK1_dp5_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_HCTRL_BANK1_UNION
 结构说明  : DP5_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x536，初值:0x11，宽度:8
 寄存器说明: dp5的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_hstop_bank1  : 4;  /* bit[0-3]: dp5传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp5_hstart_bank1 : 4;  /* bit[4-7]: dp5传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP5_HCTRL_BANK1_UNION;
#endif
#define MST_DP5_HCTRL_BANK1_dp5_hstop_bank1_START   (0)
#define MST_DP5_HCTRL_BANK1_dp5_hstop_bank1_END     (3)
#define MST_DP5_HCTRL_BANK1_dp5_hstart_bank1_START  (4)
#define MST_DP5_HCTRL_BANK1_dp5_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP5_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP5_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x537，初值:0x00，宽度:8
 寄存器说明: dp5 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_blockpackingmode_bank1 : 1;  /* bit[0]  : dp5 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP5_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP5_BLOCKCTRL3_BANK1_dp5_blockpackingmode_bank1_START  (0)
#define MST_DP5_BLOCKCTRL3_BANK1_dp5_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP5_LANECTRL_BANK1_UNION
 结构说明  : DP5_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x538，初值:0x00，宽度:8
 寄存器说明: dp5的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp5_lanecontrol_bank1 : 3;  /* bit[0-2]: dp5的数据线控制，表示dp5在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP5_LANECTRL_BANK1_UNION;
#endif
#define MST_DP5_LANECTRL_BANK1_dp5_lanecontrol_bank1_START  (0)
#define MST_DP5_LANECTRL_BANK1_dp5_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP6_INTSTAT_UNION
 结构说明  : DP6_INTSTAT 寄存器结构定义。地址偏移量:0x600，初值:0x00，宽度:8
 寄存器说明: dp6中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_test_fail_int  : 1;  /* bit[0]  : dp6 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp6_port_ready_int : 1;  /* bit[1]  : dp6 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp6_cfg_error0_int : 1;  /* bit[5]  : dp6自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp6_cfg_error1_int : 1;  /* bit[6]  : dp6自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp6_cfg_error2_int : 1;  /* bit[7]  : dp6自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP6_INTSTAT_UNION;
#endif
#define MST_DP6_INTSTAT_dp6_test_fail_int_START   (0)
#define MST_DP6_INTSTAT_dp6_test_fail_int_END     (0)
#define MST_DP6_INTSTAT_dp6_port_ready_int_START  (1)
#define MST_DP6_INTSTAT_dp6_port_ready_int_END    (1)
#define MST_DP6_INTSTAT_dp6_cfg_error0_int_START  (5)
#define MST_DP6_INTSTAT_dp6_cfg_error0_int_END    (5)
#define MST_DP6_INTSTAT_dp6_cfg_error1_int_START  (6)
#define MST_DP6_INTSTAT_dp6_cfg_error1_int_END    (6)
#define MST_DP6_INTSTAT_dp6_cfg_error2_int_START  (7)
#define MST_DP6_INTSTAT_dp6_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_INTMASK_UNION
 结构说明  : DP6_INTMASK 寄存器结构定义。地址偏移量:0x601，初值:0x00，宽度:8
 寄存器说明: dp6中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_test_fail_int_mask  : 1;  /* bit[0]  : dp6 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp6_port_ready_int_mask : 1;  /* bit[1]  : dp6 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp6_cfg_error0_int_mask : 1;  /* bit[5]  : dp6自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp6_cfg_error1_int_mask : 1;  /* bit[6]  : dp6自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp6_cfg_error2_int_mask : 1;  /* bit[7]  : dp6自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP6_INTMASK_UNION;
#endif
#define MST_DP6_INTMASK_dp6_test_fail_int_mask_START   (0)
#define MST_DP6_INTMASK_dp6_test_fail_int_mask_END     (0)
#define MST_DP6_INTMASK_dp6_port_ready_int_mask_START  (1)
#define MST_DP6_INTMASK_dp6_port_ready_int_mask_END    (1)
#define MST_DP6_INTMASK_dp6_cfg_error0_int_mask_START  (5)
#define MST_DP6_INTMASK_dp6_cfg_error0_int_mask_END    (5)
#define MST_DP6_INTMASK_dp6_cfg_error1_int_mask_START  (6)
#define MST_DP6_INTMASK_dp6_cfg_error1_int_mask_END    (6)
#define MST_DP6_INTMASK_dp6_cfg_error2_int_mask_START  (7)
#define MST_DP6_INTMASK_dp6_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_PORTCTRL_UNION
 结构说明  : DP6_PORTCTRL 寄存器结构定义。地址偏移量:0x602，初值:0x00，宽度:8
 寄存器说明: dp6的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_portflowmode   : 2;  /* bit[0-1]: dp6流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp6_portdatamode   : 2;  /* bit[2-3]: dp6 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp6_nextinvertbank : 1;  /* bit[4]  : dp6 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP6_PORTCTRL_UNION;
#endif
#define MST_DP6_PORTCTRL_dp6_portflowmode_START    (0)
#define MST_DP6_PORTCTRL_dp6_portflowmode_END      (1)
#define MST_DP6_PORTCTRL_dp6_portdatamode_START    (2)
#define MST_DP6_PORTCTRL_dp6_portdatamode_END      (3)
#define MST_DP6_PORTCTRL_dp6_nextinvertbank_START  (4)
#define MST_DP6_PORTCTRL_dp6_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP6_BLOCKCTRL1_UNION
 结构说明  : DP6_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x603，初值:0x1F，宽度:8
 寄存器说明: dp6 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_wordlength : 6;  /* bit[0-5]: dp6 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP6_BLOCKCTRL1_UNION;
#endif
#define MST_DP6_BLOCKCTRL1_dp6_wordlength_START  (0)
#define MST_DP6_BLOCKCTRL1_dp6_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP6_PREPARESTATUS_UNION
 结构说明  : DP6_PREPARESTATUS 寄存器结构定义。地址偏移量:0x604，初值:0x00，宽度:8
 寄存器说明: dp6的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_n_finished_channel1 : 1;  /* bit[0]: dp6通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel2 : 1;  /* bit[1]: dp6通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel3 : 1;  /* bit[2]: dp6通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel4 : 1;  /* bit[3]: dp6通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel5 : 1;  /* bit[4]: dp6通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel6 : 1;  /* bit[5]: dp6通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel7 : 1;  /* bit[6]: dp6通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp6_n_finished_channel8 : 1;  /* bit[7]: dp6通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP6_PREPARESTATUS_UNION;
#endif
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel1_START  (0)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel1_END    (0)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel2_START  (1)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel2_END    (1)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel3_START  (2)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel3_END    (2)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel4_START  (3)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel4_END    (3)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel5_START  (4)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel5_END    (4)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel6_START  (5)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel6_END    (5)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel7_START  (6)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel7_END    (6)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel8_START  (7)
#define MST_DP6_PREPARESTATUS_dp6_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_PREPARECTRL_UNION
 结构说明  : DP6_PREPARECTRL 寄存器结构定义。地址偏移量:0x605，初值:0x00，宽度:8
 寄存器说明: dp6的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_prepare_channel1 : 1;  /* bit[0]: dp6使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel2 : 1;  /* bit[1]: dp6使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel3 : 1;  /* bit[2]: dp6使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel4 : 1;  /* bit[3]: dp6使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel5 : 1;  /* bit[4]: dp6使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel6 : 1;  /* bit[5]: dp6使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel7 : 1;  /* bit[6]: dp6使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp6_prepare_channel8 : 1;  /* bit[7]: dp6使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP6_PREPARECTRL_UNION;
#endif
#define MST_DP6_PREPARECTRL_dp6_prepare_channel1_START  (0)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel1_END    (0)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel2_START  (1)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel2_END    (1)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel3_START  (2)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel3_END    (2)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel4_START  (3)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel4_END    (3)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel5_START  (4)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel5_END    (4)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel6_START  (5)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel6_END    (5)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel7_START  (6)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel7_END    (6)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel8_START  (7)
#define MST_DP6_PREPARECTRL_dp6_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_CHANNELEN_BANK0_UNION
 结构说明  : DP6_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x620，初值:0x00，宽度:8
 寄存器说明: dp6的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_enable_channel1_bank0 : 1;  /* bit[0]: dp6的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel2_bank0 : 1;  /* bit[1]: dp6的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel3_bank0 : 1;  /* bit[2]: dp6的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel4_bank0 : 1;  /* bit[3]: dp6的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel5_bank0 : 1;  /* bit[4]: dp6的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel6_bank0 : 1;  /* bit[5]: dp6的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel7_bank0 : 1;  /* bit[6]: dp6的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel8_bank0 : 1;  /* bit[7]: dp6的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP6_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel1_bank0_START  (0)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel1_bank0_END    (0)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel2_bank0_START  (1)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel2_bank0_END    (1)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel3_bank0_START  (2)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel3_bank0_END    (2)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel4_bank0_START  (3)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel4_bank0_END    (3)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel5_bank0_START  (4)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel5_bank0_END    (4)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel6_bank0_START  (5)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel6_bank0_END    (5)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel7_bank0_START  (6)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel7_bank0_END    (6)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel8_bank0_START  (7)
#define MST_DP6_CHANNELEN_BANK0_dp6_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP6_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x621，初值:0x00，宽度:8
 寄存器说明: dp6 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp6 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP6_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP6_BLOCKCTRL2_BANK0_dp6_blockgroupcontrol_bank0_START  (0)
#define MST_DP6_BLOCKCTRL2_BANK0_dp6_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP6_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP6_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x622，初值:0x00，宽度:8
 寄存器说明: dp6的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp6的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP6_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP6_SAMPLECTRL1_BANK0_dp6_sampleintervallow_bank0_START  (0)
#define MST_DP6_SAMPLECTRL1_BANK0_dp6_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP6_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x623，初值:0x00，宽度:8
 寄存器说明: dp6的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp6的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP6_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP6_SAMPLECTRL2_BANK0_dp6_sampleintervalhigh_bank0_START  (0)
#define MST_DP6_SAMPLECTRL2_BANK0_dp6_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP6_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x624，初值:0x00，宽度:8
 寄存器说明: dp6的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_offset1_bank0 : 8;  /* bit[0-7]: dp6的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP6_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP6_OFFSETCTRL1_BANK0_dp6_offset1_bank0_START  (0)
#define MST_DP6_OFFSETCTRL1_BANK0_dp6_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP6_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x625，初值:0x00，宽度:8
 寄存器说明: dp6的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_offset2_bank0 : 8;  /* bit[0-7]: dp6的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP6_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP6_OFFSETCTRL2_BANK0_dp6_offset2_bank0_START  (0)
#define MST_DP6_OFFSETCTRL2_BANK0_dp6_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_HCTRL_BANK0_UNION
 结构说明  : DP6_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x626，初值:0x11，宽度:8
 寄存器说明: dp6的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_hstop_bank0  : 4;  /* bit[0-3]: dp6传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp6_hstart_bank0 : 4;  /* bit[4-7]: dp6传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP6_HCTRL_BANK0_UNION;
#endif
#define MST_DP6_HCTRL_BANK0_dp6_hstop_bank0_START   (0)
#define MST_DP6_HCTRL_BANK0_dp6_hstop_bank0_END     (3)
#define MST_DP6_HCTRL_BANK0_dp6_hstart_bank0_START  (4)
#define MST_DP6_HCTRL_BANK0_dp6_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP6_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x627，初值:0x00，宽度:8
 寄存器说明: dp6 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_blockpackingmode_bank0 : 1;  /* bit[0]  : dp6 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP6_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP6_BLOCKCTRL3_BANK0_dp6_blockpackingmode_bank0_START  (0)
#define MST_DP6_BLOCKCTRL3_BANK0_dp6_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP6_LANECTRL_BANK0_UNION
 结构说明  : DP6_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x628，初值:0x00，宽度:8
 寄存器说明: dp6的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_lanecontrol_bank0 : 3;  /* bit[0-2]: dp6的数据线控制，表示dp6在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP6_LANECTRL_BANK0_UNION;
#endif
#define MST_DP6_LANECTRL_BANK0_dp6_lanecontrol_bank0_START  (0)
#define MST_DP6_LANECTRL_BANK0_dp6_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP6_CHANNELEN_BANK1_UNION
 结构说明  : DP6_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x630，初值:0x00，宽度:8
 寄存器说明: dp6的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_enable_channel1_bank1 : 1;  /* bit[0]: dp6的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel2_bank1 : 1;  /* bit[1]: dp6的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel3_bank1 : 1;  /* bit[2]: dp6的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel4_bank1 : 1;  /* bit[3]: dp6的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel5_bank1 : 1;  /* bit[4]: dp6的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel6_bank1 : 1;  /* bit[5]: dp6的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel7_bank1 : 1;  /* bit[6]: dp6的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp6_enable_channel8_bank1 : 1;  /* bit[7]: dp6的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP6_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel1_bank1_START  (0)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel1_bank1_END    (0)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel2_bank1_START  (1)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel2_bank1_END    (1)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel3_bank1_START  (2)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel3_bank1_END    (2)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel4_bank1_START  (3)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel4_bank1_END    (3)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel5_bank1_START  (4)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel5_bank1_END    (4)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel6_bank1_START  (5)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel6_bank1_END    (5)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel7_bank1_START  (6)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel7_bank1_END    (6)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel8_bank1_START  (7)
#define MST_DP6_CHANNELEN_BANK1_dp6_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP6_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x631，初值:0x00，宽度:8
 寄存器说明: dp6 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp6 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP6_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP6_BLOCKCTRL2_BANK1_dp6_blockgroupcontrol_bank1_START  (0)
#define MST_DP6_BLOCKCTRL2_BANK1_dp6_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP6_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP6_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x632，初值:0x00，宽度:8
 寄存器说明: dp6的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp6的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP6_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP6_SAMPLECTRL1_BANK1_dp6_sampleintervallow_bank1_START  (0)
#define MST_DP6_SAMPLECTRL1_BANK1_dp6_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP6_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x633，初值:0x00，宽度:8
 寄存器说明: dp6的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp6的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP6_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP6_SAMPLECTRL2_BANK1_dp6_sampleintervalhigh_bank1_START  (0)
#define MST_DP6_SAMPLECTRL2_BANK1_dp6_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP6_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x634，初值:0x00，宽度:8
 寄存器说明: dp6的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_offset1_bank1 : 8;  /* bit[0-7]: dp6的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP6_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP6_OFFSETCTRL1_BANK1_dp6_offset1_bank1_START  (0)
#define MST_DP6_OFFSETCTRL1_BANK1_dp6_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP6_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x635，初值:0x00，宽度:8
 寄存器说明: dp6的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_offset2_bank1 : 8;  /* bit[0-7]: dp6的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP6_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP6_OFFSETCTRL2_BANK1_dp6_offset2_bank1_START  (0)
#define MST_DP6_OFFSETCTRL2_BANK1_dp6_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_HCTRL_BANK1_UNION
 结构说明  : DP6_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x636，初值:0x11，宽度:8
 寄存器说明: dp6的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_hstop_bank1  : 4;  /* bit[0-3]: dp6传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp6_hstart_bank1 : 4;  /* bit[4-7]: dp6传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP6_HCTRL_BANK1_UNION;
#endif
#define MST_DP6_HCTRL_BANK1_dp6_hstop_bank1_START   (0)
#define MST_DP6_HCTRL_BANK1_dp6_hstop_bank1_END     (3)
#define MST_DP6_HCTRL_BANK1_dp6_hstart_bank1_START  (4)
#define MST_DP6_HCTRL_BANK1_dp6_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP6_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP6_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x637，初值:0x00，宽度:8
 寄存器说明: dp6 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_blockpackingmode_bank1 : 1;  /* bit[0]  : dp6 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP6_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP6_BLOCKCTRL3_BANK1_dp6_blockpackingmode_bank1_START  (0)
#define MST_DP6_BLOCKCTRL3_BANK1_dp6_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP6_LANECTRL_BANK1_UNION
 结构说明  : DP6_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x638，初值:0x00，宽度:8
 寄存器说明: dp6的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp6_lanecontrol_bank1 : 3;  /* bit[0-2]: dp6的数据线控制，表示dp6在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP6_LANECTRL_BANK1_UNION;
#endif
#define MST_DP6_LANECTRL_BANK1_dp6_lanecontrol_bank1_START  (0)
#define MST_DP6_LANECTRL_BANK1_dp6_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP7_INTSTAT_UNION
 结构说明  : DP7_INTSTAT 寄存器结构定义。地址偏移量:0x700，初值:0x00，宽度:8
 寄存器说明: dp7中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_test_fail_int  : 1;  /* bit[0]  : dp7 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp7_port_ready_int : 1;  /* bit[1]  : dp7 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp7_cfg_error0_int : 1;  /* bit[5]  : dp7自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp7_cfg_error1_int : 1;  /* bit[6]  : dp7自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp7_cfg_error2_int : 1;  /* bit[7]  : dp7自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP7_INTSTAT_UNION;
#endif
#define MST_DP7_INTSTAT_dp7_test_fail_int_START   (0)
#define MST_DP7_INTSTAT_dp7_test_fail_int_END     (0)
#define MST_DP7_INTSTAT_dp7_port_ready_int_START  (1)
#define MST_DP7_INTSTAT_dp7_port_ready_int_END    (1)
#define MST_DP7_INTSTAT_dp7_cfg_error0_int_START  (5)
#define MST_DP7_INTSTAT_dp7_cfg_error0_int_END    (5)
#define MST_DP7_INTSTAT_dp7_cfg_error1_int_START  (6)
#define MST_DP7_INTSTAT_dp7_cfg_error1_int_END    (6)
#define MST_DP7_INTSTAT_dp7_cfg_error2_int_START  (7)
#define MST_DP7_INTSTAT_dp7_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_INTMASK_UNION
 结构说明  : DP7_INTMASK 寄存器结构定义。地址偏移量:0x701，初值:0x00，宽度:8
 寄存器说明: dp7中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_test_fail_int_mask  : 1;  /* bit[0]  : dp7 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp7_port_ready_int_mask : 1;  /* bit[1]  : dp7 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp7_cfg_error0_int_mask : 1;  /* bit[5]  : dp7自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp7_cfg_error1_int_mask : 1;  /* bit[6]  : dp7自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp7_cfg_error2_int_mask : 1;  /* bit[7]  : dp7自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP7_INTMASK_UNION;
#endif
#define MST_DP7_INTMASK_dp7_test_fail_int_mask_START   (0)
#define MST_DP7_INTMASK_dp7_test_fail_int_mask_END     (0)
#define MST_DP7_INTMASK_dp7_port_ready_int_mask_START  (1)
#define MST_DP7_INTMASK_dp7_port_ready_int_mask_END    (1)
#define MST_DP7_INTMASK_dp7_cfg_error0_int_mask_START  (5)
#define MST_DP7_INTMASK_dp7_cfg_error0_int_mask_END    (5)
#define MST_DP7_INTMASK_dp7_cfg_error1_int_mask_START  (6)
#define MST_DP7_INTMASK_dp7_cfg_error1_int_mask_END    (6)
#define MST_DP7_INTMASK_dp7_cfg_error2_int_mask_START  (7)
#define MST_DP7_INTMASK_dp7_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_PORTCTRL_UNION
 结构说明  : DP7_PORTCTRL 寄存器结构定义。地址偏移量:0x702，初值:0x00，宽度:8
 寄存器说明: dp7的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_portflowmode   : 2;  /* bit[0-1]: dp7流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp7_portdatamode   : 2;  /* bit[2-3]: dp7 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp7_nextinvertbank : 1;  /* bit[4]  : dp7 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP7_PORTCTRL_UNION;
#endif
#define MST_DP7_PORTCTRL_dp7_portflowmode_START    (0)
#define MST_DP7_PORTCTRL_dp7_portflowmode_END      (1)
#define MST_DP7_PORTCTRL_dp7_portdatamode_START    (2)
#define MST_DP7_PORTCTRL_dp7_portdatamode_END      (3)
#define MST_DP7_PORTCTRL_dp7_nextinvertbank_START  (4)
#define MST_DP7_PORTCTRL_dp7_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP7_BLOCKCTRL1_UNION
 结构说明  : DP7_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x703，初值:0x1F，宽度:8
 寄存器说明: dp7 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_wordlength : 6;  /* bit[0-5]: dp7 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP7_BLOCKCTRL1_UNION;
#endif
#define MST_DP7_BLOCKCTRL1_dp7_wordlength_START  (0)
#define MST_DP7_BLOCKCTRL1_dp7_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP7_PREPARESTATUS_UNION
 结构说明  : DP7_PREPARESTATUS 寄存器结构定义。地址偏移量:0x704，初值:0x00，宽度:8
 寄存器说明: dp7的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_n_finished_channel1 : 1;  /* bit[0]: dp7通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel2 : 1;  /* bit[1]: dp7通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel3 : 1;  /* bit[2]: dp7通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel4 : 1;  /* bit[3]: dp7通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel5 : 1;  /* bit[4]: dp7通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel6 : 1;  /* bit[5]: dp7通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel7 : 1;  /* bit[6]: dp7通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp7_n_finished_channel8 : 1;  /* bit[7]: dp7通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP7_PREPARESTATUS_UNION;
#endif
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel1_START  (0)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel1_END    (0)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel2_START  (1)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel2_END    (1)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel3_START  (2)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel3_END    (2)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel4_START  (3)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel4_END    (3)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel5_START  (4)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel5_END    (4)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel6_START  (5)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel6_END    (5)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel7_START  (6)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel7_END    (6)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel8_START  (7)
#define MST_DP7_PREPARESTATUS_dp7_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_PREPARECTRL_UNION
 结构说明  : DP7_PREPARECTRL 寄存器结构定义。地址偏移量:0x705，初值:0x00，宽度:8
 寄存器说明: dp7的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_prepare_channel1 : 1;  /* bit[0]: dp7使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel2 : 1;  /* bit[1]: dp7使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel3 : 1;  /* bit[2]: dp7使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel4 : 1;  /* bit[3]: dp7使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel5 : 1;  /* bit[4]: dp7使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel6 : 1;  /* bit[5]: dp7使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel7 : 1;  /* bit[6]: dp7使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp7_prepare_channel8 : 1;  /* bit[7]: dp7使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP7_PREPARECTRL_UNION;
#endif
#define MST_DP7_PREPARECTRL_dp7_prepare_channel1_START  (0)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel1_END    (0)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel2_START  (1)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel2_END    (1)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel3_START  (2)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel3_END    (2)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel4_START  (3)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel4_END    (3)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel5_START  (4)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel5_END    (4)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel6_START  (5)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel6_END    (5)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel7_START  (6)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel7_END    (6)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel8_START  (7)
#define MST_DP7_PREPARECTRL_dp7_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_CHANNELEN_BANK0_UNION
 结构说明  : DP7_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x720，初值:0x00，宽度:8
 寄存器说明: dp7的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_enable_channel1_bank0 : 1;  /* bit[0]: dp7的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel2_bank0 : 1;  /* bit[1]: dp7的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel3_bank0 : 1;  /* bit[2]: dp7的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel4_bank0 : 1;  /* bit[3]: dp7的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel5_bank0 : 1;  /* bit[4]: dp7的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel6_bank0 : 1;  /* bit[5]: dp7的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel7_bank0 : 1;  /* bit[6]: dp7的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel8_bank0 : 1;  /* bit[7]: dp7的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP7_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel1_bank0_START  (0)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel1_bank0_END    (0)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel2_bank0_START  (1)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel2_bank0_END    (1)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel3_bank0_START  (2)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel3_bank0_END    (2)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel4_bank0_START  (3)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel4_bank0_END    (3)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel5_bank0_START  (4)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel5_bank0_END    (4)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel6_bank0_START  (5)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel6_bank0_END    (5)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel7_bank0_START  (6)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel7_bank0_END    (6)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel8_bank0_START  (7)
#define MST_DP7_CHANNELEN_BANK0_dp7_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP7_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x721，初值:0x00，宽度:8
 寄存器说明: dp7 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp7 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP7_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP7_BLOCKCTRL2_BANK0_dp7_blockgroupcontrol_bank0_START  (0)
#define MST_DP7_BLOCKCTRL2_BANK0_dp7_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP7_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP7_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x722，初值:0x00，宽度:8
 寄存器说明: dp7的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp7的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP7_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP7_SAMPLECTRL1_BANK0_dp7_sampleintervallow_bank0_START  (0)
#define MST_DP7_SAMPLECTRL1_BANK0_dp7_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP7_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x723，初值:0x00，宽度:8
 寄存器说明: dp7的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp7的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP7_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP7_SAMPLECTRL2_BANK0_dp7_sampleintervalhigh_bank0_START  (0)
#define MST_DP7_SAMPLECTRL2_BANK0_dp7_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP7_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x724，初值:0x00，宽度:8
 寄存器说明: dp7的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_offset1_bank0 : 8;  /* bit[0-7]: dp7的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP7_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP7_OFFSETCTRL1_BANK0_dp7_offset1_bank0_START  (0)
#define MST_DP7_OFFSETCTRL1_BANK0_dp7_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP7_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x725，初值:0x00，宽度:8
 寄存器说明: dp7的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_offset2_bank0 : 8;  /* bit[0-7]: dp7的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP7_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP7_OFFSETCTRL2_BANK0_dp7_offset2_bank0_START  (0)
#define MST_DP7_OFFSETCTRL2_BANK0_dp7_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_HCTRL_BANK0_UNION
 结构说明  : DP7_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x726，初值:0x11，宽度:8
 寄存器说明: dp7的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_hstop_bank0  : 4;  /* bit[0-3]: dp7传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp7_hstart_bank0 : 4;  /* bit[4-7]: dp7传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP7_HCTRL_BANK0_UNION;
#endif
#define MST_DP7_HCTRL_BANK0_dp7_hstop_bank0_START   (0)
#define MST_DP7_HCTRL_BANK0_dp7_hstop_bank0_END     (3)
#define MST_DP7_HCTRL_BANK0_dp7_hstart_bank0_START  (4)
#define MST_DP7_HCTRL_BANK0_dp7_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP7_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x727，初值:0x00，宽度:8
 寄存器说明: dp7 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_blockpackingmode_bank0 : 1;  /* bit[0]  : dp7 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP7_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP7_BLOCKCTRL3_BANK0_dp7_blockpackingmode_bank0_START  (0)
#define MST_DP7_BLOCKCTRL3_BANK0_dp7_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP7_LANECTRL_BANK0_UNION
 结构说明  : DP7_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x728，初值:0x00，宽度:8
 寄存器说明: dp7的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_lanecontrol_bank0 : 3;  /* bit[0-2]: dp7的数据线控制，表示dp7在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP7_LANECTRL_BANK0_UNION;
#endif
#define MST_DP7_LANECTRL_BANK0_dp7_lanecontrol_bank0_START  (0)
#define MST_DP7_LANECTRL_BANK0_dp7_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP7_CHANNELEN_BANK1_UNION
 结构说明  : DP7_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x730，初值:0x00，宽度:8
 寄存器说明: dp7的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_enable_channel1_bank1 : 1;  /* bit[0]: dp7的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel2_bank1 : 1;  /* bit[1]: dp7的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel3_bank1 : 1;  /* bit[2]: dp7的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel4_bank1 : 1;  /* bit[3]: dp7的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel5_bank1 : 1;  /* bit[4]: dp7的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel6_bank1 : 1;  /* bit[5]: dp7的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel7_bank1 : 1;  /* bit[6]: dp7的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp7_enable_channel8_bank1 : 1;  /* bit[7]: dp7的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP7_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel1_bank1_START  (0)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel1_bank1_END    (0)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel2_bank1_START  (1)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel2_bank1_END    (1)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel3_bank1_START  (2)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel3_bank1_END    (2)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel4_bank1_START  (3)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel4_bank1_END    (3)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel5_bank1_START  (4)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel5_bank1_END    (4)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel6_bank1_START  (5)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel6_bank1_END    (5)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel7_bank1_START  (6)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel7_bank1_END    (6)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel8_bank1_START  (7)
#define MST_DP7_CHANNELEN_BANK1_dp7_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP7_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x731，初值:0x00，宽度:8
 寄存器说明: dp7 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp7 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP7_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP7_BLOCKCTRL2_BANK1_dp7_blockgroupcontrol_bank1_START  (0)
#define MST_DP7_BLOCKCTRL2_BANK1_dp7_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP7_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP7_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x732，初值:0x00，宽度:8
 寄存器说明: dp7的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp7的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP7_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP7_SAMPLECTRL1_BANK1_dp7_sampleintervallow_bank1_START  (0)
#define MST_DP7_SAMPLECTRL1_BANK1_dp7_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP7_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x733，初值:0x00，宽度:8
 寄存器说明: dp7的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp7的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP7_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP7_SAMPLECTRL2_BANK1_dp7_sampleintervalhigh_bank1_START  (0)
#define MST_DP7_SAMPLECTRL2_BANK1_dp7_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP7_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x734，初值:0x00，宽度:8
 寄存器说明: dp7的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_offset1_bank1 : 8;  /* bit[0-7]: dp7的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP7_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP7_OFFSETCTRL1_BANK1_dp7_offset1_bank1_START  (0)
#define MST_DP7_OFFSETCTRL1_BANK1_dp7_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP7_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x735，初值:0x00，宽度:8
 寄存器说明: dp7的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_offset2_bank1 : 8;  /* bit[0-7]: dp7的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP7_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP7_OFFSETCTRL2_BANK1_dp7_offset2_bank1_START  (0)
#define MST_DP7_OFFSETCTRL2_BANK1_dp7_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_HCTRL_BANK1_UNION
 结构说明  : DP7_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x736，初值:0x11，宽度:8
 寄存器说明: dp7的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_hstop_bank1  : 4;  /* bit[0-3]: dp7传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp7_hstart_bank1 : 4;  /* bit[4-7]: dp7传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP7_HCTRL_BANK1_UNION;
#endif
#define MST_DP7_HCTRL_BANK1_dp7_hstop_bank1_START   (0)
#define MST_DP7_HCTRL_BANK1_dp7_hstop_bank1_END     (3)
#define MST_DP7_HCTRL_BANK1_dp7_hstart_bank1_START  (4)
#define MST_DP7_HCTRL_BANK1_dp7_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP7_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP7_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x737，初值:0x00，宽度:8
 寄存器说明: dp7 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_blockpackingmode_bank1 : 1;  /* bit[0]  : dp7 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP7_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP7_BLOCKCTRL3_BANK1_dp7_blockpackingmode_bank1_START  (0)
#define MST_DP7_BLOCKCTRL3_BANK1_dp7_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP7_LANECTRL_BANK1_UNION
 结构说明  : DP7_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x738，初值:0x00，宽度:8
 寄存器说明: dp7的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp7_lanecontrol_bank1 : 3;  /* bit[0-2]: dp7的数据线控制，表示dp7在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP7_LANECTRL_BANK1_UNION;
#endif
#define MST_DP7_LANECTRL_BANK1_dp7_lanecontrol_bank1_START  (0)
#define MST_DP7_LANECTRL_BANK1_dp7_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP8_INTSTAT_UNION
 结构说明  : DP8_INTSTAT 寄存器结构定义。地址偏移量:0x800，初值:0x00，宽度:8
 寄存器说明: dp8中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_test_fail_int  : 1;  /* bit[0]  : dp8 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp8_port_ready_int : 1;  /* bit[1]  : dp8 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp8_cfg_error0_int : 1;  /* bit[5]  : dp8自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp8_cfg_error1_int : 1;  /* bit[6]  : dp8自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp8_cfg_error2_int : 1;  /* bit[7]  : dp8自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP8_INTSTAT_UNION;
#endif
#define MST_DP8_INTSTAT_dp8_test_fail_int_START   (0)
#define MST_DP8_INTSTAT_dp8_test_fail_int_END     (0)
#define MST_DP8_INTSTAT_dp8_port_ready_int_START  (1)
#define MST_DP8_INTSTAT_dp8_port_ready_int_END    (1)
#define MST_DP8_INTSTAT_dp8_cfg_error0_int_START  (5)
#define MST_DP8_INTSTAT_dp8_cfg_error0_int_END    (5)
#define MST_DP8_INTSTAT_dp8_cfg_error1_int_START  (6)
#define MST_DP8_INTSTAT_dp8_cfg_error1_int_END    (6)
#define MST_DP8_INTSTAT_dp8_cfg_error2_int_START  (7)
#define MST_DP8_INTSTAT_dp8_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_INTMASK_UNION
 结构说明  : DP8_INTMASK 寄存器结构定义。地址偏移量:0x801，初值:0x00，宽度:8
 寄存器说明: dp8中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_test_fail_int_mask  : 1;  /* bit[0]  : dp8 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp8_port_ready_int_mask : 1;  /* bit[1]  : dp8 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp8_cfg_error0_int_mask : 1;  /* bit[5]  : dp8自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp8_cfg_error1_int_mask : 1;  /* bit[6]  : dp8自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp8_cfg_error2_int_mask : 1;  /* bit[7]  : dp8自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP8_INTMASK_UNION;
#endif
#define MST_DP8_INTMASK_dp8_test_fail_int_mask_START   (0)
#define MST_DP8_INTMASK_dp8_test_fail_int_mask_END     (0)
#define MST_DP8_INTMASK_dp8_port_ready_int_mask_START  (1)
#define MST_DP8_INTMASK_dp8_port_ready_int_mask_END    (1)
#define MST_DP8_INTMASK_dp8_cfg_error0_int_mask_START  (5)
#define MST_DP8_INTMASK_dp8_cfg_error0_int_mask_END    (5)
#define MST_DP8_INTMASK_dp8_cfg_error1_int_mask_START  (6)
#define MST_DP8_INTMASK_dp8_cfg_error1_int_mask_END    (6)
#define MST_DP8_INTMASK_dp8_cfg_error2_int_mask_START  (7)
#define MST_DP8_INTMASK_dp8_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_PORTCTRL_UNION
 结构说明  : DP8_PORTCTRL 寄存器结构定义。地址偏移量:0x802，初值:0x00，宽度:8
 寄存器说明: dp8的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_portflowmode   : 2;  /* bit[0-1]: dp8流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp8_portdatamode   : 2;  /* bit[2-3]: dp8 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp8_nextinvertbank : 1;  /* bit[4]  : dp8 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP8_PORTCTRL_UNION;
#endif
#define MST_DP8_PORTCTRL_dp8_portflowmode_START    (0)
#define MST_DP8_PORTCTRL_dp8_portflowmode_END      (1)
#define MST_DP8_PORTCTRL_dp8_portdatamode_START    (2)
#define MST_DP8_PORTCTRL_dp8_portdatamode_END      (3)
#define MST_DP8_PORTCTRL_dp8_nextinvertbank_START  (4)
#define MST_DP8_PORTCTRL_dp8_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP8_BLOCKCTRL1_UNION
 结构说明  : DP8_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x803，初值:0x1F，宽度:8
 寄存器说明: dp8 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_wordlength : 6;  /* bit[0-5]: dp8 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP8_BLOCKCTRL1_UNION;
#endif
#define MST_DP8_BLOCKCTRL1_dp8_wordlength_START  (0)
#define MST_DP8_BLOCKCTRL1_dp8_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP8_PREPARESTATUS_UNION
 结构说明  : DP8_PREPARESTATUS 寄存器结构定义。地址偏移量:0x804，初值:0x00，宽度:8
 寄存器说明: dp8的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_n_finished_channel1 : 1;  /* bit[0]: dp8通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel2 : 1;  /* bit[1]: dp8通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel3 : 1;  /* bit[2]: dp8通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel4 : 1;  /* bit[3]: dp8通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel5 : 1;  /* bit[4]: dp8通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel6 : 1;  /* bit[5]: dp8通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel7 : 1;  /* bit[6]: dp8通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp8_n_finished_channel8 : 1;  /* bit[7]: dp8通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP8_PREPARESTATUS_UNION;
#endif
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel1_START  (0)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel1_END    (0)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel2_START  (1)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel2_END    (1)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel3_START  (2)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel3_END    (2)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel4_START  (3)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel4_END    (3)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel5_START  (4)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel5_END    (4)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel6_START  (5)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel6_END    (5)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel7_START  (6)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel7_END    (6)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel8_START  (7)
#define MST_DP8_PREPARESTATUS_dp8_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_PREPARECTRL_UNION
 结构说明  : DP8_PREPARECTRL 寄存器结构定义。地址偏移量:0x805，初值:0x00，宽度:8
 寄存器说明: dp8的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_prepare_channel1 : 1;  /* bit[0]: dp8使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel2 : 1;  /* bit[1]: dp8使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel3 : 1;  /* bit[2]: dp8使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel4 : 1;  /* bit[3]: dp8使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel5 : 1;  /* bit[4]: dp8使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel6 : 1;  /* bit[5]: dp8使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel7 : 1;  /* bit[6]: dp8使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp8_prepare_channel8 : 1;  /* bit[7]: dp8使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP8_PREPARECTRL_UNION;
#endif
#define MST_DP8_PREPARECTRL_dp8_prepare_channel1_START  (0)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel1_END    (0)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel2_START  (1)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel2_END    (1)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel3_START  (2)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel3_END    (2)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel4_START  (3)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel4_END    (3)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel5_START  (4)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel5_END    (4)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel6_START  (5)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel6_END    (5)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel7_START  (6)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel7_END    (6)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel8_START  (7)
#define MST_DP8_PREPARECTRL_dp8_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_CHANNELEN_BANK0_UNION
 结构说明  : DP8_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x820，初值:0x00，宽度:8
 寄存器说明: dp8的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_enable_channel1_bank0 : 1;  /* bit[0]: dp8的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel2_bank0 : 1;  /* bit[1]: dp8的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel3_bank0 : 1;  /* bit[2]: dp8的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel4_bank0 : 1;  /* bit[3]: dp8的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel5_bank0 : 1;  /* bit[4]: dp8的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel6_bank0 : 1;  /* bit[5]: dp8的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel7_bank0 : 1;  /* bit[6]: dp8的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel8_bank0 : 1;  /* bit[7]: dp8的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP8_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel1_bank0_START  (0)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel1_bank0_END    (0)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel2_bank0_START  (1)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel2_bank0_END    (1)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel3_bank0_START  (2)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel3_bank0_END    (2)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel4_bank0_START  (3)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel4_bank0_END    (3)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel5_bank0_START  (4)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel5_bank0_END    (4)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel6_bank0_START  (5)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel6_bank0_END    (5)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel7_bank0_START  (6)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel7_bank0_END    (6)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel8_bank0_START  (7)
#define MST_DP8_CHANNELEN_BANK0_dp8_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP8_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x821，初值:0x00，宽度:8
 寄存器说明: dp8 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp8 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP8_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP8_BLOCKCTRL2_BANK0_dp8_blockgroupcontrol_bank0_START  (0)
#define MST_DP8_BLOCKCTRL2_BANK0_dp8_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP8_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP8_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x822，初值:0x00，宽度:8
 寄存器说明: dp8的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp8的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP8_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP8_SAMPLECTRL1_BANK0_dp8_sampleintervallow_bank0_START  (0)
#define MST_DP8_SAMPLECTRL1_BANK0_dp8_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP8_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x823，初值:0x00，宽度:8
 寄存器说明: dp8的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp8的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP8_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP8_SAMPLECTRL2_BANK0_dp8_sampleintervalhigh_bank0_START  (0)
#define MST_DP8_SAMPLECTRL2_BANK0_dp8_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP8_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x824，初值:0x00，宽度:8
 寄存器说明: dp8的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_offset1_bank0 : 8;  /* bit[0-7]: dp8的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP8_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP8_OFFSETCTRL1_BANK0_dp8_offset1_bank0_START  (0)
#define MST_DP8_OFFSETCTRL1_BANK0_dp8_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP8_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x825，初值:0x00，宽度:8
 寄存器说明: dp8的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_offset2_bank0 : 8;  /* bit[0-7]: dp8的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP8_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP8_OFFSETCTRL2_BANK0_dp8_offset2_bank0_START  (0)
#define MST_DP8_OFFSETCTRL2_BANK0_dp8_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_HCTRL_BANK0_UNION
 结构说明  : DP8_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x826，初值:0x11，宽度:8
 寄存器说明: dp8的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_hstop_bank0  : 4;  /* bit[0-3]: dp8传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp8_hstart_bank0 : 4;  /* bit[4-7]: dp8传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP8_HCTRL_BANK0_UNION;
#endif
#define MST_DP8_HCTRL_BANK0_dp8_hstop_bank0_START   (0)
#define MST_DP8_HCTRL_BANK0_dp8_hstop_bank0_END     (3)
#define MST_DP8_HCTRL_BANK0_dp8_hstart_bank0_START  (4)
#define MST_DP8_HCTRL_BANK0_dp8_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP8_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x827，初值:0x00，宽度:8
 寄存器说明: dp8 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_blockpackingmode_bank0 : 1;  /* bit[0]  : dp8 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP8_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP8_BLOCKCTRL3_BANK0_dp8_blockpackingmode_bank0_START  (0)
#define MST_DP8_BLOCKCTRL3_BANK0_dp8_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP8_LANECTRL_BANK0_UNION
 结构说明  : DP8_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x828，初值:0x00，宽度:8
 寄存器说明: dp8的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_lanecontrol_bank0 : 3;  /* bit[0-2]: dp8的数据线控制，表示dp8在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP8_LANECTRL_BANK0_UNION;
#endif
#define MST_DP8_LANECTRL_BANK0_dp8_lanecontrol_bank0_START  (0)
#define MST_DP8_LANECTRL_BANK0_dp8_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP8_CHANNELEN_BANK1_UNION
 结构说明  : DP8_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x830，初值:0x00，宽度:8
 寄存器说明: dp8的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_enable_channel1_bank1 : 1;  /* bit[0]: dp8的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel2_bank1 : 1;  /* bit[1]: dp8的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel3_bank1 : 1;  /* bit[2]: dp8的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel4_bank1 : 1;  /* bit[3]: dp8的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel5_bank1 : 1;  /* bit[4]: dp8的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel6_bank1 : 1;  /* bit[5]: dp8的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel7_bank1 : 1;  /* bit[6]: dp8的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp8_enable_channel8_bank1 : 1;  /* bit[7]: dp8的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP8_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel1_bank1_START  (0)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel1_bank1_END    (0)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel2_bank1_START  (1)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel2_bank1_END    (1)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel3_bank1_START  (2)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel3_bank1_END    (2)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel4_bank1_START  (3)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel4_bank1_END    (3)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel5_bank1_START  (4)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel5_bank1_END    (4)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel6_bank1_START  (5)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel6_bank1_END    (5)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel7_bank1_START  (6)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel7_bank1_END    (6)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel8_bank1_START  (7)
#define MST_DP8_CHANNELEN_BANK1_dp8_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP8_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x831，初值:0x00，宽度:8
 寄存器说明: dp8 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp8 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP8_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP8_BLOCKCTRL2_BANK1_dp8_blockgroupcontrol_bank1_START  (0)
#define MST_DP8_BLOCKCTRL2_BANK1_dp8_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP8_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP8_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x832，初值:0x00，宽度:8
 寄存器说明: dp8的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp8的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP8_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP8_SAMPLECTRL1_BANK1_dp8_sampleintervallow_bank1_START  (0)
#define MST_DP8_SAMPLECTRL1_BANK1_dp8_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP8_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x833，初值:0x00，宽度:8
 寄存器说明: dp8的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp8的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP8_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP8_SAMPLECTRL2_BANK1_dp8_sampleintervalhigh_bank1_START  (0)
#define MST_DP8_SAMPLECTRL2_BANK1_dp8_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP8_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x834，初值:0x00，宽度:8
 寄存器说明: dp8的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_offset1_bank1 : 8;  /* bit[0-7]: dp8的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP8_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP8_OFFSETCTRL1_BANK1_dp8_offset1_bank1_START  (0)
#define MST_DP8_OFFSETCTRL1_BANK1_dp8_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP8_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x835，初值:0x00，宽度:8
 寄存器说明: dp8的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_offset2_bank1 : 8;  /* bit[0-7]: dp8的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP8_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP8_OFFSETCTRL2_BANK1_dp8_offset2_bank1_START  (0)
#define MST_DP8_OFFSETCTRL2_BANK1_dp8_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_HCTRL_BANK1_UNION
 结构说明  : DP8_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x836，初值:0x11，宽度:8
 寄存器说明: dp8的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_hstop_bank1  : 4;  /* bit[0-3]: dp8传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp8_hstart_bank1 : 4;  /* bit[4-7]: dp8传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP8_HCTRL_BANK1_UNION;
#endif
#define MST_DP8_HCTRL_BANK1_dp8_hstop_bank1_START   (0)
#define MST_DP8_HCTRL_BANK1_dp8_hstop_bank1_END     (3)
#define MST_DP8_HCTRL_BANK1_dp8_hstart_bank1_START  (4)
#define MST_DP8_HCTRL_BANK1_dp8_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP8_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP8_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x837，初值:0x00，宽度:8
 寄存器说明: dp8 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_blockpackingmode_bank1 : 1;  /* bit[0]  : dp8 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP8_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP8_BLOCKCTRL3_BANK1_dp8_blockpackingmode_bank1_START  (0)
#define MST_DP8_BLOCKCTRL3_BANK1_dp8_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP8_LANECTRL_BANK1_UNION
 结构说明  : DP8_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x838，初值:0x00，宽度:8
 寄存器说明: dp8的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp8_lanecontrol_bank1 : 3;  /* bit[0-2]: dp8的数据线控制，表示dp8在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP8_LANECTRL_BANK1_UNION;
#endif
#define MST_DP8_LANECTRL_BANK1_dp8_lanecontrol_bank1_START  (0)
#define MST_DP8_LANECTRL_BANK1_dp8_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP9_INTSTAT_UNION
 结构说明  : DP9_INTSTAT 寄存器结构定义。地址偏移量:0x900，初值:0x00，宽度:8
 寄存器说明: dp9中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_test_fail_int  : 1;  /* bit[0]  : dp9 TestFail中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp9_port_ready_int : 1;  /* bit[1]  : dp9 PortReady中断:
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  reserved_0         : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1         : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp9_cfg_error0_int : 1;  /* bit[5]  : dp9自定义中断1: (业务起来后，fifo空)
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp9_cfg_error1_int : 1;  /* bit[6]  : dp9自定义中断2: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
        unsigned char  dp9_cfg_error2_int : 1;  /* bit[7]  : dp9自定义中断3: 
                                                             1：写1时清除中断，查询时表示中断有效；
                                                             0：查询时表示中断无效。 */
    } reg;
} MST_DP9_INTSTAT_UNION;
#endif
#define MST_DP9_INTSTAT_dp9_test_fail_int_START   (0)
#define MST_DP9_INTSTAT_dp9_test_fail_int_END     (0)
#define MST_DP9_INTSTAT_dp9_port_ready_int_START  (1)
#define MST_DP9_INTSTAT_dp9_port_ready_int_END    (1)
#define MST_DP9_INTSTAT_dp9_cfg_error0_int_START  (5)
#define MST_DP9_INTSTAT_dp9_cfg_error0_int_END    (5)
#define MST_DP9_INTSTAT_dp9_cfg_error1_int_START  (6)
#define MST_DP9_INTSTAT_dp9_cfg_error1_int_END    (6)
#define MST_DP9_INTSTAT_dp9_cfg_error2_int_START  (7)
#define MST_DP9_INTSTAT_dp9_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_INTMASK_UNION
 结构说明  : DP9_INTMASK 寄存器结构定义。地址偏移量:0x901，初值:0x00，宽度:8
 寄存器说明: dp9中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_test_fail_int_mask  : 1;  /* bit[0]  : dp9 TestFail中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp9_port_ready_int_mask : 1;  /* bit[1]  : dp9 port Ready中断屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  reserved_0              : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1              : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp9_cfg_error0_int_mask : 1;  /* bit[5]  : dp9自定义中断1屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp9_cfg_error1_int_mask : 1;  /* bit[6]  : dp9自定义中断2屏蔽位: 
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
        unsigned char  dp9_cfg_error2_int_mask : 1;  /* bit[7]  : dp9自定义中断3屏蔽位:
                                                                  1'b0：中断屏蔽;
                                                                  1'b1：中断不屏蔽; */
    } reg;
} MST_DP9_INTMASK_UNION;
#endif
#define MST_DP9_INTMASK_dp9_test_fail_int_mask_START   (0)
#define MST_DP9_INTMASK_dp9_test_fail_int_mask_END     (0)
#define MST_DP9_INTMASK_dp9_port_ready_int_mask_START  (1)
#define MST_DP9_INTMASK_dp9_port_ready_int_mask_END    (1)
#define MST_DP9_INTMASK_dp9_cfg_error0_int_mask_START  (5)
#define MST_DP9_INTMASK_dp9_cfg_error0_int_mask_END    (5)
#define MST_DP9_INTMASK_dp9_cfg_error1_int_mask_START  (6)
#define MST_DP9_INTMASK_dp9_cfg_error1_int_mask_END    (6)
#define MST_DP9_INTMASK_dp9_cfg_error2_int_mask_START  (7)
#define MST_DP9_INTMASK_dp9_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_PORTCTRL_UNION
 结构说明  : DP9_PORTCTRL 寄存器结构定义。地址偏移量:0x902，初值:0x00，宽度:8
 寄存器说明: dp9的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_portflowmode   : 2;  /* bit[0-1]: dp9流控模式选择：
                                                             2'b00:normal模式；
                                                             2'b01:push模式；
                                                             2'b10:pull模式； */
        unsigned char  dp9_portdatamode   : 2;  /* bit[2-3]: dp9 端口数据模式配置:
                                                             2'b00：Normal
                                                             2'b01：PRBS
                                                             2'b10：Static_0
                                                             2'b11：Static_1 */
        unsigned char  dp9_nextinvertbank : 1;  /* bit[4]  : dp9 NextInvertBank配置:
                                                             1'b0:取当前framectrl相同配置;
                                                             1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0         : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1         : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP9_PORTCTRL_UNION;
#endif
#define MST_DP9_PORTCTRL_dp9_portflowmode_START    (0)
#define MST_DP9_PORTCTRL_dp9_portflowmode_END      (1)
#define MST_DP9_PORTCTRL_dp9_portdatamode_START    (2)
#define MST_DP9_PORTCTRL_dp9_portdatamode_END      (3)
#define MST_DP9_PORTCTRL_dp9_nextinvertbank_START  (4)
#define MST_DP9_PORTCTRL_dp9_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP9_BLOCKCTRL1_UNION
 结构说明  : DP9_BLOCKCTRL1 寄存器结构定义。地址偏移量:0x903，初值:0x1F，宽度:8
 寄存器说明: dp9 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_wordlength : 6;  /* bit[0-5]: dp9 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved       : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP9_BLOCKCTRL1_UNION;
#endif
#define MST_DP9_BLOCKCTRL1_dp9_wordlength_START  (0)
#define MST_DP9_BLOCKCTRL1_dp9_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP9_PREPARESTATUS_UNION
 结构说明  : DP9_PREPARESTATUS 寄存器结构定义。地址偏移量:0x904，初值:0x00，宽度:8
 寄存器说明: dp9的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_n_finished_channel1 : 1;  /* bit[0]: dp9通道1状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel2 : 1;  /* bit[1]: dp9通道2状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel3 : 1;  /* bit[2]: dp9通道3状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel4 : 1;  /* bit[3]: dp9通道4状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel5 : 1;  /* bit[4]: dp9通道5状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel6 : 1;  /* bit[5]: dp9通道6状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel7 : 1;  /* bit[6]: dp9通道7状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
        unsigned char  dp9_n_finished_channel8 : 1;  /* bit[7]: dp9通道8状态未完成指示:
                                                                1'b0:完成;
                                                                1'b1:未完成; */
    } reg;
} MST_DP9_PREPARESTATUS_UNION;
#endif
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel1_START  (0)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel1_END    (0)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel2_START  (1)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel2_END    (1)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel3_START  (2)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel3_END    (2)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel4_START  (3)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel4_END    (3)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel5_START  (4)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel5_END    (4)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel6_START  (5)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel6_END    (5)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel7_START  (6)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel7_END    (6)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel8_START  (7)
#define MST_DP9_PREPARESTATUS_dp9_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_PREPARECTRL_UNION
 结构说明  : DP9_PREPARECTRL 寄存器结构定义。地址偏移量:0x905，初值:0x00，宽度:8
 寄存器说明: dp9的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_prepare_channel1 : 1;  /* bit[0]: dp9使能通道1准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel2 : 1;  /* bit[1]: dp9使能通道2准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel3 : 1;  /* bit[2]: dp9使能通道3准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel4 : 1;  /* bit[3]: dp9使能通道4准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel5 : 1;  /* bit[4]: dp9使能通道5准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel6 : 1;  /* bit[5]: dp9使能通道6准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel7 : 1;  /* bit[6]: dp9使能通道7准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
        unsigned char  dp9_prepare_channel8 : 1;  /* bit[7]: dp9使能通道准备:
                                                             1'b0:不使能;
                                                             1'b1:使能； */
    } reg;
} MST_DP9_PREPARECTRL_UNION;
#endif
#define MST_DP9_PREPARECTRL_dp9_prepare_channel1_START  (0)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel1_END    (0)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel2_START  (1)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel2_END    (1)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel3_START  (2)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel3_END    (2)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel4_START  (3)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel4_END    (3)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel5_START  (4)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel5_END    (4)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel6_START  (5)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel6_END    (5)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel7_START  (6)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel7_END    (6)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel8_START  (7)
#define MST_DP9_PREPARECTRL_dp9_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_CHANNELEN_BANK0_UNION
 结构说明  : DP9_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0x920，初值:0x00，宽度:8
 寄存器说明: dp9的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_enable_channel1_bank0 : 1;  /* bit[0]: dp9的Channel1使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel2_bank0 : 1;  /* bit[1]: dp9的Channel2使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel3_bank0 : 1;  /* bit[2]: dp9的Channel3使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel4_bank0 : 1;  /* bit[3]: dp9的Channel4使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel5_bank0 : 1;  /* bit[4]: dp9的Channel5使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel6_bank0 : 1;  /* bit[5]: dp9的Channel6使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel7_bank0 : 1;  /* bit[6]: dp9的Channel7使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel8_bank0 : 1;  /* bit[7]: dp9的Channel8使能:(bank0)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP9_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel1_bank0_START  (0)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel1_bank0_END    (0)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel2_bank0_START  (1)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel2_bank0_END    (1)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel3_bank0_START  (2)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel3_bank0_END    (2)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel4_bank0_START  (3)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel4_bank0_END    (3)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel5_bank0_START  (4)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel5_bank0_END    (4)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel6_bank0_START  (5)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel6_bank0_END    (5)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel7_bank0_START  (6)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel7_bank0_END    (6)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel8_bank0_START  (7)
#define MST_DP9_CHANNELEN_BANK0_dp9_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP9_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0x921，初值:0x00，宽度:8
 寄存器说明: dp9 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp9 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP9_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP9_BLOCKCTRL2_BANK0_dp9_blockgroupcontrol_bank0_START  (0)
#define MST_DP9_BLOCKCTRL2_BANK0_dp9_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP9_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP9_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0x922，初值:0x00，宽度:8
 寄存器说明: dp9的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp9的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP9_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP9_SAMPLECTRL1_BANK0_dp9_sampleintervallow_bank0_START  (0)
#define MST_DP9_SAMPLECTRL1_BANK0_dp9_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP9_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0x923，初值:0x00，宽度:8
 寄存器说明: dp9的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp9的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP9_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP9_SAMPLECTRL2_BANK0_dp9_sampleintervalhigh_bank0_START  (0)
#define MST_DP9_SAMPLECTRL2_BANK0_dp9_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP9_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0x924，初值:0x00，宽度:8
 寄存器说明: dp9的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_offset1_bank0 : 8;  /* bit[0-7]: dp9的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP9_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP9_OFFSETCTRL1_BANK0_dp9_offset1_bank0_START  (0)
#define MST_DP9_OFFSETCTRL1_BANK0_dp9_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP9_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0x925，初值:0x00，宽度:8
 寄存器说明: dp9的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_offset2_bank0 : 8;  /* bit[0-7]: dp9的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP9_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP9_OFFSETCTRL2_BANK0_dp9_offset2_bank0_START  (0)
#define MST_DP9_OFFSETCTRL2_BANK0_dp9_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_HCTRL_BANK0_UNION
 结构说明  : DP9_HCTRL_BANK0 寄存器结构定义。地址偏移量:0x926，初值:0x11，宽度:8
 寄存器说明: dp9的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_hstop_bank0  : 4;  /* bit[0-3]: dp9传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp9_hstart_bank0 : 4;  /* bit[4-7]: dp9传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP9_HCTRL_BANK0_UNION;
#endif
#define MST_DP9_HCTRL_BANK0_dp9_hstop_bank0_START   (0)
#define MST_DP9_HCTRL_BANK0_dp9_hstop_bank0_END     (3)
#define MST_DP9_HCTRL_BANK0_dp9_hstart_bank0_START  (4)
#define MST_DP9_HCTRL_BANK0_dp9_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP9_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0x927，初值:0x00，宽度:8
 寄存器说明: dp9 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_blockpackingmode_bank0 : 1;  /* bit[0]  : dp9 blockpacking模式配置:(bank0)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP9_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP9_BLOCKCTRL3_BANK0_dp9_blockpackingmode_bank0_START  (0)
#define MST_DP9_BLOCKCTRL3_BANK0_dp9_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP9_LANECTRL_BANK0_UNION
 结构说明  : DP9_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0x928，初值:0x00，宽度:8
 寄存器说明: dp9的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_lanecontrol_bank0 : 3;  /* bit[0-2]: dp9的数据线控制，表示dp9在哪根数据线上传输:(bank0)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP9_LANECTRL_BANK0_UNION;
#endif
#define MST_DP9_LANECTRL_BANK0_dp9_lanecontrol_bank0_START  (0)
#define MST_DP9_LANECTRL_BANK0_dp9_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP9_CHANNELEN_BANK1_UNION
 结构说明  : DP9_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0x930，初值:0x00，宽度:8
 寄存器说明: dp9的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_enable_channel1_bank1 : 1;  /* bit[0]: dp9的Channel1使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel2_bank1 : 1;  /* bit[1]: dp9的Channel2使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel3_bank1 : 1;  /* bit[2]: dp9的Channel3使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel4_bank1 : 1;  /* bit[3]: dp9的Channel4使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel5_bank1 : 1;  /* bit[4]: dp9的Channel5使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel6_bank1 : 1;  /* bit[5]: dp9的Channel6使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel7_bank1 : 1;  /* bit[6]: dp9的Channel7使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
        unsigned char  dp9_enable_channel8_bank1 : 1;  /* bit[7]: dp9的Channel8使能:(bank1)
                                                                  1'b0:不使能;
                                                                  1'b1:使能; */
    } reg;
} MST_DP9_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel1_bank1_START  (0)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel1_bank1_END    (0)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel2_bank1_START  (1)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel2_bank1_END    (1)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel3_bank1_START  (2)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel3_bank1_END    (2)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel4_bank1_START  (3)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel4_bank1_END    (3)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel5_bank1_START  (4)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel5_bank1_END    (4)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel6_bank1_START  (5)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel6_bank1_END    (5)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel7_bank1_START  (6)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel7_bank1_END    (6)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel8_bank1_START  (7)
#define MST_DP9_CHANNELEN_BANK1_dp9_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP9_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0x931，初值:0x00，宽度:8
 寄存器说明: dp9 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp9 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                    : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP9_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP9_BLOCKCTRL2_BANK1_dp9_blockgroupcontrol_bank1_START  (0)
#define MST_DP9_BLOCKCTRL2_BANK1_dp9_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP9_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP9_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0x932，初值:0x00，宽度:8
 寄存器说明: dp9的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp9的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP9_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP9_SAMPLECTRL1_BANK1_dp9_sampleintervallow_bank1_START  (0)
#define MST_DP9_SAMPLECTRL1_BANK1_dp9_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP9_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0x933，初值:0x00，宽度:8
 寄存器说明: dp9的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp9的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP9_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP9_SAMPLECTRL2_BANK1_dp9_sampleintervalhigh_bank1_START  (0)
#define MST_DP9_SAMPLECTRL2_BANK1_dp9_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP9_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0x934，初值:0x00，宽度:8
 寄存器说明: dp9的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_offset1_bank1 : 8;  /* bit[0-7]: dp9的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP9_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP9_OFFSETCTRL1_BANK1_dp9_offset1_bank1_START  (0)
#define MST_DP9_OFFSETCTRL1_BANK1_dp9_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP9_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0x935，初值:0x00，宽度:8
 寄存器说明: dp9的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_offset2_bank1 : 8;  /* bit[0-7]: dp9的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP9_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP9_OFFSETCTRL2_BANK1_dp9_offset2_bank1_START  (0)
#define MST_DP9_OFFSETCTRL2_BANK1_dp9_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_HCTRL_BANK1_UNION
 结构说明  : DP9_HCTRL_BANK1 寄存器结构定义。地址偏移量:0x936，初值:0x11，宽度:8
 寄存器说明: dp9的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_hstop_bank1  : 4;  /* bit[0-3]: dp9传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp9_hstart_bank1 : 4;  /* bit[4-7]: dp9传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP9_HCTRL_BANK1_UNION;
#endif
#define MST_DP9_HCTRL_BANK1_dp9_hstop_bank1_START   (0)
#define MST_DP9_HCTRL_BANK1_dp9_hstop_bank1_END     (3)
#define MST_DP9_HCTRL_BANK1_dp9_hstart_bank1_START  (4)
#define MST_DP9_HCTRL_BANK1_dp9_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP9_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP9_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0x937，初值:0x00，宽度:8
 寄存器说明: dp9 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_blockpackingmode_bank1 : 1;  /* bit[0]  : dp9 blockpacking模式配置:(bank1)
                                                                     1'b0:不使能；
                                                                     1'b1使能； */
        unsigned char  reserved                   : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP9_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP9_BLOCKCTRL3_BANK1_dp9_blockpackingmode_bank1_START  (0)
#define MST_DP9_BLOCKCTRL3_BANK1_dp9_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP9_LANECTRL_BANK1_UNION
 结构说明  : DP9_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0x938，初值:0x00，宽度:8
 寄存器说明: dp9的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp9_lanecontrol_bank1 : 3;  /* bit[0-2]: dp9的数据线控制，表示dp9在哪根数据线上传输:(bank1)
                                                                3'b000:data0;
                                                                3'b001:data1;（其他禁配） */
        unsigned char  reserved              : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP9_LANECTRL_BANK1_UNION;
#endif
#define MST_DP9_LANECTRL_BANK1_dp9_lanecontrol_bank1_START  (0)
#define MST_DP9_LANECTRL_BANK1_dp9_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP10_INTSTAT_UNION
 结构说明  : DP10_INTSTAT 寄存器结构定义。地址偏移量:0xA00，初值:0x00，宽度:8
 寄存器说明: dp10中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_test_fail_int  : 1;  /* bit[0]  : dp10 TestFail中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp10_port_ready_int : 1;  /* bit[1]  : dp10 PortReady中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  reserved_0          : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1          : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp10_cfg_error0_int : 1;  /* bit[5]  : dp10自定义中断1: (业务起来后，fifo空)
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp10_cfg_error1_int : 1;  /* bit[6]  : dp10自定义中断2: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp10_cfg_error2_int : 1;  /* bit[7]  : dp10自定义中断3: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
    } reg;
} MST_DP10_INTSTAT_UNION;
#endif
#define MST_DP10_INTSTAT_dp10_test_fail_int_START   (0)
#define MST_DP10_INTSTAT_dp10_test_fail_int_END     (0)
#define MST_DP10_INTSTAT_dp10_port_ready_int_START  (1)
#define MST_DP10_INTSTAT_dp10_port_ready_int_END    (1)
#define MST_DP10_INTSTAT_dp10_cfg_error0_int_START  (5)
#define MST_DP10_INTSTAT_dp10_cfg_error0_int_END    (5)
#define MST_DP10_INTSTAT_dp10_cfg_error1_int_START  (6)
#define MST_DP10_INTSTAT_dp10_cfg_error1_int_END    (6)
#define MST_DP10_INTSTAT_dp10_cfg_error2_int_START  (7)
#define MST_DP10_INTSTAT_dp10_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_INTMASK_UNION
 结构说明  : DP10_INTMASK 寄存器结构定义。地址偏移量:0xA01，初值:0x00，宽度:8
 寄存器说明: dp10中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_test_fail_int_mask  : 1;  /* bit[0]  : dp10 TestFail中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp10_port_ready_int_mask : 1;  /* bit[1]  : dp10 port Ready中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  reserved_0               : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1               : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp10_cfg_error0_int_mask : 1;  /* bit[5]  : dp10自定义中断1屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp10_cfg_error1_int_mask : 1;  /* bit[6]  : dp10自定义中断2屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp10_cfg_error2_int_mask : 1;  /* bit[7]  : dp10自定义中断3屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
    } reg;
} MST_DP10_INTMASK_UNION;
#endif
#define MST_DP10_INTMASK_dp10_test_fail_int_mask_START   (0)
#define MST_DP10_INTMASK_dp10_test_fail_int_mask_END     (0)
#define MST_DP10_INTMASK_dp10_port_ready_int_mask_START  (1)
#define MST_DP10_INTMASK_dp10_port_ready_int_mask_END    (1)
#define MST_DP10_INTMASK_dp10_cfg_error0_int_mask_START  (5)
#define MST_DP10_INTMASK_dp10_cfg_error0_int_mask_END    (5)
#define MST_DP10_INTMASK_dp10_cfg_error1_int_mask_START  (6)
#define MST_DP10_INTMASK_dp10_cfg_error1_int_mask_END    (6)
#define MST_DP10_INTMASK_dp10_cfg_error2_int_mask_START  (7)
#define MST_DP10_INTMASK_dp10_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_PORTCTRL_UNION
 结构说明  : DP10_PORTCTRL 寄存器结构定义。地址偏移量:0xA02，初值:0x00，宽度:8
 寄存器说明: dp10的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_portflowmode   : 2;  /* bit[0-1]: dp10流控模式选择：
                                                              2'b00:normal模式；
                                                              2'b01:push模式；
                                                              2'b10:pull模式； */
        unsigned char  dp10_portdatamode   : 2;  /* bit[2-3]: dp10 端口数据模式配置:
                                                              2'b00：Normal
                                                              2'b01：PRBS
                                                              2'b10：Static_0
                                                              2'b11：Static_1 */
        unsigned char  dp10_nextinvertbank : 1;  /* bit[4]  : dp10 NextInvertBank配置:
                                                              1'b0:取当前framectrl相同配置;
                                                              1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0          : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1          : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP10_PORTCTRL_UNION;
#endif
#define MST_DP10_PORTCTRL_dp10_portflowmode_START    (0)
#define MST_DP10_PORTCTRL_dp10_portflowmode_END      (1)
#define MST_DP10_PORTCTRL_dp10_portdatamode_START    (2)
#define MST_DP10_PORTCTRL_dp10_portdatamode_END      (3)
#define MST_DP10_PORTCTRL_dp10_nextinvertbank_START  (4)
#define MST_DP10_PORTCTRL_dp10_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP10_BLOCKCTRL1_UNION
 结构说明  : DP10_BLOCKCTRL1 寄存器结构定义。地址偏移量:0xA03，初值:0x1F，宽度:8
 寄存器说明: dp10 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_wordlength : 6;  /* bit[0-5]: dp10 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP10_BLOCKCTRL1_UNION;
#endif
#define MST_DP10_BLOCKCTRL1_dp10_wordlength_START  (0)
#define MST_DP10_BLOCKCTRL1_dp10_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP10_PREPARESTATUS_UNION
 结构说明  : DP10_PREPARESTATUS 寄存器结构定义。地址偏移量:0xA04，初值:0x00，宽度:8
 寄存器说明: dp10的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_n_finished_channel1 : 1;  /* bit[0]: dp10通道1状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel2 : 1;  /* bit[1]: dp10通道2状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel3 : 1;  /* bit[2]: dp10通道3状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel4 : 1;  /* bit[3]: dp10通道4状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel5 : 1;  /* bit[4]: dp10通道5状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel6 : 1;  /* bit[5]: dp10通道6状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel7 : 1;  /* bit[6]: dp10通道7状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp10_n_finished_channel8 : 1;  /* bit[7]: dp10通道8状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
    } reg;
} MST_DP10_PREPARESTATUS_UNION;
#endif
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel1_START  (0)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel1_END    (0)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel2_START  (1)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel2_END    (1)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel3_START  (2)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel3_END    (2)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel4_START  (3)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel4_END    (3)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel5_START  (4)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel5_END    (4)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel6_START  (5)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel6_END    (5)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel7_START  (6)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel7_END    (6)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel8_START  (7)
#define MST_DP10_PREPARESTATUS_dp10_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_PREPARECTRL_UNION
 结构说明  : DP10_PREPARECTRL 寄存器结构定义。地址偏移量:0xA05，初值:0x00，宽度:8
 寄存器说明: dp10的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_prepare_channel1 : 1;  /* bit[0]: dp10使能通道1准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel2 : 1;  /* bit[1]: dp10使能通道2准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel3 : 1;  /* bit[2]: dp10使能通道3准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel4 : 1;  /* bit[3]: dp10使能通道4准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel5 : 1;  /* bit[4]: dp10使能通道5准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel6 : 1;  /* bit[5]: dp10使能通道6准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel7 : 1;  /* bit[6]: dp10使能通道7准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp10_prepare_channel8 : 1;  /* bit[7]: dp10使能通道准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
    } reg;
} MST_DP10_PREPARECTRL_UNION;
#endif
#define MST_DP10_PREPARECTRL_dp10_prepare_channel1_START  (0)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel1_END    (0)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel2_START  (1)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel2_END    (1)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel3_START  (2)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel3_END    (2)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel4_START  (3)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel4_END    (3)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel5_START  (4)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel5_END    (4)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel6_START  (5)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel6_END    (5)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel7_START  (6)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel7_END    (6)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel8_START  (7)
#define MST_DP10_PREPARECTRL_dp10_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_CHANNELEN_BANK0_UNION
 结构说明  : DP10_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0xA20，初值:0x00，宽度:8
 寄存器说明: dp10的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_enable_channel1_bank0 : 1;  /* bit[0]: dp10的Channel1使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel2_bank0 : 1;  /* bit[1]: dp10的Channel2使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel3_bank0 : 1;  /* bit[2]: dp10的Channel3使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel4_bank0 : 1;  /* bit[3]: dp10的Channel4使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel5_bank0 : 1;  /* bit[4]: dp10的Channel5使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel6_bank0 : 1;  /* bit[5]: dp10的Channel6使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel7_bank0 : 1;  /* bit[6]: dp10的Channel7使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel8_bank0 : 1;  /* bit[7]: dp10的Channel8使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP10_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel1_bank0_START  (0)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel1_bank0_END    (0)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel2_bank0_START  (1)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel2_bank0_END    (1)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel3_bank0_START  (2)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel3_bank0_END    (2)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel4_bank0_START  (3)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel4_bank0_END    (3)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel5_bank0_START  (4)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel5_bank0_END    (4)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel6_bank0_START  (5)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel6_bank0_END    (5)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel7_bank0_START  (6)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel7_bank0_END    (6)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel8_bank0_START  (7)
#define MST_DP10_CHANNELEN_BANK0_dp10_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP10_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0xA21，初值:0x00，宽度:8
 寄存器说明: dp10 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp10 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP10_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP10_BLOCKCTRL2_BANK0_dp10_blockgroupcontrol_bank0_START  (0)
#define MST_DP10_BLOCKCTRL2_BANK0_dp10_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP10_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP10_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0xA22，初值:0x00，宽度:8
 寄存器说明: dp10的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp10的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP10_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP10_SAMPLECTRL1_BANK0_dp10_sampleintervallow_bank0_START  (0)
#define MST_DP10_SAMPLECTRL1_BANK0_dp10_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP10_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0xA23，初值:0x00，宽度:8
 寄存器说明: dp10的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp10的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP10_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP10_SAMPLECTRL2_BANK0_dp10_sampleintervalhigh_bank0_START  (0)
#define MST_DP10_SAMPLECTRL2_BANK0_dp10_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP10_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0xA24，初值:0x00，宽度:8
 寄存器说明: dp10的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_offset1_bank0 : 8;  /* bit[0-7]: dp10的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP10_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP10_OFFSETCTRL1_BANK0_dp10_offset1_bank0_START  (0)
#define MST_DP10_OFFSETCTRL1_BANK0_dp10_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP10_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0xA25，初值:0x00，宽度:8
 寄存器说明: dp10的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_offset2_bank0 : 8;  /* bit[0-7]: dp10的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP10_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP10_OFFSETCTRL2_BANK0_dp10_offset2_bank0_START  (0)
#define MST_DP10_OFFSETCTRL2_BANK0_dp10_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_HCTRL_BANK0_UNION
 结构说明  : DP10_HCTRL_BANK0 寄存器结构定义。地址偏移量:0xA26，初值:0x11，宽度:8
 寄存器说明: dp10的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_hstop_bank0  : 4;  /* bit[0-3]: dp10传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp10_hstart_bank0 : 4;  /* bit[4-7]: dp10传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP10_HCTRL_BANK0_UNION;
#endif
#define MST_DP10_HCTRL_BANK0_dp10_hstop_bank0_START   (0)
#define MST_DP10_HCTRL_BANK0_dp10_hstop_bank0_END     (3)
#define MST_DP10_HCTRL_BANK0_dp10_hstart_bank0_START  (4)
#define MST_DP10_HCTRL_BANK0_dp10_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP10_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0xA27，初值:0x00，宽度:8
 寄存器说明: dp10 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_blockpackingmode_bank0 : 1;  /* bit[0]  : dp10 blockpacking模式配置:(bank0)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP10_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP10_BLOCKCTRL3_BANK0_dp10_blockpackingmode_bank0_START  (0)
#define MST_DP10_BLOCKCTRL3_BANK0_dp10_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP10_LANECTRL_BANK0_UNION
 结构说明  : DP10_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0xA28，初值:0x00，宽度:8
 寄存器说明: dp10的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_lanecontrol_bank0 : 3;  /* bit[0-2]: dp10的数据线控制，表示dp10在哪根数据线上传输:(bank0)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP10_LANECTRL_BANK0_UNION;
#endif
#define MST_DP10_LANECTRL_BANK0_dp10_lanecontrol_bank0_START  (0)
#define MST_DP10_LANECTRL_BANK0_dp10_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP10_CHANNELEN_BANK1_UNION
 结构说明  : DP10_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0xA30，初值:0x00，宽度:8
 寄存器说明: dp10的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_enable_channel1_bank1 : 1;  /* bit[0]: dp10的Channel1使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel2_bank1 : 1;  /* bit[1]: dp10的Channel2使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel3_bank1 : 1;  /* bit[2]: dp10的Channel3使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel4_bank1 : 1;  /* bit[3]: dp10的Channel4使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel5_bank1 : 1;  /* bit[4]: dp10的Channel5使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel6_bank1 : 1;  /* bit[5]: dp10的Channel6使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel7_bank1 : 1;  /* bit[6]: dp10的Channel7使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp10_enable_channel8_bank1 : 1;  /* bit[7]: dp10的Channel8使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP10_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel1_bank1_START  (0)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel1_bank1_END    (0)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel2_bank1_START  (1)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel2_bank1_END    (1)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel3_bank1_START  (2)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel3_bank1_END    (2)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel4_bank1_START  (3)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel4_bank1_END    (3)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel5_bank1_START  (4)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel5_bank1_END    (4)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel6_bank1_START  (5)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel6_bank1_END    (5)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel7_bank1_START  (6)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel7_bank1_END    (6)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel8_bank1_START  (7)
#define MST_DP10_CHANNELEN_BANK1_dp10_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP10_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0xA31，初值:0x00，宽度:8
 寄存器说明: dp10 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp10 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP10_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP10_BLOCKCTRL2_BANK1_dp10_blockgroupcontrol_bank1_START  (0)
#define MST_DP10_BLOCKCTRL2_BANK1_dp10_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP10_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP10_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0xA32，初值:0x00，宽度:8
 寄存器说明: dp10的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp10的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP10_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP10_SAMPLECTRL1_BANK1_dp10_sampleintervallow_bank1_START  (0)
#define MST_DP10_SAMPLECTRL1_BANK1_dp10_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP10_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0xA33，初值:0x00，宽度:8
 寄存器说明: dp10的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp10的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP10_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP10_SAMPLECTRL2_BANK1_dp10_sampleintervalhigh_bank1_START  (0)
#define MST_DP10_SAMPLECTRL2_BANK1_dp10_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP10_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0xA34，初值:0x00，宽度:8
 寄存器说明: dp10的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_offset1_bank1 : 8;  /* bit[0-7]: dp10的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP10_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP10_OFFSETCTRL1_BANK1_dp10_offset1_bank1_START  (0)
#define MST_DP10_OFFSETCTRL1_BANK1_dp10_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP10_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0xA35，初值:0x00，宽度:8
 寄存器说明: dp10的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_offset2_bank1 : 8;  /* bit[0-7]: dp10的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP10_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP10_OFFSETCTRL2_BANK1_dp10_offset2_bank1_START  (0)
#define MST_DP10_OFFSETCTRL2_BANK1_dp10_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_HCTRL_BANK1_UNION
 结构说明  : DP10_HCTRL_BANK1 寄存器结构定义。地址偏移量:0xA36，初值:0x11，宽度:8
 寄存器说明: dp10的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_hstop_bank1  : 4;  /* bit[0-3]: dp10传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp10_hstart_bank1 : 4;  /* bit[4-7]: dp10传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP10_HCTRL_BANK1_UNION;
#endif
#define MST_DP10_HCTRL_BANK1_dp10_hstop_bank1_START   (0)
#define MST_DP10_HCTRL_BANK1_dp10_hstop_bank1_END     (3)
#define MST_DP10_HCTRL_BANK1_dp10_hstart_bank1_START  (4)
#define MST_DP10_HCTRL_BANK1_dp10_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP10_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP10_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0xA37，初值:0x00，宽度:8
 寄存器说明: dp10 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_blockpackingmode_bank1 : 1;  /* bit[0]  : dp10 blockpacking模式配置:(bank1)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP10_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP10_BLOCKCTRL3_BANK1_dp10_blockpackingmode_bank1_START  (0)
#define MST_DP10_BLOCKCTRL3_BANK1_dp10_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP10_LANECTRL_BANK1_UNION
 结构说明  : DP10_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0xA38，初值:0x00，宽度:8
 寄存器说明: dp10的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp10_lanecontrol_bank1 : 3;  /* bit[0-2]: dp10的数据线控制，表示dp10在哪根数据线上传输:(bank1)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP10_LANECTRL_BANK1_UNION;
#endif
#define MST_DP10_LANECTRL_BANK1_dp10_lanecontrol_bank1_START  (0)
#define MST_DP10_LANECTRL_BANK1_dp10_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP11_INTSTAT_UNION
 结构说明  : DP11_INTSTAT 寄存器结构定义。地址偏移量:0xB00，初值:0x00，宽度:8
 寄存器说明: dp11中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_test_fail_int  : 1;  /* bit[0]  : dp11 TestFail中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp11_port_ready_int : 1;  /* bit[1]  : dp11 PortReady中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  reserved_0          : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1          : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp11_cfg_error0_int : 1;  /* bit[5]  : dp11自定义中断1: (业务起来后，fifo空)
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp11_cfg_error1_int : 1;  /* bit[6]  : dp11自定义中断2: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp11_cfg_error2_int : 1;  /* bit[7]  : dp11自定义中断3: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
    } reg;
} MST_DP11_INTSTAT_UNION;
#endif
#define MST_DP11_INTSTAT_dp11_test_fail_int_START   (0)
#define MST_DP11_INTSTAT_dp11_test_fail_int_END     (0)
#define MST_DP11_INTSTAT_dp11_port_ready_int_START  (1)
#define MST_DP11_INTSTAT_dp11_port_ready_int_END    (1)
#define MST_DP11_INTSTAT_dp11_cfg_error0_int_START  (5)
#define MST_DP11_INTSTAT_dp11_cfg_error0_int_END    (5)
#define MST_DP11_INTSTAT_dp11_cfg_error1_int_START  (6)
#define MST_DP11_INTSTAT_dp11_cfg_error1_int_END    (6)
#define MST_DP11_INTSTAT_dp11_cfg_error2_int_START  (7)
#define MST_DP11_INTSTAT_dp11_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_INTMASK_UNION
 结构说明  : DP11_INTMASK 寄存器结构定义。地址偏移量:0xB01，初值:0x00，宽度:8
 寄存器说明: dp11中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_test_fail_int_mask  : 1;  /* bit[0]  : dp11 TestFail中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp11_port_ready_int_mask : 1;  /* bit[1]  : dp11 port Ready中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  reserved_0               : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1               : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp11_cfg_error0_int_mask : 1;  /* bit[5]  : dp11自定义中断1屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp11_cfg_error1_int_mask : 1;  /* bit[6]  : dp11自定义中断2屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp11_cfg_error2_int_mask : 1;  /* bit[7]  : dp11自定义中断3屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
    } reg;
} MST_DP11_INTMASK_UNION;
#endif
#define MST_DP11_INTMASK_dp11_test_fail_int_mask_START   (0)
#define MST_DP11_INTMASK_dp11_test_fail_int_mask_END     (0)
#define MST_DP11_INTMASK_dp11_port_ready_int_mask_START  (1)
#define MST_DP11_INTMASK_dp11_port_ready_int_mask_END    (1)
#define MST_DP11_INTMASK_dp11_cfg_error0_int_mask_START  (5)
#define MST_DP11_INTMASK_dp11_cfg_error0_int_mask_END    (5)
#define MST_DP11_INTMASK_dp11_cfg_error1_int_mask_START  (6)
#define MST_DP11_INTMASK_dp11_cfg_error1_int_mask_END    (6)
#define MST_DP11_INTMASK_dp11_cfg_error2_int_mask_START  (7)
#define MST_DP11_INTMASK_dp11_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_PORTCTRL_UNION
 结构说明  : DP11_PORTCTRL 寄存器结构定义。地址偏移量:0xB02，初值:0x00，宽度:8
 寄存器说明: dp11的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_portflowmode   : 2;  /* bit[0-1]: dp11流控模式选择：
                                                              2'b00:normal模式；
                                                              2'b01:push模式；
                                                              2'b10:pull模式； */
        unsigned char  dp11_portdatamode   : 2;  /* bit[2-3]: dp11 端口数据模式配置:
                                                              2'b00：Normal
                                                              2'b01：PRBS
                                                              2'b10：Static_0
                                                              2'b11：Static_1 */
        unsigned char  dp11_nextinvertbank : 1;  /* bit[4]  : dp11 NextInvertBank配置:
                                                              1'b0:取当前framectrl相同配置;
                                                              1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0          : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1          : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP11_PORTCTRL_UNION;
#endif
#define MST_DP11_PORTCTRL_dp11_portflowmode_START    (0)
#define MST_DP11_PORTCTRL_dp11_portflowmode_END      (1)
#define MST_DP11_PORTCTRL_dp11_portdatamode_START    (2)
#define MST_DP11_PORTCTRL_dp11_portdatamode_END      (3)
#define MST_DP11_PORTCTRL_dp11_nextinvertbank_START  (4)
#define MST_DP11_PORTCTRL_dp11_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP11_BLOCKCTRL1_UNION
 结构说明  : DP11_BLOCKCTRL1 寄存器结构定义。地址偏移量:0xB03，初值:0x1F，宽度:8
 寄存器说明: dp11 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_wordlength : 6;  /* bit[0-5]: dp11 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP11_BLOCKCTRL1_UNION;
#endif
#define MST_DP11_BLOCKCTRL1_dp11_wordlength_START  (0)
#define MST_DP11_BLOCKCTRL1_dp11_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP11_PREPARESTATUS_UNION
 结构说明  : DP11_PREPARESTATUS 寄存器结构定义。地址偏移量:0xB04，初值:0x00，宽度:8
 寄存器说明: dp11的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_n_finished_channel1 : 1;  /* bit[0]: dp11通道1状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel2 : 1;  /* bit[1]: dp11通道2状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel3 : 1;  /* bit[2]: dp11通道3状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel4 : 1;  /* bit[3]: dp11通道4状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel5 : 1;  /* bit[4]: dp11通道5状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel6 : 1;  /* bit[5]: dp11通道6状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel7 : 1;  /* bit[6]: dp11通道7状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp11_n_finished_channel8 : 1;  /* bit[7]: dp11通道8状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
    } reg;
} MST_DP11_PREPARESTATUS_UNION;
#endif
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel1_START  (0)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel1_END    (0)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel2_START  (1)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel2_END    (1)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel3_START  (2)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel3_END    (2)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel4_START  (3)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel4_END    (3)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel5_START  (4)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel5_END    (4)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel6_START  (5)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel6_END    (5)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel7_START  (6)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel7_END    (6)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel8_START  (7)
#define MST_DP11_PREPARESTATUS_dp11_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_PREPARECTRL_UNION
 结构说明  : DP11_PREPARECTRL 寄存器结构定义。地址偏移量:0xB05，初值:0x00，宽度:8
 寄存器说明: dp11的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_prepare_channel1 : 1;  /* bit[0]: dp11使能通道1准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel2 : 1;  /* bit[1]: dp11使能通道2准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel3 : 1;  /* bit[2]: dp11使能通道3准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel4 : 1;  /* bit[3]: dp11使能通道4准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel5 : 1;  /* bit[4]: dp11使能通道5准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel6 : 1;  /* bit[5]: dp11使能通道6准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel7 : 1;  /* bit[6]: dp11使能通道7准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp11_prepare_channel8 : 1;  /* bit[7]: dp11使能通道准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
    } reg;
} MST_DP11_PREPARECTRL_UNION;
#endif
#define MST_DP11_PREPARECTRL_dp11_prepare_channel1_START  (0)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel1_END    (0)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel2_START  (1)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel2_END    (1)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel3_START  (2)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel3_END    (2)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel4_START  (3)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel4_END    (3)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel5_START  (4)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel5_END    (4)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel6_START  (5)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel6_END    (5)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel7_START  (6)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel7_END    (6)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel8_START  (7)
#define MST_DP11_PREPARECTRL_dp11_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_CHANNELEN_BANK0_UNION
 结构说明  : DP11_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0xB20，初值:0x00，宽度:8
 寄存器说明: dp11的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_enable_channel1_bank0 : 1;  /* bit[0]: dp11的Channel1使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel2_bank0 : 1;  /* bit[1]: dp11的Channel2使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel3_bank0 : 1;  /* bit[2]: dp11的Channel3使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel4_bank0 : 1;  /* bit[3]: dp11的Channel4使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel5_bank0 : 1;  /* bit[4]: dp11的Channel5使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel6_bank0 : 1;  /* bit[5]: dp11的Channel6使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel7_bank0 : 1;  /* bit[6]: dp11的Channel7使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel8_bank0 : 1;  /* bit[7]: dp11的Channel8使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP11_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel1_bank0_START  (0)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel1_bank0_END    (0)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel2_bank0_START  (1)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel2_bank0_END    (1)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel3_bank0_START  (2)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel3_bank0_END    (2)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel4_bank0_START  (3)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel4_bank0_END    (3)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel5_bank0_START  (4)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel5_bank0_END    (4)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel6_bank0_START  (5)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel6_bank0_END    (5)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel7_bank0_START  (6)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel7_bank0_END    (6)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel8_bank0_START  (7)
#define MST_DP11_CHANNELEN_BANK0_dp11_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP11_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0xB21，初值:0x00，宽度:8
 寄存器说明: dp11 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp11 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP11_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP11_BLOCKCTRL2_BANK0_dp11_blockgroupcontrol_bank0_START  (0)
#define MST_DP11_BLOCKCTRL2_BANK0_dp11_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP11_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP11_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0xB22，初值:0x00，宽度:8
 寄存器说明: dp11的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp11的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP11_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP11_SAMPLECTRL1_BANK0_dp11_sampleintervallow_bank0_START  (0)
#define MST_DP11_SAMPLECTRL1_BANK0_dp11_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP11_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0xB23，初值:0x00，宽度:8
 寄存器说明: dp11的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp11的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP11_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP11_SAMPLECTRL2_BANK0_dp11_sampleintervalhigh_bank0_START  (0)
#define MST_DP11_SAMPLECTRL2_BANK0_dp11_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP11_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0xB24，初值:0x00，宽度:8
 寄存器说明: dp11的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_offset1_bank0 : 8;  /* bit[0-7]: dp11的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP11_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP11_OFFSETCTRL1_BANK0_dp11_offset1_bank0_START  (0)
#define MST_DP11_OFFSETCTRL1_BANK0_dp11_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP11_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0xB25，初值:0x00，宽度:8
 寄存器说明: dp11的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_offset2_bank0 : 8;  /* bit[0-7]: dp11的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP11_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP11_OFFSETCTRL2_BANK0_dp11_offset2_bank0_START  (0)
#define MST_DP11_OFFSETCTRL2_BANK0_dp11_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_HCTRL_BANK0_UNION
 结构说明  : DP11_HCTRL_BANK0 寄存器结构定义。地址偏移量:0xB26，初值:0x11，宽度:8
 寄存器说明: dp11的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_hstop_bank0  : 4;  /* bit[0-3]: dp11传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp11_hstart_bank0 : 4;  /* bit[4-7]: dp11传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP11_HCTRL_BANK0_UNION;
#endif
#define MST_DP11_HCTRL_BANK0_dp11_hstop_bank0_START   (0)
#define MST_DP11_HCTRL_BANK0_dp11_hstop_bank0_END     (3)
#define MST_DP11_HCTRL_BANK0_dp11_hstart_bank0_START  (4)
#define MST_DP11_HCTRL_BANK0_dp11_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP11_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0xB27，初值:0x00，宽度:8
 寄存器说明: dp11 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_blockpackingmode_bank0 : 1;  /* bit[0]  : dp11 blockpacking模式配置:(bank0)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP11_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP11_BLOCKCTRL3_BANK0_dp11_blockpackingmode_bank0_START  (0)
#define MST_DP11_BLOCKCTRL3_BANK0_dp11_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP11_LANECTRL_BANK0_UNION
 结构说明  : DP11_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0xB28，初值:0x00，宽度:8
 寄存器说明: dp11的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_lanecontrol_bank0 : 3;  /* bit[0-2]: dp11的数据线控制，表示dp11在哪根数据线上传输:(bank0)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP11_LANECTRL_BANK0_UNION;
#endif
#define MST_DP11_LANECTRL_BANK0_dp11_lanecontrol_bank0_START  (0)
#define MST_DP11_LANECTRL_BANK0_dp11_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP11_CHANNELEN_BANK1_UNION
 结构说明  : DP11_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0xB30，初值:0x00，宽度:8
 寄存器说明: dp11的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_enable_channel1_bank1 : 1;  /* bit[0]: dp11的Channel1使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel2_bank1 : 1;  /* bit[1]: dp11的Channel2使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel3_bank1 : 1;  /* bit[2]: dp11的Channel3使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel4_bank1 : 1;  /* bit[3]: dp11的Channel4使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel5_bank1 : 1;  /* bit[4]: dp11的Channel5使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel6_bank1 : 1;  /* bit[5]: dp11的Channel6使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel7_bank1 : 1;  /* bit[6]: dp11的Channel7使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp11_enable_channel8_bank1 : 1;  /* bit[7]: dp11的Channel8使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP11_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel1_bank1_START  (0)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel1_bank1_END    (0)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel2_bank1_START  (1)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel2_bank1_END    (1)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel3_bank1_START  (2)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel3_bank1_END    (2)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel4_bank1_START  (3)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel4_bank1_END    (3)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel5_bank1_START  (4)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel5_bank1_END    (4)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel6_bank1_START  (5)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel6_bank1_END    (5)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel7_bank1_START  (6)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel7_bank1_END    (6)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel8_bank1_START  (7)
#define MST_DP11_CHANNELEN_BANK1_dp11_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP11_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0xB31，初值:0x00，宽度:8
 寄存器说明: dp11 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp11 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP11_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP11_BLOCKCTRL2_BANK1_dp11_blockgroupcontrol_bank1_START  (0)
#define MST_DP11_BLOCKCTRL2_BANK1_dp11_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP11_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP11_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0xB32，初值:0x00，宽度:8
 寄存器说明: dp11的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp11的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP11_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP11_SAMPLECTRL1_BANK1_dp11_sampleintervallow_bank1_START  (0)
#define MST_DP11_SAMPLECTRL1_BANK1_dp11_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP11_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0xB33，初值:0x00，宽度:8
 寄存器说明: dp11的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp11的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP11_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP11_SAMPLECTRL2_BANK1_dp11_sampleintervalhigh_bank1_START  (0)
#define MST_DP11_SAMPLECTRL2_BANK1_dp11_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP11_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0xB34，初值:0x00，宽度:8
 寄存器说明: dp11的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_offset1_bank1 : 8;  /* bit[0-7]: dp11的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP11_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP11_OFFSETCTRL1_BANK1_dp11_offset1_bank1_START  (0)
#define MST_DP11_OFFSETCTRL1_BANK1_dp11_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP11_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0xB35，初值:0x00，宽度:8
 寄存器说明: dp11的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_offset2_bank1 : 8;  /* bit[0-7]: dp11的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP11_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP11_OFFSETCTRL2_BANK1_dp11_offset2_bank1_START  (0)
#define MST_DP11_OFFSETCTRL2_BANK1_dp11_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_HCTRL_BANK1_UNION
 结构说明  : DP11_HCTRL_BANK1 寄存器结构定义。地址偏移量:0xB36，初值:0x11，宽度:8
 寄存器说明: dp11的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_hstop_bank1  : 4;  /* bit[0-3]: dp11传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp11_hstart_bank1 : 4;  /* bit[4-7]: dp11传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP11_HCTRL_BANK1_UNION;
#endif
#define MST_DP11_HCTRL_BANK1_dp11_hstop_bank1_START   (0)
#define MST_DP11_HCTRL_BANK1_dp11_hstop_bank1_END     (3)
#define MST_DP11_HCTRL_BANK1_dp11_hstart_bank1_START  (4)
#define MST_DP11_HCTRL_BANK1_dp11_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP11_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP11_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0xB37，初值:0x00，宽度:8
 寄存器说明: dp11 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_blockpackingmode_bank1 : 1;  /* bit[0]  : dp11 blockpacking模式配置:(bank1)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP11_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP11_BLOCKCTRL3_BANK1_dp11_blockpackingmode_bank1_START  (0)
#define MST_DP11_BLOCKCTRL3_BANK1_dp11_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP11_LANECTRL_BANK1_UNION
 结构说明  : DP11_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0xB38，初值:0x00，宽度:8
 寄存器说明: dp11的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp11_lanecontrol_bank1 : 3;  /* bit[0-2]: dp11的数据线控制，表示dp11在哪根数据线上传输:(bank1)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP11_LANECTRL_BANK1_UNION;
#endif
#define MST_DP11_LANECTRL_BANK1_dp11_lanecontrol_bank1_START  (0)
#define MST_DP11_LANECTRL_BANK1_dp11_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP12_INTSTAT_UNION
 结构说明  : DP12_INTSTAT 寄存器结构定义。地址偏移量:0xC00，初值:0x00，宽度:8
 寄存器说明: dp12中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_test_fail_int  : 1;  /* bit[0]  : dp12 TestFail中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp12_port_ready_int : 1;  /* bit[1]  : dp12 PortReady中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  reserved_0          : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1          : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp12_cfg_error0_int : 1;  /* bit[5]  : dp12自定义中断1: (业务起来后，fifo空)
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp12_cfg_error1_int : 1;  /* bit[6]  : dp12自定义中断2: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp12_cfg_error2_int : 1;  /* bit[7]  : dp12自定义中断3: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
    } reg;
} MST_DP12_INTSTAT_UNION;
#endif
#define MST_DP12_INTSTAT_dp12_test_fail_int_START   (0)
#define MST_DP12_INTSTAT_dp12_test_fail_int_END     (0)
#define MST_DP12_INTSTAT_dp12_port_ready_int_START  (1)
#define MST_DP12_INTSTAT_dp12_port_ready_int_END    (1)
#define MST_DP12_INTSTAT_dp12_cfg_error0_int_START  (5)
#define MST_DP12_INTSTAT_dp12_cfg_error0_int_END    (5)
#define MST_DP12_INTSTAT_dp12_cfg_error1_int_START  (6)
#define MST_DP12_INTSTAT_dp12_cfg_error1_int_END    (6)
#define MST_DP12_INTSTAT_dp12_cfg_error2_int_START  (7)
#define MST_DP12_INTSTAT_dp12_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_INTMASK_UNION
 结构说明  : DP12_INTMASK 寄存器结构定义。地址偏移量:0xC01，初值:0x00，宽度:8
 寄存器说明: dp12中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_test_fail_int_mask  : 1;  /* bit[0]  : dp12 TestFail中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp12_port_ready_int_mask : 1;  /* bit[1]  : dp12 port Ready中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  reserved_0               : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1               : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp12_cfg_error0_int_mask : 1;  /* bit[5]  : dp12自定义中断1屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp12_cfg_error1_int_mask : 1;  /* bit[6]  : dp12自定义中断2屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp12_cfg_error2_int_mask : 1;  /* bit[7]  : dp12自定义中断3屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
    } reg;
} MST_DP12_INTMASK_UNION;
#endif
#define MST_DP12_INTMASK_dp12_test_fail_int_mask_START   (0)
#define MST_DP12_INTMASK_dp12_test_fail_int_mask_END     (0)
#define MST_DP12_INTMASK_dp12_port_ready_int_mask_START  (1)
#define MST_DP12_INTMASK_dp12_port_ready_int_mask_END    (1)
#define MST_DP12_INTMASK_dp12_cfg_error0_int_mask_START  (5)
#define MST_DP12_INTMASK_dp12_cfg_error0_int_mask_END    (5)
#define MST_DP12_INTMASK_dp12_cfg_error1_int_mask_START  (6)
#define MST_DP12_INTMASK_dp12_cfg_error1_int_mask_END    (6)
#define MST_DP12_INTMASK_dp12_cfg_error2_int_mask_START  (7)
#define MST_DP12_INTMASK_dp12_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_PORTCTRL_UNION
 结构说明  : DP12_PORTCTRL 寄存器结构定义。地址偏移量:0xC02，初值:0x00，宽度:8
 寄存器说明: dp12的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_portflowmode   : 2;  /* bit[0-1]: dp12流控模式选择：
                                                              2'b00:normal模式；
                                                              2'b01:push模式；
                                                              2'b10:pull模式； */
        unsigned char  dp12_portdatamode   : 2;  /* bit[2-3]: dp12 端口数据模式配置:
                                                              2'b00：Normal
                                                              2'b01：PRBS
                                                              2'b10：Static_0
                                                              2'b11：Static_1 */
        unsigned char  dp12_nextinvertbank : 1;  /* bit[4]  : dp12 NextInvertBank配置:
                                                              1'b0:取当前framectrl相同配置;
                                                              1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0          : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1          : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP12_PORTCTRL_UNION;
#endif
#define MST_DP12_PORTCTRL_dp12_portflowmode_START    (0)
#define MST_DP12_PORTCTRL_dp12_portflowmode_END      (1)
#define MST_DP12_PORTCTRL_dp12_portdatamode_START    (2)
#define MST_DP12_PORTCTRL_dp12_portdatamode_END      (3)
#define MST_DP12_PORTCTRL_dp12_nextinvertbank_START  (4)
#define MST_DP12_PORTCTRL_dp12_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP12_BLOCKCTRL1_UNION
 结构说明  : DP12_BLOCKCTRL1 寄存器结构定义。地址偏移量:0xC03，初值:0x1F，宽度:8
 寄存器说明: dp12 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_wordlength : 6;  /* bit[0-5]: dp12 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP12_BLOCKCTRL1_UNION;
#endif
#define MST_DP12_BLOCKCTRL1_dp12_wordlength_START  (0)
#define MST_DP12_BLOCKCTRL1_dp12_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP12_PREPARESTATUS_UNION
 结构说明  : DP12_PREPARESTATUS 寄存器结构定义。地址偏移量:0xC04，初值:0x00，宽度:8
 寄存器说明: dp12的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_n_finished_channel1 : 1;  /* bit[0]: dp12通道1状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel2 : 1;  /* bit[1]: dp12通道2状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel3 : 1;  /* bit[2]: dp12通道3状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel4 : 1;  /* bit[3]: dp12通道4状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel5 : 1;  /* bit[4]: dp12通道5状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel6 : 1;  /* bit[5]: dp12通道6状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel7 : 1;  /* bit[6]: dp12通道7状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp12_n_finished_channel8 : 1;  /* bit[7]: dp12通道8状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
    } reg;
} MST_DP12_PREPARESTATUS_UNION;
#endif
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel1_START  (0)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel1_END    (0)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel2_START  (1)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel2_END    (1)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel3_START  (2)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel3_END    (2)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel4_START  (3)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel4_END    (3)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel5_START  (4)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel5_END    (4)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel6_START  (5)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel6_END    (5)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel7_START  (6)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel7_END    (6)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel8_START  (7)
#define MST_DP12_PREPARESTATUS_dp12_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_PREPARECTRL_UNION
 结构说明  : DP12_PREPARECTRL 寄存器结构定义。地址偏移量:0xC05，初值:0x00，宽度:8
 寄存器说明: dp12的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_prepare_channel1 : 1;  /* bit[0]: dp12使能通道1准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel2 : 1;  /* bit[1]: dp12使能通道2准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel3 : 1;  /* bit[2]: dp12使能通道3准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel4 : 1;  /* bit[3]: dp12使能通道4准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel5 : 1;  /* bit[4]: dp12使能通道5准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel6 : 1;  /* bit[5]: dp12使能通道6准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel7 : 1;  /* bit[6]: dp12使能通道7准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp12_prepare_channel8 : 1;  /* bit[7]: dp12使能通道准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
    } reg;
} MST_DP12_PREPARECTRL_UNION;
#endif
#define MST_DP12_PREPARECTRL_dp12_prepare_channel1_START  (0)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel1_END    (0)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel2_START  (1)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel2_END    (1)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel3_START  (2)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel3_END    (2)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel4_START  (3)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel4_END    (3)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel5_START  (4)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel5_END    (4)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel6_START  (5)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel6_END    (5)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel7_START  (6)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel7_END    (6)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel8_START  (7)
#define MST_DP12_PREPARECTRL_dp12_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_CHANNELEN_BANK0_UNION
 结构说明  : DP12_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0xC20，初值:0x00，宽度:8
 寄存器说明: dp12的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_enable_channel1_bank0 : 1;  /* bit[0]: dp12的Channel1使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel2_bank0 : 1;  /* bit[1]: dp12的Channel2使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel3_bank0 : 1;  /* bit[2]: dp12的Channel3使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel4_bank0 : 1;  /* bit[3]: dp12的Channel4使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel5_bank0 : 1;  /* bit[4]: dp12的Channel5使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel6_bank0 : 1;  /* bit[5]: dp12的Channel6使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel7_bank0 : 1;  /* bit[6]: dp12的Channel7使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel8_bank0 : 1;  /* bit[7]: dp12的Channel8使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP12_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel1_bank0_START  (0)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel1_bank0_END    (0)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel2_bank0_START  (1)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel2_bank0_END    (1)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel3_bank0_START  (2)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel3_bank0_END    (2)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel4_bank0_START  (3)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel4_bank0_END    (3)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel5_bank0_START  (4)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel5_bank0_END    (4)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel6_bank0_START  (5)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel6_bank0_END    (5)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel7_bank0_START  (6)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel7_bank0_END    (6)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel8_bank0_START  (7)
#define MST_DP12_CHANNELEN_BANK0_dp12_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP12_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0xC21，初值:0x00，宽度:8
 寄存器说明: dp12 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp12 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP12_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP12_BLOCKCTRL2_BANK0_dp12_blockgroupcontrol_bank0_START  (0)
#define MST_DP12_BLOCKCTRL2_BANK0_dp12_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP12_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP12_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0xC22，初值:0x00，宽度:8
 寄存器说明: dp12的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp12的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP12_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP12_SAMPLECTRL1_BANK0_dp12_sampleintervallow_bank0_START  (0)
#define MST_DP12_SAMPLECTRL1_BANK0_dp12_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP12_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0xC23，初值:0x00，宽度:8
 寄存器说明: dp12的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp12的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP12_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP12_SAMPLECTRL2_BANK0_dp12_sampleintervalhigh_bank0_START  (0)
#define MST_DP12_SAMPLECTRL2_BANK0_dp12_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP12_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0xC24，初值:0x00，宽度:8
 寄存器说明: dp12的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_offset1_bank0 : 8;  /* bit[0-7]: dp12的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP12_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP12_OFFSETCTRL1_BANK0_dp12_offset1_bank0_START  (0)
#define MST_DP12_OFFSETCTRL1_BANK0_dp12_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP12_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0xC25，初值:0x00，宽度:8
 寄存器说明: dp12的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_offset2_bank0 : 8;  /* bit[0-7]: dp12的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP12_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP12_OFFSETCTRL2_BANK0_dp12_offset2_bank0_START  (0)
#define MST_DP12_OFFSETCTRL2_BANK0_dp12_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_HCTRL_BANK0_UNION
 结构说明  : DP12_HCTRL_BANK0 寄存器结构定义。地址偏移量:0xC26，初值:0x11，宽度:8
 寄存器说明: dp12的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_hstop_bank0  : 4;  /* bit[0-3]: dp12传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp12_hstart_bank0 : 4;  /* bit[4-7]: dp12传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP12_HCTRL_BANK0_UNION;
#endif
#define MST_DP12_HCTRL_BANK0_dp12_hstop_bank0_START   (0)
#define MST_DP12_HCTRL_BANK0_dp12_hstop_bank0_END     (3)
#define MST_DP12_HCTRL_BANK0_dp12_hstart_bank0_START  (4)
#define MST_DP12_HCTRL_BANK0_dp12_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP12_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0xC27，初值:0x00，宽度:8
 寄存器说明: dp12 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_blockpackingmode_bank0 : 1;  /* bit[0]  : dp12 blockpacking模式配置:(bank0)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP12_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP12_BLOCKCTRL3_BANK0_dp12_blockpackingmode_bank0_START  (0)
#define MST_DP12_BLOCKCTRL3_BANK0_dp12_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP12_LANECTRL_BANK0_UNION
 结构说明  : DP12_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0xC28，初值:0x00，宽度:8
 寄存器说明: dp12的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_lanecontrol_bank0 : 3;  /* bit[0-2]: dp12的数据线控制，表示dp12在哪根数据线上传输:(bank0)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP12_LANECTRL_BANK0_UNION;
#endif
#define MST_DP12_LANECTRL_BANK0_dp12_lanecontrol_bank0_START  (0)
#define MST_DP12_LANECTRL_BANK0_dp12_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP12_CHANNELEN_BANK1_UNION
 结构说明  : DP12_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0xC30，初值:0x00，宽度:8
 寄存器说明: dp12的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_enable_channel1_bank1 : 1;  /* bit[0]: dp12的Channel1使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel2_bank1 : 1;  /* bit[1]: dp12的Channel2使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel3_bank1 : 1;  /* bit[2]: dp12的Channel3使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel4_bank1 : 1;  /* bit[3]: dp12的Channel4使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel5_bank1 : 1;  /* bit[4]: dp12的Channel5使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel6_bank1 : 1;  /* bit[5]: dp12的Channel6使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel7_bank1 : 1;  /* bit[6]: dp12的Channel7使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp12_enable_channel8_bank1 : 1;  /* bit[7]: dp12的Channel8使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP12_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel1_bank1_START  (0)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel1_bank1_END    (0)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel2_bank1_START  (1)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel2_bank1_END    (1)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel3_bank1_START  (2)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel3_bank1_END    (2)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel4_bank1_START  (3)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel4_bank1_END    (3)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel5_bank1_START  (4)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel5_bank1_END    (4)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel6_bank1_START  (5)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel6_bank1_END    (5)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel7_bank1_START  (6)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel7_bank1_END    (6)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel8_bank1_START  (7)
#define MST_DP12_CHANNELEN_BANK1_dp12_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP12_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0xC31，初值:0x00，宽度:8
 寄存器说明: dp12 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp12 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP12_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP12_BLOCKCTRL2_BANK1_dp12_blockgroupcontrol_bank1_START  (0)
#define MST_DP12_BLOCKCTRL2_BANK1_dp12_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP12_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP12_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0xC32，初值:0x00，宽度:8
 寄存器说明: dp12的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp12的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP12_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP12_SAMPLECTRL1_BANK1_dp12_sampleintervallow_bank1_START  (0)
#define MST_DP12_SAMPLECTRL1_BANK1_dp12_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP12_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0xC33，初值:0x00，宽度:8
 寄存器说明: dp12的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp12的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP12_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP12_SAMPLECTRL2_BANK1_dp12_sampleintervalhigh_bank1_START  (0)
#define MST_DP12_SAMPLECTRL2_BANK1_dp12_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP12_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0xC34，初值:0x00，宽度:8
 寄存器说明: dp12的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_offset1_bank1 : 8;  /* bit[0-7]: dp12的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP12_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP12_OFFSETCTRL1_BANK1_dp12_offset1_bank1_START  (0)
#define MST_DP12_OFFSETCTRL1_BANK1_dp12_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP12_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0xC35，初值:0x00，宽度:8
 寄存器说明: dp12的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_offset2_bank1 : 8;  /* bit[0-7]: dp12的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP12_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP12_OFFSETCTRL2_BANK1_dp12_offset2_bank1_START  (0)
#define MST_DP12_OFFSETCTRL2_BANK1_dp12_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_HCTRL_BANK1_UNION
 结构说明  : DP12_HCTRL_BANK1 寄存器结构定义。地址偏移量:0xC36，初值:0x11，宽度:8
 寄存器说明: dp12的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_hstop_bank1  : 4;  /* bit[0-3]: dp12传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp12_hstart_bank1 : 4;  /* bit[4-7]: dp12传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP12_HCTRL_BANK1_UNION;
#endif
#define MST_DP12_HCTRL_BANK1_dp12_hstop_bank1_START   (0)
#define MST_DP12_HCTRL_BANK1_dp12_hstop_bank1_END     (3)
#define MST_DP12_HCTRL_BANK1_dp12_hstart_bank1_START  (4)
#define MST_DP12_HCTRL_BANK1_dp12_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP12_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP12_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0xC37，初值:0x00，宽度:8
 寄存器说明: dp12 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_blockpackingmode_bank1 : 1;  /* bit[0]  : dp12 blockpacking模式配置:(bank1)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP12_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP12_BLOCKCTRL3_BANK1_dp12_blockpackingmode_bank1_START  (0)
#define MST_DP12_BLOCKCTRL3_BANK1_dp12_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP12_LANECTRL_BANK1_UNION
 结构说明  : DP12_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0xC38，初值:0x00，宽度:8
 寄存器说明: dp12的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp12_lanecontrol_bank1 : 3;  /* bit[0-2]: dp12的数据线控制，表示dp12在哪根数据线上传输:(bank1)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP12_LANECTRL_BANK1_UNION;
#endif
#define MST_DP12_LANECTRL_BANK1_dp12_lanecontrol_bank1_START  (0)
#define MST_DP12_LANECTRL_BANK1_dp12_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP13_INTSTAT_UNION
 结构说明  : DP13_INTSTAT 寄存器结构定义。地址偏移量:0xD00，初值:0x00，宽度:8
 寄存器说明: dp13中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_test_fail_int  : 1;  /* bit[0]  : dp13 TestFail中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp13_port_ready_int : 1;  /* bit[1]  : dp13 PortReady中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  reserved_0          : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1          : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp13_cfg_error0_int : 1;  /* bit[5]  : dp13自定义中断1: (业务起来后，fifo空)
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp13_cfg_error1_int : 1;  /* bit[6]  : dp13自定义中断2: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp13_cfg_error2_int : 1;  /* bit[7]  : dp13自定义中断3: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
    } reg;
} MST_DP13_INTSTAT_UNION;
#endif
#define MST_DP13_INTSTAT_dp13_test_fail_int_START   (0)
#define MST_DP13_INTSTAT_dp13_test_fail_int_END     (0)
#define MST_DP13_INTSTAT_dp13_port_ready_int_START  (1)
#define MST_DP13_INTSTAT_dp13_port_ready_int_END    (1)
#define MST_DP13_INTSTAT_dp13_cfg_error0_int_START  (5)
#define MST_DP13_INTSTAT_dp13_cfg_error0_int_END    (5)
#define MST_DP13_INTSTAT_dp13_cfg_error1_int_START  (6)
#define MST_DP13_INTSTAT_dp13_cfg_error1_int_END    (6)
#define MST_DP13_INTSTAT_dp13_cfg_error2_int_START  (7)
#define MST_DP13_INTSTAT_dp13_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_INTMASK_UNION
 结构说明  : DP13_INTMASK 寄存器结构定义。地址偏移量:0xD01，初值:0x00，宽度:8
 寄存器说明: dp13中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_test_fail_int_mask  : 1;  /* bit[0]  : dp13 TestFail中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp13_port_ready_int_mask : 1;  /* bit[1]  : dp13 port Ready中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  reserved_0               : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1               : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp13_cfg_error0_int_mask : 1;  /* bit[5]  : dp13自定义中断1屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp13_cfg_error1_int_mask : 1;  /* bit[6]  : dp13自定义中断2屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp13_cfg_error2_int_mask : 1;  /* bit[7]  : dp13自定义中断3屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
    } reg;
} MST_DP13_INTMASK_UNION;
#endif
#define MST_DP13_INTMASK_dp13_test_fail_int_mask_START   (0)
#define MST_DP13_INTMASK_dp13_test_fail_int_mask_END     (0)
#define MST_DP13_INTMASK_dp13_port_ready_int_mask_START  (1)
#define MST_DP13_INTMASK_dp13_port_ready_int_mask_END    (1)
#define MST_DP13_INTMASK_dp13_cfg_error0_int_mask_START  (5)
#define MST_DP13_INTMASK_dp13_cfg_error0_int_mask_END    (5)
#define MST_DP13_INTMASK_dp13_cfg_error1_int_mask_START  (6)
#define MST_DP13_INTMASK_dp13_cfg_error1_int_mask_END    (6)
#define MST_DP13_INTMASK_dp13_cfg_error2_int_mask_START  (7)
#define MST_DP13_INTMASK_dp13_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_PORTCTRL_UNION
 结构说明  : DP13_PORTCTRL 寄存器结构定义。地址偏移量:0xD02，初值:0x00，宽度:8
 寄存器说明: dp13的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_portflowmode   : 2;  /* bit[0-1]: dp13流控模式选择：
                                                              2'b00:normal模式；
                                                              2'b01:push模式；
                                                              2'b10:pull模式； */
        unsigned char  dp13_portdatamode   : 2;  /* bit[2-3]: dp13 端口数据模式配置:
                                                              2'b00：Normal
                                                              2'b01：PRBS
                                                              2'b10：Static_0
                                                              2'b11：Static_1 */
        unsigned char  dp13_nextinvertbank : 1;  /* bit[4]  : dp13 NextInvertBank配置:
                                                              1'b0:取当前framectrl相同配置;
                                                              1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0          : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1          : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP13_PORTCTRL_UNION;
#endif
#define MST_DP13_PORTCTRL_dp13_portflowmode_START    (0)
#define MST_DP13_PORTCTRL_dp13_portflowmode_END      (1)
#define MST_DP13_PORTCTRL_dp13_portdatamode_START    (2)
#define MST_DP13_PORTCTRL_dp13_portdatamode_END      (3)
#define MST_DP13_PORTCTRL_dp13_nextinvertbank_START  (4)
#define MST_DP13_PORTCTRL_dp13_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP13_BLOCKCTRL1_UNION
 结构说明  : DP13_BLOCKCTRL1 寄存器结构定义。地址偏移量:0xD03，初值:0x1F，宽度:8
 寄存器说明: dp13 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_wordlength : 6;  /* bit[0-5]: dp13 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP13_BLOCKCTRL1_UNION;
#endif
#define MST_DP13_BLOCKCTRL1_dp13_wordlength_START  (0)
#define MST_DP13_BLOCKCTRL1_dp13_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP13_PREPARESTATUS_UNION
 结构说明  : DP13_PREPARESTATUS 寄存器结构定义。地址偏移量:0xD04，初值:0x00，宽度:8
 寄存器说明: dp13的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_n_finished_channel1 : 1;  /* bit[0]: dp13通道1状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel2 : 1;  /* bit[1]: dp13通道2状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel3 : 1;  /* bit[2]: dp13通道3状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel4 : 1;  /* bit[3]: dp13通道4状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel5 : 1;  /* bit[4]: dp13通道5状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel6 : 1;  /* bit[5]: dp13通道6状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel7 : 1;  /* bit[6]: dp13通道7状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp13_n_finished_channel8 : 1;  /* bit[7]: dp13通道8状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
    } reg;
} MST_DP13_PREPARESTATUS_UNION;
#endif
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel1_START  (0)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel1_END    (0)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel2_START  (1)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel2_END    (1)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel3_START  (2)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel3_END    (2)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel4_START  (3)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel4_END    (3)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel5_START  (4)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel5_END    (4)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel6_START  (5)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel6_END    (5)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel7_START  (6)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel7_END    (6)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel8_START  (7)
#define MST_DP13_PREPARESTATUS_dp13_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_PREPARECTRL_UNION
 结构说明  : DP13_PREPARECTRL 寄存器结构定义。地址偏移量:0xD05，初值:0x00，宽度:8
 寄存器说明: dp13的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_prepare_channel1 : 1;  /* bit[0]: dp13使能通道1准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel2 : 1;  /* bit[1]: dp13使能通道2准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel3 : 1;  /* bit[2]: dp13使能通道3准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel4 : 1;  /* bit[3]: dp13使能通道4准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel5 : 1;  /* bit[4]: dp13使能通道5准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel6 : 1;  /* bit[5]: dp13使能通道6准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel7 : 1;  /* bit[6]: dp13使能通道7准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp13_prepare_channel8 : 1;  /* bit[7]: dp13使能通道准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
    } reg;
} MST_DP13_PREPARECTRL_UNION;
#endif
#define MST_DP13_PREPARECTRL_dp13_prepare_channel1_START  (0)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel1_END    (0)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel2_START  (1)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel2_END    (1)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel3_START  (2)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel3_END    (2)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel4_START  (3)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel4_END    (3)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel5_START  (4)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel5_END    (4)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel6_START  (5)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel6_END    (5)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel7_START  (6)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel7_END    (6)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel8_START  (7)
#define MST_DP13_PREPARECTRL_dp13_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_CHANNELEN_BANK0_UNION
 结构说明  : DP13_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0xD20，初值:0x00，宽度:8
 寄存器说明: dp13的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_enable_channel1_bank0 : 1;  /* bit[0]: dp13的Channel1使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel2_bank0 : 1;  /* bit[1]: dp13的Channel2使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel3_bank0 : 1;  /* bit[2]: dp13的Channel3使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel4_bank0 : 1;  /* bit[3]: dp13的Channel4使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel5_bank0 : 1;  /* bit[4]: dp13的Channel5使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel6_bank0 : 1;  /* bit[5]: dp13的Channel6使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel7_bank0 : 1;  /* bit[6]: dp13的Channel7使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel8_bank0 : 1;  /* bit[7]: dp13的Channel8使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP13_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel1_bank0_START  (0)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel1_bank0_END    (0)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel2_bank0_START  (1)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel2_bank0_END    (1)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel3_bank0_START  (2)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel3_bank0_END    (2)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel4_bank0_START  (3)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel4_bank0_END    (3)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel5_bank0_START  (4)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel5_bank0_END    (4)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel6_bank0_START  (5)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel6_bank0_END    (5)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel7_bank0_START  (6)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel7_bank0_END    (6)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel8_bank0_START  (7)
#define MST_DP13_CHANNELEN_BANK0_dp13_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP13_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0xD21，初值:0x00，宽度:8
 寄存器说明: dp13 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp13 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP13_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP13_BLOCKCTRL2_BANK0_dp13_blockgroupcontrol_bank0_START  (0)
#define MST_DP13_BLOCKCTRL2_BANK0_dp13_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP13_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP13_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0xD22，初值:0x00，宽度:8
 寄存器说明: dp13的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp13的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP13_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP13_SAMPLECTRL1_BANK0_dp13_sampleintervallow_bank0_START  (0)
#define MST_DP13_SAMPLECTRL1_BANK0_dp13_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP13_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0xD23，初值:0x00，宽度:8
 寄存器说明: dp13的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp13的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP13_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP13_SAMPLECTRL2_BANK0_dp13_sampleintervalhigh_bank0_START  (0)
#define MST_DP13_SAMPLECTRL2_BANK0_dp13_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP13_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0xD24，初值:0x00，宽度:8
 寄存器说明: dp13的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_offset1_bank0 : 8;  /* bit[0-7]: dp13的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP13_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP13_OFFSETCTRL1_BANK0_dp13_offset1_bank0_START  (0)
#define MST_DP13_OFFSETCTRL1_BANK0_dp13_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP13_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0xD25，初值:0x00，宽度:8
 寄存器说明: dp13的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_offset2_bank0 : 8;  /* bit[0-7]: dp13的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP13_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP13_OFFSETCTRL2_BANK0_dp13_offset2_bank0_START  (0)
#define MST_DP13_OFFSETCTRL2_BANK0_dp13_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_HCTRL_BANK0_UNION
 结构说明  : DP13_HCTRL_BANK0 寄存器结构定义。地址偏移量:0xD26，初值:0x11，宽度:8
 寄存器说明: dp13的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_hstop_bank0  : 4;  /* bit[0-3]: dp13传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp13_hstart_bank0 : 4;  /* bit[4-7]: dp13传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP13_HCTRL_BANK0_UNION;
#endif
#define MST_DP13_HCTRL_BANK0_dp13_hstop_bank0_START   (0)
#define MST_DP13_HCTRL_BANK0_dp13_hstop_bank0_END     (3)
#define MST_DP13_HCTRL_BANK0_dp13_hstart_bank0_START  (4)
#define MST_DP13_HCTRL_BANK0_dp13_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP13_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0xD27，初值:0x00，宽度:8
 寄存器说明: dp13 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_blockpackingmode_bank0 : 1;  /* bit[0]  : dp13 blockpacking模式配置:(bank0)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP13_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP13_BLOCKCTRL3_BANK0_dp13_blockpackingmode_bank0_START  (0)
#define MST_DP13_BLOCKCTRL3_BANK0_dp13_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP13_LANECTRL_BANK0_UNION
 结构说明  : DP13_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0xD28，初值:0x00，宽度:8
 寄存器说明: dp13的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_lanecontrol_bank0 : 3;  /* bit[0-2]: dp13的数据线控制，表示dp13在哪根数据线上传输:(bank0)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP13_LANECTRL_BANK0_UNION;
#endif
#define MST_DP13_LANECTRL_BANK0_dp13_lanecontrol_bank0_START  (0)
#define MST_DP13_LANECTRL_BANK0_dp13_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP13_CHANNELEN_BANK1_UNION
 结构说明  : DP13_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0xD30，初值:0x00，宽度:8
 寄存器说明: dp13的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_enable_channel1_bank1 : 1;  /* bit[0]: dp13的Channel1使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel2_bank1 : 1;  /* bit[1]: dp13的Channel2使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel3_bank1 : 1;  /* bit[2]: dp13的Channel3使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel4_bank1 : 1;  /* bit[3]: dp13的Channel4使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel5_bank1 : 1;  /* bit[4]: dp13的Channel5使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel6_bank1 : 1;  /* bit[5]: dp13的Channel6使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel7_bank1 : 1;  /* bit[6]: dp13的Channel7使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp13_enable_channel8_bank1 : 1;  /* bit[7]: dp13的Channel8使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP13_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel1_bank1_START  (0)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel1_bank1_END    (0)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel2_bank1_START  (1)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel2_bank1_END    (1)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel3_bank1_START  (2)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel3_bank1_END    (2)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel4_bank1_START  (3)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel4_bank1_END    (3)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel5_bank1_START  (4)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel5_bank1_END    (4)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel6_bank1_START  (5)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel6_bank1_END    (5)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel7_bank1_START  (6)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel7_bank1_END    (6)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel8_bank1_START  (7)
#define MST_DP13_CHANNELEN_BANK1_dp13_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP13_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0xD31，初值:0x00，宽度:8
 寄存器说明: dp13 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp13 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP13_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP13_BLOCKCTRL2_BANK1_dp13_blockgroupcontrol_bank1_START  (0)
#define MST_DP13_BLOCKCTRL2_BANK1_dp13_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP13_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP13_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0xD32，初值:0x00，宽度:8
 寄存器说明: dp13的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp13的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP13_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP13_SAMPLECTRL1_BANK1_dp13_sampleintervallow_bank1_START  (0)
#define MST_DP13_SAMPLECTRL1_BANK1_dp13_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP13_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0xD33，初值:0x00，宽度:8
 寄存器说明: dp13的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp13的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP13_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP13_SAMPLECTRL2_BANK1_dp13_sampleintervalhigh_bank1_START  (0)
#define MST_DP13_SAMPLECTRL2_BANK1_dp13_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP13_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0xD34，初值:0x00，宽度:8
 寄存器说明: dp13的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_offset1_bank1 : 8;  /* bit[0-7]: dp13的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP13_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP13_OFFSETCTRL1_BANK1_dp13_offset1_bank1_START  (0)
#define MST_DP13_OFFSETCTRL1_BANK1_dp13_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP13_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0xD35，初值:0x00，宽度:8
 寄存器说明: dp13的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_offset2_bank1 : 8;  /* bit[0-7]: dp13的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP13_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP13_OFFSETCTRL2_BANK1_dp13_offset2_bank1_START  (0)
#define MST_DP13_OFFSETCTRL2_BANK1_dp13_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_HCTRL_BANK1_UNION
 结构说明  : DP13_HCTRL_BANK1 寄存器结构定义。地址偏移量:0xD36，初值:0x11，宽度:8
 寄存器说明: dp13的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_hstop_bank1  : 4;  /* bit[0-3]: dp13传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp13_hstart_bank1 : 4;  /* bit[4-7]: dp13传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP13_HCTRL_BANK1_UNION;
#endif
#define MST_DP13_HCTRL_BANK1_dp13_hstop_bank1_START   (0)
#define MST_DP13_HCTRL_BANK1_dp13_hstop_bank1_END     (3)
#define MST_DP13_HCTRL_BANK1_dp13_hstart_bank1_START  (4)
#define MST_DP13_HCTRL_BANK1_dp13_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP13_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP13_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0xD37，初值:0x00，宽度:8
 寄存器说明: dp13 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_blockpackingmode_bank1 : 1;  /* bit[0]  : dp13 blockpacking模式配置:(bank1)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP13_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP13_BLOCKCTRL3_BANK1_dp13_blockpackingmode_bank1_START  (0)
#define MST_DP13_BLOCKCTRL3_BANK1_dp13_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP13_LANECTRL_BANK1_UNION
 结构说明  : DP13_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0xD38，初值:0x00，宽度:8
 寄存器说明: dp13的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp13_lanecontrol_bank1 : 3;  /* bit[0-2]: dp13的数据线控制，表示dp13在哪根数据线上传输:(bank1)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP13_LANECTRL_BANK1_UNION;
#endif
#define MST_DP13_LANECTRL_BANK1_dp13_lanecontrol_bank1_START  (0)
#define MST_DP13_LANECTRL_BANK1_dp13_lanecontrol_bank1_END    (2)


/*****************************************************************************
 结构名    : MST_DP14_INTSTAT_UNION
 结构说明  : DP14_INTSTAT 寄存器结构定义。地址偏移量:0xE00，初值:0x00，宽度:8
 寄存器说明: dp14中断状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_test_fail_int  : 1;  /* bit[0]  : dp14 TestFail中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp14_port_ready_int : 1;  /* bit[1]  : dp14 PortReady中断:
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  reserved_0          : 1;  /* bit[2]  : 保留。 */
        unsigned char  reserved_1          : 2;  /* bit[3-4]: 保留。 */
        unsigned char  dp14_cfg_error0_int : 1;  /* bit[5]  : dp14自定义中断1: (业务起来后，fifo空)
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp14_cfg_error1_int : 1;  /* bit[6]  : dp14自定义中断2: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
        unsigned char  dp14_cfg_error2_int : 1;  /* bit[7]  : dp14自定义中断3: 
                                                              1：写1时清除中断，查询时表示中断有效；
                                                              0：查询时表示中断无效。 */
    } reg;
} MST_DP14_INTSTAT_UNION;
#endif
#define MST_DP14_INTSTAT_dp14_test_fail_int_START   (0)
#define MST_DP14_INTSTAT_dp14_test_fail_int_END     (0)
#define MST_DP14_INTSTAT_dp14_port_ready_int_START  (1)
#define MST_DP14_INTSTAT_dp14_port_ready_int_END    (1)
#define MST_DP14_INTSTAT_dp14_cfg_error0_int_START  (5)
#define MST_DP14_INTSTAT_dp14_cfg_error0_int_END    (5)
#define MST_DP14_INTSTAT_dp14_cfg_error1_int_START  (6)
#define MST_DP14_INTSTAT_dp14_cfg_error1_int_END    (6)
#define MST_DP14_INTSTAT_dp14_cfg_error2_int_START  (7)
#define MST_DP14_INTSTAT_dp14_cfg_error2_int_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_INTMASK_UNION
 结构说明  : DP14_INTMASK 寄存器结构定义。地址偏移量:0xE01，初值:0x00，宽度:8
 寄存器说明: dp14中断屏蔽寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_test_fail_int_mask  : 1;  /* bit[0]  : dp14 TestFail中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp14_port_ready_int_mask : 1;  /* bit[1]  : dp14 port Ready中断屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  reserved_0               : 1;  /* bit[2]  : 保留; */
        unsigned char  reserved_1               : 2;  /* bit[3-4]: 保留; */
        unsigned char  dp14_cfg_error0_int_mask : 1;  /* bit[5]  : dp14自定义中断1屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp14_cfg_error1_int_mask : 1;  /* bit[6]  : dp14自定义中断2屏蔽位: 
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
        unsigned char  dp14_cfg_error2_int_mask : 1;  /* bit[7]  : dp14自定义中断3屏蔽位:
                                                                   1'b0：中断屏蔽;
                                                                   1'b1：中断不屏蔽; */
    } reg;
} MST_DP14_INTMASK_UNION;
#endif
#define MST_DP14_INTMASK_dp14_test_fail_int_mask_START   (0)
#define MST_DP14_INTMASK_dp14_test_fail_int_mask_END     (0)
#define MST_DP14_INTMASK_dp14_port_ready_int_mask_START  (1)
#define MST_DP14_INTMASK_dp14_port_ready_int_mask_END    (1)
#define MST_DP14_INTMASK_dp14_cfg_error0_int_mask_START  (5)
#define MST_DP14_INTMASK_dp14_cfg_error0_int_mask_END    (5)
#define MST_DP14_INTMASK_dp14_cfg_error1_int_mask_START  (6)
#define MST_DP14_INTMASK_dp14_cfg_error1_int_mask_END    (6)
#define MST_DP14_INTMASK_dp14_cfg_error2_int_mask_START  (7)
#define MST_DP14_INTMASK_dp14_cfg_error2_int_mask_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_PORTCTRL_UNION
 结构说明  : DP14_PORTCTRL 寄存器结构定义。地址偏移量:0xE02，初值:0x00，宽度:8
 寄存器说明: dp14的端口控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_portflowmode   : 2;  /* bit[0-1]: dp14流控模式选择：
                                                              2'b00:normal模式；
                                                              2'b01:push模式；
                                                              2'b10:pull模式； */
        unsigned char  dp14_portdatamode   : 2;  /* bit[2-3]: dp14 端口数据模式配置:
                                                              2'b00：Normal
                                                              2'b01：PRBS
                                                              2'b10：Static_0
                                                              2'b11：Static_1 */
        unsigned char  dp14_nextinvertbank : 1;  /* bit[4]  : dp14 NextInvertBank配置:
                                                              1'b0:取当前framectrl相同配置;
                                                              1'b1:取当前framectrl的另一个相反配置; */
        unsigned char  reserved_0          : 1;  /* bit[5]  : 保留; */
        unsigned char  reserved_1          : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP14_PORTCTRL_UNION;
#endif
#define MST_DP14_PORTCTRL_dp14_portflowmode_START    (0)
#define MST_DP14_PORTCTRL_dp14_portflowmode_END      (1)
#define MST_DP14_PORTCTRL_dp14_portdatamode_START    (2)
#define MST_DP14_PORTCTRL_dp14_portdatamode_END      (3)
#define MST_DP14_PORTCTRL_dp14_nextinvertbank_START  (4)
#define MST_DP14_PORTCTRL_dp14_nextinvertbank_END    (4)


/*****************************************************************************
 结构名    : MST_DP14_BLOCKCTRL1_UNION
 结构说明  : DP14_BLOCKCTRL1 寄存器结构定义。地址偏移量:0xE03，初值:0x1F，宽度:8
 寄存器说明: dp14 Block1控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_wordlength : 6;  /* bit[0-5]: dp14 SampleWordLength配置，支持1~64bit位宽 */
        unsigned char  reserved        : 2;  /* bit[6-7]: 保留; */
    } reg;
} MST_DP14_BLOCKCTRL1_UNION;
#endif
#define MST_DP14_BLOCKCTRL1_dp14_wordlength_START  (0)
#define MST_DP14_BLOCKCTRL1_dp14_wordlength_END    (5)


/*****************************************************************************
 结构名    : MST_DP14_PREPARESTATUS_UNION
 结构说明  : DP14_PREPARESTATUS 寄存器结构定义。地址偏移量:0xE04，初值:0x00，宽度:8
 寄存器说明: dp14的通道准备状态寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_n_finished_channel1 : 1;  /* bit[0]: dp14通道1状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel2 : 1;  /* bit[1]: dp14通道2状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel3 : 1;  /* bit[2]: dp14通道3状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel4 : 1;  /* bit[3]: dp14通道4状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel5 : 1;  /* bit[4]: dp14通道5状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel6 : 1;  /* bit[5]: dp14通道6状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel7 : 1;  /* bit[6]: dp14通道7状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
        unsigned char  dp14_n_finished_channel8 : 1;  /* bit[7]: dp14通道8状态未完成指示:
                                                                 1'b0:完成;
                                                                 1'b1:未完成; */
    } reg;
} MST_DP14_PREPARESTATUS_UNION;
#endif
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel1_START  (0)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel1_END    (0)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel2_START  (1)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel2_END    (1)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel3_START  (2)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel3_END    (2)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel4_START  (3)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel4_END    (3)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel5_START  (4)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel5_END    (4)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel6_START  (5)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel6_END    (5)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel7_START  (6)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel7_END    (6)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel8_START  (7)
#define MST_DP14_PREPARESTATUS_dp14_n_finished_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_PREPARECTRL_UNION
 结构说明  : DP14_PREPARECTRL 寄存器结构定义。地址偏移量:0xE05，初值:0x00，宽度:8
 寄存器说明: dp14的通道准备控制寄存器。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_prepare_channel1 : 1;  /* bit[0]: dp14使能通道1准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel2 : 1;  /* bit[1]: dp14使能通道2准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel3 : 1;  /* bit[2]: dp14使能通道3准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel4 : 1;  /* bit[3]: dp14使能通道4准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel5 : 1;  /* bit[4]: dp14使能通道5准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel6 : 1;  /* bit[5]: dp14使能通道6准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel7 : 1;  /* bit[6]: dp14使能通道7准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
        unsigned char  dp14_prepare_channel8 : 1;  /* bit[7]: dp14使能通道准备:
                                                              1'b0:不使能;
                                                              1'b1:使能； */
    } reg;
} MST_DP14_PREPARECTRL_UNION;
#endif
#define MST_DP14_PREPARECTRL_dp14_prepare_channel1_START  (0)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel1_END    (0)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel2_START  (1)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel2_END    (1)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel3_START  (2)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel3_END    (2)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel4_START  (3)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel4_END    (3)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel5_START  (4)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel5_END    (4)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel6_START  (5)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel6_END    (5)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel7_START  (6)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel7_END    (6)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel8_START  (7)
#define MST_DP14_PREPARECTRL_dp14_prepare_channel8_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_CHANNELEN_BANK0_UNION
 结构说明  : DP14_CHANNELEN_BANK0 寄存器结构定义。地址偏移量:0xE20，初值:0x00，宽度:8
 寄存器说明: dp14的通道使能寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_enable_channel1_bank0 : 1;  /* bit[0]: dp14的Channel1使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel2_bank0 : 1;  /* bit[1]: dp14的Channel2使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel3_bank0 : 1;  /* bit[2]: dp14的Channel3使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel4_bank0 : 1;  /* bit[3]: dp14的Channel4使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel5_bank0 : 1;  /* bit[4]: dp14的Channel5使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel6_bank0 : 1;  /* bit[5]: dp14的Channel6使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel7_bank0 : 1;  /* bit[6]: dp14的Channel7使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel8_bank0 : 1;  /* bit[7]: dp14的Channel8使能:(bank0)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP14_CHANNELEN_BANK0_UNION;
#endif
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel1_bank0_START  (0)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel1_bank0_END    (0)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel2_bank0_START  (1)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel2_bank0_END    (1)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel3_bank0_START  (2)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel3_bank0_END    (2)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel4_bank0_START  (3)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel4_bank0_END    (3)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel5_bank0_START  (4)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel5_bank0_END    (4)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel6_bank0_START  (5)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel6_bank0_END    (5)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel7_bank0_START  (6)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel7_bank0_END    (6)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel8_bank0_START  (7)
#define MST_DP14_CHANNELEN_BANK0_dp14_enable_channel8_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_BLOCKCTRL2_BANK0_UNION
 结构说明  : DP14_BLOCKCTRL2_BANK0 寄存器结构定义。地址偏移量:0xE21，初值:0x00，宽度:8
 寄存器说明: dp14 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_blockgroupcontrol_bank0 : 2;  /* bit[0-1]: dp14 blockgroupcontrol配置(bank0) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP14_BLOCKCTRL2_BANK0_UNION;
#endif
#define MST_DP14_BLOCKCTRL2_BANK0_dp14_blockgroupcontrol_bank0_START  (0)
#define MST_DP14_BLOCKCTRL2_BANK0_dp14_blockgroupcontrol_bank0_END    (1)


/*****************************************************************************
 结构名    : MST_DP14_SAMPLECTRL1_BANK0_UNION
 结构说明  : DP14_SAMPLECTRL1_BANK0 寄存器结构定义。地址偏移量:0xE22，初值:0x00，宽度:8
 寄存器说明: dp14的采样间隔低8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_sampleintervallow_bank0 : 8;  /* bit[0-7]: dp14的采样间隔大小配置低8bit控制寄存器(bank0) */
    } reg;
} MST_DP14_SAMPLECTRL1_BANK0_UNION;
#endif
#define MST_DP14_SAMPLECTRL1_BANK0_dp14_sampleintervallow_bank0_START  (0)
#define MST_DP14_SAMPLECTRL1_BANK0_dp14_sampleintervallow_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_SAMPLECTRL2_BANK0_UNION
 结构说明  : DP14_SAMPLECTRL2_BANK0 寄存器结构定义。地址偏移量:0xE23，初值:0x00，宽度:8
 寄存器说明: dp14的采样间隔高8bit控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_sampleintervalhigh_bank0 : 8;  /* bit[0-7]: dp14的采样间隔大小配置高8bit控制寄存器(bank0) */
    } reg;
} MST_DP14_SAMPLECTRL2_BANK0_UNION;
#endif
#define MST_DP14_SAMPLECTRL2_BANK0_dp14_sampleintervalhigh_bank0_START  (0)
#define MST_DP14_SAMPLECTRL2_BANK0_dp14_sampleintervalhigh_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_OFFSETCTRL1_BANK0_UNION
 结构说明  : DP14_OFFSETCTRL1_BANK0 寄存器结构定义。地址偏移量:0xE24，初值:0x00，宽度:8
 寄存器说明: dp14的Offset控制寄存器低8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_offset1_bank0 : 8;  /* bit[0-7]: dp14的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP14_OFFSETCTRL1_BANK0_UNION;
#endif
#define MST_DP14_OFFSETCTRL1_BANK0_dp14_offset1_bank0_START  (0)
#define MST_DP14_OFFSETCTRL1_BANK0_dp14_offset1_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_OFFSETCTRL2_BANK0_UNION
 结构说明  : DP14_OFFSETCTRL2_BANK0 寄存器结构定义。地址偏移量:0xE25，初值:0x00，宽度:8
 寄存器说明: dp14的Offset控制寄存器高8位(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_offset2_bank0 : 8;  /* bit[0-7]: dp14的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank0) */
    } reg;
} MST_DP14_OFFSETCTRL2_BANK0_UNION;
#endif
#define MST_DP14_OFFSETCTRL2_BANK0_dp14_offset2_bank0_START  (0)
#define MST_DP14_OFFSETCTRL2_BANK0_dp14_offset2_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_HCTRL_BANK0_UNION
 结构说明  : DP14_HCTRL_BANK0 寄存器结构定义。地址偏移量:0xE26，初值:0x11，宽度:8
 寄存器说明: dp14的传输子帧参数控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_hstop_bank0  : 4;  /* bit[0-3]: dp14传输子帧的右边界Hstop的配置1~16(bank0) */
        unsigned char  dp14_hstart_bank0 : 4;  /* bit[4-7]: dp14传输子帧的左边界Hstart的配置1~16(bank0) */
    } reg;
} MST_DP14_HCTRL_BANK0_UNION;
#endif
#define MST_DP14_HCTRL_BANK0_dp14_hstop_bank0_START   (0)
#define MST_DP14_HCTRL_BANK0_dp14_hstop_bank0_END     (3)
#define MST_DP14_HCTRL_BANK0_dp14_hstart_bank0_START  (4)
#define MST_DP14_HCTRL_BANK0_dp14_hstart_bank0_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_BLOCKCTRL3_BANK0_UNION
 结构说明  : DP14_BLOCKCTRL3_BANK0 寄存器结构定义。地址偏移量:0xE27，初值:0x00，宽度:8
 寄存器说明: dp14 Block2控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_blockpackingmode_bank0 : 1;  /* bit[0]  : dp14 blockpacking模式配置:(bank0)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP14_BLOCKCTRL3_BANK0_UNION;
#endif
#define MST_DP14_BLOCKCTRL3_BANK0_dp14_blockpackingmode_bank0_START  (0)
#define MST_DP14_BLOCKCTRL3_BANK0_dp14_blockpackingmode_bank0_END    (0)


/*****************************************************************************
 结构名    : MST_DP14_LANECTRL_BANK0_UNION
 结构说明  : DP14_LANECTRL_BANK0 寄存器结构定义。地址偏移量:0xE28，初值:0x00，宽度:8
 寄存器说明: dp14的数据线控制寄存器(bank0)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_lanecontrol_bank0 : 3;  /* bit[0-2]: dp14的数据线控制，表示dp14在哪根数据线上传输:(bank0)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP14_LANECTRL_BANK0_UNION;
#endif
#define MST_DP14_LANECTRL_BANK0_dp14_lanecontrol_bank0_START  (0)
#define MST_DP14_LANECTRL_BANK0_dp14_lanecontrol_bank0_END    (2)


/*****************************************************************************
 结构名    : MST_DP14_CHANNELEN_BANK1_UNION
 结构说明  : DP14_CHANNELEN_BANK1 寄存器结构定义。地址偏移量:0xE30，初值:0x00，宽度:8
 寄存器说明: dp14的通道使能寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_enable_channel1_bank1 : 1;  /* bit[0]: dp14的Channel1使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel2_bank1 : 1;  /* bit[1]: dp14的Channel2使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel3_bank1 : 1;  /* bit[2]: dp14的Channel3使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel4_bank1 : 1;  /* bit[3]: dp14的Channel4使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel5_bank1 : 1;  /* bit[4]: dp14的Channel5使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel6_bank1 : 1;  /* bit[5]: dp14的Channel6使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel7_bank1 : 1;  /* bit[6]: dp14的Channel7使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
        unsigned char  dp14_enable_channel8_bank1 : 1;  /* bit[7]: dp14的Channel8使能:(bank1)
                                                                   1'b0:不使能;
                                                                   1'b1:使能; */
    } reg;
} MST_DP14_CHANNELEN_BANK1_UNION;
#endif
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel1_bank1_START  (0)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel1_bank1_END    (0)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel2_bank1_START  (1)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel2_bank1_END    (1)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel3_bank1_START  (2)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel3_bank1_END    (2)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel4_bank1_START  (3)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel4_bank1_END    (3)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel5_bank1_START  (4)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel5_bank1_END    (4)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel6_bank1_START  (5)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel6_bank1_END    (5)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel7_bank1_START  (6)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel7_bank1_END    (6)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel8_bank1_START  (7)
#define MST_DP14_CHANNELEN_BANK1_dp14_enable_channel8_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_BLOCKCTRL2_BANK1_UNION
 结构说明  : DP14_BLOCKCTRL2_BANK1 寄存器结构定义。地址偏移量:0xE31，初值:0x00，宽度:8
 寄存器说明: dp14 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_blockgroupcontrol_bank1 : 2;  /* bit[0-1]: dp14 blockgroupcontrol配置(bank1) */
        unsigned char  reserved                     : 6;  /* bit[2-7]: 保留; */
    } reg;
} MST_DP14_BLOCKCTRL2_BANK1_UNION;
#endif
#define MST_DP14_BLOCKCTRL2_BANK1_dp14_blockgroupcontrol_bank1_START  (0)
#define MST_DP14_BLOCKCTRL2_BANK1_dp14_blockgroupcontrol_bank1_END    (1)


/*****************************************************************************
 结构名    : MST_DP14_SAMPLECTRL1_BANK1_UNION
 结构说明  : DP14_SAMPLECTRL1_BANK1 寄存器结构定义。地址偏移量:0xE32，初值:0x00，宽度:8
 寄存器说明: dp14的采样间隔低8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_sampleintervallow_bank1 : 8;  /* bit[0-7]: dp14的采样间隔大小配置低8bit控制寄存器(bank1) */
    } reg;
} MST_DP14_SAMPLECTRL1_BANK1_UNION;
#endif
#define MST_DP14_SAMPLECTRL1_BANK1_dp14_sampleintervallow_bank1_START  (0)
#define MST_DP14_SAMPLECTRL1_BANK1_dp14_sampleintervallow_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_SAMPLECTRL2_BANK1_UNION
 结构说明  : DP14_SAMPLECTRL2_BANK1 寄存器结构定义。地址偏移量:0xE33，初值:0x00，宽度:8
 寄存器说明: dp14的采样间隔高8bit控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_sampleintervalhigh_bank1 : 8;  /* bit[0-7]: dp14的采样间隔大小配置高8bit控制寄存器(bank1) */
    } reg;
} MST_DP14_SAMPLECTRL2_BANK1_UNION;
#endif
#define MST_DP14_SAMPLECTRL2_BANK1_dp14_sampleintervalhigh_bank1_START  (0)
#define MST_DP14_SAMPLECTRL2_BANK1_dp14_sampleintervalhigh_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_OFFSETCTRL1_BANK1_UNION
 结构说明  : DP14_OFFSETCTRL1_BANK1 寄存器结构定义。地址偏移量:0xE34，初值:0x00，宽度:8
 寄存器说明: dp14的Offset控制寄存器低8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_offset1_bank1 : 8;  /* bit[0-7]: dp14的Offset的低8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP14_OFFSETCTRL1_BANK1_UNION;
#endif
#define MST_DP14_OFFSETCTRL1_BANK1_dp14_offset1_bank1_START  (0)
#define MST_DP14_OFFSETCTRL1_BANK1_dp14_offset1_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_OFFSETCTRL2_BANK1_UNION
 结构说明  : DP14_OFFSETCTRL2_BANK1 寄存器结构定义。地址偏移量:0xE35，初值:0x00，宽度:8
 寄存器说明: dp14的Offset控制寄存器高8位(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_offset2_bank1 : 8;  /* bit[0-7]: dp14的Offset的高8位，用于计算Block Offset和Sub-Block Offset(bank1) */
    } reg;
} MST_DP14_OFFSETCTRL2_BANK1_UNION;
#endif
#define MST_DP14_OFFSETCTRL2_BANK1_dp14_offset2_bank1_START  (0)
#define MST_DP14_OFFSETCTRL2_BANK1_dp14_offset2_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_HCTRL_BANK1_UNION
 结构说明  : DP14_HCTRL_BANK1 寄存器结构定义。地址偏移量:0xE36，初值:0x11，宽度:8
 寄存器说明: dp14的传输子帧参数控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_hstop_bank1  : 4;  /* bit[0-3]: dp14传输子帧的右边界Hstop的配置1~16(bank1) */
        unsigned char  dp14_hstart_bank1 : 4;  /* bit[4-7]: dp14传输子帧的左边界Hstart的配置1~16(bank1) */
    } reg;
} MST_DP14_HCTRL_BANK1_UNION;
#endif
#define MST_DP14_HCTRL_BANK1_dp14_hstop_bank1_START   (0)
#define MST_DP14_HCTRL_BANK1_dp14_hstop_bank1_END     (3)
#define MST_DP14_HCTRL_BANK1_dp14_hstart_bank1_START  (4)
#define MST_DP14_HCTRL_BANK1_dp14_hstart_bank1_END    (7)


/*****************************************************************************
 结构名    : MST_DP14_BLOCKCTRL3_BANK1_UNION
 结构说明  : DP14_BLOCKCTRL3_BANK1 寄存器结构定义。地址偏移量:0xE37，初值:0x00，宽度:8
 寄存器说明: dp14 Block2控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_blockpackingmode_bank1 : 1;  /* bit[0]  : dp14 blockpacking模式配置:(bank1)
                                                                      1'b0:不使能；
                                                                      1'b1使能； */
        unsigned char  reserved                    : 7;  /* bit[1-7]: 保留; */
    } reg;
} MST_DP14_BLOCKCTRL3_BANK1_UNION;
#endif
#define MST_DP14_BLOCKCTRL3_BANK1_dp14_blockpackingmode_bank1_START  (0)
#define MST_DP14_BLOCKCTRL3_BANK1_dp14_blockpackingmode_bank1_END    (0)


/*****************************************************************************
 结构名    : MST_DP14_LANECTRL_BANK1_UNION
 结构说明  : DP14_LANECTRL_BANK1 寄存器结构定义。地址偏移量:0xE38，初值:0x00，宽度:8
 寄存器说明: dp14的数据线控制寄存器(bank1)。
*****************************************************************************/
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char      value;
    struct
    {
        unsigned char  dp14_lanecontrol_bank1 : 3;  /* bit[0-2]: dp14的数据线控制，表示dp14在哪根数据线上传输:(bank1)
                                                                 3'b000:data0;
                                                                 3'b001:data1;（其他禁配） */
        unsigned char  reserved               : 5;  /* bit[3-7]: 保留; */
    } reg;
} MST_DP14_LANECTRL_BANK1_UNION;
#endif
#define MST_DP14_LANECTRL_BANK1_dp14_lanecontrol_bank1_START  (0)
#define MST_DP14_LANECTRL_BANK1_dp14_lanecontrol_bank1_END    (2)






/*****************************************************************************
  8 OTHERS定义
*****************************************************************************/



/*****************************************************************************
  9 全局变量声明
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of MST_interface.h */
