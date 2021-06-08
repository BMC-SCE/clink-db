#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "path.h"
#include <magic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int cwd(char **wd) {

  if (wd == NULL)
    return EINVAL;

  char *w = getcwd(NULL, 0);
  if (w == NULL)
    return errno;

  *wd = w;
  return 0;
}

/// does this path have the given file extension?
static bool has_ext(const char *path, const char *ext) {

  if (path == NULL || ext == NULL)
    return false;

  if (strlen(path) < strlen(ext) + strlen("."))
    return false;

  // does the extension begin where we expect?
  if (path[strlen(path) - strlen(ext) - strlen(".")] != '.')
    return false;

  // case-insensitive comparison of the extension
  return strcasecmp(path + strlen(path) - strlen(ext), ext) == 0;
}

bool is_asm(const char *filename) {
     if ( has_ext(filename, "s")
      || has_ext(filename, "asm") )
       return true;
    
    magic_t magic_cookie=magic_open(MAGIC_NONE);
    assert(magic_cookie != NULL);

    int rc=magic_load(magic_cookie, NULL);
    assert(rc == 0);

    char *magic_full=strdup(magic_file(magic_cookie, filename));
    assert(magic_full != NULL);

    magic_close(magic_cookie);

    bool rc_ret = (strncasecmp("assembler source", magic_full, strlen("assembler source")) == 0) ? true : false;
    free(magic_full);
    return rc_ret;
}

bool is_c(const char *filename, bool all_files) {
     if (all_files)
       return true;

     if ( has_ext(filename, "c")
      || has_ext(filename, "h") )
       return true;
    
    magic_t magic_cookie=magic_open(MAGIC_NONE);
    assert(magic_cookie != NULL);

    int rc=magic_load(magic_cookie, NULL);
    assert(rc == 0);

    char *magic_full=strdup(magic_file(magic_cookie, filename));
    assert(magic_full != NULL);

    magic_close(magic_cookie);

    bool rc_ret = (strncasecmp("C source", magic_full, strlen("C source")) == 0) ? true : false;
    free(magic_full);
    return rc_ret;
}


bool is_cxx (const char* filename, bool all_files) {
     if (all_files)
       return true;

    if ( has_ext(filename, "c++")
      || has_ext(filename, "cpp")
      || has_ext(filename, "cxx")
      || has_ext(filename, "cc")
      || has_ext(filename, "hpp") )
      return true;
    
    magic_t magic_cookie=magic_open(MAGIC_NONE);
    assert(magic_cookie != NULL);

    int rc=magic_load(magic_cookie, NULL);
    assert(rc == 0);

    char *magic_full=strdup(magic_file(magic_cookie, filename));
    assert(magic_full != NULL);

    magic_close(magic_cookie);

    bool rc_ret = (strncasecmp("C++ source", magic_full, strlen("C++ source")) == 0) ? true : false;
    free(magic_full);
    return rc_ret;
}

bool is_dir(const char *path) {

  if (path == NULL)
    return false;

  struct stat buf;
  if (stat(path, &buf) < 0)
    return false;

  return S_ISDIR(buf.st_mode);
}

bool is_file(const char *path) {

  if (path == NULL)
    return false;

  struct stat buf;
  if (stat(path, &buf) < 0)
    return false;

  return S_ISREG(buf.st_mode);
}

static int exist_X ( const char* file ) {
    int rc = access ( file, F_OK | X_OK ) != -1 ? 1 : 0;
    return rc;
}

bool findX_in_PATH ( const char* name) {
    bool rc = false;
    char path[PATH_MAX]={0};
    char* paths = strdup ( getenv ("PATH") );
    char* ff = paths;
    const char* item;
    while ( (item=strsep(&paths,":")) != NULL ){
            snprintf(path,PATH_MAX,"%s/%s", item, name);
            if ( exist_X(path) ) {
              rc = true;
              break;
            }
            else
              rc = false;
    }
    free(ff);
    return rc;
}


