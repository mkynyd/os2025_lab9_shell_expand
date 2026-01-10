/*
 * 优先级反转演示程序（带优先级继承机制）
 * 
 * 本版本实现了优先级继承（Priority Inheritance）机制来解决优先级反转问题
 * 
 * 演示场景：
 * 1. Low优先级任务（Priority=15）获得共享资源（信号量）
 * 2. High优先级任务（Priority=5）需要资源，但被Low占用，被阻塞
 * 3. 优先级继承机制启动：Low任务的优先级临时提升到5（和High一样）
 * 4. Mid优先级任务（Priority=10）无法抢占Low任务（因为Low优先级=5）
 * 5. Low任务快速完成并释放资源，High任务获得资源
 * 
 * 对比无优先级继承版本：
 * - 无优先级继承：High等待时间 = Low时间 + Mid时间（约57 ticks）
 * - 有优先级继承：High等待时间 = 只有Low时间（约18 ticks）
 */

#include "prt_typedef.h"
#include "prt_task.h"
#include "prt_sem.h"
#include "prt_tick.h"
#include "prt_task_external.h"

extern U32 PRT_Printf(const char *format, ...);

// 共享资源信号量（互斥锁）
static SemHandle g_sharedResource;

// High和Mid任务的PID（在Low任务中创建）
static TskHandle g_taskHighPid = 0;
static TskHandle g_taskMidPid = 0;

// 用于延迟的Tick计数
static U64 GetTickCount(void)
{
    extern volatile U64 g_uniTicks;
    return g_uniTicks;
}

// 简单延迟函数（忙等待）
static void DelayTicks(U32 ticks)
{
    U64 start = GetTickCount();
    while ((GetTickCount() - start) < ticks) {
        // 忙等待
    }
}

/*
 * 高优先级任务（Priority = 5）
 * 需要访问共享资源，但可能被Low任务阻塞
 */
static void TaskHigh(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4)
{
    (void)param1;
    (void)param2;
    (void)param3;
    (void)param4;
    
    U32 loopCount = 0;
    const U32 MAX_LOOPS = 3;  // 运行3次后结束
    
    PRT_Printf("\n========== 优先级反转演示开始（带优先级继承） ==========\n");
    PRT_Printf("任务优先级：High=5, Mid=10, Low=15（数值越小优先级越高）\n");
    PRT_Printf("共享资源：信号量（互斥锁）\n");
    PRT_Printf("优先级继承：当High等待资源时，Low的优先级会临时提升到5\n");
    PRT_Printf("效果：Mid无法抢占Low，High等待时间显著减少\n\n");
    
    PRT_Printf("[High] 高优先级任务启动，抢占Low任务 (Tick=%d)\n", (U32)GetTickCount());
    
    // High任务会立即抢占Low任务，但尝试获取资源时会被阻塞
    PRT_Printf("[High] 尝试获取共享资源 (Tick=%d)...\n", (U32)GetTickCount());
    PRT_Printf("[High] 注意：资源被Low持有，High任务被阻塞！\n");
    
    while (loopCount < MAX_LOOPS) {
        loopCount++;
        U64 tickBefore = GetTickCount();
        
        // 尝试获取共享资源（这里会被阻塞，因为Low任务持有资源）
        // 这就是优先级反转的关键：High任务被迫等待Low任务释放资源
        PRT_SemPend(g_sharedResource, OS_WAIT_FOREVER);
        
        U64 tickAfter = GetTickCount();
        U32 waitTime = (U32)(tickAfter - tickBefore);
        
        PRT_Printf("[High] Loop=%d: ✓ 获得共享资源！等待时间=%d ticks\n", 
                   loopCount, waitTime);
        PRT_Printf("[High] Loop=%d: 使用共享资源中...\n", loopCount);
        
        // 使用资源（模拟工作）
        // 分段执行，给 Shell 任务运行机会
        for (U32 k = 0; k < 5; k++) {
            for (volatile U32 j = 0; j < 100000; j++) {
                // 忙等待，模拟工作
            }
            // 每段工作后短暂让出 CPU（虽然优先级高，但可以让 Shell 在中断后有机会）
        }
        
        // 释放资源
        PRT_SemPost(g_sharedResource);
        PRT_Printf("[High] Loop=%d: 释放共享资源\n", loopCount);
        
        // 让出CPU，给其他任务（如 Shell）运行机会
        // 使用较短的延迟，确保 Shell 能及时响应
        for (volatile U32 j = 0; j < 100000; j++) {
            // 忙等待，让出 CPU
        }
    }
    
    PRT_Printf("[High] 已运行%d次，任务结束\n", loopCount);
}

/*
 * 中优先级任务（Priority = 10）
 * 不访问共享资源，但由于High任务被阻塞，Mid可以运行
 */
