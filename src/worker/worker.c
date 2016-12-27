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
#include <semaphore.h>

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
  int pid;
  task_t task[WORKER_QUEUE_COUNT];
} worker_t;

typedef struct controller_t {
  pthread_mutex_t lock;
  size_t active_entries;
  bool exit;
} controller_t;

SHARED_ARRAY_GLOBAL( static worker_t, worker, WORKER_COUNT );
SHARED_GLOBAL( static struct controller_t, controller );

static worker_t* current_worker = 0;
size_t current_worker_id = 0;

typedef void(*entry_func_t)( int argc, char* argv[] );

extern entry_func_t __start_entry_list[];
extern entry_func_t __stop_entry_list[];

static bool worker_spawn( void ){
  for( worker_t* it=worker,*end=worker+WORKER_COUNT; it<end && !current_worker; it++ ){

    pthread_mutexattr_t m_attr;
    errno_assert( pthread_mutexattr_init( &m_attr ) );
    errno_assert( pthread_mutexattr_setpshared( &m_attr, PTHREAD_PROCESS_SHARED ) );
    errno_assert( pthread_mutex_init( &it->lock, &m_attr ) );
    errno_assert( pthread_mutexattr_destroy( &m_attr ) );

    pthread_condattr_t cond_attr;
    errno_assert( pthread_condattr_init( &cond_attr ) );
    errno_assert( pthread_condattr_setpshared( &cond_attr, PTHREAD_PROCESS_SHARED ) );
    errno_assert( pthread_cond_init( &it->cond, &cond_attr ) );
    errno_assert( pthread_condattr_destroy( &cond_attr ) );

    int ret = fork();
    if( !ret ){
      current_worker = it;
    }else if( ret != -1 ){
      it->pid = ret;
    }else{
      errno_assert( errno );
    }

  }
  
  return !!current_worker;
}

static void worker_run( void ){
  current_worker_id = current_worker - worker + 1;
  while(1){
    errno_assert( pthread_mutex_lock( &current_worker->lock ) );
    while( !current_worker->size ){
      if( controller->exit ){
        errno_assert( pthread_mutex_unlock( &current_worker->lock ) );
        return;
      }
      errno_assert( pthread_cond_wait( &current_worker->cond, &current_worker->lock ) );
    }
    task_t task = *( current_worker->task + current_worker->index );
    current_worker->index++;
    if( current_worker->index >= WORKER_QUEUE_COUNT )
      current_worker->index = 0;
    current_worker->size--;
    errno_assert( pthread_mutex_unlock( &current_worker->lock ) );
    (*task.func)( task.ptr );
  }
}

static void worker_destroy(){
  errno_assert( pthread_mutex_destroy( &current_worker->lock ) );
  errno_assert( pthread_cond_destroy( &current_worker->cond ) );
}

static entry_func_t entry_points_spawn(){
  for( entry_func_t* it = __start_entry_list, *end = __stop_entry_list; it<end; it++ ){
    int ret = fork();
    if( !ret ){
      return *it;
    }else if( ret == -1 ){
      errno_assert( errno );
    }
  }
  return 0;
}

static void wait_childs( void ){
  while(!( wait(0) == -1 && errno == ECHILD ));
}

static void wakup_workers(){
  for( worker_t* it=worker,*end=worker+WORKER_COUNT; it<end; it++ ){
    errno_assert( pthread_mutex_lock( &it->lock ) );
    errno_assert( pthread_cond_broadcast( &it->cond ) );
    errno_assert( pthread_mutex_unlock( &it->lock ) );
  }
}

static void init( void ){
  pthread_mutexattr_t m_attr;
  errno_assert( pthread_mutexattr_init( &m_attr ) );
  errno_assert( pthread_mutexattr_setpshared( &m_attr, PTHREAD_PROCESS_SHARED ) );
  errno_assert( pthread_mutex_init( &controller->lock, &m_attr ) );
  errno_assert( pthread_mutexattr_destroy( &m_attr ) );
}

static void cleanup( void ){
  errno_assert( pthread_mutex_destroy( &controller->lock ) );
}

int main( int argc, char* argv[] ){
  if( !sysconf( _POSIX_THREAD_PROCESS_SHARED ) ){
    perror("_POSIX_THREAD_PROCESS_SHARED not supported");
    return 1;
  }

  init();

  if( worker_spawn() ){
    worker_run();
    worker_destroy();
    return 0;
  }

  entry_func_t entry_point = entry_points_spawn();
  if( entry_point ){
    errno_assert( pthread_mutex_lock( &controller->lock ) );
    controller->active_entries++;
    errno_assert( pthread_mutex_unlock( &controller->lock ) );
    (*entry_point)( argc, argv );
    errno_assert( pthread_mutex_lock( &controller->lock ) );
    if( !--controller->active_entries ){
      controller->exit = true;
      wakup_workers();
    }
    errno_assert( pthread_mutex_unlock( &controller->lock ) );
    return 0;
  }

  wait_childs();

  cleanup();

  return 0;
}

static worker_t* getWorker( size_t id ){
  if( !id ){
    worker_t* w = worker;
    for( worker_t* it=worker+1,*end=worker+WORKER_COUNT; it<end && w->size; it++ )
      if( it->size < w->size )
        w = it;
    return w;
  }else{
    if( id > WORKER_COUNT )
      return 0;
    return worker + id - 1;
  }
}

bool worker_queue_task( size_t id, void(*func)(void*), void* ptr ){
  worker_t* w = getWorker( id );
  if(!w) return false;
  errno_assert( pthread_mutex_lock( &w->lock ) );
  if( w->size >= WORKER_QUEUE_COUNT )
    goto error;
  task_t* task = w->task + ( ( w->index + w->size++ ) % WORKER_QUEUE_COUNT );
  task->func = func;
  task->ptr = ptr;
  errno_assert( pthread_cond_broadcast( &current_worker->cond ) );
  errno_assert( pthread_mutex_unlock( &w->lock ) );
  return true;
 error:
  errno_assert( pthread_mutex_unlock( &w->lock ) );
  return false;
}
