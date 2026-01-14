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
#include "pfs_libgen.h"
#include "files.h"

#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
// For now, this will be a hard limit in case it is not defined.
#define PATH_MAX 4096
#endif

#define STRINGIFY(x) STRINGIFY_X(x)
#define STRINGIFY_X(x) #x
#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif // BUILD_DATE
#ifndef FUSE_LIB_VERSION
#define FUSE_LIB_VERSION STRINGIFY(FUSE_USE_VERSION)
#endif // FUSE_LIB_VERSION
#define PLAYLISTFS_METADATA " (built " BUILD_DATE " with libfuse " FUSE_LIB_VERSION ")"

// Defined in operations.c.
extern struct fuse_operations pfs_operations;

#define printwarn(x) {if(!data->opts.quiet) fputs("warning: " x "\n", stderr);}
#define printwarnf(x, ...) {if(!data->opts.quiet) fprintf(stderr, "warning: " x "\n", __VA_ARGS__);}
#define printerr(x) fputs("error: " x "\n", stderr)
#define printerrf(x, ...) fprintf(stderr, "error: " x "\n", __VA_ARGS__)
#define printinfo(x) {if(data->opts.verbose) fputs(x "\n", stderr);}
#define printinfof(x, ...) {if(data->opts.verbose) fprintf(stderr, x "\n", __VA_ARGS__);}

static gboolean pfs_parse_options (
	pfs_data* data, int argc, char* argv[]
);
static gboolean pfs_check_mount_point (
	pfs_data* data
);
static gboolean pfs_build_playlist (
	pfs_data* data
);
static gboolean pfs_setup_fuse_arguments (
	int* fuse_argc, char** fuse_argv[], char* pfs_name, pfs_data* data
);

int main (int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	pfs_data* data = g_malloc0 (sizeof (*data));

	if (!pfs_parse_options (data, argc, argv)) {
		exit (EXIT_FAILURE);
	}
	if (!pfs_check_mount_point (data)) {
		exit (EXIT_FAILURE);
	}
	// When stdout/stderr is not a terminal, ensure output is actually outputted in time.
	fflush(stdout);
	fflush(stderr);

	clock_gettime(CLOCK_REALTIME, &data->opts.started_at);
	data->filetable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, pfs_file_free_void);
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

void pfs_free_pfs_data (pfs_data* data) {
	if (data->opts.files != NULL)
		g_array_free (data->opts.files, TRUE);
	if (data->opts.lists != NULL)
		g_free (data->opts.lists);
	if (data->opts.fuse.fsname != NULL)
		g_free (data->opts.fuse.fsname);
	if (data->opts.mount_point != NULL)
		g_free (data->opts.mount_point);
	if (data->filetable != NULL)
		g_hash_table_unref (data->filetable);
	g_free (data);
}

/*
---- Playlist building ----
*/

static GString* pfs_build_playlist_get_cwd (
	pfs_data* data
);
static gboolean pfs_build_playlist_process_list (
	pfs_data* data, GHashTable* filetable, GString* cwd, char* listpath
);
static gboolean pfs_build_playlist_process_path (
	pfs_data* data, GHashTable* filetable, GString* relative_base, pfs_file_entry* entry
);
static gboolean pfs_build_playlist_add_symlink (
	pfs_data* data, GHashTable* filetable, GString* relative_base, pfs_file_entry* entry
);
static gboolean pfs_build_playlist_add_regular (
	pfs_data* data, GHashTable* filetable, GString* relative_base, pfs_file_entry* entry
);
static char* pfs_build_playlist_get_full_path (
	pfs_data* data, GString* relative_base, char* path
);
static char* pfs_build_playlist_get_full_path_from_absolute (
	pfs_data* data, char* path
);
static char* pfs_build_playlist_get_full_path_from_relative (
	pfs_data* data, GString* relative_base, char* path, size_t length
);

