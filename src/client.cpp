#include "client.h"

client::client(fd_t fd, struct sockaddr_in addr): fd(fd), addr(addr) {}

void client::add_read(uint8_t *ptr, size_t size) {
	std::vector<uint8_t> bytes(ptr, ptr + size);
	readbuf.insert(readbuf.end(), bytes.begin(), bytes.end());
}

void client::add_write(uint8_t *ptr, size_t size) {
	std::vector<uint8_t> bytes(ptr, ptr + size);
	writebuf.insert(writebuf.end(), bytes.begin(), bytes.end());
}

void client::on_read() {
	printf("Readbuf contains %lu bytes\n", readbuf.size());
	if (readbuf.size() >= 100) {
		const char *msg = "Wow, you're quite the author, aren't you?\n";
		add_write((uint8_t *)msg, strlen(msg));
		readbuf.clear();
	}
}

bool client::waiting_for_write() {
	return writebuf.size() > 0;
}

size_t client::get_to_write(uint8_t **outptr) {
	*outptr = writebuf.data();
	return writebuf.size();
}

void client::clear_writebuf(size_t n) {
	writebuf.erase(writebuf.begin(), writebuf.begin() + n);
}
