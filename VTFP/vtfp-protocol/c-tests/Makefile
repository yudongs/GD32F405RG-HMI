# Makefile for host-pc C tests
# Run: make test

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Werror -O0 -g -I../c-types
LDFLAGS ?=

TESTS = test_layout test_commands test_safety test_crc test_cross_lang

.PHONY: all test clean test_schema

all: $(TESTS)

test_layout: test_layout.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_commands: test_commands.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_safety: test_safety.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

CRC_SRC = ../c-types/vtfp_crc.c

test_crc: test_crc.c $(CRC_SRC)
	$(CC) $(CFLAGS) test_crc.c $(CRC_SRC) -o $@ $(LDFLAGS)

header.bin: emit_header.py
	python emit_header.py header.bin

test_cross_lang: test_cross_lang.c $(CRC_SRC) header.bin
	$(CC) $(CFLAGS) test_cross_lang.c $(CRC_SRC) -o $@ $(LDFLAGS)

test: $(TESTS)
	@echo "--- running test_schema ---"
	@./test_schema.sh
	@echo "--- running test_layout ---"
	./test_layout
	@echo "--- running test_commands ---"
	./test_commands
	@echo "--- running test_safety ---"
	./test_safety
	@echo "--- running test_crc ---"
	./test_crc
	@echo "--- running test_cross_lang (with header.bin) ---"
	@./test_cross_lang header.bin
	@echo "=== ALL C TESTS PASSED ==="

test_schema:
	@chmod +x ./test_schema.sh
	@./test_schema.sh

clean:
	rm -f $(TESTS) *.o
