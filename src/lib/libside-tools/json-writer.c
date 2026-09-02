// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "json-writer.h"

void side_json_writer_init(struct side_json_writer *writer, FILE *out, bool compact)
{
	memset(writer, 0, sizeof(*writer));
	writer->out = out ? out : stdout;
	writer->compact = compact;
	writer->first_element = true;
}

void *side_json_resolve(struct side_json_writer *writer, const void *ptr)
{
	if (!writer->resolve)
		return (void *) ptr;
	return writer->resolve((void *) ptr, writer->resolve_priv);
}

/*
 * The separator which precedes an item: none for the first item of an
 * object or of an array, a comma for the following ones. Indented
 * output puts each item on its own line.
 */
void side_json_next(struct side_json_writer *writer)
{
	if (writer->first_element)
		writer->first_element = false;
	else
		fputc(',', writer->out);
	if (writer->compact)
		return;
	fputc('\n', writer->out);
	for (unsigned int i = 0; i < writer->nesting; i++)
		fputc('\t', writer->out);
}

void side_json_item(struct side_json_writer *writer, const char *fmt, ...)
{
	va_list ap;

	side_json_next(writer);
	va_start(ap, fmt);
	vfprintf(writer->out, fmt, ap);
	va_end(ap);
}

void side_json_raw(struct side_json_writer *writer, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(writer->out, fmt, ap);
	va_end(ap);
}

void side_json_push(struct side_json_writer *writer)
{
	writer->nesting++;
	writer->first_element = true;
}

void side_json_pop(struct side_json_writer *writer, char closing)
{
	writer->nesting--;
	writer->first_element = false;
	if (!writer->compact) {
		fputc('\n', writer->out);
		for (unsigned int i = 0; i < writer->nesting; i++)
			fputc('\t', writer->out);
	}
	fputc(closing, writer->out);
}

/*
 * A JSON string: the quotation mark, the reverse solidus and the
 * control characters have to be escaped.
 */
void side_json_string(struct side_json_writer *writer, const char *str)
{
	fputc('"', writer->out);
	if (str) {
		for (; *str; str++) {
			unsigned char c = (unsigned char) *str;

			switch (c) {
			case '"':
				fputs("\\\"", writer->out);
				break;
			case '\\':
				fputs("\\\\", writer->out);
				break;
			case '\b':
				fputs("\\b", writer->out);
				break;
			case '\f':
				fputs("\\f", writer->out);
				break;
			case '\n':
				fputs("\\n", writer->out);
				break;
			case '\r':
				fputs("\\r", writer->out);
				break;
			case '\t':
				fputs("\\t", writer->out);
				break;
			default:
				if (c < 0x20)
					fprintf(writer->out, "\\u%04x", c);
				else
					fputc(c, writer->out);
				break;
			}
		}
	}
	fputc('"', writer->out);
}

void side_json_item_name(struct side_json_writer *writer, const char *name)
{
	side_json_next(writer);
	side_json_string(writer, name);
	fputs(": ", writer->out);
}

void side_json_item_string(struct side_json_writer *writer, const char *name,
		const char *value)
{
	side_json_item_name(writer, name);
	side_json_string(writer, value);
}

