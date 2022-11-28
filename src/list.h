// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _TGIF_LIST_H
#define _TGIF_LIST_H

struct tgif_list_node {
	struct tgif_list_node *next;
	struct tgif_list_node *prev;
};

struct tgif_list_head {
	struct tgif_list_node node;
};

#define DEFINE_TGIF_LIST_HEAD(_identifier) \
	struct tgif_list_head _identifier = { \
		.node = { \
			.next = &(_identifier).node, \
			.prev = &(_identifier).node, \
		}, \
	}

static inline
void tgif_list_insert_node_tail(struct tgif_list_head *head, struct tgif_list_node *node)
{
	node->next = &head->node;
	node->prev = head->node.prev;
	node->prev->next = node;
	head->node.prev = node;
}

static inline
void tgif_list_insert_node_head(struct tgif_list_head *head, struct tgif_list_node *node)
{
	node->next = head->node.next;
	node->prev = &head->node;
	node->next->prev = node;
	head->node.next = node;
}

static inline
void tgif_list_remove_node(struct tgif_list_node *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

#define tgif_list_for_each_entry(_entry, _head, _member) \
	for ((_entry) = tgif_container_of((_head)->node.next, __typeof__(*(_entry)), _member); \
		&(_entry)->_member != &(_head)->node; \
		(_entry) = tgif_container_of((_entry)->_member.next, __typeof__(*(_entry)), _member))

/* List iteration, safe against node reclaim while iterating. */
#define tgif_list_for_each_entry_safe(_entry, _next_entry, _head, _member) \
	for ((_entry) = tgif_container_of((_head)->node.next, __typeof__(*(_entry)), _member), \
			(_next_entry) = tgif_container_of((_entry)->_member.next, __typeof__(*(_entry)), _member); \
		&(_entry)->_member != &(_head)->node; \
		(_entry) = tgif_container_of((_next_entry)->_member.next, __typeof__(*(_entry)), _member), \
		(_next_entry) = tgif_container_of((_entry)->_member.next, __typeof__(*(_entry)), _member))

#endif /* _TGIF_LIST_H */
