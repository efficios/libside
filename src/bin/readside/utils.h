// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_UTILS_H
#define	READSIDE_UTILS_H

/**
 * Join paths A with B.
 *
 * The returned value must be freed with xfree().
 *
 * Example:
 *
 * joins_path("foo/", "/bar") => "foo/bar"
 */
extern char *join_paths(const char *a, const char *b);

/**
 * Substitute basename of path A with B.
 *
 * The returned value must be freed with xfree().
 *
 * Example:
 *
 * path_substitute_basename("foo/bar.c", "bar.o") => "foo/bar.o"
 */
extern char *path_substitute_basename(const char *a, const char *b);

/**
 * Return true if the basename of PATH is a dot file.
 *
 * NOTE: . and .. are special cases and are not dot files.
 *
 * Example:
 *
 * path_is_dot_file(".bar") => true
 * path_is_dot_file("foo/.bar") => true
 * path_is_dot_file("bar") => false
 * path_is_dot_file("foo/bar") => false
 * path_is_dot_file(".") => false
 * path_is_dot_file("./") => false
 * path_is_dot_file("./foo/.") => false
 * path_is_dot_file("../foo/..") => false
 * path_is_dot_file("../.") => false
 */
extern bool path_is_dot_file(const char *path);

/**
 * Split STRING into a list of substrings delimited by the character C.
 *
 * The returned list must be freed with drop_string_list().
 */
extern char **split_string(const char *string, char c);

/**
 * Reclaim the memory of a list of substrings returned by split_string().
 */
extern void drop_string_list(char **list);

/**
 * Like basename(3), but does not mutate PATH.  The returned value must be freed
 * with xfree().
 */
extern const char *path_basename(const char *path);

#endif	/* READSIDE_UTILS_H */
