/*
 * This file is part of Playlist File System
 * Copyright ©2018 Aleksandr Bulantcov
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

#include <playlistfs.h>

//void* pfs_init (struct fuse_conn_info *conn) {
//	return fuse_get_context ()->private_data;
//}

void pfs_destroy (void* userdata) {
	pfs_data* data = userdata;
	g_hash_table_unref (data->filetable);
	free (data);
}

int pfs_getattr (const char* path, struct stat* statbuf) {
	if (0 == strcmp (path, "/")) {
		statbuf->st_mode = S_IFDIR | 0777;
		statbuf->st_nlink = 2;
		return 0;
	}
	pfs_data* data = fuse_get_context ()->private_data;
	// The path always starts with '/'
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	if (lstat (realpath, statbuf) < 0)
		return -errno;
	if (data->opts.fuse.ro)
		statbuf->st_mode &= ~0222;
	if (data->opts.fuse.noexec)
		statbuf->st_mode &= ~0111;
	return 0;
}

int pfs_readlink (const char* path, char* buf, size_t size) {
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	strncpy (buf, realpath, size);
	buf[size - 1] = '\0';
	return 0;
}

// int pfs_link (const char* path, const char* newpath);

int pfs_unlink (const char* path) {
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	g_hash_table_remove (data->filetable, path+1);
	return 0;
}

int pfs_rename (const char* path, const char* newpath) {
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = NULL;
	char* key = NULL;
	if (!g_hash_table_lookup_extended (data->filetable, path + 1, (void**) &key, (void**) &realpath))
		return -ENOENT;
	free (key);
	g_hash_table_steal (data->filetable, path + 1);
	g_hash_table_insert (data->filetable, strdup (newpath+1), realpath);
	return 0;
}

int pfs_truncate (const char* path, off_t size) {
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	if (truncate (realpath, size) < 0)
		return -errno;
	return 0;
}

int pfs_open (const char* path, struct fuse_file_info* fi) {
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	int fd = open (realpath, fi->flags);
	if (fd < 0)
		return -errno;
	fi->fh = fd;
	return 0;
}

int pfs_read (const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	return pread (fi->fh, buf, size, offset);
}

int pfs_write (const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	return pwrite (fi->fh, buf, size, offset);
}

int pfs_release (const char* path, struct fuse_file_info* fi) {
	return close (fi->fh);
}

int pfs_statfs (const char* path, struct statvfs* statbuf) {
	pfs_data* data = fuse_get_context ()->private_data;
	statbuf->f_files = g_hash_table_size (data->filetable);
	statbuf->f_ffree = 0;
	statbuf->f_favail = 0;
	statbuf->f_namemax = NAME_MAX;
	return 0;
}

int pfs_fsync (const char* path, int datasync, struct fuse_file_info* fi) {
	if (datasync)
		return fdatasync (fi->fh);
	else
		return fsync (fi->fh);
}

// TODO: handle all directories

int pfs_opendir (const char* path, struct fuse_file_info* fi) {
	if (0 != strcmp (path, "/"))
		return -ENOENT;
	// pfs_data* data = fuse_get_context ()->private_data;
	return 0;
}

int pfs_readdir (const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	if (0 != strcmp (path, "/"))
		return -ENOENT;
	pfs_data* data = fuse_get_context ()->private_data;
	unsigned int length;
	char** paths = (char**) g_hash_table_get_keys_as_array (data->filetable, &length);
	filler (buf, ".", NULL, 0);
	filler (buf, "..", NULL, 0);
	for (size_t i = 0; i < length; i++) {
		if (0 != filler (buf, paths[i], NULL, 0))
			return -EIO;
	}
	g_free (paths);
	return 0;
}

int pfs_releasedir (const char* path, struct fuse_file_info* fi) {
	if (0 != strcmp (path, "/"))
		return -ENOENT;
	// pfs_data* data = fuse_get_context ()->private_data;
	return 0;
}

int pfs_access (const char* path, int mode) {
	if (0 == strcmp(path, "/"))
		return 0;
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	if (access (realpath, mode) < 0)
		return -errno;
	return 0;
}

int pfs_ftruncate (const char* path, off_t size, struct fuse_file_info* fi) {
	if (ftruncate (fi->fh, size) < 0)
		return -errno;
	return 0;
}

int pfs_fgetattr (const char* path, struct stat* statbuf, struct fuse_file_info* fi) {
	if (0 == strcmp (path, "/"))
		return pfs_getattr (path, statbuf);
	if (fstat (fi->fh, statbuf) < 0)
		return -errno;
	return 0;
}

int pfs_utimens (const char* path, const struct timespec tv[2]) {
	pfs_data* data = fuse_get_context ()->private_data;
	char* realpath = g_hash_table_lookup (data->filetable, path + 1);
	if (!realpath)
		return -ENOENT;
	if (utimensat (AT_FDCWD, realpath, tv, 0) < 0)
		return -errno;
	return 0;
}
