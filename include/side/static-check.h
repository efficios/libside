// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef _SIDE_STATIC_CHECK_H
#define _SIDE_STATIC_CHECK_H

/*
 * The static checker works by doing macro dispatching.  A macro of the form
 * `X(...)' can be dispatched to a macro of the form `PX(...)' where `P' is a
 * dispatching prefix.  This works by concatenating the wanted dispatch prefix
 * onto the form as long as `X' is undefined.  Therefore, all macros part of the
 * Domain Specific Language (DSL) must be undefined.
 *
 * Because the DSL accepts nested form with list, it is necessary for the
 * dispatching mechanism to do recursive expansion.  This can be done with the
 * preprocessor to some extent (see *_MAP macros).  However, nested constructs
 * of identical form with unbounded level are impossible. It must be clear that
 * nested identical forms are limited to some known depth.
 *
 * Consider the following form:
 *
 *	side_field_list(side_field_null("a"), side_field_u8("b"), )
 *
 * Say it is necessary to expand this form to two things:
 *
 * First, the form must be expanded to the list of field names without commas:
 *
 *	"a" "b"
 *
 *	This can be done by defining the following dispatch prefixes:
 *
 *		#define NAME_OF(something) NAME_OF_##something
 *
 *		// For trailing comma
 *		#define NAME_OF_
 *
 *		// MAP(f, xs...) expands to: f(x1) f(x2) ...
 *		#define NAME_OF_side_field_list(fields...) MAP(NAME_OF, fields)
 *
 *		#define FIELD_NAME_OF(name, ...) name
 *
 *		#define NAME_OF_side_field_null FIELD_NAME_OF
 *		#define NAME_OF_side_field_u8 FIELD_NAME_OF
 *
 *	The only thing left is to concat `NAME_OF_' onto the form.
 *
 * Second, the form must be expanded to what it would have expanded if there
 * were no static checker.
 *
 *	This kind of expansion is expressed as "emitting" the DSL.  This can be
 *	done by defining the following dispatch prefixes:
 *
 *		#define EMIT(something) EMIT_##something
 *
 *		// For trailing comma
 *		#define EMIT_
 *
 *		// MAP_LIST(f, x...) expands to: f(x1), f(x2) ...
 *		#define EMIT_side_field_list(fields...) \
 *			_side_field_list(MAP_LIST(EMIT, fields))
 *
 *		#define EMIT_side_field_null _side_field_null
 *		#define EMIT_side_field_u8 _side_field_u8
 *
 *	The only thing left is to concat `EMIT_' onto the form.
 *
 * The static checker uses 3 kinds of dispatching.  `CHECK', `NAME_OF' and `EMIT'.
 *
 * The `CHECK' dispatching is used to get a type defined by the static checker
 * from a field or an argument.  It is used to generate function signatures at
 * event descriptions and event call sites.  Mismatches between the signatures
 * are signaled as errors by the compiler.
 *
 * The `NAME_OF' dispatching is used to extract field name.  It is used in event
 * and structure definitions to ensure that no fields are null or duplicated.
 *
 * The `EMIT' dispatching is used to forward the macro to the real
 * implementation, as if there were no static checker.
 *
 * NOTE:
 *
 *	Public macros are defined using the `SIDE_STATIC_CHECK_' prefix.
 *	Private macros are define using the `SIDE_SC_' prefix.
 *
 * NOTE:
 *
 *	The dispatching is hidden from the user.  The idea is to undefine the
 *	whole DSL so that the static checker can expand the forms passed by the
 *	user many times in different ways.
 *
 * NOTE:
 *
 *	Most of the dispatching introduce comma prefix.  For example:
 *
 *		#define SIDE_SC_CHECK_side_field_u8(...) ,SIDE_SC_TYPE(u8)
 *
 *	This is for supporting trailing comma in user inputs.  Consider the
 *	following user form:
 *
 *		list(,,x,,,y,,z,,,,,)
 *
 *	where it is desired to dispatch it to the following form without the
 *	empty arguments:
 *
 *		list(x, y, z)
 *
 *	This can be done like so:
 *
 *		#define SKIP_1(first, rest...) rest
 *
 *		#define NO_COMMA(something) NO_COMMA_##something
 *
 *		// Remove trailing comma
 *		#define NO_COMMA_
 *
 *		#define NO_COMMA_list_primitive(elements...)	\
 *			SKIP_1(elements)
 *
 *		// MAP(f, x...) expand to f(x1) f(x2) ...
 *		#define NO_COMMA_list(elements...)					\
 *			list(NO_COMMA_list_primitive(MAP(NO_COMMA, elements)))		\
 *
 *		#define NO_COMMA_x ,x
 *		#define NO_COMMA_y ,y
 *		#define NO_COMMA_z ,z
 *
 *		NO_COMMA_list(,,x,,,y,,z,,,,,)
 */

#include <side/macros.h>

/* User configuration. */

/*
 * SIDE_STATIC_CHECK_MAX_EVAL_LEVEL: Determine the maximum level of expansion by
 * the static checker.  Each level allow for 4 more time expansions.  Therefore,
 * the number of possible expansion with a `EVAL()' form is 4^N - 1, where N is
 * the selected level.  This can quickly become slow if N is too high.
 *
 * If the value is too low, then some forms will not be expanded, resulting in
 * failed compilation.
 *
 * This constraint the following forms:
 *
 *	- side_field_list()
 *	- side_arg_list() inside of:
 *		- side_event() and friends
 *		- side_arg_define_array()
 *		- side_arg_define_vla()
 *	- side_attr_list()
 *	- side_option_list()
 *	- side_dynamic_attr_list()
 */
#define SIDE_SC_DEFAULT_EVAL_LEVEL	5
#define SIDE_SC_MIN_EVAL_LEVEL		2
#define SIDE_SC_MAX_EVAL_LEVEL		10

#ifdef SIDE_STATIC_CHECK_MAX_EVAL_LEVEL
#  if SIDE_STATIC_CHECK_MAX_EVAL_LEVEL < SIDE_SC_MIN_EVAL_LEVEL
#    error "SIDE_STATIC_CHECK_MAX_EVAL_LEVEL must be greater or equal to 2."
#  elif SIDE_STATIC_CHECK_MAX_EVAL_LEVEL > SIDE_SC_MAX_EVAL_LEVEL
#    error "SIDE_STATIC_CHECK_MAX_EVAL_LEVEL must be less or equal to 10."
#  endif
#else
#  define SIDE_STATIC_CHECK_MAX_EVAL_LEVEL SIDE_SC_DEFAULT_EVAL_LEVEL
#endif

/* Helpers. */

/* Stringify X after expansion. */
#define SIDE_SC_STR_PRIMITIVE(x...) #x
#define SIDE_SC_STR(x...) SIDE_SC_STR_PRIMITIVE(x)

/* Concatenate X and Y.  Used when X and Y needs to be expanded first. */
#define SIDE_SC_CAT_PRIMITIVE(x, y) x ## y
#define SIDE_SC_CAT(x, y) SIDE_SC_CAT_PRIMITIVE(x, y)

/* Get source location as a C string. */
#define SIDE_SC_SOURCE_LOCATION() __FILE__ SIDE_SC_STR(:__LINE__)

/* Skip over the first element of a list. */
#define SIDE_SC_SKIP_1(_first, ...) __VA_ARGS__

/* Take element at a position in a list. */
#define SIDE_SC_TAKE_1(_first, ...) _first
#define SIDE_SC_TAKE_2(_first, _second, ...) _second
#define SIDE_SC_TAKE_3(_first, _second, _third, ...) _third

/* Compiler dependant diagnostics. */
#if defined(__clang__)
#  define SIDE_SC_DIAGNOSTIC(x) _Pragma(SIDE_SC_STR(clang diagnostic x)) SIDE_EXPECT_SEMICOLON()
#  define SIDE_SC_PUSH_DIAGNOSTIC()				\
	_Pragma("clang diagnostic push")			\
	SIDE_SC_DIAGNOSTIC(ignored "-Wunknown-warning-option")

#  define SIDE_SC_POP_DIAGNOSTIC() _Pragma("clang diagnostic push") SIDE_EXPECT_SEMICOLON()
#elif defined(__GNUC__)
#  define SIDE_SC_DIAGNOSTIC(x) _Pragma(SIDE_SC_STR(GCC diagnostic x)) SIDE_EXPECT_SEMICOLON()
#  define SIDE_SC_PUSH_DIAGNOSTIC() _Pragma("GCC diagnostic push") SIDE_EXPECT_SEMICOLON()
#  define SIDE_SC_POP_DIAGNOSTIC() _Pragma("GCC diagnostic pop") SIDE_EXPECT_SEMICOLON()
#endif

#ifdef __cplusplus
#  define SIDE_SC_DIAGNOSTIC_C SIDE_EXPECT_SEMICOLON
#else
#  define SIDE_SC_DIAGNOSTIC_C SIDE_SC_DIAGNOSTIC
#endif

/* Begin custom diagnostics. */
#define SIDE_SC_BEGIN_DIAGNOSTIC()				\
	SIDE_SC_PUSH_DIAGNOSTIC();				\
	SIDE_SC_DIAGNOSTIC_C(ignored "-Wnested-externs");	\
	SIDE_SC_DIAGNOSTIC(ignored "-Wredundant-decls");	\
	SIDE_SC_DIAGNOSTIC(ignored "-Wshadow");			\
	SIDE_SC_DIAGNOSTIC(ignored "-Wunused-local-typedefs");	\
	SIDE_SC_DIAGNOSTIC(error "-Warray-parameter=1")

/* End custom diagnostics. */
#define SIDE_SC_END_DIAGNOSTIC()		\
	SIDE_SC_POP_DIAGNOSTIC()

/* Recursive macros. */

/* Used by first level iteration macros. */
#define SIDE_SC_EVAL0(...) __VA_ARGS__
#define SIDE_SC_EVAL1(...) SIDE_SC_EVAL0(SIDE_SC_EVAL0(SIDE_SC_EVAL0(__VA_ARGS__)))
#define SIDE_SC_EVAL2(...) SIDE_SC_EVAL1(SIDE_SC_EVAL1(SIDE_SC_EVAL1(__VA_ARGS__)))
#define SIDE_SC_EVAL3(...) SIDE_SC_EVAL2(SIDE_SC_EVAL2(SIDE_SC_EVAL2(__VA_ARGS__)))
#define SIDE_SC_EVAL4(...) SIDE_SC_EVAL3(SIDE_SC_EVAL3(SIDE_SC_EVAL3(__VA_ARGS__)))
#define SIDE_SC_EVAL5(...) SIDE_SC_EVAL4(SIDE_SC_EVAL4(SIDE_SC_EVAL4(__VA_ARGS__)))
#define SIDE_SC_EVAL6(...) SIDE_SC_EVAL5(SIDE_SC_EVAL5(SIDE_SC_EVAL5(__VA_ARGS__)))
#define SIDE_SC_EVAL7(...) SIDE_SC_EVAL6(SIDE_SC_EVAL6(SIDE_SC_EVAL6(__VA_ARGS__)))
#define SIDE_SC_EVAL8(...) SIDE_SC_EVAL7(SIDE_SC_EVAL7(SIDE_SC_EVAL7(__VA_ARGS__)))
#define SIDE_SC_EVAL9(...) SIDE_SC_EVAL8(SIDE_SC_EVAL8(SIDE_SC_EVAL8(__VA_ARGS__)))
#define SIDE_SC_EVAL10(...) SIDE_SC_EVAL9(SIDE_SC_EVAL9(SIDE_SC_EVAL9(__VA_ARGS__)))
#define SIDE_SC_EVAL SIDE_SC_CAT(SIDE_SC_EVAL, SIDE_STATIC_CHECK_MAX_EVAL_LEVEL)

/* Used by second level iteration macros. */
#define SIDE_SC_EVAL_SUB0(...) __VA_ARGS__
#define SIDE_SC_EVAL_SUB1(...) SIDE_SC_EVAL_SUB0(SIDE_SC_EVAL_SUB0(SIDE_SC_EVAL_SUB0(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB2(...) SIDE_SC_EVAL_SUB1(SIDE_SC_EVAL_SUB1(SIDE_SC_EVAL_SUB1(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB3(...) SIDE_SC_EVAL_SUB2(SIDE_SC_EVAL_SUB2(SIDE_SC_EVAL_SUB2(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB4(...) SIDE_SC_EVAL_SUB3(SIDE_SC_EVAL_SUB3(SIDE_SC_EVAL_SUB3(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB5(...) SIDE_SC_EVAL_SUB4(SIDE_SC_EVAL_SUB4(SIDE_SC_EVAL_SUB4(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB6(...) SIDE_SC_EVAL_SUB5(SIDE_SC_EVAL_SUB5(SIDE_SC_EVAL_SUB5(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB7(...) SIDE_SC_EVAL_SUB6(SIDE_SC_EVAL_SUB6(SIDE_SC_EVAL_SUB6(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB8(...) SIDE_SC_EVAL_SUB7(SIDE_SC_EVAL_SUB7(SIDE_SC_EVAL_SUB7(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB9(...) SIDE_SC_EVAL_SUB8(SIDE_SC_EVAL_SUB8(SIDE_SC_EVAL_SUB8(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB10(...) SIDE_SC_EVAL_SUB9(SIDE_SC_EVAL_SUB9(SIDE_SC_EVAL_SUB9(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB SIDE_SC_CAT(SIDE_SC_EVAL_SUB, SIDE_STATIC_CHECK_MAX_EVAL_LEVEL)

/* Used by third level iteration macros. */
#define SIDE_SC_EVAL_SUB_SUB0(...) __VA_ARGS__
#define SIDE_SC_EVAL_SUB_SUB1(...) SIDE_SC_EVAL_SUB_SUB0(SIDE_SC_EVAL_SUB_SUB0(SIDE_SC_EVAL_SUB_SUB0(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB2(...) SIDE_SC_EVAL_SUB_SUB1(SIDE_SC_EVAL_SUB_SUB1(SIDE_SC_EVAL_SUB_SUB1(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB3(...) SIDE_SC_EVAL_SUB_SUB2(SIDE_SC_EVAL_SUB_SUB2(SIDE_SC_EVAL_SUB_SUB2(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB4(...) SIDE_SC_EVAL_SUB_SUB3(SIDE_SC_EVAL_SUB_SUB3(SIDE_SC_EVAL_SUB_SUB3(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB5(...) SIDE_SC_EVAL_SUB_SUB4(SIDE_SC_EVAL_SUB_SUB4(SIDE_SC_EVAL_SUB_SUB4(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB6(...) SIDE_SC_EVAL_SUB_SUB5(SIDE_SC_EVAL_SUB_SUB5(SIDE_SC_EVAL_SUB_SUB5(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB7(...) SIDE_SC_EVAL_SUB_SUB6(SIDE_SC_EVAL_SUB_SUB6(SIDE_SC_EVAL_SUB_SUB6(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB8(...) SIDE_SC_EVAL_SUB_SUB7(SIDE_SC_EVAL_SUB_SUB7(SIDE_SC_EVAL_SUB_SUB7(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB9(...) SIDE_SC_EVAL_SUB_SUB8(SIDE_SC_EVAL_SUB_SUB8(SIDE_SC_EVAL_SUB_SUB8(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB10(...) SIDE_SC_EVAL_SUB_SUB9(SIDE_SC_EVAL_SUB_SUB9(SIDE_SC_EVAL_SUB_SUB9(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB SIDE_SC_CAT(SIDE_SC_EVAL_SUB_SUB, SIDE_STATIC_CHECK_MAX_EVAL_LEVEL)

