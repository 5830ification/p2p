#include "p2p.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/epoll.h>

#include "client.h"
#include "log.h"

int main(int argc, char const **argv) {
  if (argc == 1) {
	printf("Usage: %s [port]\n", argv[0]);
	return 1;
  } else if (argc == 2) {
	log(LWARN, "No initial peers supplied as command line args. Starting in server-only mode...\n");
  }

  unsigned short port = atoi(argv[1]);

  fd_t sockfd = bind_and_listen(port);
  printf("Server listening on port %u!\n", port);

  int epoll = epoll_create(16); // Guesstimate of client count
  struct epoll_event events[EPOLL_EVENT_BUFFER_SIZE];

  struct epoll_event evt;
  evt.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  evt.data.fd = sockfd;
  epoll_ctl(epoll, EPOLL_CTL_ADD, sockfd, &evt);

  clientmap clients;

  for (int i = 2; i < argc; i++) {
	unsigned short port;
	struct sockaddr_in addr;

	if (!parse_peer_connstr(argv[i], &addr, &port)) {
	  log(LERROR, "Unable to parse connection string '%s'. Expected format: "
	  	"<IP>:<Port>\n", argv[i]);
	}

	log(LINFO, "Connecting to peer with IP: %s...\n",
		ipv4_to_str(addr.sin_addr.s_addr).c_str());
	fd_t clisock = connect_to_peer(addr, port);
	if (clisock < 0) {
		continue;
	}

	client cli(clisock, addr);
	prepare_client(&cli, epoll);
	clients.insert({clisock, std::make_shared<client>(cli)});
  	log(LINFO, "Connected to initial peer with IP %s\n", 
		ipv4_to_str(addr.sin_addr.s_addr).c_str());
  }

  for (;;) {
    int fd_count = epoll_wait(epoll, events, EPOLL_EVENT_BUFFER_SIZE, -1);

    if (fd_count < 0) {
      printf("epoll_wait failed!");
      return 1;
    }

    for (int e = 0; e < fd_count; e++) {
      handle_event(events[e], epoll, sockfd, clients);
    }
  }

  close(sockfd);
  close(epoll);
}

fd_t bind_and_listen(unsigned short port) {
  fd_t socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket() failed!");
    exit(1);
  }

  int _true = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &_true,
                 sizeof(socket_fd)) != 0) {
    perror("setsockopt() failed!");
    exit(1);
  }

  struct sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("bind() failed!");
    exit(1);
  }

  if (listen(socket_fd, SOCKET_LISTEN_BACKLOG) != 0) {
    perror("listen() failed!");
    exit(1);
  }

  return socket_fd;
}

client accept_client(fd_t listener) {
  struct sockaddr_in client_addr = {};
  socklen_t client_addr_len = sizeof(client_addr);
  fd_t client_sock =
      accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
  if (client_sock == -1) {
    perror("accept() failed!");
    exit(1);
  }

  client cli = {client_sock, client_addr};
  return cli;
}

void prepare_client(client *client, fd_t epoll_fd) {
  if (fcntl(client->fd, F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl() failed");
    exit(1);
  }

  struct epoll_event evt;
  evt.events = EPOLLIN;
  evt.data.fd = client->fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->fd, &evt);
}

void disconnect_client(fd_t epoll, fd_t client_fd, clientmap &clients) {
  log(LINFO, "Disconnecting from client with IP: %s\n",
      ipv4_to_str(clients[client_fd]->addr.sin_addr.s_addr).c_str());

  epoll_ctl(epoll, EPOLL_CTL_DEL, client_fd, NULL);
  clients.erase(client_fd);
  close(client_fd);
}

void handle_event(struct epoll_event evt, fd_t epoll, fd_t sockfd,
                  clientmap &clients) {
  fd_t fd = evt.data.fd;
  if (fd == sockfd) {
	handle_client_accept(sockfd, epoll, clients);
  } else if (clients.find(fd) != clients.end()) {
  	handle_client_event(evt, epoll, clients);
  } else {
	log(LERROR, "Received an untracked epoll_event on fd: %d\n", fd);
  }
}

void handle_client_accept(fd_t sockfd, fd_t epollfd, clientmap& clients) {
	client cli = accept_client(sockfd);
	prepare_client(&cli, epollfd);
	clients.insert({cli.fd, std::make_shared<client>(cli)});

	log(LINFO, "Accepted client with IP: %s at fd: %d\n", 
		ipv4_to_str(cli.addr.sin_addr.s_addr).c_str(),
		cli.fd);
}

