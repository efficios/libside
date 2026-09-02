// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_INSTRUMENTATION_C_API_H
#define SIDE_INSTRUMENTATION_C_API_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

#include <side/abi/type-value.h>
#include <side/abi/attribute.h>
#include <side/abi/type-description.h>

#if (SIDE_BYTE_ORDER == SIDE_LITTLE_ENDIAN)
# define SIDE_TYPE_BYTE_ORDER_HOST		SIDE_TYPE_BYTE_ORDER_LE
#else
# define SIDE_TYPE_BYTE_ORDER_HOST		SIDE_TYPE_BYTE_ORDER_BE
#endif

#if (SIDE_FLOAT_WORD_ORDER == SIDE_LITTLE_ENDIAN)
# define SIDE_TYPE_FLOAT_WORD_ORDER_HOST	SIDE_TYPE_BYTE_ORDER_LE
#else
# define SIDE_TYPE_FLOAT_WORD_ORDER_HOST	SIDE_TYPE_BYTE_ORDER_BE
#endif

/* Event and type attributes */

/*
 * An attribute, as the pair of its key and its value rather than as an
 * initializer.
 *
 * The key is a string, and so is a string value. Both have to become
 * objects in the section a description lives in for the distances to
 * them to be ones the assembler folds, and only the site which gives
 * the attributes an array of their own can declare one. So an attribute
 * hands both halves over and that site puts them back together, the way
 * a field hands over its name and its type.
 *
 * A dynamic attribute list is built from the same pairs, but where it
 * stands and holding addresses: nothing can be declared at a call site,
 * and what it points at is not known until it runs. Which of the two an
 * attribute holds is said by the selector beside it. See
 * side_ptr_sel_t.
 */
#define _side_attr(_key, _value)	(_key, _value)

/*
 * The attributes, as a list the site giving them an array can walk. It
 * is parenthesized rather than braced for the reason a field list is:
 * that site walks it twice, and only parentheses keep the elements
 * together as one macro argument on the way there.
 */
#define _side_attr_list(...)		( __VA_ARGS__ )

/* A value which holds no string, and one which does. */
#define _side_attr_null(_val)		(SIDE_AK_BARE, { .type = SIDE_ATTR_TYPE_NULL })
#define _side_attr_bool(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_BOOL), .u = { .bool_value = !!(_val) } })

#define __side_attr_string(_val, _byte_order, _unit_size)		\
	(SIDE_AK_STRING, _byte_order, _unit_size, _val)

/* Where an attribute puts what it holds, and under which name. */
#define SIDE_ATTR_ARRAY_SYM(_ctx)	SIDE_CAT3(_ctx, __attrs, )
#define SIDE_ATTR_ARRAY_OFF(_ctx)	SIDE_CAT3(_ctx, __attrs_off, )
#define SIDE_ATTR_KEY_SYM(_ctx, _idx)	SIDE_CAT3(_ctx, __attr_key_, _idx)
#define SIDE_ATTR_KEY_OFF(_ctx, _idx)	SIDE_CAT3(_ctx, __attr_key_off_, _idx)
#define SIDE_ATTR_VAL_PFX(_ctx, _idx)	SIDE_CAT3(_ctx, __attr_val_, _idx)
#define SIDE_ATTR_STR_SYM(_pfx)		SIDE_CAT3(_pfx, __str, )
#define SIDE_ATTR_STR_OFF(_pfx)		SIDE_CAT3(_pfx, __str_off, )

/*
 * A string an attribute holds, in the section a description is in and
 * not const, which is what lets the assembler fold a distance to it.
 * The array of attributes goes there for the same reason, and is named
 * before it is defined because the distances to the strings are
 * measured from it.
 */
#ifdef __cplusplus
#  define SIDE_ATTR_STRING_OBJECT(_name, _val)				\
	namespace {							\
		char __attribute__((section("side_event_description"), used)) \
			_name[] SIDE_ASM_LABEL(_name) = _val;		\
	}
#  define SIDE_ATTR_ARRAY_DECLARE(_name)				\
	namespace {							\
		extern struct side_attr __attribute__((section("side_event_description"))) \
			_name[] SIDE_ASM_LABEL(_name);			\
	}
#  define SIDE_ATTR_ARRAY_OBJECT(_name, _init...)			\
	namespace {							\
		struct side_attr __attribute__((section("side_event_description"), used)) \
			_name[] SIDE_ASM_LABEL(_name) = _init;		\
	}
#else
#  define SIDE_ATTR_STRING_OBJECT(_name, _val)				\
	static char __attribute__((section("side_event_description"), used)) \
		_name[] SIDE_ASM_LABEL(_name) = _val;
#  define SIDE_ATTR_ARRAY_DECLARE(_name)				\
	static struct side_attr __attribute__((section("side_event_description"))) \
		_name[] SIDE_ASM_LABEL(_name);
#  define SIDE_ATTR_ARRAY_OBJECT(_name, _init...)			\
	static struct side_attr __attribute__((section("side_event_description"), used)) \
		_name[] SIDE_ASM_LABEL(_name) = _init;
#endif

/*
 * The value of an attribute: what it needs beside it, and the
 * initializer reaching it. Only a string needs anything; a value does
 * not hold another value, so there is no ladder here.
 */
#define SIDE_ATTR_VALUE_DECLARE(_obj, _off, _pfx, _value)		\
	SIDE_ATTR_VALUE_DECLARE_1(_obj, _off, _pfx, SIDE_UNPACK _value)
#define SIDE_ATTR_VALUE_DECLARE_1(_obj, _off, _pfx, ...)		\
	SIDE_ATTR_VALUE_DECLARE_2(_obj, _off, _pfx, __VA_ARGS__)
#define SIDE_ATTR_VALUE_DECLARE_2(_obj, _off, _pfx, _kind, ...)		\
	SIDE_ATTR_VALUE_DECLARE_ ## _kind(_obj, _off, _pfx, __VA_ARGS__)

#define SIDE_ATTR_VALUE_DECLARE_SIDE_AK_BARE(_obj, _off, _pfx, ...)

#define SIDE_ATTR_VALUE_DECLARE_SIDE_AK_STRING(_obj, _off, _pfx, _byte_order, _unit_size, _val) \
	SIDE_ATTR_STRING_OBJECT(SIDE_ATTR_STR_SYM(_pfx), _val)		\
	SIDE_PTR_REL_DEFINE_AT(SIDE_ATTR_STR_OFF(_pfx), _obj,		\
		(_off) + offsetof(struct side_attr_value, u.string_value.p.u), \
		SIDE_ATTR_STR_SYM(_pfx))

#define SIDE_ATTR_VALUE_INIT(_pfx, _value)				\
	SIDE_ATTR_VALUE_INIT_1(_pfx, SIDE_UNPACK _value)
#define SIDE_ATTR_VALUE_INIT_1(_pfx, ...)				\
	SIDE_ATTR_VALUE_INIT_2(_pfx, __VA_ARGS__)
#define SIDE_ATTR_VALUE_INIT_2(_pfx, _kind, ...)			\
	SIDE_ATTR_VALUE_INIT_ ## _kind(_pfx, __VA_ARGS__)

#define SIDE_ATTR_VALUE_INIT_SIDE_AK_BARE(_pfx, ...)	__VA_ARGS__

#define SIDE_ATTR_VALUE_INIT_SIDE_AK_STRING(_pfx, _byte_order, _unit_size, _val) \
	{								\
		.type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_STRING),		\
		.u = {							\
			.string_value = {				\
				.p = SIDE_PTR_SEL_REL_INIT(SIDE_ATTR_STR_OFF(_pfx)), \
				.unit_size = _unit_size,		\
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			},						\
		},							\
	}

/*
 * The same value, where it stands and holding an address, which is what
 * a dynamic argument has to write.
 */
#define SIDE_DYNAMIC_ATTR_VALUE_INIT(_value)				\
	SIDE_DYNAMIC_ATTR_VALUE_INIT_1(SIDE_UNPACK _value)
#define SIDE_DYNAMIC_ATTR_VALUE_INIT_1(...)				\
	SIDE_DYNAMIC_ATTR_VALUE_INIT_2(__VA_ARGS__)
#define SIDE_DYNAMIC_ATTR_VALUE_INIT_2(_kind, ...)			\
	SIDE_DYNAMIC_ATTR_VALUE_INIT_ ## _kind(__VA_ARGS__)

#define SIDE_DYNAMIC_ATTR_VALUE_INIT_SIDE_AK_BARE(...)	__VA_ARGS__

#define SIDE_DYNAMIC_ATTR_VALUE_INIT_SIDE_AK_STRING(_byte_order, _unit_size, _val) \
	{								\
		.type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_STRING),		\
		.u = {							\
			.string_value = {				\
				.p = SIDE_PTR_SEL_INIT(_val),		\
				.unit_size = _unit_size,		\
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			},						\
		},							\
	}

/*
 * One attribute: the strings it holds, put where a distance to them can
 * be folded, and the array element reaching them. An element which is
 * not parenthesized is the nothing a list with no element, or the
 * trailing comma the DSL allows, leaves behind.
 */
#define SIDE_ATTR_DECLARE(_ctx, _idx, _attr)				\
	SIDE_CAT2(SIDE_ATTR_DECLARE_, SIDE_IS_PAREN(_attr))(_ctx, _idx, _attr)

#define SIDE_ATTR_DECLARE_0(_ctx, _idx, _attr)

#define SIDE_ATTR_DECLARE_1(_ctx, _idx, _attr)				\
	SIDE_ATTR_DECLARE_2(SIDE_ATTR_KEY_SYM(_ctx, _idx),		\
		SIDE_ATTR_VAL_PFX(_ctx, _idx),				\
		SIDE_ATTR_ARRAY_SYM(_ctx), SIDE_IDX_NUM(_idx),		\
		SIDE_UNPACK _attr)
#define SIDE_ATTR_DECLARE_2(_ksym, _vpfx, _attrs, _k, ...)		\
	SIDE_ATTR_DECLARE_3(_ksym, _vpfx, _attrs, _k, __VA_ARGS__)
#define SIDE_ATTR_DECLARE_3(_ksym, _vpfx, _attrs, _k, _key, _value)	\
	SIDE_ATTR_STRING_OBJECT(_ksym, _key)				\
	SIDE_PTR_REL_DEFINE_AT(SIDE_CAT3(_ksym, _off, ), _attrs,	\
		(_k) * sizeof(struct side_attr)				\
			+ offsetof(struct side_attr, key.p.u), _ksym)	\
	SIDE_ATTR_VALUE_DECLARE(_attrs,					\
		(_k) * sizeof(struct side_attr)				\
			+ offsetof(struct side_attr, value),		\
		_vpfx, _value)

#define SIDE_ATTR_INIT(_ctx, _idx, _attr)				\
	SIDE_CAT2(SIDE_ATTR_INIT_, SIDE_IS_PAREN(_attr))(_ctx, _idx, _attr)

#define SIDE_ATTR_INIT_0(_ctx, _idx, _attr)

#define SIDE_ATTR_INIT_1(_ctx, _idx, _attr)				\
	SIDE_ATTR_INIT_2(SIDE_ATTR_KEY_SYM(_ctx, _idx),			\
		SIDE_ATTR_VAL_PFX(_ctx, _idx), SIDE_UNPACK _attr)
#define SIDE_ATTR_INIT_2(_ksym, _vpfx, ...)				\
	SIDE_ATTR_INIT_3(_ksym, _vpfx, __VA_ARGS__)
#define SIDE_ATTR_INIT_3(_ksym, _vpfx, _key, _value)			\
	{								\
		.key = {						\
			.p = SIDE_PTR_SEL_REL_INIT(SIDE_CAT3(_ksym, _off, )), \
			.unit_size = sizeof(uint8_t),			\
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		},							\
		.value = SIDE_ATTR_VALUE_INIT(_vpfx, _value),		\
	}

/* The same, where it stands, for a dynamic argument. */
#define SIDE_DYNAMIC_ATTR_INIT(_ctx, _idx, _attr)			\
	SIDE_CAT2(SIDE_DYNAMIC_ATTR_INIT_, SIDE_IS_PAREN(_attr))(_ctx, _idx, _attr)

#define SIDE_DYNAMIC_ATTR_INIT_0(_ctx, _idx, _attr)

#define SIDE_DYNAMIC_ATTR_INIT_1(_ctx, _idx, _attr)			\
	SIDE_DYNAMIC_ATTR_INIT_2(SIDE_UNPACK _attr)
#define SIDE_DYNAMIC_ATTR_INIT_2(...)	SIDE_DYNAMIC_ATTR_INIT_3(__VA_ARGS__)
#define SIDE_DYNAMIC_ATTR_INIT_3(_key, _value)				\
	{								\
		.key = {						\
			.p = SIDE_PTR_SEL_INIT(_key),			\
			.unit_size = sizeof(uint8_t),			\
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		},							\
		.value = SIDE_DYNAMIC_ATTR_VALUE_INIT(_value),		\
	}

#define _side_dynamic_attr_list(...)					\
	SIDE_DYNAMIC_LITERAL_ARRAY_SEL(const struct side_attr,		\
		SIDE_MAP_LIST_IDX_P(SIDE_DYNAMIC_ATTR_INIT, _,		\
			SIDE_UNPACK ( __VA_ARGS__ )))

/*
 * The attributes of _ctx: the strings they hold, then the array itself,
 * which is named before them because the distance to each string is
 * measured from a byte of it, and last the distance to the array, from
 * byte _off of _obj where whatever holds it sits.
 */
#define SIDE_ATTRS_DECLARE(_obj, _off, _ctx, _attrs)			\
	SIDE_ATTR_ARRAY_DECLARE(SIDE_ATTR_ARRAY_SYM(_ctx))		\
	SIDE_MAP_IDX2_P(SIDE_ATTR_DECLARE, _ctx, SIDE_UNPACK _attrs)	\
	SIDE_ATTR_ARRAY_OBJECT(SIDE_ATTR_ARRAY_SYM(_ctx),		\
		{ SIDE_MAP_LIST_IDX2_P(SIDE_ATTR_INIT, _ctx, SIDE_UNPACK _attrs) }) \
	SIDE_PTR_REL_DEFINE_AT(SIDE_ATTR_ARRAY_OFF(_ctx), _obj, _off,	\
		SIDE_ATTR_ARRAY_SYM(_ctx))

/*
 * Where those attributes are. A type which a dynamic argument also
 * builds says which of the two it holds; everything else in a
 * description holds a distance and nothing else. See side_ptr_sel_t.
 */
#define SIDE_ATTRS_REF(_ctx)						\
	SIDE_LITERAL_ARRAY_SEL_REL_OF_NAMED(SIDE_ATTR_ARRAY_OFF(_ctx),	\
		SIDE_ATTR_ARRAY_SYM(_ctx))