static gboolean pfs_build_playlist (
	pfs_data* data
) {
	char** lists = data->opts.lists;
	GArray* files = data->opts.files;
	GHashTable* table = data->filetable;

	GString* cwd = pfs_build_playlist_get_cwd(data);
	if (!cwd && !data->opts.relative_disabled.all) {
		printwarn("relative paths will be ignored");
	}

	if (lists != NULL) {
		for (size_t ilist = 0; lists[ilist]; ilist++) {
			if (!pfs_build_playlist_process_list (data, table, cwd, lists[ilist])) {
				return FALSE;
			}
		}
	}

	if (files != NULL) {
		GString* files_relative_base = NULL;
		pfs_file_entry* entry = NULL;
		if (!data->opts.relative_disabled.files) {
			files_relative_base = cwd;
		}
		printinfo ("Adding individual files:");
		for (size_t ifile = 0; (entry = &g_array_index (files, pfs_file_entry, ifile)), ifile < files->len; ifile++) {
			if (!pfs_build_playlist_process_path (data, table, files_relative_base, entry)) {
				return FALSE;
			}
		}
	}

	if (g_hash_table_size(table) == 0) {
		printwarn("no lists or files specified, mounting empty filesystem");
	}

	if (cwd) {
		g_string_free (cwd, TRUE);
		cwd = NULL;
	}

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
	if (cwd->str[cwd->len - 1] != '/') {
		g_string_append_c(cwd, '/');
	}
	return cwd;
}

static gboolean pfs_build_playlist_process_list (
	pfs_data* data, GHashTable* filetable, GString* cwd, char* listpath
) {
	FILE* list = fopen (listpath, "rt");
	if (!list) {
		printwarnf ("list '%s' could not be opened, skipping", listpath);
		return TRUE;
	}
	printinfof ("Reading list '%s':", listpath);

	GString* relative_base = NULL;
	
	if (!data->opts.relative_disabled.paths) {
		char* dirpath = NULL;
		dirpath = pfs_dirname(listpath);
		if (dirpath == NULL) {
			printwarnf ("Could not determine directory for list '%s', relative paths will be ignored", listpath);
		}
		else {
			if (dirpath[0] == '/') {
				relative_base = g_string_new(dirpath);
				g_string_append_c(relative_base, '/');
			}
			else if (cwd) {
				relative_base = g_string_new_len(cwd->str, cwd->len);
				g_string_append(relative_base, dirpath);
				g_string_append_c(relative_base, '/');
			}
			else {
				printwarnf ("relative paths will be ignored for list '%s'", listpath);
			}
			g_free (dirpath);
		}
	}

	char path[PATH_MAX];
	while (fgets (path, PATH_MAX, list)) {
		size_t length = strlen (path);
		pfs_file_entry entry = { .path = path, .type = S_IFREG };
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

		pfs_build_playlist_process_path (data, filetable, relative_base, &entry);
	}

	if (feof (list)) {
		fclose (list);
	}
	else {
		printwarnf ("error when reading list '%s'", listpath);
	}

	if (relative_base) {
		g_string_free (relative_base, TRUE);
	}

	return TRUE;
}

static gboolean pfs_build_playlist_process_path (
	pfs_data* data, GHashTable* filetable, GString* relative_base, pfs_file_entry* entry
) {
	mode_t type = entry->type;

	if (S_ISLNK(type)) {
		return pfs_build_playlist_add_symlink(data, filetable, relative_base, entry);
	}
	
	return pfs_build_playlist_add_regular(data, filetable, relative_base, entry);
}

static gboolean pfs_build_playlist_add_symlink (
	pfs_data* data, GHashTable* filetable, GString* relative_base, pfs_file_entry* entry
) {
	char* path = entry->path;
	// name is used as the key, do not free it here!
	char* name = pfs_basename (path);
	if (strlen (name) <= NAME_MAX) {
		pfs_file* file = pfs_file_create (path, S_IFLNK, &data->opts.started_at);
		if (!file) {
			printerr ("could not create new file");
			return FALSE;
		}
		
		printinfof("  %s -> %s", name, path);
		// Replace in case we encountered the name already.
		if (!g_hash_table_replace (filetable, name, file)) {
			printinfof ("    Replaced previous definition of '%s'", name);
		}
	}
	else {
		printwarnf ("filename '%s' is too long, ignoring", name);
		g_free (name);
	}

	return TRUE;
}

