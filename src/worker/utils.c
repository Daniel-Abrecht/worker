#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <DPA/worker/utils.h>

static char* init_shared_data_mem = 0;

__attribute__((constructor,used))
static void init_shared_data(){
  size_t size = __stop_shared_data - __start_shared_data;
  if(!size)
    return;
  init_shared_data_mem = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if( !init_shared_data_mem )
    exit(128);
  extern char* __attribute__((weak)) __start_shared_data_ptr[];
  extern char* __attribute__((weak)) __stop_shared_data_ptr[];
  memcpy( init_shared_data_mem, __start_shared_data, size );
  for( char** it=__start_shared_data_ptr; it<__stop_shared_data_ptr; it++ )
    *it += init_shared_data_mem - __start_shared_data;
}

__attribute__((destructor,used))
static void cleanup_shared_data(){
  if(!init_shared_data_mem)
    return;
  size_t size = __stop_shared_data - __start_shared_data;
  if(!size)
    return;
  munmap( init_shared_data_mem, size );
}
