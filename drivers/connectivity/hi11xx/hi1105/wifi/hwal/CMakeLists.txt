# cmake version
cmake_minimum_required (VERSION ${cmake_version})

project (${pro_name})

set (CMAKE_C_COMPILER ${gcc_compile})

# src
file (GLOB HWAL ${CMAKE_SOURCE_DIR}/*.c)

set(compile_macro_config ${pro_name}.cmake)
message(STATUS "compile_macro_config:" ${compile_macro_config})
include(${compile_macro_config})

set(compile_include_config ${pro_name}_include.cmake)
message(STATUS "compile_include_config:" ${compile_include_config})
include(${compile_include_config})

add_library (${pro_name} STATIC ${HWAL})
