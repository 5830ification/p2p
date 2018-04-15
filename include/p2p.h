#ifndef P2P_H
#define P2P_H

#include <map>
#include <memory>
#include <string>
#include <netinet/in.h>
#include <sys/epoll.h>

#define READ_BUFFER_SIZE 1024
#define SOCKET_LISTEN_BACKLOG 16
#define EPOLL_EVENT_BUFFER_SIZE 16

class client;

typedef int fd_t;
typedef std::map<fd_t, std::shared_ptr<client>> clientmap;

fd_t bind_and_listen(unsigned short port);
std::string ipv4_to_str(in_addr_t addr);

client accept_client(fd_t listener);
void prepare_client(client *client, fd_t epoll_fd);

void handle_event(struct epoll_event evt, fd_t epoll, fd_t sockfd, clientmap& clients);

void handle_client_accept(fd_t sockfd, fd_t epollfd, clientmap& clients);
void handle_client_event(struct epoll_event evt, fd_t epollfd, clientmap& clients);

void handle_client_read(fd_t epollfd, fd_t client, clientmap& clients);
void handle_client_write(fd_t epollfd, fd_t client, clientmap& clients);

void epoll_enable_write_listener(fd_t epollfd, fd_t client);
void epoll_disable_write_listener(fd_t epollfd, fd_t client);

#endif
