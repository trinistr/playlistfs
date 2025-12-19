# PlaylistFS

PlaylistFS mounts a FUSE filesystem with files taken from user-supplied list(s).

PlaylistFS allows mounting M3U-inspired file lists as directories. It is
possible to combine several lists and/or individual files in a single mount.

As an alternative to manually mounting on command line, PlaylistFS comes with
files for registering a ".playlist" MIME type and handler for it. The handler
automatically mounts or unmounts playlists when user 'opens' them.

## Compiling

PlaylistFS has following library dependencies:
- libfuse2
- libglib2.0

On systems with separate development packages those need to be installed as
well. On Ubuntu-like systems required packages can be installed by executing the
following command:
```sh
apt install fuse libfuse-dev libglib2.0-dev
```

Makefile also uses `pkg-config`, so make sure it is installed as well.

To compile, it is enough to run
```sh
make
```

If you want to also generate a very simple man page, install `help2man` package
and run
```sh
make doc
```

## Installing

Quick install:
```sh
make install-all
```

Installation can be done in several steps:
- `make install` will install only the compiled binary and man page;
- `make install-mime` will install files needed for automatic mounting;
- `make install-mime-default` will register the provided handler as default.

By default, files are installed in `~/.local` tree. This can be changed by
supplying a PREFIX variable to make:
```sh
PREFIX=/usr make install-all
```

## Usage

### Playlist files

PlaylistFS comes with `text/x-playlist` MIME type.
It is defined by `*.playlist` glob.
Even if you do not install the MIME type, the file format is obviously the same.

Playlist file format is extremely simple:
- completely empty lines (not even with spaces) are ignored;
- if a line contains *any* characters, it is considered a file path:
  - if the line starts with '/', it is an absolute path,
  - otherwise, it is a path relative to the directory in which playlist is
    located.

This file format allows for almost any character to be used in paths, except for
new lines (LF, 0xA).

Example playlist (referred to as `example.playlist` later):
```
file1
../bin/program
/etc/default/keyboard
documents/file1
```
This playlist contains one absolute path (`/etc/default/keyboard`), other paths
are relative. Note that `documents/file1` shadows the earlier `file1`.

### Mounting on command line

> [!NOTE]
> Check all options with `playlistfs --help-all`. Options that are passed along to FUSE are not shown with plain `--help`.

When using command line, it is possible to specify files to mount in several
ways. The user can specify several playlists and individual files to include
in the mounted file system. When encountering several paths with the same
basename, later paths take precedence, and individual files take precedence
over files in playlists. It is not possible to add directories.

Example of command line use:
```sh
playlistfs -f ~/keyboard -f /etc/mtab example.playlist ~/mount_point
```
This will mount a directory at `~/mount_point` with following files:
- file1 → documents/file1
- keyboard → ~/keyboard
- mtab → /etc/mtab
- program → ../bin/program

By default, files are presented as regular files to make copying in file
managers easier. Supplying `--symlink`/`-s` options changes them to symlinks.

Unmounting is done with `fusermount` program, which is provided by FUSE.
```sh
fusermount -u ~/mount_point
```

### Automatic mounting

If PlaylistFS was installed fully, it is possible to automatically mount and
unmount playlists. This is done by 'opening' .playlist files in file manager.
Provided handler script will create an empty directory, with name formed by
removing .playlist extension from the file, and mount this playlist there.
If the directory already exists, it will be unmounted and deleted.
Note that if a non-empty directory will not be removed if it happens to be
named like the playlist, though the automatic mounting will not work.

## License

PlaylistFS, Copyright ® 2018-2020,2025 Aleksandr Bulantcov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

See LICENSE for more information.
