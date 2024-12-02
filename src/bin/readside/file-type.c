// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <elf.h>

#include "file-type.h"

/*
 * Convert st_mode to d_type.  Not defined by POSIX.
 *
 * This assumes DT_* values equal `S_IF* >> 12', which holds on Linux/glibc.
 * May need a portable implementation on other platforms.
 */
#ifndef IFTODT
#  define IFTODT(mode)    (((mode) & S_IFMT) >> 12)
#endif

/*
 * This defines the amount of bytes for scanning to determine the file type.
 */
#define HEAD_SIZE 4096u

static enum file_type match_elf(const char *bytes, size_t len)
{
	/* Elf{32,64}_Ehdr are the same at the begining. */
	u16 *e_type = cast(u16 *, bytes + offsetof(Elf64_Ehdr, e_type));

	/* Ill-formed ELF.  */
	if (len < sizeof(Elf64_Ehdr)) {
		return FILE_INVALID;
	}

	switch (*e_type) {
	case ET_REL:
		return FILE_ELF_REL;
	case ET_EXEC:
		return FILE_ELF_EXEC;
	case ET_DYN:
		return FILE_ELF_DYN;
	default:
		return FILE_UNKNOWN;
	}
}

static enum file_type match_type_bytes(const char *bytes, size_t len)
{
	if (0 == len){
		return FILE_UNKNOWN;
	}

#define MATCH(_type, _pattern...)				\
	do {							\
		u8 pattern[] = { _pattern };			\
		size_t max_n;					\
								\
		max_n = sizeof(pattern);			\
								\
		if (max_n > len) {				\
			max_n = len;				\
		}						\
		if (0 == memcmp(pattern, bytes, max_n)) {	\
			return _type;				\
		}						\
	} while (0)

	MATCH(match_elf(bytes, len), 0x7f, 'E', 'L', 'F');

#undef MATCH

	return FILE_UNKNOWN;
}

/*
 * For now, no filename match is supported.
 */
static enum file_type match_type_filename(const char *path)
{
	(void) path;

	return FILE_UNKNOWN;
}

/*
 * Pre-condition: File at (DIRFD, NAME) is a regular file.
 */
static int file_type_regular_at(int dirfd, const char *name)
{
	int fd;
	char head[HEAD_SIZE];
	ssize_t rd;
	int type = FILE_UNKNOWN;

	fd = openat(dirfd, name, O_RDONLY);

	if (fd < 0) {
		return -errno;
	}

	rd = read(fd, &head, sizeof(head));

	if (rd < 0) {
		type = -errno;
		goto out;
	}

	/* Try to match type with bytes. */
	type = match_type_bytes(head, rd);

	/* As a fallback, try to assume file type from filename. */
	if (FILE_UNKNOWN == type) {
		type = match_type_filename(name);
	}
out:
	close(fd);

	return type;
}

int file_type(const char *path)
{
	struct stat statbuf;

	if (lstat(path, &statbuf) < 0) {
		return -errno;
	}

	return file_type_at(AT_FDCWD, path, IFTODT(statbuf.st_mode));
}

int file_type_at(int dirfd, const char *name, unsigned char d_type)
{
	struct stat statbuf;

	/*
	 * Use d_type hint from readdir(3) when available to avoid fstatat(2).
	 * Only DT_REG needs further inspection.
	 */
	switch (d_type) {
	case DT_DIR:
		return FILE_DIRECTORY;
	case DT_LNK:
		return FILE_LINK;
	case DT_REG:
		return file_type_regular_at(dirfd, name);
	case DT_UNKNOWN:
		/* Fall through to fstatat(2) */
		break;
	default:
		return FILE_INVALID;
	}

	if (fstatat(dirfd, name, &statbuf, AT_SYMLINK_NOFOLLOW) < 0) {
		return -errno;
	}

	if (S_ISREG(statbuf.st_mode)) {
		return file_type_regular_at(dirfd, name);
	} else if (S_ISDIR(statbuf.st_mode)) {
		return FILE_DIRECTORY;
	} else if (S_ISLNK(statbuf.st_mode)) {
		return FILE_LINK;
	} else {
		return FILE_INVALID;
	}
}
