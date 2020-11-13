.phony: clean-obj clean-all

CFLAGS:=-Iinclude -O0 -g -DDEBUG
LIBS:=-pthread

all: test_16 test_32 test_64 test_128

test_16: $(eval WORD_SIZE:=16)
test_16: build_test_16
	mv build_test_16 test_16

test_32: $(eval WORD_SIZE:=32)
test_32: build_test_32
	mv build_test_32 test_32

test_64: $(eval WORD_SIZE:=64)
test_64: build_test_64
	mv build_test_64 test_64

test_128: $(eval WORD_SIZE:=128)
test_128: build_test_128
	mv build_test_128 test_128

obj/16/test.o: test.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=16 -c -o $@ $<

obj/16/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=16 -c -o $@ $<

obj/32/test.o: test.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=32 -c -o $@ $<

obj/32/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=32 -c -o $@ $<

obj/64/test.o: test.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=64 -c -o $@ $<

obj/64/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=64 -c -o $@ $<

obj/128/test.o: test.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=128 -c -o $@ $<

obj/128/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=128 -c -o $@ $<

obj/impl/linux.o: src/impl/linux.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build_test_%: obj/%/test.o obj/%/opcodes-builtin.o obj/%/vm.o obj/%/mmu.o obj/%/memory.o obj/%/opcode.o obj/%/thread.o obj/%/impl/linux.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean-obj:
	rm -rf obj

clean-all: clean-obj
	rm -f test_16 test_32 test_64 test_128