/* Used by fourth level iteration macros. */
#define SIDE_SC_EVAL_SUB_SUB_SUB0(...) __VA_ARGS__
#define SIDE_SC_EVAL_SUB_SUB_SUB1(...) SIDE_SC_EVAL_SUB_SUB_SUB0(SIDE_SC_EVAL_SUB_SUB_SUB0(SIDE_SC_EVAL_SUB_SUB_SUB0(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB2(...) SIDE_SC_EVAL_SUB_SUB_SUB1(SIDE_SC_EVAL_SUB_SUB_SUB1(SIDE_SC_EVAL_SUB_SUB_SUB1(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB3(...) SIDE_SC_EVAL_SUB_SUB_SUB2(SIDE_SC_EVAL_SUB_SUB_SUB2(SIDE_SC_EVAL_SUB_SUB_SUB2(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB4(...) SIDE_SC_EVAL_SUB_SUB_SUB3(SIDE_SC_EVAL_SUB_SUB_SUB3(SIDE_SC_EVAL_SUB_SUB_SUB3(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB5(...) SIDE_SC_EVAL_SUB_SUB_SUB4(SIDE_SC_EVAL_SUB_SUB_SUB4(SIDE_SC_EVAL_SUB_SUB_SUB4(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB6(...) SIDE_SC_EVAL_SUB_SUB_SUB5(SIDE_SC_EVAL_SUB_SUB_SUB5(SIDE_SC_EVAL_SUB_SUB_SUB5(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB7(...) SIDE_SC_EVAL_SUB_SUB_SUB6(SIDE_SC_EVAL_SUB_SUB_SUB6(SIDE_SC_EVAL_SUB_SUB_SUB6(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB8(...) SIDE_SC_EVAL_SUB_SUB_SUB7(SIDE_SC_EVAL_SUB_SUB_SUB7(SIDE_SC_EVAL_SUB_SUB_SUB7(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB9(...) SIDE_SC_EVAL_SUB_SUB_SUB8(SIDE_SC_EVAL_SUB_SUB_SUB8(SIDE_SC_EVAL_SUB_SUB_SUB8(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB10(...) SIDE_SC_EVAL_SUB_SUB_SUB9(SIDE_SC_EVAL_SUB_SUB_SUB9(SIDE_SC_EVAL_SUB_SUB_SUB9(__VA_ARGS__)))
#define SIDE_SC_EVAL_SUB_SUB_SUB  SIDE_SC_CAT(SIDE_SC_EVAL_SUB_SUB_SUB, SIDE_STATIC_CHECK_MAX_EVAL_LEVEL)

#define SIDE_SC_MAP_END(...)
#define SIDE_SC_MAP_OUT
#define SIDE_SC_MAP_COMMA ,

#define SIDE_SC_MAP_GET_END2() 0, SIDE_SC_MAP_END
#define SIDE_SC_MAP_GET_END1(...) SIDE_SC_MAP_GET_END2
#define SIDE_SC_MAP_GET_END(...) SIDE_SC_MAP_GET_END1
#define SIDE_SC_MAP_NEXT0(test, next, ...) next SIDE_SC_MAP_OUT
#define SIDE_SC_MAP_NEXT1(test, next) SIDE_SC_MAP_NEXT0(test, next, 0)
#define SIDE_SC_MAP_NEXT(test, next)  SIDE_SC_MAP_NEXT1(SIDE_SC_MAP_GET_END test, next)

#define SIDE_SC_MAP0(f, x, peek, ...) f(x) SIDE_SC_MAP_NEXT(peek, SIDE_SC_MAP1)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP1(f, x, peek, ...) f(x) SIDE_SC_MAP_NEXT(peek, SIDE_SC_MAP0)(f, peek, __VA_ARGS__)

/*
 * Used for first level traversal.  e.g., `side_field_list'.  Does not keep
 * commas between elements.
 *
 * Example:  SIDE_SC_MAP(SIDE_SC_STR, foo, bar) => "foo" "bar"
 */
#define SIDE_SC_MAP(f, ...) SIDE_SC_EVAL(SIDE_SC_MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/*
 * Like SIDE_SC_MAP, but do partial evaluation by passing a default argument
 * that will be applied to `f' along with the elements of the list.
 *
 * Example: SIDE_SC_MAP_CURRYING(SIDE_SC_CAT, prefix_, foo, bar) => prefix_foo prefix_bar
 */
#define SIDE_SC_MAP_NEXT0_CURRYING(test, next, ...) next SIDE_SC_MAP_OUT
#define SIDE_SC_MAP_NEXT1_CURRYING(test, next) SIDE_SC_MAP_NEXT0_CURRYING(test, next, 0)
#define SIDE_SC_MAP_NEXT_CURRYING(test, next)  SIDE_SC_MAP_NEXT1_CURRYING(SIDE_SC_MAP_GET_END test, next)
#define SIDE_SC_MAP0_CURRYING(f, partial, x, peek, ...) f(partial, x) SIDE_SC_MAP_NEXT_CURRYING(peek, SIDE_SC_MAP1_CURRYING)(f, partial, peek, __VA_ARGS__)
#define SIDE_SC_MAP1_CURRYING(f, partial, x, peek, ...) f(partial, x) SIDE_SC_MAP_NEXT_CURRYING(peek, SIDE_SC_MAP0_CURRYING)(f, partial, peek, __VA_ARGS__)
#define SIDE_SC_MAP_CURRYING(f, partial, ...) SIDE_SC_EVAL(SIDE_SC_MAP1_CURRYING(f, partial, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/*
 * Used for first level traversal.  e.g., `side_field_list'.
 *
 * Like SIDE_SC_MAP but keep the commas between elements.
 */
#define SIDE_SC_MAP_LIST_NEXT1(test, next) SIDE_SC_MAP_NEXT0(test, SIDE_SC_MAP_COMMA next, 0)
#define SIDE_SC_MAP_LIST_NEXT(test, next)  SIDE_SC_MAP_LIST_NEXT1(SIDE_SC_MAP_GET_END test, next)
#define SIDE_SC_MAP_LIST0(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_NEXT(peek, SIDE_SC_MAP_LIST1)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST1(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_NEXT(peek, SIDE_SC_MAP_LIST0)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST(f, ...) SIDE_SC_EVAL(SIDE_SC_MAP_LIST1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/* Used for nested list.  e.g., `side_elem' within `side_field_list'. */
#define SIDE_SC_MAP_LIST_SUB_NEXT1(test, next) SIDE_SC_MAP_NEXT0(test, SIDE_SC_MAP_COMMA next, 0)
#define SIDE_SC_MAP_LIST_SUB_NEXT(test, next)  SIDE_SC_MAP_LIST_SUB_NEXT1(SIDE_SC_MAP_GET_END test, next)
#define SIDE_SC_MAP_LIST_SUB0(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_SUB_NEXT(peek, SIDE_SC_MAP_LIST_SUB1)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST_SUB1(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_SUB_NEXT(peek, SIDE_SC_MAP_LIST_SUB0)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST_SUB(f, ...) SIDE_SC_EVAL_SUB(SIDE_SC_MAP_LIST_SUB1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/*
 * Used for nested list of nested list.  e.g., `side_elem' within a
 * `side_field_array' within `side_field_list'.
 */
#define SIDE_SC_MAP_LIST_SUB_SUB_NEXT1(test, next) SIDE_SC_MAP_NEXT0(test, SIDE_SC_MAP_COMMA next, 0)
#define SIDE_SC_MAP_LIST_SUB_SUB_NEXT(test, next)  SIDE_SC_MAP_LIST_SUB_SUB_NEXT1(SIDE_SC_MAP_GET_END test, next)
#define SIDE_SC_MAP_LIST_SUB_SUB0(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_SUB_SUB_NEXT(peek, SIDE_SC_MAP_LIST_SUB_SUB1)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST_SUB_SUB1(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_SUB_SUB_NEXT(peek, SIDE_SC_MAP_LIST_SUB_SUB0)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST_SUB_SUB(f, ...) SIDE_SC_EVAL_SUB_SUB(SIDE_SC_MAP_LIST_SUB_SUB1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/*
 * Used for nested list of nested list of nested list.  e.g., `side_attr_list'
 * within a `side_elem' within `side_type_array' within a `side_field_list'.
 */
#define SIDE_SC_MAP_LIST_SUB_SUB_SUB_NEXT1(test, next) SIDE_SC_MAP_NEXT0(test, SIDE_SC_MAP_COMMA next, 0)
#define SIDE_SC_MAP_LIST_SUB_SUB_SUB_NEXT(test, next)  SIDE_SC_MAP_LIST_SUB_SUB_SUB_NEXT1(SIDE_SC_MAP_GET_END test, next)
#define SIDE_SC_MAP_LIST_SUB_SUB_SUB0(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_SUB_SUB_SUB_NEXT(peek, SIDE_SC_MAP_LIST_SUB_SUB_SUB1)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST_SUB_SUB_SUB1(f, x, peek, ...) f(x) SIDE_SC_MAP_LIST_SUB_SUB_SUB_NEXT(peek, SIDE_SC_MAP_LIST_SUB_SUB_SUB0)(f, peek, __VA_ARGS__)
#define SIDE_SC_MAP_LIST_SUB_SUB_SUB(f, ...) SIDE_SC_EVAL_SUB_SUB_SUB(SIDE_SC_MAP_LIST_SUB_SUB_SUB1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/*
 * Used for combining every pairs of element.  The form:
 *
 *	SIDE_SC_MAP_COMB(f, x1, x2, x3, x4)
 *
 * will expand to:
 *
 *	f(x1, x2) f(x1, x3) f(x1, x4) f(x2, x3) f(x2, x4) f(x3, x4)
 *
 * For example:
 *
 *	SIDE_SC_MAP_COMB(SIDE_SC_CAT, x, y, z)
 *
 * will expand to:
 *
 *	xy xz yz
 */
#define SIDE_SC_MAP_COMB_NEXT1(test, next) SIDE_SC_MAP_NEXT0(test, next, 0)
#define SIDE_SC_MAP_COMB_NEXT(test, next)  SIDE_SC_MAP_COMB_NEXT1(SIDE_SC_MAP_GET_END test, next)
#define SIDE_SC_MAP_COMB0(f, p, x, peek, ...) SIDE_SC_MAP_CURRYING(f, p, x, peek, ##__VA_ARGS__) SIDE_SC_MAP_COMB_NEXT(peek, SIDE_SC_MAP_COMB1)(f, x, peek, __VA_ARGS__)
#define SIDE_SC_MAP_COMB1(f, p, x, peek, ...) SIDE_SC_MAP_CURRYING(f, p, x, peek, ##__VA_ARGS__) SIDE_SC_MAP_COMB_NEXT(peek, SIDE_SC_MAP_COMB0)(f, x, peek, __VA_ARGS__)
#define SIDE_SC_MAP_COMB(f, x, ...) SIDE_SC_EVAL_SUB(SIDE_SC_MAP_COMB1(f, x, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/* Types comparison. */

/*
 * Check that `_t1' and `_t2' are compatible types and add a `&&' token.
 */
#ifdef __cplusplus
template <typename T>
constexpr bool side_sc_type_check(T, T) { return true; }

template <typename T, typename U>
	constexpr bool side_sc_type_check(T, U) { return false; }

#  define SIDE_SC_CHECK_TYPE_COMPATIBLE(_t1, _t2)	\
	side_sc_type_check(_t1{}, _t2{}) &&
#else
#  define SIDE_SC_CHECK_TYPE_COMPATIBLE(_t1, _t2)	\
	__builtin_types_compatible_p(_t1, _t2) &&
#endif

/*
 * Implementation of types comparison.
 *
 * Since the expansion of the types has been done at the previous level,
 * `_types' is now of the form:
 *
 *	,SIDE_SC_TYPE(...) ...
 *
 * Note that the first element of `_types' is empty.  Thus the first type start
 * at the second position in `_types'.
 *
 * The comparison ensure that all types are the same.  This can be done by
 * assuming that the first type is the truth and that all other types must be
 * the same as the first.  For this, partial evaluation with currying is used.
 *
 * The first type is selected with `SIDE_SC_TAKE_2(_types)' and is used as the
 * partial evaluation for the currying.
 *
 * The mapping is done over the list of types, minus the empty element using
 * `SIDE_SC_SKIP_1(_types)'.  The reason for the first type to be check against
 * itself is to support single argument of user form input like:
 * `side_arg_list(side_arg_u32(x))'.
 *
 * The trailing 1 after the curried mapping is to accept the trailling `&&'
 * token of `SIDE_SC_CHECK_TYPE_COMPATIBLE()'.
 */
#define SIDE_SC_CHECK_TYPES_COMPATIBLE_PRIMITIVE(_types...)		\
	SIDE_SC_MAP_CURRYING(SIDE_SC_CHECK_TYPE_COMPATIBLE, SIDE_SC_TAKE_2(_types), SIDE_SC_SKIP_1(_types)) 1

/*
 * Compare `_types' in the context of `_context'.
 *
 * `_types' must be of the form:
 *
 *	side_arg_list(side_arg_*(...) ...)
 */
#define SIDE_SC_CHECK_TYPES_COMPATIBLE(_context,  _types)		\
	side_static_assert(SIDE_SC_CHECK_TYPES_COMPATIBLE_PRIMITIVE(SIDE_SC_CHECK_##_types), \
			"Types incompatible <" SIDE_SC_SOURCE_LOCATION() "> in expression: " SIDE_SC_STR(_context), \
			types_incompatible)

/*
 * String not equal of X and Y.
 *
 * X and Y needs to be stringified because they could be the empty argument.
 * Since both arguments are stringified, the comparison works the same.
 *
 * Add an extra `&&' for chaining comparisons.
 */
#define SIDE_SC_STRNEQ(x, y)						\
	(0 != __builtin_strcmp(SIDE_SC_STR(x), SIDE_SC_STR(y))) &&

/*
 * Implementation of check for duplicated/null field names.
 *
 * Since the expansion of the fields has been done at the previous level,
 * `_names' is now of the form:
 *
 *	"...", ...
 *
 * The first assertion check that all fields are not the empty string (null)
 * using a currying map.
 *
 * The second assertion check that all fields are different.
 *
 * The trailing ones are the right operands of the last `&&' of the map
 * expansions.
 */
