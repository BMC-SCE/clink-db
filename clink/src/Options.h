#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

typedef enum {
  UI_NONE,
  UI_CURSES,
  UI_LINE,
} ui_t;

struct Options {

  // path to database if it was set on the command line
  std::optional<std::filesystem::path> database_path;

  bool update_database;
  ui_t ui;

  // Parallelism (0 == auto).
  unsigned long threads;

  // Directories to look in for #include files
  std::vector<std::string> include_dirs;
};

extern Options options;
