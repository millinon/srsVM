.PHONY: all release debug clean test install

all:
	$(MAKE) -C src

release:
	$(MAKE) -C src release

debug:
	$(MAKE) -C src debug

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean

test:
	$(MAKE) -C test clean test

install: release
	$(MAKE) -C src install
