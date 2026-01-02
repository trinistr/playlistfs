/*
 * This file is part of Playlist File System
 * Copyright © 2018-2026 Alexander Bulancov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "playlistfs.h"

// Defined in operations.c.
extern struct fuse_operations pfs_operations;

#define printwarn(x) {if(!data->opts.quiet) fputs("warning: " x "\n", stderr);}
#define printwarnf(x, s) {if(!data->opts.quiet) fprintf(stderr, "warning: " x "\n", s);}
#define printerr(x) fputs("error: " x "\n", stderr)
#define printerrf(x, s) fprintf(stderr, "error: " x "\n", s)
#define printinfo(x) {if(data->opts.verbose) fputs(x "\n", stderr);}
#define printinfof(x, s) {if(data->opts.verbose) fprintf(stderr, x "\n", s);}

static GOptionContext* pfs_setup_options (
	pfs_options* opts
);
static gboolean pfs_parse_options (
	pfs_options* opts, int argc, char* argv[]
);
static gboolean pfs_build_playlist (
	pfs_data* data
);
static GString* pfs_build_playlist_get_cwd (
	pfs_data* data
);
static pfs_file* pfs_build_playlist_create_pfs_file (
	pfs_data* data, char* path, char* buffer, gboolean relative_disabled, GString* relative_base
);
static pfs_file* pfs_build_playlist_create_pfs_file_absolute (
	pfs_data* data, char* path
);
static pfs_file* pfs_build_playlist_create_pfs_file_relative (
	pfs_data* data, char* path, size_t length, char* buffer, gboolean relative_disabled, GString* relative_base
);
static gboolean pfs_setup_fuse_arguments (
	int* fuse_argc, char** fuse_argv[], char* pfs_name, pfs_data* data
);
static gboolean pfs_check_mount_point (
	char* mount_point
);

int main (int argc, char* argv[]) {
	pfs_data* data = calloc (1, sizeof (*data));
	if (!data) {
		printerr ("memory allocation failed");
		exit (EXIT_FAILURE);
	}

	if (!pfs_parse_options (&data->opts, argc, argv)) {
		exit (EXIT_FAILURE);
	}
	// When stdout/stderr is not a terminal, ensure output is actually outputted in time.
	fflush(stdout);
	fflush(stderr);

	data->filetable = g_hash_table_new_full (g_str_hash, g_str_equal, free, pfs_file_free_void);
	if (!pfs_build_playlist (data)) {
		exit (EXIT_FAILURE);
	}
	fflush(stderr);

	int fuse_argc = 0;
	char** fuse_argv = NULL;
	if (!pfs_setup_fuse_arguments (&fuse_argc, &fuse_argv, argv[0], data)) {
		exit (EXIT_FAILURE);
	}
	fflush(stderr);

	return fuse_main (fuse_argc, fuse_argv, &pfs_operations, data);
}

static gboolean pfs_setup_fuse_arguments (
	int* argc, char** argv[], char* pfs_name, pfs_data* data
) {
	int fuse_argc = 0;
	char** fuse_argv = malloc (sizeof (*fuse_argv) * 16);
	if(!fuse_argv) {
		printerr ("memory allocation failed");
		return FALSE;
	}

	fuse_argv[fuse_argc++] = pfs_name;
	fuse_argv[fuse_argc++] = data->opts.mount_point;
	fuse_argv[fuse_argc++] = "-odefault_permissions";
	fuse_argv[fuse_argc++] = "-osubtype=playlistfs";
	if (data->opts.lists[0]) {
		fuse_argv[fuse_argc] = malloc (22 + strlen (data->opts.lists[0]) + 1);
		if(!fuse_argv[fuse_argc]) {
			printerr ("memory allocation failed");
			free (fuse_argv);
			return FALSE;
		}
		sprintf (fuse_argv[fuse_argc++], "-ofsname=playlistfs-'%s'", data->opts.lists[0]);
	}
	else {
		fuse_argv[fuse_argc++] = "-ofsname=playlistfs";
	}
	printinfo ("Passing options to FUSE:");
	if (data->opts.fuse.debug) {
		fuse_argv[fuse_argc++] = "-d";
		printinfo ("\t-d (debug mode)");
	}
	if (data->opts.fuse.ro) {
		fuse_argv[fuse_argc++] = "-r";
		printinfo ("\t-r (read-only mode)");
	}
	if (data->opts.fuse.noatime) {
		fuse_argv[fuse_argc++] = "-onoatime";
		printinfo ("\t-o noatime (do not update access time)");
	}
	if (data->opts.fuse.noexec) {
		fuse_argv[fuse_argc++] = "-onoexec";
		printinfo ("\t-o noexec (do not allow execution)");
	}
	if ( !(data->opts.fuse.debug || data->opts.fuse.ro
			|| data->opts.fuse.noatime || data->opts.fuse.noexec) )
		printinfo ("\tno options to pass");

	*argc = fuse_argc;
	*argv = fuse_argv;
	return TRUE;
}

static gboolean pfs_build_playlist (
	pfs_data* data
) {
	char** lists = data->opts.lists;
	char** files = data->opts.files;
	GHashTable* table = data->filetable;
	
	char buffer[PATH_MAX];
	char path[PATH_MAX];
	struct stat filestat;
	pfs_file* saved_file = NULL;

	GString* cwd = pfs_build_playlist_get_cwd(data);
	if (!cwd && !data->opts.relative_disabled.all) {
		printwarn("relative paths will be ignored");
	}

	if (lists != NULL) {
		for (size_t ilist = 0; lists[ilist]; ilist++) {
			FILE* list = fopen (lists[ilist], "rt");
			if (!list) {
				printwarnf ("list '%s' could not be opened", lists[ilist]);
				continue;
			}
			char* listpath = dirname(lists[ilist]);
			printinfof ("Reading list '%s'", lists[ilist]);

			GString* relative_base = NULL;
			if (listpath[0] == '/') {
				relative_base = g_string_new(listpath);
				g_string_append_c(relative_base, '/');
			}
			else if (cwd) {
				relative_base = g_string_new_len(cwd->str, cwd->len);
				g_string_append(relative_base, listpath);
				g_string_append_c(relative_base, '/');
			}
			else if (!data->opts.relative_disabled.all) {
				printwarnf ("relative paths will be ignored for list '%s'", lists[ilist]);
			}

			while (fgets (path, PATH_MAX, list)) {
				size_t length = strlen (path);
				if (path[0] == '\n' || length == 0) {
					continue;
				}
				else if (path[length - 1] == '\n') {
					path[length - 1] = '\0';
				}
				else if (length == PATH_MAX - 1) {
					printwarn ("filename too long, ignoring");
					while (fgetc(list) != '\n' && !feof(list) && !ferror(list)) {}
					continue;
				}

				saved_file = pfs_build_playlist_create_pfs_file(data, path, buffer, data->opts.relative_disabled.lists, relative_base);
				if (saved_file == NULL) continue;

				if (0 != stat (saved_file->path->str, &filestat)) {
					printwarnf ("file '%s' is inaccessible, ignoring", path);
					pfs_file_free(saved_file);
				}
				else if (S_ISDIR (filestat.st_mode)) {
					printwarnf ("file '%s' is a directory, ignoring", path);
					pfs_file_free(saved_file);
				}
				else {
					char* name = strdup (basename (path));
					if (!name) {
						printerr ("memory allocation failed");
						if (saved_file)
							pfs_file_free (saved_file);
						if (relative_base)
							g_string_free (relative_base, TRUE);
						if (cwd)
							g_string_free (cwd, TRUE);
						return FALSE;
					}
					saved_file->type = filestat.st_mode&S_IFMT;

					// Replace in case we encountered the name already.
					g_hash_table_replace (table, name, saved_file);
				}
			}

			if (feof (list)) {
				fclose (list);
			}
			else {
				printwarnf ("error when reading list '%s'", lists[ilist]);
			}
			if (relative_base)
				g_string_free(relative_base, TRUE);
			printinfof ("Done with list '%s'", lists[ilist]);
		}
	}

	if (files != NULL) {
		printinfo ("Adding individual files:");
		for (size_t ifile = 0; files[ifile]; ifile++) {
			saved_file = pfs_build_playlist_create_pfs_file(data, files[ifile], buffer, data->opts.relative_disabled.files, cwd);
			if (saved_file == NULL) continue;

			if (0 != stat (saved_file->path->str, &filestat)) {
				printwarnf ("file '%s' is inaccessible, ignoring", files[ifile]);
				pfs_file_free(saved_file);
			}
			else if (S_ISDIR (filestat.st_mode)) {
				printwarnf ("file '%s' is a directory, ignoring", files[ifile]);
				pfs_file_free(saved_file);
			}
			else {
				char* name = strdup (basename (files[ifile]));
				if (!name) {
					printerr ("memory allocation failed");
					if (saved_file)
						pfs_file_free (saved_file);
					if (cwd)
						g_string_free (cwd, TRUE);
					return FALSE;
				}
				saved_file->type = filestat.st_mode&S_IFMT;

				g_hash_table_replace (table, name, saved_file);
			}
		}
	}

	if (g_hash_table_size(table) == 0) {
		printwarn("no lists or files specified, mounting empty filesystem");
	}

	if (cwd)
		g_string_free(cwd, TRUE);

	return TRUE;
}

static GString* pfs_build_playlist_get_cwd (
	pfs_data* data
) {
	if (data->opts.relative_disabled.all) return NULL;

	char string_cwd[PATH_MAX];
	if (!getcwd(string_cwd, PATH_MAX)) {
		printerr ("could not get current working directory");
		return NULL;
	}

	GString* cwd = g_string_new(string_cwd);
	if (!cwd) {
		printerr ("memory allocation failed");
		return NULL;
	}
	if (cwd->str[cwd->len - 1] != '/') {
		g_string_append_c(cwd, '/');
	}
	return cwd;
}

static pfs_file* pfs_build_playlist_create_pfs_file (
	pfs_data* data, char* path, char* buffer, gboolean relative_disabled, GString* relative_base
) {
	size_t length = strnlen (path, PATH_MAX);
	if (length == 0) {
		printwarn ("empty filename, ignoring");
		return NULL;
	}
	if (length >= PATH_MAX) {
		printwarn ("filename too long, ignoring");
		return NULL;
	}

	if (path[0] == '/') {
		return pfs_build_playlist_create_pfs_file_absolute(data, path);
	}
	else {
		return pfs_build_playlist_create_pfs_file_relative(data, path, length, buffer, relative_disabled, relative_base);
	}
}

static pfs_file* pfs_build_playlist_create_pfs_file_absolute (
	pfs_data* data, char* path
) {
	pfs_file* saved_file = NULL;

	// Absolute paths are already complete, no additional processing needed.
	saved_file = pfs_file_create (path, 0);
	printinfof ("\t%s", path);
	return saved_file;
}

static pfs_file* pfs_build_playlist_create_pfs_file_relative (
	pfs_data* data, char* path, size_t length, char* buffer, gboolean relative_disabled, GString* relative_base
) {
	pfs_file* saved_file = NULL;
	
	// Relative paths need more checks and handling.
	if (relative_disabled || relative_base == NULL) {
		printinfof ("Ignoring relative path '%s'", path);
	}
	else if (relative_base->len + length >= PATH_MAX) {
		printwarn ("filename too long, ignoring");
	}
	else {
		strcpy (buffer, relative_base->str);
		strcpy (buffer + relative_base->len, path);
		saved_file = pfs_file_create (buffer, 0);
		if (saved_file == NULL) {
			printerr ("memory allocation failed");
			return NULL;
		}
		if (data->opts.verbose) fprintf (stderr, "\t%s -> %s\n", path, buffer);
	}
	return saved_file;
}

static GOptionContext* pfs_setup_options (
	pfs_options* opts
) {
	GOptionContext* optionContext = g_option_context_new ("[LIST...] [MOUNT_DIR]");
	if (optionContext == NULL) {
		printerr ("memory allocation failed");
		return NULL;
	}

	g_option_context_set_summary (
		optionContext, "PlaylistFS mounts a FUSE filesystem with files taken from user-supplied list(s) or specified on command line."
	);
	g_option_context_set_help_enabled (optionContext, TRUE);

	GOptionEntry options[] = {
		{ "target", 't', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &opts->mount_point, "Set mount point explicitly", "MOUNT_DIR"},
		{ "file", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME_ARRAY, &opts->files, "Add a single file, overriding any lists", "FILE"},
		{ "no-relative", 'n', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->relative_disabled.all, "Disable relative path handling", NULL},
		{ "relative", 0, G_OPTION_FLAG_HIDDEN|G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &opts->relative_disabled.all, NULL, NULL},
		{ "no-relative-files", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->relative_disabled.files, "Disable relative path handling for files added with -f", NULL},
		{ "relatve-files", 0, G_OPTION_FLAG_HIDDEN|G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &opts->relative_disabled.files, NULL, NULL},
		{ "no-relative-lists", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->relative_disabled.lists, "Disable relative path handling in LISTs", NULL},
		{ "relatve-lists", 0, G_OPTION_FLAG_HIDDEN|G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &opts->relative_disabled.lists, NULL, NULL},
		{ "symlink", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->symlink, "Show symlinks instead of regular files", NULL},
		{ "verbose", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->verbose, "Describe what is happening", NULL},
		{ "quiet", 'q', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->quiet, "Suppress warnings", NULL},
		{ "version", 'V', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->show_version, "Display version information", NULL},
		{}
	};
	g_option_context_add_main_entries (optionContext, options, NULL);

	GOptionGroup* optionsFuseGroup = g_option_group_new (
		"fuse", "Options passed to FUSE:", "FUSE options", NULL, NULL
	);
	GOptionEntry optionsFuse[] = {
		{ "read-only", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.ro, "Mount file system as read-only", NULL},
		{ "noexec", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.noexec, "Do not allow execution of files", NULL},
		{ "noatime", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.noatime, "Do not update time of access", NULL},
		{ "debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.debug, "Enable debugging mode", NULL},
		{}
	};
	g_option_group_add_entries (optionsFuseGroup, optionsFuse);
	g_option_context_add_group (optionContext, optionsFuseGroup);

	return optionContext;
}

static gboolean pfs_parse_options (
	pfs_options* opts, int argc, char* argv[]
) {
	GOptionContext* optionContext = pfs_setup_options (opts);
	if (optionContext == NULL) {
		return FALSE;
	}

	GError* optionError = NULL;
	if (!g_option_context_parse (optionContext, &argc, &argv, &optionError)) {
		printerrf ("%s", optionError->message);
		fputs (g_option_context_get_help (optionContext, TRUE, NULL), stderr);
		return FALSE;
	}
	g_option_context_free (optionContext);

	if (opts->show_version) {
		puts (
			"playlistfs " PLAYLISTFS_VERSION "\n"
			"Copyright (C) 2018-2026 Alexander Bulancov\n"
			"This is free software; see the source for copying conditions.\n"
			"There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
		);
		if (argc == 1 && opts->files == NULL) {
			exit (0);
		}
	}

	if (opts->relative_disabled.all) {
		opts->relative_disabled.files = TRUE;
		opts->relative_disabled.lists = TRUE;
	}

	if (opts->mount_point == NULL) {
		if (argc == 1) {
			printerr ("no target mount point");
			return FALSE;
		}
		opts->mount_point = argv[--argc];
	}

	if (pfs_check_mount_point (opts->mount_point) == FALSE) {
		return FALSE;
	}

	opts->lists = malloc (sizeof (*opts->lists) * argc);
	if (!opts->lists) {
		printerr ("memory allocation failed");
		return FALSE;
	}
	for (int i = 1; i < argc; i++) {
		opts->lists[i - 1] = argv[i];
	}
	opts->lists[argc - 1] = NULL;

	return TRUE;
}

static gboolean pfs_check_mount_point (
	char* mount_point
) {
	struct stat mountstat;
	if (0 == stat (mount_point, &mountstat)) {
		if (S_ISDIR (mountstat.st_mode)) {
			return TRUE;
		}
		else {
			printerr ("mount target is not a directory");
			return FALSE;
		}
	}
	else {
		printerr ("mount target is not accessible");
		return FALSE;
	}
}
