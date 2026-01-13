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
	nlink_t nlink;
	mode_t type;
	ino_t ino;
} pfs_file;

pfs_file* pfs_file_create (char* path, mode_t mode);
void pfs_file_free (pfs_file*);
void pfs_file_free_void (void*);

/*
This is the minimum inode number used by pfs_file_create();
Smaller values can be used freely for other things.
*/
#define PFS_FILE_MIN_USED_INO ((ino_t)1 << 16)
/*
And this is the maximum.
Most files that can be created is
  (PFS_FILE_MAX_USED_INO - PFS_FILE_MIN_USED_INO + 1),
though, realistically, memory will run out first.
*/
#define PFS_FILE_MAX_USED_INO ((ino_t)1 << (sizeof(ino_t) * 8 - 2))

#endif // PLAYLISTFS_FILES_H
