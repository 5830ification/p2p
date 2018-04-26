#include "app.h"
#include "client.h"
#include "log.h"

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

static uint16_t parse_port(const char *str) {
	int port = std::stoi(str);
	
	if (port < (int)std::numeric_limits<uint16_t>::min() || 
		port > (int)std::numeric_limits<uint16_t>::max())
		throw std::out_of_range("Number must be within unsinged 16-bit range");

	return (uint16_t)port;
}

AppSettings::AppSettings(uint16_t listen_port,
	const std::vector<ConnectionParams>& initial_clients): 
	_listen_port(listen_port),
	_initial_clients(initial_clients)
{

}

uint16_t AppSettings::listen_port() {
	return _listen_port;
}

void AppSettings::listen_port(uint16_t lport) {
	_listen_port = lport;
}

const std::vector<ConnectionParams>& AppSettings::initial_clients() {
	return _initial_clients;
}

AppSettings AppSettings::parse(int argc, const char **argv) {
	AppSettings settings;

	if (argc >= 2) {
		try {
			settings.listen_port(parse_port(argv[1]));
		} catch (std::invalid_argument& e) {
			log(LERROR, "Could not parse port '%s': Invalid format.", argv[1]);
			throw e;
		} catch (std::out_of_range& e) {
			log(LERROR, "Could not parse port '%s': Out of range.", argv[1]);
			throw e;
		}
	} else {
		log(LWARN, "No listening port supplied, using default: %u", settings.listen_port());
	}

	if (argc >= 3) {
		settings._initial_clients = _parse_params(argc, argv);
	} else {
		log(LWARN, "No initial peers specified. Starting in server only mode...");
	}

	return settings;
}

std::vector<ConnectionParams> AppSettings::_parse_params(int argc, const char **argv) {
	if (argc <= 2) {
		throw std::invalid_argument("Can only parse initial peers if they are given...");
	}

	std::vector<ConnectionParams> params;

	for (int i = 2; i < argc; i++) {
		std::string str(argv[i]);

		size_t seperator = str.find(':');
		if (seperator == std::string::npos || seperator == str.length() - 1) {
			log(LERROR, "Error while parsing peer specifier '%s': Invalid format. " 
				"Expected: <IP>:<Port>", argv[i]);
			throw std::invalid_argument("Invalid peer specifier.");
		}

		uint16_t port = parse_port(str.substr(seperator + 1).c_str());

		std::string ip_str = str.substr(0, seperator);
		unsigned int parts[4];
		if (sscanf(ip_str.c_str(), "%u.%u.%u.%u", 
			parts, parts + 1, parts + 2, parts + 3) != 4) {
			log(LERROR, "Error while parsing peer specifier '%s': Invalid format. "
				"Expected: <IP>:<Port>", argv[i]);
			throw std::invalid_argument("Invalid peer specifier.");
		}

		struct sockaddr_in addr;
		addr.sin_addr.s_addr = 0;
		for (int j = 0; j < 4; j++) {
			if (parts[j] > 255) {
				throw std::invalid_argument("Invalid peer specifier.");
			}

			addr.sin_addr.s_addr |= (parts[j] << (8 * j));
		}

		addr.sin_family = AF_INET;
		addr.sin_port = port;

		params.push_back({addr});
	}

	return params;
}

P2PApp::P2PApp(int argc, const char **argv) {
	settings = AppSettings::parse(argc, argv);
}

void P2PApp::run() {
	log(LINFO, "Running, WOOO!");

	fd_t listen_sock = bind_and_listen(settings.listen_port());
	log(LINFO, "Listening on port %u!", settings.listen_port());

	int epoll_fd = epoll_create(16);
	struct epoll_event events[EPOLL_EVENT_BUFFER_SIZE];

	struct epoll_event evt;
	evt.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
	evt.data.fd = listen_sock;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &evt);

	clientmap clients;

	for (auto& p : settings.initial_clients()) {
		log(LINFO, "Connecting to peer with IP %s...", 
			ipv4_to_str(p.address.sin_addr.s_addr).c_str());

		fd_t clisock = connect_to_peer(p.address, p.address.sin_port);

		prepare_client(clisock, epoll_fd);
		clients.insert({clisock, std::make_unique<client>(clisock, p.address, true)});
		log(LINFO, "Connected to initial peer with IP %s!",
			ipv4_to_str(p.address.sin_addr.s_addr).c_str());
	}

	for (;;) {
		int fd_count = epoll_wait(epoll_fd, events, EPOLL_EVENT_BUFFER_SIZE, -1);

		if (fd_count < 0) {
			log(LERROR, "epoll_wait failed: %d!", fd_count);
			throw std::runtime_error("epoll_wait failed");
		}

		for (int e = 0; e < fd_count; e++) {
			handle_event(events[e], epoll_fd, listen_sock, clients);
		}
	}
}
