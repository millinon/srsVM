ARGC $ARGC
LOAD_CONST $ACC 0

LOOP:
WORD_EQ $R0 $ACC $ARGC
JMP_IF #LOOP_END $R0

ARGV $ARG $ACC
PUTS $ARG
PUTS " "

INCR $ACC

JMP #LOOP

LOOP_END:
PUTS "\n"

HALT