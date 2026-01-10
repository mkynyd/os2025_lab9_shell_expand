// src/ipc_ring_demo.c

#include "ipc_ring_demo.h"
#include "prt_sem.h"
#include "prt_task.h"
#include "prt_tick.h"
#include <string.h>

extern U32 PRT_Printf(const char *format, ...);

/* ---------- 环形缓冲区数据 ---------- */

static U32 g_ringBuf[IPC_RING_BUF_SIZE];
static U32 g_head = 0;              // 写指针
static U32 g_tail = 0;              // 读指针

/* 统计信息 */
static U32 g_producedTotal = 0;
static U32 g_consumedTotal = 0;
static U32 g_producedPerProducer[IPC_PRODUCER_NUM] = {0};
static U32 g_consumedPerConsumer[IPC_CONSUMER_NUM] = {0};

/* ---------- 信号量句柄 ---------- */

static SemHandle g_semEmpty;        // 空槽个数
static SemHandle g_semFull;         // 已填充个数
static SemHandle g_semMutex;        // 缓冲区互斥访问

/* 防止重复启动 */
static U32 g_demoStarted = 0;

/* ---------- 对 Shell 暴露的查询接口 ---------- */

void IPC_RingDemoGetStats(RingBufStats *out)
{
    if (out == NULL) {
        return;
    }

    /* 还没启动 demo，就给个空的默认值 */
    if (!g_demoStarted) {
        memset(out, 0, sizeof(RingBufStats));
        out->ringSize = IPC_RING_BUF_SIZE;
        return;
    }

    (void)PRT_SemPend(g_semMutex, OS_WAIT_FOREVER);

    out->ringSize      = IPC_RING_BUF_SIZE;
    out->writeIndex    = g_head;
    out->readIndex     = g_tail;
    out->producedTotal = g_producedTotal;
    out->consumedTotal = g_consumedTotal;

    for (U32 i = 0; i < IPC_PRODUCER_NUM; i++) {
        out->perProducer[i] = g_producedPerProducer[i];
    }
    for (U32 i = 0; i < IPC_CONSUMER_NUM; i++) {
        out->perConsumer[i] = g_consumedPerConsumer[i];
    }
    for (U32 i = 0; i < IPC_RING_BUF_SIZE; i++) {
        out->buffer[i] = (int)g_ringBuf[i];
    }

    (void)PRT_SemPost(g_semMutex);
}

/* ---------- 任务入口函数 ---------- */

static void ProducerTaskEntry1(void)
{
    U32 i;
    for (i = 0; i < IPC_PRODUCE_PER_TASK; i++) {
        (void)PRT_SemPend(g_semEmpty, OS_WAIT_FOREVER);
        (void)PRT_SemPend(g_semMutex, OS_WAIT_FOREVER);

        U32 slot = g_head;
        U32 item = i + 1;   // P1 产生 1~5
        g_ringBuf[g_head] = item;
        g_head = (g_head + 1) % IPC_RING_BUF_SIZE;

        g_producedTotal++;
        g_producedPerProducer[0]++;

        PRT_Printf("[P1] produce item %u at slot %u\n", item, slot);

        (void)PRT_SemPost(g_semMutex);
        (void)PRT_SemPost(g_semFull);
    }

    PRT_Printf("[P1] producer done.\n");
}

static void ProducerTaskEntry2(void)
{
    U32 i;
    for (i = 0; i < IPC_PRODUCE_PER_TASK; i++) {
        (void)PRT_SemPend(g_semEmpty, OS_WAIT_FOREVER);
        (void)PRT_SemPend(g_semMutex, OS_WAIT_FOREVER);

        U32 slot = g_head;
        U32 item = 100 + i + 1;   // P2 产生 101~105
        g_ringBuf[g_head] = item;
        g_head = (g_head + 1) % IPC_RING_BUF_SIZE;

        g_producedTotal++;
        g_producedPerProducer[1]++;

        PRT_Printf("[P2] produce item %u at slot %u\n", item, slot);

        (void)PRT_SemPost(g_semMutex);
        (void)PRT_SemPost(g_semFull);
    }

    PRT_Printf("[P2] producer done.\n");
}

