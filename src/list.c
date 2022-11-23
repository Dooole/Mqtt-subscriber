#include <stdlib.h>
#include <string.h>

#include "list.h"

static node_t *node_create(size_t data_size) {
	char *data = (char *)malloc(data_size);
	if (data == NULL)
		return NULL;

	memset(data, 0, data_size);

	node_t *node = (node_t *)malloc(sizeof(node_t));
	if (node == NULL) {
		free(data);
		return NULL;
	}

	memset(node, 0, sizeof(node_t));
	node->data = data;
	node->data_size = data_size;

	return node;
}

list_t *list_create(void) {
	list_t *list = (list_t *)malloc(sizeof(list_t));
	if (list == NULL)
		return NULL;

	memset(list, 0, sizeof(list_t));

	return list;
}

void list_delete(list_t *list) {
	node_t *next = NULL;

	node_t *node = list->head;
	while (node != NULL) {
		next = node->next;
		free(node->data);
		free(node);
		node = next;
	}

	free(list);
}

void *list_newnode(list_t *list, size_t data_size) {
	node_t *new = node_create(data_size);
	if (new == NULL)
		return NULL;

	node_t *node = list->head;
	if (node == NULL) {
		list->head = new;
		return new->data;
	}

	while (node->next != NULL) {
		node = node->next;
	}
	node->next = new;

	return new->data;
}
