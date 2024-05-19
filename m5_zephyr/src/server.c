/******************************** INCLUDE FILES *******************************/
#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_sd.h>

#include "server.h"

//LVGL
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <lvgl_input_device.h>
#include <stdlib.h>

/******************************** LOCAL DEFINES *******************************/
LOG_MODULE_REGISTER(server, LOG_LEVEL_DBG);
#define SERVER_IO_BUFFER_SIZE 2000
#define SERVER_MAX_CLIENT_QUEUE CONFIG_NET_SAMPLE_NUM_HANDLERS

/********************************* TYPEDEFS ***********************************/
typedef int (*server_bind_t)(server_t *);
typedef int (*server_process_t)(server_t *);

typedef struct _server_iface_t {
    server_bind_t bind;
    server_process_t process;
} server_iface_t;
typedef server_iface_t *(*server_iface_create_t)(void);

struct _server_t
{
    server_proto_t type;
    int port;
    bool register_service;

    int sock;
    server_iface_t *iface;
};

/********************************* LOCAL DATA *********************************/


/******************************* LOCAL FUNCTIONS ******************************/
static int server_udp_bind(server_t *self)
{
    int ret = 0;
    static struct sockaddr_in s_addr;
    socklen_t s_addr_len = 0;

    if (self->type != SERVER_PROTO_UDP)
    {
        LOG_ERR("Invalid server context!");
        return -1;
    }

    (void)memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(self->port);

    if (self->register_service) {
        DNS_SD_REGISTER_SERVICE(esp32, CONFIG_NET_HOSTNAME,
                    "_esp32", "_udp", "local", DNS_SD_EMPTY_TXT,
                    &s_addr.sin_port);
    }

    self->sock = socket(s_addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
    if (self->sock < 0) {
        LOG_ERR("Failed to create UDP socket: %d, %s", errno, strerror(errno));
        ret = -errno;
    }

    if (0 == ret) {
        s_addr_len = sizeof(s_addr);
        ret = bind(self->sock, (struct sockaddr *)&s_addr, s_addr_len);
        if (ret < 0) {
            LOG_ERR("Failed to bind UDP socket: %d, %s", errno, strerror(errno));
            ret = -errno;
        }
    }

    return ret;
}

static int server_udp_process(server_t *self)
{
    int ret = 0;
    int received = 0;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    char recv_buffer[SERVER_IO_BUFFER_SIZE];

    // Image data
    int chunks_received = 0;
    int total_chunks = 0;
    char image_data[20000] = {0};
    int image_size = 0;

    int header_end = 5;
    char header_buffer[6];
    char done_msg[] = "done";

    if (self->type != SERVER_PROTO_UDP)
    {
        LOG_ERR("Invalid server context!");
        return -1;
    }

    LOG_INF("Listeing on UDP port %d...", self->port);
    for(;;)
    {
        client_addr_len = sizeof(client_addr);
        received = recvfrom(self->sock, recv_buffer,
                    SERVER_IO_BUFFER_SIZE, 0, &client_addr, &client_addr_len);
        if (received < 0) {
            /* Socket error */
            NET_ERR("UDP: Connection error: %d, %s", errno, strerror(errno));
            continue;
        }
        //LOG_DBG("Received: %d bytes", received);
        
        // cut the header from the received buffer
        memcpy(header_buffer, recv_buffer, header_end);
        header_buffer[header_end] = '\0';

        // Check if the header is received
        int chunk_index, num_chunks;
        if (sscanf(header_buffer, "%d/%d", &chunk_index, &num_chunks) != 2) {
            LOG_ERR("Failed to parse the header!");
            continue;
        }
        
        // get the image size and memset the image_data 
        // and get total number of chunks
        if (chunk_index == 0) {
            // get the image size
            if (sscanf(recv_buffer + header_end, "%d", &image_size) != 1) {
                LOG_ERR("Failed to parse the image size!");
                continue;
            }
            total_chunks = num_chunks;
            LOG_DBG("Image size: %d", image_size);
            continue;
        }

        // Copy the received data to the image_data
        
        chunks_received++;

        LOG_DBG("%d/%d, %d %d:%d", chunk_index, num_chunks, received, total_chunks, chunks_received);

        // Check if all chunks are received
        if (chunks_received == total_chunks) {
            LOG_DBG("size: %d\n", strlen(image_data));
            
            // resend 'done' message
            sendto(self->sock, done_msg, sizeof(done_msg) - 1, 0, &client_addr, client_addr_len);
            
            chunks_received = 0;
            total_chunks = 0;
            image_size = 0;
        }


        // ret = sendto(self->sock, recv_buffer, received, 0,
        //          &client_addr, client_addr_len);
        // if (ret < 0) {
        //     NET_ERR("UDP: Failed to send: %d, %s", errno, strerror(errno));
        //     continue;
        // }
    }
}

static server_iface_t *server_udp_create(void)
{
    server_iface_t *udp = (server_iface_t *) k_malloc (sizeof (server_iface_t));
    udp->bind = server_udp_bind;
    udp->process = server_udp_process;

    return udp;
}

static int server_tcp_bind(server_t *self)
{
    int ret = 0;
    socklen_t s_addr_len = 0;
    static struct sockaddr_in s_addr;

    if (self->type != SERVER_PROTO_TCP)
    {
        LOG_ERR("Invalid server context!");
        return -1;
    }

    (void)memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(self->port);

    if (self->register_service) {
        DNS_SD_REGISTER_SERVICE(esp32, CONFIG_NET_HOSTNAME,
                    "_esp32", "_tcp", "local", DNS_SD_EMPTY_TXT,
                    &s_addr.sin_port);
    }

    self->sock = socket(s_addr.sin_family, SOCK_STREAM, 0);
    if (self->sock < 0) {
        LOG_ERR("Failed to create TCP socket: %d, %s", errno, strerror(errno));
        return -errno;
    }

    s_addr_len = sizeof(s_addr);
    ret = bind(self->sock, (struct sockaddr *)&s_addr, s_addr_len);
    if (ret < 0) {
        LOG_ERR("Failed to bind TCP socket: %d, %s", errno, strerror(errno));
        return -errno;
    }

    ret = listen(self->sock, SERVER_MAX_CLIENT_QUEUE);
    if (ret < 0) {
        LOG_ERR("Failed to listen on TCP socket: %d, %s", errno, strerror(errno));
        return -errno;
    }


    return 0;
}

static int server_tcp_conn_handler(server_t *self, int conn)
{
    int ret = 0;
    int received = 0;
    char recv_buffer[SERVER_IO_BUFFER_SIZE];

    char done_msg[] = "done";

    char image_data[MAX_IMAGE_SIZE] = {0};
    uint32_t image_size = 0;
    uint32_t total_received = 0;
    bool first_packet = true;

    for(;;)
    {
        received = recv(conn, recv_buffer, SERVER_IO_BUFFER_SIZE, 0);
        if (received == 0) {
            /* Connection closed */
            LOG_INF("TCP: Connection closed");
            break;
        } else if (received < 0) {
            /* Socket error */
            LOG_ERR("TCP: Connection error %d", errno);
            ret = errno;
            break;
        }

        if (first_packet) {
            if (received >= 4) {
                // Extract the first four bytes as image size
                memcpy(&image_size, recv_buffer, sizeof(image_size));
                image_size = ntohl(image_size); // Convert network byte order to host byte order

                first_packet = false; // Reset flag after the first packet
            }
        } else {
            memcpy(image_data + total_received, recv_buffer, received);
            total_received += received;
        }

        // Check if the last three bytes are 0xAA, 0xBB, 0xCC
        if (received >= 3 && recv_buffer[received - 3] == (char)0xAA && 
                recv_buffer[received - 2] == (char)0xBB && 
                recv_buffer[received - 1] == (char)0xCC) {

            

            // Image data received completely send it to the queue
            image_message_t msg = {
                .data_ptr = image_data,
                .size = image_size
            };

            if (k_msgq_put(&image_msgq, &msg, K_NO_WAIT) != 0) {
                printk("Failed to send message\n");
            } else {
                printk("Image data sent to queue\n");
            }

            // resend 'done' message
            send(conn, done_msg, sizeof(done_msg) - 1, 0);  

            // reset the image data
            // printk("Image size: %u bytes\n", sizeof(image_data));
            printk("Received: %u bytes\n", total_received);
            first_packet = true; // Reset for the next image
            total_received = 0;
        }
    }

    (void)close(conn);
    return ret;
}

static int server_tcp_process(server_t *self)
{
    int conn = -1;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;

    LOG_INF("Waiting for TCP connections on port %d...", self->port);
    client_addr_len = sizeof(client_addr);
    for (;;) {
        conn = accept(self->sock, (struct sockaddr *)&client_addr,
                &client_addr_len);
        if (conn < 0) {
            LOG_ERR("TCP: Accept error: %d, %s", errno, strerror(errno));
            continue;
        }

        server_tcp_conn_handler(self, conn);
    }

    return 0;
}

static server_iface_t *server_tcp_create(void)
{
    server_iface_t *tcp = (server_iface_t *) k_malloc (sizeof (server_iface_t));
    tcp->bind = server_tcp_bind;
    tcp->process = server_tcp_process;

    return tcp;
}

static server_iface_create_t server_iface_create[SERVER_PROTO_N] = {
    server_udp_create,
    server_tcp_create,
};

/***************************** INTERFACE FUNCTIONS ****************************/
server_t *server_new (server_proto_t type, int port, bool register_service)
{
    LOG_INF("Creating new server object");

    server_t *self = (server_t *) k_malloc (sizeof (server_t));
    assert (self);
    self->type = type;
    self->port = port;
    self->register_service = register_service;
    self->iface = server_iface_create[type]();

    return self;
}

void server_destroy (server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        server_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        k_free(self->iface);
        k_free (self);
        *self_p = NULL;
    }
}

void server_start (void *p1, void *p2, void *p3)
{
    int ret = 0;
    server_t *self = (server_t *)p1;

    ret = self->iface->bind(self);
    if (0 != ret)
    {
        LOG_ERR("Bind Failed!");
        return;
    }

    self->iface->process(self);
}
