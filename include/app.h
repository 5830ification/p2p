#ifndef APP_H
#define APP_H

#include <cstdint>
#include <vector>

#include "p2p.h"

extern "C" {
	#include <netinet/in.h>
}

typedef struct {
	struct sockaddr_in address;
} ConnectionParams;

class AppSettings {
public:
	
	AppSettings(uint16_t listen_port = 8080, 
		const std::vector<ConnectionParams>& initial_clients = {});

	uint16_t listen_port();
	void listen_port(uint16_t);

	const std::vector<ConnectionParams>& initial_clients();

	static AppSettings parse(int argc, const char **argv);
	
protected:

	uint16_t _listen_port;
	std::vector<ConnectionParams> _initial_clients;
	
	static std::vector<ConnectionParams> _parse_params(int argc, const char **argv);
};

class P2PApp {
public:

	P2PApp(int argc, const char **argv);

	void run();

protected:

	AppSettings settings;
};

#endif
