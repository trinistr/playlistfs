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
#include "files.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if FUSE_USE_VERSION >= 30
static void* pfs_init (struct fuse_conn_info *conn, struct fuse_config *cfg);
#else
static void* pfs_init (struct fuse_conn_info *conn);
#endif // pfs_init
static void pfs_destroy (void *);
static int pfs_statfs (const char *, struct statvfs *);
#if FUSE_USE_VERSION >= 30
static int pfs_getattr (const char *, struct stat *, struct fuse_file_info *);
#else
static int pfs_getattr (const char *, struct stat *);
#endif // pfs_getattr
static int pfs_readlink (const char *, char *, size_t);
static int pfs_unlink (const char *);
static int pfs_symlink (const char* path, const char* link);
#if FUSE_USE_VERSION >= 30
static int pfs_rename (const char *, const char *, unsigned int flags);
#else
static int pfs_rename (const char *, const char *);
#endif // pfs_rename
static int pfs_link (const char *, const char *);
#if FUSE_USE_VERSION >= 30
static int pfs_truncate (const char *, off_t, struct fuse_file_info *);
#else
static int pfs_truncate (const char *, off_t);
#endif // pfs_truncate
static int pfs_open (const char *, struct fuse_file_info *);
static int pfs_read (const char *, char *, size_t, off_t, struct fuse_file_info *);
static int pfs_write (const char *, const char *, size_t, off_t, struct fuse_file_info *);
static int pfs_release (const char *, struct fuse_file_info *);
static int pfs_fsync (const char *, int, struct fuse_file_info *);
static int pfs_opendir (const char *, struct fuse_file_info *);
#if FUSE_USE_VERSION >= 30
static int pfs_readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags);
#else
static int pfs_readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
#endif // pfs_readdir
static int pfs_releasedir (const char *, struct fuse_file_info *);
static int pfs_access (const char *, int);
// Reused even if FUSE_USE_VERSION >= 30
static int pfs_ftruncate (const char *, off_t, struct fuse_file_info *);
static int pfs_fgetattr (const char *, struct stat *, struct fuse_file_info *);
#if FUSE_USE_VERSION >= 30
static int pfs_utimens (const char *, const struct timespec tv[2], struct fuse_file_info *);
#else
static int pfs_utimens (const char *, const struct timespec tv[2]);
#endif // pfs_utimens
static int pfs_fallocate (const char *, int, off_t, off_t, struct fuse_file_info *);
#if FUSE_USE_VERSION >= 30
static off_t pfs_lseek (const char *, off_t, int, struct fuse_file_info *);
#endif // pfs_lseek
/*
This struct is exported to playlistfs.c.

Allowed operations. Unused ones are commented out,
deprecated are not listed at all.
*/
struct fuse_operations pfs_operations = {
	.init = pfs_init, // These two are not necessarily useful
	.destroy = pfs_destroy,
	.statfs = pfs_statfs, // = statvfs
	.getattr = pfs_getattr,
	.readlink = pfs_readlink,// For symlink mode and symlinks in general
	//.mknod = pfs_mknod, // No file creation. Regular files should use .create anyway
	//.mkdir = pfs_mkdir,
	//.rmdir = pfs_rmdir,
	.unlink = pfs_unlink,
	.symlink = pfs_symlink,
	.rename = pfs_rename,
	.link = pfs_link,
	//.chmod = pfs_chmod, // Maybe these two should not actually be allowed?
	//.chown = pfs_chown,
	.truncate = pfs_truncate,
	.open = pfs_open,
	.read = pfs_read,
	.write = pfs_write,
	//.flush = pfs_flush,
	.release = pfs_release, // Files need to be closed
	.fsync = pfs_fsync,
	//.setxatr = pfs_setxattr, // These four will not be defined for now
	//.getxattr = pfs_getxattr,
	//.listxattr = pfs_listxattr,
	//.removexattr = pfs_removexattr,
	.opendir = pfs_opendir,
	.readdir = pfs_readdir,
	.releasedir = pfs_releasedir, // Directories also need to be closed, or do they?
	//.fsyncdir = pfs_fsyncdir, //Can be called on root, probably
	.access = pfs_access, // default_permissions negates the need for this
	//.create = pfs_create, // No file creation
	#if FUSE_USE_VERSION < 30
	.ftruncate = pfs_ftruncate,
	.fgetattr = pfs_fgetattr,
	#endif
	//.lock = pfs_lock, // Implemented by kernel (not needed for local FS)
	.utimens = pfs_utimens, // Use utimensat
	//.bmap = pfs_bmap, // This FS is not backed by a device
	//.ioctl = pfs_ioctl,
	//.poll = pfs_poll,
	//.write_buf = pfs_write_buf, // Unclear that these do
	//.read_buf = pfs_read_buf,
	//.flock = pfs_flock, //The same as lock()
	.fallocate = pfs_fallocate,
	#if FUSE_USE_VERSION >= 30
	// .copy_file_range = pfs_copy_file_range,
	.lseek = pfs_lseek,
	#endif
	#if FUSE_USE_VERSION < 30
	.flag_nullpath_ok = 0,
	.flag_nopath = 0,
	.flag_utime_omit_ok = 1,
	#endif
};

