# srsVM
A virtual machine that takes itself **very** seriously

---

srsVM is a set of tools written in C that implement a toy virtual machine. In this context, 'virtual machine' refers to a process virtual machine (like the JVM implements for Java bytecode), not a system virtual machine like VirtualBox.

srsVM implements a flexible model that supports different word sizes (16 / 32 / 64 / 128 bits), determined at build-time. The 128 bit mode is only supported if the host C library implements signed and unsigned 128-bit integral types.

A srsVM program (the compiled bytecode) currently consists of several sections:

1. Metadata
    * magic number (signature): `53 52 53` (`SRS`)
    * program word size
    * program entry point
2. Register specifications
    * Register name
    * Slot number [0 - 8\*`WORD_SIZE`)
3. Virtual memory segment specifications
    * Segment location
    * Segment size
4. Literal memory segment specifications
    * Segment location
    * Segment size
    * Segment attributes
    * Segment data
        * Data may optionally be compressed
5. Contstant value specifications
    * Value type
    * Slot number [0 - 4\*`WORD_SIZE`)
    * Value data
        * Data may optionally be compressed

Once a virtual machine is instantiated and a program loaded, a thread is started at the entry point, and the bytecode is executed until the thread encounters a fault or the `HALT` instruction is executed.

The fundamental storage unit in srsVM is a register, which is modeled somewhat after a hardware register. A register is essentially a wrapper around an area of memory which is `WORD_SIZE` bits in size, which provides different views of that memory. For example, for `WORD_SIZE` 32, a register may be viewed as any of the following:

* `uint8_t[4]`
* `int8_t[4]`
* `uint16_t[2]`
* `int16_t[2]`
* `uint32_t[1]`
* `int32_t[1]`
* `float[1]`
* `WORD`
* `POINTER`
* `POINTER_OFFSET`

A running srsVM virtual machine can also allocate memory and then load/store data between registers and memory. The limits of the virtual machine are determined by the word size and by the capacity of the host machine and OS.

---

## Included programs

The main programs included are the assembler (`srsvm_as`) and the runtime (`srsvm`). The assembler can be invoked to turn a program written in srsVM assembly (`*.s`) into a srsVM program (`*.svm`). Each program is compiled to a native executable which has a suffix specifying the supported word size; for example with `WORD_SIZE = 32` the assembler will be `srsvm_as_32` and the runtime will be `srsvm_16`. There are additionaly two programs which are provided as wrappers; the `srsvm_as` wrapper takes a `-ws` argument (e.g. `-ws 16`) to specify the target word size and invokes the corresponding assembler (e.g. `srsvm_as_16`). Likewise, the `srsvm` wrapper reads the metadata of a supplied program and invokes the corresponding runtime (e.g. `srsvm_16` for a program assembled by `srsvm_as_16`).

---

## Assembly language

srsVM uses a made-up assembly language. The first token on a line is an optional label (which is assembled into an offset jump), followed by an opcode number or mnemonic, and then the list of arguments to the opcode (separated by spaces). Here is an example program:

    LOAD_CONST $MSG "Hello, world!"     ; load the string 'Hello, world!' into the register $MSG
    
    PUTS $MSG                           ; print the contents of the $MSG register to stdout
    
    HALT                                ; end execution

Here is an example of assembling this program and then executing it in 32-bit mode:

    $ srsvm_as -ws 32 examples/hello_world.s
    Serialized to examples/hello_world.svm
    $ srsvm examples/hello_world.svm
    Hello, world!

Note that if between the first command and the second, the srsvm runtime is rebuilt and the opcode values are changed (right now they are sparsely incremented, but this may change), then the program may no longer be valid and could do something strange.

---

## Modules

srsVM enables a form of dynamic linking; opcodes may include a prefix and a period delimiter to specify that the opcode should be loaded from a module. A module is implemented as a host-native library; on Linux this is a shared object and on Windows this would be a dynamically loaded library.

Here is an example of a program using opcodes in the `math` and `conversion` libraries:

    LOAD_CONST $A 10%u8                 ; load the constant 10 as a byte into the register $A
    LOAD_CONST $B 5%u8                  ; load the constant 5 as a byte into the register $B 
    
    math.MUL_U8 $RESULT $A $B           ; multiply the values of $A and $B, storing the result into the register $RESULT
    
    conversion.U8_TO_STR $MSG $RESULT   ; convert the contents of the register $RESULT to a string, storing the result into the register $MSG
    
    PUTS $MSG                           ; print the contents of the $MSG register to stdout
    
    HALT                                ; end execution

Here is assembling and running the program in 16-bit mode:

    $ srsvm_as -ws 16 examples/math_mul.s
    Serialized to examples/math_mul.svm
    $ srsvm examples/math_mul.svm
    50

This will only work if the moduled `math.svmmod` and `conversion.svmmod` are present in an expected location; if they are not, the module search path can be specified:

    $ srsvm_as -ws 16 -L 'src/mod/math;src/mod/conversion' examples/math_mul.s
    Serialized to examples/math_mul.svm
    $ SRSVM_MOD_PATH='src/mod/math;src/mod/conversion' srsvm examples/math_mul.svm
    50

---

## Building & Installing

The makefiles currently support building and installing on Linux.

This is the suggested command for installing the release build (run these in the srsVM root directory):

    $ make clean release install

This will install srsVM to your `~/.local` directory, building all four configurations. You may optionally pass a `PREFIX` value if you want to change the installation root:

    $ make clean release install PREFIX=/some/other/prefix

`DESTDIR` is also supported to install to a temporary location:

    $ make clean release install DESTDIR=~/scratch/srsvm-install # will install to ~/scratch/srsvm-install/home/<username>/.local

If you want to do a system-wide installation (why??), you can use these commands:

    $ make clean release PREFIX=/usr/local
    $ sudo make install PREFIX=/usr/local


