#include <log.h>
#include <stdarg.h>

static const char* levelToString[] = {
	[DEBUG] = "DEBUG",
	[INFO] = "INFO",
	[WARNING] = "WARNING",
	[ERROR] = "ERROR"
};

// We log to stdout as some devices don't have stderr
void logStdout(const char* file, const char* line, const char* func, int level, int trace, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	if (trace)
		printf("[%s] %s:%s %s ", levelToString[level], file, line, func);
	else 
		printf("%s: ", levelToString[level]);
	vprintf(format, ap);
	printf("\n");
	va_end(ap);
}

void logToFile(const char* file, const char* line, const char* func, int level, int trace, FILE* dest, const char* format, ...) {
	if (!file) 
		return;

	va_list ap;                                                  
	va_start(ap, format);
	if (trace)
        	fprintf(dest, "[%s] %s:%s %s ", levelToString[level], file, line, func);
	else
		fprintf(dest, "%s: ", levelToString[level]);
	vfprintf(dest, format, ap);
	fprintf(dest, "\n");
        va_end(ap);
}
