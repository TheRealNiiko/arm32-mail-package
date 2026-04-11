#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <netinet/tcp.h>

int network_create_server_socket(int port, int backlog, int use_ipv6);
int network_set_socket_options(int sock);
int network_set_nonblocking(int sock);
int network_set_read_timeout(int sock, int timeout_sec);
int network_set_write_timeout(int sock, int timeout_sec);

int network_send_data(int sock, const char *data, size_t size);
int network_receive_data(int sock, char *buffer, size_t buffer_size, int timeout);

int network_create_epoll(void);
int network_add_to_epoll(int epoll_fd, int sock, uint32_t events);
int network_remove_from_epoll(int epoll_fd, int sock);
int network_wait_events(int epoll_fd, struct epoll_event *events, int max_events, int timeout);

int network_setup_keepalive(int sock);
int network_check_connection_alive(int sock);

#endif
