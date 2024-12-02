// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_READERS_ELF_H
#define READSIDE_READERS_ELF_H

#include "libside-tools/visit-description.h"

#include "visitors/common.h"

struct elf_dynamic_list {
	struct elf_dynamic_list *next;
	char *path;
};

extern void readside_elf(const char *path, const struct visitor *visitor);

extern struct elf_dynamic_list *list_elf_dynamic(const char *path);

extern void drop_elf_dynamic_list(struct elf_dynamic_list *);

#endif /* READSIDE_READERS_ELF_H */
