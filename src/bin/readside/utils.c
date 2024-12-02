// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <stdlib.h>

#include "utils.h"

/*
 * TODO: This assumes Unix `/' file separator.  Add support for other file
 * separators.
 */

char *join_paths(const char *a, const char *b)
{
	bool need_slash;
	char *path;
	size_t a_size, b_size, size;

	a_size = strlen(a);
	b_size = strlen(b);
	need_slash = false;

	if (0 == a_size) {
		return xstrdup(b);
	} else if (0 == b_size) {
		return xstrdup(a);
	}

	if ('/' == a[a_size - 1]) {
		if ('/' == b[0]) {
			b++;
			b_size--;
		}
	} else {
		if ('/' != b[0]) {
			need_slash = true;
		}
	}

	size   = a_size + b_size + 1 + (need_slash ? 1 : 0);
	path   = xcalloc(size, sizeof(char));

	memcpy(path, a, a_size);

	if (need_slash) {
		path[a_size] = '/';
		memcpy(path + a_size + 1, b, b_size);
	} else {
		memcpy(path + a_size, b, b_size);
	}

	path[size - 1] = '\0';

	return path;
}

char *path_substitute_basename(const char *a, const char *b)
{
	char *dir_base, *path;
	size_t size, dir_base_size, b_size;

	dir_base = rindex(a, '/');

	if (!dir_base) {
		return xstrdup(b);
	}

	dir_base_size = dir_base - a + 1;
	b_size = strlen(b);

	size = dir_base_size + b_size + 1;
	path = xcalloc(size, sizeof(char));

	memcpy(path, a, dir_base_size);
	memcpy(path + dir_base_size, b, b_size);
	path[size - 1] = '\0';

	return path;
}

bool path_is_dot_file(const char *path)
{
	const char *dir_base = rindex(path, '/');
	const char *filename;

	if (dir_base) {
		filename = dir_base + 1;
	} else {
		filename = path;
	}

	if (streq(".", filename) || streq("..", filename)) {
		return false;
	}

	return '.' == filename[0];
}

char **split_string(const char *string, char c)
{
	char **list = xcalloc(1, sizeof(char *));
	size_t count = 0;
	size_t len = 1;

	if (!string) {
		list[0] = NULL;
		return list;
	}

	while (1) {

		const char *match = index(string, c);
		size_t match_size;

		if (!match) {
			break;
		}

		assert(match >= string);

		if (count == len){
			len += 1;
			list = xrealloc(list, len * sizeof(char *));
		}

		match_size = match - string;

		list[count++] = xstrndup(string, match_size);
		string = match + 1;
	}

	if ('\0' != *string) {
		if (count == len) {
			len += 1;
			list = xrealloc(list, len * sizeof(char *));
		}
		list[count++] = xstrdup(string);
	}

	if (count == len) {
		len += 1;
		list = xrealloc(list, len * sizeof(char *));
	}

	list[count] = NULL;

	return list;
}

void drop_string_list(char **list)
{
	char **head = list;

	while (*head) {
		xfree(*head);
		head += 1;
	}

	xfree(list);
}

const char *path_basename(const char *path)
{
	const char *base = rindex(path, '/');

	if (NULL != base) {
		return base + 1;
	}

	return path;
}
