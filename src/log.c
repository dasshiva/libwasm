#include <stdio.h>
#include <stdarg.h>

// We log to stdout as some devices don't have stderr
void logStandardOutput(const char* file, const char* line, const char* func, int level, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
}

void logStandardOutput(const char* file, const char* line, const char* func, int level, FILE* dest, const char* format, ...) {
	if (!file) 
		return;
}
