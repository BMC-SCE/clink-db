#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "temp_file.h"
#include <unistd.h>
#include <string.h>

int exist_X ( const char* file ) {
    int rc = access ( file, F_OK | X_OK ) != -1 ? 1 : 0;
    return rc;
}

int findX_in_PATH ( const char* name, char* fullpath, size_t sz ) {
    int rc = 0;
    char* paths = strdup ( getenv ("PATH") );
    char* ff = paths;
    const char* item;
    while ( (item=strsep(&paths,":")) != NULL ){
            snprintf(fullpath,sz,"%s/%s", item, name);
            if ( exist_X(fullpath) ) { 
              rc = 1;
              break;
            }
            else
              memset(fullpath,0,sz);
    }
    free(ff);
    return rc;
}

int temp_file(char **tfile) {

  // check where the environment wants temporary files to go
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = access ( "/dev/shm", W_OK ) == 0 ? "/dev/shm" : "/tmp"; 

  char *temp;
  if (asprintf(&temp, "%s/clink-db-ast.XXXXXX", TMPDIR) < 0)
    return errno;

  if (mkstemp(temp) == -1) {
    int rc = errno;
    free(temp);
    return rc;
  }

  *tfile = temp;
  return 0;
}



