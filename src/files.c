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

#define _GNU_SOURCE // S_IFMT and co without underscores

#include "files.h"

#include <stdlib.h>

static ino_t current_ino = PFS_FILE_INO_MIN;

pfs_file* pfs_file_create (const char* full_path, const mode_t type, const struct timespec* ts) {
	ino_t new_ino = pfs_file_next_ino ();
	if (new_ino == 0)
		return NULL;

	pfs_file* file = calloc (1, sizeof (*file));
	if (!file)
		return NULL;
	file->path = g_string_new (full_path);
	if (!file->path) {
		free (file);
		return NULL;
	}
	if (ts != NULL) {
		file->ts.tv_sec = ts->tv_sec;
		file->ts.tv_nsec = ts->tv_nsec;
	}
	file->type = type&S_IFMT;
	file->nlink = S_ISDIR(type) ? 2 : 1;
	file->ino = new_ino;
	return file;
}

void pfs_file_free (pfs_file* file) {
	g_string_free (file->path, TRUE);
	free (file);
}

void pfs_file_free_void (void* file) {
	pfs_file_free ((pfs_file*)file);
}

ino_t pfs_file_next_ino (void) {
	if (current_ino > PFS_FILE_INO_MAX)
		return 0;
	return current_ino++;
}

fsfilcnt_t pfs_file_used_ino_count (void) {
	return (fsfilcnt_t)(current_ino - PFS_FILE_INO_MIN);
}
