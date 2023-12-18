// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_RCULIST_H
#define _SIDE_RCULIST_H

#include "list_types.h"
#include "rcu.h"

static inline
void side_list_insert_node_tail_rcu(struct side_list_head *head, struct side_list_node *node)
{
	node->next = &head->node;
	node->prev = head->node.prev;
	head->node.prev = node;
	side_rcu_assign_pointer(node->prev->next, node);
}

static inline
void side_list_insert_node_head_rcu(struct side_list_head *head, struct side_list_node *node)
{
	node->next = head->node.next;
	node->prev = &head->node;
	node->next->prev = node;
	side_rcu_assign_pointer(head->node.next, node);
}

static inline
void side_list_remove_node_rcu(struct side_list_node *node)
{
	node->next->prev = node->prev;
	__atomic_store_n(&node->prev->next, node->next, __ATOMIC_RELAXED);
}

#define side_list_for_each_entry_rcu(_entry, _head, _member) \
	for ((_entry) = side_container_of(side_rcu_dereference((_head)->node.next), __typeof__(*(_entry)), _member); \
		&(_entry)->_member != &(_head)->node; \
		(_entry) = side_container_of(side_rcu_dereference((_entry)->_member.next), __typeof__(*(_entry)), _member))

#endif /* _SIDE_RCULIST_H */