#define SIDE_ATTRS_REL_REF(_ctx)					\
	SIDE_LITERAL_ARRAY_REL_OF_NAMED(SIDE_ATTR_ARRAY_OFF(_ctx),	\
		SIDE_ATTR_ARRAY_SYM(_ctx))

#define _side_attr_u8(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U8), .u = { .integer_value = { .side_u8 = (_val) } } })
#define _side_attr_u16(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U16), .u = { .integer_value = { .side_u16 = (_val) } } })
#define _side_attr_u32(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U32), .u = { .integer_value = { .side_u32 = (_val) } } })
#define _side_attr_u64(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U64), .u = { .integer_value = { .side_u64 = (_val) } } })
#define _side_attr_u128(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U128), .u = { .integer_value = { .side_u128 = (_val) } } })
#define _side_attr_s8(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S8), .u = { .integer_value = { .side_s8 = (_val) } } })
#define _side_attr_s16(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S16), .u = { .integer_value = { .side_s16 = (_val) } } })
#define _side_attr_s32(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S32), .u = { .integer_value = { .side_s32 = (_val) } } })
#define _side_attr_s64(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S64), .u = { .integer_value = { .side_s64 = (_val) } } })
#define _side_attr_s128(_val)		(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S128), .u = { .integer_value = { .side_s128 = (_val) } } })
#define _side_attr_float_binary16(_val)	(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY16), .u = { .float_value = { .side_float_binary16 = (_val) } } })
#define _side_attr_float_binary32(_val)	(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY32), .u = { .float_value = { .side_float_binary32 = (_val) } } })
#define _side_attr_float_binary64(_val)	(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY64), .u = { .float_value = { .side_float_binary64 = (_val) } } })
#define _side_attr_float_binary128(_val)	(SIDE_AK_BARE, { .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY128), .u = { .float_value = { .side_float_binary128 = (_val) } } })

#define _side_attr_string(_val)		__side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t))
#define _side_attr_string16(_val)	__side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t))
#define _side_attr_string32(_val)	__side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t))

/* Stack-copy enumeration type definitions */

/*
 * A mapping, as its label and the range it covers rather than as an
 * initializer: the label is a string, which has to become an object in
 * the section a description lives in, and only the site defining the
 * enumeration can declare one. See _side_attr().
 */
#define side_enum_mapping_range(_label, _begin, _end)	(_label, _begin, _end)
#define side_enum_mapping_value(_label, _value)		(_label, _value, _value)

#define side_enum_bitmap_mapping_range(_label, _begin, _end)	(_label, _begin, _end)
#define side_enum_bitmap_mapping_value(_label, _value)		(_label, _value, _value)

/* The mappings, as a list that site can walk. */
#define side_enum_mapping_list(...)		( __VA_ARGS__ )
#define side_enum_bitmap_mapping_list(...)	( __VA_ARGS__ )

/*
 * Where the label of mapping _k of an array sits, in bytes. The two
 * kinds of mapping differ in the range they cover and not in where the
 * label is, so the array says which it is.
 */
#define SIDE_MAPPING_LABEL_AT(_mappings, _k)				\
	((_k) * sizeof((_mappings)[0])					\
		+ offsetof(__typeof__((_mappings)[0]), label.p.u))

/* Where the mappings of _ctx live, and the label of each of them. */
#define SIDE_MAPPING_ARRAY_SYM(_ctx)		SIDE_CAT3(_ctx, __mappings, )
#define SIDE_MAPPING_ARRAY_OFF(_ctx)		SIDE_CAT3(_ctx, __mappings_off, )
#define SIDE_MAPPING_LABEL_SYM(_ctx, _idx)	SIDE_CAT3(_ctx, __label_, _idx)
#define SIDE_MAPPING_LABEL_OFF(_ctx, _idx)	SIDE_CAT3(_ctx, __label_off_, _idx)

/* The array of mappings, in the section and not const, as the rest is. */
#ifdef __cplusplus
#  define SIDE_MAPPING_ARRAY_DECLARE(_type, _name)			\
	namespace {							\
		extern _type __attribute__((section("side_event_description"))) \
			_name[] SIDE_ASM_LABEL(_name);			\
	}
#  define SIDE_MAPPING_ARRAY_OBJECT(_type, _name, _init...)		\
	namespace {							\
		_type __attribute__((section("side_event_description"), used)) \
			_name[] SIDE_ASM_LABEL(_name) = _init;		\
	}
#else
#  define SIDE_MAPPING_ARRAY_DECLARE(_type, _name)			\
	static _type __attribute__((section("side_event_description")))	\
		_name[] SIDE_ASM_LABEL(_name);
#  define SIDE_MAPPING_ARRAY_OBJECT(_type, _name, _init...)		\
	static _type __attribute__((section("side_event_description"), used)) \
		_name[] SIDE_ASM_LABEL(_name) = _init;
#endif

/* One mapping: its label, put where a distance to it can be folded. */
#define SIDE_MAPPING_DECLARE(_ctx, _idx, _mapping)			\
	SIDE_CAT2(SIDE_MAPPING_DECLARE_, SIDE_IS_PAREN(_mapping))(_ctx, _idx, _mapping)

#define SIDE_MAPPING_DECLARE_0(_ctx, _idx, _mapping)

#define SIDE_MAPPING_DECLARE_1(_ctx, _idx, _mapping)			\
	SIDE_MAPPING_DECLARE_2(SIDE_MAPPING_LABEL_SYM(_ctx, _idx),	\
		SIDE_MAPPING_ARRAY_SYM(_ctx), SIDE_IDX_NUM(_idx),	\
		SIDE_UNPACK _mapping)
#define SIDE_MAPPING_DECLARE_2(_sym, _mappings, _k, ...)		\
	SIDE_MAPPING_DECLARE_3(_sym, _mappings, _k, __VA_ARGS__)
#define SIDE_MAPPING_DECLARE_3(_sym, _mappings, _k, _label, _begin, _end) \
	SIDE_ATTR_STRING_OBJECT(_sym, _label)				\
	SIDE_PTR_REL_DEFINE_AT(SIDE_CAT3(_sym, _off, ), _mappings,	\
		SIDE_MAPPING_LABEL_AT(_mappings, _k), _sym)

#define SIDE_MAPPING_INIT(_ctx, _idx, _mapping)				\
	SIDE_CAT2(SIDE_MAPPING_INIT_, SIDE_IS_PAREN(_mapping))(_ctx, _idx, _mapping)

#define SIDE_MAPPING_INIT_0(_ctx, _idx, _mapping)

#define SIDE_MAPPING_INIT_1(_ctx, _idx, _mapping)			\
	SIDE_MAPPING_INIT_2(SIDE_MAPPING_LABEL_SYM(_ctx, _idx), SIDE_UNPACK _mapping)
#define SIDE_MAPPING_INIT_2(_sym, ...)		SIDE_MAPPING_INIT_3(_sym, __VA_ARGS__)
#define SIDE_MAPPING_INIT_3(_sym, _label, _begin, _end)			\
	{								\
		.range_begin = _begin,					\
		.range_end = _end,					\
		.label = {						\
			.p = SIDE_PTR_SEL_REL_INIT(SIDE_CAT3(_sym, _off, )), \
			.unit_size = sizeof(uint8_t),			\
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		},							\
	}

/*
 * An enumeration: its labels, its mappings and its attributes all in
 * the section a description lives in, which makes it several
 * declarations rather than one, so the linkage is part of the name as
 * it is for a structure.
 */
#define __side_define_enum(_type, _mappings_type, _forward_decl_linkage, _linkage, _identifier, _mappings, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage _mappings_type __attribute__((section("side_event_description"))) \
		_identifier SIDE_ASM_LABEL(_identifier);		\
	SIDE_MAPPING_ARRAY_DECLARE(_type, SIDE_MAPPING_ARRAY_SYM(_identifier)) \
	SIDE_MAP_IDX_P(SIDE_MAPPING_DECLARE, _identifier, SIDE_UNPACK _mappings) \
	SIDE_MAPPING_ARRAY_OBJECT(_type, SIDE_MAPPING_ARRAY_SYM(_identifier), \
		{ SIDE_MAP_LIST_IDX_P(SIDE_MAPPING_INIT, _identifier, SIDE_UNPACK _mappings) }) \
	SIDE_PTR_REL_DEFINE_AT(SIDE_MAPPING_ARRAY_OFF(_identifier),	\
		_identifier, offsetof(_mappings_type, mappings.elements), \
		SIDE_MAPPING_ARRAY_SYM(_identifier))			\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(_mappings_type, attributes.elements), _identifier, \
		SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))	\
	_linkage _mappings_type __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier) = {		\
		.mappings = SIDE_LITERAL_ARRAY_REL_OF_NAMED(SIDE_MAPPING_ARRAY_OFF(_identifier), \
			SIDE_MAPPING_ARRAY_SYM(_identifier)),		\
		.attributes = SIDE_ATTRS_REL_REF(_identifier),		\
	};								\
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

#ifdef __cplusplus
#  define _side_static_define_enum(_identifier, _mappings, _attr...)	\
	namespace {							\
		__side_define_enum(struct side_enum_mapping,		\
			struct side_enum_mappings, extern, , _identifier, \
			SIDE_PARAM(_mappings), ##_attr);		\
	}
#  define _side_static_define_enum_bitmap(_identifier, _mappings, _attr...) \
	namespace {							\
		__side_define_enum(struct side_enum_bitmap_mapping,	\
			struct side_enum_bitmap_mappings, extern, ,	\
			_identifier,					\
			SIDE_PARAM(_mappings), ##_attr);		\
	}
#else
#  define _side_static_define_enum(_identifier, _mappings, _attr...)	\
	__side_define_enum(struct side_enum_mapping,			\
		struct side_enum_mappings, static, static, _identifier,	\
		SIDE_PARAM(_mappings), ##_attr)
#  define _side_static_define_enum_bitmap(_identifier, _mappings, _attr...) \
	__side_define_enum(struct side_enum_bitmap_mapping,		\
		struct side_enum_bitmap_mappings, static, static,	\
		_identifier,						\
		SIDE_PARAM(_mappings), ##_attr)
#endif

#define side_define_enum(_identifier, _mappings, _attr...)		\
	__side_define_enum(struct side_enum_mapping,			\
		struct side_enum_mappings, extern, , _identifier,	\
		SIDE_PARAM(_mappings), ##_attr)

#define _side_define_enum_bitmap(_identifier, _mappings, _attr...)	\
	__side_define_enum(struct side_enum_bitmap_mapping,		\
		struct side_enum_bitmap_mappings, extern, , _identifier, \
		SIDE_PARAM(_mappings), ##_attr)

/* Stack-copy field and type definitions */

#define _side_type_null(_attr...)					\
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()),	\
		side_null, _side_type_null_init)
#define _side_type_null_init(_pfx)					\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_NULL),			\
		.u = {							\
			.side_null = {					\
				.attributes = SIDE_ATTRS_REF(_pfx),	\
			},						\
		},							\
	}

#define _side_type_bool(_attr...)					\
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()),	\
		side_bool, _side_type_bool_init)
#define _side_type_bool_init(_pfx)					\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_BOOL),			\
		.u = {							\
			.side_bool = {					\
				.attributes = SIDE_ATTRS_REF(_pfx),	\
				.bool_size = sizeof(uint8_t),		\
				.len_bits = 0,				\
				.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
			},						\
		},							\
	}

#define _side_type_byte(_attr...)					\
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()),	\
		side_byte, _side_type_byte_init)
#define _side_type_byte_init(_pfx)					\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_BYTE),			\
		.u = {							\
			.side_byte = {					\
				.attributes = SIDE_ATTRS_REF(_pfx),	\
			},						\
		},							\
	}

#define __side_type_string(_type, _byte_order, _unit_size, _attr)	\
	(SIDE_TK_LEAF, _attr, side_string, __side_type_string_init,	\
		_type, _byte_order, _unit_size)
#define __side_type_string_init(_pfx, _type, _byte_order, _unit_size)	\
	{								\
		.type = SIDE_ENUM_INIT(_type),				\
		.u = {							\
			.side_string = {				\
				.attributes = SIDE_ATTRS_REF(_pfx),	\
				.unit_size = _unit_size,		\
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			},						\
		},							\
	}

/* The only type which is given no attributes: it holds nothing at all. */
#define _side_type_dynamic()						\
	(SIDE_TK_BARE,							\
		{							\
			.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC),	\
			.u = { }					\
		})

#define _side_type_integer(_type, _signedness, _byte_order, _integer_size, _len_bits, _attr) \
	(SIDE_TK_LEAF, _attr, side_integer, _side_type_integer_init,	\
		_type, _signedness, _byte_order, _integer_size, _len_bits)
#define _side_type_integer_init(_pfx, _type, _signedness, _byte_order, _integer_size, _len_bits) \
	{								\
		.type = SIDE_ENUM_INIT(_type),				\
		.u = {							\
			.side_integer = {				\
				.attributes = SIDE_ATTRS_REF(_pfx),	\
				.integer_size = _integer_size,		\
				.len_bits = _len_bits,			\
				.signedness = _signedness,		\
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			},						\
		},							\
	}

#define __side_type_float(_type, _byte_order, _float_size, _attr)	\
	(SIDE_TK_LEAF, _attr, side_float, __side_type_float_init,	\
		_type, _byte_order, _float_size)
#define __side_type_float_init(_pfx, _type, _byte_order, _float_size)	\
	{								\
		.type = SIDE_ENUM_INIT(_type),				\
		.u = {							\
			.side_float = {					\
				.attributes = SIDE_ATTRS_REF(_pfx),	\
				.float_size = _float_size,		\
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			},						\
		},							\
	}

