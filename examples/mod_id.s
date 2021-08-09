LOAD_CONST $A 0xcc79%u16
LOAD_CONST $B 0x1234%u16

math.ADD_U16 $RESULT $A $B

conversion.U16_TO_STR_HEX $RESULT $RESULT

MOD_ID $MOD_ID "math"
JMP_ERR $MOD_ID #MATH_NOT_LOADED

PUT $MOD_ID
JMP #CHECK_CONV_MOD

MATH_NOT_LOADED:
PUTS "math module not loaded?"

CHECK_CONV_MOD:

MOD_ID $MOD_ID "conversion"
JMP_ERR $MOD_ID #CONV_NOT_LOADED

PUT $MOD_ID
JMP #END

CONV_NOT_LOADED:
PUTS "conversion module not loaded?"

END:

HALT
