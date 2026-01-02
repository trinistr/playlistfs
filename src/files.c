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

/*
Create a new pfs_file.
@parameter path: The path of the file
@parameter mode: The mode of the file
*/
pfs_file* pfs_file_create (char* path, __mode_t mode) {
	pfs_file* file = malloc (sizeof(*file));
	if (!file)
		return NULL;
	file->path = g_string_new(path);
	if (!file->path) {
		free (file);
		return NULL;
	}
	file->type = mode&S_IFMT;
	file->nlinks = (mode&S_IFMT) == S_IFDIR ? 2 : 1;
	return file;
}

/*
Free a pfs_file. Should only be called if deleted from the file table.
@parameter file: The pfs_file to free
*/
void pfs_file_free (pfs_file* file) {
	g_string_free (file->path, TRUE);
	free (file);
}

/*
Same as pfs_file_free, but for use with g_hash_table_foreach.
@parameter file: The pfs_file to free
*/
void pfs_file_free_void (void* file) {
	pfs_file_free ((pfs_file*)file);
}
