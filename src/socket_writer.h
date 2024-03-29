#pragma once
#include <unistd.h>
#include <time.h>

template<typename Benchmark>
struct SocketWriter {
    const int fd;
    int last_status;
    int last_errno;
    Benchmark* const benchmark_ptr;

    SocketWriter(const int fd, Benchmark* const benchmark_ptr = nullptr)
        : fd(fd)
        , last_status(0)
        , last_errno(0)
        , benchmark_ptr(benchmark_ptr)
    {}

    void Reset() {
        last_status = 0;
        last_errno = 0;
        if (benchmark_ptr) {
            benchmark_ptr->Reset();
        }
    }
    
    bool WriteStream(const char* const stream_ptr, const size_t num_bytes_to_serialise, size_t& num_bytes_serialised) {
        if (benchmark_ptr) {
            benchmark_ptr->SetLastPreOutTime();
        }

        last_status = write(fd, stream_ptr, num_bytes_to_serialise);

        if (benchmark_ptr) {
            benchmark_ptr->SetLastPostOutTime();
        }

        last_errno = errno;
        if (last_status > 0) {
            num_bytes_serialised = last_status;
            return true;
        }
        return false;
    }
};
