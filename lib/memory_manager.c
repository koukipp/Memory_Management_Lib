#include <sys/mman.h>
extern "C"
{
  #include "my_math.h"
}
#include <unistd.h>
#include <stdio.h>
#include "memory_manager_int.h"
#include "atomic.h"
#include <string.h>
#include <threads.h>
#include "list.h"
#include <new>
#define __GNU_SOURCE

static atomic_t count;
static thread_local heap_list_t heap_array[NUM_OF_CLASSES];
static thread_local long unsigned int id;
static thread_local heap_list_t local_cache[CACHE_SIZE]; //16kB -> 256 kB was on the paper 2^14 -> 2^18
static thread_local int init_flag;

static lf_cache_t global_cache[CACHE_SIZE];
static stack_head_c_t global_partial_blocks_list[NUM_OF_CLASSES];

#define MIN_EXP 14

typedef struct object{
    struct object *next;
}object_t;

typedef struct cache_node{
    void *padd1,*padd2;
    struct cache_node *next;
}pageblock_cache_node_t;


typedef struct large_object{
    unsigned long size;
    unsigned long magic;
}large_object_t;

//static __thread int flag;
struct thread_data_struct {
	int some_data;

	thread_data_struct() {

        id = atomic_inc_read(&count);
        init_flag=1;

	}

	~thread_data_struct() {
        node *curr,*next;
        stack_head_c_t old_val,new_val;
        for (int i=0;i<NUM_OF_CLASSES;i++){
            if(!is_empty(&heap_array[i])){
                curr = heap_array[i].head;
                int rem_num_pages = ( 1 << (unsigned int) my_ceil(my_log2((double)((NUM_OF_OBJECTS*(i + 1)*CLASS_GRANULARITY)/PAGE_SIZE)) + 0.1));
                int global_class = my_log2(rem_num_pages*PAGE_SIZE) - MIN_EXP;
                if(rem_num_pages < MAX_PAGES){
                    rem_num_pages = MAX_PAGES;
                }
                do{
                    next = curr->next;
                    if(((pageblock_t *)curr)->used_objects){
                        if(((unsigned long)((pageblock_t *)curr)->unallocated)+((unsigned long)((pageblock_t *)curr)->class_size) < (unsigned long)((pageblock_t *)curr)->boundary||
                        ((pageblock_t *)curr)->remotely_freed.address){
                            do{
                                old_val = global_partial_blocks_list[i];
                                ((pageblock_cache_node_t *)curr)->next = (pageblock_cache_node_t *)((unsigned long int)(old_val.address));
                                new_val.address = (unsigned long int)curr;
                                if(old_val.address == new_val.address){
                                    int a;
                                    a++;
                                }
                                /*Set count*/
                                new_val.count = (old_val.count + 1);

                                }while(!compare_and_swap((void **)&(global_partial_blocks_list[i]),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));
                        }
                        else{
                            stack_head_t old_v,new_v;
                            old_v = ((pageblock_t *)curr)->remotely_freed;
                            old_v.id = 0;
                            new_v.id = 0;
                            new_v.address = 0;
                            new_v.count = old_v.count;
                            if(!compare_and_swap((void **)&(((pageblock_t *)curr)->head),(void *)(*((unsigned long *)&old_v)),(void *)(*((unsigned long *)&new_v)))){
                                do{
                                    old_val = global_partial_blocks_list[i];
                                    ((pageblock_cache_node_t *)curr)->next = (pageblock_cache_node_t *)((unsigned long int)(old_val.address));
                                    new_val.address = (unsigned long int)curr;
                                    new_val.count = (old_val.count + 1);
                                    if(old_val.address == new_val.address){
                                        int a;
                                        a++;
                                    }
                                }while(!compare_and_swap((void **)&(global_partial_blocks_list[i]),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));

                            }
                        }
                    }
                    else if(global_cache[global_class].size < MAX_GLOBAL){
                        atomic_add(1,&global_cache[global_class].size);
                        do{
                            old_val = global_cache[global_class].stack;
                            ((pageblock_cache_node_t *)curr)->next = (pageblock_cache_node_t *)((unsigned long int)(old_val.address));
                            new_val.address = (unsigned long int)curr;
                            new_val.count = (old_val.count + 1);
                        }while(!compare_and_swap((void **)&(global_cache[global_class].stack),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));
                    }
                    else{
                        munmap(((pageblock_t *)curr),rem_num_pages*PAGE_SIZE);
                    }
                    curr = next;
                }while(curr != heap_array[i].head);
            }
        }

        for(int i=0;i<CACHE_SIZE;i++){
            if(!is_empty(&local_cache[i])){
                curr = local_cache[i].head;
                while(curr != local_cache[i].head && global_cache[i].size < MAX_GLOBAL){
                    next = curr->next;
                    if(global_cache[i].size < MAX_GLOBAL){
                        atomic_add(1,&global_cache[i].size);
                        do{
                            old_val = global_cache[i].stack;
                            ((pageblock_cache_node_t*)curr)->next = (pageblock_cache_node_t *)((unsigned long int)(old_val.address));
                            new_val.address = (unsigned long int)curr;
                            new_val.count = (old_val.count + 1);

                        }while(!compare_and_swap((void **)&(global_cache[i].stack),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));
                    }
                    else{
                        int block_size = 1 << (i+MIN_EXP);
                        munmap(((pageblock_t *)curr),block_size);
                    }
                    curr = next;
                }
            }
        }
	}
};


