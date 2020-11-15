.phony: clean-obj clean

WORD_SIZE := $(if $(WORD_SIZE),$(WORD_SIZE),16)
CFLAGS:=-Iinclude -O0 -g -DDEBUG -Wall -DSRSVM_SUPPORT_COMPRESSED_MEMORY -DWORD_SIZE=$(WORD_SIZE)
LIBS:=-pthread -ldl -lz

all: test_reader_$(WORD_SIZE) test_writer_$(WORD_SIZE)

obj/$(WORD_SIZE)/%.o: src/lib/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/$(WORD_SIZE)/impl/linux.o: src/lib/impl/linux.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

test_writer_$(WORD_SIZE): test_writer.c \
	obj/$(WORD_SIZE)/program.o \
	obj/$(WORD_SIZE)/impl/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

test_reader_$(WORD_SIZE): test_reader.c \
	obj/$(WORD_SIZE)/opcodes-builtin.o \
	obj/$(WORD_SIZE)/vm.o \
	obj/$(WORD_SIZE)/module.o \
	obj/$(WORD_SIZE)/mmu.o \
	obj/$(WORD_SIZE)/opcode.o \
	obj/$(WORD_SIZE)/register.o \
	obj/$(WORD_SIZE)/constant.o \
	obj/$(WORD_SIZE)/program.o \
	obj/$(WORD_SIZE)/thread.o \
	obj/$(WORD_SIZE)/impl/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean-obj:
	rm -rf obj

clean: clean-obj
	rm -f test_reader_$(WORD_SIZE) test_writer_$(WORD_SIZE)
