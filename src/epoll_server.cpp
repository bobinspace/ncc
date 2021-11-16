#include "epoll_server.h"
#include "socket_utils.h"
#include "logging.h"
#include "length_prefixed_stream_deserialiser.h"
#include "application_messages.h"
#include <string.h>
#include <set>
#include <map>
#include <deque>
#include <memory>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <thread>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include "session.h"
#include "serialiser.h"
#include "socket_utils.h"
#include "console_input_loop.h"

const int MaxNumEvents = 1024;

EpollServer::EpollServer(const int fd_listening, const EpollServerConfig& config)
    : fd_listening_(fd_listening)
    , config_(config)
    , sessions_(config_)
{}

EpollServer::~EpollServer() {
    CloseListeningAndSessionSockets();
}

void EpollServer::Run() {
    Loop();
    CloseListeningAndSessionSockets();
}

void EpollServer::Loop() {
    epoll_controller_.AddToInterestList(fd_listening_, EPOLLIN);

    epoll_event ready_events[MaxNumEvents] = { 0 };

    log::PrintLn(log::Info, "Entering epoll loop: monitoring %zu fds", epoll_controller_.NumWatchedFds());
    while (epoll_controller_.NumWatchedFds() > 0) {
        log::PrintLn(log::Debug, "wait ...");
        const int num_ready = epoll_controller_.WaitForEvents(ready_events, sizeof(ready_events) / sizeof(ready_events[0]), -1);
        if (-1 == num_ready) {
            if (EINTR == errno) {
                log::PrintLn(log::Info, "epoll_wait interrupted by signal. Continuing to wait ...");
                continue;
            }
            else {
                log::PrintLnCurrentErrno(log::Info, "Exit epoll loop");
                break;
            }
        }
        ProcessReadyEvents(ready_events, num_ready);
    }
    log::PrintLn(log::Info, "Exit epoll loop");
}

void EpollServer::ProcessReadyEvents(epoll_event* const ready_events, const size_t num_ready) {
    size_t num_processed_fd = 0;
    while (num_processed_fd < num_ready) {
        num_processed_fd = 0;
        for (size_t i = 0; i < num_ready; ++i) {
            auto& ready_event = ready_events[i];
            const int fd_ready = ready_event.data.fd;
            if (-1 == fd_ready) {
                continue;
            }

            Session* session_ptr = nullptr;
            sessions_.Add(fd_ready, session_ptr);
            assert(session_ptr);
            if (!session_ptr) continue;

            char events_string[512] = { 0 };
            EpollEventsToString(ready_event.events, events_string, sizeof(events_string));
            log::PrintLn(log::Debug, "%d|E|%s", fd_ready, events_string);

            bool is_processed = true;
            if (fd_listening_ == fd_ready) {
                OnListenerEvent();
            }
            else {
                if (ready_event.events & EPOLLIN) {
                    is_processed = !OnReadyToRead(*session_ptr);
                }
                if (ready_event.events & EPOLLOUT) {
                    is_processed = !OnReadyToWrite(*session_ptr);
                }
                if (ready_event.events & (EPOLLRDHUP | EPOLLHUP)) {
                    OnHangUp(fd_ready);
                }
                if (!(ready_event.events & (EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP))) {
                    OnUnknownEvent(ready_event);
                }
            }
            if (is_processed) {
                ready_event.data.fd = -1;
                ++num_processed_fd;
            }
        }
    }
}

void EpollServer::OnListenerEvent() {
    sockaddr_in client_address = { 0 };
    socklen_t client_address_size = sizeof(client_address);
    const int fd_accepted = accept(fd_listening_, (sockaddr*)&client_address, &client_address_size);

    if (-1 == fd_accepted) {
        log::PrintLnCurrentErrno(log::Error, "Failed accept");
    }
    else {
        sessions_.Add(fd_accepted);
        
        char client_address_as_string[64] = { 0 };
        inet_ntop(AF_INET, &client_address.sin_addr, client_address_as_string, sizeof(client_address_as_string));
        log::PrintLn(log::Info, "%d|Accepted|%s:%d", fd_accepted, client_address_as_string, ntohs(client_address.sin_port));

        SetNoBlocking(fd_accepted);

        static const uint32_t events_of_interest = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
        epoll_controller_.AddToInterestList(fd_accepted, events_of_interest);
    }
}

bool EpollServer::OnReadyToRead(Session& session) {
    if (!GetDataThenDeserialise
        ( session.deserialiser
        , session.socket_reader
        , session.read_threshold
        , session.ack_maker_and_serialiser)) {
        return false;
    }

    SendPendingMessagesThenSetupRetryAsNeeded(session, epoll_controller_);
    return true;
}

bool EpollServer::OnReadyToWrite(Session& session) {
    SendPendingMessagesThenSetupRetryAsNeeded(session, epoll_controller_);
    return (session.socket_writer.last_status > 0);
}

