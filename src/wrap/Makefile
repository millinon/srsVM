.PHONY: clean-obj clean

CFLAGS := -I../../include -Wall

all: progs

debug: CFLAGS += -DDEBUG -gdwarf-3
debug: LDFLAGS +=
debug: progs

release: CFLAGS += -DNEDBUG -O2 -march=native -flto
release: LDFLAGS += -s
release: progs

LIBS:=-pthread -ldl -lz

progs: srsvm srsvm_as

obj/%.o: ../lib/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/impl/linux.o: ../lib/impl/linux.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

srsvm: srsvm_wrap.c \
	obj/program.o \
	obj/impl/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

srsvm_as: srsvm_as_wrap.c \
	obj/impl/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean-obj:
	rm -rf obj

clean: clean-obj
	rm -f srsvm srsvm_as
