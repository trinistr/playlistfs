#define FUSE_USE_VERSION 26
#define _XOPEN_SOURCE 500	//_POSIX_C_SOURCE 200809L

#include <fuse.h>

// Allowed operations. Unused ones are commented out,
// deprecated are not listed at all
struct fuse_operations pfs_operations = {
	.getattr = pfs_getattr,
	//.readlink = pfs_readlink,
	//.mknod = pfs_mknod,	// No file creation. Regular files should use .create
	//.mkdir = pfs_mkdir,
	//.rmdir = pfs_rmdir,
	//.unlink = pfs_unlink,
	//.symlink = pfs_symlink,
	.rename = pfs_rename,
	.link = pfs_link,
	.chmod = pfs_chmod,	// Maybe these two should not actually be allowed?
	.chown = pfs_chown,
	.truncate = pfs_truncate,	// Questionable
	.open = pfs_open,
	.read = pfs_read,
	.write = pfs_write,
	.statfs = pfs_statvfs,	// = statvfs
	//.flush = pfs_flush,
	.release = pfs_release,	// Files need to be closed
	.fsync = pfs_fsync,
	//.setxatr = pfs_setxattr,	// These four will not be defined for now
	//.getxattr = pfs_getxattr,
	//.listxattr = pfs_listxattr,
	//.removexattr = pfs_removexattr,
	.opendir = pfs_opendir,	// Need to actually check if this gets called on root
	.readdir = pfs_readdir,
	.releasedir = pfs_releasedir,	// Directories also need to be closed, or do they?
	//.fsyncdir = pfs_fsyncdir,	//Can be called on root, probably
	.init = pfs_init,	// These two are not necessarily useful
	.destroy = pfs_destroy,
	.access = pfs_access,	// default_permissions negates the need for this
	//.create = pfs_create,	// No file creation
	.ftruncate = pfs_ftruncate,	// Seems that this one can just call truncate
	.fgetattr = pfs_fgetattr,	// The same
	//.lock = pfs_lock,	//Implemented by kernel (not needed for local FS)
	.utimens = pfs_utimens,	// Use utimensat
	//.bmap = pfs_bmap,	// This FS is not backed by a device
	.flag_nullpath_ok = 0,	// Files can not be removed, so we never work with them
	.flag_nopath = 0,	// May be allowed, as file handles are probably enough
	.flag_utime_omit_ok = 1,	// This will be proxied, so it is okay
	//.ioctl = pfs_ioctl,
	//.poll = pfs_poll,
	//.write_buf = pfs_write_buf,	// Unclear that these do
	//.read_buf = pfs_read_buf,
	//.flock = pfs_flock,	//The same as lock()
	//.fallocate = pfs_fallocate
};

int main (int argc, char* argv[]) {
    pfs_options opts = {};
    if (!pfs_parse_options (&opts, argc, argv)) {
        exit (EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

gboolean pfs_parse_options (pfs_options* opts, int argc, char* argv[]) {
    GError* optionError = NULL;
    GOptionEntry options[] = {
        { "file", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME_ARRAY, &opts->files, "Add a single file to playlist", "FILE" },
        { "symlink", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->symlink, "Create symlinks instead of regular files", NULL },
        { }
    };
    GOptionEntry optionsFuse[] = {
        { "read-only", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts->fuse.ro, "Mount file system as read-only", NULL },
        { "noexec", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &opts->fuse.noexec, "Do not allow execution of files", NULL },
        { "noatime", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &opts->fuse.noatime, "Do not update time of access", NULL },
        { }
    };
    GOptionContext* optionContext = g_option_context_new ("PLAYLIST...");
    GOptionGroup* optionsFuseGroup = g_option_group_new (
        "fuse", "Options passed to FUSE", "FUSE options", NULL, NULL
    );
    g_option_group_set_translation_domain (optionsFuseGroup, NULL);
    g_option_context_add_main_entries (optionContext, options, NULL);
    g_option_context_add_group (optionContext, optionsFuseGroup);
    g_option_group_add_entries (optionsFuseGroup, optionsFuse);
    g_option_context_set_help_enabled (optionContext, TRUE);
    if ( !g_option_context_parse (optionContext, &argc, &argv, &optionError) ) {
        printf ("error: %s\n\n", optionError->message);
        puts (g_option_context_get_help(optionContext, TRUE, NULL));
        return FALSE;
    }
    opts->playlists = (guchar**) malloc (sizeof(guchar*) * (argc-1));
    for (int i = 1; i < argc; i++) {
	opts->playlists[i-1] = argv[i];
    }    
    return TRUE;
}
