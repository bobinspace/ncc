#include "socket_utils.h"
#include "logging.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <stdio.h>
#include "logging.h"

bool BindOrConnect(char const * const hostname, const unsigned short port, const bool should_bind_instead_of_connect, int& fd) {
	fd = -1;
	char port_as_string[16] = { 0 };
	snprintf(port_as_string, sizeof(port_as_string), "%u", port);

	addrinfo hints = { 0 };
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	char const * const node = (!hostname || (!hostname[0]) || !strcmp(hostname, "*")) ? nullptr : hostname;

	addrinfo * interfaces = nullptr;
	int e_getaddrinfo = getaddrinfo(node, port_as_string, &hints, &interfaces);
	if (e_getaddrinfo) {
		log::PrintLn(log::Error, "Failed getaddrinfo: %s", gai_strerror(e_getaddrinfo));
		return false;
	}

	for (addrinfo* interface = interfaces; nullptr != interface; interface = interface->ai_next) {
		const int interface_fd = socket(interface->ai_family, interface->ai_socktype, interface->ai_protocol);
		if (-1 == interface_fd)
			continue;

		if (should_bind_instead_of_connect) {
			const int e_bind = bind(interface_fd, interface->ai_addr, interface->ai_addrlen);
			if (0 == e_bind) {
				fd = interface_fd;
				break;
			}
		}
		else {
			const int e_connect = ::connect(interface_fd, interface->ai_addr, interface->ai_addrlen);
			if (0 == e_connect) {
				fd = interface_fd;
				break;
			}
		}

		close(interface_fd);
	}

	freeaddrinfo(interfaces);

	return fd != -1;
}

int SetNoBlocking(const int fd) {
	const int original_file_status_flags = fcntl(fd, F_GETFL, 0);
	const int e = fcntl(fd, F_SETFL, original_file_status_flags | O_NONBLOCK);
	return e;
}

int CreateNonBlockingListeningFd(const unsigned short listening_port) {
	int fd_listening = -1;
	if (!BindOrConnect(nullptr, listening_port, true, fd_listening)) {
		log::PrintLnCurrentErrno(log::Error, "Failed bind to port %d", listening_port);
		close(fd_listening);
	}

	const int e_set_non_blocking = SetNoBlocking(fd_listening);
	if (e_set_non_blocking < 0) {
		log::PrintLnCurrentErrno(log::Error, "%d|Failed set non-blocking (port=%d)", fd_listening, listening_port);
		close(fd_listening);
		return e_set_non_blocking;
	}

	return fd_listening;
}

bool CreateAndListenOnNonBlockingSocket(const unsigned short listening_port, const int listening_backlog, int& fd_listening) {
	fd_listening = CreateNonBlockingListeningFd(listening_port);
	if (fd_listening < 0) {
		log::PrintLnCurrentErrno(log::Error, "Failed to create listening port on %d", listening_port);
		return false;
	}
	log::PrintLn(log::Info, "%d|Created listening port on %d", fd_listening, listening_port);

	const int e_listen = listen(fd_listening, listening_backlog);
	if (e_listen < 0) {
		log::PrintLnCurrentErrno(log::Error, "Failed to listen on port %d", listening_port);
		::close(fd_listening);
		return false;
	}
	log::PrintLn(log::Info, "%d|Listening on port %d", fd_listening, listening_port);
	return true;
}

// returns number of characters correctly written, EXCLUDING null-terminator
// events_string will contain the null-terminator if return value > 0.
int EpollEventToString(const uint32_t event, char* const events_string, const size_t n) {
	if (!(events_string && n)) return 0;

#define DOALL(DOONE) \
	DOONE(IN)\
	DOONE(PRI)\
	DOONE(OUT)\
	DOONE(ERR)\
	DOONE(HUP)\
	DOONE(RDNORM)\
	DOONE(RDBAND)\
	DOONE(WRNORM)\
	DOONE(WRBAND)\
	DOONE(MSG)\
	DOONE(RDHUP)\
	DOONE(EXCLUSIVE)\
	DOONE(WAKEUP)\
	DOONE(ONESHOT)\
	DOONE(ET)\

#define EVENT_TO_STRING(x) \
	if (EPOLL##x == event) {\
		static const char s[] = #x;\
		const size_t num_bytes_to_write = sizeof(s);\
		if (n < num_bytes_to_write) return 0;\
		strncpy(events_string, s, num_bytes_to_write);\
		return num_bytes_to_write - 1;\
	}

	DOALL(EVENT_TO_STRING)
#undef EVENT_TO_STRING
#undef DOALL

	// snprintf returns the number of characters in the desired rendered string, EXCLUDING null-terminator.
	// Therefore, string has been completely written only if returned value is positive and less than n.
	const int e = snprintf(events_string, n, "%x", event);

	return ((e > 0) && (static_cast<size_t>(e) < n)) ? e : 0;
}

void EpollEventsToString(const uint32_t events, char* const events_string, const size_t n) {
	size_t offset = 0;
	bool is_first = true;
	const uint32_t base = 1;
	for (size_t i = 0; i < sizeof(events) * 8; ++i) {
		const uint32_t event = (base << i);
		if (events & event) {
			if (is_first) {
				is_first = false;
			}
			else {
				if (offset >= (n - 1)) return;
				events_string[offset] = ' ';
				++offset;
			}
			if (offset >= (n - 1)) return;
			offset += EpollEventToString(event, events_string + offset, n - offset);
		}
	}
}
