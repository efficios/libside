// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_VISITORS_COMMON_H
#define READSIDE_VISITORS_COMMON_H

#include <side/abi/event-description.h>
#include <side/abi/type-description.h>

#include "libside-tools/visit-description.h"

struct visitor_context {
	void *(*resolve)(void *, void*);
	void *resolve_priv;
	u64 nesting;
	void *context;
};

struct visitor {
	struct side_description_visitor_callbacks description;
	void (*begin)(struct visitor_context *);
	void (*end)(struct visitor_context *);
	void *(*make_context)(void);
	void (*drop_context)(void *);
};

#define visit_pointer(ctx, ptr)						\
	((typeof (ptr))cast(struct visitor_context *, ctx)->resolve((void *)ptr, ctx))

#define visit_side_pointer(ctcx, ptr)		\
	visit_pointer(ctx, side_ptr_get(ptr))

/*
 * It is the reader that defines how to resolve pointer by the visitor.  This
 * require making a copy of the visitor structure and setting the resolve
 * pointer function in it.
 */
static inline void copy_visitor_with_resolver(const struct visitor *in,
					void *(*resolve)(void *, void *),
					struct visitor *out)
{
	memcpy(out, in, sizeof(struct visitor));
	out->description.resolve_pointer_func = resolve;
}

/**
 * Given TYPE in visitor CTX, fille ATTRS and NR_ATTRS with the attributes and
 * the number of attributes of TYPE.
 */
extern void side_type_attributes(const struct side_type *type, void *ctx,
				const struct side_attr **attrs, u32 *nr_attrs);

/**
 * Translate loglevel enumeration to string.
 */
extern const char *side_loglevel_to_string(enum side_loglevel loglevel);

/**
 * Translate type label enumeration to string.
 */
extern const char *side_type_to_string(enum side_type_label label);

/**
 * Translate gather access mode enumeration to string.
 */
static inline const char *side_access_mode_to_string(enum side_type_gather_access_mode am)
{
	switch (am) {
	case SIDE_TYPE_GATHER_ACCESS_DIRECT:
		return "direct";
	case SIDE_TYPE_GATHER_ACCESS_POINTER:
		return "pointer";
	default:
		return "<UNKNOWN>";
	}
}

/**
 * Translate type label byte order enumeration to string.
 */
static inline const char *side_byte_order_to_string(enum side_type_label_byte_order bo)
{
	switch (bo) {
	case SIDE_TYPE_BYTE_ORDER_LE: return "little";
	case SIDE_TYPE_BYTE_ORDER_BE: return "big";
	default: return "<UNKNOWN>";
	}
}

/* Available visitors. */
extern struct visitor json_visitor;
extern struct visitor sexpr_visitor;
extern struct visitor text_visitor;

#endif	/* READSIDE_VISITORS_COMMON_H */
