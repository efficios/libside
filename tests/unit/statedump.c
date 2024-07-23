// SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

#include <side/trace.h>

static
const char *mystr[] = {
	"abc",
	"def",
	"ghi",
};

static
int myint[] = {
	0, 1, 2, 3, 4, 5,
};

side_static_event(my_provider_event_dump1, "myprovider", "myevent_dump1", SIDE_LOGLEVEL_DEBUG,
	side_field_list(side_field_string("mystatestring"))
);

side_static_event(my_provider_event_dump2, "myprovider", "myevent_dump2", SIDE_LOGLEVEL_DEBUG,
	side_field_list(side_field_s32("mystateint"))
);

side_static_event(my_provider_event, "myprovider", "myevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(side_field_s32("myfield"))
);

static struct side_statedump_request_handle *statedump_request_handle;

static
void statedump_cb(void *statedump_request_key)
{
	size_t i;

	printf("Executing application state dump callback\n");
	side_event_cond(my_provider_event_dump1) {
		for (i = 0; i < SIDE_ARRAY_SIZE(mystr); i++) {
			side_statedump_event_call(my_provider_event_dump1,
				statedump_request_key,
				side_arg_list(side_arg_string(mystr[i])));
		}
	}
	side_event_cond(my_provider_event_dump2) {
		for (i = 0; i < SIDE_ARRAY_SIZE(myint); i++) {
			side_statedump_event_call(my_provider_event_dump2,
				statedump_request_key,
				side_arg_list(side_arg_s32(myint[i])));
		}
	}
}

static void my_constructor(void)
	__attribute((constructor));
static void my_constructor(void)
{
	side_event_description_ptr_init();
	statedump_request_handle = side_statedump_request_notification_register("mystatedump",
			statedump_cb, SIDE_STATEDUMP_MODE_AGENT_THREAD);
	if (!statedump_request_handle)
		abort();
}

static void my_destructor(void)
	__attribute((destructor));
static void my_destructor(void)
{
	side_statedump_request_notification_unregister(statedump_request_handle);
	side_event_description_ptr_exit();
}

int main()
{
	side_event(my_provider_event, side_arg_list(side_arg_s32(42)));
	return 0;
}