static ino_t root_ino;

#if FUSE_USE_VERSION < 30
static void* pfs_init (struct fuse_conn_info *conn) {
#else
static void* pfs_init (struct fuse_conn_info *conn, struct fuse_config *cfg) {
	// FUSE 2 uses options in argv for these, see playlistfs.c.
	cfg->attr_timeout = 0.0;
	cfg->use_ino = 1;
#endif
	root_ino = pfs_file_next_ino ();
	return fuse_get_context ()->private_data;
}

static void pfs_destroy (void* private_data) {
	pfs_free_pfs_data ((pfs_data*) private_data);
}

static int pfs_statfs (const char* path, struct statvfs* statbuf) {
	fsfilcnt_t used = pfs_file_used_ino_count ();
	statbuf->f_files = used;
	statbuf->f_ffree = PFS_FILE_INO_MAX - PFS_FILE_INO_MIN - used;
	statbuf->f_namemax = NAME_MAX;
	return 0;
}

#if FUSE_USE_VERSION < 30
static int pfs_getattr (const char* path, struct stat* statbuf) {
#else
static int pfs_getattr (const char* path, struct stat* statbuf, struct fuse_file_info* fi) {
	if (fi != NULL) {
		return pfs_fgetattr (path, statbuf, fi);
	}
#endif
	if (0 == strcmp (path, "/")) {
		statbuf->st_mode = S_IFDIR | 0777;
		statbuf->st_nlink = 2;
		statbuf->st_ino = root_ino;
		return 0;
	}

	struct fuse_context* context = fuse_get_context ();
	pfs_data* data = context->private_data;
	// The path always starts with '/'
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file)
		return -ENOENT;
	if (!data->opts.symlinks && !S_ISLNK(file->type)) {
		if (lstat (file->path->str, statbuf) < 0)
			return -errno;
		if (data->opts.fuse.ro)
			statbuf->st_mode &= ~0222;
		if (data->opts.fuse.noexec && S_ISREG(statbuf->st_mode))
			statbuf->st_mode &= ~0111;
	}
	else {
		statbuf->st_mode = S_IFLNK|0777;
		statbuf->st_uid = context->uid;
		statbuf->st_gid = context->gid;
		statbuf->st_size = file->path->len;
		statbuf->st_atim.tv_sec = statbuf->st_ctim.tv_sec = statbuf->st_mtim.tv_sec = file->ts.tv_sec;
		statbuf->st_atim.tv_nsec = statbuf->st_ctim.tv_nsec = statbuf->st_mtim.tv_nsec = file->ts.tv_nsec;
	}
	statbuf->st_ino = file->ino;
	statbuf->st_nlink = file->nlink;
	return 0;
}

static int pfs_readlink (const char* path, char* buf, size_t size) {
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file)
		return -ENOENT;
	if (S_ISLNK (file->type)) {
		strncpy (buf, file->path->str, size);
		buf[size - 1] = '\0';
	}
	else {
		ssize_t length = readlink (file->path->str, buf, size-1);
		if (length < 0)
			return -errno;
		buf[length] = '\0';
	}
	return 0;
}

