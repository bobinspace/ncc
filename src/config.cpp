#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "logging.h"

EpollServerConfig::EpollServerConfig()
    : listening_port(0)
    , read_threshold(1024)
    , write_threshold(1024)
    , listening_backlog(1024)
{}

bool StringToPort(char const* const s, unsigned short& port) {
    char* end = 0;
    unsigned short temp_port = std::strtoul(s, &end, 10);
    if (s == end) {
        log::PrintLn(log::Error, "bad port");
        return false;
    }
    port = temp_port;
    return true;
}

bool EpollServerConfig::ReadFromCommandLine(int argc, char* argv[]) {
    if (argc < 2) return false;
    return StringToPort(argv[1], listening_port);
}

ClientConfig::ClientConfig() 
    : remote_port(0)
    , read_threshold(1024)
    , write_threshold(1024)
{
    memset(hostname, 0, sizeof(hostname));
}

bool ClientConfig::ReadFromCommandLine(int argc, char* argv[]) {
    if (argc < 3) return false;
    strncpy(hostname, argv[1], sizeof(hostname) - 1);
    return StringToPort(argv[2], remote_port);
}

