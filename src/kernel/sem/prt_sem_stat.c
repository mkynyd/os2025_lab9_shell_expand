#include "prt_sem_external.h"
#include "prt_typedef.h"
#include "os_attr_armv8_external.h"

extern U32 PRT_Printf(const char *format, ...);

/* prt_sem.c 中已有的全局变量，这里做外部声明即可 */
extern U16 g_maxSem;

/*
 * 遍历所有信号量控制块，打印基础统计信息
 * 依赖：TagSemCb、GET_SEM、OS_SEM_UNUSED、semCount、semOwner、semStat 在 prt_sem_external.h 中定义
 */
OS_SEC_TEXT void OsDisplaySemStat(void)
{
    U32 used = 0;
    U32 freeCnt = 0;

    PRT_Printf("\n[SEM LIST]\n");
    PRT_Printf("ID   Count   Owner   Stat\n");
    PRT_Printf("---------------------------\n");

    for (SemHandle h = 0; h < (SemHandle)g_maxSem; h++) {
        struct TagSemCb *sem = GET_SEM(h);

        if (sem->semStat == OS_SEM_UNUSED) {
            freeCnt++;
            continue;
        }

        used++;
        PRT_Printf("%-4u %-7u %-7d 0x%x\n",
                   (U32)h,
                   (U32)sem->semCount,
                   (S32)sem->semOwner,
                   (U32)sem->semStat);
    }

    PRT_Printf("Total: %u, Used: %u, Free: %u\n",
               (U32)g_maxSem, used, freeCnt);
}
