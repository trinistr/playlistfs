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

#include "pfs_libgen.h"

#define _XOPEN_SOURCE 700 // strdup()
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

/*
From dirname(3):
    path       dirname   basename
    /usr/lib   /usr      lib
    /usr/      /         usr
    usr        .         usr
    /          /         /
    .          .         .
    ..         .         ..
*/

/*
Return position of last '/' inside path or -1 if not found.
*/
inline static ptrdiff_t last_slash_pos (const char* path);

char* pfs_dirname (const char* path) {
    if (path == NULL) {
        return NULL;
    }

    ptrdiff_t pos = last_slash_pos (path);
    char* name;
    if (pos > 0) {
        name = malloc (pos + 1);
        if (name == NULL) {
            return NULL;
        }
        strncpy (name, path, pos);
        name[pos] = '\0';
    }
    else if (pos < 0) {
        name = strdup (".");
    }
    else {
        name = strdup("/");
    }
    return name;
}

char* pfs_basename (const char* path) {
    if (path == NULL) {
        return NULL;
    }

    ptrdiff_t pos = last_slash_pos (path);
    char* name;
    if (pos >= 0) {
        if (path[pos + 1] != '\0') {
            name = strdup (path + pos + 1);
        }
        else {
            name = strdup("");
        }
    }
    else {
        name = strdup (path);
    }
    return name;
}

inline static ptrdiff_t last_slash_pos (const char* path) {
    char* pos = strrchr (path, '/');
    if (pos != NULL) {
        return pos - path;
    }
    else {
        return -1;
    }
}
