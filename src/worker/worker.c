#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <DPA/worker/utils.h>
#include <DPA/worker/worker.h>

#if !defined( _POSIX_THREAD_PROCESS_SHARED ) || _POSIX_THREAD_PROCESS_SHARED == -1
#error "_POSIX_THREAD_PROCESS_SHARED not supported"
#endif

typedef struct task_t {
  void(*func)(void*);
  void* ptr;
} task_t;

typedef struct worker_t {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  size_t index;
  size_t size;
  task_t task[WORKER_QUEUE_COUNT];
} worker_t;

SHARED_ARRAY_GLOBAL( static worker_t, worker, WORKER_COUNT );

worker_t* current_worker = 0;
size_t current_worker_id = 0;

typedef void(*entry_func_t)( int argc, char* argv[] );

extern entry_func_t __start_entry_list[];
extern entry_func_t __stop_entry_list[];

int main( int argc, char* argv[] ){
  if( !sysconf( _POSIX_THREAD_PROCESS_SHARED ) ){
    perror("_POSIX_THREAD_PROCESS_SHARED not supported");
    return 1;
  }
  for( worker_t* it=(*worker),*end=(*worker)+WORKER_COUNT; it<end && !current_worker; it++ ){

    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init( &m_attr );
    pthread_mutexattr_setpshared( &m_attr, PTHREAD_PROCESS_SHARED );
    pthread_mutex_init( &it->lock, &m_attr );
    pthread_mutexattr_destroy( &m_attr );

    pthread_condattr_t cond_attr;
    pthread_condattr_init( &cond_attr );
    pthread_condattr_setpshared( &cond_attr, PTHREAD_PROCESS_SHARED );
    pthread_cond_init( &it->cond, &cond_attr );
    pthread_condattr_destroy( &cond_attr );

    if( !fork() )
      current_worker = it;

  }
  if(!current_worker){
    for( entry_func_t* it = __start_entry_list, *end = __stop_entry_list; it<end; it++ ){
      if( !fork() ){
        (**it)( argc, argv );
        return 0;
      }
    }
    while(!( wait(0) == -1 && errno == ECHILD ));
    return 0;
  }
  current_worker_id = current_worker - (*worker) + 1;
  while(1){
    pthread_mutex_lock( &current_worker->lock );
    while( !current_worker->size )
      pthread_cond_wait( &current_worker->cond, &current_worker->lock );
    task_t task = *( current_worker->task + current_worker->index );
    current_worker->index++;
    if( current_worker->index >= WORKER_QUEUE_COUNT )
      current_worker->index = 0;
    current_worker->size--;
    pthread_mutex_unlock( &current_worker->lock );
    (*task.func)( task.ptr );
  }
  pthread_mutex_destroy( &current_worker->lock );
  pthread_cond_destroy( &current_worker->cond );
  return 0;
}

static worker_t* getWorker( size_t id ){
  if( !id ){
    worker_t* w = *worker;
    for( worker_t* it=(*worker)+1,*end=(*worker)+WORKER_COUNT; it<end && w->size; it++ )
      if( it->size < w->size )
        w = it;
    return w;
  }else{
    if( id > WORKER_COUNT )
      return 0;
    return (*worker) + id - 1;
  }
}

bool worker_queue_task( size_t id, void(*func)(void*), void* ptr ){
  worker_t* w = getWorker( id );
  if(!w) return false;
  pthread_mutex_lock( &w->lock );
  if( w->size >= WORKER_QUEUE_COUNT )
    goto error;
  task_t* task = w->task + ( ( w->index + w->size++ ) % WORKER_QUEUE_COUNT );
  task->func = func;
  task->ptr = ptr;
  pthread_cond_broadcast( &current_worker->cond );
  pthread_mutex_unlock( &w->lock );
  return true;
 error:
  pthread_mutex_unlock( &w->lock );
  return false;
}
