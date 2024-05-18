#ifndef DISTANCE_H
#define DISTANCE_H

#ifdef CONFIG_SENSOR

#define DISTANCE_THREAD_STACK_SIZE 2048
// Important that this has a higher priority than other threads
// Since the timing of the hc-sr04 driver depends on k_sleep()
#define DISTANCE_THREAD_PRIORITY 4

extern const k_tid_t distance_tid;

int distance_init(void);

float distance_get(void);

#endif

#endif
