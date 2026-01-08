# rash

rash, the rat ass shell, is a rudimentary shell written in C.

# Features

While not as featureful as other shells, rash is still very usable.

Some of rash's current features include:

- Environment variable modification
- Shell variables
- Globbing (using wildcards to match files)
- Auto Completion
- Job management
- Storing command history
- Custom prompts
- ...and much more!

# Getting Started

To build rash, take a look at [the build guide](doc/BUILD.md).

Once you have built rash, feel free to take a look at [the user guide](doc/USER_GUIDE.md).

# Setting up clangd

To setup clangd for this project, you will need to generate a `compile_commands.json` file.

For this, I use [`bear`](github.com/rizsotto/Bear), but you can use other tools if you're more familiar with them.

```bash
make clean # make sure no build files exist so that bear knows how to compile all source files
bear -- make # this will generate a compile_commands.json in the root of the project
```

You can also specify make flags here, for example:

```bash
bear -- make ERROR_HELL=1 # this will generate a compile_commands.json with all errors enabled (not recommended for development)
```

Once you have a `compile_commands.json` file, clangd should work fine.