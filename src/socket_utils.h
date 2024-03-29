#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

bool BindOrConnect(char const* const hostname, const unsigned short port, const bool should_bind_instead_of_connect, int& fd);
int SetNoBlocking(const int fd);
int CreateNonBlockingListeningFd(const unsigned short listening_port);
bool CreateAndListenOnNonBlockingSocket(const unsigned short listening_port, const int listening_backlog, int& fd_listening);

// Returns number of characters correctly written, EXCLUDING null-terminator
// events_string will contain the null-terminator if return value > 0.
int EpollEventToString(const uint32_t event, char* const events_string, const size_t n);
void EpollEventsToString(const uint32_t events, char* const events_string, const size_t n);

int DisableNaglesAlgorithm(const int fd);

template<typename T>
int SetSocketOption(const int fd, const int option, const T option_value, const int level = IPPROTO_TCP) {
    const int e = setsockopt(fd, level, option, (char *)&option_value, sizeof(option_value)); 
    return e;
}

enum SocketIOStatus {
    WouldBlock,
    WentThrough,
    PeerHungUp,
};

SocketIOStatus SummariseSocketIOStatus(const int num_desired_io_bytes_passed_to_socket_api, const int socket_api_call_return_value, const int socket_api_call_errno);