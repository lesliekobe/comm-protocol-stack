/**
 * @file test_integration.c
 * @brief 协议栈集成测试
 *
 * 模拟真实场景：发送帧、解析帧、CRC校验、缓冲区操作
 */
#include <stdio.h>
#include <string.h>
#include "../src/core/ringbuffer.h"
#include "../src/proto/proto_parser.h"
#include "../src/proto/proto_packer.h"
#include "../src/proto/session.h"

/* =============== 测试1：发送帧格式 =============== */
static int test_send_frame(void) {
    printf("\n=== Test: Send Frame Format ===\n");
    
    uint8_t addr = 0x02;
    uint8_t cmd = PROTO_CMD_READ;
    uint8_t reg_addr[] = { 0x00, 0x10 };
    
    uint8_t frame[64];
    uint16_t frame_len = proto_pack(addr, cmd, reg_addr, sizeof(reg_addr), frame, sizeof(frame));
    
    printf("Frame built: %d bytes\n", frame_len);
    
    /* 验证帧头 */
    if (frame[0] != 0xAA || frame[1] != 0xBB) {
        printf("[FAIL] Header: got %02X %02X, want AA BB\n", frame[0], frame[1]);
        return -1;
    }
    printf("[PASS] Header: AA BB\n");
    
    /* 验证帧尾 */
    if (frame[frame_len-2] != 0xCC || frame[frame_len-1] != 0xDD) {
        printf("[FAIL] Tail: got %02X %02X, want CC DD\n", frame[frame_len-2], frame[frame_len-1]);
        return -1;
    }
    printf("[PASS] Tail: CC DD\n");
    
    /* 验证长度字段 */
    uint16_t payload_len = (frame[2] << 8) | frame[3];
    /* payload = addr + cmd + data + crc = 1 + 1 + 2 + 2 = 6 */
    printf("Payload length field: %d (expect 6)\n", payload_len);
    if (payload_len != 6) {
        printf("[FAIL] Payload length wrong\n");
        return -1;
    }
    printf("[PASS] Payload length correct\n");
    
    /* 打印完整帧 */
    printf("Frame hex: ");
    for (uint16_t i = 0; i < frame_len; i++)
        printf("%02X ", frame[i]);
    printf("\n");
    
    return 0;
}

/* =============== 测试2：CRC计算一致性 =============== */
static int test_crc_consistency(void) {
    printf("\n=== Test: CRC Consistency (packer vs parser) ===\n");
    
    /* 构建帧 */
    uint8_t addr = 0x03;
    uint8_t cmd = PROTO_CMD_WRITE;
    uint8_t data[] = { 0x00, 0x01, 0x02, 0x03 };
    
    uint8_t frame[64];
    uint16_t frame_len = proto_pack(addr, cmd, data, sizeof(data), frame, sizeof(frame));
    
    /* 从帧中提取CRC（倒数第3和第2字节） */
    uint16_t packed_crc = (frame[frame_len-3] << 8) | frame[frame_len-2];
    printf("Packed CRC: 0x%04X\n", packed_crc);
    
    /* 用parser逐步计算CRC */
    proto_parser_t parser;
    proto_parser_init(&parser);
    
    /* 手动喂入每个字节并打印CRC */
    printf("Parser CRC trace:\n");
    for (uint16_t i = 0; i < frame_len - 2; i++) { /* 不含CRC和尾部 */
        int st = proto_parser_feed(&parser, frame[i]);
        printf("  byte[%02d]=0x%02X -> crc=0x%04X state=%d\n", 
               i, frame[i], parser.crc_calc, st);
    }
    
    printf("Final parser CRC: 0x%04X\n", parser.crc_calc);
    
    if (parser.crc_calc == packed_crc) {
        printf("[PASS] CRC matches!\n");
        return 0;
    }
    
    printf("[FAIL] CRC mismatch: packer=0x%04X parser=0x%04X\n", packed_crc, parser.crc_calc);
    return -1;
}

/* =============== 测试3：解析完整帧 =============== */
static int test_parse_complete_frame(void) {
    printf("\n=== Test: Parse Complete Frame ===\n");
    
    /* 构建测试帧 */
    uint8_t addr = 0x03;
    uint8_t cmd = PROTO_CMD_WRITE;
    uint8_t data[] = { 0x00, 0x01, 0x02, 0x03 };
    
    uint8_t frame[64];
    uint16_t frame_len = proto_pack(addr, cmd, data, sizeof(data), frame, sizeof(frame));
    
    printf("Built frame: %d bytes\n", frame_len);
    printf("Frame: ");
    for (uint16_t i = 0; i < frame_len; i++) printf("%02X ", frame[i]);
    printf("\n");
    
    /* 解析 */
    uint8_t rb_buf[256];
    ringbuffer_t rb;
    ringbuffer_init(&rb, rb_buf, sizeof(rb_buf));
    ringbuffer_write(&rb, frame, frame_len);
    
    proto_parser_t parser;
    proto_parser_init(&parser);
    proto_frame_t parsed;
    
    int ret = proto_parser_parse_rb(&parser, &rb, &parsed);
    
    if (ret != 0) {
        printf("[FAIL] Parse returned %d, expected 0 (success)\n", ret);
        printf("Final state: %d, crc_calc=0x%04X\n", parser.state, parser.crc_calc);
        return -1;
    }
    
    if (parsed.addr != addr) {
        printf("[FAIL] Addr: got 0x%02X, want 0x%02X\n", parsed.addr, addr);
        return -1;
    }
    if (parsed.cmd != cmd) {
        printf("[FAIL] Cmd: got 0x%02X, want 0x%02X\n", parsed.cmd, cmd);
        return -1;
    }
    if (parsed.data_len != sizeof(data)) {
        printf("[FAIL] DataLen: got %d, want %zu\n", parsed.data_len, sizeof(data));
        return -1;
    }
    if (memcmp(parsed.data, data, sizeof(data)) != 0) {
        printf("[FAIL] Data mismatch\n");
        return -1;
    }
    
    printf("[PASS] addr=0x%02X cmd=0x%02X len=%u\n", parsed.addr, parsed.cmd, parsed.data_len);
    return 0;
}

