#pragma once
#include "application_messages.h"
#include "logging.h"
#include "serialiser.h"
#include "io_benchmark.h"

struct AckMakerAndSerialiser {
    WaitableSerialiser& serialiser;
    ThreadSafeIOBenchmark& io_benchmark;

    AckMakerAndSerialiser(WaitableSerialiser& serialiser, ThreadSafeIOBenchmark& io_benchmark)
        : serialiser(serialiser)
        , io_benchmark(io_benchmark)
    {}

    bool HandleFrame(char const* const frame_ptr, const size_t num_bytes_in_frame) {
        if (num_bytes_in_frame >= sizeof(Header)) {
            const Header& header = *((Header const*)frame_ptr);
            switch (header.type) {
            case MsgType_Ack:
            {
                const FixedSizeMsg<Ack>& ackMsg = *((FixedSizeMsg<Ack> const*)frame_ptr);
                long int round_trip_duration_ns = 0;
                if (io_benchmark.NanosecSinceLastPostOutTime(round_trip_duration_ns)) {
                    log::PrintLn(log::Info, "Got Ack: rtrip=%ldus (%zu bytes)", round_trip_duration_ns / 1000, std::max(sizeof(Header), ackMsg.body.header_of_original_msg.length) - sizeof(header));
                }
            }
            break;
            default:
            {
                log::PrintLn(log::Info, "Got %s (%zu bytes)", MsgTypeToString(MsgType(header.type)), std::max(sizeof(Header), header.length) - sizeof(header));

                // Send Ack only if the incoming message is not an Ack, otherwise we end up sending Acks to-and-fro endlessly.
                serialiser.AppendFrame(FixedSizeMsg<Ack>(header));
            }
            break;
            }
            return true;
        }
        return false;
    }
};

