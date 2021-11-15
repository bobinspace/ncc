#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

namespace log {
	std::mutex g_console_out_mutex;
	Level g_level = Info;

	int PrintImpl(const Level level, const bool should_append_newline_char, va_list& args, const char* fmt) {
		if (level > g_level) return 0;
		std::lock_guard lock(g_console_out_mutex);
		char msg[2048] = { 0 };
		const int e = vsnprintf(msg, sizeof(msg), fmt, args);
		if (e > 0) {
			printf("%s", msg);
			if (should_append_newline_char) {
				printf("\n");
			}
		}
		return e;
	}

	int PrintLn(const Level level, const char* fmt, ...) {
		if (level > g_level) return 0;
		va_list args;
		va_start(args, fmt);
		const int e = PrintImpl(level, true, args, fmt);
		va_end(args);
		return e;
	}

	int Print(const Level level, const char* fmt, ...) {
		if (level > g_level) return 0;
		va_list args;
		va_start(args, fmt);
		const int e = PrintImpl(level, false, args, fmt);
		va_end(args);
		return e;
	}

	int PrintLnErrno(const Level level, const int the_errno, const char* fmt, ...) {
		if (level > g_level) return 0;
		va_list args;
		va_start(args, fmt);
		char msg[2048] = { 0 };
		const int e = vsnprintf(msg, sizeof(msg), fmt, args);
		if (e > 0) {
			char errorno_string[1024] = { 0 };
			PrintLn(level, "%s: errno(%d): %s", msg ? msg : "", the_errno, strerror_r(the_errno, errorno_string, sizeof(errorno_string)));
		}
		va_end(args);
		return e;
	}

	int PrintLnCurrentErrno(const Level level, const char* fmt, ...) {
		if (level > g_level) return 0;
		const int original_errno = errno;
		va_list args;
		va_start(args, fmt);
		char msg[2048] = { 0 };
		const int e = vsnprintf(msg, sizeof(msg), fmt, args);
		if (e > 0) {
			char errorno_string[1024] = { 0 };
			PrintLn(level, "%s: errno(%d): %s", msg ? msg : "", original_errno, strerror_r(original_errno, errorno_string, sizeof(errorno_string)));
		}
		va_end(args);
		return e;
	}
}
