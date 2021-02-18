.PHONY: all release debug clean install

all:
	$(MAKE) -C src

release:
	$(MAKE) -C src release

debug:
	$(MAKE) -C src debug

clean:
	$(MAKE) -C src clean

install: release
	$(MAKE) -C src install
