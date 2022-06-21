.PHONY: clean all debug release install

all: release

debug:
	$(MAKE) -C math debug
	$(MAKE) -C conversion debug

release:
	$(MAKE) -C math release
	$(MAKE) -C conversion release

clean:
	$(MAKE) -C math clean
	$(MAKE) -C conversion clean
	rm -rf output

install:
	$(MAKE) -C math install
	$(MAKE) -C conversion install
