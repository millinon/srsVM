.PHONY: pre-test test clean

pre-test:
	$(MAKE) -C .. release install "PREFIX=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))/install"

test: pre-test
	WORD_SIZE=16 ./test.sh
	WORD_SIZE=32 ./test.sh
	WORD_SIZE=64 ./test.sh
	WORD_SIZE=128 ./test.sh

clean:
	rm -rf install
