/**
 * @file test_ringbuffer.c
 * @brief 环形缓冲区单元测试
 */
#include <stdio.h>
#include <string.h>
#include "../src/core/ringbuffer.h"

static int test_init(void) {
    printf("\n=== Test: RingBuffer Init ===\n");
    
    ringbuffer_t rb;
    uint8_t buf[128];
    
    int ret = ringbuffer_init(&rb, buf, 128);
    if (ret == 0) {
        printf("[PASS] Init successful\n");
        return 0;
    }
    printf("[FAIL] Init failed\n");
    return -1;
}

static int test_write_read(void) {
    printf("\n=== Test: Write and Read ===\n");
    
    ringbuffer_t rb;
    uint8_t buf[128];
    ringbuffer_init(&rb, buf, 128);
    
    uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    uint16_t written = ringbuffer_write(&rb, data, sizeof(data));
    
    if (written == sizeof(data)) {
        printf("[PASS] Wrote %d bytes\n", written);
    } else {
        printf("[FAIL] Expected %d, wrote %d\n", (int)sizeof(data), written);
        return -1;
    }
    
    uint8_t read_buf[128];
    uint16_t read_len = ringbuffer_read(&rb, read_buf, sizeof(data));
    
    if (read_len == sizeof(data) && memcmp(data, read_buf, sizeof(data)) == 0) {
        printf("[PASS] Read matches written data\n");
        return 0;
    }
    printf("[FAIL] Data mismatch\n");
    return -1;
}

static int test_wraparound(void) {
    printf("\n=== Test: Wraparound ===\n");
    
    ringbuffer_t rb;
    uint8_t buf[16];  /* 小缓冲区，便于触发环绕 */
    ringbuffer_init(&rb, buf, 16);
    
    /* 写入8字节 */
    uint8_t data1[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    ringbuffer_write(&rb, data1, 8);
    
    /* 读出4字节 */
    uint8_t tmp[4];
    ringbuffer_read_pop(&rb, tmp, 4);
    
    /* 再写入8字节（触发环绕） */
    uint8_t data2[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x99, 0x00 };
    uint16_t written = ringbuffer_write(&rb, data2, 8);
    
    if (written == 8) {
        printf("[PASS] Wraparound write successful\n");
    } else {
        printf("[FAIL] Wraparound write failed\n");
        return -1;
    }
    
    /* 验证数据 */
    uint8_t read_buf[16];
    uint16_t avail = ringbuffer_available(&rb);
    ringbuffer_read_pop(&rb, read_buf, avail);
    
    /* 应该读到：data2的全部8字节 */
    if (memcmp(data2, read_buf, 8) == 0) {
        printf("[PASS] Wraparound data correct\n");
        return 0;
    }
    printf("[FAIL] Wraparound data mismatch\n");
    return -1;
}

static int test_overflow(void) {
    printf("\n=== Test: Overflow Handling ===\n");
    
    ringbuffer_t rb;
    uint8_t buf[16];
    ringbuffer_init(&rb, buf, 16);
    
    /* 写入20字节（超过缓冲区容量） */
    uint8_t data[20] = { 0 };
    for (int i = 0; i < 20; i++) data[i] = i;
    
    uint16_t written = ringbuffer_write(&rb, data, 20);
    
    if (written == 16) {  /* 只能写入16字节 */
        printf("[PASS] Overflow correctly limited to %d\n", written);
        return 0;
    }
    printf("[FAIL] Expected 16, got %d\n", written);
    return -1;
}

static int test_clear(void) {
    printf("\n=== Test: Clear ===\n");
    
    ringbuffer_t rb;
    uint8_t buf[32];
    ringbuffer_init(&rb, buf, 32);
    
    uint8_t data[] = { 0x01, 0x02, 0x03 };
    ringbuffer_write(&rb, data, 3);
    
    ringbuffer_clear(&rb);
    
    if (ringbuffer_available(&rb) == 0) {
        printf("[PASS] Clear successful\n");
        return 0;
    }
    printf("[FAIL] Clear failed\n");
    return -1;
}

int main(void) {
    printf("========================================\n");
    printf("   RingBuffer Unit Test Suite\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 5;
    
    if (test_init() == 0) passed++;
    if (test_write_read() == 0) passed++;
    if (test_wraparound() == 0) passed++;
    if (test_overflow() == 0) passed++;
    if (test_clear() == 0) passed++;
    
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}