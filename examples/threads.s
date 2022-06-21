MUTEX_INIT $IO_MUT

THREAD_START $T1 #THREAD_TARGET
THREAD_START $T2 #THREAD_TARGET
THREAD_START $T3 #THREAD_TARGET
THREAD_START $T4 #THREAD_TARGET
THREAD_START $T5 #THREAD_TARGET

THREAD_JOIN $T1
THREAD_JOIN $T2
THREAD_JOIN $T3
THREAD_JOIN $T4
THREAD_JOIN $T5

HALT

THREAD_TARGET:
MUTEX_LOCK $IO_MUT
THREAD_ID $ID

PUTS "Hello from thread "
PUT $ID
PUTS "!\n"
MUTEX_UNLOCK $IO_MUT

SLEEP 1000

JMP #THREAD_TARGET
