# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Note that versions before 0.3.0 were bumped at pretty much random points in time, while this changelog was written 5 years later, with tags set at reasonable points.

## [Next]

**Added**
- Support for FUSE 3.
  - There is no attempt to force compatibility. All capabilities are in the default state.
- FUSE and FUSE_VERSION flags for `make`.
  - FUSE=3 is default, can be set to FUSE=2 to compile against libfuse2.
  - FUSE_VERSION can set desired FUSE_USE_VERSION to use newer features of libfuse. By default it is either 35 or 29, depending on FUSE flag.
- Support for `RENAME_NOREPLACE` and `RENAME_EXCHANGE` flags to `rename` with FUSE 3. In particular, this allows `mv --exchange`.
- `uninstall*` targets for `make`, allowing easy deletion. `uninstall` targets mirror `install*` ones, except where no uninstall procedure exists.

**Changed**
- `--version`/`-V` now outputs build date and libfuse's version the binary was built against.
- Changed some option names:
  - Short form of `--no-relative` option was changed to `-N` (from `-n`).
  - `--symlink` is renamed to `--symlinks`.
  - `--(no-)relative-lists` is renamed to `--(no-)relative-paths`. An option to disable relative paths for lists themselves may be added in the future.
- `--relative*` options are now visible in help output.
- Changed installation processes:
  - Renamed targets:
    - `install-all` -> `install-full`
    - `install-mime` -> `install-supplementary`
    - `install-mime-default` -> `install-set-default`
  - Compiled binary is now stripped during installation.
  - The MIME package is now `playlistfs.xml` (previously `x-playlist.xml`), complying with [specification](https://specifications.freedesktop.org/shared-mime-info/0.21/ar01s02.html).
  - MIME type of playlists is now `text/x-playlistfs-playlist` (previously `text/x-playlist`).
    - Old MIME definitions may be removed by deleting `<MIME DIR>/packages/x-playlist.xml` and running `update-mime-database <MIME DIR>`.
  - `XDG_DATA_HOME` is used for installation if defined. `PREFIX`, if set explicitly, will override it.
  - MIME package is installed using `install-mime-package` target, which is included in `install-supplementary`.
  - `install-supplementary` will no longer install the `playlistfs` binary.

**Fixed**
- Non-executable files are no longer installed with execution permissions.
- `--no-relative*` options can be overridden by later `--relative*` options and vice versa.

[Compare v0.3.1...main](https://github.com/trinistr/playlistfs/compare/v0.3.1...main)

## [v0.3.1]

**Added**
- SANITIZER=1 flag to `make` to enable ASan. `SANITIZE=1 make test` will also add ASAN_OPTIONS to enable even more diagnostics.

**Fixed**
- Buffer overflow in start-up code.

[Compare v0.3.0...main](https://github.com/trinistr/playlistfs/compare/v0.3.0...v0.3.1)

## [v0.3.0]

This update is focused on rewriting internals to make the code more maintainable and less error-prone. Nonetheless, there are a couple of new features and bugfixes.

**Added**
- An autotest system to ensure at least some quality.
- Warning message when no lists or files are specified, only a mount point.
- `--fsname` option to set a custom filesystem name.

**Changed**
- Allow using `--version`/`-V` together with normal operations.
- `make install*` targets have been changed again: `install-bin` and `install-man` can be used to install the binary and man page separately.
- Change default fsname to basename of the first list (including extensions), or "playlistfs" if there are no lists, without any quotation marks.

**Fixed**
- In verbose display, lists names now display fully (previously, it was just dirname).
- In verbose display, relative paths in lists now display both relative and absolute paths.
- Probably some undiscovered bugs related to handling current directory and relative paths.
- Memory leaks during start-up.

[Compare v0.2.2...v0.3.0](https://github.com/trinistr/playlistfs/compare/v0.2.2...v0.3.0)

## [v0.2.2] — 2020-09-28

**Added**
- README.md with explanations and examples.

**Changed**
- `make install*` targets were changed:
  - `install` now installs only the `playlistfs` binary and the man page.
  - `install-mime` now installs both the binary and the script along with the .desktop file and a MIME definition for `x-playlist`.
  - Added `install-mime-default` that sets the .desktop file as the default handler for `x-playlist`.

**Fixed**
- `playlistfs` no longer hangs if a playlist does not end with a newline.

[Compare v0.2.1...v0.2.2](https://github.com/trinistr/playlistfs/compare/v0.2.1...v0.2.2)

## [v0.2.1] — 2020-09-01

**Added**
- `playlistfs_mount` script, `playlistfs_mount.desktop` and `x-playlist.xml` MIME definition to handle mounting and unmounting directly from the file manager.
- `make install` and `make install-mime` targets. `install-mime` installs the MIME definition for `x-playlist` and sets the .desktop file as the default handler for it.

[Compare v0.2.0...v0.2.1](https://github.com/trinistr/playlistfs/compare/v0.2.0...v0.2.1)

## [v0.2.0] — 2019-07-04

**Added**
- Handling of relative paths for both paths in lists and those given on command-line.
- Ability to disable relative path handling.

**Fixed**
- Filesystem name (visible in `/etc/mtab`) is now set to a default value when no lists are given. Usually, it is set to the name of the first list.

[Compare v0.1.0...v0.2.0](https://github.com/trinistr/playlistfs/compare/v0.1.0...v0.2.0)

## [v0.1.0] — 2018-11-14

This version was known as 0.9.0 at the time, so it reports that.

**Added**
- Reading of playlist files with absolute paths.
- Adding individual files with absolute paths.
- Ability to present files as symlinks or regular files.
- Several options passed to FUSE.

[Next]: https://github.com/trinistr/playlistfs/tree/main
[v0.3.1]: https://github.com/trinistr/playlistfs/tree/v0.3.1
[v0.3.0]: https://github.com/trinistr/playlistfs/tree/v0.3.0
[v0.2.2]: https://github.com/trinistr/playlistfs/tree/v0.2.2
[v0.2.1]: https://github.com/trinistr/playlistfs/tree/v0.2.1
[v0.2.0]: https://github.com/trinistr/playlistfs/tree/v0.2.0
[v0.1.0]: https://github.com/trinistr/playlistfs/tree/v0.1.0