/*
 * A type, as what it is rather than as an initializer.
 *
 * A type which points at another one needs that other one to be a named
 * object in the section a description lives in, so that the distance to
 * it is one the assembler folds. Only the site storing a type knows
 * where that type is going -- at which byte of which object -- and so
 * where what it points at has to be measured from. A type therefore
 * hands over what it is, and that site puts it together, the way a
 * field hands over its name and its type rather than an initializer.
 *
 * A type is a parenthesized (kind, ...): parenthesized, so that it is
 * one macro argument however many commas its initializer holds. The
 * site storing it puts it together with
 *
 *   SIDE_TYPE_DECLARE_L0(_obj, _off, _pfx, _type)
 *      what _type needs beside it, given that the struct side_type it
 *      builds lands at byte _off of the object _obj, and that what it
 *      puts in the section may be named _pfx-something;
 *
 *   SIDE_TYPE_INIT(_pfx, _type)
 *      the initializer of that struct side_type, reaching it.
 *
 * The two walks have to name the same objects, so _pfx is built from
 * the position of what is being walked and never from __COUNTER__, for
 * the reason SIDE_MAP_IDX() gives.
 *
 * A type which puts another one in the section has to declare that one
 * in turn, and a macro cannot expand within its own expansion, so the
 * declaring walk goes down a ladder of rungs rather than recursing: a
 * type at _L0 declares what it holds at _L1. A type nested more deeply
 * than the last rung says so as SIDE_TYPE_NESTED_TOO_DEEPLY; give it a
 * name of its own with side_static_define_struct(), _array(), _vla(),
 * _optional() or _variant() and refer to that, which starts the ladder
 * over.
 *
 * The kinds, and what follows each of them in the tuple:
 *
 *   SIDE_TK_LEAF	the initializer; the type holds no other
 *   SIDE_TK_PTR	label, member, target (an object with a name)
 *   SIDE_TK_ENUM	label, member, mappings, element type
 *   SIDE_TK_GSTRUCT	target, offset, size, access mode
 *   SIDE_TK_GARRAY	element type, length, offset, access mode, attributes
 *   SIDE_TK_GVLA	element type, offset, access mode, length type, attributes
 *   SIDE_TK_OPTLIT	element type, attributes (an optional with no name)
 *
 * The kind is pasted onto the name of the macro which takes it and is
 * never expanded, which is what keeps a macro of the same name
 * elsewhere in the program from being substituted for it.
 */

/* The initializer of a type, reaching what it holds. */
#define SIDE_TYPE_INIT(_pfx, _type)					\
	SIDE_TYPE_INIT_1(_pfx, SIDE_UNPACK _type)
#define SIDE_TYPE_INIT_1(_pfx, ...)	SIDE_TYPE_INIT_2(_pfx, __VA_ARGS__)
#define SIDE_TYPE_INIT_2(_pfx, _kind, ...)				\
	SIDE_TYPE_INIT_ ## _kind(_pfx, __VA_ARGS__)

/* A type which holds nothing at all, not even attributes. */
#define SIDE_TYPE_INIT_SIDE_TK_BARE(_pfx, ...)		__VA_ARGS__

/*
 * A type which holds no other type, but does hold attributes: the
 * initializer it hands over is a macro rather than tokens, because it
 * has to reach an array only the storing site can name.
 */
#define SIDE_TYPE_INIT_SIDE_TK_LEAF(_pfx, _attrs, _path, _maker, ...)	\
	_maker(_pfx, ##__VA_ARGS__)

#define SIDE_TYPE_INIT_SIDE_TK_PTR(_pfx, _label, _member, _target)	\
	{								\
		.type = SIDE_ENUM_INIT(_label),				\
		.u = {							\
			._member = SIDE_PTR_REL_INIT(SIDE_TYPE_PTR_OFF(_pfx)), \
		},							\
	}

#define SIDE_TYPE_INIT_SIDE_TK_ENUM(_pfx, _label, _member, _mappings, _elem) \
	{								\
		.type = SIDE_ENUM_INIT(_label),				\
		.u = {							\
			._member = {					\
				.mappings = SIDE_PTR_REL_INIT(SIDE_TYPE_MAPPINGS_OFF(_pfx)), \
				.elem_type = SIDE_PTR_REL_INIT(SIDE_TYPE_ELEM_OFF(_pfx)), \
			},						\
		},							\
	}

#define SIDE_TYPE_INIT_SIDE_TK_GSTRUCT(_pfx, _target, _offset, _size, _access_mode) \
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRUCT),	\
		.u = {							\
			.side_gather = {				\
				.u = {					\
					.side_struct = {		\
						.type = SIDE_PTR_REL_INIT(SIDE_TYPE_PTR_OFF(_pfx)), \
						.offset = _offset,	\
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.size = _size,		\
					},				\
				},					\
			},						\
		},							\
	}

#define SIDE_TYPE_INIT_SIDE_TK_GARRAY(_pfx, _elem, _length, _offset, _access_mode, _attr...) \
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ARRAY),		\
		.u = {							\
			.side_gather = {				\
				.u = {					\
					.side_array = {			\
						.offset = _offset,	\
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = {		\
							.elem_type = SIDE_PTR_REL_INIT(SIDE_TYPE_ELEM_OFF(_pfx)), \
							.length = _length, \
							.attributes = SIDE_ATTRS_REL_REF(_pfx), \
						},			\
					},				\
				},					\
			},						\
		},							\
	}

#define SIDE_TYPE_INIT_SIDE_TK_GVLA(_pfx, _elem, _offset, _access_mode, _length, _attr...) \
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_VLA),		\
		.u = {							\
			.side_gather = {				\
				.u = {					\
					.side_vla = {			\
						.offset = _offset,	\
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = {		\
							.elem_type = SIDE_PTR_REL_INIT(SIDE_TYPE_ELEM_OFF(_pfx)), \
							.length_type = SIDE_PTR_REL_INIT(SIDE_TYPE_LEN_OFF(_pfx)), \
							.attributes = SIDE_ATTRS_REL_REF(_pfx), \
						},			\
					},				\
				},					\
			},						\
		},							\
	}

#define SIDE_TYPE_INIT_SIDE_TK_OPTLIT(_pfx, _elem, _attr...)		\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_OPTIONAL),		\
		.u = {							\
			.side_optional = SIDE_PTR_REL_INIT(SIDE_TYPE_PTR_OFF(_pfx)), \
		},							\
	}

/* What a type puts in the section, and under which name. */
#define SIDE_TYPE_ELEM_SYM(_pfx)	SIDE_CAT3(_pfx, __elem, )
#define SIDE_TYPE_LEN_SYM(_pfx)		SIDE_CAT3(_pfx, __len, )
#define SIDE_TYPE_OPT_SYM(_pfx)		SIDE_CAT3(_pfx, __opt, )

/* Where the distance from a type to each of those lives. */
#define SIDE_TYPE_PTR_OFF(_pfx)		SIDE_CAT3(_pfx, __ptr_off, )
#define SIDE_TYPE_MAPPINGS_OFF(_pfx)	SIDE_CAT3(_pfx, __mappings_off, )
#define SIDE_TYPE_ELEM_OFF(_pfx)	SIDE_CAT3(_pfx, __elem_off, )
#define SIDE_TYPE_LEN_OFF(_pfx)		SIDE_CAT3(_pfx, __len_off, )

/*
 * The distance from the member of a struct side_type which points at
 * something to where that something is. The member is named twice: for
 * the byte it sits at, and for what the initializer writes to.
 */
#define SIDE_TYPE_PTR_DEFINE(_sym, _obj, _off, _member, _target)		\
	SIDE_PTR_REL_DEFINE_AT(_sym, _obj,				\
		(_off) + offsetof(struct side_type, u._member), _target)

/*
 * One type in an object of its own, in the section a description is in
 * and not const, which is what lets the assembler fold a distance to
 * it. See side_ptr_rel_t.
 *
 * It is named before it is defined, because a distance measured from it
 * is named before it too: what the type points at is declared between
 * the two. In C that is a tentative definition; C++ has none, so the
 * object is given external linkage within an anonymous namespace, which
 * is what an event and a structure do for the same reason.
 */
#ifdef __cplusplus
#  define SIDE_TYPE_OBJECT_DECLARE(_name)				\
	namespace {							\
		extern struct side_type __attribute__((section("side_event_description"))) \
			_name SIDE_ASM_LABEL(_name);			\
	}
#  define SIDE_TYPE_OBJECT(_name, _init...)				\
	namespace {							\
		struct side_type __attribute__((section("side_event_description"), used)) \
			_name SIDE_ASM_LABEL(_name) = _init;		\
	}
#else
#  define SIDE_TYPE_OBJECT_DECLARE(_name)				\
	static struct side_type __attribute__((section("side_event_description"))) \
		_name SIDE_ASM_LABEL(_name);
#  define SIDE_TYPE_OBJECT(_name, _init...)				\
	static struct side_type __attribute__((section("side_event_description"), used)) \
		_name SIDE_ASM_LABEL(_name) = _init;
#endif

/* The same, for the optional a field carries rather than names. */
#ifdef __cplusplus
#  define SIDE_TYPE_OPTIONAL_OBJECT_DECLARE(_name)			\
	namespace {							\
		extern struct side_type_optional __attribute__((section("side_event_description"))) \
			_name SIDE_ASM_LABEL(_name);			\
	}
#  define SIDE_TYPE_OPTIONAL_OBJECT(_name, _elem_off, _attr...)		\
	namespace {							\
		struct side_type_optional __attribute__((section("side_event_description"), used)) \
			_name SIDE_ASM_LABEL(_name) = {			\
				.elem_type = SIDE_PTR_REL_INIT(_elem_off), \
				.attributes = _attr,			\
			};						\
	}
#else
#  define SIDE_TYPE_OPTIONAL_OBJECT_DECLARE(_name)			\
	static struct side_type_optional __attribute__((section("side_event_description"))) \
		_name SIDE_ASM_LABEL(_name);
#  define SIDE_TYPE_OPTIONAL_OBJECT(_name, _elem_off, _attr...)		\
	static struct side_type_optional __attribute__((section("side_event_description"), used)) \
		_name SIDE_ASM_LABEL(_name) = {				\
			.elem_type = SIDE_PTR_REL_INIT(_elem_off),	\
			.attributes = _attr,				\
		};
#endif

/*
 * Put a type in an object of its own and declare what it holds, one
 * rung further down the ladder.
 */
#define SIDE_TYPE_HOIST_L0(_name, _type)				\
	SIDE_TYPE_OBJECT_DECLARE(_name)					\
	SIDE_TYPE_DECLARE_L0(_name, 0, _name, _type)			\
	SIDE_TYPE_OBJECT(_name, SIDE_TYPE_INIT(_name, _type))
#define SIDE_TYPE_HOIST_L1(_name, _type)				\
	SIDE_TYPE_OBJECT_DECLARE(_name)					\
	SIDE_TYPE_DECLARE_L1(_name, 0, _name, _type)			\
	SIDE_TYPE_OBJECT(_name, SIDE_TYPE_INIT(_name, _type))
#define SIDE_TYPE_HOIST_L2(_name, _type)				\
	SIDE_TYPE_OBJECT_DECLARE(_name)					\
	SIDE_TYPE_DECLARE_L2(_name, 0, _name, _type)			\
	SIDE_TYPE_OBJECT(_name, SIDE_TYPE_INIT(_name, _type))
#define SIDE_TYPE_HOIST_L3(_name, _type)				\
	SIDE_TYPE_OBJECT_DECLARE(_name)					\
	SIDE_TYPE_DECLARE_L3(_name, 0, _name, _type)			\
	SIDE_TYPE_OBJECT(_name, SIDE_TYPE_INIT(_name, _type))

/* What a type needs beside it, dispatched on its kind, rung by rung. */
#define SIDE_TYPE_DECLARE_L0(_obj, _off, _pfx, _type)			\
	SIDE_TYPE_DECLARE_L0_1(_obj, _off, _pfx, SIDE_UNPACK _type)
#define SIDE_TYPE_DECLARE_L0_1(_obj, _off, _pfx, ...)			\
	SIDE_TYPE_DECLARE_L0_2(_obj, _off, _pfx, __VA_ARGS__)
#define SIDE_TYPE_DECLARE_L0_2(_obj, _off, _pfx, _kind, ...)		\
	SIDE_TYPE_DECLARE_L0_ ## _kind(_obj, _off, _pfx, __VA_ARGS__)

#define SIDE_TYPE_DECLARE_L1(_obj, _off, _pfx, _type)			\
	SIDE_TYPE_DECLARE_L1_1(_obj, _off, _pfx, SIDE_UNPACK _type)
#define SIDE_TYPE_DECLARE_L1_1(_obj, _off, _pfx, ...)			\
	SIDE_TYPE_DECLARE_L1_2(_obj, _off, _pfx, __VA_ARGS__)
#define SIDE_TYPE_DECLARE_L1_2(_obj, _off, _pfx, _kind, ...)		\
	SIDE_TYPE_DECLARE_L1_ ## _kind(_obj, _off, _pfx, __VA_ARGS__)

#define SIDE_TYPE_DECLARE_L2(_obj, _off, _pfx, _type)			\
	SIDE_TYPE_DECLARE_L2_1(_obj, _off, _pfx, SIDE_UNPACK _type)
#define SIDE_TYPE_DECLARE_L2_1(_obj, _off, _pfx, ...)			\
	SIDE_TYPE_DECLARE_L2_2(_obj, _off, _pfx, __VA_ARGS__)
#define SIDE_TYPE_DECLARE_L2_2(_obj, _off, _pfx, _kind, ...)		\
	SIDE_TYPE_DECLARE_L2_ ## _kind(_obj, _off, _pfx, __VA_ARGS__)

#define SIDE_TYPE_DECLARE_L3(_obj, _off, _pfx, _type)			\
	SIDE_TYPE_DECLARE_L3_1(_obj, _off, _pfx, SIDE_UNPACK _type)
#define SIDE_TYPE_DECLARE_L3_1(_obj, _off, _pfx, ...)			\
	SIDE_TYPE_DECLARE_L3_2(_obj, _off, _pfx, __VA_ARGS__)
#define SIDE_TYPE_DECLARE_L3_2(_obj, _off, _pfx, _kind, ...)		\
	SIDE_TYPE_DECLARE_L3_ ## _kind(_obj, _off, _pfx, __VA_ARGS__)

/* A type which holds nothing needs nothing beside it, at any rung. */
#define SIDE_TYPE_DECLARE_NOTHING(...)

#define SIDE_TYPE_DECLARE_L0_SIDE_TK_BARE	SIDE_TYPE_DECLARE_NOTHING
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_BARE	SIDE_TYPE_DECLARE_NOTHING
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_BARE	SIDE_TYPE_DECLARE_NOTHING
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_BARE	SIDE_TYPE_DECLARE_NOTHING

/*
 * A type which holds only attributes needs them beside it, which no
 * rung of the ladder changes: an attribute holds no type.
 */
#define SIDE_TYPE_DECLARE_LEAF(_obj, _off, _pfx, _attrs, _path, _maker, ...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type, u._path.attributes.elements), \
		_pfx, _attrs)

#define SIDE_TYPE_DECLARE_L0_SIDE_TK_LEAF	SIDE_TYPE_DECLARE_LEAF
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_LEAF	SIDE_TYPE_DECLARE_LEAF
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_LEAF	SIDE_TYPE_DECLARE_LEAF
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_LEAF	SIDE_TYPE_DECLARE_LEAF

