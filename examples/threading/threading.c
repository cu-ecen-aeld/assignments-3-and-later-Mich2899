#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int ret;								 //to check the return value for different operations

    ret = usleep((thread_func_args->wait_to_obtain_ms)*1000);			 //wait for required msec to obtain the mutex
    if(ret==0){thread_func_args->thread_complete_success = true;}		 //check conditions if usleep succeeded
    else{thread_func_args->thread_complete_success = false;}


    ret = pthread_mutex_lock(thread_func_args->mutex);				 //lock the mutex
    if(ret==0){thread_func_args->thread_complete_success = true;}                //check conditions if pthread_mutex_lock succeeded
    else{thread_func_args->thread_complete_success = false;}
    
    ret = usleep((thread_func_args->wait_to_release_ms)*1000);		 	 //wait for required msec to release the mutex
    if(ret==0){thread_func_args->thread_complete_success = true;}                //check conditions if usleep succeeded
    else{thread_func_args->thread_complete_success = false;}


    ret = pthread_mutex_unlock(thread_func_args->mutex);			 //unlock the mutex
    if(ret==0){thread_func_args->thread_complete_success = true;}                //check conditions if pthread_mutex_unlock succeeded
    else{thread_func_args->thread_complete_success = false;}


    return thread_param;						 	 //return the pointer to data structure so it can be freed 
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     
     struct thread_data *thread_data_instance= malloc(sizeof(struct thread_data*)); 				//allocate memory 
     if(thread_data_instance == NULL){ERROR_LOG("No more space available on heap\n");return false;}		//if the pointer allocated is null return false

     thread_data_instance->mutex = mutex;									//assign mutex to the structure
     thread_data_instance->wait_to_obtain_ms = wait_to_obtain_ms;						//assign the obtain wait time to the structure
     thread_data_instance->wait_to_release_ms = wait_to_release_ms;						//assign teh release wait time to the structure

     int rc = pthread_create(thread, NULL, threadfunc, (void *)thread_data_instance);				//create the thread and check the return value
     if(rc==0){return true;}											//if thread created successfully return true
     else{return false;}											//else return false

    return false;
}

