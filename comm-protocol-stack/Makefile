# Makefile for Comm Protocol Stack

CC = gcc
CFLAGS = -Wall -Wextra -I./src -I./src/core -I./src/proto -I./src/transport -std=c11 -Os
LDFLAGS =

# 源文件
CORE_SRC = src/core/ringbuffer.c src/core/crc.c
PROTO_SRC = src/proto/proto_parser.c src/proto/proto_packer.c src/proto/session.c src/proto/proto_stack.c
TRANSPORT_SRC = src/transport/transport_uart.c src/transport/transport_socket.c

ALL_SRC = $(CORE_SRC) $(PROTO_SRC) $(TRANSPORT_SRC)

# 示例程序
EXAMPLES = example_uart example_tcp

# 测试程序
TESTS = test_ringbuffer test_crc test_parser test_integration

.PHONY: all clean test examples

all: examples test

# =============== 示例程序 ===============
examples: $(EXAMPLES)

example_uart: example/example_uart.c $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(ALL_SRC) $(LDFLAGS)

example_tcp: example/example_tcp.c $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(ALL_SRC) $(LDFLAGS)

# =============== 单元测试 ===============
test: $(TESTS)
	@echo ""
	@echo "========================================"
	@echo "   Running All Tests"
	@echo "========================================"
	@failed=0; \
	for t in $(TESTS); do \
		echo ""; \
		echo "Running $$t..."; \
		./$$t || failed=1; \
	done; \
	echo ""; \
	echo "========================================"; \
	if [ $$failed -eq 0 ]; then \
		echo "All tests passed!"; \
	else \
		echo "Some tests failed!"; \
	fi; \
	echo "========================================"

test_ringbuffer: test/test_ringbuffer.c $(CORE_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(CORE_SRC) $(LDFLAGS)

test_crc: test/test_crc.c $(CORE_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(CORE_SRC) $(LDFLAGS)

test_parser: test/test_parser.c $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(ALL_SRC) $(LDFLAGS)

test_integration: test/test_integration.c $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(ALL_SRC) $(LDFLAGS)

# =============== 清理 ===============
clean:
	rm -f $(EXAMPLES) $(TESTS)
	rm -f test_ringbuffer test_crc test_parser test_integration
	find . -name "*.o" -delete
	rm -f *.o

# =============== 依赖检查 ===============
dep:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null || echo "gcc not found!"
	@echo "Done."

# =============== 编译信息 ===============
info:
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Source files:"
	@echo "  Core: $(CORE_SRC)"
	@echo "  Proto: $(PROTO_SRC)"
	@echo "  Transport: $(TRANSPORT_SRC)"
	@echo "Examples: $(EXAMPLES)"
	@echo "Tests: $(TESTS)"