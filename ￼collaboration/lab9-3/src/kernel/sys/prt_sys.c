#include "prt_typedef.h"
#include "os_attr_armv8_external.h"
#include "prt_task.h"

// /* 系统主频 */
// OS_SEC_BSS U32 g_systemClock;

// /* cpup */
// OS_SEC_BSS TickEntryFunc g_tickTaskEntry;

// OS_SEC_BSS TickDispFunc g_tickDispatcher;
// /* tick中软件定时器扫描钩子 */
// OS_SEC_BSS TaskScanFunc g_taskScanHook;
// OS_SEC_BSS struct TickModInfo g_tickModInfo;

OS_SEC_L4_BSS U32 g_threadNum;
/* Tick计数 */
// OS_SEC_BSS U64 g_uniTicks;

/* 系统状态标志位 */
OS_SEC_DATA U32 g_uniFlag = 0;
// OS_SEC_BSS U32 g_tickNoRespondCnt;
OS_SEC_DATA struct TagTskCb *g_runningTask = NULL;

// src/core/kernel/task/prt_task_global.c
OS_SEC_BSS TskEntryFunc g_tskIdleEntry;
/* 任务中创建删除时调用钩子 */
// OS_SEC_BSS TaskNameAddFunc g_taskNameAdd;
// OS_SEC_BSS TaskNameGetFunc g_taskNameGet;
// OS_SEC_BSS volatile TskCoresleep g_taskCoreSleep;

OS_SEC_BSS U32 g_tskMaxNum;
OS_SEC_BSS struct TagTskCb *g_tskCbArray;
OS_SEC_BSS U32 g_tskBaseId;

OS_SEC_BSS TskHandle g_idleTaskId;
OS_SEC_BSS U16 g_uniTaskLock;
OS_SEC_BSS struct TagTskCb *g_highestTask;