void EpollServer::OnHangUp(const int fd) {
    log::PrintLn(log::Debug, "%d|Peer hung up", fd);
    epoll_controller_.RemoveFromInterestList(fd);
    const int e = close(fd);
    if (e < 0) {
        log::PrintLnCurrentErrno(log::Error, "%d|Failed to close peer", fd);
    }
    
    log::PrintLn(log::Debug, "CL: DEL fd=%d", fd);
    sessions_.Remove(fd);
}

void EpollServer::OnUnknownEvent(const epoll_event& event) {
    log::PrintLn(log::Error, "Unrecognised event bits: 0x%08x", event.events);
}

void EpollServer::CloseListeningAndSessionSockets() {
    if (fd_listening_ >= 0) {
        log::PrintLn(log::Info, "%d|Closing listening ...", fd_listening_);
        const int e_close_listening = close(fd_listening_);
        log::PrintLnCurrentErrno(log::Info, "%d|%s", fd_listening_, (e_close_listening < 0) ? "Failed to close listening socket" : "Closed listening socket");
    }
    fd_listening_ = -1;

    const size_t num_sessions = sessions_.Size();
    log::PrintLn(log::Info, "Closing all %zu accepted fds ...", num_sessions);
    const size_t n_closed = CloseSessionSockets();
    log::PrintLn(log::Info, "Closed %zu/%zu accepted fds.", n_closed, num_sessions);
    sessions_.Clear();
}

size_t EpollServer::CloseSessionSockets() {
    struct CloseSocket {
        size_t n_closed ;
        CloseSocket() : n_closed(0) {}

        bool HandleFdAndSessionPtr(const int /*fd*/, Session* const session_ptr) {
            if ((!session_ptr) || (-1 == session_ptr->fd)) return true;
            const int e = close(session_ptr->fd);
            if (e < 0) {
                log::PrintLnCurrentErrno(log::Error, "%d|Failed to close accepted", session_ptr->fd);
            }
            else {
                ++n_closed;
            }

            return true;
        }
    };
    CloseSocket close_socket;
    sessions_.ForEachDo(close_socket);
    
    return close_socket.n_closed;
}

void EpollServer::AppendAndSerialiseFrameToAllSessions(char const* const frame_ptr, const size_t n) {
    struct AppendFrame {
        char const* const frame_ptr;
        const size_t n;
        EpollController& epoll_controller;
        AppendFrame(EpollController& epoll_controller, char const* const frame_ptr, const size_t n)
            : frame_ptr(frame_ptr)
            , n(n)
            , epoll_controller(epoll_controller)
        {}
        
        bool HandleFdAndSessionPtr(const int /*fd*/, Session* const session_ptr) {
            if ((!session_ptr) || (-1 == session_ptr->fd)) return true;
            session_ptr->serialiser.AppendFrame(frame_ptr, n);
            SendPendingMessagesThenSetupRetryAsNeeded(*session_ptr, epoll_controller);
            return true;
        }
    };
    
    AppendFrame append_frame(epoll_controller_, frame_ptr, n);
    sessions_.ForEachDo(append_frame);
}

struct AppendConsoleInputToServerSerialiser {
    EpollServer& server;
    AppendConsoleInputToServerSerialiser(EpollServer& server) : server(server) {}
    bool HandleFrame(char const* const frame_ptr, const size_t n) {
        server.AppendAndSerialiseFrameToAllSessions(frame_ptr, n);
        return true;
    }
};

void RunEpollServer(const EpollServerConfig& config) {
    log::PrintLn(log::Info, "Running server, listening on port %u", config.listening_port);
    int fd_listening = -1;
    if (!CreateAndListenOnNonBlockingSocket(config.listening_port, config.listening_backlog, fd_listening)) {
        return;
    }

    EpollServer server(fd_listening, config);

    AppendConsoleInputToServerSerialiser append_console_input_to_server_serialiser(server);
    std::thread gui_thread(RunConsoleInputLoop<AppendConsoleInputToServerSerialiser>, std::ref(append_console_input_to_server_serialiser));

    server.Run();
}

void SendPendingMessagesThenSetupRetryAsNeeded(Session& session, EpollController& epoll_controller) {
    session.serialiser.Serialise(session.socket_writer, session.write_threshold);
    if (session.serialiser.HasSerialisedAll()) {
        // Nothing more to send, no need to watch for EPOLLOUT event anymore.
        epoll_controller.ModifyInterestList(session.fd, 0, EPOLLOUT);
    }
    else {
        // Still has things to send, so explicitly register interest in EPOLLOUT and send when we get the EPOLLOUT event.
        epoll_controller.ModifyInterestList(session.fd, EPOLLOUT, 0);
    }
}
