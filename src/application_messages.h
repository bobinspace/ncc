#pragma once
#include <time.h>
#include <unistd.h>
#include <string.h>

#pragma pack(push, 1)
enum MsgType {
    MsgType_HeartBeat,
    MsgType_Ack,
    MsgType_VariableLength,
};
const char* MsgTypeToString(const MsgType);

struct Header {
    size_t length;
    char type;
};

struct Ack {
    static const char msg_type = MsgType::MsgType_Ack;
    Header header_of_original_msg;
    Ack(const Header& header) {
        memcpy(&header_of_original_msg, &header, sizeof(header));
    }
};

template<typename T>
struct FixedSizeMsg {
    const Header header;
    T body;
    
    template<typename ... BodyArgs>
    FixedSizeMsg(BodyArgs&& ... body_args)
        : header({ sizeof(Header) + sizeof(T), T::msg_type })
        , body(body_args ...)
        {}
};
#pragma pack(pop)

