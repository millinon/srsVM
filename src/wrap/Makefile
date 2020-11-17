.PHONY: clean-obj clean

CFLAGS := -I../../include -Wall

all: release

debug: CFLAGS += -DDEBUG -g
debug: progs

release: CFLAGS += -DNEDBUG -O2
release: progs

LIBS:=-pthread -ldl -lz

progs: srsvm

obj/$(WORD_SIZE)/%.o: ../lib/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/$(WORD_SIZE)/impl/linux.o: ../lib/impl/linux.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

srsvm: srsvm_wrap.c \
	obj/$(WORD_SIZE)/program.o \
	obj/$(WORD_SIZE)/impl/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean-obj:
	rm -rf obj

clean: clean-obj
	rm -f srsvm
