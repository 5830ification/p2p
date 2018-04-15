#ifndef CLIENT_H
#define CLIENT_H

#include "p2p.h"

#include <cstdint>
#include <cstring>
#include <vector>

class client {
public:
	fd_t fd;
	struct sockaddr_in addr;

	void add_read(uint8_t *ptr, size_t size);
	void add_write(uint8_t *ptr, size_t size);

	void on_read();
	void on_write();

	bool waiting_for_write();
	size_t get_to_write(uint8_t **outptr);
	void clear_writebuf(size_t n);

	client(fd_t fd, struct sockaddr_in addr);

protected:
	std::vector<uint8_t> readbuf;
	std::vector<uint8_t> writebuf;
};

#endif
