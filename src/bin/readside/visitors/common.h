// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_VISITORS_COMMON_H
#define READSIDE_VISITORS_COMMON_H

#include <side/abi/event-description.h>
#include <side/abi/type-description.h>

#include "libside-tools/json-writer.h"
#include <side/abi/event-state.h>

#include "libside-tools/visit-description.h"

struct visitor_context {
	void *(*resolve)(void *, void*);
	void *resolve_priv;
	u64 nesting;
	void *context;
	/*
	 * The ABI version of the state of the event being visited. It
	 * comes from the state rather than from the description, which
	 * holds no pointer to it: see side_ptr_rel_t. The reader walks
	 * the states and reaches each description through one, so it
	 * has the version in hand and leaves it here for the visitors.
	 */
	u32 state_version;
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
