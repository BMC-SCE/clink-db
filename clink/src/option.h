#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  AUTO,
  ALWAYS,
  NEVER,
} colour_t;

typedef struct {

  // path to database if it was set on the command line
  char *database_path;

  // source files/directories to scan
  char **src;
  size_t src_len;

  // update the Clink symbol database with latest source file changes?
  bool update_database;

  // exclude symbols form default searching path (c++ -E -x c++ - -v < /dev/null)
  bool exclude_sdsp; 

  // full ast through clang/clang++
  bool full_ast;

  // all files are treated as C/C++
  bool all_files;

  // skip vim db
  bool skip_vim;

  // run the NCurses-based interface?
  bool ncurses_ui;

  // run the line-oriented interface?
  bool line_ui;

  // parallelism (0 == auto)
  unsigned long threads;

  // colour terminal output on or off
  colour_t colour;

  // arguments to pass to clang++ when parsing C/C++
  char **cxx_argv;
  size_t cxx_argc;

  // arguments to pass to clang++ when parsing C/C++
  char **c_argv;
  size_t c_argc;

  // debug mode
  bool debug;

} option_t;

extern option_t option;

// setup option.database_path after option parsing
int set_db_path(void);

// setup option.src after option parsing
int set_src(void);

// deallocate members of option
void clean_up_options(void);
