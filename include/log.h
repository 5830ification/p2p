#ifndef LOG_H
#define LOG_H

typedef enum {
	LINFO,
	LWARN,
	LERROR
} loglevel_t;

void log(loglevel_t lvl, const char *msg, ...);

#endif
