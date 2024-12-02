// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <elf.h>

#include "libside-tools/visit-description.h"

#include "readers/elf.h"
#include "readers/elf/arch.h"
#include "readers/elf/internal.h"
#include "visitors/common.h"

/**
 * Given a pointer PTR, find its bias in ELF.  Applying the bias to the pointer
 * gives the offset in ELF following this formula:
 *
 * ptr + bias = offset
 */
bool elf_pointer_bias(const struct elf *elf, intptr_t ptr, ptrdiff_t *bias)
{
	size_t lo, hi;

	lo = 0;
	hi = elf->sections_count;

	while (lo < hi) {

		size_t i;
		const struct elf_section *section;

		i = (lo + hi) / 2;
		section = &elf->sections[i];

		if (ptr < section->begin) {
			hi = i;
		} else if (ptr >= section->end) {
			lo = i + 1;
		} else {
			*bias = section->bias;
			return true;
		}
	}

	return false;
}

static void *resolve_elf_pointer(void *ptr, void *raw_context)
{
	struct visitor_context *context;
	ptrdiff_t bias;
	bool valid;

	if (NULL == ptr) {
		return NULL;
	}

	context = raw_context;

	valid = elf_pointer_bias(context->resolve_priv, cast(intptr_t, ptr), &bias);

	if (!valid) {
		warning("Could not find bias of pointer %p in ELF file %s.",
			ptr,
			cast(struct elf *, context->resolve_priv)->path);
		return NULL;
	}

	return elf_seek(context->resolve_priv, cast(intptr_t, ptr) + bias);
}

void for_each_side_event_in_elf(const struct elf *elf,
				void **ptrs, size_t length,
				const struct visitor *asked_visitor)
{
	struct visitor visitor;

	if (!ptrs) {
		warning("In ELF file %s, could not find side_event_description_ptr section.",
			elf->path);
		return;
	}

	copy_visitor_with_resolver(asked_visitor, resolve_elf_pointer,
				&visitor);

	struct visitor_context context = {
		.resolve      = resolve_elf_pointer,
		.resolve_priv = cast(void *, elf),
		.nesting      = 0,
		.context      = visitor.make_context ? visitor.make_context() : NULL,
	};


	if (visitor.begin) {
		visitor.begin(&context);
	}

	for (size_t k = 0; k < length; ++k) {
		const struct side_event_description *desc;
		struct side_description_visitor desc_visitor = {
			.callbacks = &visitor.description,
			.priv = &context,
		};

		desc = resolve_elf_pointer(ptrs[k], &context);

		visit_event_description(&desc_visitor, desc);
	}

	if (visitor.end) {
		visitor.end(&context);
	}

	if (visitor.drop_context) {
		visitor.drop_context(context.context);
	}
}

static bool file_size(int fd, size_t *size)
{
	struct stat statbuf;
	int err;

	err = fstat(fd, &statbuf);

	if (0 != err) {
		return false;
	}

	*size = cast(size_t, statbuf.st_size);

	return true;
}

static bool mmap_file(const char *path, void **pmem, size_t *pmem_size)
{
	int fd;
	void *mem;
	size_t mem_size;
	bool success = false;

	fd = open(path, O_RDONLY);

	if (fd < 0) {
		error("Failed to open file %s: %m", path);
		goto bad_fd;
	}

	if (!file_size(fd, &mem_size)) {
		error("Failed to determined size of file %s: %m", path);
		goto out;
	}

	mem = mmap(NULL, mem_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if (MAP_FAILED == mem) {
		error("Failed to map file %s (fd = %d, size = %zu): %m", path, fd, mem_size);
		goto out;
	} else {
		*pmem = mem;
		*pmem_size = mem_size;
		success = true;
	}

out:
	close(fd);
bad_fd:
	return success;
}

static bool ensure_elf(const char *path, Elf64_Ehdr *elf_header)
{
	if (ELFMAG0 != elf_header->e_ident[EI_MAG0]) {
		warning("In ELF file %s, mismatch of magic number at byte %d, expected %x but got %x",
			path, EI_MAG0, ELFMAG0, elf_header->e_ident[EI_MAG0]);
		return false;
	}

	if (ELFMAG1 != elf_header->e_ident[EI_MAG1]) {
		warning("In ELF file %s, mismatch of magic number at byte %d, expected %x but got %x",
			path, EI_MAG1, ELFMAG1, elf_header->e_ident[EI_MAG1]);
		return false;
	}

	if (ELFMAG2 != elf_header->e_ident[EI_MAG2]) {
		warning("In ELF file %s, mismatch of magic number at byte %d, expected %x but got %x",
			path, EI_MAG2, ELFMAG2, elf_header->e_ident[EI_MAG2]);
		return false;
	}

	if (ELFMAG3 != elf_header->e_ident[EI_MAG3]) {
		warning("In ELF file %s, mismatch of magic number at byte %d, expected %x but got %x",
			path, EI_MAG3, ELFMAG3, elf_header->e_ident[EI_MAG3]);
		return false;
	}

	return true;
}

void readside_elf(const char *path,
		const struct visitor *visitor)
{
	void *mem;
	size_t mem_size;
	Elf64_Ehdr *elf_header;

	if (!mmap_file(path, &mem, &mem_size)) {
		return;
	}

	elf_header = mem;

	if (!ensure_elf(path, elf_header)) {
		goto out;
	}

	switch (elf_header->e_ident[EI_CLASS]) {
	default:
		warning("Invalid architecture class %d in ELF file %s",
			elf_header->e_ident[EI_CLASS], path);
		break;
	case ELFCLASS32:
		readside_elf32(path, mem, mem_size, visitor);
		break;
	case ELFCLASS64:
		readside_elf64(path, mem, mem_size, visitor);
		break;
	}

out:
	munmap(mem, mem_size);
}

struct elf_dynamic_list *list_elf_dynamic(const char *path)
{
	void *mem;
	size_t mem_size;
	Elf64_Ehdr *elf_header;
	struct elf_dynamic_list *list = NULL;

	mmap_file(path, &mem, &mem_size);

	elf_header = mem;

	if (!ensure_elf(path, elf_header)) {
		goto out;
	}

	switch (elf_header->e_ident[EI_CLASS]) {
	default:
		warning("Invalid architecture class %d in ELF file %s",
			elf_header->e_ident[EI_CLASS], path);
		break;
	case ELFCLASS32:
		list = list_elf_dynamic32(path, mem, mem_size);
		break;
	case ELFCLASS64:
		list = list_elf_dynamic64(path, mem, mem_size);
		break;
	}

out:
	munmap(mem, mem_size);

	return list;
}

void drop_elf_dynamic_list(struct elf_dynamic_list *list)
{
	while (list) {
		struct elf_dynamic_list *tmp = list;
		list = tmp->next;
		xfree(tmp->path);
		xfree(tmp);
	}
}

int cmp_elf_section(const void *A, const void *B)
{
	const struct elf_section *a = A;
	const struct elf_section *b = B;

	if (a->begin < b->begin) {
		return -1;
	}

	if (a->begin > b->end){
		return 1;
	}

	return 0;
}
