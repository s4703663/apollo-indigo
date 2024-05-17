#ifndef DISTANCE_H
#define DISTANCE_H

#ifdef CONFIG_SENSOR

#define DISTANCE_THREAD_STACK_SIZE 2048
#define DISTANCE_THREAD_PRIORITY 5

extern const k_tid_t distance_tid;

int distance_init(void);

float distance_get(void);

#endif

#endif
