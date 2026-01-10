cmake_minimum_required(VERSION 3.12)

# 1. 必须在 project() 之前定义目标系统
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 2. 定义工具链路径 (使用你刚才确认的 Mac 路径)
set(TOOLCHAIN_PATH "/Applications/ArmGNUToolchain/15.2.rel1/aarch64-none-elf")

# 3. 显式指定编译器绝对路径（必须在 project 之前）
set(CMAKE_C_COMPILER   "${TOOLCHAIN_PATH}/bin/aarch64-none-elf-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/aarch64-none-elf-g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/bin/aarch64-none-elf-gcc")

# 4. 告知 CMake 跳过编译器检测（交叉编译裸机程序时非常有用，防止 Clang 干扰）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 定义编译和链接等选项
set(CC_OPTION "-Os   -Wformat-signedness    -Wl,--build-id=none   -fno-PIE -fno-PIE --specs=nosys.specs -fno-builtin -fno-dwarf2-cfi-asm -fomit-frame-pointer -fzero-initialized-in-bss -fdollars-in-identifiers -ffunction-sections -fdata-sections -fno-aggressive-loop-optimizations -fno-optimize-strlen -fno-schedule-insns -fno-inline-small-functions -fno-inline-functions-called-once -fno-strict-aliasing -finline-limit=20  -mlittle-endian -nostartfiles -funwind-tables")
set(AS_OPTION "-Os   -Wformat-signedness    -Wl,--build-id=none   -fno-PIE -fno-PIE --specs=nosys.specs -fno-builtin -fno-dwarf2-cfi-asm -fomit-frame-pointer -fzero-initialized-in-bss -fdollars-in-identifiers -ffunction-sections -fdata-sections -fno-aggressive-loop-optimizations -fno-optimize-strlen -fno-schedule-insns -fno-inline-small-functions -fno-inline-functions-called-once -fno-strict-aliasing -finline-limit=20  -mlittle-endian -nostartfiles -funwind-tables")
set(LD_OPTION " ")
set(CMAKE_C_FLAGS "${CC_OPTION} ")
set(CMAKE_ASM_FLAGS "${AS_OPTION} ")
set(CMAKE_LINK_FLAGS "${LD_OPTION} -T ${CMAKE_CURRENT_SOURCE_DIR}/aarch64-qemu.ld") # 指定链接脚本
set(CMAKE_EXE_LINKER_FLAGS "${LD_OPTION} -T ${CMAKE_CURRENT_SOURCE_DIR}/aarch64-qemu.ld") # 指定链接脚本
set (CMAKE_C_LINK_FLAGS " ")
set (CMAKE_CXX_LINK_FLAGS " ")

set(HOME_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(APP "miniEuler") # APP变量，后面会用到 ${APP}
project(${APP} LANGUAGES C ASM) # 工程名及所用语言
set(CMAKE_BUILD_TYPE Debug) # 生成 Debug 版本

include_directories( # include 目录
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
)

add_subdirectory(bsp) # 包含子文件夹的内容
add_subdirectory(kernel)

list(APPEND OBJS $<TARGET_OBJECTS:bsp> $<TARGET_OBJECTS:tick> $<TARGET_OBJECTS:task> $<TARGET_OBJECTS:sched> $<TARGET_OBJECTS:sys> $<TARGET_OBJECTS:sem> $<TARGET_OBJECTS:shell>)
add_executable(${APP} main.c ${OBJS}) # 可执行文件