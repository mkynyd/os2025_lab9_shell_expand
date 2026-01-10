#include "prt_sem_external.h"
#include "prt_asm_cpu_external.h"
#include "os_attr_armv8_external.h"
#include "os_cpu_armv8_external.h"

/* 核内信号量最大个数 */
OS_SEC_BSS U16 g_maxSem;

extern U32 PRT_Printf(const char *format, ...);

/*
 * 优先级继承机制实现
 * 
 * 优先级继承（Priority Inheritance）协议：
 * 当高优先级任务等待低优先级任务持有的资源时，临时提升低优先级任务的优先级
 * 到高优先级任务的级别，防止中优先级任务抢占低优先级任务。
 */

/*
 * 描述：查找等待队列中最高优先级的任务
 * 返回：最高优先级任务的优先级，如果没有等待任务，返回一个很大的值
 */
OS_SEC_ALW_INLINE INLINE TskPrior OsSemFindHighestWaiterPriority(struct TagSemCb *semPended)
{
    struct TagTskCb *taskCb = NULL;
    TskPrior highestPrio = 255;  // 初始化为一个很大的值（低优先级）
    
    if (ListEmpty(&semPended->semList)) {
        return highestPrio;
    }
    
    LIST_FOR_EACH(taskCb, &semPended->semList, struct TagTskCb, pendList) {
        if (taskCb->priority < highestPrio) {
            highestPrio = taskCb->priority;
        }
    }
    
    return highestPrio;
}

/*
 * 描述：实现优先级继承
 * 当高优先级任务等待资源时，提升资源持有者的优先级
 */
OS_SEC_L0_TEXT void OsPriorityInherit(struct TagSemCb *semPended, struct TagTskCb *waiterTask)
{
    struct TagTskCb *ownerTask = NULL;
    TskPrior highestWaiterPrio;
    
    // 如果信号量没有被持有，不需要继承
    if (semPended->semOwner == OS_INVALID_OWNER_ID) {
        return;
    }
    
    // 获取资源持有者
    ownerTask = GET_TCB_HANDLE(semPended->semOwner);
    if (ownerTask == NULL || TSK_IS_UNUSED(ownerTask)) {
        return;
    }
    
    // 查找等待队列中的最高优先级
    highestWaiterPrio = OsSemFindHighestWaiterPriority(semPended);
    if (waiterTask->priority < highestWaiterPrio) {
        highestWaiterPrio = waiterTask->priority;
    }
    
    // 如果持有者优先级低于等待者，需要提升持有者优先级
    if (ownerTask->priority > highestWaiterPrio) {
        // 保存原始优先级（如果当前优先级等于原始优先级，说明还没有进行过继承）
        if (ownerTask->priority == ownerTask->originalPriority) {
            ownerTask->originalPriority = ownerTask->priority;
        }
        
        // 提升持有者优先级到等待者的优先级
        TskPrior oldPrio = ownerTask->priority;
        ownerTask->priority = highestWaiterPrio;
        
        // 如果持有者在就绪队列中，需要重新加入队列（因为优先级改变了）
        if (TSK_STATUS_TST(ownerTask, OS_TSK_READY)) {
            OsTskReadyDel(ownerTask);
            OsTskReadyAdd(ownerTask);
        }
        
        // 调试输出
        extern U32 PRT_Printf(const char *format, ...);
        PRT_Printf("[PriorityInherit] Task PID=%d: priority %d -> %d (inherited from waiter PID=%d, prio=%d)\n",
                   ownerTask->taskPid, oldPrio, ownerTask->priority,
                   waiterTask->taskPid, waiterTask->priority);
    }
}

/*
 * 描述：恢复优先级继承
 * 当资源被释放时，恢复持有者的原始优先级
 */
OS_SEC_L0_TEXT void OsPriorityRestore(struct TagSemCb *semPosted)
{
    struct TagTskCb *ownerTask = NULL;
    TskPrior highestWaiterPrio;
    
    // 如果信号量没有被持有，不需要恢复
    if (semPosted->semOwner == OS_INVALID_OWNER_ID) {
        return;
    }
    
    // 获取资源持有者
    ownerTask = GET_TCB_HANDLE(semPosted->semOwner);
    if (ownerTask == NULL || TSK_IS_UNUSED(ownerTask)) {
        return;
    }
    
    // 如果当前优先级等于原始优先级，说明没有进行过优先级继承，不需要恢复
    if (ownerTask->priority == ownerTask->originalPriority) {
        return;
    }
    
    // 查找等待队列中剩余任务的最高优先级
    highestWaiterPrio = OsSemFindHighestWaiterPriority(semPosted);
    
    // 如果还有更高优先级的等待者，保持继承的优先级
    if (highestWaiterPrio < ownerTask->priority) {
        // 还有更高优先级的等待者，继续继承
        TskPrior oldPrio = ownerTask->priority;
        ownerTask->priority = highestWaiterPrio;
        extern U32 PRT_Printf(const char *format, ...);
        PRT_Printf("[PriorityInherit] Task PID=%d: priority %d -> %d (still has waiters with prio=%d)\n",
                   ownerTask->taskPid, oldPrio, ownerTask->priority, highestWaiterPrio);
        
        // 如果持有者在就绪队列中，需要重新加入队列（因为优先级改变了）
        if (TSK_STATUS_TST(ownerTask, OS_TSK_READY)) {
            OsTskReadyDel(ownerTask);
            OsTskReadyAdd(ownerTask);
        }
        return;
    }
    
    // 没有更高优先级的等待者，恢复原始优先级
    TskPrior oldPrio = ownerTask->priority;
    ownerTask->priority = ownerTask->originalPriority;
    
    // 如果持有者在就绪队列中，需要重新加入队列（因为优先级改变了）
    if (TSK_STATUS_TST(ownerTask, OS_TSK_READY)) {
        OsTskReadyDel(ownerTask);
        OsTskReadyAdd(ownerTask);
    }
    
    // 调试输出
    extern U32 PRT_Printf(const char *format, ...);
    PRT_Printf("[PriorityRestore] Task PID=%d: priority %d -> %d (restored to original)\n",
               ownerTask->taskPid, oldPrio, ownerTask->priority);
}


