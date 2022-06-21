#!/bin/bash

set -eu
shopt -s nullglob

cd -P -- "$(dirname -- "$0")"

if [ -z ${WORD_SIZE+x} ]; then
	echo "Error: WORD_SIZE is not set"
	exit 1
fi

NUM_PASS=0
NUM_FAIL=0

write_error(){
	tput setaf 1
	echo -n "Error: "
	tput sgr0
	echo "$1"
}

write_success(){
	tput setaf 2
	echo "$1"
	tput sgr0
}

write_warning(){
	tput setaf 3
	echo -n "Warning: "
	tput sgr0
	echo "$1"
}

test_pass(){
	local filename="$1"

	tput setaf 2
	echo -en "[\xE2\x9C\x94] PASS"
	tput sgr0
	echo ": $filename"

	((NUM_PASS=NUM_PASS+1))
}

test_fail(){
	local filename="$1"

	tput setaf 1
	echo -en "[\xE2\x9C\x97] FAIL"
	tput sgr0
	echo ": $filename"
	((NUM_FAIL=NUM_FAIL+1))
}

should_fail(){
	local filename="$1"
	local args="$2"

    if ! [ -z ${TEST_DEBUG+x} ]; then
        echo "install/bin/srsvm_run -ws "$WORD_SIZE" "$filename" -- $args >/dev/null 2>&1"
    fi

	if install/bin/srsvm_run -ws "$WORD_SIZE" "$filename" -- $args >/dev/null 2>&1; then
		test_fail "$filename"
	else
		test_pass "$filename"
	fi
}

should_succeed(){
	local filename="$1"
	local args="$2"
    
    if ! [ -z ${TEST_DEBUG+x} ]; then
        echo "install/bin/srsvm_run -ws "$WORD_SIZE" "$filename" -- $args >/dev/null 2>&1"
    fi

	if install/bin/srsvm_run -ws "$WORD_SIZE" "$filename" -- $args >/dev/null 2>&1; then
		test_pass "$filename"
	else
		test_fail "$filename"
	fi
}

show_summary(){
	echo "Tests complete: $NUM_PASS tests passed, $NUM_FAIL tests failed"
}

for dir in all "$WORD_SIZE"; do
	for input_file in cases/$dir/should_fail/*.s; do
		should_fail "$input_file" ""
	done

	for input_file in cases/$dir/should_succeed/*.s; do
		should_succeed "$input_file" ""
	done
done

should_succeed cases/args/should_succeed/00_no_args.s ""
should_succeed cases/args/should_succeed/01_one_arg.s "a"
should_succeed cases/args/should_succeed/02_two_args.s "a b"

show_summary
