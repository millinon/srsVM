.PHONY: clean-obj clean

CFLAGS :=-I../../include -Wall -DSRSVM_SUPPORT_COMPRESSION -DWORD_SIZE=$(WORD_SIZE)

all: release

debug: CFLAGS += -DDEBUG -gdwarf-3
debug: LDFLAGS += 
debug: progs

release: CFLAGS += -DNEDBUG -O2 -march=native -flto
release: LDFLAGS += -s
release: progs

LIBS:=-pthread -ldl -lz

progs: srsvm_$(WORD_SIZE) srsvm_as_$(WORD_SIZE)

obj/$(WORD_SIZE)/%.o: ../lib/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/$(WORD_SIZE)/impl/linux.o: ../lib/impl/linux.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

srsvm_$(WORD_SIZE): srsvm.c \
	obj/$(WORD_SIZE)/opcodes-builtin.o \
	obj/$(WORD_SIZE)/vm.o \
	obj/$(WORD_SIZE)/module.o \
	obj/$(WORD_SIZE)/map.o \
	obj/$(WORD_SIZE)/mmu.o \
	obj/$(WORD_SIZE)/opcode.o \
	obj/$(WORD_SIZE)/register.o \
	obj/$(WORD_SIZE)/constant.o \
	obj/$(WORD_SIZE)/program.o \
	obj/$(WORD_SIZE)/thread.o \
	obj/$(WORD_SIZE)/impl/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

srsvm_as_$(WORD_SIZE): srsvm_as.c \
	obj/$(WORD_SIZE)/asm.o \
	obj/$(WORD_SIZE)/lru.o \
	obj/$(WORD_SIZE)/opcodes-builtin.o \
	obj/$(WORD_SIZE)/vm.o \
	obj/$(WORD_SIZE)/module.o \
	obj/$(WORD_SIZE)/map.o \
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
	rm -f srsvm_$(WORD_SIZE) srsvm_as_$(WORD_SIZE)
