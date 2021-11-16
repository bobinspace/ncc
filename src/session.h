#pragma once
#include <map>
#include <mutex>
#include "config.h"
#include "length_prefixed_stream_deserialiser.h"
#include "serialiser.h"
#include "application_messages.h"
#include "logging.h"
#include "socket_reader.h"
#include "socket_writer.h"
#include "ack_maker_and_serialiser.h"
#include "io_benchmark.h"
#include "socket_utils.h"

struct Session {
    int fd;

    ThreadSafeIOBenchmark io_benchmark;

    LengthPrefixedStreamDeserialiser<size_t> deserialiser;
    SocketReader<ThreadSafeIOBenchmark> socket_reader;
    const size_t read_threshold;

    WaitableSerialiser serialiser;
    SocketWriter<ThreadSafeIOBenchmark> socket_writer;
    const size_t write_threshold;

    AckMakerAndSerialiser ack_maker_and_serialiser;

    Session(const int fd = -1, const size_t read_threshold = 1024, const size_t write_threshold = 1024);
    ~Session();

    void Reset();
    bool IsValid() const;
};

class Sessions {
    std::map<int, std::unique_ptr<Session>> sessions_;
    const EpollServerConfig& config_;
    mutable std::mutex mutex_;
public:
    Sessions(const EpollServerConfig& config) : config_(config) {}
    
    bool Add(const int fd) {
        Session* session_ptr = nullptr;
        return Add(fd, session_ptr);
    }

    bool Add(const int fd, Session*& session_ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(fd);
        const bool should_add_new = (sessions_.end() == it);
        if (should_add_new) {
            it = sessions_.insert(std::make_pair(fd, std::make_unique<Session>(fd, config_.read_threshold, config_.write_threshold))).first;
        }
        
        session_ptr = it->second.get();
        if (!session_ptr->IsValid()) {
            session_ptr->fd = fd;
        }

        return should_add_new;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.clear();
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }

    void Remove(const int fd) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(fd);
        if (sessions_.end() != it) {
            Session * session_ptr = it->second.get();
            if (session_ptr) {
                // ATTENTION: We reuse session objects by Reset-ing them instead of sessions_.erase(fd);
                session_ptr->Reset();
            }
        }
    }
    
    Session& Get(const int fd) {
        std::lock_guard<std::mutex> lock(mutex_);
        Session* session_ptr = nullptr;
        Add(fd, session_ptr);
        return *session_ptr;
    }

    template<typename FdAndSessionPtrHandler>
    void ForEachDo(FdAndSessionPtrHandler& fd_and_session_ptr_handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [fd, session_ptr] : sessions_) {
            fd_and_session_ptr_handler.HandleFdAndSessionPtr(fd, session_ptr.get());
        }
    }
};

template<typename Deserialiser, typename StreamReader, typename FrameHandler>
SocketIOStatus GetDataThenDeserialise(Deserialiser& deserialiser, StreamReader& stream_reader, const size_t read_threshold, FrameHandler& frame_handler) {
    deserialiser.AppendStream(stream_reader, read_threshold);

    const SocketIOStatus e = SummariseSocketIOStatus(read_threshold, stream_reader.last_status, stream_reader.last_errno);

    deserialiser.Deserialise(frame_handler);
    return e;
}
