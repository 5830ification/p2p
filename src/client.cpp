#include "client.h"

#include "log.h"

client::client(fd_t fd, struct sockaddr_in addr, bool active_mode): 
	fd(fd), addr(addr), _closerequest(false),
	_active_mode(active_mode), packetbuf(new Packet())
{
}

client::~client() {
	delete packetbuf;
}

void client::add_read(uint8_t *ptr, size_t size) {
	std::vector<uint8_t> bytes(ptr, ptr + size);
	readbuf.insert(readbuf.end(), bytes.begin(), bytes.end());
}

void client::add_write(uint8_t *ptr, size_t size) {
	std::vector<uint8_t> bytes(ptr, ptr + size);
	writebuf.insert(writebuf.end(), bytes.begin(), bytes.end());
}

void client::on_read() {
	//printf("Readbuf contains %lu bytes\n", readbuf.size());
	//if (readbuf.size() >= 100) {
	//	const char *msg = "Wow, you're quite the author, aren't you?\n";
	//	add_write((uint8_t *)msg, strlen(msg));
	//	readbuf.clear();
	//	_closerequest = true;
	//}

	size_t processed = packetbuf->add_data(readbuf);
	if (processed < readbuf.size()) {
		log(LDBG, "Parsed a packet of type: %d (size: %u)", 
			packetbuf->type(), packetbuf->body_size());

		delete packetbuf;
		packetbuf = new Packet();
	}

	clear_readbuf(processed);
}

void client::on_write() {

}

bool client::waiting_for_write() {
	return writebuf.size() > 0;
}

bool client::close_requested() {
	return _closerequest;
}

size_t client::get_to_write(uint8_t **outptr) {
	*outptr = writebuf.data();
	return writebuf.size();
}

void client::clear_writebuf(size_t n) {
	writebuf.erase(writebuf.begin(), writebuf.begin() + n);
}

void client::clear_readbuf(size_t n) {
	if (n == 0) return;
	readbuf.erase(readbuf.begin(), readbuf.begin() + n);
}
