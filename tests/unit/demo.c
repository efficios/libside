#include <side/trace.h>

side_static_event(my_provider_event, "myprovider", "myevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(side_field_s32("myfield", side_attr_list())),
	side_attr_list()
);

int main()
{
	side_event(my_provider_event, side_arg_list(side_arg_s32(42)));
	return 0;
}
