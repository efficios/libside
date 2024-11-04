// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_LIST_TYPES_H
#define _SIDE_LIST_TYPES_H

struct side_list_node {
	struct side_list_node *next;
	struct side_list_node *prev;
};

struct side_list_head {
	struct side_list_node node;
};

#define DEFINE_SIDE_LIST_HEAD(_identifier) \
	struct side_list_head _identifier = { \
		.node = { \
			.next = &(_identifier).node, \
			.prev = &(_identifier).node, \
		}, \
	}

#endif /* _SIDE_LIST_TYPES_H */