static void ConsumerTaskEntry1(void)
{
    U32 i;
    for (i = 0; i < IPC_CONSUME_PER_TASK; i++) {
        (void)PRT_SemPend(g_semFull, OS_WAIT_FOREVER);
        (void)PRT_SemPend(g_semMutex, OS_WAIT_FOREVER);

        U32 slot = g_tail;
        U32 item = g_ringBuf[g_tail];
        g_tail = (g_tail + 1) % IPC_RING_BUF_SIZE;

        g_consumedTotal++;
        g_consumedPerConsumer[0]++;

        PRT_Printf("    [C1] consume item %u from slot %u\n", item, slot);

        (void)PRT_SemPost(g_semMutex);
        (void)PRT_SemPost(g_semEmpty);
    }

    PRT_Printf("    [C1] consumer done.\n");
}

static void ConsumerTaskEntry2(void)
{
    U32 i;
    for (i = 0; i < IPC_CONSUME_PER_TASK; i++) {
        (void)PRT_SemPend(g_semFull, OS_WAIT_FOREVER);
        (void)PRT_SemPend(g_semMutex, OS_WAIT_FOREVER);

        U32 slot = g_tail;
        U32 item = g_ringBuf[g_tail];
        g_tail = (g_tail + 1) % IPC_RING_BUF_SIZE;

        g_consumedTotal++;
        g_consumedPerConsumer[1]++;

        PRT_Printf("    [C2] consume item %u from slot %u\n", item, slot);

        (void)PRT_SemPost(g_semMutex);
        (void)PRT_SemPost(g_semEmpty);
    }

    PRT_Printf("    [C2] consumer done.\n");
}

/* ---------- 对外启动函数 ---------- */

U32 IPC_RingDemoStart(void)
{
    if (g_demoStarted) {
        PRT_Printf("ipc demo already started.\n");
        return OS_OK;
    }

    U32 ret;

    /* 创建信号量：empty = 空槽；full = 已填；mutex = 互斥锁 */
    ret = PRT_SemCreate(IPC_RING_BUF_SIZE, &g_semEmpty);
    if (ret != OS_OK) {
        PRT_Printf("create semEmpty failed: %u\n", ret);
        return ret;
    }

    ret = PRT_SemCreate(0, &g_semFull);
    if (ret != OS_OK) {
        PRT_Printf("create semFull failed: %u\n", ret);
        return ret;
    }

    ret = PRT_SemCreate(1, &g_semMutex);
    if (ret != OS_OK) {
        PRT_Printf("create semMutex failed: %u\n", ret);
        return ret;
    }

    struct TskInitParam param;
    TskHandle tP1, tP2, tC1, tC2;

    /* Producer 1 */
    (void)memset(&param, 0, sizeof(param));
    param.taskEntry = (TskEntryFunc)ProducerTaskEntry1;
    param.taskPrio  = 35;
    param.stackSize = 0x1000;
    ret = PRT_TaskCreate(&tP1, &param);
    if (ret != OS_OK) {
        PRT_Printf("create P1 failed: %u\n", ret);
        return ret;
    }
    (void)PRT_TaskResume(tP1);

    /* Producer 2 */
    (void)memset(&param, 0, sizeof(param));
    param.taskEntry = (TskEntryFunc)ProducerTaskEntry2;
    param.taskPrio  = 36;
    param.stackSize = 0x1000;
    ret = PRT_TaskCreate(&tP2, &param);
    if (ret != OS_OK) {
        PRT_Printf("create P2 failed: %u\n", ret);
        return ret;
    }
    (void)PRT_TaskResume(tP2);

    /* Consumer 1 */
    (void)memset(&param, 0, sizeof(param));
    param.taskEntry = (TskEntryFunc)ConsumerTaskEntry1;
    param.taskPrio  = 30;
    param.stackSize = 0x1000;
    ret = PRT_TaskCreate(&tC1, &param);
    if (ret != OS_OK) {
        PRT_Printf("create C1 failed: %u\n", ret);
        return ret;
    }
    (void)PRT_TaskResume(tC1);

    /* Consumer 2 */
    (void)memset(&param, 0, sizeof(param));
    param.taskEntry = (TskEntryFunc)ConsumerTaskEntry2;
    param.taskPrio  = 31;
    param.stackSize = 0x1000;
    ret = PRT_TaskCreate(&tC2, &param);
    if (ret != OS_OK) {
        PRT_Printf("create C2 failed: %u\n", ret);
        return ret;
    }
    (void)PRT_TaskResume(tC2);

    g_demoStarted = 1;
    PRT_Printf("ipc ring-buffer demo started.\n");

    return OS_OK;
}
