/********************************************************************
 * File: instrument.c
 *
 * Instrumentation source -- link this with your application, and
 *  then execute to build trace data file (trace.txt).
 *
 * Author: M. Tim Jones <mtj@mtjones.com>
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
//#include <pthread.h>


/* Function prototypes with attributes */
void main_constructor( void )
	__attribute__ ((no_instrument_function, constructor));

void main_destructor( void )
	__attribute__ ((no_instrument_function, destructor));

void __cyg_profile_func_enter( void *, void * ) 
	__attribute__ ((no_instrument_function));

void __cyg_profile_func_exit( void *, void * )
	__attribute__ ((no_instrument_function));
#ifdef __cplusplus
}
#endif

//static pthread_mutex_t _lock_ = PTHREAD_MUTEX_INITIALIZER;
static FILE *fp;

void main_constructor( void )
{
  //pthread_mutex_init (&_lock_, NULL);	
  fp = fopen( "trace.txt", "w" );
  if (fp == NULL) exit(-1);
}


void main_deconstructor( void )
{
  fclose( fp );
  //pthread_mutex_destroy(&_lock_);
}


void __cyg_profile_func_enter( void *this_fn, void *callsite )
{
  //pthread_mutex_lock(&_lock_);
  fprintf(fp, "E%p\n", (int *)this_fn);
  //pthread_mutex_unlock(&_lock_);
}


void __cyg_profile_func_exit( void *this_fn, void *callsite )
{
  //pthread_mutex_lock(&_lock_);
  fprintf(fp, "X%p\n", (int *)this_fn);
  //pthread_mutex_unlock(&_lock_);
}


