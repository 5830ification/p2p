#include "protocol.h"

#include <algorithm>

const std::map<protostate_t, std::tuple<packettype_t, protostate_t>> 
	ProtocolHandler::transitions = {
	{PSTATE_PASSV, {PACKET_HELLO, PSTATE_PASSV_HELLO}}, // Passive --Hello--> PassiveHello
	{PSTATE_PASSV_HELLO, {PACKET_HELLO, PSTATE_CONNECTED}} // PassiveHello --Hello--> Connected
};

bool Packet::have_header() const {
	return _data.size() >= (sizeof(uint16_t) + sizeof(uint64_t));
}

size_t Packet::body_size() const {
	if (!have_header()) {
		throw "Invalid operation.";
	}

	uint64_t *sz = (uint64_t *)(_data.data() + 2);
	return (size_t)*sz;
}

packettype_t Packet::type() const {
	if (!have_header()) {
		throw "Invalid operation.";
	}

	uint16_t *ty = (uint16_t *)(_data.data());
	return (packettype_t)*ty;
}

size_t Packet::add_data(const std::vector<uint8_t>& data) {
	if (have_header()) {
		size_t bsize = body_size();
		size_t body_have_bytes = _data.size() - (sizeof(uint16_t) + sizeof(uint64_t));

		if (bsize < body_have_bytes) {
			throw "Invalid state.";
		} else if (bsize == body_have_bytes) {
			return 0;
		} else {
			size_t remaining = bsize - body_have_bytes;
			size_t to_consume = std::min(remaining, data.size());

			_data.insert(_data.end(), data.begin(), data.begin() + to_consume);
			return to_consume;
		}
	} else {
		size_t header_rest = (sizeof(uint16_t) + sizeof(uint64_t)) - _data.size();
		size_t to_consume = std::min(header_rest, data.size());
		_data.insert(_data.end(), data.begin(), data.begin() + to_consume);

		if (!have_header())
			return to_consume;

		// We have now read the header...
		if (to_consume == data.size())
			return to_consume;
		else {
			std::vector<uint8_t> temp(data.begin() + to_consume, data.end());
			return to_consume + add_data(temp);
		}
	}
}

bool Packet::is_complete() const {
	if (!have_header())
		return false;
	
	return _data.size() - (sizeof(uint16_t) + sizeof(uint64_t)) == body_size(); 
}
