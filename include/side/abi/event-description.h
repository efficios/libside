// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_EVENT_DESCRIPTION_H
#define SIDE_ABI_EVENT_DESCRIPTION_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

/*
 * SIDE ABI event description.
 *
 * The extensibility scheme for the SIDE ABI event description is as
 * follows:
 *
 * * Changing the semantic of the existing event description fields is a
 *   breaking ABI change: the SIDE_EVENT_DESCRIPTION_ABI_VERSION should
 *   be increased to reflect this.
 *
 * * Event descriptions can be extended by adding fields at the end of
 *   the structure. The "struct side_event_description" is a structure
 *   with flexible size which must not be used within arrays.
 */

#define SIDE_EVENT_DESCRIPTION_ABI_VERSION	0

enum side_event_flags {
	SIDE_EVENT_FLAG_VARIADIC = (1 << 0),
};

enum side_loglevel {
	SIDE_LOGLEVEL_EMERG = 0,
	SIDE_LOGLEVEL_ALERT = 1,
	SIDE_LOGLEVEL_CRIT = 2,
	SIDE_LOGLEVEL_ERR = 3,
	SIDE_LOGLEVEL_WARNING = 4,
	SIDE_LOGLEVEL_NOTICE = 5,
	SIDE_LOGLEVEL_INFO = 6,
	SIDE_LOGLEVEL_DEBUG = 7,
};

#define SIDE_BEGIN_ABI(V)			\
	char side_begin_abi_tag_##V [0]

#define SIDE_END_ABI(V)				\
	char side_end_abi_tag_##V [0]

struct side_event_description {

	SIDE_BEGIN_ABI(0);

	uint32_t struct_size;	/* Size of this structure. */
	uint32_t version;	/* Event description ABI version. */

	side_ptr_t(struct side_event_state) state;
	side_ptr_t(const char) provider_name;
	side_ptr_t(const char) event_name;
	side_array_t(const struct side_event_field) fields;
	side_array_t(const struct side_attr) attributes;
	uint64_t flags;	/* Bitwise OR of enum side_event_flags */
	uint16_t nr_side_type_label;
	uint16_t nr_side_attr_type;
	side_enum_t(enum side_loglevel, uint32_t) loglevel;

	SIDE_END_ABI(0);

	char end[0];	/* End with a zero-sized array to account for extensibility. */
} SIDE_PACKED;

#define side_event_description_orig_abi_last	\
	side_end_abi_tag_0

#endif /* SIDE_ABI_EVENT_DESCRIPTION_H */
