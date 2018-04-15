#include "log.h"

#include <cstdio>
#include <cstdarg>

static loglevel_t G_LOGLEVEL = LINFO;

void log(loglevel_t lvl, const char *msg, ...) {
	va_list args;
	va_start(args, msg);

	if (lvl >= G_LOGLEVEL) {
		vprintf(msg, args);
	}

	va_end(args);
}
