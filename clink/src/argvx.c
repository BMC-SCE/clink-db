#include "argvx.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef NULL
#define NULL 0
#endif

#ifndef EOS
#define EOS '\0'
#endif

#define INITIAL_MAXARGC 8

static void free_argv (vector)
char **vector;
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

static char **build_argv (input)
char *input;
{
  char *arg;
  char *copybuf;
  int squote = 0;
  int dquote = 0;
  int bsquote = 0;
  int argc = 0;
  int maxargc = 0;
  char **argv = NULL;
  char **nargv;

  if (input != NULL)
    {
      copybuf = (char *) alloca (strlen (input) + 1);
      /* Is a do{}while to always execute the loop once.  Always return an
         argv, even for null strings.  See NOTES above, test case below. */
      do
        {
          /* Pick off argv[argc] */
          while (isspace (*input))
            {
              input++;
            }
          if ((maxargc == 0) || (argc >= (maxargc - 1)))
            {
              /* argv needs initialization, or expansion */
              if (argv == NULL)
                {
                  maxargc = INITIAL_MAXARGC;
                  nargv = (char **) malloc (maxargc * sizeof (char *));
                }
              else
                {
                  maxargc *= 2;
                  nargv = (char **) realloc (argv, maxargc * sizeof (char *));
                }
              if (nargv == NULL)
                {
                  if (argv != NULL)
                    {
                      free_argv (argv);
                      argv = NULL;
                    }
                  break;
                }
              argv = nargv;
              argv[argc] = NULL;
            }
          /* Begin scanning arg */
          arg = copybuf;
          while (*input != EOS)
            {
              if (isspace (*input) && !squote && !dquote && !bsquote)
                {
                  break;
                }
              else
                {
                  if (bsquote)
                    {
                      bsquote = 0;
                      *arg++ = *input;
                    }
                  else if (*input == '\\')
                    {
                      bsquote = 1;
                    }
                  else if (squote)
                    {
                      if (*input == '\'')
                        {
                          squote = 0;
                        }
                      else
                        {
                          *arg++ = *input;
                        }
                    }
                  else if (dquote)
                    {
                      if (*input == '"')
                        {
                          dquote = 0;
                        }
                      else
                        {
                          *arg++ = *input;
                        }
                    }
                  else
                    {
                      if (*input == '\'')
                        {
                          squote = 1;
                        }
                      else if (*input == '"')
                        {
                          dquote = 1;
                        }
                      else
                        {
                          *arg++ = *input;
                        }
                    }
                  input++;
                }
            }
          *arg = EOS;
          argv[argc] = strdup (copybuf);
          if (argv[argc] == NULL)
            {
              free_argv (argv);
              argv = NULL;
              break;
            }
          argc++;
          argv[argc] = NULL;

          while (isspace (*input))
            {
              input++;
            }
        }
      while (*input != EOS);
    }
  return (argv);
}

static void join_argv (dst_p, src_p, clen)
char ***dst_p;
char ***src_p;
size_t *clen;
{
  size_t src_len = 0;
  size_t dst_len = 0;
  size_t idx;

  assert(*src_p != NULL);
  for (src_len = 0; *(*src_p+src_len) != NULL; src_len++);
  
  if (*dst_p !=NULL)
  for (dst_len = 0; *(*dst_p+dst_len) != NULL; dst_len++);

  *dst_p=(char **) realloc (*dst_p, (src_len + dst_len + 1) * sizeof (**dst_p));
  assert(*dst_p != NULL);

  /* the strings */
  for (idx = 0; *(*src_p+idx) != NULL; idx++)
     {
     size_t len = strlen (*(*src_p+idx));
     *(*dst_p+dst_len+idx) = malloc (sizeof (**dst_p) * (len + 1));
     assert (*(*dst_p+dst_len+idx) != NULL);
     strcpy (*(*dst_p+dst_len+idx), *(*src_p+idx));
     }
  *(*dst_p+dst_len+idx) = NULL;
  *clen=dst_len+idx;
}

int build_argv_clang (char *filename, char ***argvx_p, size_t *argcx)
{
  int rc = 0;
  char *line = NULL;
  char **line_argv = NULL;
  size_t line_size = 0;
  ssize_t read_length = 0;
  FILE *in = NULL;

  if ( access(filename, R_OK) < 0 ) {
    fprintf(stderr, "unable to read %s\n", filename);
    exit(EXIT_FAILURE);
  }

  in = fopen(filename, "r");
  assert (in != NULL);

  for (;;) {

    if ((read_length = getline(&line, &line_size, in)) < 0) {
      rc = errno;
    if (rc == 0)
      rc = ENOTSUP;
    goto done;
  }
  if ( line[read_length-1] == '\n' ) {
    line[read_length-1]=0;
    read_length--;
  }

  line_argv = build_argv(line);
  join_argv (argvx_p, &line_argv, argcx);
}


done:
if (in)
 fclose(in);
free(line);
free_argv (line_argv);
return rc;
}

