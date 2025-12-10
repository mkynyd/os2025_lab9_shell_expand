#  0. 请务必将src/CMakeLists.txt中的TOOLCHAIN_PATH的路径修改为实际路径！

## 1. 目录结构概览

本仓库主要目录如下（仅列出与 Lab9 相关的部分）：

```text
.
├── makeMiniEuler.sh         # 构建脚本（调用 CMake + GNU Make）
├── runMiniEuler.sh          # 运行脚本（调用 qemu-system-aarch64）
├── src/
│   ├── CMakeLists.txt       # Lab9 工程 CMake 配置（需修改工具链路径）
│   ├── kernel/
│   │   ├── shell/
│   │   │   └── shmsg.c      # Shell 主任务与命令实现
│   │   ├── task/            # 任务管理相关源码（来自 UniProton）
│   │   ├── sem/
│   │   │   ├── prt_sem.c
│   │   │   ├── prt_sem_init.c
│   │   │   └── prt_sem_stat.c # 新增：信号量调试打印
│   │   └── ...
│   ├── bsp/                 # QEMU virt 平台相关启动与外设支持
│   └── main.c               # Lab9 主函数（初始化内核模块 + 启动 Shell）
└── ...
```

------

## 2. 编译前准备

### 2.1 交叉编译工具链

本工程使用 ARM GNU Toolchain 的 `aarch64-none-elf` 交叉编译链，示例版本：

- `arm-gnu-toolchain-14.3.rel1-aarch64-none-elf`
- 主可执行文件：`aarch64-none-elf-gcc` 等

请先在宿主机/虚拟机中安装好该工具链，并记下实际安装路径，例如：

```bash
/home/<your_user>/arm-gnu-toolchain-14.3.rel1-aarch64-aarch64-none-elf
```

> 注意：不同环境下目录名可能略有差异，请以实际安装路径为准。

### 2.2 QEMU

需要安装支持 AArch64 的 QEMU，例如：

```bash
sudo apt-get install qemu-system-arm
```

本工程运行时使用：

```bash
qemu-system-aarch64 -machine virt -cpu cortex-a53 -nographic -kernel build/miniEuler
```

该命令已封装在 `runMiniEuler.sh` 脚本中。

------

## 3. 修改 `src/CMakeLists.txt` 中的工具链路径（必做）

工程默认假定一个本地工具链路径，你需要根据自己的安装路径修改。

打开：

```text
src/CMakeLists.txt
```

在文件开头附近可以看到类似配置（示例）：

```cmake
# 交叉编译工具链根目录，需要根据实际环境修改
set(TOOLCHAIN_PATH "/home/yinjunhang/arm-gnu-toolchain-14.3.rel1-aarch64-aarch64-none-elf")

set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/bin/aarch64-none-elf-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/bin/aarch64-none-elf-gcc)
set(CMAKE_LINKER ${TOOLCHAIN_PATH}/bin/aarch64-none-elf-ld)
```

请将 `TOOLCHAIN_PATH` 修改为你本机的实际安装路径，例如：

```cmake
set(TOOLCHAIN_PATH "/home/<your_user>/arm-gnu-toolchain-14.3.rel1-aarch64-aarch64-none-elf")
```

确保以下命令在终端中可以正常执行（路径与 CMake 中一致）：

```bash
$ /home/<your_user>/arm-gnu-toolchain-14.3.rel1-aarch64-aarch64-none-elf/bin/aarch64-none-elf-gcc --version
```

如果工具链在 `PATH` 中，也可以直接改为：

```cmake
set(CMAKE_C_COMPILER aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER aarch64-none-elf-g++)
set(CMAKE_ASM_COMPILER aarch64-none-elf-gcc)
set(CMAKE_LINKER aarch64-none-elf-ld)
```

但推荐仍显式设置 `TOOLCHAIN_PATH`，便于在报告中说明环境。

------

## 4. 编译与运行

在 Ubuntu/QEMU 环境中执行：

```bash
cd /path/to/os2025_lab9_shell_expand

# 清理旧构建（可选）
rm -rf build

# 构建
./makeMiniEuler.sh

# 运行
./runMiniEuler.sh
```

如果一切正常，终端会出现 miniEuler 的 ASCII 标志和 Shell 提示符：

```text
miniEuler #
```

此时可以输入 `help` 查看所有支持的命令。

------

## 5. Shell 命令说明

本工程在 UniProton/miniEuler 上实现了一个简易内核 Shell，并扩展了若干“可观测性”相关命令。

### 5.1 `help`

```text
miniEuler # help
```

输出当前可用的所有命令及其简要说明。命令列表来自内部命令表，新增命令时只需在表中增加一项，`help` 会自动更新。

### 5.2 `ps`

```text
miniEuler # ps
```

打印任务快照信息，例如：

```text
PID             Priority        Stack Size
1               63              4096
0               9               4096
Total 2 tasks
```

用于验证任务管理与调度子系统是否正常工作。

### 5.3 `top`

```text
miniEuler # top
```

简化版 `top`：循环多次输出 `ps` 的结果，观察随时间变化的任务列表：

```text
--- top snapshot 0 ---
PID ...
...

--- top snapshot 9 ---
...
[top] done.
```

用于演示任务列表的周期性刷新行为。

### 5.4 `tick`

```text
miniEuler # tick
```

输出当前系统 Tick 计数，例如：

```text
Current Tick: 9610
```

用于验证定时器/时钟中断子系统工作正常。

### 5.5 `memstat`

```text
miniEuler # memstat
```

基于链接脚本导出的符号，展示镜像各段在内存中的布局，例如：

```text
[MEM LAYOUT]
.text   : 0x40000000 - 0x40005950 (size: 22864 bytes)
.rodata : ...
.data   : ...
.bss    : ...
.heap   : ...
.sys_sp : ...
```

可配合 `hi3093.ld` 的 MEMORY/SECTIONS 说明，分析代码段、数据段、BSS、堆、系统栈在 IMU_SRAM 区域的占用情况，是报告中“内存管理与布局”部分的直观依据。

### 5.6 `semstat`

```text
miniEuler # semstat
```

遍历内核信号量控制块数组，输出当前所有“在用”信号量的状态：

```text
[SEM LIST]
ID   Count   Owner   Stat
---------------------------
0    0       0       0x1
Total: 73, Used: 1, Free: 72
```

可以看到 UART RX 使用的同步信号量以及系统信号量池的使用情况，有利于分析同步原语的正确性和资源占用。

### 5.7 `quit`

```text
miniEuler # quit
```

输出如何退出 QEMU（`Ctrl-A X`），方便在演示和调试时快速关闭虚拟机。

------