/*
 * A type which points at an object someone else named needs only the
 * distance to it, which no rung of the ladder changes.
 */
#define SIDE_TYPE_DECLARE_PTR(_obj, _off, _pfx, _label, _member, _target) \
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_PTR_OFF(_pfx), _obj, _off, _member, _target)

#define SIDE_TYPE_DECLARE_L0_SIDE_TK_PTR	SIDE_TYPE_DECLARE_PTR
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_PTR	SIDE_TYPE_DECLARE_PTR
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_PTR	SIDE_TYPE_DECLARE_PTR
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_PTR	SIDE_TYPE_DECLARE_PTR

#define SIDE_TYPE_DECLARE_GSTRUCT(_obj, _off, _pfx, _target, _offset, _size, _access_mode) \
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_PTR_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_struct.type, _target)

#define SIDE_TYPE_DECLARE_L0_SIDE_TK_GSTRUCT	SIDE_TYPE_DECLARE_GSTRUCT
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_GSTRUCT	SIDE_TYPE_DECLARE_GSTRUCT
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_GSTRUCT	SIDE_TYPE_DECLARE_GSTRUCT
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_GSTRUCT	SIDE_TYPE_DECLARE_GSTRUCT

/*
 * The last rung. A type nested deeper than this has nowhere left to put
 * what it holds; give it a name of its own and refer to that.
 */
#define SIDE_TYPE_DECLARE_TOO_DEEP(_obj, _off, _pfx, ...)		\
	side_static_assert(0, "side: type nested too deeply; give it a name of its own", \
			SIDE_TYPE_NESTED_TOO_DEEPLY);

#define SIDE_TYPE_DECLARE_L3_SIDE_TK_ENUM	SIDE_TYPE_DECLARE_TOO_DEEP
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_GARRAY	SIDE_TYPE_DECLARE_TOO_DEEP
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_GVLA	SIDE_TYPE_DECLARE_TOO_DEEP
#define SIDE_TYPE_DECLARE_L3_SIDE_TK_OPTLIT	SIDE_TYPE_DECLARE_TOO_DEEP

/* An enumeration, which holds the type of what it maps. */
#define SIDE_TYPE_DECLARE_L0_SIDE_TK_ENUM(_obj, _off, _pfx, _label, _member, _mappings, _elem) \
	SIDE_TYPE_HOIST_L1(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		_member.elem_type, SIDE_TYPE_ELEM_SYM(_pfx))		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_MAPPINGS_OFF(_pfx), _obj, _off,	\
		_member.mappings, _mappings)
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_ENUM(_obj, _off, _pfx, _label, _member, _mappings, _elem) \
	SIDE_TYPE_HOIST_L2(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		_member.elem_type, SIDE_TYPE_ELEM_SYM(_pfx))		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_MAPPINGS_OFF(_pfx), _obj, _off,	\
		_member.mappings, _mappings)
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_ENUM(_obj, _off, _pfx, _label, _member, _mappings, _elem) \
	SIDE_TYPE_HOIST_L3(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		_member.elem_type, SIDE_TYPE_ELEM_SYM(_pfx))		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_MAPPINGS_OFF(_pfx), _obj, _off,	\
		_member.mappings, _mappings)

/* A gather array, which holds the type of its elements. */
#define SIDE_TYPE_DECLARE_L0_SIDE_TK_GARRAY(_obj, _off, _pfx, _elem, _length, _offset, _access_mode, _attr...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type,			\
			u.side_gather.u.side_array.type.attributes.elements),	\
		_pfx, _attr)					\
	SIDE_TYPE_HOIST_L1(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_array.type.elem_type,		\
		SIDE_TYPE_ELEM_SYM(_pfx))
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_GARRAY(_obj, _off, _pfx, _elem, _length, _offset, _access_mode, _attr...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type,			\
			u.side_gather.u.side_array.type.attributes.elements),	\
		_pfx, _attr)					\
	SIDE_TYPE_HOIST_L2(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_array.type.elem_type,		\
		SIDE_TYPE_ELEM_SYM(_pfx))
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_GARRAY(_obj, _off, _pfx, _elem, _length, _offset, _access_mode, _attr...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type,			\
			u.side_gather.u.side_array.type.attributes.elements),	\
		_pfx, _attr)					\
	SIDE_TYPE_HOIST_L3(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_array.type.elem_type,		\
		SIDE_TYPE_ELEM_SYM(_pfx))

/* A gather vla, which holds the type of its elements and of its length. */
#define SIDE_TYPE_DECLARE_L0_SIDE_TK_GVLA(_obj, _off, _pfx, _elem, _offset, _access_mode, _length, _attr...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type,			\
			u.side_gather.u.side_vla.type.attributes.elements),	\
		_pfx, _attr)					\
	SIDE_TYPE_HOIST_L1(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_HOIST_L1(SIDE_TYPE_LEN_SYM(_pfx), _length)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_vla.type.elem_type,			\
		SIDE_TYPE_ELEM_SYM(_pfx))				\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_LEN_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_vla.type.length_type,		\
		SIDE_TYPE_LEN_SYM(_pfx))
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_GVLA(_obj, _off, _pfx, _elem, _offset, _access_mode, _length, _attr...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type,			\
			u.side_gather.u.side_vla.type.attributes.elements),	\
		_pfx, _attr)					\
	SIDE_TYPE_HOIST_L2(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_HOIST_L2(SIDE_TYPE_LEN_SYM(_pfx), _length)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_vla.type.elem_type,			\
		SIDE_TYPE_ELEM_SYM(_pfx))				\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_LEN_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_vla.type.length_type,		\
		SIDE_TYPE_LEN_SYM(_pfx))
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_GVLA(_obj, _off, _pfx, _elem, _offset, _access_mode, _length, _attr...) \
	SIDE_ATTRS_DECLARE(_obj,					\
		(_off) + offsetof(struct side_type,			\
			u.side_gather.u.side_vla.type.attributes.elements),	\
		_pfx, _attr)					\
	SIDE_TYPE_HOIST_L3(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_TYPE_HOIST_L3(SIDE_TYPE_LEN_SYM(_pfx), _length)		\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_vla.type.elem_type,			\
		SIDE_TYPE_ELEM_SYM(_pfx))				\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_LEN_OFF(_pfx), _obj, _off,	\
		side_gather.u.side_vla.type.length_type,		\
		SIDE_TYPE_LEN_SYM(_pfx))

/*
 * An optional a field carries rather than names: the optional itself
 * goes in the section too, since nothing else gives it a name.
 */
#define SIDE_TYPE_DECLARE_L0_SIDE_TK_OPTLIT(_obj, _off, _pfx, _elem, _attr...) \
	SIDE_TYPE_OPTIONAL_OBJECT_DECLARE(SIDE_TYPE_OPT_SYM(_pfx))	\
	SIDE_ATTRS_DECLARE(SIDE_TYPE_OPT_SYM(_pfx),			\
		offsetof(struct side_type_optional, attributes.elements), \
		_pfx, _attr)						\
	SIDE_TYPE_HOIST_L1(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), SIDE_TYPE_OPT_SYM(_pfx), \
		struct side_type_optional, elem_type, SIDE_TYPE_ELEM_SYM(_pfx)) \
	SIDE_TYPE_OPTIONAL_OBJECT(SIDE_TYPE_OPT_SYM(_pfx),		\
		SIDE_TYPE_ELEM_OFF(_pfx), SIDE_ATTRS_REL_REF(_pfx))			\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_PTR_OFF(_pfx), _obj, _off,	\
		side_optional, SIDE_TYPE_OPT_SYM(_pfx))
#define SIDE_TYPE_DECLARE_L1_SIDE_TK_OPTLIT(_obj, _off, _pfx, _elem, _attr...) \
	SIDE_TYPE_OPTIONAL_OBJECT_DECLARE(SIDE_TYPE_OPT_SYM(_pfx))	\
	SIDE_ATTRS_DECLARE(SIDE_TYPE_OPT_SYM(_pfx),			\
		offsetof(struct side_type_optional, attributes.elements), \
		_pfx, _attr)						\
	SIDE_TYPE_HOIST_L2(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), SIDE_TYPE_OPT_SYM(_pfx), \
		struct side_type_optional, elem_type, SIDE_TYPE_ELEM_SYM(_pfx)) \
	SIDE_TYPE_OPTIONAL_OBJECT(SIDE_TYPE_OPT_SYM(_pfx),		\
		SIDE_TYPE_ELEM_OFF(_pfx), SIDE_ATTRS_REL_REF(_pfx))			\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_PTR_OFF(_pfx), _obj, _off,	\
		side_optional, SIDE_TYPE_OPT_SYM(_pfx))
#define SIDE_TYPE_DECLARE_L2_SIDE_TK_OPTLIT(_obj, _off, _pfx, _elem, _attr...) \
	SIDE_TYPE_OPTIONAL_OBJECT_DECLARE(SIDE_TYPE_OPT_SYM(_pfx))	\
	SIDE_ATTRS_DECLARE(SIDE_TYPE_OPT_SYM(_pfx),			\
		offsetof(struct side_type_optional, attributes.elements), \
		_pfx, _attr)						\
	SIDE_TYPE_HOIST_L3(SIDE_TYPE_ELEM_SYM(_pfx), _elem)		\
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_ELEM_OFF(_pfx), SIDE_TYPE_OPT_SYM(_pfx), \
		struct side_type_optional, elem_type, SIDE_TYPE_ELEM_SYM(_pfx)) \
	SIDE_TYPE_OPTIONAL_OBJECT(SIDE_TYPE_OPT_SYM(_pfx),		\
		SIDE_TYPE_ELEM_OFF(_pfx), SIDE_ATTRS_REL_REF(_pfx))			\
	SIDE_TYPE_PTR_DEFINE(SIDE_TYPE_PTR_OFF(_pfx), _obj, _off,	\
		side_optional, SIDE_TYPE_OPT_SYM(_pfx))

/*
 * A field, as the pair of what it is called and what it is, rather than
 * as an initializer.
 *
 * The name has to become an object of its own for the distance to it to
 * be one the assembler folds, and only the site defining the event or
 * the structure the field belongs to can declare one. So the field
 * hands both halves over and that site puts them back together, through
 * SIDE_FIELD_DECLARE() and SIDE_FIELD_INIT().
 */
#define _side_field(_name, _type)	(_name, _type)

/* Where the name of field _idx of _ctx lives, and the distance to it. */
#define SIDE_FIELD_NAME_SYM(_ctx, _idx)	SIDE_CAT3(_ctx, __field_name_, _idx)
#define SIDE_FIELD_NAME_OFF(_ctx, _idx)	SIDE_CAT3(_ctx, __field_name_off_, _idx)

/* Under which name the type of that field puts what it holds. */
#define SIDE_FIELD_TYPE_SYM(_ctx, _idx)	SIDE_CAT3(_ctx, __field_type_, _idx)

/*
 * Hoist the name of one field into the section its array is in, and
 * name the distance from the member which holds it to where it now is.
 * Neither end is const, and both are in one section, which is what lets
 * the assembler fold the distance. See side_ptr_rel_t.
 *
 * An element which is not parenthesized is the nothing an empty list,
 * or the trailing comma the DSL allows, leaves behind; there is no
 * field there to declare.
 */
#define SIDE_FIELD_DECLARE(_ctx, _idx, _field)				\
	SIDE_CAT2(SIDE_FIELD_DECLARE_, SIDE_IS_PAREN(_field))(_ctx, _idx, _field)

#define SIDE_FIELD_DECLARE_0(_ctx, _idx, _field)

#define SIDE_FIELD_DECLARE_1(_ctx, _idx, _field)			\
	SIDE_FIELD_DECLARE_2(SIDE_FIELD_NAME_SYM(_ctx, _idx),		\
		SIDE_FIELD_NAME_OFF(_ctx, _idx),			\
		SIDE_FIELD_TYPE_SYM(_ctx, _idx),			\
		SIDE_CAT3(_ctx, __fields, ), SIDE_IDX_NUM(_idx),		\
		SIDE_UNPACK _field)
#define SIDE_FIELD_DECLARE_2(_sym, _off, _tsym, _fields, _k, ...)	\
	SIDE_FIELD_DECLARE_3(_sym, _off, _tsym, _fields, _k, __VA_ARGS__)
#define SIDE_FIELD_DECLARE_3(_sym, _off, _tsym, _fields, _k, _name, _type) \
	static char __attribute__((section("side_event_description"), used)) \
		_sym[] SIDE_ASM_LABEL(_sym) = _name;			\
	SIDE_PTR_REL_DEFINE_AT(_off, _fields,				\
		(_k) * sizeof(struct side_event_field)			\
			+ offsetof(struct side_event_field, field_name), \
		_sym)							\
	SIDE_TYPE_DECLARE_L0(_fields,					\
		(_k) * sizeof(struct side_event_field)			\
			+ offsetof(struct side_event_field, side_type),	\
		_tsym, _type)

/* The array element, reaching its name by that distance. */
#define SIDE_FIELD_INIT(_ctx, _idx, _field)				\
	SIDE_CAT2(SIDE_FIELD_INIT_, SIDE_IS_PAREN(_field))(_ctx, _idx, _field)

#define SIDE_FIELD_INIT_0(_ctx, _idx, _field)

#define SIDE_FIELD_INIT_1(_ctx, _idx, _field)				\
	SIDE_FIELD_INIT_2(SIDE_FIELD_NAME_OFF(_ctx, _idx),		\
		SIDE_FIELD_TYPE_SYM(_ctx, _idx), SIDE_UNPACK _field)
#define SIDE_FIELD_INIT_2(_off, _tsym, ...)				\
	SIDE_FIELD_INIT_3(_off, _tsym, __VA_ARGS__)
#define SIDE_FIELD_INIT_3(_off, _tsym, _name, _type)			\
	{								\
		.field_name = SIDE_PTR_REL_INIT(_off),			\
		.side_type = SIDE_TYPE_INIT(_tsym, _type),		\
	}

/*
 * The fields of _ctx: the names hoisted out of them, then the array
 * itself. The array is declared before the names because the distance
 * to each of them is measured from a byte of it.
 */
#define SIDE_FIELDS_DECLARE(_ctx, _fields)				\
	SIDE_MAP_IDX_P(SIDE_FIELD_DECLARE, _ctx, SIDE_UNPACK _fields)

#define SIDE_FIELDS_INIT(_ctx, _fields)					\
	{ SIDE_MAP_LIST_IDX_P(SIDE_FIELD_INIT, _ctx, SIDE_UNPACK _fields) }

/*
 * An option of a variant, as the range it covers and what it is, rather
 * than as an initializer: the type it holds goes where the site
 * defining the variant puts it, as a field's does.
 */
