// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#ifndef SIDE_TOOLS_JSON_WRITER_H
#define SIDE_TOOLS_JSON_WRITER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <side/abi/attribute.h>
#include <side/abi/event-description.h>
#include <side/abi/type-description.h>

/*
 * Emission of JSON, shared by the tools which describe side events:
 * the separators, the nesting and the escaping are here, what to emit
 * is up to the caller.
 *
 * Two layouts: indented, one item per line, which suits the dump of
 * the descriptions of a file, and compact, which suits a stream of
 * events, one object per line.
 */
struct side_json_writer {
	FILE *out;
	/*
	 * Translate a pointer of a description into a pointer usable by
	 * this process. A reader of descriptions found in a file maps
	 * them elsewhere; NULL when the descriptions are those of this
	 * process.
	 */
	void *(*resolve)(void *ptr, void *priv);
	void *resolve_priv;
	unsigned int nesting;
	bool first_element;
	bool compact;
};

void side_json_writer_init(struct side_json_writer *writer, FILE *out, bool compact);

/*
 * Resolve a pointer of a description, see the resolve member.
 */
void *side_json_resolve(struct side_json_writer *writer, const void *ptr);

#define side_json_resolve_ptr(writer, ptr) \
	((__typeof__(side_ptr_get(ptr))) side_json_resolve(writer, side_ptr_get(ptr)))

/*
 * A pointer which says which of the two it holds needs resolving only
 * where it holds an address: a distance is measured from the field
 * holding it, so it resolves within whatever mapping the description
 * was read into. See side_ptr_sel_t.
 */
#define side_json_resolve_sel_ptr(writer, ptr)				\
	((__typeof__(side_ptr_sel_get(ptr)))				\
		((ptr).is_offset ? side_ptr_sel_get(ptr) :		\
			side_json_resolve(writer, side_ptr_sel_get(ptr))))

/*
 * Emit the separator and the indentation which precede an item: none
 * for the first item of an object or of an array, a comma for the
 * following ones.
 */
void side_json_next(struct side_json_writer *writer);

/* Emit the separator and the indentation, then the item itself. */
void side_json_item(struct side_json_writer *writer, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

/*
 * Emit as-is: the value of an item which has been introduced by its
 * name, or the punctuation of an object or of an array.
 */
void side_json_raw(struct side_json_writer *writer, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

/* Enter and leave a nested object or array. */
void side_json_push(struct side_json_writer *writer);
void side_json_pop(struct side_json_writer *writer, char closing);

/* Emit a string as a JSON string, quoted and escaped. */
void side_json_string(struct side_json_writer *writer, const char *str);

/*
 * Emit an item whose name and value are strings, both escaped. The
 * name of an item and the value of a string come from the description
 * of an event, which is written by the instrumented application: they
 * cannot be emitted as-is.
 */
void side_json_item_string(struct side_json_writer *writer, const char *name,
		const char *value);

/* Emit the name of an item, escaped, and the colon which follows it. */
void side_json_item_name(struct side_json_writer *writer, const char *name);

/*
 * The value of an attribute as JSON, valid until the next call from
 * the same thread. Returns NULL for a string value, which the caller
 * emits with side_json_string so that it is escaped.
 */
const char *side_json_attr_value(struct side_json_writer *writer,
		const struct side_attr_value *value);

/* Emit the attributes of an event or of a type as a JSON object. */
void side_json_attributes(struct side_json_writer *writer,
		const struct side_attr *attr, uint32_t nr_attr);

/* Descriptions of the side enumerations, as strings. */
const char *side_loglevel_to_string(enum side_loglevel loglevel);
const char *side_type_to_string(enum side_type_label label);
const char *side_access_mode_to_string(enum side_type_gather_access_mode access_mode);
const char *side_byte_order_to_string(enum side_type_label_byte_order byte_order);

#endif /* SIDE_TOOLS_JSON_WRITER_H */