OS_SEC_ALW_INLINE INLINE U32 OsSemPostErrorCheck(struct TagSemCb *semPosted, SemHandle semHandle)
{
    (void)semHandle;
    /* 检查信号量控制块是否UNUSED，排除大部分错误场景 */
    if (semPosted->semStat == OS_SEM_UNUSED) {
        return OS_ERRNO_SEM_INVALID;
    }

    /* post计数型信号量的错误场景, 释放计数型信号量且信号量计数大于最大计数 */
    if ((semPosted)->semCount >= OS_SEM_COUNT_MAX) {
        return OS_ERRNO_SEM_OVERFLOW;
    }

    return OS_OK;
}


/*
* 描述：把当前运行任务挂接到信号量链表上
*/
OS_SEC_L0_TEXT void OsSemPendListPut(struct TagSemCb *semPended, U32 timeOut)
{
    struct TagTskCb *curTskCb = NULL;
    struct TagTskCb *runTsk = RUNNING_TASK;
    struct TagListObject *pendObj = &runTsk->pendList;

    OsTskReadyDel((struct TagTskCb *)runTsk);

    runTsk->taskSem = (void *)semPended;

    TSK_STATUS_SET(runTsk, OS_TSK_PEND);
    
    /* 根据唤醒方式挂接此链表，同优先级再按FIFO子顺序插入 */
    if (semPended->semMode == SEM_MODE_PRIOR) {
        LIST_FOR_EACH(curTskCb, &semPended->semList, struct TagTskCb, pendList) {
            if (curTskCb->priority > runTsk->priority) {
                ListTailAdd(pendObj, &curTskCb->pendList);
                // 优先级继承：在将任务加入等待队列后，检查是否需要提升持有者优先级
                OsPriorityInherit(semPended, runTsk);
                return;
            }
        }
    }
    /* 如果到这里，说明是FIFO方式；或者是优先级方式且挂接首个节点或者挂接尾节点 */
    ListTailAdd(pendObj, &semPended->semList);
    
    /* 优先级继承：在将任务加入等待队列后，检查是否需要提升持有者优先级 */
    OsPriorityInherit(semPended, runTsk);
}

/*
* 描述：从非空信号量链表上摘首个任务放入到ready队列
*/
OS_SEC_L0_TEXT struct TagTskCb *OsSemPendListGet(struct TagSemCb *semPended)
{
    struct TagTskCb *taskCb = GET_TCB_PEND(LIST_FIRST(&(semPended->semList)));

    ListDelete(LIST_FIRST(&(semPended->semList)));
    /* 如果阻塞的任务属于定时等待的任务时候，去掉其定时等待标志位，并将其从去除 */
    if (TSK_STATUS_TST(taskCb, OS_TSK_TIMEOUT)) {
        OS_TSK_DELAY_LOCKED_DETACH(taskCb);
    }

    /* 必须先去除 OS_TSK_TIMEOUT 态，再入队[睡眠时是先出ready队，再置OS_TSK_TIMEOUT态] */
    TSK_STATUS_CLEAR(taskCb, OS_TSK_TIMEOUT | OS_TSK_PEND);
    taskCb->taskSem = NULL;
    /* 如果去除信号量阻塞位后，该任务不处于阻塞态则将该任务挂入就绪队列并触发任务调度 */
    if (!TSK_STATUS_TST(taskCb, OS_TSK_SUSPEND)) {
        OsTskReadyAddBgd(taskCb);
    }

    return taskCb;
}

OS_SEC_L0_TEXT U32 OsSemPendParaCheck(U32 timeout)
{
    if (timeout == 0) {
        return OS_ERRNO_SEM_UNAVAILABLE;
    }

    if (OS_TASK_LOCK_DATA != 0) {
        return OS_ERRNO_SEM_PEND_IN_LOCK;
    }
    return OS_OK;
}

