// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

/*
 * The second translation unit: it defines the types, with external
 * linkage, and never refers to them. extern-type.c does the referring.
 */

#include "extern-type.h"

side_define_struct(shared_struct,
	side_field_list(
		side_field_u32("there_x"),
		side_field_s64("there_y"),
	)
);

side_define_enum(shared_enum,
	side_enum_mapping_list(
		side_enum_mapping_range("there-one-ten", 1, 10),
	)
);
