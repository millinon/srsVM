.PHONY: clean all debug release

all: release

debug: CFLAGS += -DDEBUG -g
debug:
	$(MAKE) -C wrap debug
	$(MAKE) WORD_SIZE=16 -C arch debug
	$(MAKE) WORD_SIZE=32 -C arch debug
	$(MAKE) WORD_SIZE=64 -C arch debug
	$(MAKE) WORD_SIZE=128 -C arch debug

release: CFLAGS += -DNEDBUG -O2
release:
	$(MAKE) -C wrap release
	$(MAKE) WORD_SIZE=16 -C arch release
	$(MAKE) WORD_SIZE=32 -C arch release
	$(MAKE) WORD_SIZE=64 -C arch release
	$(MAKE) WORD_SIZE=128 -C arch release

clean:
	$(MAKE) -C wrap clean
	$(MAKE) WORD_SIZE=16 -C arch clean
	$(MAKE) WORD_SIZE=32 -C arch clean
	$(MAKE) WORD_SIZE=64 -C arch clean
	$(MAKE) WORD_SIZE=128 -C arch clean
