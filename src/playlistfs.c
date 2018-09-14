/*
 * This file is part of Playlist File System
 * Copyright ©2018 Aleksandr Bulantcov
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

#include <playlistfs.h>

// Allowed operations. Unused ones are commented out,
// deprecated are not listed at all
static struct fuse_operations pfs_operations = {
	.getattr = pfs_getattr,
	.readlink = pfs_readlink,	// For symlink mode
	//.mknod = pfs_mknod,	// No file creation. Regular files should use .create
	//.mkdir = pfs_mkdir,
	//.rmdir = pfs_rmdir,
	.unlink = pfs_unlink,
	//.symlink = pfs_symlink,
	.rename = pfs_rename,
	//.link = pfs_link,
	//.chmod = pfs_chmod,	// Maybe these two should not actually be allowed?
	//.chown = pfs_chown,
	.truncate = pfs_truncate,	// Questionable
	.open = pfs_open,
	.read = pfs_read,
	.write = pfs_write,
	.statfs = pfs_statfs,	// = statvfs
	//.flush = pfs_flush,
	.release = pfs_release,	// Files need to be closed
	.fsync = pfs_fsync,
	//.setxatr = pfs_setxattr,	// These four will not be defined for now
	//.getxattr = pfs_getxattr,
	//.listxattr = pfs_listxattr,
	//.removexattr = pfs_removexattr,
	.opendir = pfs_opendir,	// Need to actually check if this gets called on root
	.readdir = pfs_readdir,
	.releasedir = pfs_releasedir,	// Directories also need to be closed, or do they?
	//.fsyncdir = pfs_fsyncdir,	//Can be called on root, probably
	//.init = pfs_init,	// These two are not necessarily useful
	.destroy = pfs_destroy,
	.access = pfs_access,	// default_permissions negates the need for this
	//.create = pfs_create,	// No file creation
	.ftruncate = pfs_ftruncate,	// Seems that this one can just call truncate
	.fgetattr = pfs_fgetattr,	// The same
	//.lock = pfs_lock,	// Implemented by kernel (not needed for local FS)
	.utimens = pfs_utimens,	// Use utimensat
	//.bmap = pfs_bmap,	// This FS is not backed by a device
	.flag_nullpath_ok = 0,	// Files can not be removed, so we never work with them
	.flag_nopath = 0,	// May be allowed, as file handles are probably enough
	.flag_utime_omit_ok = 1,	// This will be proxied, so it is okay
	//.ioctl = pfs_ioctl,
	//.poll = pfs_poll,
	//.write_buf = pfs_write_buf,	// Unclear that these do
	//.read_buf = pfs_read_buf,
	//.flock = pfs_flock,	//The same as lock()
	//.fallocate = pfs_fallocate
};

#define printwarn(x) {if(!data->opts.quiet)fputs("warning: " x "\n", stderr);}
#define printwarnf(x, s) {if(!data->opts.quiet)fprintf(stderr, "warning: " x "\n", s);}
#define printerr(x) fputs("error: " x "\n", stderr)
#define printerrf(x, s) fprintf(stderr, "error: " x "\n", s)
#define printinfo(x) {if(data->opts.verbose)fputs(x "\n", stderr);}
#define printinfof(x, s) {if(data->opts.verbose)fprintf(stderr, x "\n", s);}

int main (int argc, char* argv[]) {
	pfs_data* data = calloc (sizeof(*data), 1);
	if (!data) {
		printerr ("memory allocation failed");
		exit (EXIT_FAILURE);
	}
	
	if (!pfs_parse_options (&data->opts, argc, argv)) {
		exit (EXIT_FAILURE);
	}
	
	data->filetable = g_hash_table_new_full (g_str_hash, g_str_equal, free, free);
	if (!pfs_build_playlist (data)) {
		exit (EXIT_FAILURE);
	}
	
	char** fuse_argv = malloc (sizeof (*fuse_argv) * 16);
	size_t fuse_argc = 0;
	fuse_argv[fuse_argc++] = argv[0];
	fuse_argv[fuse_argc++] = data->opts.mount_point;
	fuse_argv[fuse_argc++] = "-odefault_permissions";
	fuse_argv[fuse_argc] = malloc (11 + strlen (data->opts.lists[0]) + 1);
	sprintf (fuse_argv[fuse_argc++], "-ofsname='%s'", data->opts.lists[0]);
	//fuse_argv[fuse_argc++] = "-ofsname=playlistfs";
	fuse_argv[fuse_argc++] = "-osubtype=playlistfs";
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
	
	if (!fuse_main (fuse_argc, fuse_argv, &pfs_operations, data)) {
		printerr ("calling FUSE failed");
		exit (EXIT_FAILURE);
	}
	
	return EXIT_SUCCESS;
}

gboolean pfs_build_playlist (pfs_data* data) {
	char** lists = data->opts.lists;
	char** files = data->opts.files;
	GHashTable* table = data->filetable;
	char* path = malloc(sizeof(*path) * PATH_MAX);
	struct stat filestat;

	if (lists != NULL) {
		for (size_t ilist = 0; lists[ilist]; ilist++) {
			FILE* list = fopen (lists[ilist], "rt");
			if (list) {
				printinfof ("Reading list '%s'", lists[ilist]);
				while (fgets (path, PATH_MAX, list)) {
					size_t length = strlen (path);
					if (path[0] == '\n' || length == 0) {
						continue;
					}
					else if (path[length - 1] == '\n') {
						path[length - 1] = '\0';
					}
					else if (length == PATH_MAX-1) {
						printwarn ("filename too long, ignoring");
						while (fgetc(list) != '\n') {}
						continue;
					}
					if (path[0] == '/') {
						if (0 == stat (path, &filestat)) {
							if (!S_ISDIR (filestat.st_mode)) {
								char* name = strdup (basename (path));
								char* saved_path = strdup (path);
								if (!name || !saved_path) {
									printerr ("memory allocation failed");
									return FALSE;
								}
								g_hash_table_insert (table, name, saved_path);
							}
							else {
								printwarnf ("file '%s' is a directory, ignoring", path);
							}
						}
						else {
							printwarnf ("file '%s' is inaccessible, ignoring", path);
						}
					}
					else {
						printwarnf ("path '%s' is not absolute, ignoring", path);
					}
				}
				if (feof (list)) {
					fclose (list);
				}
				else {
					printwarnf ("error when reading list '%s'", lists[ilist]);
				}
			}
			else {
				printwarnf ("list '%s' could not be opened", lists[ilist]);
			}
		}
	}

	if (files != NULL) {
		printinfo ("Adding individual files");
		size_t cwdlength;
		if (getcwd (path, PATH_MAX) && path[0] == '/') {
			cwdlength = strlen (path);
			if (path[cwdlength-1] != '/') {
				path[cwdlength++] = '/';
			}
		}
		else {
			printerr ("could not get current working directory, relative filenames will be ignored");
			cwdlength = 0;
		}
		for (size_t ifile = 0; files[ifile]; ifile++) {
			char* saved_path;
			if (files[ifile][0] != '/') {
				if (cwdlength) {
					size_t length = strlen(files[ifile]);
					if (length > 0) {
						if (cwdlength + length <= PATH_MAX) {
							strcpy (path+cwdlength, files[ifile]);
							saved_path = strdup (path);
							if (!saved_path) {
								printerr ("memory allocation failed");
								return FALSE;
							}
						}
						else {
							printwarn ("filename too long, ignoring");
							continue;
						}
					}
					else {
						printwarn ("empty filename");
						continue;
					}
				}
				else {
					printwarnf ("Ignoring file '%s'", files[ifile]);
					continue;
				}
				
			}
			else {
				saved_path = strdup(files[ifile]);
			}
			if (0 == stat (saved_path, &filestat)) {
				if (!S_ISDIR (filestat.st_mode)) {
					char* name = strdup (basename (path));
					if (!name) {
						printerr ("memory allocation failed");
						return FALSE;
					}
					g_hash_table_insert (table, name, saved_path);
				}
				else {
					printwarnf ("file '%s' is a directory, ignoring", files[ifile]);
				}
			}
			else {
				printwarnf ("file '%s' is inaccessible, ignoring", files[ifile]);
			}
		}
	}

	return TRUE;
}

gboolean pfs_parse_options (pfs_options* opts, int argc, char* argv[]) {
	GError* optionError = NULL;
	GOptionEntry options[] = {
		{ "target", 't', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &opts->mount_point, "Set mount point explicitly", "DIR"},
		{ "file", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME_ARRAY, &opts->files, "Add a single file to playlist", "FILE"},
		{ "symlink", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->symlink, "Create symlinks instead of regular files", NULL},
		{ "verbose", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->verbose, "Describe what is happening", NULL},
		{ "quiet", 'q', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->quiet, "Suppress warnings", NULL},
		{}
	};
	GOptionEntry optionsFuse[] = {
		{ "read-only", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.ro, "Mount file system as read-only", NULL},
		{ "noexec", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.noexec, "Do not allow execution of files", NULL},
		{ "noatime", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.noatime, "Do not update time of access", NULL},
		{ "debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.debug, "Enable debugging mode", NULL},
		{}
	};
	GOptionContext* optionContext = g_option_context_new ("LIST... [DIR]");
	GOptionGroup* optionsFuseGroup = g_option_group_new (
			"fuse", "Options passed to FUSE", "FUSE options", NULL, NULL
			);

	g_option_context_add_main_entries (optionContext, options, NULL);
	g_option_context_add_group (optionContext, optionsFuseGroup);
	g_option_group_add_entries (optionsFuseGroup, optionsFuse);
	g_option_context_set_help_enabled (optionContext, TRUE);

	if (!g_option_context_parse (optionContext, &argc, &argv, &optionError)) {
		printerrf ("%s", optionError->message);
		fputs (g_option_context_get_help (optionContext, TRUE, NULL), stderr);
		return FALSE;
	}
	g_option_context_free (optionContext);

	if (opts->mount_point == NULL) {
		if (argc == 1) {
			printerr ("no target mount point");
			return FALSE;
		}
		opts->mount_point = argv[--argc];
	}
	{
		struct stat mountstat;
		if (0 == stat(opts->mount_point, &mountstat)) {
			if (!S_ISDIR(mountstat.st_mode)) {
				printerr ("target is not a suitable mount point");
				return FALSE;
			}
		}
		else {
			printerr ("target is not accessible");
			return FALSE;
		}
	}
	if (argc > 1) {
		opts->lists = malloc (sizeof (*opts->lists) * argc);
		if (!opts->lists) {
			printerr ("memory allocation failed");
			return FALSE;
		}
		for (int i = 1; i < argc; i++) {
			opts->lists[i - 1] = argv[i];
		}
		opts->lists[argc - 1] = NULL;
	}
	return TRUE;
}
