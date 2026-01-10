# 3. Shell 操作说明

## 3.1 启动与基本交互

系统启动后会输出 Banner，并进入交互提示符：

```text
miniEuler #
```

输入支持：

-   **字符回显**：输入的可打印字符会实时显示在终端。
-   **退格键**：支持 `Backspace`（`b`）与 `DEL(0x7F)`，可删除已输入字符。
-   **回车提交**：按 `Enter`（`r`/`n`）提交命令行。

退出 QEMU（无图形模式）常用方式：

-   `Ctrl-A` 然后按 `X`

---

## 3.2 当前实际支持的所有命令

命令在 `src/kernel/shell/shmsg.c` 的 `g_shellCmds[]` 注册，最终版本支持：

命令

作用

备注

`help`

列出所有命令

自动从命令表生成

`ps`

打印任务快照

任务 ID / Priority / Stack Size

`top`

循环打印任务快照（10 次）

简化版 top

`tick`

打印当前系统 Tick

基于 `g_uniTicks`

`memstat`

打印内存布局（段边界/大小）

读取链接脚本符号

`semstat`

打印信号量使用情况

遍历 sem 控制块

`ipc`

启动 ring-buffer 生产者/消费者 demo

只会启动一次

`ipcs`

查看 ring-buffer 统计与快照

相当于“ipcstat”

`demo`

启动优先级继承（优先级反转）演示

运行期间 Shell 可能暂时无响应

`quit`

打印退出 QEMU 的提示

不是直接退出

---

## 3.3 预期输出格式示例（按代码实际格式）

> 注意：以下示例中的 PID、地址、计数等会因运行时状态而变化，示例只展示“输出结构”。

### 1) `help`

```text
Supported commands:  help       - show this help message  ps         - show task information snapshot  top        - show task information snapshot loop  tick       - show current system tick  memstat    - show memory layout statistics  semstat    - show semaphore statistics  ipc        - start ipc ring-buffer demo  ipcs       - show ipc ring-buffer statistics  demo       - run priority inheritance demonstration  quit       - how to exit QEMU
```

### 2) `ps`

来自 `src/kernel/task/prt_task.c` 的 `OsDisplayTasksInfo()`：

```text
PID         Priority        Stack Size0           9               40961           31              4096...Total N tasks
```

### 3) `top`

`top` 会循环输出 10 次任务快照，格式为：

```text
--- top snapshot 0 ---PID         Priority        Stack Size...Total N tasks--- top snapshot 1 ---...[top] done.
```

### 4) `tick`

来自 `src/kernel/tick/prt_tick.c` 的 `OsDisplayCurTick()`：

```text
Current Tick: 12345
```

### 5) `memstat`

来自 `src/kernel/shell/shmsg.c` 的 `ShellCmd_MemStat()`：

```text
[MEM LAYOUT].text   : 0xXXXXXXXX - 0xXXXXXXXX (size: XXXX bytes).rodata : 0xXXXXXXXX - 0xXXXXXXXX (size: XXXX bytes).data   : 0xXXXXXXXX - 0xXXXXXXXX (size: XXXX bytes).bss    : 0xXXXXXXXX - 0xXXXXXXXX (size: XXXX bytes).heap   : 0xXXXXXXXX - 0xXXXXXXXX (size: XXXX bytes).sys_sp : 0xXXXXXXXX - 0xXXXXXXXX (size: XXXX bytes)
```

### 6) `semstat`

来自 `src/kernel/sem/prt_sem_stat.c` 的 `OsDisplaySemStat()`：

```text
[SEM LIST]ID   Count   Owner   Stat---------------------------0    1       0       0x11    0       -2      0x1...Total: M, Used: U, Free: F
```

说明：

-   `Owner` 若为无效值，通常会显示为 `-2`（`OS_INVALID_OWNER_ID` 的有符号打印效果）。

### 7) `ipc`（启动生产者-消费者 demo）

执行 `ipc` 后会创建 3 个信号量和 4 个任务（2 Producer + 2 Consumer），典型输出结构：

```text
ipc ring-buffer demo started.[P1] produce item 1 at slot 0    [C1] consume item 1 from slot 0[P2] produce item 101 at slot 1    [C2] consume item 101 from slot 1...[P1] producer done.[P2] producer done.    [C1] consumer done.    [C2] consumer done.
```

### 8) `ipcs`（查看 IPC ring-buffer 状态；等价“ipcstat”）

来自 `src/kernel/shell/shmsg.c` 的 `ShellCmd_Ipcs()`：

```text
Ring buffer size: 4writeIndex = 2, readIndex = 2Produced total = 10, Consumed total = 10Per Producer:  P1: 5  P2: 5Per Consumer:  C1: 5  C2: 5Buffer snapshot:  slot[0] = 0  slot[1] = 0  slot[2] = 0  slot[3] = 0
```

### 9) `demo`（优先级反转 + 优先级继承演示）

执行 `demo` 会创建演示任务。Shell 会先打印提示，然后后台任务输出示例结构：

```text
========== 启动优先级反转演示 ==========...[Demo] 优先级反转演示任务已创建[Low] 低优先级任务启动 ...[High] 尝试获取共享资源 ...[PriorityInherit] Task PID=...: priority 15 -> 5 ......[PriorityRestore] Task PID=...: priority 5 -> 15 ...
```

注意：

-   演示任务优先级高于 Shell（Shell 任务优先级为 9），因此演示期间 Shell 可能暂时无法响应；等待演示任务结束后即可恢复。

### 10) `quit`

```text
Use Ctrl-A X to exit QEMU.
```

---

## 3.4 如何启动生产者-消费者演示（基于命令触发）

1.  进入提示符后输入：
    -   `ipc`（回车）：启动 demo（只启动一次）。
2.  demo 运行期间/结束后输入：
    -   `ipcs`（回车）：查看 ring buffer 状态与统计（用于展示与验收）。