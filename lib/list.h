#ifndef LIST_H
#define LIST_H
#include <stddef.h>

typedef struct node {
  void *data,*padding;
  struct node *next;
  struct node *prev;
} node;

node *node_create ();
void node_free (node *p);

typedef struct list {
  node *head;
  node *tail;
  size_t size;
} list_t;


list_t *list_create ();
void list_free (list_t *l);
node *list_remove (list_t *l, node *p);
int is_empty(list_t *l);
node *list_dequeue (list_t *l);
node *list_remove_last (list_t *l);
void list_enqueue (list_t *l, node *p);
void list_push (list_t *l, node *p);
void list_init (list_t *l);
void list_move_to_tail(list_t *l);
int transfer_half(list_t *src,list_t *dst);

#endif
