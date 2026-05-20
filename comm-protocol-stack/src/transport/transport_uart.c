/**
 * @file transport_uart.c
 * @brief UART传输层实现
 *
 * 适配串口RS232/RS485通信
 * 实现：Linux (termios) / Windows (Win32 API)
 */
#include "transport_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define SERIAL_HANDLE HANDLE
    #define SERIAL_INVALID INVALID_HANDLE_VALUE
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <errno.h>
    #define SERIAL_HANDLE int
    #define SERIAL_INVALID (-1)
#endif

/* =============== UART句柄结构 =============== */
struct uart_handle {
    SERIAL_HANDLE fd;           /* 串口文件描述符/句柄 */
    char port_name[32];        /* 串口名 */
    uart_config_t config;      /* 串口配置 */
    int is_open;               /* 是否打开 */
};

/* =============== 平台实现 =============== */

#if defined(_WIN32) || defined(_WIN64)

static int uart_open_platform(uart_handle_t *handle) {
    char port_path[64];
    if (strncmp(handle->port_name, "COM", 3) == 0) {
        snprintf(port_path, sizeof(port_path), "\\\\.\\%s", handle->port_name);
    } else {
        strncpy(port_path, handle->port_name, sizeof(port_path) - 1);
    }
    
    handle->fd = CreateFileA(port_path, GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (handle->fd == SERIAL_INVALID) {
        return -1;
    }
    
    /* 配置波特率 */
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle->fd, &dcb)) {
        CloseHandle(handle->fd);
        return -1;
    }
    
    dcb.BaudRate = handle->config.baudrate;
    dcb.ByteSize = (handle->config.databits > 8) ? 8 : handle->config.databits;
    dcb.StopBits = (handle->config.stopbits == 1) ? ONESTOPBIT : TWOSTOPBITS;
    dcb.Parity = (handle->config.parity == 1) ? ODDPARITY : 
                 (handle->config.parity == 2) ? EVENPARITY : NOPARITY;
    
    if (!SetCommState(handle->fd, &dcb)) {
        CloseHandle(handle->fd);
        return -1;
    }
    
    /* 设置超时 */
    COMMTIMEOUTS timeout = {0};
    timeout.ReadIntervalTimeout = handle->config.rx_timeout_ms;
    timeout.ReadTotalTimeoutConstant = handle->config.rx_timeout_ms;
    SetCommTimeouts(handle->fd, &timeout);
    
    return 0;
}

static void uart_close_platform(uart_handle_t *handle) {
    if (handle->fd != SERIAL_INVALID) {
        CloseHandle(handle->fd);
        handle->fd = SERIAL_INVALID;
    }
}

static int uart_send_platform(uart_handle_t *handle, const uint8_t *data, uint16_t len) {
    if (!handle || !data || handle->fd == SERIAL_INVALID)
        return -1;
    
    DWORD written = 0;
    if (!WriteFile(handle->fd, data, len, &written, NULL)) {
        return -1;
    }
    return (int)written;
}

static int uart_recv_platform(uart_handle_t *handle, uint8_t *buf, uint16_t buf_size) {
    if (!handle || !buf || handle->fd == SERIAL_INVALID)
        return -1;
    
    DWORD read = 0;
    if (!ReadFile(handle->fd, buf, buf_size, &read, NULL)) {
        return -1;
    }
    return (int)read;
}

#else  /* Linux */

static int uart_open_platform(uart_handle_t *handle) {
    handle->fd = open(handle->port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (handle->fd < 0) {
        return -1;
    }
    
    /* 配置为原始模式 */
    struct termios tty = {0};
    if (tcgetattr(handle->fd, &tty) != 0) {
        close(handle->fd);
        return -1;
    }
    
    /* 设置波特率 */
    speed_t speed;
    switch (handle->config.baudrate) {
        case 9600:   speed = B9600;   break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 921600: speed = B921600; break;
        default:     speed = B115200; break;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    /* 8N1 模式 */
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~PARENB;  /* 无校验 */
    tty.c_cflag &= ~CSTOPB;  /* 1停止位 */
    #ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
    #elif defined(CNEW_RTSCTS)
    tty.c_cflag &= ~CNEW_RTSCTS;
    #else
    tty.c_cflag &= ~0;  /* 无硬件流控 */
    #endif
    
    /* 原始输入模式 */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    /* 原始输出模式 */
    tty.c_oflag &= ~OPOST;
    
    /* 关闭软件流控 */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    /* 设置超时 */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = handle->config.rx_timeout_ms / 100;
    
    tcflush(handle->fd, TCIFLUSH);
    if (tcsetattr(handle->fd, TCSANOW, &tty) != 0) {
        close(handle->fd);
        return -1;
    }
    
    return 0;
}

static void uart_close_platform(uart_handle_t *handle) {
    if (handle->fd >= 0) {
        close(handle->fd);
        handle->fd = SERIAL_INVALID;
    }
}

static int uart_send_platform(uart_handle_t *handle, const uint8_t *data, uint16_t len) {
    if (!handle || !data || handle->fd < 0)
        return -1;
    
    int written = write(handle->fd, data, len);
    if (written < 0) {
        return -1;
    }
    return written;
}

static int uart_recv_platform(uart_handle_t *handle, uint8_t *buf, uint16_t buf_size) {
    if (!handle || !buf || handle->fd < 0)
        return -1;
    
    /* 非阻塞读取 */
    int n = read(handle->fd, buf, buf_size);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  /* 无数据 */
        }
        return -1;
    }
    return n;
}

#endif

/* =============== 公开接口 =============== */

uart_handle_t* uart_open(const char *port, const uart_config_t *config) {
    if (!port || !config)
        return NULL;
    
    uart_handle_t *handle = (uart_handle_t *)malloc(sizeof(uart_handle_t));
    if (!handle)
        return NULL;
    
    memset(handle, 0, sizeof(uart_handle_t));
    strncpy(handle->port_name, port, sizeof(handle->port_name) - 1);
    memcpy(&handle->config, config, sizeof(uart_config_t));
    
#if defined(_WIN32) || defined(_WIN64)
    handle->fd = SERIAL_INVALID;
#else
    handle->fd = SERIAL_INVALID;
#endif
    
    if (uart_open_platform(handle) != 0) {
        free(handle);
        return NULL;
    }
    
    handle->is_open = 1;
    return handle;
}

void uart_close(uart_handle_t *handle) {
    if (!handle)
        return;
    
    if (handle->is_open) {
        uart_close_platform(handle);
        handle->is_open = 0;
    }
    free(handle);
}

int uart_send(uart_handle_t *handle, const uint8_t *data, uint16_t len) {
    return uart_send_platform(handle, data, len);
}

int uart_recv(uart_handle_t *handle, uint8_t *buf, uint16_t buf_size) {
    return uart_recv_platform(handle, buf, buf_size);
}

bool uart_is_open(uart_handle_t *handle) {
    return (handle && handle->is_open);
}

/* =============== 统一传输接口适配 =============== */
int uart_transport_send(const uint8_t *data, uint16_t len, void *arg) {
    uart_handle_t *h = (uart_handle_t *)arg;
    return uart_send(h, data, len);
}

int uart_transport_recv(uint8_t *buf, uint16_t buf_size, void *arg) {
    uart_handle_t *h = (uart_handle_t *)arg;
    return uart_recv(h, buf, buf_size);
}