void* page_block_alloc(pageblock_t *page_block){
    void* pointer = NULL;
    /*Check freed first*/
    if(page_block->freed != NULL){
        pointer = page_block->freed;
        page_block->freed = ((object_t *)page_block->freed)->next;

        page_block->used_objects +=1;
    }
    else if( ((unsigned long int)page_block->unallocated) + page_block->class_size <= (unsigned long int)page_block->boundary){
        if(((unsigned long int)page_block->unallocated) + page_block->class_size >= (unsigned long int)page_block->next_page){
            /*2*sizeof(void *) = self + magic*/
            page_block->unallocated = (void *)((unsigned long int)page_block->next_page + 2*sizeof(void *));
            page_block->next_page = (void *)((unsigned long int) page_block->next_page + PAGE_SIZE);
        }

        page_block->used_objects+=1;
        pointer = page_block->unallocated;
        page_block->unallocated = (void *)((unsigned long int)page_block->unallocated + page_block->class_size);
    }
    else if(page_block->remotely_freed.address !=0){
        //REMOTE FREE CHAIN
        stack_head_t old_val,new_val;

        do{
            old_val = page_block->remotely_freed;
            //stack_head_t new_val = (global_cache[cache_index].address) ->next;
            new_val.address = (unsigned long)0;
            new_val.count = page_block->remotely_freed.count+1;
            new_val.id = old_val.id;
        }while(!compare_and_swap((void **)&(page_block->head),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));

        if(old_val.address != 0){
            int i=0;
            object_t *curr,*prev_curr;
            curr = (object_t *)((unsigned long)old_val.address+ (unsigned long)page_block->self);
            while(curr!= NULL){
                i++;
                prev_curr = curr;
                curr = curr->next;
            }
            prev_curr->next = (object_t *)page_block->freed;
            page_block->freed = (void *)((unsigned long)old_val.address+(unsigned long)page_block->self);

            page_block->used_objects -= i;
            pointer = page_block->freed;
            page_block->freed = ((object_t *)page_block->freed)->next;

            page_block->used_objects +=1;

        }
    }

    
    return pointer;
}

