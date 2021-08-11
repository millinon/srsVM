LOAD_CONST $A 10%u8                 ; load the constant 10 as a byte into the register $A
LOAD_CONST $B 5%u8                  ; load the constant 5 as a byte into the register $B

math.MUL_U8 $RESULT $A $B           ; multiply the values of $A and $B, storing the result into the register $RESULT

conversion.U8_TO_STR $MSG $RESULT   ; convert the contents of the register $RESULT to a string, storing the result into the register $MSG

PUTS $MSG                           ; print the contents of the $MSG register to stdout
PUTS "\n"                           ; print a newline

HALT                                ; end execution
