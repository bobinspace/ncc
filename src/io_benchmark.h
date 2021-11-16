#pragma once
#include <time.h>
#include <mutex>

class IOBenchmark {
    timespec last_pre_in_time_;
    timespec last_post_in_time_;

    timespec last_pre_out_time_;
    timespec last_post_out_time_;
public:
    IOBenchmark();
    void SetLastPreInTime(timespec const* const t = 0);
    void SetLastPostInTime(timespec const* const t = 0);
    void SetLastPreOutTime(timespec const* const t = 0);
    void SetLastPostOutTime(timespec const* const t = 0);

    bool NanosecSinceLastPreInTime(long int&, timespec const * const t1_ptr = 0);
    bool NanosecSinceLastPostInTime(long int&, timespec const * const t1_ptr = 0);
    bool NanosecSinceLastPreOutTime(long int&, timespec const* const t1_ptr = 0);
    bool NanosecSinceLastPostOutTime(long int&, timespec const* const t1_ptr = 0);

    void Reset();
};

class ThreadSafeIOBenchmark : IOBenchmark {
    std::mutex mutex_;
    IOBenchmark io_benchmark_;
public:
    void SetLastPreInTime(timespec const * const t = 0);
    void SetLastPostInTime(timespec const * const t = 0);
    void SetLastPreOutTime(timespec const * const t = 0);
    void SetLastPostOutTime(timespec const * const t = 0);

    bool NanosecSinceLastPreInTime(long int&, timespec const * const t1_ptr = 0);
    bool NanosecSinceLastPostInTime(long int&, timespec const * const t1_ptr = 0);
    bool NanosecSinceLastPreOutTime(long int&, timespec const * const t1_ptr = 0);
    bool NanosecSinceLastPostOutTime(long int&, timespec const * const t1_ptr = 0);

    void Reset();
};