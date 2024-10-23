side_static_event(event, "provider", "event", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_u32("a"),
		side_field_u32("b"),
	)
);

int main(void)
{
	side_event(event, side_arg_list(side_arg_u32(1), side_arg_u64(2)));
	return 0;
}
