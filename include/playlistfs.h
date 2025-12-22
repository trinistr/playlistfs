/*
 * This file is part of Playlist File System
 * Copyright © 2018-2020,2025 Alexander Bulancov
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

#ifndef PLAYLISTFS_H
#define PLAYLISTFS_H

#define FUSE_USE_VERSION 26
#define _XOPEN_SOURCE 700	// Several functions, including pread and pwrite
#define _GNU_SOURCE	// GNU basename()

#include <string.h>
#include <fuse.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#ifndef PLAYLISTFS_VERSION
#define PLAYLISTFS_VERSION "0.2.2"
#endif

typedef struct {
	char** files;
	char** lists;
	char* mount_point;
	gboolean symlink;
	struct {
		gboolean all;
		gboolean files;
		gboolean lists;
	} relative_disabled;
	gboolean verbose;
	gboolean show_version;
	gboolean quiet;
	struct {
		gboolean ro;
		gboolean noexec;
		gboolean noatime;
		gboolean debug;
	} fuse;
} pfs_options;

typedef struct {
	pfs_options opts;
	GHashTable* filetable;
} pfs_data;

typedef struct {
	GString* path;
	__mode_t type;
	char nlinks;
} pfs_file;

pfs_file* pfs_file_create (char* path, __mode_t mode);
void pfs_file_free (pfs_file*);
void pfs_file_free_void (void*);

void* pfs_init (struct fuse_conn_info *conn);
void pfs_destroy (void *);
int pfs_getattr (const char *, struct stat *);
int pfs_readlink (const char *, char *, size_t);
int pfs_unlink (const char *);
int pfs_symlink (const char* path, const char* link);
int pfs_rename (const char *, const char *);
int pfs_link (const char *, const char *);
int pfs_truncate (const char *, off_t);
int pfs_open (const char *, struct fuse_file_info *);
int pfs_read (const char *, char *, size_t, off_t, struct fuse_file_info *);
int pfs_write (const char *, const char *, size_t, off_t, struct fuse_file_info *);
int pfs_statfs (const char *, struct statvfs *);
int pfs_release (const char *, struct fuse_file_info *);
int pfs_fsync (const char *, int, struct fuse_file_info *);
int pfs_opendir (const char *, struct fuse_file_info *);
int pfs_readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
int pfs_releasedir (const char *, struct fuse_file_info *);
int pfs_access (const char *, int);
int pfs_ftruncate (const char *, off_t, struct fuse_file_info *);
int pfs_fgetattr (const char *, struct stat *, struct fuse_file_info *);
int pfs_utimens (const char *, const struct timespec tv[2]);
int pfs_fallocate (const char *, int, off_t, off_t, struct fuse_file_info *);

#endif // PLAYLISTFS_H
