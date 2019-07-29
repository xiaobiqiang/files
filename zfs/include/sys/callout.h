#ifndef __SYS_CALLOUT_H
#define __SYS_CALLOUT_H

#include <linux/types.h>

typedef void **timeout_id_t;

extern timeout_id_t
timeout(void (*func)(void *), void *arg, clock_t delta);

extern clock_t
untimeout(timeout_id_t id_arg);
#endif 	/* __SYS_CALLOUT_H */