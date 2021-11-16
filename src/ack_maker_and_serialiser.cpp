#include "application_messages.h"

const char* MsgTypeToString(const MsgType msg_type) {
    switch (msg_type) {
    case MsgType_HeartBeat: return "HeartBeat";
    case MsgType_Ack: return "Ack";
    case MsgType_VariableLength: return "VarLength";
    }
    return "Unknown";
}