static gboolean pfs_build_playlist_add_regular (
	pfs_data* data, GHashTable* filetable, GString* relative_base, pfs_file_entry* entry
) {
	char* path = entry->path;
	char* full_path = pfs_build_playlist_get_full_path(data, relative_base, path);
	if (full_path == NULL) {
		// Something happened, warning was already printed, skip file. 
		return TRUE;
	}

	struct stat filestat;
	if (0 != lstat (full_path, &filestat)) {
		printwarnf ("file '%s' is inaccessible, ignoring", path);
	}
	else if (S_ISDIR (filestat.st_mode)) {
		printwarnf ("file '%s' is a directory, ignoring", path);
	}
	else {
		// name is used as the key, do not free it here!
		char* name = pfs_basename (path);
		if (strlen (name) <= NAME_MAX) {
			// Set type to symlink/regular based on what we need, not what the file is.
			mode_t type = !data->opts.symlinks ? S_IFREG : S_IFLNK;
			pfs_file* file = pfs_file_create (full_path, type, &data->opts.started_at);
			if (!file) {
				printerr ("could not create new file");
				return FALSE;
			}
			
			printinfof("  %s : %s", name, full_path);
			// Replace in case we encountered the name already.
			if (!g_hash_table_replace (filetable, name, file)) {
				printinfof ("    Replaced previous definition of '%s'", name);
			}
		}
		else {
			printwarnf ("filename '%s' is too long, ignoring", name);
			g_free (name);
		}
	}
	g_free (full_path);

	return TRUE;
}

static char* pfs_build_playlist_get_full_path (
	pfs_data* data, GString* relative_base, char* path
) {
	size_t length = strnlen (path, PATH_MAX);
	if (length == 0) {
		printwarn ("empty filename, ignoring");
		return NULL;
	}

	if (path[0] == '/') {
		return pfs_build_playlist_get_full_path_from_absolute(data, path);
	}
	else {
		return pfs_build_playlist_get_full_path_from_relative(data, relative_base, path, length);
	}
}

static char* pfs_build_playlist_get_full_path_from_absolute (
	pfs_data* data, char* path
) {
	// Absolute paths are already complete, no additional processing needed.
	// We could call realpath(), but it can fail for overly long paths.
	// In this case, we trust that the user knows what they are doing.
	return g_strdup (path);
}

static char* pfs_build_playlist_get_full_path_from_relative (
	pfs_data* data, GString* relative_base, char* path, size_t length
) {
	char* full_path = NULL;
	
	// Relative paths need more checks and handling.
	if (relative_base == NULL) {
		printinfof ("Ignoring relative path '%s'", path);
	}
	else if (relative_base->len + length >= PATH_MAX) {
		printwarn ("filename too long, ignoring");
	}
	else {
		full_path = g_malloc (sizeof (*full_path) * (relative_base->len + length + 1));
		strcpy (full_path, relative_base->str);
		strcpy (full_path + relative_base->len, path);
	}
	return full_path;
}

/*
---- Option parsing ----
*/

static gboolean pfs_option_callback_add_file (
	const char* option_name, const char* value, gpointer gdata, GError** error
);
static gboolean pfs_option_callback_add_symlink (
	const char* option_name, const char* value, gpointer gdata, GError** error
);

static gboolean pfs_option_callback_no_relative (
	const char* option_name, const char* value, gpointer gdata, GError** error
);
static gboolean pfs_option_callback_relative (
	const char* option_name, const char* value, gpointer gdata, GError** error
);

// static gboolean pfs_show_fuse_help_callback (
// 	const char* option_name, const char* value, gpointer gdata, GError** error
// ) {
// 	*error = NULL;
// 	char* argv[] = {"", "--help", NULL};
// 	fuse_main(2, argv, &pfs_operations, NULL);
// 	return TRUE;
// }

