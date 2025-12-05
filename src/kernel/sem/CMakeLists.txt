set(SRCS prt_sem.c prt_sem_init.c)
add_library(sem OBJECT ${SRCS})  # OBJECT类型只编译生成.o目标文件，但不实际链接成库