## 一、在 shell 里加命令

我们在 `shmsg.c` 里做两件事：

1. 头文件里 `#include "ipc_ring_demo.h"`
2. 在命令解析 if-else 里加两个分支：`ipc` 和 `ipcs`

### 1. 修改头部 include

原来的 `shmsg.c` 头上大概是这样（你 ppt 里那段）：

```c
#include "prt_typedef.h"
#include "prt_shell.h"
#include "os_attr_armv8_external.h"
#include "prt_task.h"
#include "prt_sem.h"

extern SemHandle sem_uart_rx;
extern U32 PRT_Printf(const char *format, ...);
extern void OsDisplayTasksInfo(void);
extern void OsDisplayCurTick(void);
```

改成：

```c
#include "prt_typedef.h"
#include "prt_shell.h"
#include "os_attr_armv8_external.h"
#include "prt_task.h"
#include "prt_sem.h"
#include "ipc_ring_demo.h"   // ★ 新增

extern SemHandle sem_uart_rx;
extern U32 PRT_Printf(const char *format, ...);
extern void OsDisplayTasksInfo(void);
extern void OsDisplayCurTick(void);
```

---

### 2. 在 `ShellTask` 中加命令分支

你的 `ShellTask` 现在差不多长这样（简化了一下）：

```c
S32 ShellTask(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4)
{
    U32 ret;
    char ch;
    char cmd[SHELL_SHOW_MAX_LEN];
    U32 idx;
    ShellCB *shellCB = (ShellCB *)param1;

    while (1) {
        PRT_Printf("\nminiEuler # ");
        idx = 0;
        for (int i = 0; i < SHELL_SHOW_MAX_LEN; i++) {
            cmd[i] = 0;
        }

        while (1) {
            PRT_SemPend(sem_uart_rx, OS_WAIT_FOREVER);

            ch = shellCB->shellBuf[shellCB->shellBufReadOffset];
            cmd[idx] = ch;
            idx++;
            shellCB->shellBufReadOffset++;
            if (shellCB->shellBufReadOffset == SHELL_SHOW_MAX_LEN) {
                shellCB->shellBufReadOffset = 0;
            }

            PRT_Printf("%c", ch);

            if (ch == '\r') {
                break;
            }
        }

        PRT_Printf("\n");

        if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'p') {
            OsDisplayTasksInfo();
        } else if (cmd[0] == 't' && cmd[1] == 'i' && cmd[2] == 'c' && cmd[3] == 'k') {
            OsDisplayCurTick();
        } else {
            PRT_Printf("unknown command: %s\n", cmd);
        }
    }
}
```

在两个已存在的分支之间，插入我们的 `ipc` / `ipcs` 逻辑：

```c
        if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'p') {
            OsDisplayTasksInfo();

        } else if (cmd[0] == 't' && cmd[1] == 'i' && cmd[2] == 'c' && cmd[3] == 'k') {
            OsDisplayCurTick();

        } else if (cmd[0] == 'i' && cmd[1] == 'p' && cmd[2] == 'c' && cmd[3] == '\r') {
            /* miniEuler # ipc  —— 启动生产者-消费者 demo */
            (void)IPC_RingDemoStart();

        } else if (cmd[0] == 'i' && cmd[1] == 'p' && cmd[2] == 'c' && cmd[3] == 's') {
            /* miniEuler # ipcs —— 查看当前 ring buffer 状态 */
            RingBufStats stats;
            IPC_RingDemoGetStats(&stats);

            PRT_Printf("Ring buffer size: %u\n", stats.ringSize);
            PRT_Printf("writeIndex = %u, readIndex = %u\n",
                       stats.writeIndex, stats.readIndex);
            PRT_Printf("Produced total = %u, Consumed total = %u\n",
                       stats.producedTotal, stats.consumedTotal);

            PRT_Printf("Per Producer:\n");
            for (U32 i = 0; i < IPC_PRODUCER_NUM; i++) {
                PRT_Printf("  P%u: %u\n", i + 1, stats.perProducer[i]);
            }

            PRT_Printf("Per Consumer:\n");
            for (U32 i = 0; i < IPC_CONSUMER_NUM; i++) {
                PRT_Printf("  C%u: %u\n", i + 1, stats.perConsumer[i]);
            }

            PRT_Printf("Buffer snapshot:\n");
            for (U32 i = 0; i < stats.ringSize; i++) {
                PRT_Printf("  slot[%u] = %d\n", i, stats.buffer[i]);
            }

        } else {
            PRT_Printf("unknown command: %s\n", cmd);
        }
```

> 解析规则很“笨”，但完全跟文档给的 `top` / `tick` 一致，这样助教一看就知道是你自己手写的。

---

## 二、main.c 不用动（重点）

现在 **main.c 只负责：**

* OS 基础模块初始化（任务、信号量、tick、MMU、shell 等）
* 调用 `ShellTaskInit(&g_shellCB)` 创建一个 shell 任务
* 调用 `OsActivate()` 启动调度

你的生产者–消费者 demo 完全通过 shell 命令触发：

```text
miniEuler # ipc      <- 创建 4 个任务 + 3 个信号量
[P1] produce item 1 ...
    [C1] consume item 1 ...
...
[P1] producer done.
...
[P2] producer done.

miniEuler # ipcs
Ring buffer size: 4
writeIndex = ...
readIndex = ...
Produced total = 10, Consumed total = 10
Per Producer:
  P1: 5
  P2: 5
Per Consumer:
  C1: 5
  C2: 5
Buffer snapshot:
  slot[0] = ...
  ...
miniEuler #
```

这样展示的时候，你就可以说：

> 我们在 lab9 的 shell 之上，集成了一个“基于 OS 原生信号量和任务机制实现的并发 IPC ring buffer demo”。
> 通过 `ipc` 命令动态启动 4 个并发任务（两个生产者两个消费者），通过 `ipcs` 命令实时查看 ring buffer 内部状态和统计信息，从而验证 OS 提供的同步原语在实际并发程序中的使用效果。

---

## 三、实际操作的顺序

1. 在 `lab9/src/include` 新建 `ipc_ring_demo.h`，复制上面的内容。
2. 在 `lab9/src` 新建 `ipc_ring_demo.c`，复制上面的内容。
3. 修改 `lab9/src/CMakeLists.txt`，把 `ipc_ring_demo.c` 加进 `add_executable(miniEuler ...)`。
4. 修改 `kernel/shell/shmsg.c`：

   * 加 `#include "ipc_ring_demo.h"`
   * 在 `ShellTask` 命令解析里插入 `ipc` / `ipcs` 两个分支。
5. `bash ./makeMiniEuler.sh`
6. `bash ./runMiniEuler.sh`
7. QEMU 里依次输入：

   * `ipc` 回车，看生产者消费者并发打印。
   * `ipcs` 回车，看 ring buffer 状态。

如果中间某一步报错，你把报错信息贴给我，我帮你对着改 👀
