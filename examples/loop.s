LOAD_CONST $ACC 0

LOOP_START: PUTS "Help, I'm stuck in a loop!"

INCR $ACC
WORD_EQ $R0 $ACC 10
JMP_IF #LOOP_END $R0

JMP #LOOP_START

LOOP_END: PUTS "All done."

HALT
