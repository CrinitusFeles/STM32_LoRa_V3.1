cmake_minimum_required(VERSION 3.22)

message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

project(STM32_LoRa_v3.1)
enable_language(C CXX ASM)

set(CPU_PARAMETERS
    -mthumb
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)


if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-O0 -g)
endif ()

# Add you source file
file(GLOB_RECURSE SOURCES
        "Core/Src/*.c"
        "CMSIS_L431/src/*.c"
        "Cortex/src/*.c"
        "ExternalDevices/src/*.c"
        "Main/src/*.c"
        "Middleware/src/*.c"
        "Periph/src/*.c"
        "CMSIS_L431/Startup/startup_stm32l431rbtx.s"
        "console/src/*.c"
        "FreeRTOS/src/*.c"
        "XModem/*.c"
        "BootLib/*.c"
        "FAT_FS/*.c"
        # "lua-5.4.7/src/*.c"
        # "lua-5.4.7/*.c"
        )

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug_800")
    message(STATUS "Linking RESERVE firmware 0x8000000")
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/CMSIS_L431/Startup/Linker_800.ld)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug_801")
    message(STATUS "Linking MAIN firmware 0x8010000")
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/CMSIS_L431/Startup/Linker_801.ld)
else ()
    message(STATUS "INCORRECT CMAKE_BUILD_TYPE")
endif ()
set(EXECUTABLE ${PROJECT_NAME}.out)

add_executable(${EXECUTABLE} ${SOURCES})
# Add Include directories
target_include_directories(${EXECUTABLE} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ExternalDevices/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Main/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS_L431/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Periph/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Cortex/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/console/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/XModem
        ${CMAKE_CURRENT_SOURCE_DIR}/BootLib
        ${CMAKE_CURRENT_SOURCE_DIR}/FAT_FS
        # ${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.7/src
)

target_compile_definitions(${EXECUTABLE} PRIVATE -DSTM32L431xx)

target_link_options(${EXECUTABLE} PRIVATE
    -T${LINKER_SCRIPT}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -ffunction-sections
    -Wl,--no-warn-rwx-segments
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    # -Wl,-u,vfprintf;
    # -Wl,-u,_printf_float;
    # -Wl,-u,_scanf_float;
)

target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    # -Wno-unused-parameter
    # Full debug configuration
    -fstack-usage
    -Wformat=0
    -O0 -g -ggdb
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${PROJECT_NAME}.bin
        )