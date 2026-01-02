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

#ifndef PLAYLISTFS_LIBGEN_H
#define PLAYLISTFS_LIBGEN_H

/*
Return part of path up to, but not including, the last "/".
If path does not include "/", return "."
If path is "/" or directly under it, return "/".
Returned string must be freed by the caller.
@parameter path: a Unix file path
*/
char* pfs_dirname (const char* path);

/*
Return part of path after the last "/".
If path does not include "/", return a copy of path.
If path ends on a "/", return an empty string.
Returned string must be freed by the caller.
@parameter path: a Unix file path
*/
char* pfs_basename (const char* path);

#endif // PLAYLISTFS_LIBGEN_H
