// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_FILE_TYPE
#define READSIDE_FILE_TYPE

/**
 * FILE_INVALID: The file type is invalid (e.g. corrupted ELF).
 *
 * FILE_UNKNOWN: The file type is not known.
 *
 * FILE_LINK: The file type is a symbolic link.
 *
 * FILE_DIRECTORY: The file type is a directory.
 *
 * FILE_ELF_REL: The file type is a relocable ELF.
 *
 * FILE_ELF_EXEC: The file type is an executable ELF.
 *
 * FILE_ELF_DYN: The file type is a shared object ELF.
 */
enum file_type {
	FILE_INVALID,
	FILE_UNKNOWN,
	FILE_LINK,
	FILE_DIRECTORY,
	FILE_ELF_REL,
	FILE_ELF_EXEC,
	FILE_ELF_DYN,
};

/**
 * Return the file type of PATH.
 *
 * If the retuned value is positive or zero, it can safely be cast to enum
 * file_type.  Otherwise, the value is a negated errno of the system.
 */
extern int file_type(const char *path);

/**
 * Return the file type of NAME relative to directory DIRFD.
 *
 * D_TYPE is the d_type field from struct dirent.  If D_TYPE is DT_UNKNOWN,
 * fstatat() will be used to determine the file type.  Otherwise, the d_type
 * hint is used to avoid syscalls when possible.
 *
 * If the returned value is positive or zero, it can safely be cast to enum
 * file_type.  Otherwise, the value is a negated errno of the system.
 */
extern int file_type_at(int dirfd, const char *name, unsigned char d_type);

#endif	/* READSIDE_FILE_TYPE */