static GOptionContext* pfs_setup_options (
	pfs_data* data
) {
	GOptionContext* optionContext = g_option_context_new ("[LIST...] [MOUNT_DIR]");

	g_option_context_set_summary (
		optionContext, "PlaylistFS mounts a FUSE filesystem with files taken from user-supplied list(s) or specified on command line."
	);
	g_option_context_set_help_enabled (optionContext, TRUE);

	GOptionGroup* mainGroup = g_option_group_new (
		NULL, NULL, NULL, data, NULL
	);
	GOptionEntry options[] = {
		{ "target", 't', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &data->opts.mount_point, "Set mount point explicitly", "MOUNT_DIR" },
		{ "file", 'f', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_CALLBACK, pfs_option_callback_add_file, "Add a single FILE, overriding any lists", "FILE" },
		{ "symlink", 's', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_CALLBACK, pfs_option_callback_add_symlink, "Add a single symlink to FILE, overriding any lists", "FILE" },
		{ "symlinks", 'S', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.symlinks, "Display all files as symlinks to originals", NULL },
		{ "no-relative", 'N', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, pfs_option_callback_no_relative, "Combine --no-relative-files and --no-relative-paths", NULL },
		{ "relative", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, pfs_option_callback_relative, "Enable all relative path handling", NULL },
		{ "no-relative-files", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.relative_disabled.files, "Disable relative path handling for files added with --file", NULL },
		{ "relative-files", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &data->opts.relative_disabled.files, "Reverse effect of --no-relative-files", NULL },
		{ "no-relative-paths", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.relative_disabled.paths, "Disable relative path handling in LISTs", NULL },
		{ "relative-paths", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &data->opts.relative_disabled.paths, "Reverse effect of --no-relative-paths", NULL },
		{ "verbose", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.verbose, "Describe what is happening", NULL },
		{ "quiet", 'q', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.quiet, "Suppress warnings", NULL },
		{ "version", 'V', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.show_version, "Display version information", NULL },
		{}
	};
	g_option_group_add_entries (mainGroup, options);
	g_option_context_set_main_group (optionContext, mainGroup);

	GOptionGroup* optionsFuseGroup = g_option_group_new (
		"fuse", "Options passed to FUSE:", "Show FUSE options", NULL, NULL
	);
	GOptionEntry optionsFuse[] = {
		{ "read-only", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.fuse.ro, "Mount file system as read-only", NULL },
		{ "noexec", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.fuse.noexec, "Do not allow execution of files", NULL },
		{ "noatime", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.fuse.noatime, "Do not update time of access", NULL },
		{ "fsname", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &data->opts.fuse.fsname, "Set filesystem name", "NAME" },
		{ "nonempty", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.fuse.nonempty, "Allow mounts over non-empty targets (ignored on FUSE 3)", NULL },
		{ "debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &data->opts.fuse.debug, "Enable debugging mode", NULL },
		// { "fuse-help", 0, G_OPTION_FLAG_HIDDEN|G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, pfs_show_fuse_help_callback, NULL, NULL},
		{}
	};
	g_option_group_add_entries (optionsFuseGroup, optionsFuse);
	g_option_context_add_group (optionContext, optionsFuseGroup);

	return optionContext;
}

static gboolean pfs_parse_options (
	pfs_data* data, int argc, char* argv[]
) {
	GOptionContext* optionContext = pfs_setup_options (data);

	GError* optionError = NULL;
	if (!g_option_context_parse (optionContext, &argc, &argv, &optionError)) {
		printerrf ("%s", optionError->message);
		fputs (g_option_context_get_help (optionContext, TRUE, NULL), stderr);
		return FALSE;
	}
	g_option_context_free (optionContext);
	optionContext = NULL;

	if (data->opts.show_version) {
		puts (
			"PlaylistFS " PLAYLISTFS_VERSION PLAYLISTFS_METADATA "\n"
			"Copyright (C) 2018-2026 Alexander Bulancov\n"
			"This is free software; see the source for copying conditions.\n"
			"There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
		);
		if (argc == 1 && data->opts.files == NULL) {
			exit (0);
		}
	}

	if (!data->opts.relative_disabled.files || !data->opts.relative_disabled.paths) {
		data->opts.relative_disabled.all = FALSE;
	}
	else {
		data->opts.relative_disabled.all = TRUE;
	}

	if (data->opts.mount_point == NULL) {
		if (argc == 1) {
			printerr ("no target mount point");
			return FALSE;
		}
		data->opts.mount_point = g_malloc (sizeof (*data->opts.mount_point) * (strlen(argv[--argc]) + 1));
		strcpy (data->opts.mount_point, argv[argc]);
	}

	data->opts.lists = g_new (char*, argc);
	for (int i = 1; i < argc; i++) {
		data->opts.lists[i - 1] = argv[i];
	}
	data->opts.lists[argc - 1] = NULL;

	return TRUE;
}

// Individual file entry callbacks

void pfs_option_clear_file_entry (void* pointer) {
	pfs_file_entry* entry = (pfs_file_entry*) pointer;
	g_clear_pointer (&entry->path, g_free);
	entry->type = 0;
}

static gboolean pfs_option_callback_add_entry (
	const char* option_name, const char* value, gpointer gdata, GError** error, mode_t type
) {
	*error = NULL;

	pfs_data* data = (pfs_data*) gdata;
	if (data->opts.files == NULL) {
		data->opts.files = g_array_new (TRUE, FALSE, sizeof (pfs_file_entry));
		g_array_set_clear_func (data->opts.files, pfs_option_clear_file_entry);
	}

	pfs_file_entry entry = { .path = g_strdup (value), .type = type };
	g_array_append_vals (data->opts.files, &entry, 1);
	return TRUE;
}

static gboolean pfs_option_callback_add_file (
	const char* option_name, const char* value, gpointer gdata, GError** error
) {
	return pfs_option_callback_add_entry(option_name, value, gdata, error, S_IFREG);
}
static gboolean pfs_option_callback_add_symlink (
	const char* option_name, const char* value, gpointer gdata, GError** error
) {
	return pfs_option_callback_add_entry(option_name, value, gdata, error, S_IFLNK);
}

// --relative and --no-relative callbacks

static gboolean pfs_option_callback_no_relative (
	const char* option_name, const char* value, gpointer gdata, GError** error
) {
	*error = NULL;

	pfs_data* data = (pfs_data*) gdata;
	data->opts.relative_disabled.all = TRUE;
	data->opts.relative_disabled.files = TRUE;
	data->opts.relative_disabled.paths = TRUE;

	return TRUE;
}

static gboolean pfs_option_callback_relative (
	const char* option_name, const char* value, gpointer gdata, GError** error
) {
	*error = NULL;

	pfs_data* data = (pfs_data*) gdata;
	data->opts.relative_disabled.all = FALSE;
	data->opts.relative_disabled.files = FALSE;
	data->opts.relative_disabled.paths = FALSE;

	return TRUE;
}

/*
---- Other helpers ----
*/

static gboolean pfs_check_mount_point (
	pfs_data* data
) {
	char* mount_point = data->opts.mount_point;
	struct stat mountstat;
	if (0 == stat (mount_point, &mountstat)) {
		if (S_ISDIR (mountstat.st_mode)) {
			printinfof ("Mounting to '%s'", mount_point);
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

/*
---- Arguments to FUSE ----
*/
static char* pfs_setup_fuse_fsname (
	pfs_data* data
);

static gboolean pfs_setup_fuse_arguments (
	int* argc, char** argv[], char* pfs_name, pfs_data* data
) {
	int fuse_argc = 0;
	char** fuse_argv = g_new (char*, 16);

	fuse_argv[fuse_argc++] = pfs_name;
	fuse_argv[fuse_argc++] = data->opts.mount_point;

	fuse_argv[fuse_argc] = pfs_setup_fuse_fsname(data);
	printinfo ("Passing options to FUSE:");
	printinfof ("  %s (set filesystem name)", fuse_argv[fuse_argc]);
	fuse_argc++;

	fuse_argv[fuse_argc++] = "-odefault_permissions";
	fuse_argv[fuse_argc++] = "-osubtype=playlistfs";

	if (data->opts.fuse.debug) {
		fuse_argv[fuse_argc++] = "-d";
		printinfo ("  -d (enable foreground operation and debug output)");
	}
	if (data->opts.fuse.ro) {
		fuse_argv[fuse_argc++] = "-r";
		printinfo ("  -r (mount system read-only)");
	}
	if (data->opts.fuse.noatime) {
		fuse_argv[fuse_argc++] = "-onoatime";
		printinfo ("  -onoatime (do not update access time)");
	}
	if (data->opts.fuse.noexec) {
		fuse_argv[fuse_argc++] = "-onoexec";
		printinfo ("  -onoexec (do not allow execution)");
	}
	// This is always on with FUSE 3, so no point in passing it.
	#if FUSE_USE_VERSION < 30
	if (data->opts.fuse.nonempty) {
		fuse_argv[fuse_argc++] = "-ononempty";
		printinfo ("  -ononempty (allow mounts over non-empty file/dir)");
	}
	#endif

	// FUSE 3 has these in struct fuse_config, see operations.c.
	#if FUSE_USE_VERSION < 30
	fuse_argv[fuse_argc++] = "-oattr_timeout=0";
	fuse_argv[fuse_argc++] = "-ouse_ino";
	#endif

	*argc = fuse_argc;
	*argv = fuse_argv;
	return TRUE;
}

static char* pfs_setup_fuse_fsname (
	pfs_data* data
) {
	char* name = NULL;
	char* fuse_fsname = NULL;

	if (data->opts.fuse.fsname != NULL) {
		name = data->opts.fuse.fsname;
		fuse_fsname = g_malloc (sizeof(*fuse_fsname) * (10 + strlen (name)));
		sprintf (fuse_fsname, "-ofsname=%s", name);
	}
	else if (data->opts.lists[0]) {
		name = pfs_basename (data->opts.lists[0]);
		fuse_fsname = g_malloc (sizeof(*fuse_fsname) * (10 + strlen (name)));
		sprintf (fuse_fsname, "-ofsname=%s", name);
		g_free (name);
	}
	else {
		fuse_fsname = g_strdup ("-ofsname=playlistfs");
	}
	return fuse_fsname;
}
