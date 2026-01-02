# rash Build Guide

rash uses GNU make for it's build system and doesn't need any external dependencies, so if you have access to GNU make and a C compiler, chances are you can install rash.

# Downloading rash

First off, you need to get rash, you can either download the latest release from [the releases tab](https://git.myriation.xyz/parker_macdonald/rash/releases), or you run `git clone https://git.myriation.xyz/parker_macdonald/rash.git`.

If you get rash via git clone, you may be a few changes ahead of a release. While I would expect everything to work fine, if you want a stable experience, you should download a release.

# Getting GNU make and a C compiler

First off, you need to get GNU make and a C compiler to build rash, lets go over how to get these on different platforms.

## Linux

If you're running Ubuntu, Debian, Linux Mint, or some other Debian derivative can get GNU make and a C compiler with the `build-essential` package.

```bash
sudo apt install build-essential
```

If you're running Arch Linux, Manjaro, or another Arch derivative, you can get GNU make and a C compiler with the `base-devel` package.

```bash
sudo pacman -S base-devel
```

rash is also packaged in the AUR under [`rash-sh`](https://aur.archlinux.org/packages/rash-sh) if you would prefer to install it that way.

## OpenBSD

OpenBSD comes preinstalled with `clang` as it's default C compiler, so all you have to do is install GNU make from the `gmake` package.

```bash
doas pkg_add gmake
```

## FreeBSD

FreeBSD comes preinstalled with `clang` as it's default C compiler, so all you have to do is install GNU make from the `gmake` package.

```bash
sudo pkg install gmake
```

## Windows

rash relies on *nix (Linux, BSD, or anything vaguely POSIX compliant) system calls, and because of this cannot be run natively on Windows. However, you do have a few options.

### Cygwin

[Cygwin](https://cygwin.com/) is a project that works to provide *nix system calls on Windows through a custom library. Once you have Cygwin installed, you will have access to GNU make and a C compiler inside the Cygwin terminal.

### WSL

[WSL (Windows Subsystem for Linux)](https://learn.microsoft.com/en-us/windows/wsl/install), allows you to run Linux applications under Windows, once you install it, you can follow the instructions for a Linux installation.

# Building

Now that you have GNU make and a C compiler, you can run `make CC=cc DEBUG=0` (or `gmake CC=cc DEBUG=0` on BSD) in the root of the project to build an executable at `./build/rash`.

## Make Flags

When you build rash with make, you can set a variety of flags to make changes to the build process.

- `CC`
  - This flag is used to set which C compiler to use, by default this is `clang` (this it's the C compiler I like the most), but you probably want to set the to `cc` to use your system's default C compiler.
- `DEBUG`
  - This flag is used to make either a debug or release build.
  - `DEBUG=1` is the default, and will produce a debug build (-O0, -g3).
  - `DEBUG=0` will produce a release build (-O3, lto, and symbols stripped).
- `STATIC`
  - This flag is used to make either a statically or dynamically linked executable. If you don't know what this means, you can safely leave it at the default.
  - `STATIC=0` is the default, and will produce a dynamically linked executable.
  - `STATIC=1` will produce a statically linked executable.
- `SANITIZER`
  - This flag is used to set a sanitizer when compiling (`-fsanitize=...`). This is used for development purposes and does not need to be set for a release build.
- `ERROR_HELL`
  - This flag enables all (meaningful) errors and sets the compiler to `clang`. This is used for development purposes and does not need to be set for a release build.
  - `ERROR_HELL=0` is the default, and changes nothing about the build process.
  - `ERROR_HELL=1` enables all (meaningful) errors and sets the compiler to `clang`.

# Installing

Now that you have an executable in `./build/rash`, you can install it using `make install`, (or `gmake install` on BSD). Keep in mind this needs to be run as root.

By default this will install rash to `/usr/local/bin/rash`, but you can change this by running `make install PREFIX=...` with different values for PREFIX.

- `PREFIX=/usr/local` is the default, this installs rash at `/usr/local/bin/rash`.
- Some other examples might be `PREFIX=/` to install rash to `/bin/rash` or `PREFIX=/usr` to install rash to `/usr/bin/rash`.

# Using rash

Once you have rash installed, consider checking out [the user guide](USER_GUIDE.md).