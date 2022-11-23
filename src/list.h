#ifndef LIST_H
#define LIST_H

typedef struct node {
	char *data;
	size_t data_size;
	struct node *next;
} node_t;

typedef struct list {
	struct node *head;
} list_t;

list_t *list_create(void);
void list_delete(list_t *list);
void *list_newnode(list_t *list, size_t data_size);

#define list_foreach(list, node) \
	for (node_t *node = list->head; node != NULL; node = node->next)

#endif
