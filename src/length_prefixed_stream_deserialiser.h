#pragma once
#include <vector>
#include <assert.h>
#include "string.h"

template<typename LengthFieldType = size_t>
class LengthPrefixedStreamDeserialiser {
	std::vector<char> buffer_;
	size_t num_populated_;

	char* FirstFramePtr() {
		return buffer_.empty() ? nullptr : &buffer_[0];
	}

	char const* ConstFirstFramePtr() const {
		return buffer_.empty() ? nullptr : &buffer_[0];
	}

	template<typename FrameHandler>
	void MoveTo(FrameHandler& frame_handler) {
		TrimLeft(num_populated_, &frame_handler);
	}

	template<typename FrameHandler>
	bool TrimLeft(const size_t n, FrameHandler* const frame_handler_ptr = 0) {
		const size_t num_bytes_to_trim = std::min(n, num_populated_);
		if (num_bytes_to_trim < 1) {
			return false;
		}

		char * const first_frame_ptr = FirstFramePtr();
		if (!first_frame_ptr) {
			return false;
		}

		if (frame_handler_ptr) {
			frame_handler_ptr->HandleFrame(first_frame_ptr, num_bytes_to_trim);
		}

		const size_t num_bytes_preserved = num_populated_ - num_bytes_to_trim;
		if (num_bytes_preserved > 0) {
			::memmove(first_frame_ptr, first_frame_ptr + num_bytes_to_trim, num_bytes_preserved);
		}

		num_populated_ -= num_bytes_to_trim;

		return true;
	}


	bool HasCompleteFrame(LengthFieldType& frame_length) const {
		if (num_populated_ < sizeof(LengthFieldType)) return false;

		char const* const first_frame_ptr = ConstFirstFramePtr();
		assert(first_frame_ptr);
		if (!first_frame_ptr) return false;

		frame_length = *((LengthFieldType*)first_frame_ptr);

		return (num_populated_ >= frame_length);
	}

	bool ResizeToFitMoreUsefulBytes(const size_t num_additional_bytes_to_populate) {
		const size_t capacity = buffer_.size();
		const size_t free_space = capacity - num_populated_;
		if (free_space < num_additional_bytes_to_populate) {
			const size_t shortfall = num_additional_bytes_to_populate - free_space;
			buffer_.resize(capacity + shortfall);
			return true;
		}
		return false;
	}
public:
	LengthPrefixedStreamDeserialiser()
		: num_populated_(0)
	{}

	// StreamReader: Functor signature: (char * const stream_ptr, const size_t num_bytes_to_read, size_t& num_bytes_read);
	template<typename StreamReader>
	size_t AppendStream(StreamReader& stream_reader, const size_t max_bytes_to_read) {
		ResizeToFitMoreUsefulBytes(max_bytes_to_read);
		
		size_t num_bytes_read = 0;
		stream_reader.ReadStream(&buffer_[num_populated_], max_bytes_to_read, num_bytes_read);
		num_populated_ += num_bytes_read;
		
		return num_bytes_read;
	}

	void AppendStream(char const* const stream_ptr, const size_t n) {
		if (!(stream_ptr && n)) return;

		ResizeToFitMoreUsefulBytes(n);

		memcpy(&buffer_[num_populated_], stream_ptr, n);
		num_populated_ += n;
	}

	template<typename T>
	void AppendStream(const T& x) {
		AppendStream((char const* const)(&x), sizeof(x));
	}

	// At the end of this, buffer_ will either have 0 useful bytes, or contain at most one partial frame.
	// FrameHandler: Functor signature: (char const * const frame_ptr, const size_t num_bytes_in_frame);
	template<typename FrameHandler>
	size_t Deserialise(FrameHandler& frame_handler) {
		size_t nFrame = 0;
		LengthFieldType frame_length = 0;
		while (HasCompleteFrame(frame_length)) {
			++nFrame;
			// ATTENTION: Since the length field includes its own size, 
			// even if the length value is less than the size of the length field itself,
			// we will still consider the frame length to be the size of the length field.
			// Hence the std::max.
			TrimLeft(std::max(sizeof(LengthFieldType), frame_length), &frame_handler);
		}
		return nFrame;
	}

	void Reset() {
		num_populated_ = 0;
	}
};
