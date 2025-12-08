#include "prt_typedef.h"
#include "prt_shell.h"
#include "os_attr_armv8_external.h"
#include "prt_task.h"
#include "prt_sem.h"

extern SemHandle sem_uart_rx;
extern U32 PRT_Printf(const char *format, ...);
extern void OsDisplayTasksInfo(void);
extern void OsDisplayCurTick(void);

/* 来自链接脚本 hi3093.ld 中的段边界符号 */
extern char __text_start;
extern char __text_end;
extern char __rodata_start;
extern char __rodata_end;
extern char __data_start;
extern char __data_end;
extern char __bss_start__;
extern char __bss_end__;
extern char __HEAP_INIT;
extern char __HEAP_END;
extern char __os_sys_sp_start;
extern char __os_sys_sp_end;

/* 在 sem 模块新增的调试接口 */
extern void OsDisplaySemStat(void);

typedef void (*ShellCmdHandler)(const char *args);

typedef struct {
    const char      *name;
    const char      *help;
    ShellCmdHandler  handler;
} ShellCmd;

/*---------------- 简单工具函数 ----------------*/

static bool ShellStrEqual(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return FALSE;
        }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

/*---------------- 命令声明 ----------------*/

static void ShellCmd_Help(const char *args);
static void ShellCmd_Ps(const char *args);
static void ShellCmd_Top(const char *args);
static void ShellCmd_Tick(const char *args);
static void ShellCmd_MemStat(const char *args);
static void ShellCmd_SemStat(const char *args);
static void ShellCmd_Quit(const char *args);

/* 命令表：新增命令只要在这里加一行，help 会自动列出 */
static const ShellCmd g_shellCmds[] = {
    { "help",    "show this help message",                 ShellCmd_Help    },
    { "ps",      "show task information snapshot",         ShellCmd_Ps      },
    { "top",     "show task information snapshot loop",    ShellCmd_Top     },
    { "tick",    "show current system tick",               ShellCmd_Tick    },
    { "memstat", "show memory layout statistics",          ShellCmd_MemStat },
    { "semstat", "show semaphore statistics",              ShellCmd_SemStat },
    { "quit",    "how to exit QEMU",                       ShellCmd_Quit    },
};

static const U32 g_shellCmdNum = sizeof(g_shellCmds) / sizeof(g_shellCmds[0]);

/*---------------- 各命令实现 ----------------*/

static void ShellCmd_Help(const char *args)
{
    (void)args;
    PRT_Printf("\nSupported commands:\n");
    for (U32 i = 0; i < g_shellCmdNum; i++) {
        PRT_Printf("  %-10s - %s\n", g_shellCmds[i].name, g_shellCmds[i].help);
    }
}

static void ShellCmd_Ps(const char *args)
{
    (void)args;
    OsDisplayTasksInfo();
}

/* 简化版 top：循环多次输出 ps，可以后续再加 CPU 占用统计 */
static void ShellCmd_Top(const char *args)
{
    (void)args;

    /* 刷新次数和间隔可以按需要调，这里简单刷 10 次 */
    const U32 loop = 10;
    const U32 dummyDelay = 1000000;

    for (U32 i = 0; i < loop; i++) {
        PRT_Printf("\n--- top snapshot %u ---\n", i);
        OsDisplayTasksInfo();

        /* 简单忙等待，避免刷太快 */
        volatile U32 d = dummyDelay;
        while (d--) {
            /* 防止被编译器优化掉 */
            __asm__ __volatile__("" ::: "memory");
        }
    }

    PRT_Printf("\n[top] done.\n");
}

static void ShellCmd_Tick(const char *args)
{
    (void)args;
    OsDisplayCurTick();
}

/* 使用链接脚本符号做内存布局可视化 */
static void ShellCmd_MemStat(const char *args)
{
    (void)args;

    uintptr_t text_start   = (uintptr_t)&__text_start;
    uintptr_t text_end     = (uintptr_t)&__text_end;
    uintptr_t rodata_start = (uintptr_t)&__rodata_start;
    uintptr_t rodata_end   = (uintptr_t)&__rodata_end;
    uintptr_t data_start   = (uintptr_t)&__data_start;
    uintptr_t data_end     = (uintptr_t)&__data_end;
    uintptr_t bss_start    = (uintptr_t)&__bss_start__;
    uintptr_t bss_end      = (uintptr_t)&__bss_end__;
    uintptr_t heap_start   = (uintptr_t)&__HEAP_INIT;
    uintptr_t heap_end     = (uintptr_t)&__HEAP_END;
    uintptr_t stack_start  = (uintptr_t)&__os_sys_sp_start;
    uintptr_t stack_end    = (uintptr_t)&__os_sys_sp_end;

    PRT_Printf("\n[MEM LAYOUT]\n");

    PRT_Printf(".text   : %p - %p (size: %u bytes)\n",
               &__text_start, &__text_end,
               (U32)(text_end - text_start));

    PRT_Printf(".rodata : %p - %p (size: %u bytes)\n",
               &__rodata_start, &__rodata_end,
               (U32)(rodata_end - rodata_start));

    PRT_Printf(".data   : %p - %p (size: %u bytes)\n",
               &__data_start, &__data_end,
               (U32)(data_end - data_start));

    PRT_Printf(".bss    : %p - %p (size: %u bytes)\n",
               &__bss_start__, &__bss_end__,
               (U32)(bss_end - bss_start));

    PRT_Printf(".heap   : %p - %p (size: %u bytes)\n",
               &__HEAP_INIT, &__HEAP_END,
               (U32)(heap_end - heap_start));

    PRT_Printf(".sys_sp : %p - %p (size: %u bytes)\n",
               &__os_sys_sp_start, &__os_sys_sp_end,
               (U32)(stack_end - stack_start));
}

