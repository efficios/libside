// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <errno.h>
#include <sys/stat.h>

#include "readers/elf.h"
#include "readers/elf/arch.h"
#include "readers/elf/internal.h"
#include "utils.h"

#define ElfN(type) CAT(CAT(CAT(Elf, ARCH_CLASS), _), type)
#define FUNCN(name) CAT(name, ARCH_CLASS)

static ElfN(Shdr) *FUNCN(find_header_section)(struct elf *elf,
					const char *name)
{
	ElfN(Shdr) *shdr = elf->shdr;
	for (u64 k = 0; k < elf->shnum; ++k) {
		if (streq(name, elf->string_table + shdr[k].sh_name)) {
			return &shdr[k];
		}
	}

	return NULL;
}

static ptrdiff_t FUNCN(shdr_bias)(ElfN(Shdr) *shdr)
{
	return cast(intptr_t, shdr->sh_offset) - cast(intptr_t, shdr->sh_addr);
}

static void FUNCN(init_elf_section)(const char *string_table,
				ElfN(Shdr) *shdr, struct elf_section *section)
{
	section->begin = cast(intptr_t, shdr->sh_addr);
	section->end = section->begin + cast(intptr_t, shdr->sh_size);
	section->bias = FUNCN(shdr_bias)(shdr);
	section->name = string_table + shdr->sh_name;
}

static bool FUNCN(open_elf)(struct elf *elf,
			const char *path,
			ElfN(Ehdr) *ehdr, size_t size)
{
	ElfN(Shdr) *shdr;
	u64 shnum;
	u64 shstrndx;

	elf->head = cast(intptr_t, ehdr);
	elf->size = size;
	elf->sections = NULL;
	elf->sections_count = 0;

	shdr      = elf_seek(elf, ehdr->e_shoff);
	shnum     = ehdr->e_shnum;
	shstrndx  = ehdr->e_shstrndx;

	/*
	 * This member holds the section header table's file offset in bytes.
	 * If the file has no section header table, this member holds zero.
	 */
	if (0 == ehdr->e_shoff) {
		warning("No section header table found in ELF file %s", path);
		return false;
	}

	/*
	 * This member holds the number of entries in the section header  table.
	 * Thus  the product of e_shentsize and e_shnum gives the section header
	 * table's size in bytes.  If  a  file  has  no  section  header  table,
	 * e_shnum holds the value of zero.
	 *
	 * If the number of entries in the section header table is larger than
	 * or equal to SHN_LORESERVE (0xff00), e_shnum holds the value zero and
	 * the real number of entries in the section header table is held in the
	 * sh_size member of the initial entry in section header table.  Other‐
	 * wise, the sh_size member of the initial entry in the section header
	 * table holds the value zero.
	 */
	if (0 == shnum) {
		shnum = shdr->sh_size;
		if (0 == shnum) {
			warning("In ELF file %s, number of entries in "
				"section header table is greater than SHN_LORESERVE, "
				"but sh_size is zero.",
				path);
			return false;
		}
	}

	if (SHN_UNDEF == shstrndx) {
		warning("Index for section head table of name "
			"string table not define in ELF file %s",
			path);
		return false;
	}

	/*
	 * This member holds the section header table index of the entry
	 * associated with the section name string table.  If the file has no
	 * section name string table, this member holds the value SHN_UNDEF.
	 *
         * If the index of section name string table section is larger than or
         * equal to SHN_LORESERVE (0xff00), this member holds SHN_XINDEX
         * (0xffff) and the real index of the section name string table section
         * is held in the sh_link member of the initial entry in section header
         * table.  Otherwise, the sh_link member of the initial entry in section
         * header table contains the value zero.
	 */
	if (SHN_XINDEX == shstrndx) {
		shstrndx = shdr->sh_link;
		if (0 == shstrndx) {
			warning("In ELF file %s, string table section index "
				"larger than or equal to SHN_LORESERVE, but "
				"sh_link is zero.",
				path);
			return false;
		}
	}

	elf->string_table = elf_seek(elf, shdr[shstrndx].sh_offset);

	if (!elf->string_table) {
		warning("In ELF file %s, no string table found", elf->path);
		return false;
	}

	elf->shdr = shdr;
	elf->shnum = shnum;
	elf->sections_count = shnum;
	elf->sections = xcalloc(shnum, sizeof(struct elf_section));

	u64 wr = 0;
	for (u64 rd = 0; rd < shnum; ++rd) {
		if (shdr[rd].sh_addr && shdr[rd].sh_size) {
			FUNCN(init_elf_section)(elf->string_table, &shdr[rd], &elf->sections[wr++]);
		}
	}

	elf->sections_count = wr;

	qsort(elf->sections, wr, sizeof(struct elf_section), cmp_elf_section);

	return true;
}