#define _side_option_range(_range_begin, _range_end, _type)		\
	(_range_begin, _range_end, _type)

#define _side_option(_value, _type)					\
	_side_option_range(_value, _value, _type)

/* Under which name the type of option _idx of _ctx puts what it holds. */
#define SIDE_OPTION_TYPE_SYM(_ctx, _idx)	SIDE_CAT3(_ctx, __option_type_, _idx)

/*
 * The type of one option, put at the byte of the array where the option
 * holding it lands. An element which is not parenthesized is the
 * nothing a list with no element, or the trailing comma the DSL allows,
 * leaves behind.
 */
#define SIDE_OPTION_DECLARE(_ctx, _idx, _option)				\
	SIDE_CAT2(SIDE_OPTION_DECLARE_, SIDE_IS_PAREN(_option))(_ctx, _idx, _option)

#define SIDE_OPTION_DECLARE_0(_ctx, _idx, _option)

#define SIDE_OPTION_DECLARE_1(_ctx, _idx, _option)			\
	SIDE_OPTION_DECLARE_2(SIDE_OPTION_TYPE_SYM(_ctx, _idx),		\
		SIDE_CAT3(_ctx, __options, ), SIDE_IDX_NUM(_idx),	\
		SIDE_UNPACK _option)
#define SIDE_OPTION_DECLARE_2(_tsym, _options, _k, ...)			\
	SIDE_OPTION_DECLARE_3(_tsym, _options, _k, __VA_ARGS__)
#define SIDE_OPTION_DECLARE_3(_tsym, _options, _k, _begin, _end, _type)	\
	SIDE_TYPE_DECLARE_L0(_options,					\
		(_k) * sizeof(struct side_variant_option)		\
			+ offsetof(struct side_variant_option, side_type), \
		_tsym, _type)

/* The array element, reaching what its type holds. */
#define SIDE_OPTION_INIT(_ctx, _idx, _option)				\
	SIDE_CAT2(SIDE_OPTION_INIT_, SIDE_IS_PAREN(_option))(_ctx, _idx, _option)

#define SIDE_OPTION_INIT_0(_ctx, _idx, _option)

#define SIDE_OPTION_INIT_1(_ctx, _idx, _option)				\
	SIDE_OPTION_INIT_2(SIDE_OPTION_TYPE_SYM(_ctx, _idx), SIDE_UNPACK _option)
#define SIDE_OPTION_INIT_2(_tsym, ...)	SIDE_OPTION_INIT_3(_tsym, __VA_ARGS__)
#define SIDE_OPTION_INIT_3(_tsym, _begin, _end, _type)			\
	{								\
		.range_begin = _begin,					\
		.range_end = _end,					\
		.side_type = SIDE_TYPE_INIT(_tsym, _type),		\
	}

/*
 * The options of _ctx: what their types hold, then the array itself,
 * which is declared before them because the distance to each of them is
 * measured from a byte of it.
 */
#define SIDE_OPTIONS_DECLARE(_ctx, _options)				\
	SIDE_MAP_IDX_P(SIDE_OPTION_DECLARE, _ctx, SIDE_UNPACK _options)

#define SIDE_OPTIONS_INIT(_ctx, _options)				\
	{ SIDE_MAP_LIST_IDX_P(SIDE_OPTION_INIT, _ctx, SIDE_UNPACK _options) }

/* Host endian */
#define _side_type_u8(_attr...)				_side_type_integer(SIDE_TYPE_U8, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u16(_attr...)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u32(_attr...)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u64(_attr...)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u128(_attr...)			_side_type_integer(SIDE_TYPE_U128, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s8(_attr...)				_side_type_integer(SIDE_TYPE_S8, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s16(_attr...)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s32(_attr...)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s64(_attr...)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s128(_attr...)			_side_type_integer(SIDE_TYPE_S128, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_pointer(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary16(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary32(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary64(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary128(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string(_attr...)			__side_type_string(SIDE_TYPE_STRING_UTF8, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string16(_attr...) 			__side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string32(_attr...)		 	__side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_null(_name, _attr...)		_side_field(_name, _side_type_null(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_bool(_name, _attr...)		_side_field(_name, _side_type_bool(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u8(_name, _attr...)			_side_field(_name, _side_type_u8(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u16(_name, _attr...)			_side_field(_name, _side_type_u16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u32(_name, _attr...)			_side_field(_name, _side_type_u32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u64(_name, _attr...)			_side_field(_name, _side_type_u64(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u128(_name, _attr...)		_side_field(_name, _side_type_u128(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s8(_name, _attr...)			_side_field(_name, _side_type_s8(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s16(_name, _attr...)			_side_field(_name, _side_type_s16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s32(_name, _attr...)			_side_field(_name, _side_type_s32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s64(_name, _attr...)			_side_field(_name, _side_type_s64(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s128(_name, _attr...)		_side_field(_name, _side_type_s128(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_byte(_name, _attr...)		_side_field(_name, _side_type_byte(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_pointer(_name, _attr...)		_side_field(_name, _side_type_pointer(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary16(_name, _attr...)	_side_field(_name, _side_type_float_binary16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary32(_name, _attr...)	_side_field(_name, _side_type_float_binary32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary64(_name, _attr...)	_side_field(_name, _side_type_float_binary64(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary128(_name, _attr...)	_side_field(_name, _side_type_float_binary128(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string(_name, _attr...)		_side_field(_name, _side_type_string(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string16(_name, _attr...)		_side_field(_name, _side_type_string16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string32(_name, _attr...)		_side_field(_name, _side_type_string32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_dynamic(_name)			_side_field(_name, _side_type_dynamic())

/* C native types. */

/*
 * The SIDE ABI specifies fixed sizes integers and floating points.  However, as
 * a convenience for C/C++, the SIDE C API supports C native types (e.g. char)
 * which are translated to their equivalent.
 *
 * Note that the translation of C * native types is toolchain dependent and
 * therefore could produce different * results.
 *
 * The main use case is for auto-generating SIDE events for public API of shared
 * libraries.
 */

#ifdef __CHAR_UNSIGNED__
#  define _side_field_char(_name, _attr...) _side_field_uchar(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_char(_args...) _side_arg_uchar(_args)
#  define _side_type_char _side_type_uchar
#else
#  define _side_field_char(_name, _attr...) _side_field_schar(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_char(_args...) _side_arg_schar(_args)
#  define _side_type_char _side_type_schar
#endif

#define _side_field_schar(_name, _attr...) _side_field_s8(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_arg_schar(_args...) _side_arg_s8(_args)
#define _side_type_schar _side_type_s8

#define _side_field_uchar(_name, _attr...) _side_field_u8(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_arg_uchar(_args...) _side_arg_u8(_args)
#define _side_type_uchar _side_type_u8

#if __SIZEOF_SHORT__ <= 2
#  define _side_field_short(_name, _attr...) _side_field_s16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_short(_args...) _side_arg_s16(_args)
#  define _side_type_short _side_type_s16
#  define _side_field_ushort(_name, _attr...) _side_field_u16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ushort(_args...) _side_arg_u16(_args)
#  define _side_type_ushort _side_type_u16
#elif __SIZEOF_SHORT__ <= 4
#  define _side_field_short(_name, _attr...) _side_field_s32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_short(_args...) _side_arg_s32(_args)
#  define _side_type_short _side_type_s32
#  define _side_field_ushort(_name, _attr...) _side_field_u32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ushort(_args...) _side_arg_u32(_args)
#  define _side_type_ushort _side_type_u32
#elif __SIZEOF_SHORT__ <= 8
#  define _side_field_short(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_short(_args...) _side_arg_s64(_args)
#  define _side_type_short _side_type_s64
#  define _side_field_ushort(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ushort(_args...) _side_arg_u64(_args)
#  define _side_type_ushort _side_type_u64
#else
#  define _side_field_short(...)					\
	side_static_assert(0, "Type `signed short int' is not supported", type__signed_short_int__is_not_supported)
#  define _side_arg_short(...)					\
	side_static_assert(0, "Type `signed short int' is not supported", type__signed_short_int__is_not_supported)
#  define _side_type_short(...)					\
	side_static_assert(0, "Type `signed short int' is not supported", type__signed_short_int__is_not_supported)
#  define _side_field_ushort(...)					\
	side_static_assert(0, "Type `unsigned short int' is not supported", type__unsigned_short_int__is_not_supported)
#  define _side_arg_ushort(...)						\
	side_static_assert(0, "Type `unsigned short int' is not supported", type__unsigned_short_int__is_not_supported)
#  define _side_type_ushort(...)					\
	side_static_assert(0, "Type `unsigned short int' is not supported", type__unsigned_short_int__is_not_supported)
#endif

#if __SIZEOF_INT__ <= 2
#  define _side_field_int(_name, _attr...) _side_field_s16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_int(_args...) _side_arg_s16(_args)
#  define _side_type_int _side_type_s16
#  define _side_field_uint(_name, _attr...) _side_field_u16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_uint(_args...) _side_arg_u16(_args)
#  define _side_type_uint _side_type_u16
#elif __SIZEOF_INT__ <= 4
#  define _side_field_int(_name, _attr...) _side_field_s32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_int(_args...) _side_arg_s32(_args)
#  define _side_type_int _side_type_s32
#  define _side_field_uint(_name, _attr...) _side_field_u32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_uint(_args...) _side_arg_u32(_args)
#  define _side_type_uint _side_type_u32
#elif __SIZEOF_INT__ <= 8
#  define _side_field_int(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_int(_args...) _side_arg_s64(_args)
#  define _side_type_int _side_type_s64
#  define _side_field_uint(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_uint(_args...) _side_arg_u64(_args)
#  define _side_type_uint _side_type_u64
#else
#  define _side_field_int(...)						\
	side_static_assert(0, "Type `signed int' is not supported", type__signed_int__is_not_supported)
#  define _side_arg_int(...)						\
	side_static_assert(0, "Type `signed int' is not supported", type__signed_int__is_not_supported)
#  define _side_type_int(...)						\
	side_static_assert(0, "Type `signed int' is not supported", type__signed_int__is_not_supported)
#  define _side_field_uint(...)						\
	side_static_assert(0, "Type `unsigned int' is not supported", type__unsigned_int__is_not_supported)
#  define _side_arg_uint(...)						\
	side_static_assert(0, "Type `unsigned int' is not supported", type__unsigned_int__is_not_supported)
#  define _side_type_uint(...)						\
	side_static_assert(0, "Type `unsigned int' is not supported", type__unsigned_int__is_not_supported)
#endif

#if __SIZEOF_LONG__ <= 4
#  define _side_field_long(_name, _attr...) _side_field_s32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_long(_args...) _side_arg_s32(_args)
#  define _side_type_long _side_type_s32
#  define _side_field_ulong(_name, _attr...) _side_field_u32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ulong(_args...) _side_arg_u32(_args)
#  define _side_type_ulong _side_type_u32
#elif __SIZEOF_LONG__ <= 8
#  define _side_field_long(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_long(_args...) _side_arg_s64(_args)
#  define _side_type_long _side_type_s64
#  define _side_field_ulong(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ulong(_args...) _side_arg_u64(_args)
#  define _side_type_ulong _side_type_u64
#else
#  define _side_field_long(...)						\
	side_static_assert(0, "Type `signed long int' is not supported", type__signed_long_int__is_not_supported)
#  define _side_arg_long(...)					\
	side_static_assert(0, "Type `signed long int' is not supported", type__signed_long_int__is_not_supported)
#  define _side_type_long(...)					\
	side_static_assert(0, "Type `signed long int' is not supported", type__signed_long_int__is_not_supported)
#  define _side_field_ulong(...)					\
	side_static_assert(0, "Type `unsigned long int' is not supported", type__unsigned_long_int__is_not_supported)
#  define _side_arg_ulong(...)						\
	side_static_assert(0, "Type `unsigned long int' is not supported", type__unsigned_long_int__is_not_supported)
#  define _side_type_ulong(...)						\
	side_static_assert(0, "Type `unsigned long int' is not supported", type__unsigned_long_int__is_not_supported)
#endif

#if __SIZEOF_LONG_LONG__ <= 8
#  define _side_field_long_long(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_long_long(_args...) _side_arg_s64(_args)
#  define _side_type_long_long _side_type_s64
#  define _side_field_ulong_long(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ulong_long(_args...) _side_arg_u64(_args)
#  define _side_type_ulong_long _side_type_u64
#else
#  define _side_field_long_long(...)					\
	side_static_assert(0, "Type `signed long long int' is not supported", type__signed_long_long_int__is_not_supported)
#  define _side_arg_long_long(...)					\
	side_static_assert(0, "Type `signed long long int' is not supported", type__signed_long_long_int__is_not_supported)
#  define _side_type_long_long(...)					\
	side_static_assert(0, "Type `signed long long int' is not supported", type__signed_long_long_int__is_not_supported)
#  define _side_field_ulong_long(...)					\
	side_static_assert(0, "Type `unsigned long long int' is not supported", type__unsigned_long_long_int__is_not_supported)
#  define _side_arg_ulong_long(...)					\
	side_static_assert(0, "Type `unsigned long long int' is not supported", type__unsigned_long_long_int__is_not_supported)
#  define _side_arg_ulong_long(...)					\
	side_static_assert(0, "Type `unsigned long long int' is not supported", type__unsigned_long_long_int__is_not_supported)
#endif

#if __SIZEOF_FLOAT__ <= 4 && __HAVE_FLOAT32
#  define _side_field_float(_name, _attr...) _side_field_float_binary32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_float(_args...) _side_arg_float_binary32(_args)
#  define _side_type_float _side_type_float_binary32
#elif __SIZEOF_FLOAT__ <= 8 && __HAVE_FLOAT64
#  define _side_field_float(_name, _attr...) _side_field_float_binary64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_float(_args...) _side_arg_float_binary64(_args)
#  define _side_type_float _side_type_float_binary64
#elif __SIZEOF_FLOAT__ <= 16 && __HAVE_FLOAT128
#  define _side_field_float(_name, _attr...) _side_field_float_binary128(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_float(_args...) _side_arg_float_binary128(_args)
#  define _side_type_float _side_type_float_binary128
#else
#  define _side_field_float(...)					\
	side_static_assert(0, "Type `float' is not supported", type__float__is_not_supported)
#  define _side_arg_float(...)					\
	side_static_assert(0, "Type `float' is not supported", type__float__is_not_supported)
#  define _side_type_float(...)					\
	side_static_assert(0, "Type `float' is not supported", type__float__is_not_supported)
#endif

