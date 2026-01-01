# rash User Guide

This guide, while not super in depth, shows you how to use rash, and some of it's differences compared to other shells.

# Operating Modes

rash has different operating modes based off the parameters you run it with.

## Interactive Mode

When you run rash with no arguments, you enter interactive mode.

You can type out commands and run them just as with other shells.

### Prompts

Like most shells you can set a custom prompt, but in rash this is accomplished with the PS1 shell variable instead of the environment variable.

For example:

```bash
setvar PS1 '\u@\h:\w\$ ' # sets prompt to user@hostname:/path/to/working/dir$
```

### Line Expansion

If you want to expand the line you are currently editing, you can press Shift+Tab. This expands globs, shell variables, and environment variables.

For example:

```bash
$ setvar a hello!
$ ls
file_a file_b file_c
$ echo {a} file_* # Shift+Tab
$ echo hello! file_a file_b file_c
```

### .rashrc file

If you have a `.rashrc` file in the root of your HOME folder, it will get executed when you start up rash in interactive mode (and no other mode!).

For example:

```bash
$ cat ~/.rashrc
setvar PS1 '\u@\h:\w\$ '
$ rash
user@hostname:~$ 
```

## Scripting Mode

When you run rash with one argument, you enter scripting mode, which is used to run commands from files.

While rash is not designed to be used for shell scripting, you still can run commands inside files.

For example:

```bash
$ cat example.rash
setvar a hello
setvar b world

echo {a} {b} # prints hello world
$ rash example.rash
hello world
```

Most shell scripting functionality, such as loops, if blocks, and case statements is not present in rash. While they might be added in the future, I wouldn't get my hopes up.

It's also worth noting, that unlike other shells, pressing Ctrl+C while executing a shell script will exit out of the entire shell script, not just the current command.

## One-Shot Mode

When you run rash with the `-c` option, rash is run in one-shot mode. This is used to run a single command then exit.

For example:

```bash
$ rash -c "echo hello world!"
hello world!
```

You can also can use a semicolon to run multiple commands, for example:

```bash
$ rash -c "echo hello; echo goodbye"
hello
goodbye
```

# Features

## Shell Variables

In rash, shell variables are declared with the `setvar` command, and retrived with braces (`{}`) syntax. For example:

```bash
setvar key value
echo {key} # prints value
```

To unset a shell variable, you use the `unsetvar` command, and you can list all declared shell variables with the `var` command.

For example:

```bash
$ setvar key value
$ setvar key2 value2
$ var
{?}:    "0"
{key}:  "value"
{key2}: "value2"
$ unsetvar key
$ var
{?}:    "0"
{key2}: "value2"
```

You might be wondering, what the `?` is doing in the list of shell variables. It contains the exit code of the last program to run. (For pipelines this is the exit code of the last program in the pipeline).

For example:

```bash
$ rash # creates a second instance of rash inside the first
$ exit 69
$ echo {?}
69
$ false
$ echo {?}
1
$ true
$ echo {?}
0
$ true | true | false # this is a pipeline
$ echo {?}
1
```

## Environment Variables

Environment variables are retrived with `$` syntax (just like in bash), and can also be declared using the `setenv` or `export` commands.

```bash
setenv A hello
export B=world

echo $A $B # prints hello world
echo ${A} ${B} # also prints hello world
```