static void TaskMid(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4)
{
    (void)param1;
    (void)param2;
    (void)param3;
    (void)param4;
    
    U32 loopCount = 0;
    const U32 MAX_LOOPS = 15;  // 运行15次后结束
    
    PRT_Printf("[Mid] 中优先级任务启动 (Tick=%d)\n", (U32)GetTickCount());
    PRT_Printf("[Mid] 注意：High任务被阻塞，Mid任务可以运行\n");
    
    while (loopCount < MAX_LOOPS) {
        loopCount++;
        PRT_Printf("[Mid] Loop=%d: 运行中（不访问共享资源） Tick=%d\n", 
                   loopCount, (U32)GetTickCount());
        
        // 做一些工作（模拟计算密集型任务）
        for (volatile U32 j = 0; j < 800000; j++) {
            // 忙等待，模拟工作
        }
    }
    
    PRT_Printf("[Mid] 已运行%d次，任务结束\n", loopCount);
}

/*
 * 低优先级任务（Priority = 15）
 * 先获得共享资源，使用较长时间
 */
static void TaskLow(uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4)
{
    (void)param1;
    (void)param2;
    (void)param3;
    (void)param4;
    
    U32 loopCount = 0;
    const U32 MAX_LOOPS = 2;  // 运行2次后结束
    U32 ret;
    struct TskInitParam initParam = {0};
    
    PRT_Printf("[Low] 低优先级任务启动 (Tick=%d)\n", (U32)GetTickCount());
    
    // 步骤1：首先立即获取资源
    PRT_Printf("[Low] 步骤1: 准备获取共享资源 (Tick=%d)...\n", (U32)GetTickCount());
    PRT_SemPend(g_sharedResource, OS_WAIT_FOREVER);
    PRT_Printf("[Low] 步骤1: ✓ 获得共享资源！\n");
    
    // 步骤2：创建High任务（High会立即抢占Low，但尝试获取资源时会被阻塞）
    PRT_Printf("[Low] 步骤2: 创建High任务 (Tick=%d)...\n", (U32)GetTickCount());
    initParam.taskEntry = TaskHigh;
    initParam.taskPrio = 5;   // 高优先级
    initParam.args[0] = 0;
    initParam.args[1] = 0;
    initParam.args[2] = 0;
    initParam.args[3] = 0;
    initParam.stackSize = 4096;
    initParam.stackAddr = 0;
    ret = PRT_TaskCreate(&g_taskHighPid, &initParam);
    if (ret != OS_OK) {
        PRT_Printf("[Low] 创建High任务失败: 0x%x\n", ret);
        return;
    }
    PRT_Printf("[Low] 创建High优先级任务成功: PID=%d, Priority=5\n", g_taskHighPid);
    ret = PRT_TaskResume(g_taskHighPid);
    if (ret != OS_OK) {
        PRT_Printf("[Low] 恢复High任务失败: 0x%x\n", ret);
        return;
    }
    PRT_Printf("[Low] 步骤2: High任务已启动，会立即抢占Low任务\n");
    
    // 等待一段时间，让High任务有机会运行并尝试获取资源（会被阻塞）
    for (volatile U32 i = 0; i < 500000; i++) {
        // 忙等待，High任务会抢占
    }
    
    // 步骤3：创建Mid任务（Mid会等待High，但High被阻塞了，所以Mid可以运行）
    PRT_Printf("[Low] 步骤3: 创建Mid任务 (Tick=%d)...\n", (U32)GetTickCount());
    initParam.taskEntry = TaskMid;
    initParam.taskPrio = 10;  // 中优先级
    initParam.args[0] = 0;
    initParam.args[1] = 0;
    initParam.args[2] = 0;
    initParam.args[3] = 0;
    initParam.stackSize = 4096;
    initParam.stackAddr = 0;
    ret = PRT_TaskCreate(&g_taskMidPid, &initParam);
    if (ret != OS_OK) {
        PRT_Printf("[Low] 创建Mid任务失败: 0x%x\n", ret);
    } else {
        PRT_Printf("[Low] 创建Mid优先级任务成功: PID=%d, Priority=10\n", g_taskMidPid);
        ret = PRT_TaskResume(g_taskMidPid);
        if (ret != OS_OK) {
            PRT_Printf("[Low] 恢复Mid任务失败: 0x%x\n", ret);
        } else {
            PRT_Printf("[Low] 步骤3: Mid任务已启动，由于High被阻塞，Mid可以运行\n");
        }
    }
    
    // 等待Mid任务运行完成
    for (volatile U32 i = 0; i < 2000000; i++) {
        // 忙等待，Mid任务会运行
    }
    
    // 步骤4：Mid任务完成后，Low任务继续运行，释放资源
    PRT_Printf("[Low] 步骤4: Mid任务完成，Low任务继续运行 (Tick=%d)\n", (U32)GetTickCount());
    
    while (loopCount < MAX_LOOPS) {
        loopCount++;
        PRT_Printf("[Low] Loop=%d: 使用共享资源中 (Tick=%d)...\n", 
                   loopCount, (U32)GetTickCount());
        
        // 使用资源（模拟工作）
        for (U32 i = 0; i < 5; i++) {
            PRT_Printf("[Low] Loop=%d: 使用资源中... (步骤 %d/5) Tick=%d\n", 
                       loopCount, i + 1, (U32)GetTickCount());
            for (volatile U32 j = 0; j < 500000; j++) {
                // 忙等待，模拟工作
            }
        }
        
        // 释放资源（High任务会立即获得资源并运行）
        PRT_Printf("[Low] Loop=%d: ✓ 释放共享资源 (Tick=%d)\n", 
                   loopCount, (U32)GetTickCount());
        PRT_SemPost(g_sharedResource);
        PRT_Printf("[Low] 步骤5: 资源已释放，High任务会立即获得资源并运行\n");
        
        // 短暂延迟，让High任务有机会运行
        // 注意：High任务运行完成后会自动结束，不需要无限等待
        for (volatile U32 i = 0; i < 300000; i++) {
            // 忙等待，给High任务运行时间
        }
        
        if (loopCount < MAX_LOOPS) {
            // 重新获取资源，进入下一个循环
            // 如果High任务已经结束，资源会被立即获取
            PRT_Printf("[Low] Loop=%d: 重新获取共享资源...\n", loopCount + 1);
            PRT_SemPend(g_sharedResource, OS_WAIT_FOREVER);
        } else {
            // 最后一次循环，不需要重新获取资源
            // 等待High任务完成（如果还在运行）
            PRT_Printf("[Low] 等待High任务完成...\n");
            for (volatile U32 i = 0; i < 500000; i++) {
                // 忙等待，给High任务完成时间
            }
        }
    }
    
    PRT_Printf("[Low] 已运行%d次，任务结束\n", loopCount);
    PRT_Printf("\n========== 优先级反转演示完成 ==========\n");
    PRT_Printf("所有演示任务已结束\n");
    PRT_Printf("提示：按 Enter 键继续，Shell 提示符将出现\n");
    PRT_Printf("==========================================\n");
    
    // 任务函数返回，触发 OsTaskExit，任务真正结束
    // 这会触发任务调度，Shell 任务应该会运行
}

