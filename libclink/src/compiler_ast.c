#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include "get_environ.h"
#include "compiler_ast.h"


/// mutual exclusion mechanism for writing log
pthread_mutex_t clang_log_lock;
pthread_mutex_t clink_log_lock;

static void free_vector (char **vector)
{
  register char **scan;

  if (vector != NULL)
    {
      for (scan = vector; *scan != NULL; scan++)
        {
          free (*scan);
        }
      free (vector);
    }
}

///write log
int write_log(unsigned char *data, size_t length, const char* log, bool log_kind) {

int rc=0;
int res;

assert(data != NULL);
assert(length != 0);
assert(log != NULL);

if (log_kind)
  res = pthread_mutex_lock(&clang_log_lock);
else
  res = pthread_mutex_lock(&clink_log_lock);
  
if (res == 0) {

  // get our current directory
  char *wd = getcwd(NULL, 0);

  char *path = NULL;
  asprintf(&path, "%s/%s", wd, log);

  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

  if ( fd < 0 ) {
    rc = errno;
    goto done;
}

  ssize_t r = write(fd, data, length);
  if (r < 0 || (size_t)r != length)
    rc = errno;

  fsync(fd);
  close(fd);
}

done:
if (log_kind)
 (void)pthread_mutex_unlock(&clang_log_lock);
else
 (void)pthread_mutex_unlock(&clink_log_lock);
return rc;
}


///read stderr or stdout output from the compiler
static int read_std ( FILE *in, char **argv_clang ) {

int rc=0;
unsigned char* content = NULL;
unsigned char c;
size_t contentSize=0;


int fd = fileno (in);

  if ( fd < 0 ) {
    rc = errno;
    goto done;
}

 while ( read(fd,&c,sizeof(c)) > 0 )
 {
  content = (unsigned char*)(realloc(content, contentSize+1));
  if (content == NULL)
    {
    rc = ENOMEM;
    goto done;
    }
  content[contentSize] = c;
  contentSize++;
 }


done:
if ( contentSize ) {
  content[contentSize-1]='\0';

  // allocate memory for all the strings, plus one space per argument, plus a NUL terminator.
   size_t size = 1;
   for (int idx = 0; argv_clang[idx] != NULL; idx++) {
     size += strlen(argv_clang[idx]) + 1;
    }
   
   char *str_argv = malloc(size);
   assert (str_argv != NULL);

   size = 0;
   for (int idx = 0; argv_clang[idx] != NULL; idx++) {
     size += sprintf(str_argv + size, "%s ", argv_clang[idx]);
     assert(str_argv[size] == '\0');
    }

   str_argv[--size] = '\0';
   
   char *write_buffer = NULL;
   int write_buffer_size = asprintf(&write_buffer, "%s\n%s\n%s\n%s\n", "===========running===========", str_argv, "===========errors===========", content);
   
   rc = write_log ((unsigned char*)write_buffer, write_buffer_size, "clang-parse.log", true);

  free(write_buffer);
  free(str_argv);
  free(content);
}

return rc;
}

int clink_compiler_ast (const char *compiler, const char *filename, size_t argc,
    const char **argv, char *ast_out) {

    int rc = 0;
    size_t argc_idx;
    char **argv_clang;

  /* the vector */
  for (argc_idx = 0; argc_idx < argc + 7; argc_idx++);
  argv_clang = (char **) malloc ((argc_idx + 1) * sizeof (char *));
  
  assert(argv_clang != NULL);

  /* the strings */
  argv_clang[0]=strdup(compiler);
  argv_clang[1]=strdup("-emit-ast");
  argv_clang[2]=strdup("-w");
  argv_clang[3]=strdup("-ferror-limit=0");
  argv_clang[4]=strdup("-o");
  argv_clang[5]=strdup(ast_out); 

  if (argv == NULL) {
    argv_clang[6]=strdup(filename);
    argv_clang[7]=NULL;
  }
  else
  {
     for (argc_idx=0; argc_idx<argc; argc_idx++)
        {
         int len = strlen (argv[argc_idx]);
         argv_clang[argc_idx+6] = malloc (sizeof (char *) * (len + 1));
         assert(argv_clang[argc_idx+6] != NULL);
         strcpy (argv_clang[argc_idx+6], argv[argc_idx]);
        }
    argv_clang[argc_idx+6]=strdup(filename);
    argv_clang[argc_idx+7]=NULL;
  }

  posix_spawn_file_actions_t fa;
  int devnull = -1;
  int channel_stdout[2] = { -1, -1 };
  int channel_stderr[2] = { -1, -1 };

  FILE *child_stdout = NULL;
  FILE *child_stderr = NULL;

  if ((rc = posix_spawn_file_actions_init(&fa)))
    return rc;

  // wire the child’s stdin to /dev/null
  if ((devnull = open("/dev/null", O_RDWR)) < 0) {
    rc = errno;
    goto done;
  }

  // create a pipe for communicating with the compiler stdout
  if (pipe(channel_stdout)) {
    rc = errno;
    goto done;
  }

  // create a pipe for communicating with the compiler stderr
  if (pipe(channel_stderr)) {
    rc = errno;
    goto done;
  }

  // open the read end as a stream so we can later read on it
  if ((child_stdout = fdopen(channel_stdout[0], "r")) == NULL) {
    rc = errno;
    goto done;
  }

  // open the read end as a stream so we can later read on it
  if ((child_stderr = fdopen(channel_stderr[0], "r")) == NULL) {
    rc = errno;
    goto done;
  }

  // wire the child’s stdin to /dev/null
  if ((rc = posix_spawn_file_actions_adddup2(&fa, devnull, STDIN_FILENO)))
    goto done;

  // wire the child’s stdout to the write end of the pipe
  if ((rc = posix_spawn_file_actions_adddup2(&fa, channel_stdout[1], STDOUT_FILENO)))
    goto done;

  // wire the child’s stdout to the write end of the pipe
  if ((rc = posix_spawn_file_actions_adddup2(&fa, channel_stderr[1], STDERR_FILENO)))
    goto done;

{
    // start the child
    pid_t pid;
    char *const *args;
    args = (char*const*)argv_clang;
    if ((rc = posix_spawnp(&pid, argv_clang[0], &fa, NULL, args, get_environ())))
      goto done;

    close(channel_stderr[1]);
    close(channel_stdout[1]);

    read_std (child_stderr,argv_clang);
    read_std (child_stdout,argv_clang);

    // close end of the child’s stdout, stderr to force a SIGPIPE and exit the child
    fclose(child_stderr);
    fclose(child_stdout);

    child_stdout = NULL;
    child_stderr = NULL;

    // clean up the child
    int ignored;
    (void)waitpid(pid, &ignored, 0);

}

done:
  if (devnull != -1)
    close(devnull);
  if(child_stdout)
    fclose(child_stdout);
  if (child_stderr)
    fclose(child_stderr);
  if (channel_stdout[1] != -1)
    close(channel_stdout[1]);
  if (channel_stdout[0] != -1)
    close(channel_stdout[0]);
  if (channel_stderr[1] != -1)
    close(channel_stderr[1]);
  if (channel_stderr[0] != -1)
    close(channel_stderr[0]);

  (void)posix_spawn_file_actions_destroy(&fa);
  if (argv_clang != NULL)
    free_vector(argv_clang);

  return rc;
}