pageblock_t *alloc_page_block(int index,heap_list_t * heap){
    double res;
    int num_of_pages;
    pageblock_t *page_block;
    
    res = my_log2((double)((NUM_OF_OBJECTS*(index + 1)*CLASS_GRANULARITY)/PAGE_SIZE)); //step1: log([K*(i+1)*step/PAGE_SIZE]), with K = # of objects
    num_of_pages = ( 1 << (unsigned int) my_ceil(res + 0.1)); // step2: 1 << my_ceil(log([K*(i+1)*step/PAGE_SIZE])+ w), with w = 0.1
    if(num_of_pages > MAX_PAGES){
        num_of_pages = MAX_PAGES;
    }
    //from 8kB to 256kB
    int cache_index = my_log2(num_of_pages*PAGE_SIZE) - MIN_EXP;
    if(!is_empty((&local_cache[cache_index]))){
        pageblock_t *locally_cached = (pageblock_t *)list_dequeue (&(local_cache[cache_index]));

        //RESET PAGEBLOCK//
        locally_cached->class_size = (unsigned long int)(index + 1)*CLASS_GRANULARITY;
        locally_cached->freed = NULL;
        locally_cached->unallocated = (void *)((unsigned long int)locally_cached + sizeof(pageblock_t));
        locally_cached->remotely_freed.address = 0;
        locally_cached->remotely_freed.count =  0;
        locally_cached->next_page = (void *)((unsigned long int)locally_cached + (unsigned long int)PAGE_SIZE);
        list_push(&heap_array[index],(node *)locally_cached);
        return(locally_cached);
    }
    else if((global_cache[cache_index].stack.address)!=0){
        stack_head_c_t old_val,new_val;
        /*DEQUEUE FROM GLOBAL CACHE*/
        do{
            old_val = global_cache[cache_index].stack;
            if(old_val.address == 0){
                break;
            }
            new_val.address = (unsigned long)((pageblock_cache_node_t *)(unsigned long)(old_val.address))->next;
            new_val.count = old_val.count+1;
        }while(!compare_and_swap((void**)&(global_cache[cache_index].stack),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));

        /*ENQUEUE TO LOCAL HEAP*/
        if(old_val.address != 0){
            atomic_add(-1,&global_cache[cache_index].size);
            page_block = ((pageblock_t *)(unsigned long)(old_val.address));
            page_block->remotely_freed.id = id;
            page_block->class_size = (unsigned long int)(index + 1)*CLASS_GRANULARITY;
            page_block->freed = NULL;
            page_block->unallocated = (void *)((unsigned long int)page_block + sizeof(pageblock_t));
            page_block->remotely_freed.address = 0;
            page_block->remotely_freed.count =  0;
            page_block->next_page = (void *)((unsigned long int)page_block + (unsigned long int)PAGE_SIZE);
            list_push(&heap_array[index],((node *)(unsigned long)(old_val.address)));
            return((pageblock_t*)(unsigned long)(old_val.address));
        }
    }
    else if((global_partial_blocks_list[index].address)!=0){
        stack_head_c_t old_val,new_val;
        /*DEQUEUE FROM GLOBAL_PARTIAL*/
        do{
            old_val = global_partial_blocks_list[index];
            if(old_val.address == 0){
                break;
            }
            new_val.address = (unsigned long)((pageblock_cache_node_t *)(unsigned long)(old_val.address))->next;
            new_val.count = old_val.count+1;
            if(old_val.address == new_val.address){
                int a;
                a++;
             }
        }while(!compare_and_swap((void**)&(global_partial_blocks_list[index]),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));

        /*ENQUEUE TO LOCAL HEAP*/
        if(old_val.address != 0){
            page_block = ((pageblock_t *)(unsigned long)(old_val.address));
            page_block->remotely_freed.id = id;
            list_push(&heap_array[index],((node *)(unsigned long)(old_val.address)));
            return((pageblock_t*)(unsigned long)(old_val.address));
        }
    }

    /*Aligned to PAGE_SIZE.*/
    page_block = (pageblock_t *)mmap(NULL,num_of_pages*PAGE_SIZE,PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(page_block ==(void *) -1){
        return page_block;
    }
    page_block->self = page_block;
    page_block->freed = NULL;
    page_block->unallocated = (void *)((unsigned long int)page_block + sizeof(pageblock_t));
    page_block->remotely_freed.address = 0;
    page_block->remotely_freed.id = id;
    page_block->remotely_freed.count =  0;
    //Occupancy as a percentage could be 100*(used_objects/((page_block_size/class_size)-(freed.count+remotely_freed.count)))
    page_block->class_size = (unsigned long int)(index + 1)*CLASS_GRANULARITY;
    page_block->used_objects = (unsigned long int)0;
    page_block->boundary = (void *)((unsigned long int)page_block + (unsigned long int)num_of_pages*PAGE_SIZE - 1);
    page_block->next_page = (void *)((unsigned long int)page_block + (unsigned long int)PAGE_SIZE);
    for (int i=1;i<num_of_pages;i++){
       ((void **) page_block)[(i*PAGE_SIZE)/sizeof(void *)] = page_block;
       ((void **) page_block)[((i*PAGE_SIZE)/sizeof(void *))+1] = 0;
    }
    list_push(&heap_array[index],(node *)page_block);
    return(page_block);
}


extern "C" void *umalloc(size_t size){
    int index;
    size_t alloc_size = size;
    void *pointer = NULL;
    pageblock_t *page_block;
    if(!init_flag){
        thread_local static struct thread_data_struct thread_data;
    }
    if(!size){
        //Some code depends that malloc
        //will return something !=0 in case
        //size is 0.The C standard leaves it to the
        //impelementation.May need to uncomment for
        //applications that depend on this behaviour
        //return (void *)1;
        return NULL;
    }

    if(size < MIN_OBJECT_SIZE){
        alloc_size = MIN_OBJECT_SIZE;
    }
    if(size > SMALL_OBJECT){
        large_object_t *mem;
        int num_of_pages;
        num_of_pages = (int)my_ceil(((double)(size+sizeof(large_object_t)))/PAGE_SIZE);
        mem = (large_object_t *)mmap(NULL,num_of_pages*PAGE_SIZE,PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(mem == (void *)-1){
            return NULL;
        }
        mem->size = num_of_pages*PAGE_SIZE;
        mem->magic = 1;

        return((void *)((unsigned long)mem + sizeof(large_object_t)));
    }

    index = (alloc_size - 1)/ CLASS_GRANULARITY; //find the index to its corresponding size class

    if(is_empty(&heap_array[index])){
        while(pointer == NULL){
            page_block = alloc_page_block(index,&heap_array[index]);

            if(page_block == (void *)-1){
                return NULL;
            }
        
            pointer = page_block_alloc(page_block);
        }

    }
    else{
        page_block = (pageblock_t *)(heap_array[index].head);
        for(unsigned int i=0 ;i<heap_array[index].size;i++){
            pointer = page_block_alloc(page_block);
            if(pointer != NULL){
                break;
            }
            else{
                list_move_to_tail(&heap_array[index]);
            }
            page_block = page_block->next;
        }
 
        /*If we dont find a page_block with freed-unallocated memory,
        we allocate a new one and put it in the heap.*/
        while(pointer == NULL){
            //printf("No Freed Mem available, allocating a new chunk..\n");

            page_block = alloc_page_block(index,&heap_array[index]);
            if(page_block == (void *)-1){
                return NULL;
            }
            pointer = page_block_alloc(page_block);
            if(pointer == NULL){
                list_move_to_tail(&heap_array[index]);
            }
            
        }

    }

    return pointer;
}

void free_local(void* ptr,pageblock_t * page_block){
    ((object_t *)ptr)->next = (object_t *)page_block->freed;
    page_block->freed = ptr;
    page_block->used_objects -=1;
    if(page_block->used_objects == 0){
        int rem_idx = ((page_block->class_size)/CLASS_GRANULARITY)-1;
        int rem_num_pages = ( 1 << (unsigned int) my_ceil(my_log2((double)((NUM_OF_OBJECTS*(rem_idx + 1)*CLASS_GRANULARITY)/PAGE_SIZE)) + 0.1));
        if(rem_num_pages > MAX_PAGES){
            rem_num_pages = MAX_PAGES;
        }
        int i = my_log2(rem_num_pages*PAGE_SIZE) - MIN_EXP;
        list_remove(&(heap_array[rem_idx]),(node*)page_block);
        if(local_cache[i].size < MAX_LOCAL){
            list_enqueue(&local_cache[i],(node*)page_block);
        }
        else if(global_cache[i].size < MAX_GLOBAL){
            stack_head_c_t old_val,new_val;
            atomic_add(1,&global_cache[i].size);
            do{
                ((pageblock_cache_node_t *)page_block)->next = (pageblock_cache_node_t *)((unsigned long int)(global_cache[i].stack.address));
                old_val = global_cache[i].stack;
                new_val.address = (unsigned long int)page_block;
                new_val.count = (old_val.count + 1);

            }while(!compare_and_swap((void **)&(global_cache[i].stack),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));
        }
        else{
            munmap(page_block,rem_num_pages*PAGE_SIZE);
        }
    }
}

void free_remote(void* ptr,pageblock_t * page_block){
    stack_head_t old_val,new_val;
    do{
        old_val = page_block->remotely_freed;
        if(!old_val.id){
            new_val.address = old_val.address;
            new_val.count = (old_val.count + 1);
            new_val.id = id;
            if(compare_and_swap((void **)&(page_block->head),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val)))){
                
                int rem_idx = ((page_block->class_size)/CLASS_GRANULARITY)-1;
                list_enqueue(&heap_array[rem_idx],(node *)page_block);
                free_local(ptr,page_block);

            }
            else{
                free_remote(ptr,page_block);
            }
            break;

        }
        if(old_val.address){
            ((object_t *)ptr)->next = (object_t *)((unsigned long int)(old_val.address)+(unsigned long int)page_block->self);
        }
        else{
            ((object_t *)ptr)->next = 0;
        }
        new_val.address = ((unsigned long int)ptr-(unsigned long int)page_block->self);
        new_val.count = (old_val.count + 1);
        new_val.id = old_val.id;

    }while(!compare_and_swap((void **)&(page_block->head),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val))));

}

