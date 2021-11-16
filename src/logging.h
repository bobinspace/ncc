#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <mutex>
namespace log {
    enum Level {
        Info,
        Warn,
        Error,
        Debug,
    };
    int PrintLn(const Level, const char* fmt, ...);
    int Print(const Level, const char* fmt, ...);
    
    // Append errno number and description at the end of the printed line.
    int PrintLnCurrentErrno(const Level, const char* fmt, ...);
    int PrintLnErrno(const Level, const int the_errno, const char* fmt, ...);
}
