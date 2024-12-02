#ifndef READSIDE_READERS_ELF_INTERNAL_H
#define READSIDE_READERS_ELF_INTERNAL_H

struct elf_section {
	intptr_t begin;
	intptr_t end;
	ptrdiff_t bias;
	const char *name;
};

struct elf {
	const char *path;
	const char *string_table;

	struct elf_section *sections;
	u64                 sections_count;

	intptr_t head;
	u64      size;

	void *shdr;
	u64   shnum;
};

extern bool elf_pointer_bias(const struct elf *elf, intptr_t ptr, ptrdiff_t *bias);

extern void for_each_side_event_in_elf(const struct elf *elf,
				void **ptrs, size_t length,
				const struct visitor *visitor);

extern int cmp_elf_section(const void *A, const void *B);

/**
 * Seek in ELF at OFFSET.
 */
__attribute__((returns_nonnull))
static inline void *elf_seek(const struct elf *elf, size_t offset)
{
	if (offset >= elf->size) {
		return NULL;
	}

	return cast(void *, elf->head) + offset;
}

#endif	/* READSIDE_READERS_ELF_INTERNAL_H */
