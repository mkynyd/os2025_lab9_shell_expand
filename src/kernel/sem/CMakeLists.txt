set(SRCS
    prt_sem.c
    prt_sem_init.c
    prt_sem_stat.c
)

add_library(sem OBJECT ${SRCS})
