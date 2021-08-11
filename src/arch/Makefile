.PHONY: clean-obj clean

CFLAGS := -I../../include -fpic -Wall -DSRSVM_INTERNAL -DWORD_SIZE=$(WORD_SIZE) -DPREFIX='"$(PREFIX)"' -DSRSVM_SUPPORT_COMPRESSION
LDFLAGS := -rdynamic -fvisibility=hidden

all: release

debug: CFLAGS += -DDEBUG -gdwarf-3
debug: LDFLAGS += 
debug: progs

release: CFLAGS += -DNEDBUG -O2 -march=native -flto
release: LDFLAGS += -s
release: progs

LIBS:=-pthread -ldl -lz

progs: srsvm_$(WORD_SIZE) srsvm_as_$(WORD_SIZE) srsvm_run_$(WORD_SIZE)

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

srsvm_run_$(WORD_SIZE): srsvm_run.c \
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
	for arch in 16 32 64 128; do \
		rm -f srsvm_$$arch srsvm_as_$$arch srsvm_run_$$arch; \
	done

install: 
	install -d "$(DESTDIR)$(PREFIX)/libexec/srsvm"
	for arch in 16 32 64 128; do \
		install -s -m 0755 srsvm_$$arch "$(DESTDIR)$(PREFIX)/libexec/srsvm/"; \
		install -s -m 0755 srsvm_as_$$arch "$(DESTDIR)$(PREFIX)/libexec/srsvm/"; \
		install -s -m 0755 srsvm_run_$$arch "$(DESTDIR)$(PREFIX)/libexec/srsvm/"; \
		done
