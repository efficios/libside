// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_EVENT_DESCRIPTION_H
#define SIDE_ABI_EVENT_DESCRIPTION_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

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

struct side_event_description {
	uint32_t struct_size;	/* Size of this structure. */
	uint32_t version;	/* Event description ABI version. */

	side_ptr_t(struct side_event_state) state;
	side_ptr_t(const char) provider_name;
	side_ptr_t(const char) event_name;
	side_ptr_t(const struct side_event_field) fields;
	side_ptr_t(const struct side_attr) attr;
	uint64_t flags;	/* Bitwise OR of enum side_event_flags */
	uint16_t nr_side_type_label;
	uint16_t nr_side_attr_type;
	side_enum_t(enum side_loglevel, uint32_t) loglevel;
	uint32_t nr_fields;
	uint32_t nr_attr;
	uint32_t nr_callbacks;
#define side_event_description_orig_abi_last nr_callbacks
	/* End of fields supported in the original ABI. */

	char end[];	/* End with a flexible array to account for extensibility. */
} SIDE_PACKED;

#endif /* SIDE_ABI_EVENT_DESCRIPTION_H */
