#ifndef LOG_H
#define LOG_H

typedef enum {
	LDBG,
	LINFO,
	LWARN,
	LERROR
} loglevel_t;

void log(loglevel_t lvl, const char *msg, ...);

#endif