#if __SIZEOF_DOUBLE__ <= 4 && __HAVE_FLOAT32
#  define _side_field_double(_name, _attr...) _side_field_float_binary32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_double(_args...) _side_arg_float_binary32(_args)
#  define _side_type_double _side_type_float_binary32
#elif __SIZEOF_DOUBLE__ <= 8 && __HAVE_FLOAT64
#  define _side_field_double(_name, _attr...) _side_field_float_binary64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_double(_args...) _side_arg_float_binary64(_args)
#  define _side_type_double _side_type_float_binary64
#elif __SIZEOF_DOUBLE__ <= 16 && __HAVE_FLOAT128
#  define _side_field_double(_name, _attr...) _side_field_double_binary128(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_double(_args...) _side_arg_float_binary128(_args)
#  define _side_type_double _side_type_float_binary128
#else
#  define _side_field_double(...)					\
	side_static_assert(0, "Type `double' is not supported", type__double__is_not_supported)
#  define _side_arg_double(...)						\
	side_static_assert(0, "Type `double' is not supported", type__double__is_not_supported)
#  define _side_type_double(...)					\
	side_static_assert(0, "Type `double' is not supported", type__double__is_not_supported)
#endif

#ifdef __SIZEOF_LONG_DOUBLE__
#  if __SIZEOF_LONG_DOUBLE__ <= 4 && __HAVE_FLOAT32
#    define _side_field_long_double(_name, _attr...) _side_field_float_binary32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#    define _side_arg_long_double(_args...) _side_arg_float_binary32(_args)
#    define _side_type_long_double _side_type_float_binary32
#  elif __SIZEOF_LONG_DOUBLE__ <= 8 && __HAVE_FLOAT64
#    define _side_field_long_double(_name, _attr...) _side_field_float_binary64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#    define _side_arg_long_double(_args...) _side_arg_float_binary64(_args)
#    define _side_type_long_double _side_type_float_binary64
#  elif __SIZEOF_LONG_DOUBLE__ <= 16 && __HAVE_FLOAT128
#    define _side_field_long_double(_name, _attr...) _side_field_float_binary128(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#    define _side_arg_long_double(_args...) _side_arg_float_binary128(_args)
#    define _side_type_long_double _side_type_float_binary128
#  else
#    define _side_field_long_double(...)					\
	side_static_assert(0, "Type `long double' is not supported", type__long_double__is_not_supported)
#    define _side_arg_long_double(...)					\
	side_static_assert(0, "Type `long double' is not supported", type__long_double__is_not_supported)
#    define _side_type_long_double(...)					\
	side_static_assert(0, "Type `long double' is not supported", type__long_double__is_not_supported)
#  endif
#endif	/* __SIZEOF_LONG_DOUBLE__ */

/* Little endian */
#define _side_type_u16_le(_attr...)			_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u32_le(_attr...)			_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u64_le(_attr...)			_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u128_le(_attr...)			_side_type_integer(SIDE_TYPE_U128, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s16_le(_attr...)			_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s32_le(_attr...)			_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s64_le(_attr...)			_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s128_le(_attr...)			_side_type_integer(SIDE_TYPE_S128, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_pointer_le(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary16_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary32_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary64_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary128_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string16_le(_attr...) 		__side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string32_le(_attr...)		 	__side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_u16_le(_name, _attr...)		_side_field(_name, _side_type_u16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u32_le(_name, _attr...)		_side_field(_name, _side_type_u32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u64_le(_name, _attr...)		_side_field(_name, _side_type_u64_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u128_le(_name, _attr...)		_side_field(_name, _side_type_u128_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s16_le(_name, _attr...)		_side_field(_name, _side_type_s16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s32_le(_name, _attr...)		_side_field(_name, _side_type_s32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s64_le(_name, _attr...)		_side_field(_name, _side_type_s64_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s128_le(_name, _attr...)		_side_field(_name, _side_type_s128_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_pointer_le(_name, _attr...)		_side_field(_name, _side_type_pointer_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary16_le(_name, _attr...)	_side_field(_name, _side_type_float_binary16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary32_le(_name, _attr...)	_side_field(_name, _side_type_float_binary32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary64_le(_name, _attr...)	_side_field(_name, _side_type_float_binary64_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary128_le(_name, _attr...)	_side_field(_name, _side_type_float_binary128_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string16_le(_name, _attr...)		_side_field(_name, _side_type_string16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string32_le(_name, _attr...)		_side_field(_name, _side_type_string32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

/* Big endian */
#define _side_type_u16_be(_attr...)			_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u32_be(_attr...)			_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u64_be(_attr...)			_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u128_be(_attr...)			_side_type_integer(SIDE_TYPE_U128, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s16_be(_attr...)			_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s32_be(_attr...)			_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s64_be(_attr...)			_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s128_be(_attr...)			_side_type_integer(SIDE_TYPE_S128, true, SIDE_TYPE_BYTE_ORDER_BE, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_pointer_be(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary16_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary32_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary64_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary128_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string16_be(_attr...) 		__side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string32_be(_attr...)		 	__side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_u16_be(_name, _attr...)		_side_field(_name, _side_type_u16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u32_be(_name, _attr...)		_side_field(_name, _side_type_u32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u64_be(_name, _attr...)		_side_field(_name, _side_type_u64_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u128_be(_name, _attr...)		_side_field(_name, _side_type_u128_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s16_be(_name, _attr...)		_side_field(_name, _side_type_s16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s32_be(_name, _attr...)		_side_field(_name, _side_type_s32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s64_be(_name, _attr...)		_side_field(_name, _side_type_s64_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s128_be(_name, _attr...)		_side_field(_name, _side_type_s128_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_pointer_be(_name, _attr...)		_side_field(_name, _side_type_pointer_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary16_be(_name, _attr...)	_side_field(_name, _side_type_float_binary16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary32_be(_name, _attr...)	_side_field(_name, _side_type_float_binary32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary64_be(_name, _attr...)	_side_field(_name, _side_type_float_binary64_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary128_be(_name, _attr...)	_side_field(_name, _side_type_float_binary128_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string16_be(_name, _attr...)		_side_field(_name, _side_type_string16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string32_be(_name, _attr...)		_side_field(_name, _side_type_string32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

/*
 * The mappings are named rather than pointed at: the distance to them
 * is measured from the type holding it, which needs the name of the
 * object rather than its address. side_field_gather_enum() already
 * took it that way.
 */
#define _side_type_enum(_mappings, _elem_type) \
	(SIDE_TK_ENUM, SIDE_TYPE_ENUM, side_enum, _mappings, _elem_type)