extern "C" void ufree(void *ptr){
    large_object_t *large_object;
    pageblock_t *page_block;
    if(!init_flag){
        thread_local static struct thread_data_struct thread_data;
    }
    if(ptr == NULL){
        return;
    }
    large_object = (large_object_t *)(((void **)(((unsigned long)ptr) & ~ (unsigned long)(PAGE_SIZE - 1))));
    if(large_object->magic){

        munmap(large_object,large_object->size);

    }
    else{
        page_block = (pageblock_t *)(*((void **)(((unsigned long)ptr) & ~ (unsigned long)(PAGE_SIZE - 1))));

        stack_head_t old_val,new_val;
        old_val = page_block->remotely_freed;
        if(id == old_val.id){
            free_local(ptr,page_block);
        }
        else if (old_val.id){
            free_remote(ptr,page_block);
        }
        else{
            new_val.address = old_val.address;
            new_val.count = (old_val.count + 1);
            new_val.id = id;
            if(compare_and_swap((void **)&(page_block->head),(void *)(*((unsigned long *)&old_val)),(void *)(*((unsigned long *)&new_val)))){
                
                int rem_idx = ((page_block->class_size)/CLASS_GRANULARITY)-1;
                list_enqueue(&heap_array[rem_idx],(node *)page_block);
                free_local(ptr,page_block);

            }
            else{
                free_remote(ptr,page_block);
            }

        }
    }
}
extern "C" void *urealloc(void *ptr,size_t size){
    large_object_t *large_object;
    pageblock_t *page_block;
    void * ret=NULL;
    if(!init_flag){
        thread_local static struct thread_data_struct thread_data;
    }
    if(!size){
        return ret;
    }
    if(!ptr){
        return umalloc(size);
    }
    large_object = (large_object_t *)(((void **)(((unsigned long)ptr) & ~ (unsigned long)(PAGE_SIZE - 1))));
    if(large_object->magic){
        long unsigned int num_of_pages,new_size;
        num_of_pages = (int)my_ceil(((double)(size + sizeof(large_object_t)))/PAGE_SIZE);
        new_size = num_of_pages*PAGE_SIZE;
        if(size <= SMALL_OBJECT){
            ret = umalloc(size);
            if(ret != NULL){
                memcpy(ret,ptr,size);
                munmap(large_object,large_object->size);

            }
            return ret;
        }
        else{
            ret = mremap(large_object,large_object->size,new_size,MREMAP_MAYMOVE);
            if(ret == (void *)-1){
                return NULL;
            }
            ((large_object_t *)ret)->size = new_size;
            ret = (void *)((unsigned long)ret + sizeof(large_object_t));

        }
    }
    else{
        page_block = (pageblock_t *)(*((void **)(((unsigned long)ptr) & ~ (unsigned long)(PAGE_SIZE - 1))));
        int index;
        index = (size - 1)/ CLASS_GRANULARITY;
        long unsigned int new_size = (index +1)*CLASS_GRANULARITY;
        if(new_size > page_block->class_size){
            ret = umalloc(size);
            if(ret != NULL){
                memcpy(ret,ptr,page_block->class_size);
                ufree(ptr);
            }
        }
        else if(new_size < page_block->class_size){
            ret = umalloc(size);
            if(ret != NULL){
                memcpy(ret,ptr,size);
                ufree(ptr);
            }
        }
        else{
            return ptr;
        }


    }

    return ret;
}


extern "C" void* ucalloc(size_t nitems,size_t size){
    void *ptr;
    if(!init_flag){
        thread_local static struct thread_data_struct thread_data;
    }
    ptr = umalloc(nitems*size);
    if(!ptr){
        return NULL;
    }

    return memset(ptr,0,nitems*size);
}

#ifdef MALLOC
extern "C" void* malloc(size_t size){
    return umalloc(size);
}

extern "C" void free(void* ptr){
    ufree(ptr);
    return;
}

extern "C" void* realloc(void *ptr,size_t size){
    return urealloc(ptr,size);
}

extern "C" void* calloc(size_t nitems,size_t size){
    return ucalloc(nitems,size);
}
#endif

void* operator new(size_t size)
{
	return umalloc(size);
}

void * operator new(size_t size, const std::nothrow_t&) throw()
{
	return umalloc(size);
}

void operator delete(void * ptr) throw()
{
	ufree(ptr);
}

void* operator new[](size_t size)
{
	return umalloc(size);
}

void * operator new[](size_t size, const std::nothrow_t&) throw()
{
	return umalloc(size);
}

void operator delete[](void * ptr) throw()
{
	ufree(ptr);
}