void handle_client_event(struct epoll_event evt, fd_t epollfd, clientmap& clients) {
	fd_t fd = evt.data.fd;
	if (evt.events & EPOLLIN) {
		handle_client_read(epollfd, fd, clients);
	}

	if (evt.events & EPOLLOUT) {
		handle_client_write(epollfd, fd, clients);
	}
}

void handle_client_read(fd_t epollfd, fd_t client, clientmap& clients) {
	uint8_t buffer[READ_BUFFER_SIZE];
	ssize_t nread = read(client, buffer, READ_BUFFER_SIZE);
	
	if (nread < 0) {
		log(LERROR, "Unknown error while reading: %d\n", nread);
	} else if (nread == 0) {
		disconnect_client(epollfd, client, clients);
	} else {
		clients[client]->add_read(buffer, nread);
		clients[client]->on_read();

		if (clients[client]->waiting_for_write()) {
			epoll_enable_write_listener(epollfd, client);	
		} else if (clients[client]->close_requested()) {
			disconnect_client(epollfd, client, clients);
		}
	}
}

void handle_client_write(fd_t epollfd, fd_t client, clientmap& clients) {
	if (!clients[client]->waiting_for_write()) {
		epoll_disable_write_listener(epollfd, client);
	} else {
		uint8_t *writebuf;
		size_t to_write = clients[client]->get_to_write(&writebuf);
		ssize_t written = write(client, writebuf, to_write);
		
		if (written <= 0) {
			log(LERROR, "Unkown error while writing: %d\n", written);
		} else {
			clients[client]->clear_writebuf(written);
			clients[client]->on_write();

			if (clients[client]->close_requested() && !clients[client]->waiting_for_write()) {
				disconnect_client(epollfd, client, clients);
			}
		}
	}
}

std::string ipv4_to_str(in_addr_t addr) {
  return std::to_string((unsigned int)(addr & 0xFF)) + '.' +
         std::to_string((unsigned int)((addr >> 8) & 0xFF)) + '.' +
         std::to_string((unsigned int)((addr >> 16) & 0xFF)) + '.' +
         std::to_string((unsigned int)((addr >> 24) & 0xFF));
}

void epoll_enable_write_listener(fd_t epollfd, fd_t client) {
	struct epoll_event evt;
	evt.data.fd = client;
	evt.events = EPOLLIN | EPOLLOUT;
	if (epoll_ctl(epollfd, EPOLL_CTL_MOD, client, &evt) != 0) {
		log(LERROR, "epoll_ctl() failed!\n");
	}
}

void epoll_disable_write_listener(fd_t epollfd, fd_t client) {
	struct epoll_event evt;
	evt.data.fd = client;
	evt.events = EPOLLIN;
	if (epoll_ctl(epollfd, EPOLL_CTL_MOD, client, &evt) != 0) {
		log(LERROR, "epoll_ctl() failed!\n");
	}
}

bool parse_peer_connstr(char const *str, struct sockaddr_in *addr, unsigned short *port) {
	size_t len = strlen(str);
	char const *colon = strchr(str, ':');
	if (colon == NULL) {
		return false;
	}

	if (colon + 1 >= str + len) {
		return false;
	}

	unsigned int digits[4];
	if (sscanf(str, "%u.%u.%u.%u", &digits[0], &digits[1], &digits[2], &digits[3]) != 4) {
		return false;
	}

	unsigned int res = 0;
	for (unsigned int i = 0; i < 4; i++) {
		if (digits[i] > 255)
			return false;

		res |= (digits[i] << (8 * i));
	}

	unsigned int p = 0;
	if (sscanf(colon, ":%u", &p) != 1) {
		return false;
	}

	if (p > 0xFFFF) {
		return false;
	}

	addr->sin_addr.s_addr = res;
	*port = p;
	return true;
}

fd_t connect_to_peer(struct sockaddr_in addr, unsigned short port) {
	fd_t sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		log(LERROR, "Client connection failed: Socket creation failed: %d\n", sock);
		return -1;
	}

	struct sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr = addr.sin_addr;
	server_addr.sin_port = htons(port);

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
		log(LERROR, "Client connection failed: connect() failed!\n");
		return -1;
	}

	return sock;
}
