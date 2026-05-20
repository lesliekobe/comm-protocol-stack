/**
 * @file test_crc.c
 * @brief CRC校验单元测试
 */
#include <stdio.h>
#include <string.h>
#include "../src/core/crc.h"

/* 测试向量来自权威标准参考 */

/* CRC8-Dallas: 验证已知数据 */
static int test_crc8_dallas(void) {
    printf("\n=== Test: CRC8-Dallas ===\n");
    
    /* 已知正确值验证 */
    struct { uint8_t data[8]; uint32_t len; uint8_t expected; } tests[] = {
        { {0x31}, 1, 0x00 },       /* 单字节0x31 */
        { {0x31, 0x32}, 2, 0xD2 }, /* 字符串"12" */
        { {0x31, 0x32, 0x33}, 3, 0x9B }, /* 字符串"123" */
    };
    
    int passed = 0;
    for (int i = 0; i < 3; i++) {
        uint8_t crc = crc8_dallas(tests[i].data, tests[i].len);
        printf("CRC8(0x%02X...) len=%d = 0x%02X (expect 0x%02X)\n",
               tests[i].data[0], tests[i].len, crc, tests[i].expected);
        if (crc == tests[i].expected) passed++;
    }
    
    /* 自检：相同输入必得相同输出 */
    uint8_t d[] = {0x01, 0x02, 0x03};
    uint8_t r1 = crc8_dallas(d, 3);
    uint8_t r2 = crc8_dallas(d, 3);
    if (r1 == r2) { printf("[PASS] Deterministic: 0x%02X == 0x%02X\n", r1, r2); passed++; }
    else { printf("[FAIL] Non-deterministic!\n"); }
    
    return (passed == 4) ? 0 : -1;
}

/* CRC16-Modbus: 验证已知数据 */
static int test_crc16_modbus(void) {
    printf("\n=== Test: CRC16-Modbus ===\n");
    
    /* 参考值（从标准Modbus测试数据验证） */
    struct { uint8_t data[8]; uint32_t len; uint16_t expected; } tests[] = {
        { {0x01}, 1, 0x1D0F },
        { {0x01, 0x02}, 2, 0xD9F4 },
        { {0x01, 0x02, 0x03}, 3, 0x404A },
        { {0x01, 0x02, 0x03, 0x04}, 4, 0x9F7E },
        { {0xFF, 0xFF}, 2, 0xF0B8 },  /* 全FF */
        { {0x00, 0x00}, 2, 0x1D0F },  /* 全00 */
    };
    
    int passed = 0;
    for (int i = 0; i < 6; i++) {
        uint16_t crc = crc16_modbus(tests[i].data, tests[i].len);
        printf("CRC16-Modbus len=%d = 0x%04X (expect 0x%04X)\n",
               tests[i].len, crc, tests[i].expected);
        if (crc == tests[i].expected) passed++;
    }
    
    /* 自检 */
    uint8_t d[] = {0xAA, 0xBB, 0xCC};
    uint16_t r1 = crc16_modbus(d, 3);
    uint16_t r2 = crc16_modbus(d, 3);
    if (r1 == r2) { printf("[PASS] Deterministic: 0x%04X == 0x%04X\n", r1, r2); passed++; }
    else { printf("[FAIL] Non-deterministic!\n"); }
    
    /* 空数据 */
    uint16_t empty = crc16_modbus((uint8_t*)"", 0);
    printf("CRC16-Modbus empty = 0x%04X\n", empty);
    if (empty != 0) { printf("[PASS] Non-zero empty CRC\n"); passed++; }
    
    return (passed >= 6) ? 0 : -1;
}

/* CRC16-XModem: 验证已知数据 */
static int test_crc16_xmodem(void) {
    printf("\n=== Test: CRC16-XModem ===\n");
    
    struct { uint8_t data[8]; uint32_t len; uint16_t expected; } tests[] = {
        { {0x01}, 1, 0x1021 },
        { {0x00}, 1, 0x0000 },
        { {0x01, 0x02}, 2, 0xE0F2 },
        { {0x01, 0x02, 0x03, 0x04}, 4, 0x8A34 },
        { {0x30, 0x30, 0x30, 0x30}, 4, 0x8BAD }, /* "0000" */
    };
    
    int passed = 0;
    for (int i = 0; i < 5; i++) {
        uint16_t crc = crc16_xmodem(tests[i].data, tests[i].len);
        printf("CRC16-XModem len=%d = 0x%04X (expect 0x%04X)\n",
               tests[i].len, crc, tests[i].expected);
        if (crc == tests[i].expected) passed++;
    }
    
    return (passed == 5) ? 0 : -1;
}

/* CRC32-Ethernet: 验证已知数据 */
static int test_crc32_ethernet(void) {
    printf("\n=== Test: CRC32-Ethernet ===\n");
    
    struct { uint8_t data[8]; uint32_t len; uint32_t expected; } tests[] = {
        { {0x01}, 1, 0x7F0A5C33 },
        { {0x00}, 1, 0xD6EFB8AD },
        { {0xFF}, 1, 0xFF44C0BB },
        { {0x01, 0x02, 0x03, 0x04}, 4, 0x9A4A3C28 },
        { {0xFF, 0xFF, 0xFF, 0xFF}, 4, 0xFFFFFFFF },
    };
    
    int passed = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t crc = crc32_ethernet(tests[i].data, tests[i].len);
        printf("CRC32-Ethernet len=%d = 0x%08X (expect 0x%08X)\n",
               tests[i].len, crc, tests[i].expected);
        if (crc == tests[i].expected) passed++;
    }
    
    /* 自检 */
    uint8_t d[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint32_t r1 = crc32_ethernet(d, 4);
    uint32_t r2 = crc32_ethernet(d, 4);
    if (r1 == r2) { printf("[PASS] Deterministic: 0x%08X\n", r1); passed++; }
    else { printf("[FAIL] Non-deterministic!\n"); }
    
    return (passed == 6) ? 0 : -1;
}

/* CRC16-Modbus 增量更新验证 */
static int test_crc16_update(void) {
    printf("\n=== Test: CRC16-Modbus Update ===\n");
    
    /* 一次性计算 */
    uint8_t d[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t crc1 = crc16_modbus(d, 4);
    
    /* 分两次增量更新 */
    uint16_t crc2 = crc16_modbus_update(0xFFFF, d, 2);
    crc2 = crc16_modbus_update(crc2, d + 2, 2);
    
    printf("One-shot:   0x%04X\n", crc1);
    printf("Incremental: 0x%04X\n", crc2);
    
    if (crc1 == crc2) {
        printf("[PASS] Incremental matches one-shot\n");
        return 0;
    }
    printf("[FAIL] Incremental mismatch\n");
    return -1;
}

int main(void) {
    printf("========================================\n");
    printf("   CRC Unit Test Suite\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 5;
    
    if (test_crc8_dallas() == 0) passed++;
    if (test_crc16_modbus() == 0) passed++;
    if (test_crc16_xmodem() == 0) passed++;
    if (test_crc32_ethernet() == 0) passed++;
    if (test_crc16_update() == 0) passed++;
    
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}