#define SIDE_SC_CHECK_FIELDS_NAMES_PRIMITIVE2(_expr, _names...)		\
	side_static_assert(SIDE_SC_MAP_CURRYING(SIDE_SC_STRNEQ, "", ##_names) 1, \
			"Null field name <" SIDE_SC_SOURCE_LOCATION() ">: " SIDE_SC_STR(_expr), \
			null_field_name);				\
	side_static_assert(SIDE_SC_MAP_COMB(SIDE_SC_STRNEQ, _names, "") 1, \
			"Duplicated field names <" SIDE_SC_SOURCE_LOCATION() ">: " SIDE_SC_STR(_expr), \
			duplicated_field_names)


#define SIDE_SC_CHECK_FIELDS_NAMES_PRIMITIVE(_expr, _names...)		\
	SIDE_SC_CHECK_FIELDS_NAMES_PRIMITIVE2(_expr, SIDE_SC_SKIP_1(_names))

/*
 * Check for duplicated/null field names.
 *
 * `_fields' is of the form:
 *
 *	side_field_list(side_field_*(...) ...)
 */
#define SIDE_SC_CHECK_FIELD_NAMES(_fields)				\
	SIDE_SC_CHECK_FIELDS_NAMES_PRIMITIVE(_fields, SIDE_SC_NAME_OF_##_fields)

/*
 * Used by SIDE_SC_NAME_OF_* macros to extract field name of the form
 * `side_field_*(_name, ...)'.
 */
#define SIDE_SC_EXTRACT_FIELD_NAME(_name, ...) ,_name

/*
 * Dispatch `CHECK' for elements of sub-level.  For examples, the form:
 * `side_field_list(side_field_*(...) ...)' will dispatch `CHECK' on its
 * sub-elements of the form `side_field_*()'.
 */
#define SIDE_SC_CHECK_THIS(_this...)		\
	SIDE_SC_CHECK_##_this


/*
 * Dispatch `CHECK' for elements of sub-sub-level.  For example, the form
 * `side_define_array(_, side_elem(...)), _)' will dispatch `CHECK' on the
 * sub-sub-element `side_elem()'.
 */
#define SIDE_SC_CHECK_THIS_SUB(_this...)	\
	SIDE_SC_CHECK_##_this

/*
 * Dispatch `CHECK' for elements of sub-sub-sub-level.  For example, the form
 * `side_field_list(side_field_array("x", side_elem(...)))' will dispatch
 * `CHECK' on the sub-sub-sub-element `side_elem()'.
 */
#define SIDE_SC_CHECK_THIS_SUB_SUB(_this...)	\
	SIDE_SC_CHECK_##_this

/*
 * Dispatch `EMIT' for elements of sub-level.
 */
#define SIDE_SC_EMIT_THIS(_this...)		\
	SIDE_SC_EMIT_##_this


/*
 * Dispatch `EMIT' for elements of sub-sub-level.
 */
#define SIDE_SC_EMIT_THIS_SUB(_this...)		\
	SIDE_SC_EMIT_##_this

/*
 * Dispatch `EMIT' for elements of sub-sub-sub-level.
 */
#define SIDE_SC_EMIT_THIS_SUB_SUB(_this...)	\
	SIDE_SC_EMIT_##_this

/*
 * Dispatch `EMIT' for elements of sub-sub-sub-sub-level.
 *
 * There is not equivalent of this macro for `CHECK' dispatching.  This is
 * because this level of nesting is only done by `side_attr_list()' form, which
 * are never expanded during a `CHECK' dispatching.
 */
#define SIDE_SC_EMIT_THIS_SUB_SUB_SUB(_this...)	\
	SIDE_SC_EMIT_##_this

/*
 * User input forms can have trailing comma and empty elements in them.  When
 * dispatching , these empty element will expand to `SIDE_SC_*_'.  Expanding
 * these macros to nothing remove the empty elements and the trailing comma in a
 * list.
 */
#define SIDE_SC_CHECK_
#define SIDE_SC_NAME_OF_
#define SIDE_SC_EMIT_

/*
 * All types defined by the static checker are prefixed with `side_sc_type_'.
 */
#define SIDE_SC_TYPE(name) side_sc_type_##name

/*
 * Define a new static checker type.  Diagnostics is set around the definition
 * in case the type is never used (-Wunused-local-typedefs).
 */
#define SIDE_SC_DEFINE_TYPE(name)		\
	SIDE_SC_BEGIN_DIAGNOSTIC();		\
	typedef struct {			\
	} SIDE_SC_TYPE(name);			\
	SIDE_SC_END_DIAGNOSTIC()

/*
 * Used by non variadic event.
 */
#define SIDE_SC_CHECK_EVENT(_identifier, _lst)				\
	SIDE_SC_CHECK_EVENT_PRIMITIVE(_identifier, SIDE_SC_CHECK_##_lst)

/*
 * Used by variadic event.
 *
 * NOTE: The usage of the `variadic' type instead of `...' is because Clang
 * says:
 *
 *	"ISO C requires a named parameter before '...'"
 */
SIDE_SC_DEFINE_TYPE(variadic);
#define SIDE_SC_CHECK_EVENT_VARIADIC(_identifier, _lst)			\
	SIDE_SC_CHECK_EVENT_PRIMITIVE(_identifier, SIDE_SC_CHECK_##_lst, SIDE_SC_TYPE(variadic))

#define SIDE_SC_CHECK_EVENT_CALL(_identifier, _lst)			\
	SIDE_SC_CHECK_EVENT_CALL_PRIMITIVE(_identifier, SIDE_SC_CHECK_##_lst)

#define SIDE_SC_CHECK_EVENT_CALL_VARIADIC(_identifier, _lst)		\
	SIDE_SC_CHECK_EVENT_CALL_PRIMITIVE(_identifier, SIDE_SC_CHECK_##_lst, SIDE_SC_TYPE(variadic))

#ifdef __cplusplus

/*
 * Define a static inline that encodes the event description.
 *
 * Comparison of event description with call sites is made by storing a pointer
 * of the function to a local variable.
 *
 * `_lst' has already been expanded at lower level and is now of the form:
 *
 *	,SIDE_SC_TYPE(...) ... [,...]
 *
 * Therefore, the first element is skip as always for supporting trailing comma
 * in user input form.
 */
#  define SIDE_SC_CHECK_EVENT_PRIMITIVE(_identifier, _lst...)		\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	static inline void SIDE_SC_TYPE(user_define_event__##_identifier) (SIDE_SC_SKIP_1(_lst)) { } \
	SIDE_SC_END_DIAGNOSTIC()

#  define SIDE_SC_CHECK_EVENT_CALL_PRIMITIVE(_identifier, _lst...)	\
	void (*__foo)(SIDE_SC_SKIP_1(_lst)) = SIDE_SC_TYPE(user_define_event__##_identifier); \
	(void)__foo
#else

/*
 * Define an external function that encodes either the event description
 * or the event call site.
 *
 * Mismatches between a event description and event call sites are
 * caught by the compiler when the repeated function signature does not
 * match.
 *
 * `_lst' has already been expanded at lower level and is now of the form:
 *
 *	,SIDE_SC_TYPE(...) ... [,...]
 *
 * Therefore, the first element is skip as always for supporting trailing comma
 * in user input form.
 */
#  define SIDE_SC_CHECK_EVENT_PRIMITIVE(_identifier, _lst...)		\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	extern void SIDE_SC_TYPE(user_define_event__##_identifier) (SIDE_SC_SKIP_1(_lst)); \
	SIDE_SC_END_DIAGNOSTIC()

#  define SIDE_SC_CHECK_EVENT_CALL_PRIMITIVE SIDE_SC_CHECK_EVENT_PRIMITIVE
#endif

/* Dispatch: length */
#undef side_length
#define SIDE_SC_EMIT_side_length(_element_type)		\
	_side_length(SIDE_SC_EMIT_##_element_type)

#undef side_length
#define SIDE_SC_EMIT_SUB_side_length(_element_type)	\
	_side_length(SIDE_SC_EMIT_##_element_type)

/* Dispatch: element */
#undef side_elem
#define SIDE_SC_CHECK_side_elem(_element_type)	\
	SIDE_SC_CHECK_##_element_type

#define SIDE_SC_EMIT_side_elem(_element_type)		\
	_side_elem(SIDE_SC_EMIT_##_element_type)

#define SIDE_SC_CHECK_SUB_side_elem(_element_type)	\
	SIDE_SC_CHECK_##_element_type

#define SIDE_SC_EMIT_SUB_side_elem(_element_type)	\
	_side_elem(SIDE_SC_EMIT_##_element_type)

/* Dispatch: attribute */
#undef SIDE_DEFAULT_ATTR
#define SIDE_DEFAULT_ATTR(_arg0, _arg1, ...) SIDE_SC_EMIT_##_arg1

#define SIDE_SC_EMIT_SIDE_DEFAULT_ATTR(_arg0, _arg1, ...) SIDE_SC_EMIT_SUB_##_arg1
#define SIDE_SC_EMIT_SUB_SIDE_DEFAULT_ATTR(_arg0, _arg1, ...) SIDE_SC_EMIT_SUB_SUB_##_arg1
#define SIDE_SC_EMIT_SUB_SUB_SIDE_DEFAULT_ATTR(_arg0, _arg1, ...) SIDE_SC_EMIT_SUB_SUB_SUB_##_arg1

/*
 * Attribute lists can be either part of:
 *
 *	- Event description
 *	- Field description
 *	- Element type description
 *
 * For element type description, the attribute lists can be nested within a
 * `side_elem' that is itself withing a `side_type_array' that is itself in
 * another `side_elem' nested within a `side_field_list'.  Therefore, it is
 * necessary to map using the SUB_SUB_SUB form.
 */
#undef side_attr_list
#define SIDE_SC_EMIT_side_attr_list(_attr...)				\
	_side_attr_list(SIDE_SC_MAP_LIST_SUB_SUB_SUB(SIDE_SC_EMIT_THIS_SUB_SUB_SUB, _attr))

#define SIDE_SC_EMIT_side_dynamic_attr_list(_attr...)	\
	SIDE_SC_side_dynamic_attr_list(_attr)

#define _side_allocate_dynamic_SIDE_SC_side_dynamic_attr_list(...)	\
	SIDE_DYNAMIC_COMPOUND_LITERAL(const struct side_attr,		\
				SIDE_SC_MAP_LIST_SUB_SUB_SUB(SIDE_SC_EMIT_THIS_SUB_SUB_SUB, __VA_ARGS__))

#define _side_allocate_static_SIDE_SC_side_dynamic_attr_list(...)	\
	SIDE_COMPOUND_LITERAL(const struct side_attr,			\
			SIDE_SC_MAP_LIST_SUB_SUB_SUB(SIDE_SC_EMIT_THIS_SUB_SUB_SUB, __VA_ARGS__))

#define SIDE_SC_EMIT_SUB_side_attr_list SIDE_SC_EMIT_side_attr_list
#define SIDE_SC_EMIT_SUB_SUB_side_attr_list SIDE_SC_EMIT_side_attr_list
#define SIDE_SC_EMIT_SUB_SUB_SUB_side_attr_list SIDE_SC_EMIT_side_attr_list

#define SIDE_SC_EMIT_SUB_side_dynamic_attr_list SIDE_SC_EMIT_side_dynamic_attr_list
#define SIDE_SC_EMIT_SUB_SUB_side_dynamic_attr_list SIDE_SC_EMIT_side_dynamic_attr_list
#define SIDE_SC_EMIT_SUB_SUB_SUB_side_dynamic_attr_list SIDE_SC_EMIT_side_dynamic_attr_list

#undef side_attr
#define SIDE_SC_EMIT_side_attr(_name, _attr)	\
	_side_attr(_name, SIDE_SC_EMIT_##_attr)

#undef side_attr_bool
#define SIDE_SC_EMIT_side_attr_bool _side_attr_bool

#undef side_attr_u8
#define SIDE_SC_EMIT_side_attr_u8 _side_attr_u8

#undef side_attr_u16
#define SIDE_SC_EMIT_side_attr_u16 _side_attr_u16

#undef side_attr_u32
#define SIDE_SC_EMIT_side_attr_u32 _side_attr_u32

#undef side_attr_u64
#define SIDE_SC_EMIT_side_attr_u64 _side_attr_u64

#undef side_attr_u128
#define SIDE_SC_EMIT_side_attr_u128 _side_attr_u128

#undef side_attr_s8
#define SIDE_SC_EMIT_side_attr_s8 _side_attr_s8

#undef side_attr_s16
#define SIDE_SC_EMIT_side_attr_s16 _side_attr_s16

#undef side_attr_s32
#define SIDE_SC_EMIT_side_attr_s32 _side_attr_s32

#undef side_attr_s64
#define SIDE_SC_EMIT_side_attr_s64 _side_attr_s64

#undef side_attr_s128
#define SIDE_SC_EMIT_side_attr_s128 _side_attr_s128

#undef side_attr_float_binary16
#define SIDE_SC_EMIT_side_attr_float_binary16 _side_attr_float_binary16

#undef side_attr_float_binary32
#define SIDE_SC_EMIT_side_attr_float_binary32 _side_attr_float_binary32

#undef side_attr_float_binary64
#define SIDE_SC_EMIT_side_attr_float_binary64 _side_attr_float_binary64

#undef side_attr_float_binary128
#define SIDE_SC_EMIT_side_attr_float_binary128 _side_attr_float_binary128

#undef side_attr_string
#define SIDE_SC_EMIT_side_attr_string _side_attr_string

#undef side_attr_string16
#define SIDE_SC_EMIT_side_attr_string16 _side_attr_string16

#undef side_attr_string32
#define SIDE_SC_EMIT_side_attr_string32 _side_attr_string32

/* Dispatch: field_list */
#undef side_field_list
#define SIDE_SC_CHECK_side_field_list(_lst...)	\
	SIDE_SC_MAP(SIDE_SC_CHECK_THIS, _lst)

#define SIDE_SC_NAME_OF_THIS(_field)		\
	SIDE_SC_NAME_OF_##_field

#define SIDE_SC_NAME_OF_side_field_list(_lst...)	\
	SIDE_SC_MAP(SIDE_SC_NAME_OF_THIS, _lst)

#define SIDE_SC_EMIT_side_field_list(_lst...)				\
	_side_field_list(SIDE_SC_MAP_LIST(SIDE_SC_EMIT_THIS, _lst))

/* Dispatch: arg_list */
#undef side_arg_list
#define SIDE_SC_CHECK_side_arg_list(_lst...)	\
	SIDE_SC_MAP(SIDE_SC_CHECK_THIS, _lst)

#define SIDE_SC_EMIT_side_arg_list(_lst...)				\
	_side_arg_list(SIDE_SC_MAP_LIST(SIDE_SC_EMIT_THIS, _lst))

/* Dispatch: option_list */
#undef side_option_list
#define SIDE_SC_EMIT_side_option_list(_options...) _side_option_list(SIDE_SC_MAP_LIST(SIDE_SC_EMIT_THIS, _options))

/* Dispatch: option */
#undef side_option
#define SIDE_SC_EMIT_side_option(_value, _type)	\
	_side_option(_value, SIDE_SC_EMIT_##_type)

/* Dispatch: option_range */
#undef side_option_range
#define SIDE_SC_EMIT_side_option_range(_range_begin, _range_end, _type)	\
	_side_option_range(_range_begin, _range_end, SIDE_SC_EMIT_##_type)

/* Dispatch: null */
SIDE_SC_DEFINE_TYPE(null);

#undef side_field_null
#define SIDE_SC_CHECK_side_field_null(...) ,SIDE_SC_TYPE(null)
#define SIDE_SC_NAME_OF_side_field_null SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_null _side_field_null

#undef side_arg_null
#define SIDE_SC_CHECK_side_arg_null(...) ,SIDE_SC_TYPE(null)
#define SIDE_SC_EMIT_side_arg_null _side_arg_null

/* Dispatch: bool */
/*
 * bool will expand to _Bool in some cases but not always dues to some complex
 * expansion above, resulting in having mismatch of types.  Therefore the
 * definition of `Bool' instead of `bool'.
 */
SIDE_SC_DEFINE_TYPE(Bool);

#undef side_field_bool
#define SIDE_SC_CHECK_side_field_bool(...) ,SIDE_SC_TYPE(Bool)
#define SIDE_SC_NAME_OF_side_field_bool SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_bool _side_field_bool

#undef side_arg_bool
#define SIDE_SC_CHECK_side_arg_bool(...) ,SIDE_SC_TYPE(Bool)
#define SIDE_SC_EMIT_side_arg_bool _side_arg_bool

/* Dispatch: byte */
SIDE_SC_DEFINE_TYPE(byte);

#undef side_field_byte

#define SIDE_SC_CHECK_side_field_byte(...) ,SIDE_SC_TYPE(byte)
#define SIDE_SC_NAME_OF_side_field_byte SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_byte _side_field_byte

#undef side_arg_byte
#define SIDE_SC_CHECK_side_arg_byte(...) ,SIDE_SC_TYPE(byte)
#define SIDE_SC_EMIT_side_arg_byte _side_arg_byte

/* Dispatch: string */
SIDE_SC_DEFINE_TYPE(string);

#undef side_field_string

#define SIDE_SC_CHECK_side_field_string(...) ,SIDE_SC_TYPE(string)
#define SIDE_SC_NAME_OF_side_field_string SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string _side_field_string

#undef side_arg_string
#define SIDE_SC_CHECK_side_arg_string(...) ,SIDE_SC_TYPE(string)
#define SIDE_SC_EMIT_side_arg_string _side_arg_string

/* Dispatch: string16 */
SIDE_SC_DEFINE_TYPE(string16);

#undef side_field_string16

#define SIDE_SC_CHECK_side_field_string16(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_NAME_OF_side_field_string16 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string16 _side_field_string16

#undef side_arg_string16
#define SIDE_SC_CHECK_side_arg_string16(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_EMIT_side_arg_string16 _side_arg_string16

/* Dispatch: string32 */
SIDE_SC_DEFINE_TYPE(string32);

#undef side_field_string32
#define SIDE_SC_CHECK_side_field_string32(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_NAME_OF_side_field_string32 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string32 _side_field_string32

#undef side_arg_string32
#define SIDE_SC_CHECK_side_arg_string32(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_EMIT_side_arg_string32 _side_arg_string32

/* Dispatch: string16_le */
#undef side_field_string16_le
#define SIDE_SC_CHECK_side_field_string16_le(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_NAME_OF_side_field_string16_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string16_le _side_field_string16_le

/* Dispatch: string32_le */
#undef side_field_string32_le
#define SIDE_SC_CHECK_side_field_string32_le(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_NAME_OF_side_field_string32_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string32_le _side_field_string32_le

/* Dispatch: string16_be */
#undef side_field_string16_be
#define SIDE_SC_CHECK_side_field_string16_be(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_NAME_OF_side_field_string16_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string16_be _side_field_string16_be

/* Dispatch: string32_be */
#undef side_field_string32_be
#define SIDE_SC_CHECK_side_field_string32_be(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_NAME_OF_side_field_string32_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_string32_be _side_field_string32_be

/* Dispatch: pointer */
SIDE_SC_DEFINE_TYPE(pointer);

#undef side_field_pointer
#define SIDE_SC_CHECK_side_field_pointer(...) ,SIDE_SC_TYPE(pointer)
#define SIDE_SC_NAME_OF_side_field_pointer SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_pointer _side_field_pointer

#undef side_arg_pointer
#define SIDE_SC_CHECK_side_arg_pointer(...) ,SIDE_SC_TYPE(pointer)
#define SIDE_SC_EMIT_side_arg_pointer _side_arg_pointer

/* Dispatch: pointer_le */
#undef side_field_pointer_le
#define SIDE_SC_CHECK_side_field_pointer_le(...) ,SIDE_SC_TYPE(pointere)
#define SIDE_SC_NAME_OF_side_field_pointer_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_pointer_le _side_field_pointer_le

/* Dispatch: pointer_be */
#undef side_field_pointer_be
#define SIDE_SC_CHECK_side_field_pointer_be(...) ,SIDE_SC_TYPE(pointer)
#define SIDE_SC_NAME_OF_side_field_pointer_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_pointer_be _side_field_pointer_be

/* Dispatch: float_binary16 */
SIDE_SC_DEFINE_TYPE(float16);

#undef side_field_float_binary16
#define SIDE_SC_CHECK_side_field_float_binary16(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_NAME_OF_side_field_float_binary16 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary16 _side_field_float_binary16

#undef side_arg_float_binary16
#define SIDE_SC_CHECK_side_arg_float_binary16(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_EMIT_side_arg_float_binary16 _side_arg_float_binary16

/* Dispatch: float_binary32 */
SIDE_SC_DEFINE_TYPE(float32);

#undef side_field_float_binary32
#define SIDE_SC_CHECK_side_field_float_binary32(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_NAME_OF_side_field_float_binary32 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary32 _side_field_float_binary32

#undef side_arg_float_binary32
#define SIDE_SC_CHECK_side_arg_float_binary32(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_EMIT_side_arg_float_binary32 _side_arg_float_binary32

/* Dispatch: float_binary64 */
SIDE_SC_DEFINE_TYPE(float64);

#undef side_field_float_binary64
#define SIDE_SC_CHECK_side_field_float_binary64(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_NAME_OF_side_field_float_binary64 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary64 _side_field_float_binary64

#undef side_arg_float_binary64
#define SIDE_SC_CHECK_side_arg_float_binary64(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_EMIT_side_arg_float_binary64 _side_arg_float_binary64

/* Dispatch: float_binary128 */
SIDE_SC_DEFINE_TYPE(float128);

#undef side_field_float_binary128
#define SIDE_SC_CHECK_side_field_float_binary128(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_NAME_OF_side_field_float_binary128 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary128 _side_field_float_binary128

#undef side_arg_float_binary128
#define SIDE_SC_CHECK_side_arg_float_binary128(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_EMIT_side_arg_float_binary128 _side_arg_float_binary128

/* Dispatch: float_binary16_le */
#undef side_field_float_binary16_le
#define SIDE_SC_CHECK_side_field_float_binary16_le(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_NAME_OF_side_field_float_binary16_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary16_le _side_field_float_binary16_le

/* Dispatch: float_binary32_le */
#undef side_field_float_binary32_le
#define SIDE_SC_CHECK_side_field_float_binary32_le(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_NAME_OF_side_field_float_binary32_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary32_le _side_field_float_binary32_le

/* Dispatch: float_binary64_le */
#undef side_field_float_binary64_le
#define SIDE_SC_CHECK_side_field_float_binary64_le(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_NAME_OF_side_field_float_binary64_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary64_le _side_field_float_binary64_le

/* Dispatch: float_binary128_le */
#undef side_field_float_binary128_le
#define SIDE_SC_CHECK_side_field_float_binary128_le(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_NAME_OF_side_field_float_binary128_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary128_le _side_field_float_binary128_le

/* Dispatch: float_binary16_be */
#undef side_field_float_binary16_be
#define SIDE_SC_CHECK_side_field_float_binary16_be(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_NAME_OF_side_field_float_binary16_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary16_be _side_field_float_binary16_be

/* Dispatch: float_binary32_be */
#undef side_field_float_binary32_be
#define SIDE_SC_CHECK_side_field_float_binary32_be(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_NAME_OF_side_field_float_binary32_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary32_be _side_field_float_binary32_be

/* Dispatch: float_binary64_be */
#undef side_field_float_binary64_be
#define SIDE_SC_CHECK_side_field_float_binary64_be(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_NAME_OF_side_field_float_binary64_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary64_be _side_field_float_binary64_be

/* Dispatch: float_binary128_be */
#undef side_field_float_binary128_be
#define SIDE_SC_CHECK_side_field_float_binary128_be(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_NAME_OF_side_field_float_binary128_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float_binary128_be _side_field_float_binary128_be

/* Dispatch: char */
SIDE_SC_DEFINE_TYPE(char);

#undef side_field_char
#define SIDE_SC_CHECK_side_field_char(...) ,SIDE_SC_TYPE(char)
#define SIDE_SC_NAME_OF_side_field_char SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_char _side_field_char

#undef side_arg_char
#define SIDE_SC_CHECK_side_arg_char(...) ,SIDE_SC_TYPE(char)
#define SIDE_SC_EMIT_side_arg_char _side_arg_char

/* Dispatch: uchar */
SIDE_SC_DEFINE_TYPE(uchar);

#undef side_field_uchar

#define SIDE_SC_CHECK_side_field_uchar(...) ,SIDE_SC_TYPE(uchar)
#define SIDE_SC_NAME_OF_side_field_uchar SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_uchar _side_field_uchar

#undef side_arg_uchar
#define SIDE_SC_CHECK_side_arg_uchar(...) ,SIDE_SC_TYPE(uchar)
#define SIDE_SC_EMIT_side_arg_uchar _side_arg_uchar

/* Dispatch: schar */
SIDE_SC_DEFINE_TYPE(schar);

#undef side_field_schar
#define SIDE_SC_CHECK_side_field_schar(...) ,SIDE_SC_TYPE(schar)
#define SIDE_SC_NAME_OF_side_field_schar SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_schar _side_field_schar

#undef side_arg_schar
#define SIDE_SC_CHECK_side_arg_schar(...) ,SIDE_SC_TYPE(schar)
#define SIDE_SC_EMIT_side_arg_schar _side_arg_schar

/* Dispatch: short */
SIDE_SC_DEFINE_TYPE(short);

#undef side_field_short

#define SIDE_SC_CHECK_side_field_short(...) ,SIDE_SC_TYPE(short)
#define SIDE_SC_NAME_OF_side_field_short SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_short _side_field_short

#undef side_arg_short
#define SIDE_SC_CHECK_side_arg_short(...) ,SIDE_SC_TYPE(short)
#define SIDE_SC_EMIT_side_arg_short _side_arg_short

/* Dispatch: ushort */
SIDE_SC_DEFINE_TYPE(ushort);

#undef side_field_ushort
#define SIDE_SC_CHECK_side_field_ushort(...) ,SIDE_SC_TYPE(ushort)
#define SIDE_SC_NAME_OF_side_field_ushort SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_ushort _side_field_ushort

#undef side_arg_ushort
#define SIDE_SC_CHECK_side_arg_ushort(...) ,SIDE_SC_TYPE(ushort)
#define SIDE_SC_EMIT_side_arg_ushort _side_arg_ushort

/* Dispatch: int */
SIDE_SC_DEFINE_TYPE(int);

#undef side_field_int

#define SIDE_SC_CHECK_side_field_int(...) ,SIDE_SC_TYPE(int)
#define SIDE_SC_NAME_OF_side_field_int SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_int _side_field_int

#undef side_arg_int
#define SIDE_SC_CHECK_side_arg_int(...) ,SIDE_SC_TYPE(int)
#define SIDE_SC_EMIT_side_arg_int _side_arg_int

/* Dispatch: uint */
SIDE_SC_DEFINE_TYPE(uint);

#undef side_field_uint
#define SIDE_SC_CHECK_side_field_uint(...) ,SIDE_SC_TYPE(uint)
#define SIDE_SC_NAME_OF_side_field_uint SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_uint _side_field_uint

#undef side_arg_uint
#define SIDE_SC_CHECK_side_arg_uint(...) ,SIDE_SC_TYPE(uint)
#define SIDE_SC_EMIT_side_arg_uint _side_arg_uint

/* Dispatch: long */
SIDE_SC_DEFINE_TYPE(long);

#undef side_field_long
#define SIDE_SC_CHECK_side_field_long(...) ,SIDE_SC_TYPE(long)
#define SIDE_SC_NAME_OF_side_field_long SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_long _side_field_long

#undef side_arg_long
#define SIDE_SC_CHECK_side_arg_long(...) ,SIDE_SC_TYPE(long)
#define SIDE_SC_EMIT_side_arg_long _side_arg_long

/* Dispatch: ulong */
SIDE_SC_DEFINE_TYPE(ulong);

#undef side_field_ulong
#define SIDE_SC_CHECK_side_field_ulong(...) ,SIDE_SC_TYPE(ulong)
#define SIDE_SC_NAME_OF_side_field_ulong SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_ulong _side_field_ulong

#undef side_arg_ulong
#define SIDE_SC_CHECK_side_arg_ulong(...) ,SIDE_SC_TYPE(ulong)
#define SIDE_SC_EMIT_side_arg_ulong _side_arg_ulong

/* Dispatch: long_long */
SIDE_SC_DEFINE_TYPE(long_long);

#undef side_field_long_long
#define SIDE_SC_CHECK_side_field_long_long(...) ,SIDE_SC_TYPE(long_long)
#define SIDE_SC_NAME_OF_side_field_long_long SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_long_long _side_field_long_long

#undef side_arg_long_long
#define SIDE_SC_CHECK_side_arg_long_long(...) ,SIDE_SC_TYPE(long_long)
#define SIDE_SC_EMIT_side_arg_long_long _side_arg_long_long

/* Dispatch: ulong_long */
SIDE_SC_DEFINE_TYPE(ulong_long);

#undef side_field_ulong_long
#define SIDE_SC_CHECK_side_field_ulong_long(...) ,SIDE_SC_TYPE(ulong_long)
#define SIDE_SC_NAME_OF_side_field_ulong_long SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_ulong_long _side_field_ulong_long

#undef side_arg_ulong_long
#define SIDE_SC_CHECK_side_arg_ulong_long(...) ,SIDE_SC_TYPE(ulong_long)
#define SIDE_SC_EMIT_side_arg_ulong_long _side_arg_ulong_long

/* Dispatch: float */
SIDE_SC_DEFINE_TYPE(float);

#undef side_field_float
#define SIDE_SC_CHECK_side_field_float(...) ,SIDE_SC_TYPE(float)
#define SIDE_SC_NAME_OF_side_field_float SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_float _side_field_float

#undef side_arg_float
#define SIDE_SC_CHECK_side_arg_float(...) ,SIDE_SC_TYPE(float)
#define SIDE_SC_EMIT_side_arg_float _side_arg_float

/* Dispatch: double */
SIDE_SC_DEFINE_TYPE(double);

#undef side_field_double
#define SIDE_SC_CHECK_side_field_double(...) ,SIDE_SC_TYPE(double)
#define SIDE_SC_NAME_OF_side_field_double SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_double _side_field_double

#undef side_arg_double
#define SIDE_SC_CHECK_side_arg_double(...) ,SIDE_SC_TYPE(double)
#define SIDE_SC_EMIT_side_arg_double _side_arg_double

/* Dispatch: long_double */
SIDE_SC_DEFINE_TYPE(long_double);

#undef side_field_long_double
#define SIDE_SC_CHECK_side_field_long_double(...) ,side_sc_long_double_type
#define SIDE_SC_NAME_OF_side_field_long_double SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_long_double _side_field_long_double

#undef side_arg_long_double
#define SIDE_SC_CHECK_side_arg_long_double(...) ,side_sc_long_double_type
#define SIDE_SC_EMIT_side_arg_long_double _side_arg_long_double

/* Dispatch: u8 */
SIDE_SC_DEFINE_TYPE(u8);

#undef side_field_u8
#define SIDE_SC_CHECK_side_field_u8(...) ,SIDE_SC_TYPE(u8)
#define SIDE_SC_NAME_OF_side_field_u8 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u8 _side_field_u8

#undef side_arg_u8
#define SIDE_SC_CHECK_side_arg_u8(...) ,SIDE_SC_TYPE(u8)
#define SIDE_SC_EMIT_side_arg_u8 _side_arg_u8

/* Dispatch: u16 */
SIDE_SC_DEFINE_TYPE(u16);

#undef side_field_u16
#define SIDE_SC_CHECK_side_field_u16(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_NAME_OF_side_field_u16 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u16 _side_field_u16

#undef side_arg_u16
#define SIDE_SC_CHECK_side_arg_u16(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_EMIT_side_arg_u16 _side_arg_u16

/* Dispatch: u32 */
SIDE_SC_DEFINE_TYPE(u32);

#undef side_field_u32
#define SIDE_SC_CHECK_side_field_u32(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_NAME_OF_side_field_u32 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u32 _side_field_u32

#undef side_arg_u32
#define SIDE_SC_CHECK_side_arg_u32(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_EMIT_side_arg_u32 _side_arg_u32

/* Dispatch: u64 */
SIDE_SC_DEFINE_TYPE(u64);

#undef side_field_u64
#define SIDE_SC_CHECK_side_field_u64(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_NAME_OF_side_field_u64 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u64 _side_field_u64

#undef side_arg_u64
#define SIDE_SC_CHECK_side_arg_u64(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_EMIT_side_arg_u64 _side_arg_u64

/* Dispatch: u128 */
SIDE_SC_DEFINE_TYPE(u128);

#undef side_field_u128
#define SIDE_SC_CHECK_side_field_u128(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_NAME_OF_side_field_u128 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u128 _side_field_u128

#undef side_arg_u128
#define SIDE_SC_CHECK_side_arg_u128(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_EMIT_side_arg_u128 _side_arg_u128

/* Dispatch: s8 */
SIDE_SC_DEFINE_TYPE(s8);

#undef side_field_s8
#define SIDE_SC_CHECK_side_field_s8(...) ,SIDE_SC_TYPE(s8)
#define SIDE_SC_NAME_OF_side_field_s8 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s8 _side_field_s8

#undef side_arg_s8
#define SIDE_SC_CHECK_side_arg_s8(...) ,SIDE_SC_TYPE(s8)
#define SIDE_SC_EMIT_side_arg_s8 _side_arg_s8

/* Dispatch: s16 */
SIDE_SC_DEFINE_TYPE(s16);

#undef side_field_s16
#define SIDE_SC_CHECK_side_field_s16(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_NAME_OF_side_field_s16 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s16 _side_field_s16

#undef side_arg_s16
#define SIDE_SC_CHECK_side_arg_s16(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_EMIT_side_arg_s16 _side_arg_s16

/* Dispatch: s32 */
SIDE_SC_DEFINE_TYPE(s32);

#undef side_field_s32
#define SIDE_SC_CHECK_side_field_s32(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_NAME_OF_side_field_s32 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s32 _side_field_s32

#undef side_arg_s32
#define SIDE_SC_CHECK_side_arg_s32(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_EMIT_side_arg_s32 _side_arg_s32

/* Dispatch: s64 */
SIDE_SC_DEFINE_TYPE(s64);

#undef side_field_s64
#define SIDE_SC_CHECK_side_field_s64(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_NAME_OF_side_field_s64 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s64 _side_field_s64

#undef side_arg_s64
#define SIDE_SC_CHECK_side_arg_s64(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_EMIT_side_arg_s64 _side_arg_s64

/* Dispatch: s128 */
SIDE_SC_DEFINE_TYPE(s128);

#undef side_field_s128
#define SIDE_SC_CHECK_side_field_s128(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_NAME_OF_side_field_s128 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s128 _side_field_s128

#undef side_arg_s128
#define SIDE_SC_CHECK_side_arg_s128(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_EMIT_side_arg_s128 _side_arg_s128

/* Dispatch: u16_le */
#undef side_field_u16_le
#define SIDE_SC_CHECK_side_field_u16_le(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_NAME_OF_side_field_u16_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u16_le _side_field_u16_le

/* Dispatch: u32_le */
#undef side_field_u32_le
#define SIDE_SC_CHECK_side_field_u32_le(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_NAME_OF_side_field_u32_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u32_le _side_field_u32_le

/* Dispatch: u64_le */
#undef side_field_u64_le
#define SIDE_SC_CHECK_side_field_u64_le(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_NAME_OF_side_field_u64_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u64_le _side_field_u64_le

/* Dispatch: u128_le */
#undef side_field_u128_le
#define SIDE_SC_CHECK_side_field_u128_le(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_NAME_OF_side_field_u128_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u128_le _side_field_u128_le

/* Dispatch: s16_le */
#undef side_field_s16_le
#define SIDE_SC_CHECK_side_field_s16_le(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_NAME_OF_side_field_s16_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s16_le _side_field_s16_le

/* Dispatch: s32_le */
#undef side_field_s32_le
#define SIDE_SC_CHECK_side_field_s32_le(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_NAME_OF_side_field_s32_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s32_le _side_field_s32_le

/* Dispatch: s64_le */
#undef side_field_s64_le
#define SIDE_SC_CHECK_side_field_s64_le(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_NAME_OF_side_field_s64_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s64_le _side_field_s64_le

/* Dispatch: s128_le */
#undef side_field_s128_le
#define SIDE_SC_CHECK_side_field_s128_le(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_NAME_OF_side_field_s128_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s128_le _side_field_s128_le

/* Dispatch: u16_be */
#undef side_field_u16_be
#define SIDE_SC_CHECK_side_field_u16_be(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_NAME_OF_side_field_u16_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u16_be _side_field_u16_be

/* Dispatch: u32_be */
#undef side_field_u32_be
#define SIDE_SC_CHECK_side_field_u32_be(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_NAME_OF_side_field_u32_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u32_be _side_field_u32_be

/* Dispatch: u64_be */
#undef side_field_u64_be
#define SIDE_SC_CHECK_side_field_u64_be(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_NAME_OF_side_field_u64_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u64_be _side_field_u64_be

/* Dispatch: u128_be */
#undef side_field_u128_be
#define SIDE_SC_CHECK_side_field_u128_be(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_NAME_OF_side_field_u128_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_u128_be _side_field_u128_be

/* Dispatch: s16_be */
#undef side_field_s16_be
#define SIDE_SC_CHECK_side_field_s16_be(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_NAME_OF_side_field_s16_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s16_be _side_field_s16_be

/* Dispatch: s32_be */
#undef side_field_s32_be
#define SIDE_SC_CHECK_side_field_s32_be(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_NAME_OF_side_field_s32_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s32_be _side_field_s32_be

/* Dispatch: s64_be */
#undef side_field_s64_be
#define SIDE_SC_CHECK_side_field_s64_be(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_NAME_OF_side_field_s64_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s64_be _side_field_s64_be

/* Dispatch: s128_be */
#undef side_field_s128_be
#define SIDE_SC_CHECK_side_field_s128_be(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_NAME_OF_side_field_s128_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_s128_be _side_field_s128_be

/* Dispatch: gather_byte */
SIDE_SC_DEFINE_TYPE(gather_byte);

#undef side_field_gather_byte
#define SIDE_SC_CHECK_side_field_gather_byte(...) ,SIDE_SC_TYPE(gather_byte)
#define SIDE_SC_NAME_OF_side_field_gather_byte SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_byte _side_field_gather_byte

#undef side_arg_gather_byte
#define SIDE_SC_CHECK_side_arg_gather_byte(...) ,SIDE_SC_TYPE(gather_byte)
#define SIDE_SC_EMIT_side_arg_gather_byte _side_arg_gather_byte

/* Dispatch: gather_bool */
SIDE_SC_DEFINE_TYPE(gather_bool);

#undef side_field_gather_bool
#define SIDE_SC_CHECK_side_field_gather_bool(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_NAME_OF_side_field_gather_bool SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_bool _side_field_gather_bool

#undef side_arg_gather_bool
#define SIDE_SC_CHECK_side_arg_gather_bool(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_EMIT_side_arg_gather_bool _side_arg_gather_bool

/* Dispatch: gather_bool_le */
#undef side_field_gather_bool_le
#define SIDE_SC_CHECK_side_field_gather_bool_le(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_NAME_OF_side_field_gather_bool_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_bool_le _side_field_gather_bool_le

/* Dispatch: gather_bool_be */
#undef side_field_gather_bool_be
#define SIDE_SC_CHECK_side_field_gather_bool_be(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_NAME_OF_side_field_gather_bool_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_bool_be _side_field_gather_bool_be

/* Dispatch: gather_integer */
SIDE_SC_DEFINE_TYPE(gather_integer);

#undef side_arg_gather_integer
#define SIDE_SC_CHECK_side_arg_gather_integer(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_integer SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_arg_gather_integer _side_arg_gather_integer

/* Dispatch: gather_unsigned_integer */
#undef side_field_gather_unsigned_integer
#define SIDE_SC_CHECK_side_field_gather_unsigned_integer(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_unsigned_integer SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_unsigned_integer _side_field_gather_unsigned_integer

/* Dispatch: gather_unsigned_integer_le */
#undef side_field_gather_unsigned_integer_le
#define SIDE_SC_CHECK_side_field_gather_unsigned_integer_le(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_unsigned_integer_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_unsigned_integer_le _side_field_gather_unsigned_integer_le

/* Dispatch: gather_unsigned_integer_be */
#undef side_field_gather_unsigned_integer_be
#define SIDE_SC_CHECK_side_field_gather_unsigned_integer_be(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_unsigned_integer_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_unsigned_integer_be _side_field_gather_unsigned_integer_be

/* Dispatch: gather_signed_integer */
#undef side_field_gather_signed_integer
#define SIDE_SC_CHECK_side_field_gather_signed_integer(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_signed_integer SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_signed_integer _side_field_gather_signed_integer

/* Dispatch: gather_signed_integer_le */
#undef side_field_gather_signed_integer_le
#define SIDE_SC_CHECK_side_field_gather_signed_integer_le(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_signed_integer_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_signed_integer_le _side_field_gather_signed_integer_le

/* Dispatch: gather_signed_integer_be */
#undef side_field_gather_signed_integer_be
#define SIDE_SC_CHECK_side_field_gather_signed_integer_be(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_NAME_OF_side_field_gather_signed_integer_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_signed_integer_be _side_field_gather_signed_integer_be

/* Dispatch: gather_pointer */
SIDE_SC_DEFINE_TYPE(gather_pointer);

#undef side_field_gather_pointer
#define SIDE_SC_CHECK_side_field_gather_pointer(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_NAME_OF_side_field_gather_pointer SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_pointer _side_field_gather_pointer

#undef side_arg_gather_pointer
#define SIDE_SC_CHECK_side_arg_gather_pointer(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_EMIT_side_arg_gather_pointer _side_arg_gather_pointer

/* Dispatch: gather_pointer_le */
#undef side_field_gather_pointer_le
#define SIDE_SC_CHECK_side_field_gather_pointer_le(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_NAME_OF_side_field_gather_pointer_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_pointer_le _side_field_gather_pointer_le

/* Dispatch: gather_pointer_be */
#undef side_field_gather_pointer_be
#define SIDE_SC_CHECK_side_field_gather_pointer_be(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_NAME_OF_side_field_gather_pointer_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_pointer_be _side_field_gather_pointer_be

/* Dispatch: gather_float */
SIDE_SC_DEFINE_TYPE(gather_float);

#undef side_field_gather_float
#define SIDE_SC_CHECK_side_field_gather_float(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_NAME_OF_side_field_gather_float SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_float _side_field_gather_float

#undef side_arg_gather_float
#define SIDE_SC_CHECK_side_arg_gather_float(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_EMIT_side_arg_gather_float _side_arg_gather_float

/* Dispatch: gather_float_le */
#undef side_field_gather_float_le
#define SIDE_SC_CHECK_side_field_gather_float_le(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_NAME_OF_side_field_gather_pointer_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_float_le _side_field_gather_float_le

/* Dispatch: gather_float_be */
#undef side_field_gather_float_be
#define SIDE_SC_CHECK_side_field_gather_float_be(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_NAME_OF_side_field_gather_pointer_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_float_be _side_field_gather_float_be

/* Dispatch: gather_string */
SIDE_SC_DEFINE_TYPE(gather_string);

#undef side_field_gather_string
#define SIDE_SC_CHECK_side_field_gather_string(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string _side_field_gather_string

#undef side_arg_gather_string
#define SIDE_SC_CHECK_side_arg_gather_string(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_arg_gather_string _side_arg_gather_string

/* Dispatch: gather_string16 */
#undef side_field_gather_string16
#define SIDE_SC_CHECK_side_field_gather_string16(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string16 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string16 _side_field_gather_string16

/* Dispatch: gather_string16_le */
#undef side_field_gather_string16_le
#define SIDE_SC_CHECK_side_field_gather_string16_le(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string16_le _side_field_gather_string16_le

/* Dispatch: gather_string16_be */
#undef side_field_gather_string16_be
#define SIDE_SC_CHECK_side_field_gather_string16_be(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string16_be _side_field_gather_string16_be

/* Dispatch: gather_string32 */
#undef side_field_gather_string32
#define SIDE_SC_CHECK_side_field_gather_string32(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string32 SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string32 _side_field_gather_string32

/* Dispatch: gather_string32_le */
#undef side_field_gather_string32_le
#define SIDE_SC_CHECK_side_field_gather_string32_le(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string32_le SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string32_le _side_field_gather_string32_le

/* Dispatch: gather_string32_be */
#undef side_field_gather_string32_be
#define SIDE_SC_CHECK_side_field_gather_string32_be(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_NAME_OF_side_field_gather_string32_be SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_string32_be _side_field_gather_string32_be

/* Dispatch: gather_enum */
#undef side_field_gather_enum
#define SIDE_SC_CHECK_side_field_gather_enum(_name, _mappings, _elem_type) SIDE_SC_CHECK_##_elem_type
#define SIDE_SC_NAME_OF_side_field_gather_enum SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_enum(_name, _mappings, _elem_type) \
	_side_field_gather_enum(_name, _mappings, SIDE_SC_EMIT_##_elem_type)

/* Dispatch: gather_struct */
SIDE_SC_DEFINE_TYPE(gather_struct);

#undef side_field_gather_struct
#define SIDE_SC_CHECK_side_field_gather_struct(...) ,SIDE_SC_TYPE(gather_struct)
#define SIDE_SC_NAME_OF_side_field_gather_struct SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_struct _side_field_gather_struct

#undef side_arg_gather_struct
#define SIDE_SC_CHECK_side_arg_gather_struct(...) ,SIDE_SC_TYPE(gather_struct)
#define SIDE_SC_EMIT_side_arg_gather_struct _side_arg_gather_struct

/* Dispatch: gather_array */
SIDE_SC_DEFINE_TYPE(gather_array);

#undef side_field_gather_array
#define SIDE_SC_CHECK_side_field_gather_array(...) ,SIDE_SC_TYPE(gather_array)
#define SIDE_SC_NAME_OF_side_field_gather_array SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_array(_name, _elem_type, _length, _offset, _access_mode, _attr...) \
	_side_field_gather_array(_name, SIDE_SC_EMIT_##_elem_type, _length, _offset, _access_mode, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#undef side_arg_gather_array
#define SIDE_SC_CHECK_side_arg_gather_array(...) ,SIDE_SC_TYPE(gather_array)
#define SIDE_SC_EMIT_side_arg_gather_array _side_arg_gather_array

/* Dispatch: gather_vla */
SIDE_SC_DEFINE_TYPE(gather_vla);

#undef side_field_gather_vla
#define SIDE_SC_CHECK_side_field_gather_vla(...) ,SIDE_SC_TYPE(gather_vla)
#define SIDE_SC_NAME_OF_side_field_gather_vla SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_gather_vla(_name, _elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	_side_field_gather_vla(_name, SIDE_SC_EMIT_##_elem_type_gather, _offset, _access_mode, \
			SIDE_SC_EMIT_##_length_type_gather,		\
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#undef side_arg_gather_vla
#define SIDE_SC_CHECK_side_arg_gather_vla(...) ,SIDE_SC_TYPE(gather_vla)
#define SIDE_SC_EMIT_side_arg_gather_vla _side_arg_gather_vla

/* Dispatch: variant */
SIDE_SC_DEFINE_TYPE(variant);

#undef side_field_variant
#define SIDE_SC_CHECK_side_field_variant(_name, _identifier) ,void (*)(SIDE_SC_TYPE(variant) SIDE_SC_TYPE(user_define_variant__##_identifier))
#define SIDE_SC_NAME_OF_side_field_variant SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_variant _side_field_variant

#undef side_arg_variant
#define SIDE_SC_CHECK_side_arg_variant(_identifier) ,SIDE_SC_TYPE(user_arg_define_variant__##_identifier)
#define SIDE_SC_EMIT_side_arg_variant _side_arg_variant

/* Dispatch: optional */
SIDE_SC_DEFINE_TYPE(optional);

#undef side_field_optional
#define SIDE_SC_CHECK_side_field_optional(_name, _identifier) ,SIDE_SC_TYPE(user_define_optional__##_identifier)
#define SIDE_SC_NAME_OF_side_field_optional SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_optional(_name, _identifier) _side_field_optional(_name, _identifier)

#undef side_field_optional_literal
#define SIDE_SC_CHECK_side_field_optional_literal(_name, _elem) ,void (*)(SIDE_SC_TYPE(optional) SIDE_SC_CHECK_##_elem)
#define SIDE_SC_NAME_OF_side_field_optional_literal SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_optional_literal(_name, _elem) _side_field_optional_literal(_name, SIDE_SC_EMIT_##_elem)

#undef side_arg_optional
#define SIDE_SC_CHECK_side_arg_optional(_identifier) ,SIDE_SC_TYPE(user_arg_define_optional__##_identifier)
#define SIDE_SC_EMIT_side_arg_optional(_identifier) _side_arg_optional(_identifier)

/* Dispatch: array */
#undef side_field_array
#define SIDE_SC_CHECK_side_field_array(_name, _identifier) ,SIDE_SC_TYPE(user_define_array__##_identifier)
#define SIDE_SC_NAME_OF_side_field_array SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_array _side_field_array

#undef side_arg_array
#define SIDE_SC_CHECK_side_arg_array(_identifier) ,SIDE_SC_TYPE(user_arg_define_array__##_identifier)
#define SIDE_SC_EMIT_side_arg_array _side_arg_array

/* Dispatch: vla */
#undef side_field_vla
#define SIDE_SC_CHECK_side_field_vla(_name, _identifier) ,SIDE_SC_TYPE(user_define_vla__##_identifier)
#define SIDE_SC_NAME_OF_side_field_vla SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_vla _side_field_vla

#undef side_arg_vla
#define SIDE_SC_CHECK_side_arg_vla(_identifier) ,SIDE_SC_TYPE(user_arg_define_vla__##_identifier)
#define SIDE_SC_EMIT_side_arg_vla _side_arg_vla

/* Dispatch: struct */
#undef side_field_struct
#define SIDE_SC_CHECK_side_field_struct(_name, _identifier) ,SIDE_SC_TYPE(user_define_struct__##_identifier)
#define SIDE_SC_NAME_OF_side_field_struct SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_struct _side_field_struct
;;
#undef side_arg_struct
#define SIDE_SC_CHECK_side_arg_struct(_identifier) ,SIDE_SC_TYPE(user_arg_define_struct__##_identifier)
#define SIDE_SC_EMIT_side_arg_struct _side_arg_struct

/* Dispatch: visitor */
#undef side_field_vla_visitor
#define SIDE_SC_CHECK_side_field_vla_visitor(_name, _identifier) ,SIDE_SC_TYPE(user_define_vla_visitor__##_identifier) *
#define SIDE_SC_EMIT_side_field_vla_visitor _side_field_vla_visitor

#undef side_arg_vla_visitor
#define SIDE_SC_CHECK_side_arg_vla_visitor(_identifier) ,SIDE_SC_TYPE(user_arg_define_vla_visitor__##_identifier)
#define SIDE_SC_EMIT_side_arg_vla_visitor _side_arg_vla_visitor

/* Dispatch: enum */
#undef side_field_enum
#define SIDE_SC_CHECK_side_field_enum(_name, _mappings, _elem) SIDE_SC_CHECK_##_elem
#define SIDE_SC_EMIT_side_field_enum(_name, _mappings, _elem)		\
	_side_field_enum(_name, _mappings, SIDE_SC_EMIT_##_elem)

#undef side_field_enum_bitmap
#define SIDE_SC_CHECK_side_field_enum_bitmap(_name, _mappings, _elem) SIDE_SC_CHECK_##_elem
#define SIDE_SC_EMIT_side_field_enum_bitmap(_name, _mappings, _elem)	\
	_side_field_enum_bitmap(_name, _mappings, SIDE_SC_EMIT_##_elem)

/* Dispatch: dynamic. */
SIDE_SC_DEFINE_TYPE(dynamic);

#undef side_field_dynamic
#define SIDE_SC_CHECK_side_field_dynamic(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_NAME_OF_side_field_dynamic SIDE_SC_EXTRACT_FIELD_NAME
#define SIDE_SC_EMIT_side_field_dynamic _side_field_dynamic

#undef side_arg_dynamic_null
#define SIDE_SC_CHECK_side_arg_dynamic_null(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_null _side_arg_dynamic_null

#undef side_arg_dynamic_bool
#define SIDE_SC_CHECK_side_arg_dynamic_bool(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_bool _side_arg_dynamic_bool

#undef side_arg_dynamic_byte
#define SIDE_SC_CHECK_side_arg_dynamic_byte(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_byte _side_arg_dynamic_byte

#undef side_arg_dynamic_string
#define SIDE_SC_CHECK_side_arg_dynamic_string(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string _side_arg_dynamic_string

#undef side_arg_dynamic_string16
#define SIDE_SC_CHECK_side_arg_dynamic_string16(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string16 _side_arg_dynamic_string16

#undef side_arg_dynamic_string16_le
#define SIDE_SC_CHECK_side_arg_dynamic_string16_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string16_le _side_arg_dynamic_string16_le

#undef side_arg_dynamic_string16_be
#define SIDE_SC_CHECK_side_arg_dynamic_string16_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string16_be _side_arg_dynamic_string16_be

#undef side_arg_dynamic_string32
#define SIDE_SC_CHECK_side_arg_dynamic_string32(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string32 _side_arg_dynamic_string32

#undef side_arg_dynamic_string32_le
#define SIDE_SC_CHECK_side_arg_dynamic_string32_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string32_le _side_arg_dynamic_string32_le

#undef side_arg_dynamic_string32_be
#define SIDE_SC_CHECK_side_arg_dynamic_string32_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_string32_be _side_arg_dynamic_string32_be

#undef side_arg_dynamic_u8
#define SIDE_SC_CHECK_side_arg_dynamic_u8(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u8 _side_arg_dynamic_u8

#undef side_arg_dynamic_u16
#define SIDE_SC_CHECK_side_arg_dynamic_u16(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u16 _side_arg_dynamic_u16

#undef side_arg_dynamic_u32
#define SIDE_SC_CHECK_side_arg_dynamic_u32(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u32 _side_arg_dynamic_u32

#undef side_arg_dynamic_u64
#define SIDE_SC_CHECK_side_arg_dynamic_u64(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u64 _side_arg_dynamic_u64

#undef side_arg_dynamic_u128
#define SIDE_SC_CHECK_side_arg_dynamic_u128(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u128 _side_arg_dynamic_u128

#undef side_arg_dynamic_s8
#define SIDE_SC_CHECK_side_arg_dynamic_s8(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s8 _side_arg_dynamic_s8

#undef side_arg_dynamic_s16
#define SIDE_SC_CHECK_side_arg_dynamic_s16(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s16 _side_arg_dynamic_s16

#undef side_arg_dynamic_s32
#define SIDE_SC_CHECK_side_arg_dynamic_s32(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s32 _side_arg_dynamic_s32

#undef side_arg_dynamic_s64
#define SIDE_SC_CHECK_side_arg_dynamic_s64(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s64 _side_arg_dynamic_s64

#undef side_arg_dynamic_s128
#define SIDE_SC_CHECK_side_arg_dynamic_s128(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s128 _side_arg_dynamic_s128

#undef side_arg_dynamic_pointer
#define SIDE_SC_CHECK_side_arg_dynamic_pointer(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_pointer _side_arg_dynamic_pointer

#undef side_arg_dynamic_float_binary16
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary16(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary16 _side_arg_dynamic_float_binary16

#undef side_arg_dynamic_float_binary32
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary32(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary32 _side_arg_dynamic_float_binary32

#undef side_arg_dynamic_float_binary64
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary64(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary64 _side_arg_dynamic_float_binary64

#undef side_arg_dynamic_float_binary128
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary128(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary128 _side_arg_dynamic_float_binary128

#undef side_arg_dynamic_u16_le
#define SIDE_SC_CHECK_side_arg_dynamic_u16_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u16_le _side_arg_dynamic_u16_le

#undef side_arg_dynamic_u32_le
#define SIDE_SC_CHECK_side_arg_dynamic_u32_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u32_le _side_arg_dynamic_u32_le

#undef side_arg_dynamic_u64_le
#define SIDE_SC_CHECK_side_arg_dynamic_u64_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u64_le _side_arg_dynamic_u64_le

#undef side_arg_dynamic_u128_le
#define SIDE_SC_CHECK_side_arg_dynamic_u128_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u128_le _side_arg_dynamic_u128_le

#undef side_arg_dynamic_s16_le
#define SIDE_SC_CHECK_side_arg_dynamic_s16_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s16_le _side_arg_dynamic_s16_le

#undef side_arg_dynamic_s32_le
#define SIDE_SC_CHECK_side_arg_dynamic_s32_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s32_le _side_arg_dynamic_s32_le

#undef side_arg_dynamic_s64_le
#define SIDE_SC_CHECK_side_arg_dynamic_s64_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s64_le _side_arg_dynamic_s64_le

#undef side_arg_dynamic_s128_le
#define SIDE_SC_CHECK_side_arg_dynamic_s128_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s128_le _side_arg_dynamic_s128_le

#undef side_arg_dynamic_pointer_le
#define SIDE_SC_CHECK_side_arg_dynamic_pointer_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_pointer_le _side_arg_dynamic_pointer_le

#undef side_arg_dynamic_float_binary16_le
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary16_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary16_le _side_arg_dynamic_float_binary16_le

#undef side_arg_dynamic_float_binary32_le
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary32_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary32_le _side_arg_dynamic_float_binary32_le

#undef side_arg_dynamic_float_binary64_le
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary64_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary64_le _side_arg_dynamic_float_binary64_le

#undef side_arg_dynamic_float_binary128_le
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary128_le(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary128_le _side_arg_dynamic_float_binary128_le

#undef side_arg_dynamic_u16_be
#define SIDE_SC_CHECK_side_arg_dynamic_u16_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u16_be _side_arg_dynamic_u16_be

#undef side_arg_dynamic_u32_be
#define SIDE_SC_CHECK_side_arg_dynamic_u32_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u32_be _side_arg_dynamic_u32_be

#undef side_arg_dynamic_u64_be
#define SIDE_SC_CHECK_side_arg_dynamic_u64_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u64_be _side_arg_dynamic_u64_be

#undef side_arg_dynamic_u128_be
#define SIDE_SC_CHECK_side_arg_dynamic_u128_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_u128_be _side_arg_dynamic_u16

#undef side_arg_dynamic_s16_be
#define SIDE_SC_CHECK_side_arg_dynamic_s16_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s16_be _side_arg_dynamic_s16_be

#undef side_arg_dynamic_s32_be
#define SIDE_SC_CHECK_side_arg_dynamic_s32_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s32_be _side_arg_dynamic_s32_be

#undef side_arg_dynamic_s64_be
#define SIDE_SC_CHECK_side_arg_dynamic_s64_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s64_be _side_arg_dynamic_s64_be

#undef side_arg_dynamic_s128_be
#define SIDE_SC_CHECK_side_arg_dynamic_s128_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_s128_be _side_arg_dynamic_s128_be

#undef side_arg_dynamic_pointer_be
#define SIDE_SC_CHECK_side_arg_dynamic_pointer_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_pointer_be _side_arg_dynamic_pointer_be

#undef side_arg_dynamic_float_binary16_be
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary16_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary16_be _side_arg_dynamic_float_binary16_be

#undef side_arg_dynamic_float_binary32_be
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary32_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary32_be _side_arg_dynamic_float_binary32_be

#undef side_arg_dynamic_float_binary64_be
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary64_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary64_be _side_arg_dynamic_float_binary64_be

#undef side_arg_dynamic_float_binary128_be
#define SIDE_SC_CHECK_side_arg_dynamic_float_binary128_be(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_float_binary128_be _side_arg_dynamic_float_binary128_be

#undef side_arg_dynamic_vla
#define SIDE_SC_CHECK_side_arg_dynamic_vla(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_vla(...) _side_arg_dynamic_vla(__VA_ARGS__)

#undef side_arg_dynamic_vla_visitor
#define SIDE_SC_CHECK_side_arg_dynamic_vla_visitor(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_vla_visitor(...) _side_arg_dynamic_vla_visitor(__VA_ARGS__)

#undef side_arg_dynamic_struct
#define SIDE_SC_CHECK_side_arg_dynamic_struct(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_struct(...) _side_arg_dynamic_struct(__VA_ARGS__)

#undef side_arg_dynamic_struct_visitor
#define SIDE_SC_CHECK_side_arg_dynamic_struct_visitor(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_arg_dynamic_struct_visitor(...) _side_arg_dynamic_struct_visitor(__VA_ARGS__)

/* Dispatch: dynamic_field */
#undef side_arg_dynamic_field
#define SIDE_SC_EMIT_side_arg_dynamic_field(_name, _elem)	\
	_side_arg_dynamic_field(_name, SIDE_SC_EMIT_##_elem)

/* Dispatch: dynamic_define_vec */
#undef side_arg_dynamic_define_vec
#define side_arg_dynamic_define_vec(_identifier, _sav, _attr...)	\
	_side_arg_dynamic_define_vec(_identifier,			\
				SIDE_SC_EMIT_##_sav,			\
				SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Dispatch: dynamic_define_struct */
#undef side_arg_dynamic_define_struct
#define side_arg_dynamic_define_struct(_identifier, _sav, _attr...)	\
	_side_arg_dynamic_define_struct(_identifier,			\
					SIDE_SC_EMIT_##_sav,		\
					SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Dispatch: type */

/* Dispatch: type_null */
#undef side_type_null
#define SIDE_SC_CHECK_side_type_null(...) ,SIDE_SC_TYPE(null)
#define SIDE_SC_EMIT_side_type_null _side_type_null

/* Dispatch: type_bool */
#undef side_type_bool
#define SIDE_SC_CHECK_side_type_bool(...) ,SIDE_SC_TYPE(Bool)
#define SIDE_SC_EMIT_side_type_bool _side_type_bool

/* Dispatch: type_byte */
#undef side_type_byte
#define SIDE_SC_CHECK_side_type_byte(...) ,SIDE_SC_TYPE(byte)
#define SIDE_SC_EMIT_side_type_byte _side_type_byte

/* Dispatch: type_variant */
#undef side_type_variant
#define SIDE_SC_CHECK_side_type_variant(_identifier) ,SIDE_SC_TYPE(user_define_variant__##_identifier)
#define SIDE_SC_EMIT_side_type_variant _side_type_variant

/* Dispatch: type_optional */
#undef side_type_optional
#define SIDE_SC_CHECK_side_type_optional(_identifier) ,SIDE_SC_TYPE(user_define_optional__##_identifier)
#define SIDE_SC_EMIT_side_type_optional(_identifier) &_identifier

/* Dispatch: type_array */
#undef side_type_array
#define SIDE_SC_CHECK_side_type_array(_identifier) ,SIDE_SC_TYPE(user_define_array__##_identifier)
#define SIDE_SC_EMIT_side_type_array _side_type_array

/* Dispatch: type_struct */
#undef side_type_struct
#define SIDE_SC_CHECK_side_type_struct(_identifier) ,SIDE_SC_TYPE(user_define_struct__##_identifier)
#define SIDE_SC_EMIT_side_type_struct _side_type_struct

/* Dispatch: type_vla */
#undef side_type_vla
#define SIDE_SC_CHECK_side_type_vla(_identifier) ,SIDE_SC_TYPE(user_define_vla__##_identifier)
#define SIDE_SC_EMIT_side_type_vla _side_type_vla

/* Dispatch: type_dynamic */
#undef side_type_dynamic
#define SIDE_SC_CHECK_side_type_dynamic(...) ,SIDE_SC_TYPE(dynamic)
#define SIDE_SC_EMIT_side_type_dynamic _side_type_dynamic

/* Dispatch: type_vla_visitor */
#undef side_type_vla_visitor
#define SIDE_SC_EMIT_side_type_vla_visitor _side_type_vla_visitor

/* Dispatch: type_pointer */
#undef side_type_pointer
#define SIDE_SC_CHECK_side_type_pointer(...) ,SIDE_SC_TYPE(pointer)
#define SIDE_SC_EMIT_side_type_pointer _side_type_pointer

/* Dispatch: type_char */
#undef side_type_char
#define SIDE_SC_CHECK_side_type_char(...) ,SIDE_SC_TYPE(char)
#define SIDE_SC_EMIT_side_type_char _side_type_char
/* Dispatch: type_uchar */
#undef side_type_uchar
#define SIDE_SC_CHECK_side_type_uchar(...) ,SIDE_SC_TYPE(uchar)
#define SIDE_SC_EMIT_side_type_uchar _side_type_uchar
/* Dispatch: type_schar */
#undef side_type_schar
#define SIDE_SC_CHECK_side_type_schar(...) ,SIDE_SC_TYPE(schar)
#define SIDE_SC_EMIT_side_type_schar _side_type_schar
/* Dispatch: type_short */
#undef side_type_short
#define SIDE_SC_CHECK_side_type_short(...) ,SIDE_SC_TYPE(short)
#define SIDE_SC_EMIT_side_type_short _side_type_short
/* Dispatch: type_ushort */
#undef side_type_ushort
#define SIDE_SC_CHECK_side_type_ushort(...) ,SIDE_SC_TYPE(ushort)
#define SIDE_SC_EMIT_side_type_ushort _side_type_ushort
/* Dispatch: type_int */
#undef side_type_int
#define SIDE_SC_CHECK_side_type_int(...) ,SIDE_SC_TYPE(int)
#define SIDE_SC_EMIT_side_type_int _side_type_int
/* Dispatch: type_uint */
#undef side_type_uint
#define SIDE_SC_CHECK_side_type_uint(...) ,SIDE_SC_TYPE(uint)
#define SIDE_SC_EMIT_side_type_uint _side_type_uint
/* Dispatch: type_long */
#undef side_type_long
#define SIDE_SC_CHECK_side_type_long(...) ,SIDE_SC_TYPE(long)
#define SIDE_SC_EMIT_side_type_long _side_type_long
/* Dispatch: type_ulong */
#undef side_type_ulong
#define SIDE_SC_CHECK_side_type_ulong(...) ,SIDE_SC_TYPE(ulong)
#define SIDE_SC_EMIT_side_type_ulong _side_type_ulong
/* Dispatch: type_long_long */
#undef side_type_long_long
#define SIDE_SC_CHECK_side_type_long_long(...) ,SIDE_SC_TYPE(long_long)
#define SIDE_SC_EMIT_side_type_long_long _side_type_long_long
/* Dispatch: type_ulong_long */
#undef side_type_ulong_long
#define SIDE_SC_CHECK_side_type_ulong_long(...) ,SIDE_SC_TYPE(ulong_long)
#define SIDE_SC_EMIT_side_type_ulong_long _side_type_ulong_long
/* Dispatch: type_float */
#undef side_type_float
#define SIDE_SC_CHECK_side_type_float(...) ,SIDE_SC_TYPE(float)
#define SIDE_SC_EMIT_side_type_float _side_type_float
/* Dispatch: type_double */
#undef side_type_double
#define SIDE_SC_CHECK_side_type_double(...) ,SIDE_SC_TYPE(double)
#define SIDE_SC_EMIT_side_type_double _side_type_double
/* Dispatch: type_long_double */
#undef side_type_long_double
#define SIDE_SC_CHECK_side_type_long_double(...) ,SIDE_SC_TYPE(long_double)
#define SIDE_SC_EMIT_side_type_long_double _side_type_long_double

/* Dispatch: type_string */
#undef side_type_string
#define SIDE_SC_CHECK_side_type_string(...) ,SIDE_SC_TYPE(string)
#define SIDE_SC_EMIT_side_type_string _side_type_string

/* Dispatch: type_u8 */
#undef side_type_u8
#define SIDE_SC_CHECK_side_type_u8(...) ,SIDE_SC_TYPE(u8)
#define SIDE_SC_EMIT_side_type_u8 _side_type_u8

/* Dispatch: type_u16 */
#undef side_type_u16
#define SIDE_SC_CHECK_side_type_u16(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_EMIT_side_type_u16 _side_type_u16

/* Dispatch: type_u32 */
#undef side_type_u32
#define SIDE_SC_CHECK_side_type_u32(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_EMIT_side_type_u32 _side_type_u32

/* Dispatch: type_u64 */
#undef side_type_u64
#define SIDE_SC_CHECK_side_type_u64(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_EMIT_side_type_u64 _side_type_u64

/* Dispatch: type_u128 */
#undef side_type_u128
#define SIDE_SC_CHECK_side_type_u128(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_EMIT_side_type_u128 _side_type_u128

/* Dispatch: type_s8 */
#undef side_type_s8
#define SIDE_SC_CHECK_side_type_s8(...) ,SIDE_SC_TYPE(s8)
#define SIDE_SC_EMIT_side_type_s8 _side_type_s8

/* Dispatch: type_s16 */
#undef side_type_s16
#define SIDE_SC_CHECK_side_type_s16(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_EMIT_side_type_s16 _side_type_s16

/* Dispatch: type_s32 */
#undef side_type_s32
#define SIDE_SC_CHECK_side_type_s32(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_EMIT_side_type_s32 _side_type_s32

/* Dispatch: type_s64 */
#undef side_type_s64
#define SIDE_SC_CHECK_side_type_s64(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_EMIT_side_type_s64 _side_type_s64

/* Dispatch: type_s128 */
#undef side_type_s128
#define SIDE_SC_CHECK_side_type_s128(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_EMIT_side_type_s128 _side_type_s128

/* Dispatch: type_float_binary16 */
#undef side_type_float_binary16
#define SIDE_SC_CHECK_side_type_float_binary16(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_EMIT_side_type_float_binary16 _side_type_float_binary16

/* Dispatch: type_float_binary32 */
#undef side_type_float_binary32
#define SIDE_SC_CHECK_side_type_float_binary32(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_EMIT_side_type_float_binary32 _side_type_float_binary32

/* Dispatch: type_float_binary64 */
#undef side_type_float_binary64
#define SIDE_SC_CHECK_side_type_float_binary64(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_EMIT_side_type_float_binary64 _side_type_float_binary64

/* Dispatch: type_float_binary128 */
#undef side_type_float_binary128
#define SIDE_SC_CHECK_side_type_float_binary128(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_EMIT_side_type_float_binary128 _side_type_float_binary128

/* Dispatch: type_string16 */
#undef side_type_string16
#define SIDE_SC_CHECK_side_type_string16(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_EMIT_side_type_string16 _side_type_string16

/* Dispatch: type_string32 */
#undef side_type_string32
#define SIDE_SC_CHECK_side_type_string32(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_EMIT_side_type_string32 _side_type_string32

/* Dispatch: type_u16_le */
#undef side_type_u16_le
#define SIDE_SC_CHECK_side_type_u16_le(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_EMIT_side_type_u16_le _side_type_u16_le

/* Dispatch: type_u32_le */
#undef side_type_u32_le
#define SIDE_SC_CHECK_side_type_u32_le(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_EMIT_side_type_u32_le _side_type_u32_le

/* Dispatch: type_u64_le */
#undef side_type_u64_le
#define SIDE_SC_CHECK_side_type_u64_le(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_EMIT_side_type_u64_le _side_type_u64_le

/* Dispatch: type_u128_le */
#undef side_type_u128_le
#define SIDE_SC_CHECK_side_type_u128_le(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_EMIT_side_type_u128_le _side_type_u128_le

/* Dispatch: type_s16_le */
#undef side_type_s16_le
#define SIDE_SC_CHECK_side_type_s16_le(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_EMIT_side_type_s16_le _side_type_s16_le

/* Dispatch: type_s32_le */
#undef side_type_s32_le
#define SIDE_SC_CHECK_side_type_s32_le(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_EMIT_side_type_s32_le _side_type_s32_le

/* Dispatch: type_s64_le */
#undef side_type_s64_le
#define SIDE_SC_CHECK_side_type_s64_le(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_EMIT_side_type_s64_le _side_type_s64_le

/* Dispatch: type_s128_le */
#undef side_type_s128_le
#define SIDE_SC_CHECK_side_type_s128_le(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_EMIT_side_type_s128_le _side_type_s128_le

/* Dispatch: type_float_binary16_le */
#undef side_type_float_binary16_le
#define SIDE_SC_CHECK_side_type_float_binary16_le(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_EMIT_side_type_float_binary16_le _side_type_float_binary16_le

/* Dispatch: type_float_binary32_le */
#undef side_type_float_binary32_le
#define SIDE_SC_CHECK_side_type_float_binary32_le(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_EMIT_side_type_float_binary32_le _side_type_float_binary32_le

/* Dispatch: type_float_binary64_le */
#undef side_type_float_binary64_le
#define SIDE_SC_CHECK_side_type_float_binary64_le(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_EMIT_side_type_float_binary64_le _side_type_float_binary64_le

/* Dispatch: type_float_binary128_le */
#undef side_type_float_binary128_le
#define SIDE_SC_CHECK_side_type_float_binary128_le(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_EMIT_side_type_float_binary128_le _side_type_float_binary128_le

/* Dispatch: type_string16_le */
#undef side_type_string16_le
#define SIDE_SC_CHECK_side_type_string16_le(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_EMIT_side_type_string16_le _side_type_string16_le

/* Dispatch: type_string32_le */
#undef side_type_string32_le
#define SIDE_SC_CHECK_side_type_string32_le(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_EMIT_side_type_string32_le _side_type_string32_le

/* Dispatch: type_u16_be */
#undef side_type_u16_be
#define SIDE_SC_CHECK_side_type_u16_be(...) ,SIDE_SC_TYPE(u16)
#define SIDE_SC_EMIT_side_type_u16_be _side_type_u16_be

/* Dispatch: type_u32_be */
#undef side_type_u32_be
#define SIDE_SC_CHECK_side_type_u32_be(...) ,SIDE_SC_TYPE(u32)
#define SIDE_SC_EMIT_side_type_u32_be _side_type_u32_be

/* Dispatch: type_u64_be */
#undef side_type_u64_be
#define SIDE_SC_CHECK_side_type_u64_be(...) ,SIDE_SC_TYPE(u64)
#define SIDE_SC_EMIT_side_type_u64_be _side_type_u64_be

/* Dispatch: type_u128_be */
#undef side_type_u128_be
#define SIDE_SC_CHECK_side_type_u128_be(...) ,SIDE_SC_TYPE(u128)
#define SIDE_SC_EMIT_side_type_u128_be _side_type_u128_be

/* Dispatch: type_s16_be */
#undef side_type_s16_be
#define SIDE_SC_CHECK_side_type_s16_be(...) ,SIDE_SC_TYPE(s16)
#define SIDE_SC_EMIT_side_type_s16_be _side_type_s16_be

/* Dispatch: type_s32_be */
#undef side_type_s32_be
#define SIDE_SC_CHECK_side_type_s32_be(...) ,SIDE_SC_TYPE(s32)
#define SIDE_SC_EMIT_side_type_s32_be _side_type_s32_be

/* Dispatch: type_s64_be */
#undef side_type_s64_be
#define SIDE_SC_CHECK_side_type_s64_be(...) ,SIDE_SC_TYPE(s64)
#define SIDE_SC_EMIT_side_type_s64_be _side_type_s64_be

/* Dispatch: type_s128_be */
#undef side_type_s128_be
#define SIDE_SC_CHECK_side_type_s128_be(...) ,SIDE_SC_TYPE(s128)
#define SIDE_SC_EMIT_side_type_s128_be _side_type_s128_be

/* Dispatch: type_float_binary16_be */
#undef side_type_float_binary16_be
#define SIDE_SC_CHECK_side_type_float_binary16_be(...) ,SIDE_SC_TYPE(float16)
#define SIDE_SC_EMIT_side_type_float_binary16_be _side_type_float_binary16_be

/* Dispatch: type_float_binary32_be */
#undef side_type_float_binary32_be
#define SIDE_SC_CHECK_side_type_float_binary32_be(...) ,SIDE_SC_TYPE(float32)
#define SIDE_SC_EMIT_side_type_float_binary32_be _side_type_float_binary32_be

/* Dispatch: type_float_binary64_be */
#undef side_type_float_binary64_be
#define SIDE_SC_CHECK_side_type_float_binary64_be(...) ,SIDE_SC_TYPE(float64)
#define SIDE_SC_EMIT_side_type_float_binary64_be _side_type_float_binary64_be

/* Dispatch: type_float_binary128_be */
#undef side_type_float_binary128_be
#define SIDE_SC_CHECK_side_type_float_binary128_be(...) ,SIDE_SC_TYPE(float128)
#define SIDE_SC_EMIT_side_type_float_binary128_be _side_type_float_binary128_be

/* Dispatch: type_string16_be */
#undef side_type_string16_be
#define SIDE_SC_CHECK_side_type_string16_be(...) ,SIDE_SC_TYPE(string16)
#define SIDE_SC_EMIT_side_type_string16_be _side_type_string16_be

/* Dispatch: type_string32_be */
#undef side_type_string32_be
#define SIDE_SC_CHECK_side_type_string32_be(...) ,SIDE_SC_TYPE(string32)
#define SIDE_SC_EMIT_side_type_string32_be _side_type_string32_be

/* Dispatch: type_gather_byte */
#undef side_type_gather_byte
#define SIDE_SC_CHECK_side_type_gather_byte(...) ,SIDE_SC_TYPE(gather_byte)
#define SIDE_SC_EMIT_side_type_gather_byte(...) _side_type_gather_byte(__VA_ARGS__)

/* Dispatch: type_gather_bool */
#undef side_type_gather_bool
#define SIDE_SC_CHECK_side_type_gather_bool(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_EMIT_side_type_gather_bool(...) _side_type_gather_bool(__VA_ARGS__)

/* Dispatch: type_gather_bool_le */
#undef side_type_gather_bool_le
#define SIDE_SC_CHECK_side_type_gather_bool_le(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_EMIT_side_type_gather_bool_le _side_type_gather_bool_le

/* Dispatch: type_gather_bool_be */
#undef side_type_gather_bool_be
#define SIDE_SC_CHECK_side_type_gather_bool_be(...) ,SIDE_SC_TYPE(gather_bool)
#define SIDE_SC_EMIT_side_type_gather_bool_be _side_type_gather_bool_be

/* Dispatch: type_gather_unsigned_integer */
#undef side_type_gather_unsigned_integer
#define SIDE_SC_CHECK_side_type_gather_unsigned_integer(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_EMIT_side_type_gather_unsigned_integer _side_type_gather_unsigned_integer

/* Dispatch: type_gather_unsigned_integer_le */
#undef side_type_gather_unsigned_integer_le
#define SIDE_SC_CHECK_side_type_gather_unsigned_integer_le(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_EMIT_side_type_gather_unsigned_integer_le _side_type_gather_unsigned_integer_le

/* Dispatch: type_gather_unsigned_integer_be */
#undef side_type_gather_unsigned_integer_be
#define SIDE_SC_CHECK_side_type_gather_unsigned_integer_be(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_EMIT_side_type_gather_unsigned_integer_be _side_type_gather_unsigned_integer_be

/* Dispatch: type_gather_signed_integer */
#undef side_type_gather_signed_integer
#define SIDE_SC_CHECK_side_type_gather_signed_integer(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_EMIT_side_type_gather_signed_integer _side_type_gather_signed_integer

/* Dispatch: type_gather_signed_integer_le */
#undef side_type_gather_signed_integer_le
#define SIDE_SC_CHECK_side_type_gather_signed_integer_le(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_EMIT_side_type_gather_signed_integer_le _side_type_gather_signed_integer_le

/* Dispatch: type_gather_signed_integer_be */
#undef side_type_gather_signed_integer_be
#define SIDE_SC_CHECK_side_type_gather_signed_integer_be(...) ,SIDE_SC_TYPE(gather_integer)
#define SIDE_SC_EMIT_side_type_gather_signed_integer_be _side_type_gather_signed_integer_be

/* Dispatch: type_gather_pointer */
#undef side_type_gather_pointer
#define SIDE_SC_CHECK_side_type_gather_pointer(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_EMIT_side_type_gather_pointer _side_type_gather_pointer

/* Dispatch: type_gather_pointer_le */
#undef side_type_gather_pointer_le
#define SIDE_SC_CHECK_side_type_gather_pointer_le(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_EMIT_side_type_gather_pointer_le _side_type_gather_pointer_le

/* Dispatch: type_gather_pointer_be */
#undef side_type_gather_pointer_be
#define SIDE_SC_CHECK_side_type_gather_pointer_be(...) ,SIDE_SC_TYPE(gather_pointer)
#define SIDE_SC_EMIT_side_type_gather_pointer_be _side_type_gather_pointer_be

/* Dispatch: type_gather_float */
#undef side_type_gather_float
#define SIDE_SC_CHECK_side_type_gather_float(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_EMIT_side_type_gather_float(...) _side_type_gather_float(__VA_ARGS__)

/* Dispatch: type_gather_float_le */
#undef side_type_gather_float_le
#define SIDE_SC_CHECK_side_type_gather_float_le(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_EMIT_side_type_gather_float_le _side_type_gather_float_le

/* Dispatch: type_gather_float_be */
#undef side_type_gather_float_be
#define SIDE_SC_CHECK_side_type_gather_float_be(...) ,SIDE_SC_TYPE(gather_float)
#define SIDE_SC_EMIT_side_type_gather_float_be _side_type_gather_float_be

/* Dispatch: type_gather_string */
#undef side_type_gather_string
#define SIDE_SC_CHECK_side_type_gather_string(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string(...) _side_type_gather_string(__VA_ARGS__)

/* Dispatch: type_gather_string16 */
#undef side_type_gather_string16
#define SIDE_SC_CHECK_side_type_gather_string16(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string16 _side_type_gather_string16

/* Dispatch: type_gather_string16_le */
#undef side_type_gather_string16_le
#define SIDE_SC_CHECK_side_type_gather_string16_le(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string16_le _side_type_gather_string16_le

/* Dispatch: type_gather_string16_be */
#undef side_type_gather_string16_be
#define SIDE_SC_CHECK_side_type_gather_string16_be(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string16_be _side_type_gather_string16_be

/* Dispatch: type_gather_string32 */
#undef side_type_gather_string32
#define SIDE_SC_CHECK_side_type_gather_string32(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string32 _side_type_gather_string32

/* Dispatch: type_gather_string32_le */
#undef side_type_gather_string32_le
#define SIDE_SC_CHECK_side_type_gather_string32_le(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string32_le _side_type_gather_string32_le

/* Dispatch: type_gather_string32_be */
#undef side_type_gather_string32_be
#define SIDE_SC_CHECK_side_type_gather_string32_be(...) ,SIDE_SC_TYPE(gather_string)
#define SIDE_SC_EMIT_side_type_gather_string32_be _side_type_gather_string32_be

/* Dispatch: type_gather_struct */
#undef side_type_gather_struct
#define SIDE_SC_CHECK_side_type_gather_struct(...) ,SIDE_SC_TYPE(gather_struct)
#define SIDE_SC_EMIT_side_type_gather_struct(...) _side_type_gather_struct(__VA_ARGS__)

/* Dispatch: type_gather_array */
#undef side_type_gather_array
#define SIDE_SC_CHECK_side_type_gather_array(...) ,SIDE_SC_TYPE(gather_array)
#define SIDE_SC_EMIT_side_type_gather_array(_elem_type_gather, _length, _offset, _access_mode, _attr...) \
	_side_type_gather_array(SIDE_SC_EMIT_SUB_##_elem_type_gather, _length, _offset, _access_mode, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

/* Dispatch: type_gather_vla */
#undef side_type_gather_vla
#define SIDE_SC_CHECK_side_type_gather_vla(...) ,SIDE_SC_TYPE(gather_vla)
#define SIDE_SC_EMIT_side_type_gather_vla(_elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	_side_type_gather_vla(SIDE_SC_EMIT_SUB_##_elem_type_gather, _offset, _access_mode, SIDE_SC_EMIT_SUB_##_length_type_gather, \
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

/* Dispatch: define_variant */
#undef side_define_variant
#define side_define_variant(_identifier, _selector, _options, _attr...) \
	_side_define_variant(_identifier, SIDE_SC_EMIT_##_selector, SIDE_SC_EMIT_##_options, \
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_DEFINE_VARIANT(_identifier, SIDE_SC_CHECK_##_selector)

#define SIDE_SC_DEFINE_VARIANT(_identifier, _selector)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef void (*SIDE_SC_TYPE(user_define_variant__##_identifier))(SIDE_SC_TYPE(variant) SIDE_SC_TAKE_2(_selector)); \
	SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: arg_define_variant */
#undef side_arg_define_variant
#define side_arg_define_variant(_identifier, _selector_val, _option)	\
	_side_arg_define_variant(_identifier, SIDE_SC_EMIT_##_selector_val, SIDE_SC_EMIT_##_option); \
	SIDE_SC_DEFINE_ARG_VARIANT(_identifier, SIDE_SC_CHECK_##_selector_val)

#define SIDE_SC_DEFINE_ARG_VARIANT(_identifier, _selector)		\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef void (*SIDE_SC_TYPE(user_arg_define_variant__##_identifier))(SIDE_SC_TYPE(variant) SIDE_SC_TAKE_2(_selector)); \
	SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: define_array */
#undef side_define_array
#define side_define_array(_identifier, _elem, _length, _attr...)	\
	_side_define_array(_identifier, SIDE_SC_EMIT_##_elem,		\
			_length, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_DEFINE_ARRAY(_identifier, SIDE_SC_CHECK_array_##_elem, _length)

#define SIDE_SC_DEFINE_ARRAY(_identifier, _type, _length)		\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef SIDE_SC_TAKE_2(_type) SIDE_SC_TYPE(user_define_array__##_identifier) [_length]; \
	SIDE_SC_END_DIAGNOSTIC()

#define SIDE_SC_CHECK_array_side_elem(_element)	\
	SIDE_SC_CHECK_##_element

/* Dispatch: arg_define_array */
/* FIXME: side_arg_list() must have a least one argument. */
#undef side_arg_define_array
#define side_arg_define_array(_identifier, _sav)			\
	_side_arg_define_vec(_identifier, SIDE_SC_EMIT_##_sav);		\
	SIDE_SC_DEFINE_ARG_ARRAY(_identifier, SIDE_SC_CHECK_array_##_sav); \
	SIDE_SC_CHECK_TYPES_COMPATIBLE(side_arg_define_array(_sav), _sav)

#define SIDE_SC_DEFINE_ARG_ARRAY(_identifier, _type)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef SIDE_SC_TAKE_2(_type) SIDE_SC_TYPE(user_arg_define_array__##_identifier) SIDE_SC_TAKE_3(_type); \
	SIDE_SC_END_DIAGNOSTIC()

#define SIDE_SC_CHECK_array_side_arg_list(_first, _elements...)	\
	SIDE_SC_CHECK_##_first, [1 + SIDE_ARRAY_SIZE(SIDE_PARAM((const struct side_arg []){ SIDE_SC_MAP_LIST(SIDE_SC_EMIT_THIS, _elements) }))]

/* Dispatch: define_optional */
#undef side_define_optional
#define side_define_optional(_identifier, _elem_type)			\
	_side_define_optional(_identifier, SIDE_SC_EMIT_##_elem_type);	\
	SIDE_SC_DEFINE_OPTIONAL(_identifier, SIDE_SC_CHECK_##_elem_type)

#define SIDE_SC_DEFINE_OPTIONAL(_identifier, _type)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef void (*SIDE_SC_TYPE(user_define_optional__##_identifier))(SIDE_SC_TYPE(optional) _type); \
	SIDE_SC_END_DIAGNOSTIC()

#undef side_arg_define_optional
#define side_arg_define_optional(_identifier, _arg, _selector)		\
	_side_arg_define_optional(_identifier, SIDE_SC_EMIT_##_arg, _selector); \
	SIDE_SC_DEFINE_ARG_OPTIONAL(_identifier, SIDE_SC_CHECK_##_arg)

#define SIDE_SC_DEFINE_ARG_OPTIONAL(_identifier, _arg)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef void (*SIDE_SC_TYPE(user_arg_define_optional__##_identifier))(SIDE_SC_TYPE(optional) _arg); \
	SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: define_vla */
#undef side_define_vla
#define side_define_vla(_identifier, _elem_type, _length_type, _attr...) \
	_side_define_vla(_identifier,					\
			SIDE_SC_EMIT_##_elem_type, SIDE_SC_EMIT_##_length_type, \
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_DEFINE_VLA(_identifier, SIDE_SC_CHECK_vla_##_elem_type)

#define SIDE_SC_DEFINE_VLA(_identifier, _type)				\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef SIDE_SC_TAKE_2(_type) *SIDE_SC_TYPE(user_define_vla__##_identifier); \
	SIDE_SC_END_DIAGNOSTIC()

#define SIDE_SC_CHECK_vla_side_elem(_elem)	\
	SIDE_SC_CHECK_##_elem

/* Dispatch: arg_define_vla */
/* FIXME: side_arg_list() must have a least one argument. */
#undef side_arg_define_vla
#define side_arg_define_vla(_identifier, _sav)				\
	_side_arg_define_vec(_identifier, SIDE_SC_EMIT_##_sav);		\
	SIDE_SC_DEFINE_ARG_VLA(_identifier, SIDE_SC_CHECK_vla_##_sav);	\
	SIDE_SC_CHECK_TYPES_COMPATIBLE(side_arg_define_vla(_sav), _sav)

#define SIDE_SC_DEFINE_ARG_VLA(_identifier, _type)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef SIDE_SC_TAKE_2(_type) * SIDE_SC_TYPE(user_arg_define_vla__##_identifier); \
	SIDE_SC_END_DIAGNOSTIC()

#define SIDE_SC_CHECK_vla_side_arg_list(_first, ...)	\
	SIDE_SC_CHECK_##_first

/* Dispatch: define_struct */
#undef side_define_struct
#define side_define_struct(_identifier, _fields, _attr...)		\
	_side_define_struct(_identifier, SIDE_SC_EMIT_##_fields,	\
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_DEFINE_STRUCT(_identifier, SIDE_SC_CHECK_##_fields)

#define SIDE_SC_DEFINE_STRUCT(_identifier, _fields)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef void (*SIDE_SC_TYPE(user_define_struct__##_identifier))(SIDE_SC_SKIP_1(_fields)); \
	SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: arg_define_struct */
#undef side_arg_define_struct
#define side_arg_define_struct(_identifier, _sav)			\
	_side_arg_define_vec(_identifier, SIDE_SC_EMIT_##_sav);		\
	SIDE_SC_ARG_DEFINE_STRUCT(_identifier, SIDE_SC_CHECK_##_sav)

#define SIDE_SC_ARG_DEFINE_STRUCT(_identifier, _sav)			\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef void (*SIDE_SC_TYPE(user_arg_define_struct__##_identifier))(SIDE_SC_SKIP_1(_sav)); \
	SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: visit_dynamic_arg */
#undef side_visit_dynamic_arg
#define side_visit_dynamic_arg(_what, _expr...) SIDE_SC_EMIT_##_what(_expr)

/* Dispatch: visit_dynamic_field */
#undef side_visit_dynamic_field
#define side_visit_dynamic_field(_what, _name, _expr...)		\
	_side_arg_dynamic_field(_name, SIDE_SC_EMIT_##_what(_expr))

/* Dispatch: define_static_vla_visitor */
#undef side_define_static_vla_visitor
#define side_define_static_vla_visitor(_identifier, _elem_type, _length_type, _func, _type, _attr...) \
	static enum side_visitor_status _side_vla_visitor_func_##_identifier(const struct side_tracer_visitor_ctx *_side_tracer_ctx, \
									void *_side_ctx) \
	{								\
		return _func(_side_tracer_ctx, (_type *)_side_ctx);	\
	}								\
	static const struct side_type_vla_visitor _identifier =		\
		_side_type_vla_visitor_define(SIDE_SC_EMIT_##_elem_type, SIDE_SC_EMIT_##_length_type, \
					_side_vla_visitor_func_##_identifier, \
					SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef _type SIDE_SC_TYPE(user_define_vla_visitor__##_identifier); \
	SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: arg_define_vla_visitor */
#undef side_arg_define_vla_visitor
#define side_arg_define_vla_visitor(_identifier, _ctx)			\
	_side_arg_define_vla_visitor(_identifier, _ctx);		\
	SIDE_SC_BEGIN_DIAGNOSTIC();					\
	typedef __typeof__(_ctx) SIDE_SC_TYPE(user_arg_define_vla_visitor__##_identifier); \
				 SIDE_SC_END_DIAGNOSTIC()

/* Dispatch: event_call */
#undef side_event_call
#define side_event_call(_identifier, _sav)				\
	_side_event_call(side_call, _identifier, SIDE_SC_EMIT_##_sav);	\
	SIDE_SC_CHECK_EVENT_CALL(_identifier, _sav)

/* Dispatch: event_call_variadic */
#undef side_event_call_variadic
#define side_event_call_variadic(_identifier, _sav, _var_fields, _attr...) \
	_side_event_call_variadic(side_call_variadic, _identifier, SIDE_SC_EMIT_##_sav, SIDE_SC_EMIT_##_var_fields, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list())); \
	SIDE_SC_CHECK_EVENT_CALL_VARIADIC(_identifier, _sav)

/* Dispatch: statedump_event_call */
#undef side_statedump_event_call
#define side_statedump_event_call(_identifier, _key, _sav, _attr...)	\
	_side_statedump_event_call(side_statedump_call, _identifier, _key, SIDE_SC_EMIT_##_sav); \
	SIDE_SC_CHECK_EVENT_CALL(_identifier, _sav)

/* Dispatch: statedump_event_call */
#undef side_statedump_event_call_variadic
#define side_statedump_event_call_variadic(_identifier, _key, _sav, _var_fields, _attr...) \
	_side_statedump_event_call_variadic(side_statedump_call_variadic, _identifier, _key, SIDE_SC_EMIT_##_sav, SIDE_SC_EMIT_##_var_fields, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list())); \
	SIDE_SC_CHECK_EVENT_CALL_VARIADIC(_identifier, _sav)

/* Dispatch: event */
#undef side_event
#define side_event(_identifier, _sav)					\
	do {								\
		if (side_event_enabled(_identifier)) {			\
			_side_event_call(side_call, _identifier, SIDE_SC_EMIT_##_sav); \
			SIDE_SC_CHECK_EVENT_CALL(_identifier, _sav);	\
		}							\
	} while(0)

/* Dispatch: event_variadic */
#undef side_event_variadic
#define side_event_variadic(_identifier, _sav, _var, _attr...)		\
	do {								\
		if (side_event_enabled(_identifier)) {			\
			_side_event_call_variadic(side_call_variadic, _identifier, SIDE_SC_EMIT_##_sav, SIDE_SC_EMIT_##_var, \
						SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list())); \
			SIDE_SC_CHECK_EVENT_CALL_VARIADIC(_identifier, _sav); \
		}							\
	} while(0)

/* Dispatch: statedump_event */
#undef side_statedump_event
#define side_statedump_event(_identifier, _key, _sav)			\
	do {								\
		if (side_event_enabled(_identifier)) {			\
			_side_statedump_event_call(side_call, _identifier, _key, SIDE_SC_EMIT_##_sav); \
			SIDE_SC_CHECK_EVENT_CALL(_identifier, _sav);	\
		}							\
	} while(0)

/* Dispatch: statedump_event_variadic */
#undef side_statedump_event_variadic
#define side_statedump_event_variadic(_identifier, _key, _sav, _var, _attr...) \
	do {								\
		if (side_event_enabled(_identifier)) {			\
			_side_statedump_event_call_variadic(side_call_variadic, _identifier, _key, SIDE_SC_EMIT_##_sav, SIDE_SC_EMIT_##_var, \
						SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list())); \
			SIDE_SC_CHECK_EVENT_CALL_VARIADIC(_identifier, _sav); \
		}							\
	} while(0)

/* Dispatch: static_event */
#undef side_static_event
#define side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_static_event(_identifier, _provider, _event, _loglevel, SIDE_SC_EMIT_##_fields, \
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_CHECK_EVENT(_identifier, _fields)

#undef side_static_event_variadic
#define side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_static_event_variadic(_identifier, _provider, _event, _loglevel, SIDE_SC_EMIT_##_fields, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_CHECK_EVENT_VARIADIC(_identifier, _fields)

#undef side_hidden_event
#define side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_hidden_event(_identifier, _provider, _event, _loglevel, SIDE_SC_EMIT_##_fields, \
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_CHECK_EVENT(_identifier, _fields)

#undef side_hidden_event_variadic
#define side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, SIDE_SC_EMIT_##_fields, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_dattr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_CHECK_EVENT_VARIADIC(_identifier, _fields)

#undef side_export_event
#define side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_export_event(_identifier, _provider, _event, _loglevel, SIDE_SC_EMIT_##_fields, \
			SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_CHECK_EVENT(_identifier, _fields)

#undef side_export_event_variadic
#define side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_export_event_variadic(_identifier, _provider, _event, _loglevel, SIDE_SC_EMIT_##_fields, \
				SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	SIDE_SC_CHECK_FIELD_NAMES(_fields);				\
	SIDE_SC_CHECK_EVENT_VARIADIC(_identifier, _fields)

#endif	/* _SIDE_STATIC_CHECK_H */
