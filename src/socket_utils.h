#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

bool BindOrConnect(char const* const hostname, const unsigned short port, const bool should_bind_instead_of_connect, int& fd);
int SetNoBlocking(const int fd);
int CreateNonBlockingListeningFd(const unsigned short listening_port);
bool CreateAndListenOnNonBlockingSocket(const unsigned short listening_port, const int listening_backlog, int& fd_listening);

// returns number of characters correctly written, EXCLUDING null-terminator
// events_string will contain the null-terminator if return value > 0.
int EpollEventToString(const uint32_t event, char* const events_string, const size_t n);
void EpollEventsToString(const uint32_t events, char* const events_string, const size_t n);
