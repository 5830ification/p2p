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
	
	static bool parse_header(const std::vector<uint8_t>& data, packettype_t& outpkt);

protected:
	packettype_t _type;
	std::vector<uint8_t> _data;
};

class ProtocolHandler {

protected:
	const static std::map<protostate_t, 
		std::tuple<packettype_t, protostate_t>
	> transitions;

	protostate_t _state;
};

#endif
