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

#ifndef PLAYLISTFS_FILES_H
#define PLAYLISTFS_FILES_H

// You probably will want to define _GNU_SOURCE / _XOPEN_SOURCE before
// including this file to get S_IFMT and co without leading underscores.

#include <glib.h>
#include <sys/stat.h>

typedef struct {
	GString* path;
	__nlink_t nlinks;
	__mode_t type;
} pfs_file;

pfs_file* pfs_file_create (char* path, __mode_t mode);
void pfs_file_free (pfs_file*);
void pfs_file_free_void (void*);

#endif // PLAYLISTFS_FILES_H
