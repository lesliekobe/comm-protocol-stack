/**
 * @file crc.h
 * @brief CRC校验算法 - CRC16/CRC32实现
 *
 * 支持多种CRC模式：
 * - CRC16-CCITT (Modbus标准)
 * - CRC16-CCITT (XModem)
 * - CRC32 (MPEG2/POSIX标准)
 */
#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/**
 * CRC16-CCITT (Modbus标准)
 * 多项式: 0x8005
 * 初始值: 0xFFFF
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC16校验码
 */
uint16_t crc16_modbus(const uint8_t *data, uint32_t len);

/**
 * CRC16-CCITT (XModem标准)
 * 多项式: 0x1021
 * 初始值: 0x0000
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC16校验码
 */
uint16_t crc16_xmodem(const uint8_t *data, uint32_t len);

/**
 * CRC16-CCITT (ITU标准)
 * 多项式: 0x1021
 * 初始值: 0xFFFF
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC16校验码
 */
uint16_t crc16_itu(const uint8_t *data, uint32_t len);

/**
 * CRC32 (MPEG2/POSIX标准)
 * 多项式: 0x04C11DB7
 * 初始值: 0xFFFFFFFF
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC32校验码
 */
uint32_t crc32_mpeg2(const uint8_t *data, uint32_t len);

/**
 * CRC32 (标准以太网)
 * 多项式: 0x04C11DB7
 * 初始值: 0xFFFFFFFF
 * 结果异或: 0xFFFFFFFF
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC32校验码
 */
uint32_t crc32_ethernet(const uint8_t *data, uint32_t len);

/**
 * CRC8 (Dallas单总线)
 * 多项式: 0x31
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC8校验码
 */
uint8_t crc8_dallas(const uint8_t *data, uint32_t len);

/**
 * 简易8位累加校验和
 * @param data 数据指针
 * @param len  数据长度
 * @return 校验和
 */
uint8_t checksum_sum8(const uint8_t *data, uint32_t len);

/**
 * 简易16位累加校验和
 * @param data 数据指针
 * @param len  数据长度
 * @return 校验和
 */
uint16_t checksum_sum16(const uint8_t *data, uint32_t len);

/**
 * CRC16-Modbus 增量更新（用于状态机中逐字节计算）
 * @param crc   已有CRC值
 * @param data  数据指针
 * @param len   数据长度
 * @return 更新后的CRC值
 */
uint16_t crc16_modbus_update(uint16_t crc, const uint8_t *data, uint32_t len);

#endif /* CRC_H */