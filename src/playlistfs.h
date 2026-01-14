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

#include "playlistfs_version.h"

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 35
#endif

#define _GNU_SOURCE // _XOPEN_SOURCE & GNU fallocate(), pread(), pwrite() and other
#define _FILE_OFFSET_BITS 64 // FUSE requires 64-bit off_t

#include <fuse.h>
#include <glib.h>
#include <time.h>

typedef struct {
	char* path;
	mode_t type;
} pfs_file_entry;

typedef struct {
	char** lists;
	GArray* files;
	char* mount_point;
	struct timespec started_at;
	gboolean symlinks;
	gboolean verbose;
	gboolean show_version;
	gboolean quiet;
	struct {
		gboolean all;
		gboolean files;
		gboolean paths;
	} relative_disabled;
	struct {
		char* fsname;
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

void pfs_free_pfs_data (pfs_data* data);

#endif // PLAYLISTFS_H
