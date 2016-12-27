#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <DPA/worker/utils.h>

static char* init_shared_data_mem = 0;

__attribute__((constructor,used))
static void init_shared_data(){
  size_t size = __stop_shared_data - __start_shared_data;
  if(!size)
    return;
  init_shared_data_mem = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if( !init_shared_data_mem ){
    errno_assert(errno);
    exit(128); // never reached
  }
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
  errno_print( munmap( init_shared_data_mem, size ) );
}
 
void m_errno_print( int err, const char* str ){
  if(!err)
    return;
  fprintf( stderr, "%s: error %d: %s\n", str, err, strerror(err) );
}

void m_errno_assert( int err, const char* str ){
  if( !err )
    return;
  m_errno_print( err, str );
  exit(-1);
}
