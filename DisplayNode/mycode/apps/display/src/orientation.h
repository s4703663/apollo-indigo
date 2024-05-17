#ifndef ORIENTATION_H
#define ORIENTATION_H

#ifdef CONFIG_SENSOR

#include "orientation_enum.h"

#define ORIENTATION_THREAD_STACK_SIZE 2048
#define ORIENTATION_THREAD_PRIORITY 5

extern const k_tid_t orientation_tid;

int orientation_init(void);

enum Orientation orientation_get(void);

#endif

#endif
