/**
 * @file proto_parser.c
 * @brief 协议解析器实现 - 解包状态机
 */
#include "proto_parser.h"
#include "../core/crc.h"
#include <string.h>

void proto_parser_init(proto_parser_t *parser) {
    if (!parser)
        return;
    
    memset(parser, 0, sizeof(proto_parser_t));
    parser->state = PARSER_STATE_WAIT_HEAD1;
}

void proto_parser_reset(proto_parser_t *parser) {
    proto_parser_init(parser);
}

static uint16_t read_uint16_be(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static void write_uint16_be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

int proto_parser_feed(proto_parser_t *parser, uint8_t byte) {
    if (!parser)
        return -2;
    
    parser->error = 0;
    
    switch (parser->state) {
        /* ========== 等待帧头 0xAA ========== */
        case PARSER_STATE_WAIT_HEAD1:
            if (byte == 0xAA) {
                parser->state = PARSER_STATE_WAIT_HEAD2;
            }
            break;
        
        /* ========== 等待帧头 0xBB ========== */
        case PARSER_STATE_WAIT_HEAD2:
            if (byte == 0xBB) {
                parser->state = PARSER_STATE_WAIT_LEN1;
                parser->crc_calc = 0xFFFF;  /* CRC16初始值 */
            } else if (byte == 0xAA) {
                /* 重新同步帧头 */
                parser->state = PARSER_STATE_WAIT_HEAD2;
            } else {
                parser->state = PARSER_STATE_WAIT_HEAD1;
            }
            break;
        
        /* ========== 等待长度高字节 ========== */
        case PARSER_STATE_WAIT_LEN1:
            parser->expected_len = ((uint16_t)byte << 8);
            /* 更新CRC（不含长度bytes本身，在后面统一处理） */
            parser->crc_calc = crc16_modbus_update(parser->crc_calc, &byte, 1);
            parser->state = PARSER_STATE_WAIT_LEN2;
            break;
        
        /* ========== 等待长度低字节 ========== */
        case PARSER_STATE_WAIT_LEN2:
            parser->expected_len |= byte;
            parser->crc_calc = crc16_modbus_update(parser->crc_calc, &byte, 1);
            
            /* 检查长度合理性：最小帧头+尾+校验+地址+命令 = 8字节 */
            /* 但这里 expected_len 是从地址开始到校验前的长度 */
            if (parser->expected_len < PROTO_ADDR_SIZE + PROTO_CMD_SIZE + PROTO_CRC_SIZE) {
                parser->error = PROTO_ERR_LEN;
                parser->state = PARSER_STATE_ERROR;
                return -2;
            }
            
            /* 检查是否超出最大帧长 */
            if (parser->expected_len > PROTO_MAX_FRAME_SIZE) {
                parser->error = PROTO_ERR_LEN;
                parser->state = PARSER_STATE_ERROR;
                return -2;
            }
            
            /* 初始化帧结构 */
            parser->frame.data_len = parser->expected_len - PROTO_ADDR_SIZE - PROTO_CMD_SIZE - PROTO_CRC_SIZE;
            parser->received_len = 0;
            memset(parser->frame.data, 0, sizeof(parser->frame.data));
            
            parser->state = PARSER_STATE_WAIT_ADDR;
            break;
        
        /* ========== 等待设备地址 ========== */
        case PARSER_STATE_WAIT_ADDR:
            parser->frame.addr = byte;
            parser->crc_calc = crc16_modbus_update(parser->crc_calc, &byte, 1);
            parser->state = PARSER_STATE_WAIT_CMD;
            break;
        
        /* ========== 等待命令码 ========== */
        case PARSER_STATE_WAIT_CMD:
            parser->frame.cmd = byte;
            parser->crc_calc = crc16_modbus_update(parser->crc_calc, &byte, 1);
            
            /* 根据数据长度决定下一个状态 */
            if (parser->frame.data_len > 0) {
                parser->state = PARSER_STATE_WAIT_DATA;
            } else {
                parser->state = PARSER_STATE_WAIT_CRC1;
            }
            break;
        
        /* ========== 等待数据域 ========== */
        case PARSER_STATE_WAIT_DATA:
            if (parser->received_len < parser->frame.data_len) {
                parser->frame.data[parser->received_len++] = byte;
                parser->crc_calc = crc16_modbus_update(parser->crc_calc, &byte, 1);
            }
            
            if (parser->received_len >= parser->frame.data_len) {
                parser->state = PARSER_STATE_WAIT_CRC1;
            }
            break;
        
        /* ========== 等待CRC高字节 ========== */
        case PARSER_STATE_WAIT_CRC1:
            parser->frame.crc = ((uint16_t)byte << 8);
            /* CRC不计入校验 */
            parser->state = PARSER_STATE_WAIT_CRC2;
            break;
        
        /* ========== 等待CRC低字节 ========== */
        case PARSER_STATE_WAIT_CRC2:
            parser->frame.crc |= byte;
            
            /* 校验CRC */
            /* 注意：CRC计算范围是从长度到数据，不包含CRC本身 */
            /* 这里简化处理，实际CRC计算需要在解析长度时就开始累积 */
            /* 由于我们已在流程中逐步更新CRC，这里直接比较 */
            if (parser->frame.crc != parser->crc_calc) {
                parser->error = PROTO_ERR_CRC;
                parser->state = PARSER_STATE_ERROR;
                return -2;
            }
            
            parser->state = PARSER_STATE_WAIT_TAIL1;
            break;
        
        /* ========== 等待帧尾 0xCC ========== */
        case PARSER_STATE_WAIT_TAIL1:
            if (byte == 0xCC) {
                parser->state = PARSER_STATE_WAIT_TAIL2;
            } else {
                parser->error = PROTO_ERR_FRAME;
                parser->state = PARSER_STATE_ERROR;
                return -2;
            }
            break;
        
        /* ========== 等待帧尾 0xDD ========== */
        case PARSER_STATE_WAIT_TAIL2:
            if (byte == 0xDD) {
                parser->state = PARSER_STATE_COMPLETE;
                return 0;  /* 成功解析到完整帧 */
            } else {
                parser->error = PROTO_ERR_FRAME;
                parser->state = PARSER_STATE_ERROR;
                return -2;
            }
        
        /* ========== 完成或错误状态 ========== */
        case PARSER_STATE_COMPLETE:
            /* 帧已解析完成，等待被取出后重置 */
            return 0;
        
        case PARSER_STATE_ERROR:
            /* 错误状态，重新等待新帧头 */
            if (byte == 0xAA) {
                parser->state = PARSER_STATE_WAIT_HEAD2;
                parser->error = 0;
            }
            return -2;
    }
    
    return -1;  /* 解析中 */
}

int proto_parser_parse_rb(proto_parser_t *parser, ringbuffer_t *rb, proto_frame_t *frame) {
    if (!parser || !rb || !frame)
        return -1;
    
    uint8_t byte;
    uint16_t avail = ringbuffer_available(rb);
    
    for (uint16_t i = 0; i < avail; i++) {
        ringbuffer_read_pop(rb, &byte, 1);
        int ret = proto_parser_feed(parser, byte);
        
        if (ret == 0) {
            /* 成功解析到完整帧 */
            memcpy(frame, &parser->frame, sizeof(proto_frame_t));
            proto_parser_reset(parser);
            return 0;
        } else if (ret == -2) {
            /* 解析错误，跳过当前帧继续寻找下一个帧头 */
            proto_parser_reset(parser);
        }
    }
    
    return -1;  /* 数据不足或解析中 */
}

const char* proto_parser_state_str(parser_state_t state) {
    switch (state) {
        case PARSER_STATE_WAIT_HEAD1: return "WAIT_HEAD1";
        case PARSER_STATE_WAIT_HEAD2: return "WAIT_HEAD2";
        case PARSER_STATE_WAIT_LEN1:   return "WAIT_LEN1";
        case PARSER_STATE_WAIT_LEN2:   return "WAIT_LEN2";
        case PARSER_STATE_WAIT_ADDR:   return "WAIT_ADDR";
        case PARSER_STATE_WAIT_CMD:    return "WAIT_CMD";
        case PARSER_STATE_WAIT_DATA:   return "WAIT_DATA";
        case PARSER_STATE_WAIT_CRC1:   return "WAIT_CRC1";
        case PARSER_STATE_WAIT_CRC2:   return "WAIT_CRC2";
        case PARSER_STATE_WAIT_TAIL1:  return "WAIT_TAIL1";
        case PARSER_STATE_WAIT_TAIL2:  return "WAIT_TAIL2";
        case PARSER_STATE_COMPLETE:    return "COMPLETE";
        case PARSER_STATE_ERROR:       return "ERROR";
        default: return "UNKNOWN";
    }
}