/* =============== 测试4：CRC错误检测 =============== */
static int test_crc_error_detection(void) {
    printf("\n=== Test: CRC Error Detection ===\n");
    
    /* 构建正确帧 */
    uint8_t frame[64];
    proto_pack(0x05, PROTO_CMD_READ, NULL, 0, frame, sizeof(frame));
    
    /* 篡改数据（不是CRC字节） */
    frame[5] ^= 0xFF;  /* 篡改命令码字节 */
    
    /* 逐字节喂入parser */
    proto_parser_t parser;
    proto_parser_init(&parser);
    
    for (int i = 0; i < 15; i++) {
        int ret = proto_parser_feed(&parser, frame[i]);
        if (ret == -2) {
            printf("[PASS] Error detected at byte %d (state=%d)\n", i, parser.state);
            return 0;
        }
    }
    
    printf("[FAIL] Error not detected\n");
    return -1;
}

/* =============== 测试5：环形缓冲区 =============== */
static int test_ringbuffer_operations(void) {
    printf("\n=== Test: RingBuffer Operations ===\n");
    
    uint8_t buf[32];  /* 小缓冲区 */
    ringbuffer_t rb;
    ringbuffer_init(&rb, buf, 32);
    
    printf("Capacity: %d (expect 32)\n", rb.capacity);
    
    /* 写10字节 */
    uint8_t wdata[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A};
    uint16_t w1 = ringbuffer_write(&rb, wdata, 10);
    printf("Write 10: got %d (expect 10)\n", w1);
    
    /* 再写10字节 */
    uint16_t w2 = ringbuffer_write(&rb, wdata, 10);
    printf("Write 10 more: got %d (expect 10)\n", w2);
    
    /* 再写20字节（触发溢出） */
    uint16_t w3 = ringbuffer_write(&rb, wdata, 20);
    printf("Write 20 (overflow): got %d (expect 12, 32-20 used)\n", w3);
    
    uint16_t avail = ringbuffer_available(&rb);
    printf("Available: %d (expect 32)\n", avail);
    
    /* 读出 */
    uint8_t rbuf[32];
    uint16_t r1 = ringbuffer_read_pop(&rb, rbuf, 15);
    printf("Read 15: got %d (expect 15)\n", r1);
    
    avail = ringbuffer_available(&rb);
    printf("Available after read 15: %d (expect 17)\n", avail);
    
    /* 验证数据 */
    int data_ok = 1;
    for (int i = 0; i < 15; i++) {
        if (rbuf[i] != wdata[i]) {
            data_ok = 0;
            printf("Data mismatch at [%d]: got 0x%02X, want 0x%02X\n",
                   i, rbuf[i], wdata[i]);
        }
    }
    
    if (w1 == 10 && w2 == 10 && w3 == 12 && data_ok) {
        printf("[PASS] RingBuffer correct\n");
        return 0;
    }
    
    printf("[FAIL] RingBuffer operations\n");
    return -1;
}

/* =============== 测试6：会话状态机 =============== */
static int test_session_state_machine(void) {
    printf("\n=== Test: Session State Machine ===\n");
    
    session_t session;
    session_init(&session, 0x02, 1000, 3, 5000);
    
    if (session.state != SESSION_STATE_IDLE) {
        printf("[FAIL] Initial state: got %d, want IDLE(0)\n", session.state);
        return -1;
    }
    printf("[PASS] Initial: IDLE\n");
    
    session_on_send(&session, 1000);
    if (session.state != SESSION_STATE_BUSY || !session.waiting_ack) {
        printf("[FAIL] After send: state=%d waiting=%d\n", session.state, session.waiting_ack);
        return -1;
    }
    printf("[PASS] After send: BUSY, waiting ACK\n");
    
    session_on_recv(&session, 2000);
    if (session.state != SESSION_STATE_CONNECTED || session.waiting_ack) {
        printf("[FAIL] After recv: state=%d waiting=%d\n", session.state, session.waiting_ack);
        return -1;
    }
    printf("[PASS] After recv: CONNECTED, ACK cleared\n");
    
    return 0;
}

/* =============== 主函数 =============== */
int main(void) {
    printf("========================================\n");
    printf("   Protocol Stack Integration Test\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 6;
    
    if (test_send_frame() == 0) passed++;
    if (test_crc_consistency() == 0) passed++;
    if (test_parse_complete_frame() == 0) passed++;
    if (test_crc_error_detection() == 0) passed++;
    if (test_ringbuffer_operations() == 0) passed++;
    if (test_session_state_machine() == 0) passed++;
    
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}