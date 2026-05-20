/**
 * @file transport_socket.c
 * @brief Socket传输层实现
 *
 * 适配TCP客户端/服务端、UDP通信
 * 支持Linux (BSD socket) 和 Windows (Winsock2)
 */
#include "transport_socket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_fd_t;
    #define SOCKET_INVALID INVALID_SOCKET
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    typedef int socket_fd_t;
    #define SOCKET_INVALID (-1)
    #define closesocket close
#endif

/* =============== Socket句柄结构 =============== */
struct socket_handle {
    socket_fd_t fd;             /* 套接字描述符 */
    socket_type_t type;         /* Socket类型 */
    int is_connected;           /* 是否已连接 */
    socket_config_t config;    /* 配置 */
};

/* =============== 平台初始化 =============== */
#if defined(_WIN32) || defined(_WIN64)
static int g_winsock_init = 0;
static int socket_init(void) {
    if (!g_winsock_init) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
            return -1;
        g_winsock_init = 1;
    }
    return 0;
}
#else
static int socket_init(void) { return 0; }
#endif

/* =============== 创建Socket =============== */
socket_handle_t* socket_create(const socket_config_t *config) {
    if (!config)
        return NULL;
    
    if (socket_init() != 0)
        return NULL;
    
    socket_handle_t *h = (socket_handle_t *)malloc(sizeof(socket_handle_t));
    if (!h)
        return NULL;
    
    memset(h, 0, sizeof(socket_handle_t));
    memcpy(&h->config, config, sizeof(socket_config_t));
    h->type = config->type;
    h->is_connected = 0;
    
    int domain = AF_INET;
    int type = (config->type == SOCKET_TYPE_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int protocol = 0;
    
    h->fd = socket(domain, type, protocol);
    if (h->fd < 0) {
        free(h);
        return NULL;
    }
    
    /* 非阻塞模式 */
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    ioctlsocket(h->fd, FIONBIO, &mode);
#else
    int flags = fcntl(h->fd, F_GETFL, 0);
    fcntl(h->fd, F_SETFL, flags | O_NONBLOCK);
#endif
    
    /* UDP 直接返回 */
    if (config->type == SOCKET_TYPE_UDP) {
        h->is_connected = 1;
        return h;
    }
    
    /* TCP 客户端：连接服务器 */
    if (config->type == SOCKET_TYPE_TCP_CLIENT) {
        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(config->remote_port);
        
        if (inet_pton(AF_INET, config->remote_ip, &server.sin_addr) <= 0) {
            closesocket(h->fd);
            free(h);
            return NULL;
        }
        
        if (connect(h->fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
#if defined(_WIN32) || defined(_WIN64)
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                closesocket(h->fd);
                free(h);
                return NULL;
            }
#else
            if (errno != EINPROGRESS) {
                closesocket(h->fd);
                free(h);
                return NULL;
            }
#endif
        }
        h->is_connected = 1;
    }
    
    /* TCP 服务端：绑定监听 */
    if (config->type == SOCKET_TYPE_TCP_SERVER) {
        int opt = 1;
        setsockopt(h->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
        
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(config->local_port);
        
        if (bind(h->fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
            closesocket(h->fd);
            free(h);
            return NULL;
        }
        
        if (listen(h->fd, 5) < 0) {
            closesocket(h->fd);
            free(h);
            return NULL;
        }
        h->is_connected = 1;
    }
    
    return h;
}

/* =============== 销毁Socket =============== */
void socket_destroy(socket_handle_t *handle) {
    if (!handle)
        return;
    
    if (handle->fd >= 0) {
        closesocket(handle->fd);
        handle->fd = SOCKET_INVALID;
    }
    handle->is_connected = 0;
    free(handle);
}

/* =============== 发送数据 =============== */
int socket_send(socket_handle_t *handle, const uint8_t *data, uint16_t len) {
    if (!handle || !data || len == 0 || handle->fd < 0)
        return -1;
    
    if (!handle->is_connected && handle->type != SOCKET_TYPE_UDP)
        return -1;
    
    int ret = send(handle->fd, (const char *)data, len, 0);
    if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return 0;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
#endif
        return -1;
    }
    return ret;
}

/* =============== 接收数据 =============== */
int socket_recv(socket_handle_t *handle, uint8_t *buf, uint16_t buf_size) {
    if (!handle || !buf || buf_size == 0 || handle->fd < 0)
        return -1;
    
    if (!handle->is_connected && handle->type != SOCKET_TYPE_UDP)
        return -1;
    
    int ret = recv(handle->fd, (char *)buf, buf_size, 0);
    if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return 0;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
#endif
        return -1;
    }
    if (ret == 0) {
        handle->is_connected = 0;
        return -1;  /* 连接关闭 */
    }
    return ret;
}

/* =============== 检查连接状态 =============== */
bool socket_is_connected(socket_handle_t *handle) {
    if (!handle)
        return false;
    return handle->is_connected != 0;
}

/* =============== 主动断开 =============== */
void socket_disconnect(socket_handle_t *handle) {
    if (!handle)
        return;
    if (handle->fd >= 0) {
        shutdown(handle->fd, 2);  /* 关闭读写 */
        closesocket(handle->fd);
        handle->fd = SOCKET_INVALID;
    }
    handle->is_connected = 0;
}

/* =============== 接受客户端连接 =============== */
socket_handle_t* socket_accept(socket_handle_t *server) {
    if (!server || server->type != SOCKET_TYPE_TCP_SERVER || server->fd < 0)
        return NULL;
    
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    memset(&client, 0, sizeof(client));
    
    socket_fd_t client_fd = accept(server->fd, (struct sockaddr *)&client, &addrlen);
    if (client_fd < 0) {
#if defined(_WIN32) || defined(_WIN64)
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return NULL;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return NULL;
#endif
        return NULL;
    }
    
    socket_handle_t *h = (socket_handle_t *)malloc(sizeof(socket_handle_t));
    if (!h) {
        closesocket(client_fd);
        return NULL;
    }
    
    memset(h, 0, sizeof(socket_handle_t));
    h->fd = client_fd;
    h->type = SOCKET_TYPE_TCP_CLIENT;
    h->is_connected = 1;
    
    /* 客户端非阻塞 */
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    ioctlsocket(h->fd, FIONBIO, &mode);
#else
    int flags = fcntl(h->fd, F_GETFL, 0);
    fcntl(h->fd, F_SETFL, flags | O_NONBLOCK);
#endif
    
    return h;
}

/* =============== 统一传输接口适配 =============== */
int socket_transport_send(const uint8_t *data, uint16_t len, void *arg) {
    socket_handle_t *h = (socket_handle_t *)arg;
    return socket_send(h, data, len);
}

int socket_transport_recv(uint8_t *buf, uint16_t buf_size, void *arg) {
    socket_handle_t *h = (socket_handle_t *)arg;
    return socket_recv(h, buf, buf_size);
}