/*
 * 创建优先级反转演示任务
 */
U32 CreatePriorityInversionDemo(void)
{
    U32 ret;
    TskHandle taskHighPid, taskMidPid, taskLowPid;
    struct TskInitParam initParam = {0};  // 初始化为0
    
    PRT_Printf("\n========== 创建优先级反转演示任务 ==========\n");
    
    // 创建共享资源信号量（互斥锁，初始值为1）
    ret = PRT_SemCreate(1, &g_sharedResource);
    if (ret != OS_OK) {
        PRT_Printf("创建信号量失败: 0x%x\n", ret);
        return ret;
    }
    PRT_Printf("创建共享资源信号量成功: semHandle=%d\n", g_sharedResource);
    
    // 创建低优先级任务（Priority = 15，最低优先级）
    initParam.taskEntry = TaskLow;
    initParam.taskPrio = 15;  // 低优先级
    initParam.args[0] = 0;
    initParam.args[1] = 0;
    initParam.args[2] = 0;
    initParam.args[3] = 0;
    initParam.stackSize = 4096;
    initParam.stackAddr = 0;
    ret = PRT_TaskCreate(&taskLowPid, &initParam);
    if (ret != OS_OK) {
        PRT_Printf("创建Low任务失败: 0x%x\n", ret);
        return ret;
    }
    PRT_Printf("创建Low优先级任务成功: PID=%d, Priority=15\n", taskLowPid);
    ret = PRT_TaskResume(taskLowPid);
    if (ret != OS_OK) {
        PRT_Printf("恢复Low任务失败: 0x%x\n", ret);
    } else {
        PRT_Printf("Low任务已加入就绪队列\n");
    }
    
    PRT_Printf("注意：只创建Low任务，让它先运行并获取资源\n");
    PRT_Printf("Mid和High任务将由Low任务在获取资源后创建\n");
    
    PRT_Printf("========== Low任务创建完成 ==========\n");
    PRT_Printf("预期执行顺序：\n");
    PRT_Printf("1. Low任务开始运行\n");
    PRT_Printf("2. Low任务获得共享资源\n");
    PRT_Printf("3. Low任务创建High任务，High抢占Low\n");
    PRT_Printf("4. High任务尝试获取资源，但被Low持有，High被阻塞\n");
    PRT_Printf("5. Low任务创建Mid任务，由于High被阻塞，Mid可以运行\n");
    PRT_Printf("6. Mid任务执行完成\n");
    PRT_Printf("7. Low任务继续执行，释放资源\n");
    PRT_Printf("8. High任务获得资源并执行\n");
    PRT_Printf("9. Low任务执行结束\n\n");
    
    return OS_OK;
}

