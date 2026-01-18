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
$ rash -c "setvar a 'hello world!'; echo {a}"
hello world!
```

# Features

## Shell Variables

In rash, shell variables are declared with the `setvar` command, and retrived by putting the variable name in braces {}. For example:

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
$ rash -c "exit 69" # use one-shot mode to run a process with an exit code of 69
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

Environment variables are retrived with `$` syntax (just like in bash), and can be declared using the `setenv` or `export` commands.

```bash
setenv A hello
# or
export B=world

echo $A $B # prints hello world
echo ${A} ${B} # also prints hello world
```

The main difference between putting braces around the name of the variable, it that you can access variables that have more than just letters, numbers, and underscores in the.

```bash
setenv 'what!?' value
echo $what!? # rash: environment variable ‘what’ does not exist.
echo ${what!?} # value
```

### Subshells

You can run commands inside of another other commands using something called subshells.

Lets say you want to do something with the output of a command, but don't want that command to change something about rash (like setting an environment variable).

You could write something like this:

```bash
echo $(command_that_sets_vars)
```

The result of `command_that_sets_vars` gets set as the first argument to echo, so if `command_that_sets_vars` prints `hello world`, `hello world` ends up getting printed.

Another useful thing you can do with subshells is run a command with an set environment variable without changing environment variables of the current shell.

```bash
echo $(export a=5; echo $a) # 5
echo $a # rash: environment variable ‘a’ does not exist.
```

Another thing worth noting is that control characters get stripped out of the output, so for example:

```bash
echo $(echo -e 'line 1\nline2') # line1line2
```

Since `\n` is a control character, it is not present in the final output.

Another another thing worth noting is that the result of the command in the subshell is only used in one argument, for example:

```bash
echo contents > 'file with space.txt'

cat $(echo 'file with space.txt') # prints: 'contents'
# is equivalent to:
cat 'file with space.txt' # also prints: 'contents'
# and is NOT equivalent to:
cat file with space.txt # cannot find 'file', 'with', or 'space.txt'
```