OS_SEC_L0_TEXT bool OsSemPendNotNeedSche(struct TagSemCb *semPended, struct TagTskCb *runTsk)
{
    if (semPended->semCount > 0) {
        semPended->semCount--;
        semPended->semOwner = runTsk->taskPid;

        return TRUE;
    }
    return FALSE;
}

/*
* 描述：指定信号量的P操作
*/
OS_SEC_L0_TEXT U32 PRT_SemPend(SemHandle semHandle, U32 timeout)
{
    uintptr_t intSave;
    U32 ret;
    struct TagTskCb *runTsk = NULL;
    struct TagSemCb *semPended = NULL;

    if (semHandle >= (SemHandle)g_maxSem) {
        return OS_ERRNO_SEM_INVALID;
    }

    semPended = GET_SEM(semHandle);

    intSave = OsIntLock();
    if (semPended->semStat == OS_SEM_UNUSED) {
        OsIntRestore(intSave);
        return OS_ERRNO_SEM_INVALID;
    }

    if (OS_INT_ACTIVE) {
        OsIntRestore(intSave);
        return OS_ERRNO_SEM_PEND_INTERR;
    }

    runTsk = (struct TagTskCb *)RUNNING_TASK;

    if (OsSemPendNotNeedSche(semPended, runTsk) == TRUE) {
        OsIntRestore(intSave);
        return OS_OK;
    }

    ret = OsSemPendParaCheck(timeout);
    if (ret != OS_OK) {
        OsIntRestore(intSave);
        return ret;
    }
    /* 把当前任务挂接在信号量链表上 */
    OsSemPendListPut(semPended, timeout);
    if (timeout != OS_WAIT_FOREVER) {
        OsIntRestore(intSave);
        return OS_ERRNO_SEM_FUNC_NOT_SUPPORT;
    } else {
        /* 恢复ps的快速切换 */
        OsTskScheduleFastPs(intSave);

    }

    OsIntRestore(intSave);
    return OS_OK;
}

OS_SEC_ALW_INLINE INLINE void OsSemPostSchePre(struct TagSemCb *semPosted)
{
    struct TagTskCb *resumedTask = NULL;
    struct TagTskCb *oldOwner = NULL;
    TskHandle oldOwnerId;

    // 保存旧持有者ID，用于优先级恢复
    oldOwnerId = semPosted->semOwner;
    
    // 优先级恢复：在从等待队列移除任务之前，恢复旧持有者的优先级
    // 这样OsPriorityRestore可以正确检查等待队列中剩余的任务
    if (oldOwnerId != OS_INVALID_OWNER_ID) {
        oldOwner = GET_TCB_HANDLE(oldOwnerId);
        if (oldOwner != NULL && !TSK_IS_UNUSED(oldOwner)) {
            OsPriorityRestore(semPosted);
        }
    }
    
    // 从等待队列获取第一个任务
    resumedTask = OsSemPendListGet(semPosted);
    semPosted->semOwner = resumedTask->taskPid;
}

/*
* 描述：判断信号量post是否有效
* 备注：以下情况表示post无效，返回TRUE: post互斥二进制信号量，若该信号量被嵌套pend或者已处于空闲状态
*/
OS_SEC_ALW_INLINE INLINE bool OsSemPostIsInvalid(struct TagSemCb *semPosted)
{
    if (GET_SEM_TYPE(semPosted->semType) == SEM_TYPE_BIN) {
        /* 释放互斥二进制信号量且信号量已处于空闲状态 */
        if ((semPosted)->semCount == OS_SEM_FULL) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
* 描述：指定信号量的V操作
*/
OS_SEC_L0_TEXT U32 PRT_SemPost(SemHandle semHandle)
{
    U32 ret;
    uintptr_t intSave;
    struct TagSemCb *semPosted = NULL;

    if (semHandle >= (SemHandle)g_maxSem) {
        return OS_ERRNO_SEM_INVALID;
    }

    semPosted = GET_SEM(semHandle);
    intSave = OsIntLock();

    ret = OsSemPostErrorCheck(semPosted, semHandle);
    if (ret != OS_OK) {
        OsIntRestore(intSave);
        return ret;
    }

    /* 信号量post无效，不需要调度 */
    if (OsSemPostIsInvalid(semPosted) == TRUE) {
        OsIntRestore(intSave);
        return OS_OK;
    }

    /* 如果有任务阻塞在信号量上，就激活信号量阻塞队列上的首个任务 */
    if (!ListEmpty(&semPosted->semList)) {
        OsSemPostSchePre(semPosted);
        /* 相当于快速切换+中断恢复 */
        OsTskScheduleFastPs(intSave);
    } else {
        // 没有等待任务，恢复持有者优先级
        OsPriorityRestore(semPosted);
        semPosted->semCount++;
        semPosted->semOwner = OS_INVALID_OWNER_ID;
    }

    OsIntRestore(intSave);
    return OS_OK;
}