static void FUNCN(close_elf)(struct elf *elf)
{
	xfree(elf->sections);
}

void FUNCN(readside_elf)(const char *path,
			ElfN(Ehdr) *ehdr, size_t size,
			const struct visitor *visitor)
{
	ElfN(Shdr) *side_section_ptr;
	bool valid;
	struct elf elf = {
		.path = path,
	};

	valid = FUNCN(open_elf)(&elf, path, ehdr, size);

	if (!valid) {
		return;
	}

	side_section_ptr = FUNCN(find_header_section)(&elf, "side_event_description_ptr");

	if (side_section_ptr) {
		printf("%s:\n", path);
		for_each_side_event_in_elf(&elf,
					elf_seek(&elf, side_section_ptr->sh_offset),
					side_section_ptr->sh_size / sizeof(struct side_event_description *),
					visitor);
		printf("\n");
	}

	FUNCN(close_elf)(&elf);
}

static struct elf_dynamic_list *add_needed(char **runpath_list,
					const char *path,
					struct elf_dynamic_list *list)
{
	while (*runpath_list) {

		struct stat dummy;
		const char *runpath = *runpath_list;
		char *try = join_paths(runpath, path);

		errno = 0;
		lstat(try, &dummy);

		if (ENOENT != errno) {

			struct elf_dynamic_list *new_needed =
				xcalloc(1, sizeof(*new_needed));

			new_needed->path = try;
			new_needed->next = list;

			return new_needed;
		}

		xfree(try);

		runpath_list += 1;
	}

	return list;
}

static struct elf_dynamic_list *FUNCN(list_needed_in_shdr)(struct elf *elf,
							ElfN(Shdr) *shdr,
							struct elf_dynamic_list *list)
{
	const char *string_table = NULL, *runpath = NULL;
	char **runpath_list;

	ElfN(Dyn) *dynamics = elf_seek(elf, shdr->sh_offset);
	ElfN(Word) runpath_offset = 0;
	bool found_run_path = false;

	if (!dynamics) {
		warning("Could not find section header of type SHT_DYNAMIC");
		return list;
	}

	for (size_t k = 0; k < shdr->sh_size / sizeof(ElfN(Dyn)); ++k) {
		ElfN(Dyn) *dyn = &dynamics[k];

		if (DT_STRTAB == dyn->d_tag) {

			intptr_t ptr;
			ptrdiff_t bias;
			bool valid;

			ptr = cast(intptr_t, dyn->d_un.d_ptr);
			valid = elf_pointer_bias(elf, ptr, &bias);

			if (!valid) {
				error("Ill-formed ELF file %s: "
					"could not resolve dynamic string table.",
					elf->path);
				return list;
			}

			string_table = elf_seek(elf, ptr + bias);
			break;
		} else if (DT_RUNPATH == dyn->d_tag) {
			found_run_path = true;
			runpath_offset = dyn->d_un.d_val;
		}
	}

	/* TODO: Check for legacy DT_RPATH */
	if (!found_run_path) {
		return list;
	}

	assert(string_table);

	runpath = string_table + runpath_offset;

	runpath_list = split_string(runpath, ':');

	for (size_t k = 0; k < shdr->sh_size / sizeof(ElfN(Dyn)); ++k) {

		ElfN(Dyn) *dyn = &dynamics[k];
		const char *path;

		switch (dyn->d_tag) {
		case DT_NEEDED:
			path = string_table + dyn->d_un.d_val;
			list = add_needed(runpath_list, path, list);
			break;
		case DT_NULL:
			goto out;
		default:
			break;
		}
	}

	warning("Ill-formed ELF file %s.  DT_NULL not present in DYNAMIC section.",
		elf->path);
out:
	drop_string_list(runpath_list);

	return list;
}

static struct elf_dynamic_list *FUNCN(list_needed)(struct elf *elf)
{
	struct elf_dynamic_list *list = NULL;
	ElfN(Shdr) *shdr = elf->shdr;

	for (u64 k = 0; k < elf->shnum; ++k) {
		switch (shdr[k].sh_type) {
		case SHT_DYNAMIC:
			list = FUNCN(list_needed_in_shdr)(elf, &shdr[k], list);
			break;
		default:
			break;
		}
	}

	return list;
}

struct elf_dynamic_list *FUNCN(list_elf_dynamic)(const char *path,
						ElfN(Ehdr) *ehdr, size_t size)
{
	struct elf_dynamic_list *list;
	bool valid;
	struct elf elf = {
		.path = path,
	};

	list = NULL;
	valid = FUNCN(open_elf)(&elf, path, ehdr, size);

	if (!valid) {
		goto invalid_elf;
	}

	list = FUNCN(list_needed)(&elf);

	FUNCN(close_elf)(&elf);

invalid_elf:
	return list;
}