const char *side_json_attr_value(struct side_json_writer *writer,
		const struct side_attr_value *value)
{
	static _Thread_local char buffer[4096];

	switch (side_enum_get(value->type)) {
	case SIDE_ATTR_TYPE_NULL:
		return "null";
	case SIDE_ATTR_TYPE_BOOL:
		return value->u.bool_value ? "true" : "false";
	case SIDE_ATTR_TYPE_U8:
		snprintf(buffer, sizeof(buffer), "%" PRIu8, value->u.integer_value.side_u8);
		return buffer;
	case SIDE_ATTR_TYPE_U16:
		snprintf(buffer, sizeof(buffer), "%" PRIu16, value->u.integer_value.side_u16);
		return buffer;
	case SIDE_ATTR_TYPE_U32:
		snprintf(buffer, sizeof(buffer), "%" PRIu32, value->u.integer_value.side_u32);
		return buffer;
	case SIDE_ATTR_TYPE_U64:
		snprintf(buffer, sizeof(buffer), "%" PRIu64, value->u.integer_value.side_u64);
		return buffer;
	case SIDE_ATTR_TYPE_S8:
		snprintf(buffer, sizeof(buffer), "%" PRId8, value->u.integer_value.side_s8);
		return buffer;
	case SIDE_ATTR_TYPE_S16:
		snprintf(buffer, sizeof(buffer), "%" PRId16, value->u.integer_value.side_s16);
		return buffer;
	case SIDE_ATTR_TYPE_S32:
		snprintf(buffer, sizeof(buffer), "%" PRId32, value->u.integer_value.side_s32);
		return buffer;
	case SIDE_ATTR_TYPE_S64:
		snprintf(buffer, sizeof(buffer), "%" PRId64, value->u.integer_value.side_s64);
		return buffer;
#if __HAVE_FLOAT32
	case SIDE_ATTR_TYPE_FLOAT_BINARY32:
		snprintf(buffer, sizeof(buffer), "%g",
			(double) value->u.float_value.side_float_binary32);
		return buffer;
#endif
#if __HAVE_FLOAT64
	case SIDE_ATTR_TYPE_FLOAT_BINARY64:
		snprintf(buffer, sizeof(buffer), "%g",
			(double) value->u.float_value.side_float_binary64);
		return buffer;
#endif
	case SIDE_ATTR_TYPE_STRING:
		/*
		 * The value of a string attribute has to be escaped,
		 * which the caller does with side_json_attr_value_is_string.
		 */
		return NULL;
	default:
		return "\"<UNKNOWN>\"";
	}
}

void side_json_attributes(struct side_json_writer *writer,
		const struct side_attr *attr, uint32_t nr_attr)
{
	uint32_t i;

	if (!attr || !nr_attr) {
		side_json_item(writer, "\"attributes\": {}");
		return;
	}
	side_json_item(writer, "\"attributes\": {");
	side_json_push(writer);
	for (i = 0; i < nr_attr; i++) {
		const char *key = side_json_resolve_sel_ptr(writer, attr[i].key.p);
		const struct side_attr_value *value = &attr[i].value;
		const char *json_value = side_json_attr_value(writer, value);

		side_json_item_name(writer, key);
		if (json_value) {
			side_json_raw(writer, "%s", json_value);
			continue;
		}
		/* A string value: escape it. */
		if (value->u.string_value.unit_size != 1) {
			side_json_string(writer, "<UNKNOWN>");
			continue;
		}
		side_json_string(writer,
			side_json_resolve_sel_ptr(writer, value->u.string_value.p));
	}
	side_json_pop(writer, '}');
}

const char *side_loglevel_to_string(enum side_loglevel loglevel)
{
	switch (loglevel) {
	case SIDE_LOGLEVEL_EMERG:
		return "EMERG";
	case SIDE_LOGLEVEL_ALERT:
		return "ALERT";
	case SIDE_LOGLEVEL_CRIT:
		return "CRIT";
	case SIDE_LOGLEVEL_ERR:
		return "ERR";
	case SIDE_LOGLEVEL_WARNING:
		return "WARNING";
	case SIDE_LOGLEVEL_NOTICE:
		return "NOTICE";
	case SIDE_LOGLEVEL_INFO:
		return "INFO";
	case SIDE_LOGLEVEL_DEBUG:
		return "DEBUG";
	default:
		return "<UNKNOWN>";
	}
}

