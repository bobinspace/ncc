#include "io_benchmark.h"
#include <string.h>

void SetTime(timespec& dest, timespec const * const src_ptr) {
	if (src_ptr) {
		memcpy(&dest, src_ptr, sizeof(*src_ptr));
	}
	else {
		clock_gettime(CLOCK_REALTIME, &dest);
	}
}

bool NanosecElapsed(long int& nanosec_elapsed, const timespec& t0, const timespec& t1) {
	if (t1.tv_sec < t0.tv_sec) return false;
	if ((t1.tv_sec == t0.tv_sec) && (t1.tv_nsec < t0.tv_nsec)) {
		 return false;
	}
	nanosec_elapsed = (1000000000 * (t1.tv_sec - t0.tv_sec)) + (t1.tv_nsec - t0.tv_nsec);
	return true;
}

bool NanosecElapsed(long int& nanosec_elapsed, const timespec& t0, timespec const * const t1_ptr) {
	if (t1_ptr) {
		return NanosecElapsed(nanosec_elapsed, t0, *t1_ptr);
	}
	else {
		timespec now = { 0 };
		clock_gettime(CLOCK_REALTIME, &now);
		return NanosecElapsed(nanosec_elapsed, t0, now);
	}
}

IOBenchmark::IOBenchmark() {
	Reset();
}

void IOBenchmark::Reset() {
	memset(&last_pre_in_time_, 0, sizeof(last_pre_in_time_));
	memset(&last_post_in_time_, 0, sizeof(last_post_in_time_));
	memset(&last_pre_out_time_, 0, sizeof(last_pre_out_time_));
	memset(&last_post_out_time_, 0, sizeof(last_post_out_time_));
}

void IOBenchmark::SetLastPreInTime(timespec const * const t) {
	SetTime(last_pre_in_time_, t);
}

void IOBenchmark::SetLastPostInTime(timespec const * const t) {
	SetTime(last_post_in_time_, t);
}

void IOBenchmark::SetLastPreOutTime(timespec const * const t) {
	SetTime(last_pre_out_time_, t);
}

void IOBenchmark::SetLastPostOutTime(timespec const * const t) {
	SetTime(last_post_out_time_, t);
}

bool IOBenchmark::NanosecSinceLastPreInTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	return NanosecElapsed(nanosec_elapsed, last_pre_in_time_, t1_ptr);
}

bool IOBenchmark::NanosecSinceLastPostInTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	return NanosecElapsed(nanosec_elapsed, last_post_in_time_, t1_ptr);
}

bool IOBenchmark::NanosecSinceLastPreOutTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	return NanosecElapsed(nanosec_elapsed, last_pre_out_time_, t1_ptr);
}

bool IOBenchmark::NanosecSinceLastPostOutTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	return NanosecElapsed(nanosec_elapsed, last_post_out_time_, t1_ptr);
}

void ThreadSafeIOBenchmark::SetLastPreInTime(timespec const* const t) {
	std::lock_guard<std::mutex> lock(mutex_);
	io_benchmark_.SetLastPreInTime(t);
}

void ThreadSafeIOBenchmark::SetLastPostInTime(timespec const* const t) {
	std::lock_guard<std::mutex> lock(mutex_);
	io_benchmark_.SetLastPostInTime(t);
}

void ThreadSafeIOBenchmark::SetLastPreOutTime(timespec const* const t) {
	std::lock_guard<std::mutex> lock(mutex_);
	io_benchmark_.SetLastPreOutTime(t);
}

void ThreadSafeIOBenchmark::SetLastPostOutTime(timespec const* const t) {
	std::lock_guard<std::mutex> lock(mutex_);
	io_benchmark_.SetLastPostOutTime(t);
}

bool ThreadSafeIOBenchmark::NanosecSinceLastPreInTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	return io_benchmark_.NanosecSinceLastPreInTime(nanosec_elapsed, t1_ptr);
}

bool ThreadSafeIOBenchmark::NanosecSinceLastPostInTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	return io_benchmark_.NanosecSinceLastPostInTime(nanosec_elapsed, t1_ptr);
}

bool ThreadSafeIOBenchmark::NanosecSinceLastPreOutTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	return io_benchmark_.NanosecSinceLastPreOutTime(nanosec_elapsed, t1_ptr);
}

bool ThreadSafeIOBenchmark::NanosecSinceLastPostOutTime(long int& nanosec_elapsed, timespec const* const t1_ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	return io_benchmark_.NanosecSinceLastPostOutTime(nanosec_elapsed, t1_ptr);
}

void ThreadSafeIOBenchmark::Reset() {
	std::lock_guard<std::mutex> lock(mutex_);
	return io_benchmark_.Reset();
}