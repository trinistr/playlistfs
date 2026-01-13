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
#include <sys/statvfs.h>
#include <sys/types.h>

typedef struct {
	GString* path;
	nlink_t nlink;
	mode_t type;
	ino_t ino;
} pfs_file;

/*
Create a new pfs_file.
@parameter full_path: The path of the file
@parameter mode: The mode of the file
*/
pfs_file* pfs_file_create (char* path, mode_t mode);

/*
Free a pfs_file. Should only be called if deleted from the file table.
@parameter file: The pfs_file to free
*/
void pfs_file_free (pfs_file*);

/*
Same as pfs_file_free, but for use with GHashTable.
@parameter file: The pfs_file to free
*/
void pfs_file_free_void (void*);

/*
Get next inode number between PFS_FILE_INO_MIN and PFS_FILE_INO_MAX, inclusive.
Returns 0 if the pool is exhausted (quite unlikely to happen in reality).
*/
ino_t pfs_file_next_ino (void);

/*
Get count of currently used inode numbers.
Numbers are never returned, so deleting files does not decrease this number.
*/
fsfilcnt_t pfs_file_used_ino_count (void);

/*
This is the minimum inode number used by pfs_file_next_ino() and pfs_file_create().
Smaller values can be used for other things, if required.
*/
#define PFS_FILE_INO_MIN ((ino_t)1 << 16)
/*
And this is the maximum.
*/
#define PFS_FILE_INO_MAX ((ino_t)1 << (sizeof(ino_t) * 8 - 2))

#endif // PLAYLISTFS_FILES_H
