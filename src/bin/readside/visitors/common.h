// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_VISITORS_COMMON_H
#define READSIDE_VISITORS_COMMON_H

#include <side/abi/event-description.h>
#include <side/abi/type-description.h>

#include "libside-tools/json-writer.h"
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
 * A self-relative pointer needs no resolver: the distance it holds is
 * measured from the field which holds it, so it resolves within
 * whatever mapping the description was read into rather than against
 * the address space it was written in.
 *
 * FIXME: this assumes the description is read as one contiguous blob,
 * laid out as the producer wrote it. That holds for a description read
 * from a section; it does not for one reassembled piece by piece.
 */
#define visit_side_rel_pointer(ctx, ptr)	side_ptr_rel_get(ptr)

/* The elements of an array reached by a distance, for the same reason. */
#define visit_side_rel_array_elements(ctx, array)	side_array_rel_elements(&(array))

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



/* Available visitors. */
extern struct visitor json_visitor;
extern struct visitor sexpr_visitor;
extern struct visitor text_visitor;

#endif	/* READSIDE_VISITORS_COMMON_H */
