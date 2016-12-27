#ifndef DPA_UTILS_H
#define DPA_UTILS_H

#include <stddef.h>

#define SHARED_GLOBAL(TYPE,NAME) \
  TYPE __attribute__((section("shared_data"))) __shared_ ## NAME; \
  TYPE* __attribute__((section("shared_data_ptr"),used)) NAME = &__shared_ ## NAME

#define SHARED_GLOBAL_INIT(TYPE,NAME,VALUE) \
  TYPE __attribute__((section("shared_data"))) __shared_ ## NAME = (VALUE); \
  TYPE* __attribute__((section("shared_data_ptr"),used)) NAME = &__shared_ ## NAME

#define SHARED_ARRAY_GLOBAL(TYPE,NAME,N) \
  TYPE __attribute__((section("shared_data"))) __shared_ ## NAME[N]; \
  TYPE* NAME __attribute__((section("shared_data_ptr"),used)) = __shared_ ## NAME

#define SHARED_ARRAY_GLOBAL_INIT(TYPE,NAME,VALUE) \
  TYPE __attribute__((section("shared_data"))) __shared_ ## NAME[] = { UNPACK VALUE }; \
  TYPE* NAME __attribute__((section("shared_data_ptr"),used)) = __shared_ ## NAME

#define UNPACK(...) __VA_ARGS__

#define STR(X) #X
#define ESTR(X) STR(X)

#define CONCAT(A,B) A ## B
#define CONCAT_EVAL(A,B) CONCAT(A,B)

#define ENTRY static void CONCAT_EVAL(___entry_,__LINE__)( int argc, char* argv[] ); \
  static void(*CONCAT_EVAL(__entry_,__LINE__))( int argc, char* argv[] ) \
  __attribute__((section("entry_list"),used)) = &CONCAT_EVAL(___entry_,__LINE__); \
  static void CONCAT_EVAL(___entry_,__LINE__)

#define errno_assert( X ) m_errno_assert(X,__FILE__ ":" ESTR(__LINE__))
#define errno_print( X ) m_errno_print(X,__FILE__ ":" ESTR(__LINE__))

extern const char __attribute__((weak)) __start_shared_data[];
extern const char __attribute__((weak)) __stop_shared_data[];

void m_errno_assert( int, const char* );
void m_errno_print( int, const char* );

#endif
