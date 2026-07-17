# rash Build Guide

rash uses CMake for it's build system and doesn't need any external dependencies.

# Downloading rash

First off, you need to get rash, you can either download the latest release from [the releases tab](https://git.myriation.xyz/parker_macdonald/rash/releases), or you run `git clone https://git.myriation.xyz/parker_macdonald/rash.git`.

There is also a github mirror available [here](https://github.com/parker-macdonald/rash).

If you get rash via git clone, you may be a few changes ahead of a release. While I would expect everything to work fine, if you want a stable experience, you should download a release.

# Getting CMake and a C compiler

First off, you need to get CMake and some other tools, lets go over how to get these on different platforms.

## Linux

### Ubuntu, Linux Mint, Debian, etc.

If you're running Ubuntu, Debian, Linux Mint, or some other Debian derivative can get CMake with the `cmake` and a C compiler with the `build-essential` package.

```bash
sudo apt install cmake build-essential
```

### Arch Linux, Manjaro, EndeavourOS, etc.

If you're running Arch Linux, Manjaro, or another Arch derivative, you can get CMake with the `cmake` and a C compiler with the `base-devel` package.

```bash
sudo pacman -S cmake base-devel
```

rash is also packaged in the AUR under [`rash-sh`](https://aur.archlinux.org/packages/rash-sh) if you would prefer to install it that way.

### NixOS

There are nix files in the root of the project, but I don't personally run NixOS, so I can't vouch for how well they work.

In theory, you should be able to run `nix build` and get a binary in `./result/bin/rash`.

You can also run rash on NixOS using the `nix run` command. For example:

```bash
nix run github:parker-macdonald/rash # download from the github mirror
nix run git+https://git.myriation.xyz/parker_macdonald/rash.git # download rash from the official repository
```

Keep in mind though using `nix run` will compile the latest changes on the main branch, which, while they should be stable, won't be as well tested as an official release.

## OpenBSD

OpenBSD comes preinstalled with `clang` as it's default C compiler, so all you have to do is install CMake from the `cmake` package.

```bash
doas pkg_add cmake
```

## FreeBSD

FreeBSD comes preinstalled with `clang` as it's default C compiler, so all you have to do is install GNU make from the `cmake` package.

```bash
sudo pkg install cmake
```

## Windows

rash relies on *nix (Linux, BSD, or anything vaguely POSIX compliant) system calls, and because of this cannot be run natively on Windows. However, you do have a few options.

### Cygwin

[Cygwin](https://cygwin.com/) is a project that works to provide *nix system calls on Windows through a custom library. Once you have Cygwin installed, you will have access to CMake and a C compiler inside the Cygwin terminal.

### WSL

[WSL (Windows Subsystem for Linux)](https://learn.microsoft.com/en-us/windows/wsl/install), allows you to run Linux applications under Windows, once you install it, you can follow the instructions for a Linux installation.

# Building

Now that you have GNU make and a C compiler, you can run `cmake -S . -B build/` in the root of the project followed by `make -C build/` to build an executable at `./build/rash`.

# Installing

Now that you have an executable in `./build/rash`, you can install it using `make install`, (or `gmake install` on BSD). Keep in mind this needs to be run as root.

By default this will install rash to `/usr/local/bin/rash`, but you can change this by running `make install PREFIX=...` with different values for PREFIX.

- `PREFIX=/usr/local` is the default, this installs rash at `/usr/local/bin/rash`.
- Some other examples might be `PREFIX=/` to install rash to `/bin/rash` or `PREFIX=/usr` to install rash to `/usr/bin/rash`.

# Using rash

Once you have rash installed, consider checking out [the user guide](USER_GUIDE.md).