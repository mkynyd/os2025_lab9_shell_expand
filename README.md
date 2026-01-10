# Lab9：MiniEuler 内核 Shell + 优先级继承 + IPC Demo（集成版）

基于 MiniEuler在 QEMU AArch64平台上提供可交互的内核 Shell，并集成：

-   可扩展 Shell 命令系统（命令表 + 函数指针分发）
-   系统可观测命令：`ps/top/tick/memstat/semstat`
-   IPC 生产者-消费者 + Ring Buffer 演示：`ipc/ipcs`
-   优先级反转 + 优先级继承（Priority Inheritance）演示：`demo`

---

## 目录结构

```text
.├── src/                 # 最终集成后的内核源码（构建入口）├── OriginalLab9/         # 原始未修改版本（对比用）├── docs/                # 实验文档/报告├── build/               # 构建产物（脚本会生成/清理）├── makeMiniEuler.sh      # 一键编译（CMake）└── runMiniEuler.sh       # 一键运行（qemu-system-aarch64）
```

---

## 构建与运行

### 1) 准备工具链

本项目使用 `aarch64-none-elf-gcc` 交叉编译工具链。请先修改：

-   `src/CMakeLists.txt`：`TOOLCHAIN_PATH` 指向你本机实际安装目录

### 2) 编译

```bash
sh makeMiniEuler.sh
```

如需打印详细编译命令：

```bash
sh makeMiniEuler.sh -v
```

### 3) 运行（QEMU）

```bash
sh runMiniEuler.sh
```

启动后在入口处暂停（便于调试）：

```bash
sh runMiniEuler.sh -S
```

> `runMiniEuler.sh` 默认带 `-s`，会开启 gdb stub（端口 `:1234`）。

退出 QEMU：

-   终端按 `Ctrl-A` 再按 `X`（Shell 中输入 `quit` 会提示该方式）

---

## Shell 命令与演示

启动后进入提示符：

```text
miniEuler #
```

输入 `help` 查看命令列表。最终版本命令包括：

-   `help`：列出命令
-   `ps`：任务快照
-   `top`：循环打印任务快照（10 次）
-   `tick`：当前 Tick 计数
-   `memstat`：内存段布局（来自链接脚本符号）
-   `semstat`：信号量统计（遍历 sem 控制块）
-   `ipc`：启动生产者-消费者 + ring buffer demo（只启动一次）
-   `ipcs`：查看 ring buffer 状态与统计（相当于 IPC stat）
-   `demo`：启动优先级反转演示（包含优先级继承日志）
-   `quit`：退出 QEMU 提示

推荐验收流程：

1.  `help`（确认命令表正常）
2.  `ps` / `tick` / `memstat` / `semstat`（确认可观测性）
3.  `ipc` → `ipcs`（确认 IPC 同步与统计）
4.  `demo`（确认优先级继承生效；演示任务期间 Shell 可能暂时无响应，等待结束即可）

---

## 文档

-   `docs/1_实现原理与创新点对比.md`：对比 `src/` vs `OriginalLab9/src/` 的改动与原理复盘
-   `docs/2_分工与整合报告.md`：模块 A/B/C 映射与整合逻辑
-   `docs/3_Shell操作说明.md`：命令清单与输出格式示例
-   `docs/4_实验报告.md`：系统设计/实现/问题解决/总结（含截图占位）