#define _side_field_enum(_name, _mappings, _elem_type) \
	_side_field(_name, _side_type_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define _side_type_enum_bitmap(_mappings, _elem_type) \
	(SIDE_TK_ENUM, SIDE_TYPE_ENUM_BITMAP, side_enum_bitmap, _mappings, _elem_type)
#define _side_field_enum_bitmap(_name, _mappings, _elem_type) \
	_side_field(_name, _side_type_enum_bitmap(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define _side_type_struct(_struct) \
	(SIDE_TK_PTR, SIDE_TYPE_STRUCT, side_struct, _struct)

#define _side_field_struct(_name, _struct) \
	_side_field(_name, _side_type_struct(SIDE_PARAM(_struct)))

#define _side_type_struct_define(_fields, _attr)			\
	{								\
		.fields = _fields,				\
		.attributes = _attr,				\
	}

/*
 * The fields of a structure live in the section a description is in,
 * and are not const, for the same reason an event's do: the assembler
 * folds a distance to the name of a field only within one section, and
 * only between objects it may place there. The structure itself stays
 * where it was and is still reached by an address, which is what it has
 * to be, being in another section.
 *
 * That makes a structure definition several declarations rather than
 * one, so it can no longer be written `static side_define_struct(...)':
 * a storage class in front of it would land on the first of them. The
 * linkage is part of the name instead, the way it is for an event.
 */
#define __side_define_struct(_forward_decl_linkage, _linkage, _identifier, _fields, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage struct side_event_field __attribute__((section("side_event_description"))) \
		_identifier##__fields[] SIDE_ASM_LABEL(_identifier##__fields); \
	_forward_decl_linkage struct side_type_struct __attribute__((section("side_event_description"))) \
		_identifier SIDE_ASM_LABEL(_identifier);		\
	SIDE_FIELDS_DECLARE(_identifier, _fields)			\
	_linkage struct side_event_field __attribute__((section("side_event_description"), used)) \
		_identifier##__fields[] SIDE_ASM_LABEL(_identifier##__fields) = \
			SIDE_FIELDS_INIT(_identifier, _fields);		\
	SIDE_PTR_REL_DEFINE(_identifier##__fields_off, _identifier,	\
		struct side_type_struct, fields.elements,		\
		_identifier##__fields)					\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(struct side_type_struct, attributes.elements),	\
		_identifier, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())) \
	_linkage struct side_type_struct __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier) =		\
		_side_type_struct_define(SIDE_PARAM(SIDE_LITERAL_ARRAY_REL_OF_NAMED(_identifier##__fields_off, _identifier##__fields)), \
			SIDE_ATTRS_REL_REF(_identifier));			\
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

/*
 * In C++, a static cannot be forward declared; an anonymous namespace
 * gives the same reach, as it does for an event.
 */
#ifdef __cplusplus
#  define _side_static_define_struct(_identifier, _fields, _attr...)	\
	namespace {							\
		__side_define_struct(extern, , _identifier, SIDE_PARAM(_fields), ##_attr); \
	}
#else
#  define _side_static_define_struct(_identifier, _fields, _attr...)	\
	__side_define_struct(static, static, _identifier, SIDE_PARAM(_fields), ##_attr)
#endif

#define _side_define_struct(_identifier, _fields, _attr...)		\
	__side_define_struct(extern, , _identifier, SIDE_PARAM(_fields), ##_attr)

#define _side_type_variant(_variant) \
	(SIDE_TK_PTR, SIDE_TYPE_VARIANT, side_variant, _variant)

#define _side_field_variant(_name, _variant) \
	_side_field(_name, _side_type_variant(_variant))

#define _side_type_variant_define(_selector, _options, _attr)	     \
	{							     \
		.options = _options,			     \
		.selector = _selector,				     \
		.attributes = _attr,			     \
	}

/*
 * A variant, its options and the type each of them selects, in the
 * section a description is in: what a field holding this variant points
 * at has to be an object there for the distance to it to be one the
 * assembler folds, and so has everything it reaches in turn. That makes
 * the definition several declarations rather than one, so the linkage
 * is part of the name, as it is for a structure.
 */
#define __side_define_variant(_forward_decl_linkage, _linkage, _identifier, _selector, _options, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage struct side_variant_option __attribute__((section("side_event_description"))) \
		_identifier##__options[] SIDE_ASM_LABEL(_identifier##__options); \
	_forward_decl_linkage struct side_type_variant __attribute__((section("side_event_description"))) \
		_identifier SIDE_ASM_LABEL(_identifier);		\
	SIDE_OPTIONS_DECLARE(_identifier, _options)			\
	_linkage struct side_variant_option __attribute__((section("side_event_description"), used)) \
		_identifier##__options[] SIDE_ASM_LABEL(_identifier##__options) = \
			SIDE_OPTIONS_INIT(_identifier, _options);	\
	SIDE_PTR_REL_DEFINE(_identifier##__options_off, _identifier,	\
		struct side_type_variant, options.elements,		\
		_identifier##__options)					\
	SIDE_TYPE_DECLARE_L0(_identifier,				\
		offsetof(struct side_type_variant, selector),		\
		SIDE_CAT3(_identifier, __selector, ), _selector)	\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(struct side_type_variant, attributes.elements),	\
		_identifier, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())) \
	_linkage struct side_type_variant __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier) =		\
		_side_type_variant_define(				\
			SIDE_PARAM(SIDE_TYPE_INIT(SIDE_CAT3(_identifier, __selector, ), _selector)), \
			SIDE_PARAM(SIDE_LITERAL_ARRAY_REL_OF_NAMED(_identifier##__options_off, _identifier##__options)), \
			SIDE_ATTRS_REL_REF(_identifier));			\
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

#ifdef __cplusplus
#  define _side_static_define_variant(_identifier, _selector, _options, _attr...) \
	namespace {							\
		__side_define_variant(extern, , _identifier, SIDE_PARAM(_selector), \
				SIDE_PARAM(_options), ##_attr);		\
	}
#else
#  define _side_static_define_variant(_identifier, _selector, _options, _attr...) \
	__side_define_variant(static, static, _identifier, SIDE_PARAM(_selector), \
			SIDE_PARAM(_options), ##_attr)
#endif

#define _side_define_variant(_identifier, _selector, _options, _attr...) \
	__side_define_variant(extern, , _identifier, SIDE_PARAM(_selector),	\
			SIDE_PARAM(_options), ##_attr)

enum {
	SIDE_OPTIONAL_DISABLED = 0,
	SIDE_OPTIONAL_ENABLED = 1,
};

#define _side_type_optional(_optional)					\
	(SIDE_TK_PTR, SIDE_TYPE_OPTIONAL, side_optional, _optional)

#define _side_type_optional_define(_elem_off, _attr...)			\
	{								\
		.elem_type = SIDE_PTR_REL_INIT(_elem_off),		\
		.attributes = _attr,					\
	}

/*
 * An optional and the type it holds, in the section a description is
 * in, for the reason a variant is. The linkage is part of the name for
 * the same reason too.
 */
#define __side_define_optional(_forward_decl_linkage, _linkage, _identifier, _elem_type, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage struct side_type_optional __attribute__((section("side_event_description"))) \
		_identifier SIDE_ASM_LABEL(_identifier);		\
	SIDE_TYPE_HOIST_L0(SIDE_TYPE_ELEM_SYM(_identifier), _elem_type)	\
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_ELEM_OFF(_identifier), _identifier, \
		struct side_type_optional, elem_type,			\
		SIDE_TYPE_ELEM_SYM(_identifier))			\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(struct side_type_optional, attributes.elements),	\
		_identifier, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())) \
	_linkage struct side_type_optional __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier) =		\
		_side_type_optional_define(SIDE_TYPE_ELEM_OFF(_identifier), \
			SIDE_ATTRS_REL_REF(_identifier));			\
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

#ifdef __cplusplus
#  define _side_static_define_optional(_identifier, _elem_type, _attr...) \
	namespace {							\
		__side_define_optional(extern, , _identifier, _elem_type, ##_attr); \
	}
#else
#  define _side_static_define_optional(_identifier, _elem_type, _attr...) \
	__side_define_optional(static, static, _identifier, _elem_type, ##_attr)
#endif

#define _side_define_optional(_identifier, _elem_type, _attr...)	\
	__side_define_optional(extern, , _identifier, _elem_type, ##_attr)

#define _side_field_optional(_name, _identifier)		\
	_side_field(_name, _side_type_optional(_identifier))

#define _side_field_optional_literal(_name, _elem_type, _attr...)		\
	_side_field(_name, (SIDE_TK_OPTLIT, _elem_type,			\
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_array(_array)				\
	(SIDE_TK_PTR, SIDE_TYPE_ARRAY, side_array, _array)

#define _side_field_array(_name, _array) \
	_side_field(_name, _side_type_array(_array))

/*
 * An array and the type of its elements, in the section a description
 * is in, for the reason a variant is. The linkage is part of the name
 * for the same reason too.
 */
#define __side_define_array(_forward_decl_linkage, _linkage, _identifier, _elem_type, _length, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage struct side_type_array __attribute__((section("side_event_description"))) \
		_identifier SIDE_ASM_LABEL(_identifier);		\
	SIDE_TYPE_HOIST_L0(SIDE_TYPE_ELEM_SYM(_identifier), _elem_type)	\
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_ELEM_OFF(_identifier), _identifier, \
		struct side_type_array, elem_type,			\
		SIDE_TYPE_ELEM_SYM(_identifier))			\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(struct side_type_array, attributes.elements),	\
		_identifier, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())) \
	_linkage struct side_type_array __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier) = {		\
		.elem_type = SIDE_PTR_REL_INIT(SIDE_TYPE_ELEM_OFF(_identifier)), \
		.length = _length,					\
		.attributes = SIDE_ATTRS_REL_REF(_identifier),		\
	};								\
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

#ifdef __cplusplus
#  define _side_static_define_array(_identifier, _elem_type, _length, _attr...) \
	namespace {							\
		__side_define_array(extern, , _identifier, _elem_type, _length, ##_attr); \
	}
#else
#  define _side_static_define_array(_identifier, _elem_type, _length, _attr...) \
	__side_define_array(static, static, _identifier, _elem_type, _length, ##_attr)
#endif

#define _side_define_array(_identifier, _elem_type, _length, _attr...)	\
	__side_define_array(extern, , _identifier, _elem_type, _length, ##_attr)

#define _side_type_vla(_vla)					\
	(SIDE_TK_PTR, SIDE_TYPE_VLA, side_vla, _vla)

#define _side_field_vla(_name, _vla) \
	_side_field(_name, _side_type_vla(_vla))

/*
 * A vla, the type of its elements and the type of its length, in the
 * section a description is in, for the reason a variant is. The linkage
 * is part of the name for the same reason too.
 */
#define __side_define_vla(_forward_decl_linkage, _linkage, _identifier, _elem_type, _length_type, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage struct side_type_vla __attribute__((section("side_event_description"))) \
		_identifier SIDE_ASM_LABEL(_identifier);		\
	SIDE_TYPE_HOIST_L0(SIDE_TYPE_ELEM_SYM(_identifier), _elem_type)	\
	SIDE_TYPE_HOIST_L0(SIDE_TYPE_LEN_SYM(_identifier), _length_type) \
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_ELEM_OFF(_identifier), _identifier, \
		struct side_type_vla, elem_type,			\
		SIDE_TYPE_ELEM_SYM(_identifier))			\
	SIDE_PTR_REL_DEFINE(SIDE_TYPE_LEN_OFF(_identifier), _identifier, \
		struct side_type_vla, length_type,			\
		SIDE_TYPE_LEN_SYM(_identifier))				\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(struct side_type_vla, attributes.elements),	\
		_identifier, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())) \
	_linkage struct side_type_vla __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier) = {		\
		.elem_type = SIDE_PTR_REL_INIT(SIDE_TYPE_ELEM_OFF(_identifier)), \
		.length_type = SIDE_PTR_REL_INIT(SIDE_TYPE_LEN_OFF(_identifier)), \
		.attributes = SIDE_ATTRS_REL_REF(_identifier),		\
	};								\
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

#ifdef __cplusplus
#  define _side_static_define_vla(_identifier, _elem_type, _length_type, _attr...) \
	namespace {							\
		__side_define_vla(extern, , _identifier, _elem_type, _length_type, ##_attr); \
	}
#else
#  define _side_static_define_vla(_identifier, _elem_type, _length_type, _attr...) \
	__side_define_vla(static, static, _identifier, _elem_type, _length_type, ##_attr)
#endif

#define _side_define_vla(_identifier, _elem_type, _length_type, _attr...) \
	__side_define_vla(extern, , _identifier, _elem_type, _length_type, ##_attr)

/* Gather field and type definitions */

#define _side_type_gather_byte(_offset, _access_mode, _attr...) \
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()), \
		side_gather.u.side_byte.type, _side_type_gather_byte_init, _offset, _access_mode)
#define _side_type_gather_byte_init(_pfx, _offset, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BYTE), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_byte = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
						.attributes = SIDE_ATTRS_REF(_pfx), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_field_gather_byte(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_byte(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define __side_type_gather_bool(_byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()), \
		side_gather.u.side_bool.type, __side_type_gather_bool_init, _byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode)
#define __side_type_gather_bool_init(_pfx, _byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BOOL), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_bool = { \
						.offset = _offset, \
						.offset_bits = _offset_bits, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attributes = SIDE_ATTRS_REF(_pfx), \
							.bool_size = _bool_size, \
							.len_bits = _len_bits, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	__side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_HOST, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	__side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_LE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	__side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_BE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_bool(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_bool_le(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_bool_be(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_integer(_type, _signedness, _byte_order, _offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()), \
		side_gather.u.side_integer.type, _side_type_gather_integer_init, _type, _signedness, _byte_order, _offset, _integer_size, _offset_bits, _len_bits, _access_mode)
#define _side_type_gather_integer_init(_pfx, _type, _signedness, _byte_order, _offset, _integer_size, _offset_bits, _len_bits, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_integer = { \
						.offset = _offset, \
						.offset_bits = _offset_bits, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attributes = SIDE_ATTRS_REF(_pfx), \
							.integer_size = _integer_size, \
							.len_bits = _len_bits, \
							.signedness = _signedness, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_unsigned_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_signed_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_field_gather_unsigned_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_signed_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_field_gather_unsigned_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_signed_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_pointer(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_pointer(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_pointer(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_pointer_le(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_pointer_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_pointer_le(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_pointer_be(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_pointer_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_pointer_be(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define __side_type_gather_float(_byte_order, _offset, _float_size, _access_mode, _attr...) \
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()), \
		side_gather.u.side_float.type, __side_type_gather_float_init, _byte_order, _offset, _float_size, _access_mode)
#define __side_type_gather_float_init(_pfx, _byte_order, _offset, _float_size, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_FLOAT), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_float = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attributes = SIDE_ATTRS_REF(_pfx), \
							.float_size = _float_size, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_type_gather_float(_offset, _float_size, _access_mode, _attr...) \
	__side_type_gather_float(SIDE_TYPE_FLOAT_WORD_ORDER_HOST, _offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_float_le(_offset, _float_size, _access_mode, _attr...) \
	__side_type_gather_float(SIDE_TYPE_BYTE_ORDER_LE, _offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_float_be(_offset, _float_size, _access_mode, _attr...) \
	__side_type_gather_float(SIDE_TYPE_BYTE_ORDER_BE, _offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_float(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_float(_offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_float_le(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_float_le(_offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_float_be(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_float_be(_offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define __side_type_gather_string(_offset, _byte_order, _unit_size, _access_mode, _attr...) \
	(SIDE_TK_LEAF, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()), \
		side_gather.u.side_string.type, __side_type_gather_string_init, _offset, _byte_order, _unit_size, _access_mode)
#define __side_type_gather_string_init(_pfx, _offset, _byte_order, _unit_size, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRING), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_string = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attributes = SIDE_ATTRS_REF(_pfx), \
							.unit_size = _unit_size, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_type_gather_string(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_string(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_string16(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string16_le(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string16_be(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_string16(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string16(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string16_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string16_le(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string16_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string16_be(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_string32(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string32_le(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string32_be(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_string32(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string32(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string32_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string32_le(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string32_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string32_be(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_enum(_mappings, _elem_type) \
	(SIDE_TK_ENUM, SIDE_TYPE_GATHER_ENUM, side_enum, _mappings, _elem_type)
#define _side_field_gather_enum(_name, _mappings, _elem_type) \
	_side_field(_name, _side_type_gather_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define _side_type_gather_struct(_struct_gather, _offset, _size, _access_mode) \
	(SIDE_TK_GSTRUCT, _struct_gather, _offset, _size, _access_mode)
#define _side_field_gather_struct(_name, _struct_gather, _offset, _size, _access_mode) \
	_side_field(_name, _side_type_gather_struct(SIDE_PARAM(_struct_gather), _offset, _size, _access_mode))

#define _side_type_gather_array(_elem_type_gather, _length, _offset, _access_mode, _attr...) \
	(SIDE_TK_GARRAY, _elem_type_gather, _length, _offset, _access_mode, \
		SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_array(_name, _elem_type, _length, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_array(SIDE_PARAM(_elem_type), _length, _offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_vla(_elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	(SIDE_TK_GVLA, _elem_type_gather, _offset, _access_mode, _length_type_gather, \
		SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_vla(_name, _elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	_side_field(_name, _side_type_gather_vla(SIDE_PARAM(_elem_type_gather), _offset, _access_mode, SIDE_PARAM(_length_type_gather), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

/*
 * The type of an element, and the type of a length. A type is already
 * what it is rather than an initializer, so there is nothing left for
 * these to do but hand it over; the site storing it is what puts it
 * where it belongs. See SIDE_TYPE_DECLARE_L0().
 */
#define _side_elem(_type)	_type

#define _side_length(_type)	_type

/*
 * The fields, as a list the site defining the event or the structure
 * they belong to can walk. It is parenthesized rather than braced
 * because that site walks it twice, and only parentheses keep the
 * elements together as one macro argument on the way there.
 */
#define _side_field_list(...) \
	( __VA_ARGS__ )

/*
 * The options, as a list the site defining the variant can walk. It is
 * parenthesized rather than braced for the reason a field list is: that
 * site walks it twice, and only parentheses keep the elements together
 * as one macro argument on the way there.
 */
#define _side_option_list(...) \
	( __VA_ARGS__ )

/* Stack-copy field arguments */

#define _side_arg_null(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_NULL), .flags = 0, .u = { .side_static = {  } } }
#define _side_arg_bool(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_BOOL), .flags = 0, .u = { .side_static = { .bool_value = { .side_bool8 = !!(_val) } } } }
#define _side_arg_byte(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_BYTE), .flags = 0, .u = { .side_static = { .byte_value = (_val) } } }
#define _side_arg_string(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF8), .flags = 0, .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }
#define _side_arg_string16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF16), .flags = 0, .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }
#define _side_arg_string32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF32), .flags = 0, .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }

#define _side_arg_u8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U8), .flags = 0, .u = { .side_static = {  .integer_value = { .side_u8 = (_val) } } } }
#define _side_arg_u16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U16), .flags = 0, .u = { .side_static = { .integer_value = { .side_u16 = (_val) } } } }
#define _side_arg_u32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U32), .flags = 0, .u = { .side_static = { .integer_value = { .side_u32 = (_val) } } } }
#define _side_arg_u64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U64), .flags = 0, .u = { .side_static = { .integer_value = { .side_u64 = (_val) } } } }
#define _side_arg_u128(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U128), .flags = 0, .u = { .side_static = { .integer_value = { .side_u128 = (_val) } } } }
#define _side_arg_s8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S8), .flags = 0, .u = { .side_static = { .integer_value = { .side_s8 = (_val) } } } }
#define _side_arg_s16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S16), .flags = 0, .u = { .side_static = { .integer_value = { .side_s16 = (_val) } } } }
#define _side_arg_s32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S32), .flags = 0, .u = { .side_static = { .integer_value = { .side_s32 = (_val) } } } }
#define _side_arg_s64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S64), .flags = 0, .u = { .side_static = { .integer_value = { .side_s64 = (_val) } } } }
#define _side_arg_s128(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S128), .flags = 0, .u = { .side_static = { .integer_value = { .side_s128 = (_val) } } } }
#define _side_arg_pointer(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_POINTER), .flags = 0, .u = { .side_static = { .integer_value = { .side_uptr = (uintptr_t) (_val) } } } }
#define _side_arg_float_binary16(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY16), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary16 = (_val) } } } }
#define _side_arg_float_binary32(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY32), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary32 = (_val) } } } }
#define _side_arg_float_binary64(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY64), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary64 = (_val) } } } }
#define _side_arg_float_binary128(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY128), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary128 = (_val) } } } }

#define _side_arg_struct(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRUCT), .flags = 0, .u = { .side_static = { .side_struct = SIDE_PTR_INIT(&_side_type) } } }

#define _side_arg_define_variant(_identifier, _selector_val, _option) \
	const struct side_arg_variant _identifier = { \
		.selector = _selector_val, \
		.option = _option, \
	}

#define _side_arg_variant(_side_variant) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VARIANT), \
		.flags = 0, \
		.u = { \
			.side_static = { \
				.side_variant = SIDE_PTR_INIT(&_side_variant), \
			}, \
		}, \
	}

#define _side_arg_define_optional(_identifier, _value, _selector)	\
	const struct side_arg_optional _identifier = {			\
		.side_static = _value,					\
		.selector = _selector,					\
	}

#define _side_arg_optional(_identifier)					\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_OPTIONAL),		\
		.flags = 0,						\
		.u = {							\
			.side_static = {				\
				.side_optional = SIDE_PTR_INIT(&(_identifier)), \
			},						\
		},							\
	}

#define _side_arg_array(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_ARRAY), .flags = 0, .u = { .side_static = { .side_array = SIDE_PTR_INIT(&_side_type) } } }

#define _side_arg_vla(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_VLA), .flags = 0, .u = { .side_static = { .side_vla = SIDE_PTR_INIT(&_side_type) } } }

/* Gather field arguments */

#define _side_arg_gather_bool(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BOOL), .flags = 0, .u = { .side_static = { .side_bool_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_byte(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BYTE), .flags = 0, .u = { .side_static = { .side_byte_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_pointer(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_POINTER), .flags = 0, .u = { .side_static = { .side_integer_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_integer(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_INTEGER), .flags = 0, .u = { .side_static = { .side_integer_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_float(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_FLOAT), .flags = 0, .u = { .side_static = { .side_float_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_string(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRING), .flags = 0, .u = { .side_static = { .side_string_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_struct(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRUCT), .flags = 0, .u = { .side_static = { .side_struct_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_array(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ARRAY), .flags = 0, .u = { .side_static = { .side_array_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_vla(_ptr, _length_ptr)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_VLA), .flags = 0, .u = { .side_static = { .side_vla_gather = { .ptr = SIDE_PTR_INIT(_ptr), .length_ptr = SIDE_PTR_INIT(_length_ptr) } } } }

/* Dynamic field arguments */

#define _side_arg_dynamic_null(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_NULL), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_null = { \
					.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()) \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_bool(_val, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_BOOL), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_bool = { \
					.type = { \
						.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
						.bool_size = sizeof(uint8_t), \
						.len_bits = 0, \
						.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
					}, \
					.value = { \
						.side_bool8 = !!(_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_byte(_val, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_BYTE), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_byte = { \
					.type = { \
						.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
					}, \
					.value = (_val), \
				}, \
			}, \
		}, \
	}

#define __side_arg_dynamic_string(_val, _byte_order, _unit_size, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRING), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_string = { \
					.type = { \
						.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
						.unit_size = _unit_size, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = (uintptr_t) (_val), \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_string(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string16(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string16_le(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string16_be(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string32(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string32_le(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string32_be(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define _side_arg_dynamic_integer(_field, _val, _type, _signedness, _byte_order, _integer_size, _len_bits, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_integer = { \
					.type = { \
						.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
						.integer_size = _integer_size, \
						.len_bits = _len_bits, \
						.signedness = _signedness, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_u8(_val, _attr...) \
	_side_arg_dynamic_integer(.side_u8, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s8(_val, _attr...) \
	_side_arg_dynamic_integer(.side_s8, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_u16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u16, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_u32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u32, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_u64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u64, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_u128(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u128, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_s16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s16, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_s32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s32, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_s64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s64, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_s128(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s128, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_pointer(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_uptr, (uintptr_t) (_val), SIDE_TYPE_DYNAMIC_POINTER, false, _byte_order, \
			sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_float(_field, _val, _type, _byte_order, _float_size, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_float = { \
					.type = { \
						.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
						.float_size = _float_size, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define __side_arg_dynamic_float_binary16(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary16, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_float_binary32(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary32, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_float_binary64(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary64, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_float_binary128(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary128, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Host endian */
#define _side_arg_dynamic_u16(_val, _attr...) 			__side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u32(_val, _attr...) 			__side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u64(_val, _attr...) 			__side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u128(_val, _attr...) 			__side_arg_dynamic_u128(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s16(_val, _attr...) 			__side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s32(_val, _attr...) 			__side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s64(_val, _attr...) 			__side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s128(_val, _attr...) 			__side_arg_dynamic_s128(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_pointer(_val, _attr...) 		__side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary16(_val, _attr...)		__side_arg_dynamic_float_binary16(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary32(_val, _attr...)		__side_arg_dynamic_float_binary32(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary64(_val, _attr...)		__side_arg_dynamic_float_binary64(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary128(_val, _attr...)	__side_arg_dynamic_float_binary128(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Little endian */
#define _side_arg_dynamic_u16_le(_val, _attr...) 		__side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u32_le(_val, _attr...) 		__side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u64_le(_val, _attr...) 		__side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u128_le(_val, _attr...) 		__side_arg_dynamic_u128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s16_le(_val, _attr...) 		__side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s32_le(_val, _attr...) 		__side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s64_le(_val, _attr...) 		__side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s128_le(_val, _attr...) 		__side_arg_dynamic_s128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_pointer_le(_val, _attr...) 		__side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary16_le(_val, _attr...)	__side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary32_le(_val, _attr...)	__side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary64_le(_val, _attr...)	__side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary128_le(_val, _attr...)	__side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Big endian */
#define _side_arg_dynamic_u16_be(_val, _attr...) 		__side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u32_be(_val, _attr...) 		__side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u64_be(_val, _attr...) 		__side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u128_be(_val, _attr...) 		_side_arg_dynamic_u128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s16_be(_val, _attr...) 		__side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s32_be(_val, _attr...) 		__side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s64_be(_val, _attr...) 		__side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s128_be(_val, _attr...) 		__side_arg_dynamic_s128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_pointer_be(_val, _attr...) 		__side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary16_be(_val, _attr...)	__side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary32_be(_val, _attr...)	__side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary64_be(_val, _attr...)	__side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary128_be(_val, _attr...)	__side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define _side_arg_dynamic_vla(_vla) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_VLA), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla = SIDE_PTR_INIT(_vla), \
			}, \
		}, \
	}

#define _side_arg_dynamic_struct(_struct) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRUCT), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct = SIDE_PTR_INIT(_struct), \
			}, \
		}, \
	}

#define _side_arg_dynamic_define_vec(_identifier, _sav, _attr...) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_dynamic_vla _identifier = { \
		.sav = SIDE_PTR_INIT(_identifier##_vec), \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
		.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
	}

#define _side_arg_dynamic_define_struct(_identifier, _struct_fields, _attr...) \
	const struct side_arg_dynamic_field _identifier##_fields[] = { _struct_fields }; \
	const struct side_arg_dynamic_struct _identifier = { \
		.fields = SIDE_PTR_INIT(_identifier##_fields),	\
		.len = SIDE_ARRAY_SIZE(_identifier##_fields),		\
		.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
	}

#define _side_arg_define_vec(_identifier, _sav) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_vec _identifier = { \
		.sav = SIDE_PTR_INIT(_identifier##_vec), \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
	}

#define _side_arg_dynamic_field(_name, _elem) \
	{ \
		.field_name = SIDE_PTR_INIT(_name), \
		.elem = _elem, \
	}

/*
 * Event instrumentation description registration, runtime enabled state
 * check, and instrumentation invocation.
 */

#define _side_arg_list(...)	__VA_ARGS__

#define side_event_enabled(_identifier) \
	side_unlikely(__atomic_load_n(&side_event_state__##_identifier.enabled, \
					__ATOMIC_RELAXED))

#define _side_event(_identifier, _sav)					\
	if (side_event_enabled(_identifier))				\
		_side_event_call(side_call, _identifier, SIDE_PARAM(_sav))

#define _side_event_variadic(_identifier, _sav, _var, _attr...) \
	if (side_event_enabled(_identifier))				\
		_side_event_call_variadic(side_call_variadic, _identifier, \
					SIDE_PARAM(_sav), SIDE_PARAM(_var), \
					SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define _side_event_call(_call, _identifier, _sav) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		_call(&(side_event_state__##_identifier).parent, &side_arg_vec); \
	}

#define _side_event_call_variadic(_call, _identifier, _sav, _var_fields, _attr...) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		const struct side_arg_dynamic_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_struct var_struct = { \
			.fields = SIDE_PTR_INIT(side_fields), \
			.len = SIDE_ARRAY_SIZE(side_fields), \
			.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
		}; \
		_call(&(side_event_state__##_identifier.parent), &side_arg_vec, &var_struct); \
	}

#define _side_statedump_event_call(_call, _identifier, _key, _sav) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		_call(&(side_event_state__##_identifier).parent, &side_arg_vec, _key); \
	}

#define side_statedump_event_call(_identifier, _key, _sav) \
	_side_statedump_event_call(side_statedump_call, _identifier, _key, SIDE_PARAM(_sav))

#define _side_statedump_event_call_variadic(_call, _identifier, _key, _sav, _var_fields, _attr...) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = side_ptr_init(side_sav), \
			.len = side_array_size(side_sav), \
		}; \
		const struct side_arg_dynamic_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_struct var_struct = { \
			.fields = side_ptr_init(side_fields), \
			.len = side_array_size(side_fields), \
			.attributes = SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()), \
		}; \
		_call(&(side_event_state__##_identifier.parent), &side_arg_vec, &var_struct, _key); \
	}

#define side_statedump_event_call_variadic(_identifier, _key, _sav, _var_fields, _attr...) \
	_side_statedump_event_call_variadic(side_statedump_call_variadic, _identifier, _key, SIDE_PARAM(_sav), SIDE_PARAM(_var_fields), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))


/*
 * The forward declaration linkage is always the same in C. In C++ however, it
 * is necessary to not use the same linkage as the declaration.
 *
 * Rationale for disabled diagnostics:
 *
 *   -Wsection:
 *      Clang complains about redeclared sections.
 */
#define _side_define_event(_forward_decl_linkage, _linkage, _identifier, _provider, _event, _loglevel, _fields, _flags, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	/*								\
	 * Used, like everything else a distance is measured between:	\
	 * every distance in a description is measured from the		\
	 * description, and a name the compiler believes unreferenced	\
	 * is not there for the assembler to subtract from. See		\
	 * side_ptr_rel_t.						\
	 */								\
	_forward_decl_linkage struct side_event_description __attribute__((section("side_event_description"), used)) \
		_identifier SIDE_ASM_LABEL(_identifier);			\
	/*								\
	 * The names live in the section the description is in, and are	\
	 * not const, because the distances to them are folded by the	\
	 * assembler and it only folds within one section. See		\
	 * side_ptr_rel_t.						\
	 */								\
	_forward_decl_linkage char __attribute__((section("side_event_description"), used)) \
		_identifier##__provider_name[] SIDE_ASM_LABEL(_identifier##__provider_name); \
	_forward_decl_linkage char __attribute__((section("side_event_description"), used)) \
		_identifier##__event_name[] SIDE_ASM_LABEL(_identifier##__event_name); \
	_linkage char __attribute__((section("side_event_description"), used)) \
		_identifier##__provider_name[] = _provider;		\
	_linkage char __attribute__((section("side_event_description"), used)) \
		_identifier##__event_name[] = _event;			\
	SIDE_PTR_REL_DEFINE(_identifier##__provider_name_off, _identifier, \
		struct side_event_description, provider_name,		\
		_identifier##__provider_name)				\
	SIDE_PTR_REL_DEFINE(_identifier##__event_name_off, _identifier,	\
		struct side_event_description, event_name,		\
		_identifier##__event_name)				\
	/*							\
	 * The fields, named so a distance to them can be taken, and	\
	 * declared before their names because the distance to each	\
	 * name is measured from a byte of this array.			\
	 */								\
	_forward_decl_linkage struct side_event_field __attribute__((section("side_event_description"))) \
		_identifier##__fields[] SIDE_ASM_LABEL(_identifier##__fields); \
	SIDE_FIELDS_DECLARE(_identifier, _fields)			\
	_linkage struct side_event_field __attribute__((section("side_event_description"), used)) \
		_identifier##__fields[] SIDE_ASM_LABEL(_identifier##__fields) = \
			SIDE_FIELDS_INIT(_identifier, _fields);		\
	SIDE_PTR_REL_DEFINE(_identifier##__fields_off, _identifier,	\
		struct side_event_description, fields.elements,		\
		_identifier##__fields)					\
	/* The attributes of the event, and the strings they hold. */	\
	SIDE_ATTRS_DECLARE(_identifier,					\
		offsetof(struct side_event_description, attributes.elements), \
		_identifier, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())) \
	_forward_decl_linkage struct side_event_state_0 __attribute__((section("side_event_state"))) \
		side_event_state__##_identifier;				\
	/*								\
	 * The state is the only thing which names the description, so \
	 * it is the only thing which can take it away from the		\
	 * assembly measuring distances from it.			\
	 */								\
	SIDE_LTO_KEEP_TOGETHER(_identifier, _identifier,		\
		side_event_state__##_identifier)			\
	_linkage struct side_event_state_0 __attribute__((section("side_event_state"))) \
		side_event_state__##_identifier = {			\
		.parent = {						\
			.version = SIDE_EVENT_STATE_ABI_VERSION,	\
		},							\
		.nr_callbacks = 0,					\
		.enabled = 0,						\
		.callbacks = (const struct side_callback *) &side_empty_callback[0], \
		.desc = &(_identifier),					\
	};								\
	_linkage struct side_event_description __attribute__((section("side_event_description"), used)) \
		_identifier = {						\
		.side_begin_abi_tag_0 = {},				\
		.struct_size = offsetof(struct side_event_description, end), \
		.version = SIDE_EVENT_DESCRIPTION_ABI_VERSION,		\
		.provider_name = SIDE_PTR_REL_INIT(_identifier##__provider_name_off), \
		.event_name = SIDE_PTR_REL_INIT(_identifier##__event_name_off), \
		.fields = {						\
			.elements = SIDE_PTR_REL_INIT(_identifier##__fields_off), \
			.length = SIDE_ARRAY_SIZE(_identifier##__fields), \
		},							\
		.attributes = {						\
			.elements = SIDE_PTR_REL_INIT(SIDE_ATTR_ARRAY_OFF(_identifier)), \
			.length = SIDE_ARRAY_SIZE(SIDE_ATTR_ARRAY_SYM(_identifier)), \
		},							\
		.flags = (_flags),					\
		.nr_side_type_label = _NR_SIDE_TYPE_LABEL,		\
		.nr_side_attr_type = _NR_SIDE_ATTR_TYPE,		\
		.loglevel = SIDE_ENUM_INIT(_loglevel),			\
		.side_end_abi_tag_0 = {},				\
		.end = {}						\
	};								\
	/*							\
	 * One pointer per event, by which libside finds it. It is the \
	 * state, not the description: the state is what a tracer	\
	 * writes to, and it carries the description, so this one	\
	 * relocation reaches both. The description holds no address of \
	 * its own that way.					\
	 */								\
	static struct side_event_state __attribute__((section("side_event_state_ptr"), used)) \
	*side_event_ptr__##_identifier = &(side_event_state__##_identifier.parent); \
	SIDE_POP_DIAGNOSTIC() SIDE_EXPECT_SEMICOLON()

/*
 * In C++, it is not possible to forward declare a static variable.  Use
 * anonymous namespace with external linkage instead.
 */
#ifdef __cplusplus
#define _side_cxx_define_event(_namespace, _linkage, _identifier, _provider, _event, _loglevel, _fields, _flags, _attr...) \
	_namespace {							\
		_side_define_event(extern, _linkage, _identifier, _provider, _event, _loglevel, \
				SIDE_PARAM(_fields), _flags, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	}

#define _side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(namespace, , _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(namespace, , _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_declare_event(_identifier)				\
	extern "C" struct side_event_description _identifier;		\
	extern "C" struct side_event_state_0 side_event_state_##_identifier
#else
#define _side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(static, static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(static, static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("hidden"))), __attribute__((visibility("hidden"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("hidden"))), __attribute__((visibility("hidden"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, \
			   SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("default"))), __attribute__((visibility("default"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("default"))), __attribute__((visibility("default"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, \
			   SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_declare_event(_identifier) \
	extern struct side_event_state_0 side_event_state_##_identifier; \
	extern struct side_event_description _identifier
#endif	/* __cplusplus */

#endif /* SIDE_INSTRUMENTATION_C_API_H */
