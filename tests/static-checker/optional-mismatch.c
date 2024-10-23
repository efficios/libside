side_static_event(event, "provider", "event", SIDE_LOGLEVEL_ERR,
	side_field_list(
		side_field_optional_literal("foo", side_elem(side_type_u32())),
	)
);

int main()
{
	if (side_event_enabled(event)) {
		side_arg_define_optional(my_optional, side_arg_u64(1),
					SIDE_OPTIONAL_ENABLED);
		side_event_call(event,
			side_arg_list(
				side_arg_optional(my_optional),
			)
		);
	}
}
