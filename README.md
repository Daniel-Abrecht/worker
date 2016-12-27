# DPA-Worker Library

This library allows, amongst other things, the definition of as many
program entry points as you want, and creates some worker processes
to which you can assign tasks. This Library is intended to be used with
unix like systems and gcc. It shouldn't be used with shared libraries. 

## Functions & Macros
### worker.h
| Function / Macro | Description |
|------------------|-------------|
| WORKER_COUNT     | specifies how many workers will be spawned |
| WORKER_QUEUE_COUNT | specifies how many tasks a worker queue can hold |
| ```ENTRY( int argc, char* argv[] ){}``` | Defines an new Program Entry Point |
| ```bool worker_queue_task( size_t workerid, void(*task)(void*), void* ptr )``` | Adds a task to a worker. If workerid is zero, the worker with the least pending tasks is chosen. Returns true un success and false on error. |
| ```size_t current_worker_id``` | ID of worker process, 0 if process isn't a worker |

### utils.h
| Function / Macro | Description |
|------------------|-------------|
| SHARED_GLOBAL(TYPE,NAME) | Defines a pointer to a variable which will be relocated into shared memory during startup. |
| SHARED_GLOBAL_INIT(TYPE,NAME,VALUE) | Defines a pointer to a variable which will be copied into shared memory during startup and is initialized with VALUE. |
| SHARED_ARRAY_GLOBAL(TYPE,NAME,COUNT) | Defines a pointer to the first entry of an array which will be copied into shared memory during startup. |
| SHARED_ARRAY_GLOBAL_INIT(TYPE,NAME,VALUE) | Defines a pointer to the first entry of an array which will be copied into shared memory during startup and is initialized with VALUE. |
| errno_assert( int errno ) | Prints an error message if errno != 0 and exits the current process if an error occurs |
| errno_print( int errno ) | Prints an error message if errno != 0 |

## Example

```c
#include <stdio.h>
#include <DPA/worker/worker.h>

void test( void* ptr ){
  puts(ptr);
}

ENTRY( int argc, char* argv[] ){
  (void)argc;
  (void)argv;
  printf("test\n");
  worker_queue_task(0,test,"test2");
  printf("test3\n");
}

ENTRY( int argc, char* argv[] ){
  (void)argc;
  (void)argv;
  printf("test4\n");
}
```

## How to build
```bash
git clone https://github.com/Daniel-Abrecht/worker.git
cd worker
mkdir bin
make
```

## How to use
```bash
gcc -Wl,--whole-archive /path/to/worker/bin/worker.a -Wl,--no-whole-archive -pthread -I /path/to/worker/src/header/ your_source.c -o program
```
