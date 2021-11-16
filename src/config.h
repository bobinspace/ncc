#pragma once
#include <unistd.h>

// Purpose of read_threshold and write_threshold:
// Since we are using epoll and servicing ready fds in a round-robin fashion in a single thread, 
// we must ensure that no single fd hogs the i/o at the expense of other ready fds. 
// Therefore, when we service each of the ready fds, we constrain the number of bytes per read/write using read_threshold and write_threshold.
struct EpollServerConfig {
    unsigned short listening_port;
    size_t read_threshold;
    size_t write_threshold;
    int listening_backlog;
    
    EpollServerConfig();
    bool ReadFromCommandLine(int argc, char* argv[]);
};

struct ClientConfig {
    char hostname[1024];
    unsigned short remote_port;
    size_t read_threshold;
    size_t write_threshold;

    ClientConfig();
    bool ReadFromCommandLine(int argc, char* argv[]);
};