static int pfs_unlink (const char* path) {
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file;
	char* key;
	if (!g_hash_table_lookup_extended (data->filetable, path + 1, (void**)&key, (void**) &file))
		return -ENOENT;
	g_hash_table_steal (data->filetable, key);
	free (key);
	if (--file->nlink == 0) {
		pfs_file_free (file);
	}
	return 0;
}

static int pfs_symlink (const char* path, const char* link) {
	pfs_data* data = fuse_get_context ()->private_data;
	if (g_hash_table_contains (data->filetable, link + 1))
		return -EEXIST;
	struct timespec now;
	clock_gettime (CLOCK_REALTIME, &now);
	pfs_file* file = pfs_file_create (strdup(path), S_IFLNK, &now);
	if (file == NULL)
		return -ENOSPC;
	g_hash_table_insert (data->filetable, strdup(link + 1), file);
	return 0;
}

#if FUSE_USE_VERSION < 30
static int pfs_rename (const char* path, const char* newpath) {
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = NULL;
	char* key = NULL;

	if (!g_hash_table_lookup_extended (data->filetable, path + 1, (void**) &key, (void**) &file))
		return -ENOENT;

	g_hash_table_steal (data->filetable, key);
	free (key);
	g_hash_table_insert (data->filetable, strdup (newpath+1), file);
	return 0;
}
#else
static int pfs_rename (const char* path, const char* newpath, unsigned int flags) {
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file1 = NULL;
	char* name1 = NULL;
	pfs_file* file2 = NULL;
	char* name2 = NULL;

	if (!g_hash_table_lookup_extended (data->filetable, path + 1, (void**) &name1, (void**) &file1))
		return -ENOENT;
	// All variants need to check the target in some way.
	g_hash_table_lookup_extended (data->filetable, newpath + 1, (void**) &name2, (void**) &file2);

	// Rename should first replace the target, according to standards.
	// From rename(2):
	//   If newpath already exists, it will be atomically replaced, so that there
	//   is no point at which another process attempting to access newpath will
	//   find it missing. However, there will probably be a window in which both
	//   oldpath and newpath refer to the file being renamed.
	switch (flags) {
	case RENAME_EXCHANGE:
		if (file2 == NULL)
			return -ENOENT;
		g_hash_table_steal (data->filetable, name2);
		g_hash_table_insert (data->filetable, name2, file1);
		g_hash_table_steal (data->filetable, name1);
		g_hash_table_insert (data->filetable, name1, file2);
		break;
	case RENAME_NOREPLACE:
		if (file2 != NULL)
			return -EEXIST;
		// Fall through to default rename
	default:
		// From rename(2):
		//   If oldpath and newpath are existing hard links referring to the
		//   same file, then rename() does nothing, and returns a success status.
		if (file2 != NULL && file1 == file2)
			return 0;
		g_hash_table_insert (data->filetable, strdup (newpath+1), file1);
		g_hash_table_steal (data->filetable, name1);
		free (name1);
	}
	return 0;
}
#endif

static int pfs_link (const char* path, const char* newpath) {
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file || S_ISDIR(file->type))
		return -ENOENT;
	char* key = strdup (newpath + 1);
	if (!key)
		return -errno;
	g_hash_table_insert (data->filetable, key, file);
	file->nlink++;
	return 0;
}

#if FUSE_USE_VERSION < 30
static int pfs_truncate (const char* path, off_t size) {
#else
static int pfs_truncate (const char* path, off_t size, struct fuse_file_info* fi) {
	if (fi != NULL) {
		return pfs_ftruncate (path, size, fi);
	}
#endif
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file)
		return -ENOENT;
	if (truncate (file->path->str, size) < 0)
		return -errno;
	return 0;
}

