#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

typedef enum {
	PSTATE_PASSV,
	PSTATE_ACT,
	PSTATE_PASSV_HELLO,
	PSTATE_ACT_HELLO,
	PSTATE_CONNECTED
} protostate_t;

typedef enum {
	PACKET_HELLO
} packettype_t;

class Packet {
public:
	/*
	 * Check whether this packet has already 
	 * read its header into the _data buffer.
	 */
	bool have_header() const;

	/*
	 * Read body_size from header.
	 * !Warning! Outsize-controlled data!
	 */
	size_t body_size() const;

	/*
	 * Read packet type from header.
	 */
	packettype_t type() const;

	/*
	 * Returns whether this packet has been parsed
	 * completely.
	 */
	bool is_complete() const;

	/*
	 * Add 'data' into this packets internal buffer.
	 * Will return the amount of bytes that were consumed.
	 * If add_data returns less than data.size(), the packet
	 * does not require additional data.
	 */
	size_t add_data(const std::vector<uint8_t>& data);

protected:
	std::vector<uint8_t> _data;
	packettype_t _type;
};

class ProtocolHandler {

protected:
	const static std::map<protostate_t, 
		std::tuple<packettype_t, protostate_t>
	> transitions;

	protostate_t _state;
};

#endif
