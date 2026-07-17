cmake_minimum_required(VERSION 3.5)

add_compile_definitions(VERSION=0.7.9)

# Set the project name
project (rash)

# Create a sources variable with a link to all cpp files to compile
set(src_lib
  src/lib/dynamic_sprintf.c
  src/lib/sys.c
  src/lib/next_pow_2.c
  src/lib/error.c
  src/lib/buffer.c
  src/lib/sort.c
  src/lib/utf_8.c
  src/lib/hash_map.c
  src/lib/search_path.c
  src/lib/parse.c
)

set(src_builtins
  src/builtins/find_builtin.c
  src/builtins/utils.c
  src/builtins/commands/setenv.c
  src/builtins/commands/history.c
  src/builtins/commands/version.c
  src/builtins/commands/unsetvar.c
  src/builtins/commands/setvar.c
  src/builtins/commands/help.c
  src/builtins/commands/eval.c
  src/builtins/commands/exit.c
  src/builtins/commands/export.c
  src/builtins/commands/jobs.c
  src/builtins/commands/which.c
  src/builtins/commands/env.c
  src/builtins/commands/true.c
  src/builtins/commands/cd.c
  src/builtins/commands/time.c
  src/builtins/commands/pwd.c
  src/builtins/commands/bg.c
  src/builtins/commands/exec.c
  src/builtins/commands/fg.c
  src/builtins/commands/false.c
  src/builtins/commands/source.c
  src/builtins/commands/var.c
  src/builtins/commands/unsetenv.c
)

set(src_interpreter
  src/interpreter/execute.c
  src/interpreter/repl.c
  src/interpreter/evaluate.c
  src/interpreter/lex.c
  src/interpreter/glob.c
)

set(src_line_reader
  src/line_reader/auto_complete.c
  src/line_reader/raw_mode.c
  src/line_reader/history.c
  src/line_reader/draw.c
  src/line_reader/actions.c
  src/line_reader/action_utils.c
  src/line_reader/line_reader.c
  src/line_reader/prompt.c
  src/line_reader/actions_all.c
)

set(src_shell_vars
  src/shell_vars/shell_vars.c
  src/shell_vars/util.c
  src/shell_vars/eval.c
  src/shell_vars/token.c
  src/shell_vars/lexer.c
)

set(src_strings
  src/strings/version.c
)

set(src
  ${src_lib}
  ${src_builtins}
  ${src_interpreter}
  ${src_line_reader}
  ${src_shell_vars}
  ${src_strings}
  src/shlvl.c
  src/jobs.c
  src/rashrc.c
  src/file_reader.c
  src/main.c
)

# Add an executable with the above sources
add_executable(rash ${src})

target_include_directories(rash PRIVATE src/)
target_link_libraries(rash m)
target_compile_options(rash PRIVATE -Werror -Wall -Wextra -Wvla -Wunreachable-code -Wshadow -Wpedantic -Wconversion)
set_property(TARGET rash PROPERTY C_STANDARD 17)
set_property(TARGET rash PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET rash PROPERTY C_EXTENSIONS OFF)
add_compile_definitions(_DEFAULT_SOURCE)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories(rash
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)