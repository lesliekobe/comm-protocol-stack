/**
 * @file proto_packer.c
 * @brief 协议封包器实现 - 发送端组帧
 */
#include "proto_packer.h"
#include "../core/crc.h"
#include <string.h>

uint16_t proto_pack(uint8_t addr, uint8_t cmd, const uint8_t *data, uint16_t data_len,
                    uint8_t *frame_buf, uint16_t buf_size) {
    /* 边界检查：数据过长 */
    if (data_len > PROTO_MAX_DATA_SIZE)
        return 0;
    
    /* 计算帧总长度 */
    uint16_t payload_len = PROTO_ADDR_SIZE + PROTO_CMD_SIZE + data_len + PROTO_CRC_SIZE;
    uint16_t frame_len = PROTO_HEAD_SIZE + PROTO_LEN_SIZE + payload_len + PROTO_TAIL_SIZE;
    
    /* 检查缓冲区大小 */
    if (buf_size < frame_len)
        return 0;
    
    uint8_t *p = frame_buf;
    
    /* 1. 填充帧头 0xAA 0xBB */
    *p++ = 0xAA;
    *p++ = 0xBB;
    
    /* 2. 填写长度（高字节在前，Big-Endian） */
    *p++ = (uint8_t)(payload_len >> 8);
    *p++ = (uint8_t)(payload_len & 0xFF);
    
    /* 3. 填写地址 */
    *p++ = addr;
    
    /* 4. 填写命令码 */
    *p++ = cmd;
    
    /* 5. 拷贝业务数据 */
    if (data && data_len > 0) {
        memcpy(p, data, data_len);
        p += data_len;
    }
    
    /* 6. 计算并填充校验码（从长度到数据结束） */
    uint16_t crc = crc16_modbus(frame_buf + PROTO_HEAD_SIZE + PROTO_LEN_SIZE, 
                                 payload_len - PROTO_CRC_SIZE);
    *p++ = (uint8_t)(crc >> 8);
    *p++ = (uint8_t)(crc & 0xFF);
    
    /* 7. 填充帧尾 0xCC 0xDD */
    *p++ = 0xCC;
    *p++ = 0xDD;
    
    return frame_len;
}

uint16_t proto_frame_to_str(const proto_frame_t *frame, char *buf, uint16_t buf_size) {
    if (!frame || !buf || buf_size < 64)
        return 0;
    
    char *p = buf;
    int n;
    
    n = snprintf(p, buf_size - (p - buf), 
                 "[ADDR=0x%02X CMD=0x%02X DATA_LEN=%u DATA=",
                 frame->addr, frame->cmd, frame->data_len);
    if (n < 0) return 0;
    p += n;
    
    /* 添加数据域的十六进制表示 */
    for (uint16_t i = 0; i < frame->data_len && (p - buf) < (int)buf_size - 4; i++) {
        n = snprintf(p, buf_size - (p - buf), "%02X ", frame->data[i]);
        if (n < 0) return 0;
        p += n;
    }
    
    n = snprintf(p, buf_size - (p - buf), "CRC=0x%04X]", frame->crc);
    if (n < 0) return 0;
    p += n;
    
    return (uint16_t)(p - buf);
}

uint16_t bytes_to_hex(const uint8_t *data, uint16_t len, char *buf, uint16_t buf_size) {
    if (!data || !buf || buf_size < len * 2)
        return 0;
    
    char *p = buf;
    for (uint16_t i = 0; i < len; i++) {
        snprintf(p, buf_size - (p - buf), "%02X", data[i]);
        p += 2;
    }
    return (uint16_t)(p - buf);
}

uint16_t hex_to_bytes(const char *hex_str, uint8_t *buf, uint16_t buf_size) {
    if (!hex_str || !buf || buf_size == 0)
        return 0;
    
    /* 跳过空格和逗号 */
    while (*hex_str == ' ' || *hex_str == ',' || *hex_str == ':')
        hex_str++;
    
    uint16_t count = 0;
    const char *p = hex_str;
    
    while (*p && count < buf_size) {
        /* 跳过分隔符 */
        if (*p == ' ' || *p == ',' || *p == ':' || *p == '-') {
            p++;
            continue;
        }
        
        /* 读取两个十六进制字符 */
        char high = *p++;
        char low = *p++;
        
        if (!low) break;  /* 不完整的半字节 */
        
        /* 转换高位 */
        uint8_t h, l;
        if (high >= '0' && high <= '9') h = high - '0';
        else if (high >= 'A' && high <= 'F') h = high - 'A' + 10;
        else if (high >= 'a' && high <= 'f') h = high - 'a' + 10;
        else continue;  /* 无效字符 */
        
        /* 转换低位 */
        if (low >= '0' && low <= '9') l = low - '0';
        else if (low >= 'A' && low <= 'F') l = low - 'A' + 10;
        else if (low >= 'a' && low <= 'f') l = low - 'a' + 10;
        else continue;  /* 无效字符 */
        
        buf[count++] = (h << 4) | l;
    }
    
    return count;
}