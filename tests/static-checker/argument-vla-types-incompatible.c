int main(void)
{
	side_arg_define_vla(my_vla, side_arg_list(side_arg_u32(1), side_arg_u64(2)));

	return 0;
}
