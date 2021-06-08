#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "temp_file.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

bool is_fileRWNZ(const char *path) {

  if (path == NULL)
    return false;

  struct stat buf;
  if (stat(path, &buf) < 0)
    return false;

  return (S_IRUSR & buf.st_mode) && (S_IWUSR & buf.st_mode) && (buf.st_size > 0);
}


int temp_file(char **tfile, const char *filename) {

  // check where the environment wants temporary files to go
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = access ( "/dev/shm", W_OK ) == 0 ? "/dev/shm" : "/tmp"; 

  char *path = strdup (filename);
  char *file = basename (path);

  char *temp;
  if (asprintf(&temp, "%s/%s-ast-XXXXXX", TMPDIR, file) < 0)
    return errno;

  if (mkstemp(temp) == -1) {
    int rc = errno;
    free(temp);
    return rc;
  }

  *tfile = temp;
  free(path);
  return 0;
}

