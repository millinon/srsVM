.PHONY: clean-obj clean

CFLAGS := -I../../../include -Wall -fPIC -march=native -ftree-vectorize -ffast-math -DPREFIX='"$(PREFIX)"'

MOD_NAME := math

all: release

debug: CFLAGS += -DDEBUG -gdwarf-3
debug: LDFLAGS += 
debug: ../output/$(MOD_NAME).svmmod

release: CFLAGS += -DNDEBUG -O2 -flto
release: LDFLAGS += -s
release: ../output/$(MOD_NAME).svmmod

obj/16/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=16 -c -o $@ $<

obj/32/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=32 -c -o $@ $<

obj/64/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=64 -c -o $@ $<

obj/128/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWORD_SIZE=128 -c -o $@ $<

../output/$(MOD_NAME).svmmod: main.c \
	obj/16/mod_math.o obj/32/mod_math.o obj/64/mod_math.o obj/128/mod_math.o  \
	obj/16/loader.o obj/32/loader.o obj/64/loader.o obj/128/loader.o
	mkdir -pv $(dir $@)
	$(CC) -shared $(CFLAGS) -o $@ $^

clean-obj:
	rm -rf obj

clean: clean-obj
	rm -f ../output/$(MOD_NAME).svmmod

install: release
	install -d "$(DESTDIR)$(PREFIX)/lib/srsvm/multilib"
	install -m 0755 ../output/$(MOD_NAME).svmmod "$(DESTDIR)$(PREFIX)/lib/srsvm/multilib/"
