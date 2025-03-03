#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

enum {
	DEBUG,
	INFO,
	WARNING,
	ERROR,
};

#ifdef YDEBUG

#define info(...) logStdout(__FILE__, __LINE__, __func__,  INFO, 1, __VA_ARGS__)
#define warn(...) logStdout(__FILE__, __LINE__, __func__,  WARNING, 1, __VA_ARGS__)
#define error(...) logStdout(__FILE__, __LINE__, __func__, ERROR, 1, __VA_ARGS__)
#define debug(...) logStdout(__FILE__, __LINE__, __func__, DEBUG, 1, __VA_ARGS__)

#else

#define debug(...)   
#define info(...) 
#define warn(...) logStdout(NULL, 0, NULL, WARNING, 0, __VA_ARGS__)
#define error(...) logStdout(NULL, 0, NULL, ERROR, 0, __VA_ARGS__)
#endif

#ifdef SUPPRESS_ALL_MESSAGES
#undef debug
#undef warn
#undef error
#undef info
#endif
// Both of these must not be called directly, use the macros
void logStdout(const char* file, int line, const char* func, int level, int trace, const char* format, ...);

void logToFile(const char* file, int line, const char* func, int level, int trace, FILE* dest, const char* format, ...);

#endif
