#include "protocol.h"

const std::map<protostate_t, std::tuple<packettype_t, protostate_t>> 
	ProtocolHandler::transitions = {
	{PSTATE_PASSV, {PACKET_HELLO, PSTATE_PASSV_HELLO}}, // Passive --Hello--> PassiveHello
	{PSTATE_PASSV_HELLO, {PACKET_HELLO, PSTATE_CONNECTED}} // PassiveHello --Hello--> Connected	
};
