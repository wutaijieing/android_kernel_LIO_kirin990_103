################################################################################################
# purpose:
#    This file provide two vars: VFMW_FLAG, VFMW_INC, VFMW_OBJ
#    inc-flags     --- compile options for vfmw
#    CFILES      --- specify the files to be compiled
###############################################################################################

set(VFMW_INC_DIR ${VFMW_6_2_DIR})
set(CFILES_DIR ${VFMW_6_2_DIR})

include(${VFMW_6_2_DIR}/product/${PRODUCT_DIR}/sec_cfg/vfmw_config.cmake)

#===============================================================================
#   options
#===============================================================================
list(APPEND TEE_C_DEFINITIONS
    ENV_SOS_KERNEL
) #ENV_ARMLINUX_KERNEL

list(APPEND TEE_C_DEFINITIONS
    # SCD_BUSY_WAITTING
    IP_CANCEL_SUPPORT
    VFMW_SMMU_SUPPORT
    # VFMW_MD5_SUPPORT

    VDH_BSP_NUM_IN_USE=${VDH_BSP_NUM_IN_USE}
    STM_DEV_NUM=${STM_DEV_NUM}
    MAX_OPEN_COUNT=${MAX_OPEN_COUNT}
    SCD_UMSG_STEP=${SCD_UMSG_STEP}
    SCD_UMSG_NUM=${SCD_UMSG_NUM}
    SCD_DMSG_NUM=${SCD_DMSG_NUM}
)
set(LINUX_VER tee)

if ("${VFMW_SCD_SUPPORT}" STREQUAL "y")
   set(SCD_VER v3r3)
endif()

if ("${VFMW_VDH_SUPPORT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        VDH_DEC_SUPPORT
    )
    set(VDH_VER v5r7b5)
endif()

if ("${VFMW_DPRINT_SUPPORT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        VFMW_DPRINT_SUPPORT
    )
endif()

if ("${VFMW_MMU_SUPPORT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        VFMW_MMU_SUPPORT
    )
endif()

if ("${VFMW_SYSTEM_TIME_OUT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        SCD_TIME_OUT=${VFMW_SCD_TIME_OUT}
        SCD_FPGA_TIME_OUT=${VFMW_SCD_FPGA_TIME_OUT}
        VDH_TIME_OUT=${VFMW_VDH_TIME_OUT}
        VDH_FPGA_TIME_OUT=${VFMW_VDH_FPGA_TIME_OUT}
        VDH_ONE_FRM_PERF=${VFMW_VDH_ONE_FRM_PERF}
        VDH_FPGA_ONE_FRM_PERF=${VFMW_VDH_FPGA_ONE_FRM_PERF}
    )
endif()

if ("${VFMW_CHAN_SUPPORT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        CFG_MAX_CHAN_NUM=${VFMW_MAX_CHAN_NUM}
    )
endif()

#===============================================================================
#   include path
#===============================================================================
list(APPEND PLATDRV_INCLUDE_PATH
    ${VFMW_INC_DIR}/../../include
    ${VFMW_INC_DIR}
    ${VFMW_INC_DIR}/include
    ${VFMW_INC_DIR}/core
    ${VFMW_INC_DIR}/core/stream
    ${VFMW_INC_DIR}/core/stream/hal/${SCD_VER}
    ${VFMW_INC_DIR}/core/decode
    ${VFMW_INC_DIR}/core/decode/hal/${VDH_VER}
    ${VFMW_INC_DIR}/intf
    ${VFMW_INC_DIR}/intf/sec_smmu
    ${VFMW_INC_DIR}/osal
    ${VFMW_INC_DIR}/osal/${LINUX_VER}
    ${VFMW_INC_DIR}/product
    ${VFMW_INC_DIR}/product/${PRODUCT_DIR}
)

set(CORE_DIR ${CFILES_DIR}/core)
set(STM_DIR ${CORE_DIR}/stream)
set(DEC_DIR ${CORE_DIR}/decode)
set(SCD_DIR ${CORE_DIR}/stream/hal/${SCD_VER})
set(VDH_DIR ${CORE_DIR}/decode/hal/${VDH_VER})
set(INTF_DIR ${CFILES_DIR}/intf)
set(OSL_DIR ${CFILES_DIR}/osal)
set(PDT_DIR ${CFILES_DIR}/product/${PRODUCT_DIR})

#===============================================================================
#   vfmw_obj_list
#===============================================================================

#core/stream
list(APPEND TEE_C_SOURCES
    ${STM_DIR}/stm_dev.c
    ${SCD_DIR}/scd_hal.c
)

#core/decode
list(APPEND TEE_C_SOURCES
    ${DEC_DIR}/dec_dev.c
    ${VDH_DIR}/dec_hal.c
)

#proc
if ("${VFMW_PROC_SUPPORT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        VFMW_PROC_SUPPORT
    )
endif()

#intf
list(APPEND TEE_C_SOURCES
    ${INTF_DIR}/sec_intf.c
)

#smmu
list(APPEND TEE_C_SOURCES
    ${INTF_DIR}/sec_smmu/smmu.c
)

if ("${VFMW_STREAM_SUPPORT}" STREQUAL "y")
    list(APPEND TEE_C_DEFINITIONS
        VFMW_STREAM_SUPPORT
    )
endif()

#osal
list(APPEND TEE_C_SOURCES
    ${OSL_DIR}/${LINUX_VER}/vfmw_osal.c
    ${OSL_DIR}/${LINUX_VER}/tee_osal.c
)

#product
list(APPEND TEE_C_SOURCES
    ${PDT_DIR}/product.c
)

