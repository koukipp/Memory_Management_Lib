#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H


void *umalloc(size_t size);
void ufree(void *ptr);
void urealloc(void *ptr,size_t size);
void *ucalloc(size_t nitems,size_t size);

#endif
