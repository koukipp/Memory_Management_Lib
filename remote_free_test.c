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
void* shared_ptr[100];

#define NUM_OF_KERNEL_THREADS 2
#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void *k_wrapper(void* arg);
void kernel_exit(void);
void init(void);

pthread_t kernel_level_ids[NUM_OF_KERNEL_THREADS]={(pthread_t)0};
int ids[NUM_OF_KERNEL_THREADS]={0};

/*Fill some pageblocks.Thread 1 mallocs some memory and puts some
of the pointers in a global array.Thread 2 remotely frees some of the memory.
Thread 1 then mallocs some memory again, and will also use the memory from 
the remotely freed which will add to its freed list in one cmp_and_swap.This 
is guaranteed to happen as we have filled a pageblock later (which also got adopted by thread 2
when it remotely freed objects).*/

void *k_wrapper(void* arg){
    printf("Hello from the other side of thread\n");

    ////////////////
    if(*(int *)arg == 0){
        printf("remote free\n");
        for(int i = 0;i<100;i++){
            ufree(shared_ptr[i]);
        }
        printf("success\n");
    }
    ////////////////
    return(NULL);
    //exit(EXIT_SUCCESS); /* Terminate all threads */ 
}



void k_lib_init(){
    int s;
    for(int i = 0;i<100;i++){
        shared_ptr[i] = (void *)umalloc(sizeof(void *));
    }
    for(int i = 0;i<2048;i++){
        umalloc(sizeof(void *));
    }
    for (int i =0;i<NUM_OF_KERNEL_THREADS-1;i++){                                   
        ids[i] = i;
        s = pthread_create(&kernel_level_ids[i], NULL,&k_wrapper,(void *)&ids[i]);
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
    k_lib_init();
    kernel_exit();
    printf("exit\n");
    for(int i = 0;i<2048;i++){
        umalloc(sizeof(void *));
    }
    printf("OK2\n");
    exit(EXIT_SUCCESS);
}