static void ShellCmd_SemStat(const char *args)
{
    (void)args;
    OsDisplaySemStat();
}

static void ShellCmd_Quit(const char *args)
{
    (void)args;
    PRT_Printf("Use Ctrl-A X to exit QEMU.\n");
}

/*---------------- 命令分发 ----------------*/

static void ShellDispatchCmd(const char *line)
{
    if ((line == NULL) || (line[0] == '\0')) {
        return;
    }

    char cmd[16];
    U32 i = 0;

    /* 取第一个 token 当命令名 */
    while ((line[i] != '\0') && (line[i] != ' ') && (i < sizeof(cmd) - 1)) {
        cmd[i] = line[i];
        i++;
    }
    cmd[i] = '\0';

    const char *args = line + i;
    while (*args == ' ') {
        args++;
    }

    for (U32 idx = 0; idx < g_shellCmdNum; idx++) {
        if (ShellStrEqual(cmd, g_shellCmds[idx].name)) {
            g_shellCmds[idx].handler(args);
            return;
        }
    }

    PRT_Printf("Unknown command: %s\n", cmd);
    PRT_Printf("Type 'help' to see available commands.\n");
}

/*---------------- Shell 主任务 ----------------*/

OS_SEC_TEXT void ShellTask(uintptr_t param1, uintptr_t param2,
                           uintptr_t param3, uintptr_t param4)
{
    (void)param2;
    (void)param3;
    (void)param4;

    char ch;
    char cmdBuf[SHELL_SHOW_MAX_LEN];
    U32 idx;
    ShellCB *shellCB = (ShellCB *)param1;

    while (1) {
        PRT_Printf("\nminiEuler # ");
        idx = 0;

        for (U32 i = 0; i < SHELL_SHOW_MAX_LEN; i++) {
            cmdBuf[i] = 0;
        }

        while (1) {
            /* 阻塞等待 UART 收一个字符 */
            PRT_SemPend(sem_uart_rx, OS_WAIT_FOREVER);

            ch = shellCB->shellBuf[shellCB->shellBufReadOffset];
            shellCB->shellBufReadOffset++;
            if (shellCB->shellBufReadOffset == SHELL_SHOW_MAX_LEN) {
                shellCB->shellBufReadOffset = 0;
            }

            /* 退格处理：DEL(0x7F) 或 BS('\b') */
            if ((ch == 0x7F) || (ch == '\b')) {
                if (idx > 0) {
                    idx--;
                    cmdBuf[idx] = '\0';
                    /* 光标左移、输出空格覆盖、再左移 */
                    PRT_Printf("\b \b");
                }
                continue;
            }

            /* 回车/换行：结束本次命令 */
            if ((ch == '\r') || (ch == '\n')) {
                PRT_Printf("\n");
                cmdBuf[idx] = '\0';
                ShellDispatchCmd(cmdBuf);
                break;
            }

            /* 只接收可打印字符 */
            if ((ch >= 0x20) && (ch < 0x7F)) {
                if (idx < SHELL_SHOW_MAX_LEN - 1) {
                    cmdBuf[idx] = ch;
                    idx++;
                    PRT_Printf("%c", ch); /* 回显 */
                }
                /* 超长的直接丢弃 */
            }
        }
    }
}

/*---------------- Shell 任务创建 ----------------*/

OS_SEC_TEXT U32 ShellTaskInit(ShellCB *shellCB)
{
    U32 ret;
    struct TskInitParam param = {0};

    param.taskEntry = (TskEntryFunc)ShellTask;
    param.taskPrio  = 9;
    param.stackSize = 0x1000;
    param.args[0]   = (uintptr_t)shellCB;

    TskHandle tskHandle;
    ret = PRT_TaskCreate(&tskHandle, &param);
    if (ret != OS_OK) {
        return ret;
    }

    ret = PRT_TaskResume(tskHandle);
    if (ret != OS_OK) {
        return ret;
    }

    return OS_OK;
}
