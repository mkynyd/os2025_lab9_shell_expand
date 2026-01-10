// src/include/ipc_ring_demo.h

#ifndef IPC_RING_DEMO_H
#define IPC_RING_DEMO_H

#include "prt_typedef.h"

/* 和你原来 lab7 一样的配置 */
#define IPC_RING_BUF_SIZE      4
#define IPC_PRODUCER_NUM       2
#define IPC_CONSUMER_NUM       2
#define IPC_PRODUCE_PER_TASK   5
#define IPC_CONSUME_PER_TASK   5

/* 对 Shell 暴露的统计结构 */
typedef struct {
    U32 ringSize;

    U32 writeIndex;              // 下一个写入位置
    U32 readIndex;               // 下一个读出位置

    U32 producedTotal;           // 总共生产数量
    U32 consumedTotal;           // 总共消费数量

    U32 perProducer[IPC_PRODUCER_NUM];   // 每个生产者的生产数
    U32 perConsumer[IPC_CONSUMER_NUM];   // 每个消费者的消费数

    int buffer[IPC_RING_BUF_SIZE];       // ring buffer 内容快照
} RingBufStats;

/* 启动 demo：创建信号量+4 个任务，只启动一次 */
U32 IPC_RingDemoStart(void);

/* 查询当前 ring buffer 状态，供 shell 使用 */
void IPC_RingDemoGetStats(RingBufStats *out);

#endif /* IPC_RING_DEMO_H */
