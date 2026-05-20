/**
 * @file test_ringbuffer.c
 * @brief 环形缓冲区单元测试
 */
#include <stdio.h>
#include <string.h>
#include "../src/core/ringbuffer.h"

static int test_basic(void) {
    printf("\n=== Test: Basic Init ===\n");
    ringbuffer_t rb;
    uint8_t buf[64];
    ringbuffer_init(&rb, buf, 64);
    printf("capacity=%d avail=%d free=%d\n", rb.capacity, ringbuffer_available(&rb), ringbuffer_free(&rb));
    if (rb.capacity == 64 && ringbuffer_available(&rb) == 0) {
        printf("[PASS] Init OK\n");
        return 0;
    }
    printf("[FAIL] Init\n");
    return -1;
}

static int test_write_read(void) {
    printf("\n=== Test: Write/Read ===\n");
    ringbuffer_t rb;
    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));
    ringbuffer_init(&rb, buf, 64);

    uint8_t w[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t n = ringbuffer_write(&rb, w, 5);
    printf("write 5: got %d (expect 5)\n", n);

    uint8_t r[5];
    n = ringbuffer_read(&rb, r, 5);
    printf("read 5: got %d (expect 5)\n", n);

    if (memcmp(w, r, 5) == 0) {
        printf("[PASS] Data matches\n");
        return 0;
    }
    printf("[FAIL] Data mismatch\n");
    return -1;
}

static int test_wraparound(void) {
    printf("\n=== Test: Wraparound ===\n");
    ringbuffer_t rb;
    uint8_t buf[16];
    memset(buf, 0xAA, sizeof(buf)); /* 填充0xAA便于观察 */
    ringbuffer_init(&rb, buf, 16);

    /* 写8字节，read_pos推进4，write_pos在8 */
    uint8_t w1[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    ringbuffer_write(&rb, w1, 8);
    printf("after write8: read_pos=%d write_pos=%d\n", rb.read_pos, rb.write_pos);

    /* 读4字节，read_pos从0推进到4 */
    uint8_t tmp[4];
    ringbuffer_read_pop(&rb, tmp, 4);
    printf("after read4: read_pos=%d write_pos=%d\n", rb.read_pos, rb.write_pos);

    /* 再写8字节，触发环绕，write_pos从8→4 */
    uint8_t w2[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x99, 0x00};
    uint16_t n = ringbuffer_write(&rb, w2, 8);
    printf("write8 after wrap: wrote=%d (expect 8) read_pos=%d write_pos=%d\n",
           n, rb.read_pos, rb.write_pos);

    /* 此时：buf[0..7]=w2, buf[8..15]=w1[4..7], read_pos=4, write_pos=4 */
    /* 读取全部12字节验证 */
    uint8_t r[16];
    n = ringbuffer_read_pop(&rb, r, 12);
    printf("read12: got=%d (expect 12)\n", n);

    /* 验证：前4字节是w1[4..7]，后8字节是w2 */
    int ok = 1;
    for (int i = 0; i < 4; i++) {
        if (r[i] != w1[4+i]) {
            printf("data[%d]: got 0x%02X, want 0x%02X\n", i, r[i], w1[4+i]);
            ok = 0;
        }
    }
    for (int i = 0; i < 8; i++) {
        if (r[4+i] != w2[i]) {
            printf("data[%d]: got 0x%02X, want 0x%02X\n", 4+i, r[4+i], w2[i]);
            ok = 0;
        }
    }

    if (ok) {
        printf("[PASS] Wraparound data correct\n");
        return 0;
    }
    printf("[FAIL] Wraparound data mismatch\n");
    return -1;
}

static int test_overflow(void) {
    printf("\n=== Test: Overflow ===\n");
    ringbuffer_t rb;
    uint8_t buf[16];
    memset(buf, 0xBB, sizeof(buf)); /* 填充0xBB便于观察 */
    ringbuffer_init(&rb, buf, 16);

    /* 写20字节（超过capacity=16） */
    uint8_t w[20];
    for (int i = 0; i < 20; i++) w[i] = i + 1;
    uint16_t n = ringbuffer_write(&rb, w, 20);
    printf("write20: got %d (expect 16)\n", n);

    uint16_t avail = ringbuffer_available(&rb);
    printf("avail=%d (expect 16)\n", avail);

    if (n == 16 && avail == 16) {
        printf("[PASS] Overflow capped correctly\n");
        return 0;
    }
    printf("[FAIL] Overflow handling wrong\n");
    return -1;
}

static int test_pop_after_wrap(void) {
    printf("\n=== Test: Pop After Wrap ===\n");
    ringbuffer_t rb;
    uint8_t buf[16];
    memset(buf, 0, sizeof(buf));
    ringbuffer_init(&rb, buf, 16);

    /* 写满16字节 */
    uint8_t w[16];
    for (int i = 0; i < 16; i++) w[i] = i;
    ringbuffer_write(&rb, w, 16);
    printf("write16: avail=%d (expect 16)\n", ringbuffer_available(&rb));

    /* 读6字节，read_pos=6 */
    uint8_t r[6];
    ringbuffer_read_pop(&rb, r, 6);
    printf("read6: read_pos=%d (expect 6)\n", rb.read_pos);

    /* 再写10字节（环绕后覆盖） */
    uint8_t w2[] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9};
    uint16_t n = ringbuffer_write(&rb, w2, 10);
    printf("write10: wrote=%d read_pos=%d write_pos=%d avail=%d\n",
           n, rb.read_pos, rb.write_pos, ringbuffer_available(&rb));

    /* avail应该=10(read_pos=6, write_pos=6, 6→no wrap) or 10? */
    /* buf[0..9]=w2, read_pos=6, write_pos=6 */
    /* avail = 6-6=0? NO! write_pos=6 >= read_pos=6 → avail=0??? */

    /* 读出全部10字节 */
    uint8_t r2[10];
    n = ringbuffer_read_pop(&rb, r2, 10);
    printf("read10: got=%d (expect 10)\n", n);

    int ok = (n == 10);
    for (int i = 0; i < n; i++) {
        if (r2[i] != w2[i]) {
            printf("data[%d]: got 0x%02X, want 0x%02X\n", i, r2[i], w2[i]);
            ok = 0;
        }
    }

    if (ok) {
        printf("[PASS] Pop after wrap correct\n");
        return 0;
    }
    printf("[FAIL] Pop after wrap wrong\n");
    return -1;
}

int main(void) {
    printf("========================================\n");
    printf("   RingBuffer Unit Test Suite\n");
    printf("========================================\n");

    int passed = 0;
    int total = 5;

    if (test_basic() == 0) passed++;
    if (test_write_read() == 0) passed++;
    if (test_wraparound() == 0) passed++;
    if (test_overflow() == 0) passed++;
    if (test_pop_after_wrap() == 0) passed++;

    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");

    return (passed == total) ? 0 : 1;
}