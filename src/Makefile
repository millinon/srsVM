.PHONY: clean all debug release install

PREFIX := ${HOME}/.local
export PREFIX

all: release

debug: CFLAGS += -DDEBUG -g
debug:
	$(MAKE) -C wrap debug
	$(MAKE) WORD_SIZE=16 -C arch debug
	$(MAKE) WORD_SIZE=16 -C mod debug
	$(MAKE) WORD_SIZE=32 -C arch debug
	$(MAKE) WORD_SIZE=32 -C mod debug
	$(MAKE) WORD_SIZE=64 -C arch debug
	$(MAKE) WORD_SIZE=64 -C mod debug
	$(MAKE) WORD_SIZE=128 -C arch debug
	$(MAKE) WORD_SIZE=128 -C mod debug

release: CFLAGS += -DNEDBUG -O2
release: 
	$(MAKE) -C wrap release
	$(MAKE) WORD_SIZE=16 -C arch release
	$(MAKE) WORD_SIZE=16 -C mod release
	$(MAKE) WORD_SIZE=32 -C arch release
	$(MAKE) WORD_SIZE=32 -C mod release
	$(MAKE) WORD_SIZE=64 -C arch release
	$(MAKE) WORD_SIZE=64 -C mod release
	$(MAKE) WORD_SIZE=128 -C arch release
	$(MAKE) WORD_SIZE=128 -C mod release

clean:
	$(MAKE) -C wrap clean
	$(MAKE) -C arch clean
	$(MAKE) -C mod clean

install: release
	$(MAKE) -C wrap install
	$(MAKE) -C arch install
	$(MAKE) -C mod install
