#include "log.h"

#include <cstdio>
#include <cstdarg>

static loglevel_t G_LOGLEVEL = LINFO;

static const char *lltos(loglevel_t ll) {
	switch (ll) {
		case LINFO: return "INFO";
		case LWARN: return "WARN";
		case LERROR: return "ERROR";
		default: return "<Unkown>";
	}
}

void log(loglevel_t lvl, const char *msg, ...) {
	va_list args;
	va_start(args, msg);

	if (lvl >= G_LOGLEVEL) {
		printf("[%s] ", lltos(lvl));
		vprintf(msg, args);
	}

	va_end(args);
}
