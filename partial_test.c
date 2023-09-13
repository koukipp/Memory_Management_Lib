#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "lib/memory_manager.h"

#define NUM_OF_KERNEL_THREADS 2
#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void *k_wrapper(void* arg);
void kernel_exit(void);
void init(void);


pthread_t kernel_level_ids[NUM_OF_KERNEL_THREADS]={(pthread_t)0};
int ids[NUM_OF_KERNEL_THREADS]={0};
/*Partial block allocation showcase.Thread 1 will adopt the 
pageblock from thread 2 and will malloc 16 bytes(internally) ptr2-ptr1 = 16
*/
void *k_wrapper(void* arg){
    void *ptr1;

    ptr1 = umalloc(sizeof(void *));
    printf("ptr1:%p\n",ptr1);

    ////////////////
    return(NULL);
    //exit(EXIT_SUCCESS); /* Terminate all threads */ 
}



void k_lib_init(){
    int s;
    for (int i =0;i<NUM_OF_KERNEL_THREADS-1;i++){                                   
        ids[i] = i;
        s = pthread_create(&kernel_level_ids[i], NULL,(void*)&k_wrapper,(void *)&ids[i]);
        if (s != 0) handle_error_en(s, "pthread_create");
    }


}


void kernel_exit(){
    for (int i =0;i<NUM_OF_KERNEL_THREADS-1;i++){ 
        printf("OK\n");
        pthread_join(kernel_level_ids[i], NULL);
    }
}


int main(int argc, char * argv[]){
    void *new_ptr;
    k_lib_init();
    kernel_exit();
    //printf("shared %p\n",shared_ptr);
    new_ptr = umalloc(sizeof(void *));
    ufree(new_ptr);
    //memset(shared_ptr,1,2032);
    printf("ptr2: %p\n",new_ptr);
    return 0;
}


