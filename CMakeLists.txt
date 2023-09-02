cmake_minimum_required(VERSION 3.17)
set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
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
        )
set(include_path_DIRS ${CMAKE_SOURCE_DIR}Main/inc)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/CMSIS_L431/Startup/STM32L431RBTX_FLASH.ld)

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
)

target_compile_definitions(${EXECUTABLE} PRIVATE -DSTM32L431xx)

target_link_options(${EXECUTABLE} PRIVATE
    -T${LINKER_SCRIPT}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -ffunction-sections
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,--allow-multiple-definition
)

target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    # Full debug configuration
    -fstack-usage
    -Wno-unused-variable -Wno-tautological-compare -Wformat=0

    -O0 -g -ggdb
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${PROJECT_NAME}.bin
        )