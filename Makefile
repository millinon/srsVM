.PHONY: all release debug clean

all:
	$(MAKE) -C src

release:
	$(MAKE) -C src release

debug:
	$(MAKE) -C src debug

clean:
	$(MAKE) -C src clean
