#pragma once

/**
 * port_log.h — Unified crash-safe logging for the SSB64 PC port.
 *
 * All port logging goes through port_log(). Output is written to a single
 * file (ssb64.log) with immediate fflush after every write, so nothing is
 * lost on crash. Call port_log_init() once at the very start of main(),
 * before any LUS initialization (which redirects stderr).
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Open the log file. Call once, before anything else. */
void port_log_init(const char *path);

/* Close the log file. Call at shutdown. */
void port_log_close(void);

/* Return the raw file descriptor of the log file, or -1 if not open.
 * Used by async-signal-safe crash handlers that cannot call fprintf. */
int port_log_get_fd(void);

/* Write a formatted message to the log file. */
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
void port_log(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
