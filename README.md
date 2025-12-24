# PlaylistFS

[![CI](https://github.com/trinistr/playlistfs/actions/workflows/CI.yaml/badge.svg)](https://github.com/trinistr/playlistfs/actions/workflows/CI.yaml)

PlaylistFS is a FUSE filesystem that allows to easily fill a directory
with disparate files, similar to UnionFS, but much simpler.

PlaylistFS uses "playlists" to organize files conveniently, and provdes
a way to mount them automatically, but can also use multiple lists
to pull everything in one place. Files in PlaylistFS work similar
to symlinks, but are presented as regular files by default.

Some examples of what PlaylistFS can be used for:
- presenting your playlists in filesystem, not just in an audio player
  (that's where the name comes from!)
- organizing a library by genres or authors without copying files
- putting library header files into a single directory for reference

## Compiling

PlaylistFS has following library dependencies:
- libfuse2
- glib2

On systems with separate development packages those need to be installed as
well. Of course, basic development environment is also needed:
`pkg-config`, `make`, `gcc`.

> [!WARNING]
> Ubuntu has fuse2 packages incompatible with important GUI (and other) packages.
> Installing them will probably break your system.
> Currently, I recommend *not* using this software on Ubuntu and Ubuntu-derived distros.
> In the future, PlaylistFS will work with fuse3, removing this problem.

Ubuntu and Ubuntu-based distros (like Linux Mint):
```sh
apt install libfuse-dev libglib2.0-dev
```

Archlinux and Archlinux-based distros (like Manjaro):
```sh
pacman -S fuse2 glib2
```

To compile `playlistfs`, it is enough to run
```sh
make
```

If you want to also generate a very simple man page, install `help2man` package
and run
```sh
make doc
```

All artifacts and supplementary files are stored in `dist/` directory.

## Testing

You can run tests using any Bourne shell:
```sh
make test
```

`make test` recompiles the binary automatically before testing.
`make test-current` can be used to run tests without recompilation.

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

If you have a compiled binary, and your system has separate development packages,
those aren't needed to just run `playlistfs`.

For example, on Ubuntu it is enough to install `libfuse2t64` and `libglib2.0-0t64`.

### Playlist files

Playlist file format is inspired by M3U and is extremely simple:
- Completely empty lines (not even with spaces) are ignored.
- If a line contains *any* characters (besides new line), it is considered a file path:
  - if the line starts with '/', it is an absolute path,
  - otherwise, it is a path relative to the directory in which playlist is
    located.
- Currently, a line starting with '#' is considered a file path.

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

Unmounting can be done with `fusermount` program, which is provided by FUSE, or `umount`:
```sh
fusermount -u ~/mount_point
# or
umount ~/mount_point
```

### Automatic mounting

PlaylistFS comes with `text/x-playlist` MIME type. It is defined by `*.playlist` glob.
Corresponding file is installed by `make install-mime`.
`make install-mime-default` will also register the provided handler as default.

If a default handler is registered, it is possible to automatically mount and
unmount playlists. This is done by 'opening' .playlist files in file manager.
Provided handler script will create an empty directory, with name formed by
removing .playlist extension from the file, and mount this playlist there.
If the directory already exists, it will be unmounted and deleted instead.
Note that if a non-empty directory will not be removed if it happens to be
named like the playlist, and the automatic mounting will not work.

## License

PlaylistFS, Copyright ® 2018-2020,2025 Alexander Bulancov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

See LICENSE for more information.
