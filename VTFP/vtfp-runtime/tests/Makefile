# Host-pc tests for vtfp-runtime
# Run: make test

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Werror -O0 -g \
           -I../include \
           -I../src \
           -I../../vtfp-protocol/c-types \
           -Wno-int-to-pointer-cast
LDFLAGS ?=

TESTS = test_vtfp_checksum test_vtfp_arm test_vtfp_dispatch test_vtfp_core

.PHONY: all test clean

all: $(TESTS)

test_vtfp_checksum: test_vtfp_checksum.c ../src/vtfp_checksum.c ../../vtfp-protocol/c-types/vtfp_crc.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_vtfp_arm: test_vtfp_arm.c ../src/vtfp_arm.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_vtfp_dispatch: test_vtfp_dispatch.c ../src/vtfp_dispatch.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_vtfp_core: test_vtfp_core.c ../src/vtfp_core.c ../src/vtfp_checksum.c ../src/vtfp_arm.c ../src/vtfp_dispatch.c ../../vtfp-protocol/c-types/vtfp_crc.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TESTS)
	@echo "--- running test_vtfp_checksum ---"
	./test_vtfp_checksum
	@echo "--- running test_vtfp_arm ---"
	./test_vtfp_arm
	@echo "--- running test_vtfp_dispatch ---"
	./test_vtfp_dispatch
	@echo "--- running test_vtfp_core ---"
	./test_vtfp_core
	@echo "=== ALL M2 C TESTS PASSED ==="

clean:
	rm -f $(TESTS) *.o
