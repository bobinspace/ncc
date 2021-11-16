#include <stdlib.h>
#include <signal.h>
#include "epoll_server.h"
#include "tcp_client.h"
#include "config.h"
#include "logging.h"

bool ReadFromCommandLineThenRun(int argc, char* argv[]) {
    if (argc > 1) {
        // Ignore broken pipe signal (e.g. from writing to closed client socket which hung up)
        signal(SIGPIPE, SIG_IGN);

        if (argc > 2) {
            ClientConfig config;
            if (!config.ReadFromCommandLine(argc, argv)) {
                return false;
            }
            RunTCPClient(config);
        }
        else {
            EpollServerConfig config;
            if (!config.ReadFromCommandLine(argc, argv)) {
                return false;
            }
            RunEpollServer(config);
        }

        return true;
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    if (!ReadFromCommandLineThenRun(argc, argv)) {
        log::PrintLn(log::Info, "Usage: ncc <listening_port|remote_host:remote_port>");
        return -1;
    }
    
    return 0;
}
