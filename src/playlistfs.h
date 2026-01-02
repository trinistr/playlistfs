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

#ifndef PLAYLISTFS_H
#define PLAYLISTFS_H

#define FUSE_USE_VERSION 26
#define _XOPEN_SOURCE 700 // Several functions, including pread and pwrite
#define _GNU_SOURCE // GNU fallocate()
#define _FILE_OFFSET_BITS 64 // FUSE requires 64-bit off_t

#include <string.h>
#include <fuse.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>

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
		gchar* fsname;
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

#endif // PLAYLISTFS_H
