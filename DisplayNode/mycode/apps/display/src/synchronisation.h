#ifndef SYNCHRONISATION_H
#define SYNCHRONISATION_H

#include <zephyr/kernel.h>

#define SYNCHRONISATION_THREAD_STACK_SIZE 1024
#define SYNCRHONISATION_THREAD_PRIORITY 5

extern volatile uint32_t tick;
extern struct k_event tick_event;

void heartbeat_tick(uint32_t tick);

#endif
