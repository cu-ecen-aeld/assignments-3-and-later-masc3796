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
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    //As suggested above...
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    ///PThread Reference: Linux System Programming, pgs 226-239
    
    //thread_complete_success will get set false if mutex lock/unlock fails
	thread_func_args->thread_complete_success = true;    
	
    //Wait
	usleep((thread_func_args->wait_to_obtain_ms)*1000);
    
    //Obtain Mutex
	int test_lock;
    test_lock = pthread_mutex_lock(thread_func_args->mutex);
    DEBUG_LOG("test_lock: %d\n",test_lock);
    
    if (test_lock != 0)  //Failed to lock Mutex, success=False
    {
	    thread_func_args->thread_complete_success = false;
	    ERROR_LOG("Failed to lock mutex\n");
    }
    
    else //Successfully obtained Mutex... proceed
    {
		//Wait
		usleep((thread_func_args->wait_to_release_ms)*1000);   

	    //Release Mutex    
    	int test_unlock;
	    test_unlock = pthread_mutex_unlock(thread_func_args->mutex);  
        DEBUG_LOG("test_unlock: %d\n",test_unlock);
	    
    	if (test_unlock != 0)  //Failed to unlock Mutex, success=False
    	{
    		thread_func_args->thread_complete_success = false;
		    ERROR_LOG("Failed to unlock mutex\n");
    	}
    
    }
   
    return thread_param;
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

	bool success;
	success = true;
	
	//allocate memory for thread data
	struct thread_data *my_thread = malloc( sizeof(*my_thread) );
	
	//setup mutex and wait arguments (initialize struct)
	//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	my_thread->thread = thread;
	my_thread->mutex = mutex;
	my_thread->wait_to_obtain_ms = wait_to_obtain_ms;
	my_thread->wait_to_release_ms = wait_to_release_ms;
	
	//Create thread
	int test_create;
	test_create = pthread_create(thread, NULL, threadfunc, my_thread);
	DEBUG_LOG("test_create: %d\n", test_create);
	
	if (test_create != 0)
	{
		success = false;
		ERROR_LOG("Failed to create thread\n");
	}
	

    return success;
}

