// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_LIST_H
#define _SIDE_LIST_H

#include "list_types.h"

static inline
void side_list_head_init(struct side_list_head *head)
{
	head->node.next = head->node.prev = &head->node;
}

static inline
void side_list_insert_node_tail(struct side_list_head *head, struct side_list_node *node)
{
	node->next = &head->node;
	node->prev = head->node.prev;
	node->prev->next = node;
	head->node.prev = node;
}

static inline
void side_list_insert_node_head(struct side_list_head *head, struct side_list_node *node)
{
	node->next = head->node.next;
	node->prev = &head->node;
	node->next->prev = node;
	head->node.next = node;
}

static inline
void side_list_remove_node(struct side_list_node *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

static inline
bool side_list_empty(struct side_list_head *head)
{
	return &head->node == head->node.next;
}

/*
 * Splice "from" list at the beginning of "to" list.
 */
static inline
void side_list_splice(struct side_list_head *from, struct side_list_head *to)
{
	if (side_list_empty(from))
		return;
	from->node.next->prev = &to->node;
	from->node.prev->next = to->node.next;
	to->node.next->prev = from->node.prev;
	to->node.next = from->node.next;
}

#define side_list_for_each_entry(_entry, _head, _member) \
	for ((_entry) = side_container_of((_head)->node.next, __typeof__(*(_entry)), _member); \
		&(_entry)->_member != &(_head)->node; \
		(_entry) = side_container_of((_entry)->_member.next, __typeof__(*(_entry)), _member))

/* List iteration, safe against node reclaim while iterating. */
#define side_list_for_each_entry_safe(_entry, _next_entry, _head, _member) \
	for ((_entry) = side_container_of((_head)->node.next, __typeof__(*(_entry)), _member), \
			(_next_entry) = side_container_of((_entry)->_member.next, __typeof__(*(_entry)), _member); \
		&(_entry)->_member != &(_head)->node; \
		(_entry) = (_next_entry), \
		(_next_entry) = side_container_of((_entry)->_member.next, __typeof__(*(_entry)), _member))

#endif /* _SIDE_LIST_H */
