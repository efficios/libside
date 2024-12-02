// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_READERS_ELF_ARCH_H
#define READSIDE_READERS_ELF_ARCH_H

#include <elf.h>

#include "visitors/common.h"

extern void readside_elf32(const char *path, Elf32_Ehdr *elf, size_t size, const struct visitor *);
extern void readside_elf64(const char *path, Elf64_Ehdr *elf, size_t size, const struct visitor *);

extern struct elf_dynamic_list *list_elf_dynamic32(const char *path, Elf32_Ehdr *elf, size_t size);
extern struct elf_dynamic_list *list_elf_dynamic64(const char *path, Elf64_Ehdr *elf, size_t size);

#endif /* READSIDE_READERS_ELF_ARCH_H */
