#include <stdio.h>
#include <stdarg.h>

static const char* levelToString = {
	[DEBUG] = "DEBUG",
	[INFO] = "INFO",
	[WARNING] = "WARNING",
	[ERROR] = "ERROR",
	[INTERNAL_ERROR] = "INTERNAL ERROR",
	[FATAL] = "FATAL"
};

// We log to stdout as some devices don't have stderr
void logStandardOutput(const char* file, const char* line, const char* func, int level, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	printf("[%s] %s:%s %s ", levelToString[level], file, line, func);
	vprintf(format, ap);
	printf("\n");
	va_end(ap);
}

void logStandardOutput(const char* file, const char* line, const char* func, int level, FILE* dest, const char* format, ...) {
	if (!file) 
		return;
}
