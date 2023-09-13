#include "list.h"


void list_init (list_t *l) {
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
}


node *list_remove (list_t *l, node *p) {
    if (l->size == 1) {
        l->head = NULL;
        l->tail = NULL;
    } 
    else {
        if (l->tail == p) {
            l->tail = p->prev;
            p->prev->next=l->head;
            l->head->prev = l->tail;
        }
        else if (l->head == p) {
            l->head = p->next;
            l->head->prev = l->tail;
            l->tail->next= l->head;
        }
        else {
            p->prev->next = p->next;
            p->next->prev = p->prev;
        }
    }

    l->size--;
    return p;
}


void list_move_to_tail(list_t *l){
    node *p = l->head;
    l->head = l->head->next;
    l->head->prev = l->tail;
    l->tail->next= l->head;
    
    p->next = l->head;
    p->prev = l->tail;
    l->tail->next = l->head;
    l->tail = l->head;
    l->head->prev = l->tail;
}


/*Remove the head of the queue-list.*/
node *list_dequeue (list_t *l) {
    node *n= NULL;
    if (l->size != 0) {
        n = list_remove(l, l->head);
    }
    return n;
}

/*Remove the tail of the queue-list.*/
node *list_remove_last (list_t *l) {
    node *n= NULL;
    if (l->size != 0) {
        n = list_remove(l, l->tail);
    }
    return n;
}

int is_empty(list_t *l){
    if (l->size == 0) {
        return 1;
    }
    else {
        return 0;
    }
}
/*Add node at the end of the queue-list.*/
void list_enqueue (list_t *l, node *p) {
    if (l->size == 0) {
        l->head = p;
        l->tail = p;
        p->prev = p;
	    p->next = p;
    } 
    else {   
        p->next = l->head;
        p->prev = l->tail;
        
        l->tail->next = p;
        l->tail = p;
        l->head->prev = l->tail;
    }

    l->size++;
}

/*Add node at the beggining of the queue-list.*/
void list_push (list_t *l, node *p) {
    if (l->size == 0) {
        l->head = p;
        l->tail = p;
        p->prev = p;
	    p->next = p;
    } 
    else {
        p->next = l->head;
        p->prev = l->tail;
        l->head->prev = p;
        l->head = p;
        l->tail->next = l->head;
    }
    l->size++;
}

