#include "prt_typedef.h"
#include "prt_shell.h"
#include "os_attr_armv8_external.h"
#include "prt_task.h"
#include "prt_sem.h"

#include <string.h>

extern SemHandle sem_uart_rx;
extern U32 PRT_Printf(const char *format, ...);

/* 来自内核的现有观测函数 */
extern void OsDisplayTasksInfo(void);
extern void OsDisplayCurTick(void);

/* 命令处理函数类型 */
typedef void (*ShellCmdHandler)(const char *args);

typedef struct {
    const char     *name;
    const char     *help;
    ShellCmdHandler handler;
} ShellCmd;

/*===================== 各命令处理函数 =====================*/

static void ShellCmd_Help(const char *args)
{
    (void)args;
    PRT_Printf("\nSupported commands:\n");
    PRT_Printf("  help        - show this help message\n");
    PRT_Printf("  ps          - show task information snapshot\n");
    PRT_Printf("  top         - show task information snapshot\n");
    PRT_Printf("  tick        - show current system tick\n");
    PRT_Printf("  memstat     - show memory usage statistics (stub)\n");
    PRT_Printf("  semstat     - show semaphore statistics (stub)\n");
    PRT_Printf("  quit        - how to exit QEMU\n");
}

static void ShellCmd_Ps(const char *args)
{
    (void)args;
    OsDisplayTasksInfo();
}

static void ShellCmd_Top(const char *args)
{
    (void)args;
    /* 简化版：当前与 ps 相同，后续可以改为循环刷新版本 */
    OsDisplayTasksInfo();
}

static void ShellCmd_Tick(const char *args)
{
    (void)args;
    OsDisplayCurTick();
}

/* 注意：下面两个函数是 stub，不再依赖外部的 OsDisplayMemStat/OsDisplaySemStat */

static void ShellCmd_MemStat(const char *args)
{
    (void)args;
    PRT_Printf("\n[memstat] Not implemented yet.\n");
    PRT_Printf("You can extend ShellCmd_MemStat() to dump your lab4 memory info.\n");
}

static void ShellCmd_SemStat(const char *args)
{
    (void)args;
    PRT_Printf("\n[semstat] Not implemented yet.\n");
    PRT_Printf("You can extend ShellCmd_SemStat() to dump your lab7 semaphore info.\n");
}

static void ShellCmd_Quit(const char *args)
{
    (void)args;
    PRT_Printf("\nTo quit QEMU: press 'Ctrl-A' then 'X'.\n");
}

/*===================== 命令表 =====================*/

static const ShellCmd g_shellCmdTable[] = {
    { "help",    "show this help message",           ShellCmd_Help    },
    { "ps",      "show task information snapshot",   ShellCmd_Ps      },
    { "top",     "show task information snapshot",   ShellCmd_Top     },
    { "tick",    "show current system tick",         ShellCmd_Tick    },
    { "memstat", "show memory usage statistics",     ShellCmd_MemStat },
    { "semstat", "show semaphore statistics",        ShellCmd_SemStat },
    { "quit",    "how to exit QEMU",                 ShellCmd_Quit    },
};

#define SHELL_CMD_NUM (sizeof(g_shellCmdTable) / sizeof(g_shellCmdTable[0]))

/*===================== 命令行解析和分发 =====================*/

static void ShellDispatchCmd(char *line)
{
    /* 去掉结尾的回车换行 */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
        line[--len] = '\0';
    }

    /* 跳过前导空白 */
    char *p = line;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '\0') {
        return; /* 空行 */
    }

    /* 提取命令名：到第一个空白为止 */
    char *cmd = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        ++p;
    }
    if (*p != '\0') {
        *p++ = '\0';
    }

    /* 跳过参数前导空白（目前 args 未使用，预留扩展） */
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    const char *args = p;

    for (size_t i = 0; i < SHELL_CMD_NUM; ++i) {
        if (strcmp(cmd, g_shellCmdTable[i].name) == 0) {
            g_shellCmdTable[i].handler(args);
            return;
        }
    }

    PRT_Printf("\nUnknown command: %s\n", cmd);
    PRT_Printf("Type 'help' to see available commands.\n");
}

/*===================== Shell 主任务 =====================*/

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
            /* 阻塞等待 UART 收到一个字符 */
            PRT_SemPend(sem_uart_rx, OS_WAIT_FOREVER);

            ch = shellCB->shellBuf[shellCB->shellBufReadOffset];
            shellCB->shellBufReadOffset++;
            if (shellCB->shellBufReadOffset == SHELL_SHOW_MAX_LEN) {
                shellCB->shellBufReadOffset = 0;
            }

            /* 处理退格：0x7F (DEL) 或 0x08 (BS) */
            if (ch == 0x7F || ch == 0x08) {
                if (idx > 0) {
                    /* 从本地缓冲删一个字符 */
                    idx--;
                    cmdBuf[idx] = '\0';
                    /* 在终端上“回退一格 + 空格覆盖 + 再回退” */
                    PRT_Printf("\b \b");
                }
                continue;
            }

            /* 正常字符 */
            if (idx < SHELL_SHOW_MAX_LEN - 1) {
                cmdBuf[idx++] = ch;
            }
            PRT_Printf("%c", ch);  /* 回显 */

            /* 回车 / 换行：结束一条命令 */
            if (ch == '\r' || ch == '\n') {
                cmdBuf[idx] = '\0';
                ShellDispatchCmd(cmdBuf);
                break;
            }
        }

    }
}

/*===================== Shell 任务创建 =====================*/

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
        PRT_Printf("ShellTaskInit: create task failed, ret=0x%x\n", ret);
        return ret;
    }

    ret = PRT_TaskResume(tskHandle);
    if (ret != OS_OK) {
        PRT_Printf("ShellTaskInit: resume task failed, ret=0x%x\n", ret);
        return ret;
    }

    return OS_OK;
}
