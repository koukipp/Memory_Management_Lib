#ifndef MEM_MANAGER_INT_H
#define MEM_MANAGER_INT_H

#include "list.h"
#include "atomic.h"

#define MIN_OBJECT_SIZE 16
#define CLASS_GRANULARITY 16
#define SMALL_OBJECT 2048
#define NUM_OF_CLASSES SMALL_OBJECT/CLASS_GRANULARITY
#define PAGE_SIZE 4096
#define MIN_PAGE_BLOCK 4*PAGE_SIZE
#define MAX_PAGE_BLOCK 64*PAGE_SIZE
#define NUM_OF_OBJECTS 1024
#define MAX_SHARED 2048
#define CACHE_SIZE 5
#define MAX_LOCAL 4
#define MAX_GLOBAL 4
#define MAX_PAGES 64
typedef list_t heap_list_t;

/*Object offset from start of pageblock
ranges from 0 to 256KB thus 18 bits suffice.*/
typedef struct stack_head{
    volatile unsigned long address:18;
    volatile unsigned long count:16;
    volatile unsigned long id:30;
}stack_head_t;

/*48 bits used for virtual*/
/*address in x86-64 architectures*/
typedef struct stack_head_c{
    volatile unsigned long address:48;
    volatile unsigned long count:16;
}stack_head_c_t;

typedef struct lf_cache{
    stack_head_c_t stack;
    atomic_t size;
}lf_cache_t;

typedef struct pageblock_struct{
    void *self; 
    unsigned long magic;
    struct pageblock_struct *next,*prev; // overlaps with cache next
    //This is included inside remotely_freed (47-55 bits).
    //unsigned int tid;
    union{
        stack_head_t remotely_freed;
        void *head;
    };
    unsigned long int padding;
    void *freed,*unallocated;
    unsigned long int class_size;
    unsigned long int used_objects;
    void *next_page;
    void *boundary;
    
}pageblock_t;
#endif