static int pfs_open (const char* path, struct fuse_file_info* fi) {
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file)
		return -ENOENT;
	int fd = open (file->path->str, fi->flags);
	if (fd < 0)
		return -errno;
	fi->fh = fd;
	return 0;
}

static int pfs_read (const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	return pread (fi->fh, buf, size, offset);
}

static int pfs_write (const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	return pwrite (fi->fh, buf, size, offset);
}

static int pfs_release (const char* path, struct fuse_file_info* fi) {
	return close (fi->fh);
}

static int pfs_fsync (const char* path, int datasync, struct fuse_file_info* fi) {
	if (datasync)
		return fdatasync (fi->fh);
	else
		return fsync (fi->fh);
}

// TODO: handle all directories

static int pfs_opendir (const char* path, struct fuse_file_info* fi) {
	if (0 != strcmp (path, "/"))
		return -ENOENT;
	// pfs_data* data = fuse_get_context ()->private_data;
	return 0;
}

inline static int pfs_readdir_call_filler(fuse_fill_dir_t filler, void* buf, const char* name) {
	#if FUSE_USE_VERSION < 30
		return filler (buf, name, NULL, 0);
	#else
		return filler (buf, name, NULL, 0, 0);
	#endif
}

#if FUSE_USE_VERSION < 30
static int pfs_readdir (const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
#else
static int pfs_readdir (const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
#endif
	if (0 != strcmp (path, "/"))
		return -ENOENT;

	if (0 != pfs_readdir_call_filler (filler, buf, ".")) {
		return -EIO;
	}
	if (0 != pfs_readdir_call_filler (filler, buf, "..")) {
		return -EIO;
	}
	
	pfs_data* data = fuse_get_context ()->private_data;
	unsigned int length;
	char** paths = (char**) g_hash_table_get_keys_as_array (data->filetable, &length);
	for (size_t i = 0; i < length; i++) {
		if (0 != pfs_readdir_call_filler (filler, buf, paths[i])) {
			g_free (paths);
			return -EIO;
		}
	}
	g_free (paths);
	return 0;
}

static int pfs_releasedir (const char* path, struct fuse_file_info* fi) {
	if (0 != strcmp (path, "/"))
		return -ENOENT;
	// pfs_data* data = fuse_get_context ()->private_data;
	return 0;
}

static int pfs_access (const char* path, int mode) {
	if (0 == strcmp(path, "/"))
		return 0;
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file)
		return -ENOENT;
	if (access (file->path->str, mode) < 0)
		return -errno;
	return 0;
}

// These two are used in pfs_getattr and pfs_truncate if FUSE_USE_VERSION >= 30.
static int pfs_fgetattr (const char* path, struct stat* statbuf, struct fuse_file_info* fi) {
	if (fstat (fi->fh, statbuf) < 0)
		return -errno;
	return 0;
}

static int pfs_ftruncate (const char* path, off_t size, struct fuse_file_info* fi) {
	if (ftruncate (fi->fh, size) < 0)
		return -errno;
	return 0;
}

#if FUSE_USE_VERSION < 30
static int pfs_utimens (const char* path, const struct timespec tv[2]) {
#else
static int pfs_utimens (const char* path, const struct timespec tv[2], struct fuse_file_info* fi) {
	if (fi != NULL) {
		if (!futimens(fi->fh, tv))
			return -errno;
		return 0;
	}
#endif
	pfs_data* data = fuse_get_context ()->private_data;
	pfs_file* file = g_hash_table_lookup (data->filetable, path + 1);
	if (!file)
		return -ENOENT;
	if (utimensat (AT_FDCWD, file->path->str, tv, 0) < 0)
		return -errno;
	return 0;
}

static int pfs_fallocate (const char* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) {
	if (fallocate (fi->fh, mode, offset, length) < 0)
		return -errno;
	return 0;
}

#if FUSE_USE_VERSION >= 30
static off_t pfs_lseek (const char* path, off_t offset, int whence, struct fuse_file_info *fi) {
	off_t result = lseek (fi->fh, offset, whence);
	if (result < 0)
		return -errno;
	return result;
}
#endif // pfs_lseek
