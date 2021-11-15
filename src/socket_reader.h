#pragma once
#include <unistd.h>
#include <time.h>

template<typename Benchmark>
struct SocketReader {
	const int fd;
	int last_status;
	int last_errno;
	Benchmark* const benchmark_ptr;

	SocketReader(const int fd, Benchmark* const benchmark_ptr = nullptr)
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
	
	bool ReadStream(char* const p, const size_t num_bytes_to_read, size_t& num_bytes_read) {
		num_bytes_read = 0;
		if (0 == num_bytes_to_read) return false;
		
		if (benchmark_ptr) {
			benchmark_ptr->SetLastPreInTime();
		}

		last_status = ::read(fd, p, num_bytes_to_read);

		if (benchmark_ptr) {
			benchmark_ptr->SetLastPostInTime();
		}

		last_errno = errno;
		if (last_status > 0) {
			num_bytes_read = static_cast<size_t>(last_status);
			return true;
		}
		return false;
	}
};
