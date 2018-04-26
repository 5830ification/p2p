#ifndef CLIENT_H
#define CLIENT_H

#include "p2p.h"
#include "protocol.h"

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

	bool close_requested();

	size_t get_to_write(uint8_t **outptr);
	void clear_writebuf(size_t n);

	/*
	 * fd and addr for outside use by the IO driver...
	 * active_mode:
	 * 	true => this client is the initiator of a peer connection
	 *			-> it is this client's task to send the hello packet
	 *  false => this client has to wait for a hello packet 
	 */
	client(fd_t fd, struct sockaddr_in addr, bool active_mode);
	
	~client();

	// No copy
	client(const client&) = delete;
protected:
	std::vector<uint8_t> readbuf;
	std::vector<uint8_t> writebuf;
	bool _closerequest;
	bool _active_mode;

	Packet *packetbuf;

	void clear_readbuf(size_t n);
};

#endif
