// #include <math.h>
#include "synchronisation.h"

#define WINDOW_SIZE 8
static uint32_t x[WINDOW_SIZE] = {0};
static uint32_t y[WINDOW_SIZE] = {0};
static size_t i = 0;

volatile uint32_t tick;

K_EVENT_DEFINE(tick_event);

K_MUTEX_DEFINE(sync_mutex);

// static void sync_thread(void *, void *, void *);

// K_THREAD_DEFINE(sync_tid, SYNCHRONISATION_THREAD_STACK_SIZE, sync_thread, NULL, NULL, NULL,
//         SYNCRHONISATION_THREAD_PRIORITY, 0, 0);

static int linear_regression(size_t n, uint32_t x[], uint32_t y[], float *m, float *c)
{
    float sumx  = 0.0f;  // sum of x
    float sumx2 = 0.0f;  // sum of x**2
    float sumxy = 0.0f;  // sum of x * y
    float sumy  = 0.0f;  // sum of y
    float sumy2 = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        sumx  += x[i];       
        sumx2 += x[i] * x[i];  
        sumxy += x[i] * y[i];
        sumy  += y[i];      
        sumy2 += y[i] * y[i];
    }

    float denom = n * sumx2 - sumx * sumx;
    if (denom == 0.f) {
        *m = 0.f;
        *c = 0.f;
        return 1;
    }

    *m = (n * sumxy - sumx * sumy) / denom;
    *c = (sumy * sumx2 - sumx * sumxy) / denom;

    return 0;
}

void heartbeat_tick(uint32_t tick)
{
    k_mutex_lock(&sync_mutex, K_FOREVER);
    x[i] = tick;
    y[i] = k_uptime_get_32();
    i = (i + 1) % WINDOW_SIZE;
    k_mutex_unlock(&sync_mutex);
}

// static void sync_thread(void *, void *, void *)
// {
//     float m, c;
//     int ret;

//     for (;;) {
//         k_mutex_lock(&sync_mutex, K_FOREVER);
//         ret = linear_regression(WINDOW_SIZE, x, y, &m, &c);
//         k_mutex_unlock(&sync_mutex);
//         if (ret != 0) {
//             k_msleep(1000);
//         } else {
//             uint32_t next_time = m * (tick + 1) + c;
//             int32_t time_diff = next_time - k_uptime_get_32();
//             if (time_diff < 500) {
//                 time_diff = 500;
//             }
//             if (time_diff > 1500) {
//                 time_diff = 1500;
//             }
//             k_msleep(time_diff);
//         }

//         ++tick;
//         printk("POSTING TICK ========================================\n");
//         k_event_post(&tick_event, 1);
//     }
// }
