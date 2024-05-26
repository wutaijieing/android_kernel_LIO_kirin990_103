set (DRIVER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../)
set (LITEOSTOPDIR ${compents_dir}/liteos/)
set (LWIPTOPDIR ${compents_dir}/lwip-2.0.3/open_source/lwip/lwip-2.0/src/)
include_directories (
	${CMAKE_SOURCE_DIR}/
	${LITEOSTOPDIR}/compat/linux/include/
	${LITEOSTOPDIR}/lib/libc/include/
	${LITEOSTOPDIR}/lib/libc/kernel/uapi/
	${LITEOSTOPDIR}/lib/libc/kernel/uapi/asm-arm64/
	${LITEOSTOPDIR}/kernel/include/
	${LITEOSTOPDIR}/kernel/base/include/
	${LITEOSTOPDIR}/platform/bsp/hi3518ev200/config/
	${LITEOSTOPDIR}/platform/bsp/hi3518ev200/include/
	${LITEOSTOPDIR}/platform/bsp/hi3518ev200/
	${LITEOSTOPDIR}/platform/cpu/arm/include/
	${LITEOSTOPDIR}/fs/include/
	${DRIVER_SOURCE_DIR}/platform/inc/oal/liteos/
	${DRIVER_SOURCE_DIR}/platform/inc/
	${DRIVER_SOURCE_DIR}/platform/inc/oal/
	${DRIVER_SOURCE_DIR}/platform/inc/oam/
	${DRIVER_SOURCE_DIR}/platform/inc/frw/
	${DRIVER_SOURCE_DIR}/platform/oal/
	${DRIVER_SOURCE_DIR}/platform/oam/
	${DRIVER_SOURCE_DIR}/common/inc/
	${DRIVER_SOURCE_DIR}/common/chr_log/
	${DRIVER_SOURCE_DIR}/wifi/customize/
    ${DRIVER_SOURCE_DIR}/wifi/inc/
	${DRIVER_SOURCE_DIR}/wifi/wal/
	${DRIVER_SOURCE_DIR}/wifi/inc/hd_common/
	${DRIVER_SOURCE_DIR}/wifi/hmac/
	${LWIPTOPDIR}/include/
	#${COMPONENTS_DIR}/lwip-2.1.2/patch/huawei_patch/include/
	#${COMPONENTS_DIR}/lwip-2.1.2/patch/huawei_patch/include/config/
)
