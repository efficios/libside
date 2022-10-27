// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <side/trace.h>
#include "tracer.h"
#include "rcu.h"

/* Top 8 bits reserved for kernel tracer use. */
#define SIDE_EVENT_ENABLED_KERNEL_MASK			0xFF000000
#define SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK	0x80000000

/* Allow 2^24 tracers to be registered on an event. */
#define SIDE_EVENT_ENABLED_USER_MASK			0x00FFFFFF

struct side_rcu_gp_state rcu_gp;

/*
 * Lazy initialization for early use within library constructors.
 */
static bool initialized;

static
void side_init(void)
	__attribute__((constructor));

const struct side_callback side_empty_callback;

void side_call(const struct side_event_description *desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_callback *side_cb;
	unsigned int rcu_period;

	if (side_unlikely(!initialized))
		side_init();
	if (side_unlikely(desc->flags & SIDE_EVENT_FLAG_VARIADIC)) {
		printf("ERROR: unexpected variadic event description\n");
		abort();
	}
	if (side_unlikely(*desc->enabled & SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel write.
	}
	if (side_unlikely(!(*desc->enabled & SIDE_EVENT_ENABLED_USER_MASK)))
		return;

	//TODO: replace tracer_call by rcu iteration on list of registered callbacks
	tracer_call(desc, sav_desc, NULL);

	rcu_period = side_rcu_read_begin(&rcu_gp);
	for (side_cb = side_rcu_dereference(desc->callbacks); side_cb->u.call != NULL; side_cb++)
		side_cb->u.call(desc, sav_desc, side_cb->priv);
	side_rcu_read_end(&rcu_gp, rcu_period);
}

void side_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec_description *sav_desc,
	const struct side_arg_dynamic_event_struct *var_struct)
{
	const struct side_callback *side_cb;
	unsigned int rcu_period;

	if (side_unlikely(!initialized))
		side_init();
	if (side_unlikely(*desc->enabled & SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel write.
	}
	if (side_unlikely(!(*desc->enabled & SIDE_EVENT_ENABLED_USER_MASK)))
		return;

	//TODO: replace tracer_call by rcu iteration on list of registered callbacks
	tracer_call_variadic(desc, sav_desc, var_struct, NULL);

	rcu_period = side_rcu_read_begin(&rcu_gp);
	for (side_cb = side_rcu_dereference(desc->callbacks); side_cb->u.call_variadic != NULL; side_cb++)
		side_cb->u.call_variadic(desc, sav_desc, var_struct, side_cb->priv);
	side_rcu_read_end(&rcu_gp, rcu_period);
}

void side_init(void)
{
	if (initialized)
		return;
	side_rcu_gp_init(&rcu_gp);
	initialized = true;
}
