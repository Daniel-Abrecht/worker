#include <stdio.h>
#include <DPA/worker/utils.h>
#include <DPA/worker/worker.h>

void test( void* ptr ){
  (void)ptr;
  printf("test2\n");
}

ENTRY( int argc, char* argv[] ){
  (void)argc;
  (void)argv;
  printf("test\n");
  worker_queue_task(0,test,0);
  printf("test3\n");
}

ENTRY( int argc, char* argv[] ){
  (void)argc;
  (void)argv;
  printf("test4\n");
}
