// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "argpar.h"
#include "file-type.h"
#include "readers/elf.h"
#include "set.h"
#include "utils.h"
#include "visitors/common.h"

struct format {
	const char *name;
	const struct visitor *visitor;
};

/*
 * Table of formats.  The format is selected by the users with NAME.  Therefore,
 * NAME is API.
 */
static const struct format formats[] = {
	{ "json", &json_visitor, },
	{ "s-expr", &sexpr_visitor, },
	{ "text", &text_visitor, },
};

struct state {
	set_t visited;
	const struct visitor *visitor;
	FILE *output;
	char *ignore_pattern;
	char *match_pattern;
	unsigned long max_depth;
	bool follow_dynamic_links;
	bool follow_recursively;
	bool follow_symbolic_links;
	bool ignore_dot_files;
};

/*
 * Recursive state.
 */
struct rec_state {
	unsigned long depth;
};

static bool is_state_root(const struct rec_state *rstate)
{
	return 0 == rstate->depth;
}

/*
 * Print (FMT % ...) on stderr if RSTATE is at the root level.
 *
 * The rationale is to avoid printing error when doing recursion.  That is, the
 * users is interested by errors for inputs passed on the command line, but not
 * for errors of recursived traversal.
 */
#define error_if_root(rstate, fmt, ...)			\
	if (is_state_root(rstate)) error(fmt, ##__VA_ARGS__)

/* Forward declarations for recursion. */
static void readside(const char *path,
		const struct state *state,
		const struct rec_state *rstate);

static void readside_at(int dirfd, const char *dirpath, const char *name,
		unsigned char d_type,
		const struct state *state,
		const struct rec_state *rstate);

static void readside_dynamic_links(const char *path,
				const struct state *state,
				const struct rec_state *rstate)
{
	struct elf_dynamic_list *list;
	struct rec_state next_rstate = {
		.depth = rstate->depth + 1,
	};

	list = list_elf_dynamic(path);

	for (struct elf_dynamic_list *entry = list; entry; entry=entry->next) {
		readside(entry->path, state, &next_rstate);
	}

	drop_elf_dynamic_list(list);
}

static void readside_symbolic_link(const char *path,
				const struct state *state,
				const struct rec_state *rstate)
{
	char resolved_path[PATH_MAX];
	ssize_t rd;

	rd = readlink(path, resolved_path, sizeof(resolved_path));

	if (rd < 0) {
		error_if_root(rstate,
				"system error while resolving file `%s': %m\n", path);
		return;
	}

	resolved_path[rd] = '\0';

	if ('/' == resolved_path[0]) {
		readside(resolved_path, state, rstate);
	} else {
		char *new_path = path_substitute_basename(path, resolved_path);
		readside(new_path, state, rstate);
		xfree(new_path);
	}
}

static void readside_directory(const char *path,
			const struct state *state,
			const struct rec_state *rstate)
{
	DIR *dir;
	int dfd;

	dir = opendir(path);

	if (!dir) {
		error_if_root(rstate, "could not open directory `%s': %m\n",
				path);
		return;
	}

	dfd = dirfd(dir);

	while (1) {
		struct dirent *dirent;

		errno = 0;
		dirent = readdir(dir);
		if (!dirent) {
			if (errno) {
				error_if_root(rstate,
						"error while reading directory `%s': %m\n",
						path);
			}
			break;
		}

		struct rec_state new_rstate = {
			.depth = rstate->depth + 1,
		};

		/* Always ignore . and .. */
		if (streq(".", dirent->d_name) || streq("..", dirent->d_name)) {
			continue;
		}

		readside_at(dfd, path, dirent->d_name, dirent->d_type,
			    state, &new_rstate);
	}

	closedir(dir);
}

/*
 * Optimized entry point for directory entries.  Uses file descriptor-relative
 * operations and D_TYPE hints to minimize syscalls.
 */
static void readside_at(int dirfd, const char *dirpath, const char *name,
		unsigned char d_type,
		const struct state *state,
		const struct rec_state *rstate)
{
	int type;
	char *path;

	/*
	 * Early filtering before building full path or doing syscalls.
	 */
	if (state->ignore_dot_files && name[0] == '.') {
		return;
	}

	if (state->match_pattern) {
		if (0 != fnmatch(state->match_pattern, name, 0)) {
			/*
			 * Non-matching files are skipped, but we still need to
			 * recurse into directories.
			 */
			if (d_type != DT_DIR && d_type != DT_UNKNOWN) {
				return;
			}
		}
	}

	if (state->ignore_pattern) {
		if (0 == fnmatch(state->ignore_pattern, name, 0)) {
			return;
		}
	}

	/*
	 * Use file_type_at() with d_type hint to avoid stat(2) syscalls when
	 * possible.  For directories and symlinks, d_type gives us the answer
	 * directly.  For regular files, we still need to open+read to detect
	 * ELF files.
	 */
	type = file_type_at(dirfd, name, d_type);

	if (type < 0) {
		error_if_root(rstate, "system error while determining type of file `%s/%s': %s\n",
				dirpath, name, strerror(-type));
		return;
	}

	/*
	 * Skip non-interesting file types early, before building the full path.
	 */
	switch (cast(enum file_type, type)) {
	case FILE_ELF_EXEC:
	case FILE_ELF_DYN:
	case FILE_LINK:
	case FILE_DIRECTORY:
		break;
	default:
		/* FILE_INVALID, FILE_UNKNOWN, FILE_ELF_REL - skip silently in recursion */
		return;
	}

	/* Build full path only for files we'll actually process. */
	path = join_paths(dirpath, name);

	/* Check if already visited. */
	if (!add_to_set(state->visited, path)) {
		xfree(path);
		return;
	}

	switch (cast(enum file_type, type)) {
	case FILE_LINK:
		if (state->follow_symbolic_links) {
			readside_symbolic_link(path, state, rstate);
		}
		break;
	case FILE_DIRECTORY:
		if ((rstate->depth < state->max_depth) && (state->follow_recursively || (0 == rstate->depth))) {
			readside_directory(path, state, rstate);
		}
		break;
	case FILE_ELF_EXEC:
	case FILE_ELF_DYN:
		readside_elf(path, state->visitor);
		if (state->follow_dynamic_links) {
			readside_dynamic_links(path, state, rstate);
		}
		break;
	default:
		break;
	}

	xfree(path);
}

static void readside(const char *path,
		const struct state *state,
		const struct rec_state *rstate)
{
	int type;

	/* Path already visited? */
	if (!add_to_set(state->visited, path)) {
		return;
	}

	type = file_type(path);

	if (type < 0) {
		error_if_root(rstate, "system error while determing type of file `%s': %s\n",
				path, strerror(-type));
		return;
	}

	switch (cast(enum file_type, type)) {
	case FILE_LINK:		/* fall through */
	case FILE_ELF_DYN:	/* fall through */
	case FILE_ELF_EXEC:
		if (state->match_pattern) {
			if (0 != fnmatch(state->match_pattern, path_basename(path), 0)) {
				return;
			}
		}

		if (state->ignore_pattern) {
			if (0 == fnmatch(state->ignore_pattern, path_basename(path), 0)) {
				return;
			}
		}
		break;
	default:
		break;
	}

	if (state->ignore_dot_files) {
		if (path_is_dot_file(path)) {
			return;
		}
	}

	switch (cast(enum file_type, type)) {
	case FILE_INVALID:
		error_if_root(rstate, "file `%s' is of invalid type\n", path);
		break;
	case FILE_UNKNOWN:
		error_if_root(rstate, "unknown file type `%s'\n", path);
		break;
	case FILE_LINK:
		if (state->follow_symbolic_links) {
			readside_symbolic_link(path, state, rstate);
		}
		break;
	case FILE_DIRECTORY:
		/*
		 * Follow directory if current depth is lower than max depth AND
		 * if recursive following is enabled or current depth is 0.
		 */
		if ((rstate->depth < state->max_depth) && (state->follow_recursively || (0 == rstate->depth))) {
			readside_directory(path, state, rstate);
		}
		break;
	case FILE_ELF_EXEC:	/* fall through */
	case FILE_ELF_DYN:
		readside_elf(path, state->visitor);
		if (state->follow_dynamic_links) {
			readside_dynamic_links(path, state, rstate);
		}
		break;
	case FILE_ELF_REL:
		error_if_root(rstate,
				"relocatable ELF files are not supported\n");
		break;
	}
}

static void new_input(const char *input, struct state *state)
{
	struct rec_state rstate = {
		.depth = 0,
	};

	FILE *old_stdout = stdout;

	state->visited = make_set(0);

	if (state->output) {
		stdout = state->output;
	}

	readside(input, state, &rstate);

	if (state->output) {
		stdout = old_stdout;
	}

	drop_set(state->visited);
	state->visited = NULL;
}

static void usage(int exit_code)
{
	static const char usage_text[] =
		"Usage: readside [OPTIONS...] FILES ...\n"
		"Read and display SIDE events in FILES\n"
		"\n"
		"OPTIONS:\n"
		"  -a, --all                do not ignore entries starting with `.'\n"
		"  -d, --dynamic-link       follow dynaminc linking on ELF files\n"
		"  -f, --format=FORMAT      emit results in FORMAT\n"
		"  -h, --help               display this message\n"
		"  -i, --ignore=PATTERN     ignore files matching the glob PATTERN\n"
		"      --list-formats       list available formats and exit\n"
		"  -l, --link               follow symbolic links\n"
		"  -m, --match=PATTERN      only read files matching the glob PATTERN\n"
		"      --max-depth=DEPTH    limit recursive following to DEPTH\n"
		"  -o, --output=PATH        set output to PATH\n"
		"  -r, --recursive          follow subdirectories recursively\n"
		"      --version            display version and exit\n"
		"  -v, --verbose            increase level of verbosity\n"
		;

	FILE *out = EXIT_SUCCESS == exit_code ? stdout : stderr;

	fprintf(out, usage_text);

	exit(exit_code);
}

static void do_list_formats(FILE *out)
{
	fprintf(out, "The available formats are:\n\n");
	for (size_t k = 0;  k < array_size(formats); ++k) {
		fprintf(out,"  - %s\n", formats[k].name);
	}

}

static void list_formats(void)
{
	do_list_formats(stdout);
	exit(EXIT_SUCCESS);
}

static void help(void)
{
	usage(EXIT_SUCCESS);
}

static void show_version(void)
{
	u8 major = 0;
	u8 minor = 1;
	u8 patch = 0;

	printf("readside %u.%u.%u\n", major, minor, patch);
	exit(EXIT_SUCCESS);
}

static void set_ignore_pattern(struct state *state, const char *pattern)
{
	if (state->ignore_pattern) {
		xfree(state->ignore_pattern);
	}
	state->ignore_pattern = xstrdup(pattern);
}

static void set_match_pattern(struct state *state, const char *pattern)
{
	if (state->match_pattern) {
		xfree(state->match_pattern);
	}
	state->match_pattern = xstrdup(pattern);
}

static void set_output(struct state *state, const char *output)
{
	if (state->output) {
		fclose(state->output);
	}
	state->output = fopen(output, "a");
}

static void set_max_depth(struct state *state, const char *max_depth)
{
	char *endptr;

	errno = 0;

	state->max_depth = strtoul(max_depth, &endptr, 0);

	if (0 != errno || *endptr != '\0') {
		die("Could not do convertion to unsigned long for option max-depth: %s", max_depth);
	}
}

static void select_format(const struct visitor **visitor, const char *arg)
{
	for (size_t k = 0; k < array_size(formats); ++k) {
		if (streq(arg, formats[k].name)) {
			*visitor = formats[k].visitor;
			return;
		}
	}

	error("Invalid format `%s'.\n", arg);
	do_list_formats(stderr);
	exit(EXIT_FAILURE);
}

static void cleanup(struct state *state)
{
	if (state->output) {
		fclose(state->output);
	}

	if (state->ignore_pattern) {
		xfree(state->ignore_pattern);
	}

	if (state->match_pattern) {
		xfree(state->match_pattern);
	}
}

enum opt_id {
	OPT_ALL,
	OPT_DYNAMIC_LINK,
	OPT_FORMAT,
	OPT_HELP,
	OPT_IGNORE,
	OPT_LIST_FORMATS,
	OPT_LINK,
	OPT_MATCH,
	OPT_MAX_DEPTH,
	OPT_OUTPUT,
	OPT_RECURSIVE,
	OPT_VERBOSE,
	OPT_VERSION,
};

static void handle_parse_error(const argpar_error_t *error)
{
	bool is_short;
	const argpar_opt_descr_t *desc;

	switch (argpar_error_type(error)) {
	case ARGPAR_ERROR_TYPE_UNKNOWN_OPT:
		error("Unknown option `%s'.\n",
			argpar_error_unknown_opt_name(error));
		break;
	case ARGPAR_ERROR_TYPE_MISSING_OPT_ARG:
		desc = argpar_error_opt_descr(error, &is_short);
		if (is_short) {
			error("Missing required argument for option `%c'.\n",
				desc->short_name);
		} else {
			error("Missing required argument for option `%s'.\n",
				desc->long_name);
		}
		break;
	case ARGPAR_ERROR_TYPE_UNEXPECTED_OPT_ARG:
		desc = argpar_error_opt_descr(error, &is_short);
		if (is_short) {
			error("Unexpected argument for option `%c'.\n",
				desc->short_name);
		} else {
			error("Unexpected argument for option `%s'.\n",
				desc->long_name);
		}
		break;
	}

	argpar_error_destroy(error);

	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	static struct argpar_opt_descr options[] = {
		{ OPT_ALL, 'a', "all", false },
		{ OPT_DYNAMIC_LINK, 'd', "dynamic-link", false },
		{ OPT_FORMAT, 'f', "format", true },
		{ OPT_HELP, 'h', "help", false },
		{ OPT_IGNORE, 'i', "ignore", true },
		{ OPT_LIST_FORMATS, '\0', "list-formats", false },
		{ OPT_LINK, 'l', "link", false },
		{ OPT_MATCH, 'm', "match", true },
		{ OPT_MAX_DEPTH, '\0', "max-depth", true },
		{ OPT_OUTPUT, 'o', "output", true },
		{ OPT_RECURSIVE, 'r', "recursive", false },
		{ OPT_VERBOSE, 'v', "verbose", false },
		{ OPT_VERSION, '\0', "version", false },
		ARGPAR_OPT_DESCR_SENTINEL
	};

	/* These are the defaults. */
	struct state state = {
		.visitor = &json_visitor,
		.output = NULL,
		.ignore_pattern = NULL,
		.match_pattern = NULL,
		.max_depth = LONG_MAX,
		.follow_dynamic_links = false,
		.follow_recursively = false,
		.follow_symbolic_links = false,
		.ignore_dot_files = true,
	};

	argpar_iter_t *argpar_iter = NULL;
	const argpar_item_t *argpar_item = NULL;

	/* Skip first argument. */
	if (argc < 1) {
		die("No argument passed to process.");
	}

	argc -=1;
	argv += 1;

	argpar_iter = argpar_iter_create(argc, (void*)argv, options);

	while (1) {

		argpar_iter_next_status_t status;
		const argpar_error_t *error;
		const char *opt_arg;

		ARGPAR_ITEM_DESTROY_AND_RESET(argpar_item);

		status = argpar_iter_next(argpar_iter, &argpar_item, &error);

		switch (status) {
		case ARGPAR_ITER_NEXT_STATUS_OK:
			break;
		case ARGPAR_ITER_NEXT_STATUS_END:
			goto end_parsing;
		case ARGPAR_ITER_NEXT_STATUS_ERROR:
			handle_parse_error(error);
			break;
		case ARGPAR_ITER_NEXT_STATUS_ERROR_MEMORY:
			error("out of memory\n");
			exit(EXIT_FAILURE);
			break;
		default:
			error("invalid parser status: %d\n", status);
			exit(EXIT_FAILURE);
			break;
		}

		switch (argpar_item_type(argpar_item)) {
		case ARGPAR_ITEM_TYPE_OPT:
			opt_arg = argpar_item_opt_arg(argpar_item);
			switch (argpar_item_opt_descr(argpar_item)->id) {
			case OPT_ALL:
				state.ignore_dot_files = false;
				break;
			case OPT_DYNAMIC_LINK:
				state.follow_dynamic_links = true;
				break;
			case OPT_FORMAT:
				select_format(&state.visitor, opt_arg);
				break;
			case OPT_HELP:
				help();
				break;
			case OPT_IGNORE:
				set_ignore_pattern(&state, opt_arg);
				break;
			case OPT_LIST_FORMATS:
				list_formats();
				break;
			case OPT_LINK:
				state.follow_symbolic_links = true;
				break;
			case OPT_MATCH:
				set_match_pattern(&state, opt_arg);
				break;
			case OPT_MAX_DEPTH:
				set_max_depth(&state, opt_arg);
				break;
			case OPT_OUTPUT:
				set_output(&state, opt_arg);
				break;
			case OPT_RECURSIVE:
				state.follow_recursively = true;
				break;
			case OPT_VERBOSE:
				current_loglevel++;
				break;
			case OPT_VERSION:
				show_version();
				break;
			}
			break;
		case ARGPAR_ITEM_TYPE_NON_OPT:
			new_input(argpar_item_non_opt_arg(argpar_item), &state);
			break;
		}
	}

end_parsing:
	argpar_iter_destroy(argpar_iter);

	cleanup(&state);

	return EXIT_SUCCESS;
}
