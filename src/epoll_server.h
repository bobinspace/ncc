#pragma once
#include <unistd.h>
#include "config.h"
#include "session.h"
#include "epoll_controller.h"
#include "socket_utils.h"

struct epoll_event;

class EpollServer {
    EpollController epoll_controller_;
    int fd_listening_;
    const EpollServerConfig config_;
    Sessions sessions_;
    
    void Loop();
    size_t ProcessReadyEvents(epoll_event* const ready_events, const size_t num_ready);
    void OnListenerEvent();
    SocketIOStatus OnReadyToRead(Session&);
    SocketIOStatus OnReadyToWrite(Session&);
    void OnHangUp(const int fd);
    void OnUnknownEvent(const epoll_event&);
    
    void CloseListeningAndSessionSockets();
    size_t CloseSessionSockets();

public:
    EpollServer(const int fd_listening, const EpollServerConfig&);
    ~EpollServer();
    void AppendAndSerialiseFrameToAllSessions(char const* const frame_ptr, const size_t n);
    
    void Run();
};

/*
Attempt to stream out as much data stored in the serialiser as possible (within write_threshold limits).

If after that attempt, the serialiser still has data remaining, register interest in EPOLLOUT so that 
we can wait for that event and send again.

If after the send attempt, the serialiser doesn't have any more pending data, then unregister interest in 
EPOLLOUT, otherwise we will most likely be unnecessarily flooded with EPOLLOUT events even when we don'
t have data to write.
*/ 
SocketIOStatus SendPendingMessagesThenSetupRetryAsNeeded(Session& session, EpollController& epoll_controller);

void RunEpollServer(const EpollServerConfig&);