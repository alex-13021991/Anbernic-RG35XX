#ifndef SCREENSHOT_DAEMON_H
#define SCREENSHOT_DAEMON_H

/* Returns 1 for active, 0 for stopped and -1 on inspection failure. */
int daemon_status(void);
int daemon_start(void);
int daemon_stop(void);
#endif
