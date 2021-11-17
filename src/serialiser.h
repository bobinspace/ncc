#pragma once
#include <vector>
#include <assert.h>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <string.h>

/*
Collects bytes into a single vector and serialises them out into a stream_writer, possibly over many batches.
"Append" methods collects bytes to be serialised.
Serialise(...) can be called repeatedly to sequentially serialise the collected bytes out into a stream_writer.
*/
class Serialiser {
    std::vector<char> buffer_;
    size_t num_serialised_;
    size_t num_populated_;

    char const * UnserialisedStartPtr() const {
        if (num_serialised_ < buffer_.size()) {
            return &buffer_[num_serialised_];
        }
        return nullptr;
    }

    bool ResetIfHasSerialisedAll() {
        if (HasSerialisedAll()) {
            Reset();
            return true;
        }
        return false;
    }

    size_t NumBytesLeftToSerialise() const {
        if (num_serialised_ < num_populated_) {
            return num_populated_ - num_serialised_;
        }
        return 0;
    }

public:
    Serialiser() {
        Reset();
    }

    bool HasSerialisedAll() const {
        assert(num_serialised_ <= num_populated_);
        return num_serialised_ == num_populated_;
    }

    void Reset() {
        num_serialised_ = 0;
        num_populated_ = 0;
    }

    void AppendFrame(char const* const frame_ptr, const size_t n) {
        if (!(frame_ptr && n)) return;

        const size_t N = buffer_.size();
        const size_t new_num_populated = num_populated_ + n;
        if (N < new_num_populated) {
            buffer_.resize(new_num_populated);
        }
        memcpy(&buffer_[num_populated_], frame_ptr, n);
        num_populated_ += n;
    }

    template<typename Frame>
    void AppendFrame(const Frame& frame) {
        AppendFrame((char const* const)(&frame), sizeof(frame));
    }

    template<typename StreamWriter>
    size_t Serialise(StreamWriter& stream_writer, const size_t max_bytes_to_serialise) {
        size_t num_bytes_serialised = 0;
        const size_t num_bytes_to_serialise = (std::min)(max_bytes_to_serialise, NumBytesLeftToSerialise());
        
        if (num_bytes_to_serialise > 0) {
            char const * const stream_ptr = UnserialisedStartPtr();
            if (stream_ptr) {
                stream_writer.WriteStream(stream_ptr, num_bytes_to_serialise, num_bytes_serialised);
                if (num_bytes_serialised > 0) {
                    num_serialised_ += num_bytes_serialised;
                    ResetIfHasSerialisedAll();
                }
            }
        }
        
        return num_bytes_serialised;
    }
};

struct BooleanConditionVariable {
    mutable std::mutex mutex;
    std::condition_variable condition;
    bool variable;
};

class WaitableSerialiser {
    BooleanConditionVariable has_items_;
    Serialiser serialiser_;
public:
    WaitableSerialiser() {
        has_items_.variable = false;
    }

    bool HasSerialisedAll() const {
        std::lock_guard<std::mutex> lock(has_items_.mutex);
        return serialiser_.HasSerialisedAll();
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(has_items_.mutex);
        serialiser_.Reset();
    }

    void AppendFrame(char const* const frame_ptr, const size_t n) {
        {
            std::lock_guard<std::mutex> lock(has_items_.mutex);
            serialiser_.AppendFrame(frame_ptr, n);
            has_items_.variable = !serialiser_.HasSerialisedAll();
        }
        has_items_.condition.notify_all();
    }

    template<typename T>
    void AppendFrame(const T& x) {
        AppendFrame((char const* const)(&x), sizeof(x));
    }

    // StreamWriter: Functor signature: (const char * const stream_ptr, const size_t num_bytes_to_serialise, size_t& num_bytes_serialised);
    template<typename StreamWriter>
    size_t Serialise(StreamWriter& stream_writer, const size_t max_bytes_to_serialise) {
        std::lock_guard<std::mutex> lock(has_items_.mutex);
        const size_t num_bytes_serialised = serialiser_.Serialise(stream_writer, max_bytes_to_serialise);
        
        has_items_.variable = !serialiser_.HasSerialisedAll();
        
        return num_bytes_serialised;
    }

    template<typename StreamWriter>
    size_t WaitSerialise(StreamWriter& stream_writer, const size_t max_bytes_to_serialise) {
        std::unique_lock<std::mutex> lock(has_items_.mutex);

        // has_items_.condition.wait releases the mutex, suspends the thread, and waits for another thread 
        // to call has_items_.condition.notifyxxx to wake us up.
        has_items_.condition.wait(lock, [&] { return has_items_.variable; });
        // Once has_items_.condition.wait returns, mutex is acquired atomically

        const size_t num_bytes_serialised = serialiser_.Serialise(stream_writer, max_bytes_to_serialise);
        
        has_items_.variable = !serialiser_.HasSerialisedAll();

        return num_bytes_serialised;
    }
};