#ifndef DPA_WORKER_POOL
#define DPA_WORKER_POOL

#include <stddef.h>
#include <stdbool.h>

#ifndef WORKER_COUNT
#define WORKER_COUNT 64
#endif

#ifndef WORKER_QUEUE_COUNT
#define WORKER_QUEUE_COUNT 1024
#endif

bool worker_queue_task( size_t, void(*)(void*), void* );

#endif