const char *side_type_to_string(enum side_type_label label)
{
	switch (label) {
	case SIDE_TYPE_NULL:
		return "NULL";
	case SIDE_TYPE_BOOL:
		return "BOOL";
	case SIDE_TYPE_U8:
		return "U8";
	case SIDE_TYPE_U16:
		return "U16";
	case SIDE_TYPE_U32:
		return "U32";
	case SIDE_TYPE_U64:
		return "U64";
	case SIDE_TYPE_U128:
		return "U128";
	case SIDE_TYPE_S8:
		return "S8";
	case SIDE_TYPE_S16:
		return "S16";
	case SIDE_TYPE_S32:
		return "S32";
	case SIDE_TYPE_S64:
		return "S64";
	case SIDE_TYPE_S128:
		return "S128";
	case SIDE_TYPE_BYTE:
		return "BYTE";
	case SIDE_TYPE_POINTER:
		return "POINTER";
	case SIDE_TYPE_FLOAT_BINARY16:
		return "FLOAT_BINARY16";
	case SIDE_TYPE_FLOAT_BINARY32:
		return "FLOAT_BINARY32";
	case SIDE_TYPE_FLOAT_BINARY64:
		return "FLOAT_BINARY64";
	case SIDE_TYPE_FLOAT_BINARY128:
		return "FLOAT_BINARY128";
	case SIDE_TYPE_STRING_UTF8:
		return "STRING_UTF8";
	case SIDE_TYPE_STRING_UTF16:
		return "STRING_UTF16";
	case SIDE_TYPE_STRING_UTF32:
		return "STRING_UTF32";
	case SIDE_TYPE_STRUCT:
		return "STRUCT";
	case SIDE_TYPE_VARIANT:
		return "VARIANT";
	case SIDE_TYPE_OPTIONAL:
		return "OPTIONAL";
	case SIDE_TYPE_ARRAY:
		return "ARRAY";
	case SIDE_TYPE_VLA:
		return "VLA";
	case SIDE_TYPE_ENUM:
		return "ENUM";
	case SIDE_TYPE_ENUM_BITMAP:
		return "ENUM_BITMAP";
	case SIDE_TYPE_DYNAMIC:
		return "DYNAMIC";
	case SIDE_TYPE_GATHER_BOOL:
		return "GATHER_BOOL";
	case SIDE_TYPE_GATHER_INTEGER:
		return "GATHER_INTEGER";
	case SIDE_TYPE_GATHER_BYTE:
		return "GATHER_BYTE";
	case SIDE_TYPE_GATHER_POINTER:
		return "GATHER_POINTER";
	case SIDE_TYPE_GATHER_FLOAT:
		return "GATHER_FLOAT";
	case SIDE_TYPE_GATHER_STRING:
		return "GATHER_STRING";
	case SIDE_TYPE_GATHER_STRUCT:
		return "GATHER_STRUCT";
	case SIDE_TYPE_GATHER_ARRAY:
		return "GATHER_ARRAY";
	case SIDE_TYPE_GATHER_VLA:
		return "GATHER_VLA";
	case SIDE_TYPE_GATHER_ENUM:
		return "GATHER_ENUM";
	case SIDE_TYPE_DYNAMIC_NULL:
		return "DYNAMIC_NULL";
	case SIDE_TYPE_DYNAMIC_BOOL:
		return "DYNAMIC_BOOL";
	case SIDE_TYPE_DYNAMIC_INTEGER:
		return "DYNAMIC_INTEGER";
	case SIDE_TYPE_DYNAMIC_BYTE:
		return "DYNAMIC_BYTE";
	case SIDE_TYPE_DYNAMIC_POINTER:
		return "DYNAMIC_POINTER";
	case SIDE_TYPE_DYNAMIC_FLOAT:
		return "DYNAMIC_FLOAT";
	case SIDE_TYPE_DYNAMIC_STRING:
		return "DYNAMIC_STRING";
	case SIDE_TYPE_DYNAMIC_STRUCT:
		return "DYNAMIC_STRUCT";
	case SIDE_TYPE_DYNAMIC_VLA:
		return "DYNAMIC_VLA";
	default:
		return "<UNKNOWN>";
	}
}

const char *side_access_mode_to_string(enum side_type_gather_access_mode am)
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

const char *side_byte_order_to_string(enum side_type_label_byte_order bo)
{
	switch (bo) {
	case SIDE_TYPE_BYTE_ORDER_LE: return "little";
	case SIDE_TYPE_BYTE_ORDER_BE: return "big";
	default: return "<UNKNOWN>";
	}
}
