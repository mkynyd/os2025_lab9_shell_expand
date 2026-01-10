# Lab9-3: 优先级继承 + Shell 系统

## 📋 项目说明

Lab9-3 是 **Lab9-1（优先级继承）** 和 **Lab9-2（Shell 系统）** 的合并版本，同时具备：

1. ✅ **优先级继承机制**：解决优先级反转问题
2. ✅ **Shell 命令系统**：提供 8 个调试命令（包括 `demo` 命令）

## 🎯 核心功能

### 1. 优先级继承机制

当高优先级任务等待低优先级任务持有的资源时，临时提升低优先级任务的优先级，防止中优先级任务抢占，从而减少高优先级任务的等待时间。

**效果**：
- High 任务等待时间从 57 ticks 减少到 20 ticks（减少 65%）
- Mid 任务无法抢占 Low 任务（Low 优先级临时提升）

### 2. Shell 命令系统

提供 8 个 Shell 命令用于系统调试和监控：

| 命令 | 功能 | 说明 |
|------|------|------|
| `help` | 显示帮助 | 列出所有可用命令 |
| `ps` | 任务快照 | 显示当前所有任务信息 |
| `top` | 任务监控 | 循环显示任务信息（10次） |
| `tick` | 系统时钟 | 显示当前系统 Tick 计数 |
| `memstat` | 内存布局 | 显示内存各段布局信息 |
| `semstat` | 信号量统计 | 显示所有信号量状态 |
| `demo` | 优先级反转演示 | 启动优先级继承演示 |
| `quit` | 退出提示 | 显示如何退出 QEMU |

## 🚀 快速开始

### 编译前准备

1. **修改工具链路径**：编辑 `src/CMakeLists.txt`，将 `TOOLCHAIN_PATH` 修改为你的实际路径

2. **确保 QEMU 已安装**：
```bash
sudo apt-get install qemu-system-arm
```

### 编译

```bash
cd /home/zzq/桌面/lab-all/lab9-3
rm -rf build/
sh makeMiniEuler.sh
```

### 运行

```bash
sh runMiniEuler.sh
```

## 📊 预期行为

### 1. 优先级反转演示

运行后会自动启动优先级反转演示，你会看到：

```
[PriorityInherit] Task PID=0: priority 15 -> 5 (inherited from waiter PID=3, prio=5)
[PriorityRestore] Task PID=0: priority 5 -> 15 (restored to original)
[High] Loop=1: ✓ 获得共享资源！等待时间=20 ticks
```

### 2. Shell 提示符

演示任务运行完成后，会出现 Shell 提示符：

```
miniEuler #
```

此时可以输入命令，例如：
- `help` - 查看所有命令
- `demo` - 启动优先级反转演示
- `ps` - 查看任务信息
- `semstat` - 查看信号量状态（可以看到优先级反转演示使用的信号量）
- `tick` - 查看当前 Tick

## 📁 文件结构

```
lab9-3/
├── src/
│   ├── kernel/
│   │   ├── sem/
│   │   │   ├── prt_sem.c          # 优先级继承实现
│   │   │   ├── prt_sem_init.c
│   │   │   └── prt_sem_stat.c      # 信号量统计（来自 lab9-2）
│   │   ├── shell/
│   │   │   └── shmsg.c             # Shell 命令实现（来自 lab9-2）
│   │   └── task/
│   │       └── prt_task_init.c    # 任务初始化（设置originalPriority）
│   ├── include/
│   │   └── prt_task_external.h    # 添加originalPriority字段
│   ├── main.c                      # 同时调用演示和 Shell
│   └── test_priority_inversion.c  # 优先级反转演示程序
├── makeMiniEuler.sh
├── runMiniEuler.sh
└── README.md
```

## 🔍 关键代码位置

### 优先级继承
- **优先级继承函数**: `src/kernel/sem/prt_sem.c` - `OsPriorityInherit()`
- **优先级恢复函数**: `src/kernel/sem/prt_sem.c` - `OsPriorityRestore()`
- **数据结构**: `src/include/prt_task_external.h` - `TagTskCb.originalPriority`

### Shell 系统
- **Shell 命令实现**: `src/kernel/shell/shmsg.c`
- **信号量统计**: `src/kernel/sem/prt_sem_stat.c`

## 🧪 测试方法

### 1. 编译测试

```bash
cd lab9-3
rm -rf build/
sh makeMiniEuler.sh
```

应该看到 `[100%] Built target miniEuler`，表示编译成功。

### 2. 功能测试

```bash
sh runMiniEuler.sh
```

**测试优先级继承**：
- 观察 `[PriorityInherit]` 和 `[PriorityRestore]` 消息
- 观察 High 任务的等待时间（应该是 20 ticks 左右）

**测试 Shell 命令**：
- 等待 Shell 提示符出现
- 输入 `help` 查看所有命令
- 输入 `demo` 启动优先级反转演示
- 输入 `ps` 查看任务信息
- 输入 `semstat` 查看信号量状态
- 输入 `tick` 查看当前 Tick

## 📚 相关文档

- **Lab9-1 文档**: `../lab9-1/README.md` - 优先级继承详细说明
- **Lab9-2 文档**: `../lab9-2/README.md` - Shell 系统详细说明
- **原始问题演示**: `../lab9/优先级反转演示说明.md`

## ✅ 合并验证

### 功能验证清单

- [x] 优先级继承机制正常工作
- [x] Shell 命令系统正常工作
- [x] 优先级反转演示自动运行
- [x] Shell 提示符正常出现
- [x] 所有 Shell 命令可以正常使用
- [x] 信号量统计功能正常
- [x] 编译无错误无警告

## 🎓 学习要点

1. **优先级反转问题**：高优先级任务被低优先级任务间接阻塞
2. **优先级继承机制**：临时提升持有者优先级，防止被抢占
3. **Shell 系统**：内核调试和系统监控工具
4. **系统可观测性**：通过 Shell 命令观察系统状态

## 💡 技术亮点

- ✅ **功能完整**：既有问题演示，又有解决方案，还有调试工具
- ✅ **效果明显**：等待时间减少 65%，易于验证
- ✅ **易于调试**：Shell 命令提供系统可观测性
- ✅ **代码清晰**：有详细的调试输出

---

**最后更新**: 2024年12月
