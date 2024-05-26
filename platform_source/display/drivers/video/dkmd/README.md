# DKMD系统软件设计说明书

## 修订记录

| 日期       | 修订版本 | 修改章节             | 修改描述 | 作者     |
| ---------- | -------- | -------------------- | -------- | -------- |
| 2021-02-18 | V0.1     | 梳理DKMD整体设计逻辑 | 初稿     | 00274020 |
| 2021-03-08 | V0.2     | 增加时序和流程逻辑   | 新增     | 00274020 |

- [DKMD系统软件设计说明书](#dkmd系统软件设计说明书)
  - [修订记录](#修订记录)
  - [1 显示内核驱动简介](#1-显示内核驱动简介)
  - [2 DKMD整体设计描述](#2-dkmd整体设计描述)
    - [2.1 DKMD软件系统模块说明](#21-dkmd软件系统模块说明)
    - [2.2 DKMD软件系统时序说明](#22-dkmd软件系统时序说明)

## 1 显示内核驱动简介

显示子系统重构设计的整体思路是为了不让芯片核心架构由于驱动代码开源而暴露，需要将驱动中更多的资源、策略以及链路链接关系等计算上移到HAL层，HAL层以库文件交付；
所以架构整体做了如下调整：

```plantuml
@startuml
skinparam rectangle {
    roundCorner 25
}
rectangle " Old to New" <<$archimate/business-process>> {
    rectangle "旧架构" as old_arch {
        package "内核" as old_kernel {
            component "cmdlist配置" as old_cmdlist
            component "online_play" as old_online
            component "offline_play" as old_offline

            old_online ..> old_cmdlist
            old_offline ..> old_cmdlist
        }
        note left of old_online
            创建配置流程、
            计算分块配置、
            逐个模块解析、
            配置硬件链路、
            ...
        end note

        package "HAL" as old_hwcomposer {
            component "online_policy_config" as old_online_policy
            component "offline_policy_config" as old_offline_policy
            component "dss_overlay_t" as old_data

            old_online_policy .up.> old_data
            old_offline_policy .up.> old_data
        }
        note left of old_online_policy
            策略预处理、
            策略识别判断、
            资源合理分配、
            数据参数配置
            ...
        end note
        old_online_policy --> old_online : "dss_overlay_t"
        old_offline_policy --> old_offline : "dss_overlay_t"
    }

    rectangle "新架构" as new_arch {
        package "内核" as new_kernel {
            component "present" as kcomposer
            note right of kcomposer
                直接下发cmdlist任务
                ...
            end note
        }
        package "RSIC-V" as rsicv {
            component "MCU" as mcu
            note right of mcu
                根据寄存器解析配置
                ...
            end note
        }
        package "HAL" as new_hwcomposer #lightblue {
            component "libhwc" as hwc
            component "libdumd" as dumd
            component "libcmdlist" as cmdlist
            hwc .left.> dumd
            dumd .left.> cmdlist
        }
        note right of new_hwcomposer
            策略预处理、
            策略识别判断、
            资源合理分配、
            创建配置流程、
            计算分块配置、
            逐个模块解析、
            配置硬件链路、
            ...
        end note
        hwc --> kcomposer : ioctl下发cmdlist_id和layer信息
        kcomposer --> mcu : cmdlist + frm_start
    }
    old_arch -down-> new_arch
}

caption 图1.显示内核架构变化视图
@enduml
```

如上图可以看出几点核心变化：
**新架构的所有资源使用、策略匹配以及场景配置计算都是在HAL完成计算实现，内核驱动只做任务最后的提交执行。**

## 2 DKMD整体设计描述

驱动设计使用模块化编程思想，将各个模块依赖关系和接口梳理清楚后组合完成内核显示子系统搭建；
另外分为基础模块和扩展模块：
基础模块完成提供显示基本功能，属于A包范畴；
扩展模块是提供额外特性功能的模块，如局部刷新、低功耗调频、在线回写等，属于B包范畴。

虽然显示配置等复杂计算任务，在内核中不需要关注了，但仍然需要有相关设备驱动作为数据计算的基础，这些重要的模块有：

- Cmdlist模块驱动
- Panel模块驱动
- MIPI_DSI模块驱动
- Composer模块驱动
- FBDevice模块驱动
- DP模块驱动等

### 2.1 DKMD软件系统模块说明

除了基础内核驱动模块外，显示内核还有一些通用接口模块，他们与基础模块之间如下图所示：

```plantuml
@startuml
    (dpu_panel) as panel
    (dpu_offline) as offline
    (dpu_dp) as dp
    dp . panel
    panel . offline
    rectangle "cmdlist" as cmdlist {
        component "dkmd_cmdlist" as dkmd_cmdlist {
            [cmdlist_dev] ..> [cmdlist_interface] : 依赖
        }
    }
    rectangle "dpu-begonia" as dpu {
        component "dpu_res_mgr" as res_mgr #lightgray {
            (mem_mgr/gr_stub/dvfs/..)
            [opr_mgr/..]
            (config)
        }
        [dpu_composer]
        component "gfxdev_mgr" as gfxdev {
            (fb) .> (gfx)
            (gfx) <. (drm)
        }
        [dpu_composer] .right.> (config) : 依赖
        [dpu_composer] -up-> (gfx) #green;text:green : 引用
    }
    rectangle "dksm" as dksm {
        component "dkmd_dksm" as dkmd_dksm #lightgreen {
            [buf_sync/fence/timeline/..] . [isr]
            [isr] . [chrdev/peri/..]
        }
    }
    [cmdlist_interface] .down.> [buf_sync/fence/timeline/..] : 依赖
    dkmd_connector .left.> [chrdev/peri/..] : 依赖

    rectangle "connector" as connector {
        component "dkmd_connector" as dkmd_connector {
        }
        [dp_ctrl] . [dsc]
        [dsc] . [mipi]
        [mipi] -|> dkmd_connector
    }
    panel .up.> [mipi] : 依赖
    dp .up.> [dp_ctrl] : 依赖

    offline .up.> dkmd_connector : 依赖
    ' [mipi] ..up.> [dpu_composer] : 依赖
    ' [dp_ctrl] ..up.> [dpu_composer] : 依赖
    [dpu_composer] <.down. dkmd_connector : 注册
    ' [dpu_composer] .down.> dkmd_dksm : 依赖

    [opr_mgr/..] -up--|> (/dev/dpu_res) : implement
    (fb) -up-|> (/dev/graphics/fb0) : implement
    (drm) -up-|> (/dev/dri/card0) : implement
    (gfx) -up-|> (/dev/gfx_offline) : implement
    (gfx) -up-|> (/dev/gfx_dp) : implement
    [cmdlist_dev] -up-|> (/dev/dpu_cmdlist) : implement

    ' [opr_mgr/..] ..down.> [chrdev/peri/..] : 依赖
    caption 图2. 显示内核模块图
@enduml
```

每一个模块都有自己模块宏控制，在满足依赖的情况下，可以独立编译运行，他们之间的依赖关系如下表所示：

| 显示驱动目录    |           |          | 控制宏                     | 功能描述                            | 接口依赖关系                                       |
| --------------- | --------- | -------- | -------------------------- | ----------------------------------- | -------------------------------------------------- |
| kernel/.../dkmd |           |          | CONFIG_DKMD_ENABLE         | 控制编译显示整个驱动模块            | NA                                                 |
|                 | cmdlist   |          | CONFIG_DKMD_CMDLIST_ENABLE | 使能cmdlist驱动模块                 | CONFIG_DKMD_DKSM                                   |
|                 | connector |          | CONFIG_DKMD_DPU_CONNECTOR  | 使能connector模块                   | NA                                                 |
|                 |           | dp       | CONFIG_DKMD_DPU_DP         | dp模块使能                          | NA(TODO)                                           |
|                 |           | offline  | CONFIG_DKMD_DPU_OFFLINE    | offline模块使能                     | CONFIG_DKMD_DPU_COMPOSER                           |
|                 |           | panel    | CONFIG_DKMD_DPU_PANEL      | 屏模块使能                          | CONFIG_DKMD_DPU_CONNECTOR                           |
|                 | dksm      |          | CONFIG_DKMD_DKSM           | fence、timeline、buf_sync等接口函数 | NA                                                 |
|                 | dpu       |          | CONFIG_DKMD_DPU_ENABLE     | 使能DPU叠加驱动核心模块             | NA                                                 |
|                 |           | composer | CONFIG_DKMD_DPU_COMPOSER   | 叠加设备驱动模块                    | CONFIG_DKMD_CMDLIST_ENABLE<br/>CONFIG_DKMD_DPU_RES_MGR |
|                 |           | device   | CONFIG_DKMD_DPU_DEVICE     | 对外显示设备管理模块                | CONFIG_DKMD_DPU_COMPOSER                           |
|                 |           | res_mgr  | CONFIG_DKMD_DPU_RES_MGR    | 叠加资源管理模块                    | CONFIG_DKMD_DKSM                                   |
表1. 显示内核模块依赖表


### 2.2 DKMD软件系统时序说明

硬件设备信息通过DTS定义声明，加载解析设备信息流程依赖DTS的加载顺序；特别地，最后枚举出对外设备接口时，需要由输出设备来完成加载调用。
这里的输出设备通常有：屏（panel）送显、DP送显、离线写DDR；所以DTS设备信息需要在对外输出设备加载对齐接口设备前完成信息获取和解析，
然后由输出设备驱动完成链路调用与注册，最终完成对外设备的枚举。在设备被正常流程调用使用时，调用的接口作用刚好相反，
因此这个加载注册的调用流程，称之为：“反向注册”。如下为主屏（/dev/graphics/fb0）反向注册流程：

```plantuml
@startuml
    skinparam roundcorner 20
    skinparam handwritten true
    skinparam sequence {
        ParticipantFontSize 17
    }

    ' Title 驱动加载调用流程时序
    participant "DTS设备加载" as dpu_dtsi #lightgray
    box "connector" #LightBlue
        participant "panel" as panel #lightgray
        participant "mipi_dsi" as dsi #lightgray
        ' participant "dp" as dp
    end box

    box "dpu-begonia" #LightBlue
        participant "composer" as composer #lightgray
        participant "device" as device #lightgray
        participant "res_mgr" as res_mgr #lightgray
    end box
    participant "dkmd_cmdlist" as cmdlist #lightgray

    note over dpu_dtsi
        1、DTS声明多个硬件设备，驱动复用，即驱动与设备是一对多的关系;
        2、DTS差异化定义了硬件资源：时钟、电源、IO及内存空间等；
        3、DTS设备依赖关系（待定义）
        ............
    end note

    group 内核驱动注册
        device -> device : module_init
        ?->o device : DTS设备加载
        device -> device : probe
        ...
        dsi -> dsi : module_init
        ?->o dsi : DTS设备加载
        dsi -> dsi : probe
        ...
        panel -> panel : late_initcall
        dpu_dtsi -[#red]>o panel : DTS设备加载
        panel -> panel : probe

        res_mgr -> res_mgr : module_init
        ?->o res_mgr : DTS设备加载
        res_mgr -> res_mgr : probe

        cmdlist -> cmdlist : module_init
        ?->o cmdlist : DTS设备加载
        cmdlist -> cmdlist : probe
        activate res_mgr
        activate cmdlist
        activate panel
    end

    group 硬件设备加载对外接口
        res_mgr -[#green]> "/dev/dpu_res" ** : create
        hnote over "/dev/dpu_res" : idle
        deactivate res_mgr
        cmdlist -[#green]> "/dev/cmdlist" ** : create
        hnote over "/dev/cmdlist" : idle
        deactivate cmdlist

        autonumber
        panel -[#Red]> connector : register_connector
        activate connector
        connector -[#Red]> composer : register_composer
        activate composer
        composer -[#Red]> device : device_mgr_create_gfxdev
        activate device
        device -[#Red]> device : register_framebuffer\nregister_chrdev
        device -[#green]> "/dev/graphics/fb*\n/dev/gfx_offline**" ** : create
        hnote over "/dev/graphics/fb*\n/dev/gfx_offline**" : idle
        return
        return
        return
        deactivate panel
    end
    ...

    caption 图3. 显示设备驱动加载流程时序图
@enduml
```
