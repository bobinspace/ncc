#include "tcp_client.h"
#include "socket_utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "session.h"
#include "logging.h"
#include "config.h"
#include "console_input_loop.h"

#include <iostream>
#include <string>
#include "application_messages.h"
#include <vector>
#include <thread>
#include <mutex>

bool RunReadLoop(Session& session) {
    while (GetDataThenDeserialise
        ( session.deserialiser
        , session.socket_reader
        , session.read_threshold
        , session.ack_maker_and_serialiser)) 
    {}

    return true;
}

bool RunWriteLoop(Session& session) {
    while (1) {
        session.serialiser.WaitSerialise(session.socket_writer, session.write_threshold);
        if (session.socket_writer.last_status <= 0) {
            return false;
        }
    }
    return true;
}

struct TrySerialiseAsap {
    WaitableSerialiser& serialiser;
    TrySerialiseAsap(WaitableSerialiser& serialiser) : serialiser(serialiser) {}
    bool HandleFrame(char const* const frame_ptr, const size_t n) {
        serialiser.AppendFrame(frame_ptr, n);
        return true;
    }
};

int RunTCPClient(const ClientConfig& config) {
    log::PrintLn(log::Info, "Running client connecting to %s:%u", config.hostname, config.remote_port);
    int fd = -1;
    if (!BindOrConnect(config.hostname, config.remote_port, false, fd)) {
        log::PrintLn(log::Error, "Failed to connect to %s:%u", config.hostname, config.remote_port);
        close(fd);
        return -1;
    }
    
    DisableNaglesAlgorithm(fd);

    Session session(fd, config.read_threshold, config.write_threshold);
    
    TrySerialiseAsap try_serialise_asap(session.serialiser);

    std::thread gui_thread(RunConsoleInputLoop<TrySerialiseAsap>, std::ref(try_serialise_asap));
    std::thread reader_thread(RunReadLoop, std::ref(session));
    std::thread writer_thread(RunWriteLoop, std::ref(session));
    
    writer_thread.join();
    reader_thread.join();
    gui_thread.join();

    close(fd);
    return 0;
}
