#include <assert.h>
#include "build.h"
#include <clink/clink.h>
#include <errno.h>
#include <limits.h>
#include "option.h"
#include "path.h"
#include <pthread.h>
#include <regex.h>
#include "sigint.h"
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "work_queue.h"

/// mutual exclusion mechanism for writing to the database
pthread_mutex_t db_lock;

/// add a symbol to the Clink database
static int add_symbol(clink_db_t *db, const clink_symbol_t *symbol) {

  int rc = pthread_mutex_lock(&db_lock);
  if (rc)
    return rc;

  rc = clink_db_add_symbol(db, symbol);

  (void)pthread_mutex_unlock(&db_lock);

  return rc;
}

/// add a content line to the Clink database
static int add_line(clink_db_t *db, const char *path, unsigned long lineno,
    const char *line) {

  int rc = pthread_mutex_lock(&db_lock);
  if (rc)
    return rc;

  rc = clink_db_add_line(db, path, lineno, line);

  (void)pthread_mutex_unlock(&db_lock);

  return rc;
}

/// mutual exclusion mechanism for using stdout/stderr
pthread_mutex_t print_lock;

/// Is stdout a tty? Initialised in build().
static bool tty;

/// use ANSI codes to move the cursor around to generate smoother progress
/// output?
static bool smart_progress(void) {

  // do not play ANSI tricks if we are debugging
  if (option.debug)
    return false;

  // also do not do it if we are piped into something else
  if (!tty)
    return false;

  // also not if we are using the line-oriented interface because we assume we
  // are being called by Vim that does not expect this progress output
  if (option.line_ui)
    return false;

  return true;
}

/// print progress indication
__attribute__((format(printf, 2, 3)))
static void progress(unsigned long thread_id, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = pthread_mutex_lock(&print_lock);
  if (r == 0) {

    // move up to this thread’s progress line
    if (smart_progress())
      printf("\033[%luA\033[K", option.threads - thread_id);

    printf("%lu: ", thread_id);
    vprintf(fmt, ap);
    printf("\n");

    // move back to the bottom
    if (smart_progress() && thread_id != option.threads - 1) {
      printf("\033[%luB", option.threads - thread_id - 1);
      fflush(stdout);
    }

    (void)pthread_mutex_unlock(&print_lock);
  }
  va_end(ap);
}

/// print an error message
__attribute__((format(printf, 2, 3)))
static void error(unsigned long thread_id, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = pthread_mutex_lock(&print_lock);
  if (r == 0) {
    if (smart_progress())
      printf("\033[%luA\033[K", option.threads - thread_id);
    printf("%lu: ", thread_id);
    if (option.colour == ALWAYS)
      printf("\033[31m"); // red
    vprintf(fmt, ap);
    if (option.colour == ALWAYS)
      printf("\033[0m"); // reset
    printf("\n");
    if (smart_progress() && thread_id != option.threads - 1) {
      printf("\033[%luB", option.threads - thread_id - 1);
      fflush(stdout);
    }
    (void)pthread_mutex_unlock(&print_lock);
  }
  va_end(ap);
}

/// Debug printf. This is implemented as a macro to avoid expensive varargs
/// handling when we are not in debug mode.
#define DEBUG(args...) \
  do { \
    if (option.debug) { \
      progress(thread_id, args); \
    } \
  } while (0)

///  compile a regular expression from compiler default search path
static int compile_ipath (regex_t *rp) {
  int rc = 0;
  char **includes = NULL;
  size_t includes_len = 0;
  const char *compiler = NULL;
  
  rc = clink_compiler_includes(compiler, &includes, &includes_len);

  if (rc) {
    fprintf(stderr, "clink_compiler_includes: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  char *regex_p=(char*)calloc(includes_len*PATH_MAX,1);
  assert (regex_p != NULL);

  char* resolved_path=NULL;
  for (size_t i = 0; i < includes_len; ++i){
     if ( i == includes_len - 1 ) {
          resolved_path=realpath(includes[i],NULL);
	  if (resolved_path == NULL) { rc = errno;  fprintf(stderr, "failed to make %s absolute: %s\n", includes[i], strerror(rc)); return rc; }
	  strncat (regex_p, resolved_path, strlen(resolved_path));
	  free(resolved_path);
     }
     else {
	  resolved_path=realpath(includes[i],NULL);
          if (resolved_path == NULL) { rc = errno;  fprintf(stderr, "failed to make %s absolute: %s\n", includes[i], strerror(rc)); return rc; }
          strncat (regex_p, resolved_path, strlen(resolved_path));
	  strcat  (regex_p, "|");
	  free(resolved_path);
      }
   }

      rc = regcomp(rp, regex_p, REG_EXTENDED|REG_NOSUB);
      if ( rc ) { rc = errno;  fprintf(stderr, "failed to compile regex expression %s: %s\n", regex_p, strerror(rc)); }

      free(regex_p);
      return rc;
}

///  compile a regular expression from source path
static int compile_spath (regex_t *rp) {
  int rc = 0;

  char *regex_p=(char*)calloc(option.src_len*PATH_MAX,1);
  assert (regex_p != NULL);

  char* resolved_path=NULL;
  for (size_t i = 0; i < option.src_len; ++i) {
     if ( i == option.src_len - 1 ) {
      resolved_path=realpath(option.src[i],NULL);
      if (resolved_path == NULL) { rc = errno;  fprintf(stderr, "failed to make %s absolute: %s\n", option.src[i], strerror(rc)); return rc; }
      strncat (regex_p, resolved_path, strlen(resolved_path));
      free(resolved_path);
     }
     else {
      resolved_path=realpath(option.src[i],NULL);
      if (resolved_path == NULL) { rc = errno;  fprintf(stderr, "failed to make %s absolute: %s\n", option.src[i], strerror(rc)); return rc; }
      strncat (regex_p, resolved_path, strlen(resolved_path));
      strcat  (regex_p, "|");
      free(resolved_path);
     }
  }
      
      rc = regcomp(rp, regex_p, REG_EXTENDED|REG_NOSUB);
      if ( rc ) { rc = errno;  fprintf(stderr, "failed to compile regex expression %s: %s\n", regex_p, strerror(rc)); }

      free(regex_p);
      return rc;
}

/// drain a work queue, processing its entries into the database
static int process(unsigned long thread_id, pthread_t *threads, clink_db_t *db,
    work_queue_t *wq) {

  assert(db != NULL);
  assert(wq != NULL);

  int rc = 0;

  regex_t rpi = {0};
  regex_t rps = {0};

  if (option.exclude_sdsp) {
  compile_ipath (&rpi);
  compile_spath (&rps);
  }

  for (;;) {

    // get an item from the work queue
    task_t t;
    rc = work_queue_pop(wq, &t);

    // if we have exhausted the work queue, we are done
    if (rc == ENOMSG) {
      progress(thread_id, "finishing...");
      rc = 0;
      break;
    }

    if (rc) {
      error(thread_id, "failed to pop work queue: %s", strerror(rc));
      break;
    }

    assert(t.path != NULL);

    // see if we know of this file
    {
      uint64_t timestamp = 0;
      int r = clink_db_find_record(db, t.path, NULL, &timestamp);
      if (r == 0) {
        // stat the file to see if it has changed
        uint64_t timestamp2;
        r = mtime(t.path, &timestamp2);
        // if it has not changed since last update, skip it
        if (r == 0 && timestamp == timestamp2) {
          DEBUG("skipping unmodified file %s", t.path);
          free(t.path);
          continue;
        }
      }
    }

    // generate a friendlier name for the source path
    char *display = NULL;
    if ((rc = disppath(t.path, &display))) {
      free(t.path);
      error(thread_id, "failed to make %s relative: %s", t.path, strerror(rc));
      break;
    }

    switch (t.type) {

      // a file to be parsed
      case PARSE: {

        // remove anything related to the file we are about to parse
        clink_db_remove(db, t.path);

        // enqueue this file for reading, as we know we will need its contents
        if ((rc = work_queue_push_for_read(wq, t.path))) {
          error(thread_id, "failed to queue %s for reading: %s", display,
            strerror(rc));
          break;
        }

        clink_iter_t *it = NULL;

        // assembly
        if (is_asm(t.path)) {
          progress(thread_id, "parsing asm file %s", display);
          rc = clink_parse_asm(&it, t.path);

        // C/C++
        } else {
          assert(is_c(t.path,option.all_files) || is_cxx(t.path,option.all_files));
          progress(thread_id, "parsing C/C++ file %s", display);

          char *compiler = getenv("CLINK_CC");
	  bool free_compiler = false;
	  const char **argv = NULL;
          size_t argc = 0;

	  if ( ( compiler != NULL ) && ( strncmp("clang", compiler, strlen("clang")) != 0 ) && ( strncmp("clang++", compiler, strlen("clang++")) != 0 ) )
           compiler = NULL;
          
	  if ( compiler == NULL) {
           if ( is_cxx(t.path, option.all_files) ) {
            compiler = strdup("clang++");
	    free_compiler = true;
	   }
           else {
            compiler = strdup("clang");
	    free_compiler = true;
          }
         }

	  if(strcmp("clang++", compiler)==0){
            argv = (const char**)option.cxx_argv;
	    argc = option.cxx_argc;
	  }
	  else {
            argv = (const char**)option.c_argv;
            argc = option.c_argc;
	  }
	  if (option.full_ast)
            rc = clink_parse_c_f(&it, t.path, compiler, argc, argv);
	  else
            rc = clink_parse_c_l(&it, t.path, "libclang", argc, argv);
          
	  /// the second attempt if the first fails
          if (rc && option.full_ast && (strcmp("clang", compiler)==0)) {
           compiler = strdup("clang++");
	   free_compiler = true;
	   argv = (const char**)option.cxx_argv;
           argc = option.cxx_argc;
           rc = clink_parse_c_f(&it, t.path, compiler, argc, argv);
	  } else if (rc && option.full_ast && (strcmp("clang++", compiler)==0)) {
           compiler = strdup("clang");
	   free_compiler = true;
           argv = (const char**)option.c_argv;
           argc = option.c_argc;
           rc = clink_parse_c_f(&it, t.path, compiler, argc, argv);
	  }
          
          // the third atempt if second fails, the last desperate attempt
	  if (rc && option.full_ast) {
           compiler = strdup("libclang");
           free_compiler = true;
	   argv = (const char**)option.cxx_argv;
           argc = option.cxx_argc;
           rc = clink_parse_c_l(&it, t.path, compiler, argc, argv);
           if (rc) {
           argv = (const char**)option.c_argv;
           argc = option.c_argc;
           rc = clink_parse_c_l(&it, t.path, compiler, argc, argv);
	   }
	  }

          if(free_compiler)
  	   free(compiler);
        }
        
	if (rc) {
          free(t.path);
          error(thread_id, "clang/clang++ and libclang failed to parse %s: %s", display, strerror(rc));
	  continue;
	}

        if (rc == 0) {
          // parse all symbols and add them to the database
          while (clink_iter_has_next(it)) {

            const clink_symbol_t *symbol = NULL;
            if ((rc = clink_iter_next_symbol(it, &symbol)))
              break;
            assert(symbol != NULL);

            DEBUG("adding symbol %s:%lu:%lu:%s:%u:%u", symbol->path, symbol->lineno, symbol->colno, symbol->name, symbol->category, symbol->cx_category);

            if (option.exclude_sdsp){	      
            char* resolved_path=realpath(symbol->path,NULL);
              if (resolved_path == NULL)
	        error(thread_id, "failed to make %s absolute: %s\n", symbol->path, strerror(errno));
              if ( ( regexec(&rpi, resolved_path, (size_t) 0, NULL, 0) == 0 ) || ( regexec(&rps, resolved_path, (size_t) 0, NULL, 0) != 0 ) ){            
                DEBUG("excluding symbol %s:%lu:%lu:%s:%u:%u", symbol->path, symbol->lineno, symbol->colno, symbol->name, symbol->category, symbol->cx_category);
		free(resolved_path);
	        continue;
	      }
	    free(resolved_path);
	    }

            if ((rc = add_symbol(db, symbol)))
              break;
          }
        }

        clink_iter_free(&it);

        if (rc)
          error(thread_id, "failed to parse %s: %s", display, strerror(rc));

         break;
      }

      // a file to be read and syntax highlighted
      case READ: {
	if (option.skip_vim) break;
        progress(thread_id, "syntax highlighting %s", display);
        clink_iter_t *it = NULL;
        rc = clink_vim_highlight(&it, t.path);

        if (rc == 0) {
          // retrieve all lines and add them to the database
          unsigned long lineno = 1;
          while (clink_iter_has_next(it)) {

            const char *line = NULL;
            if ((rc = clink_iter_next_str(it, &line)))
              break;

            if ((rc = add_line(db, t.path, lineno, line)))
              break;

            ++lineno;
          }
        }

        clink_iter_free(&it);

        if (rc) {

          // If the user hit Ctrl+C, Vim may have been SIGINTed causing it to fail
          // cryptically. If it looks like this happened, give the user a less
          // confusing message.
          if (sigint_pending()) {
            error(thread_id, "failed to read %s: received SIGINT", display);

          } else {
            error(thread_id, "failed to read %s: %s", display, strerror(rc));
          }
        }

        // now we can insert a record for the file
        if (rc == 0) {
          uint64_t hash = 0; // TODO
          uint64_t timestamp;
          if (mtime(t.path, &timestamp) == 0)
            (void)clink_db_add_record(db, t.path, hash, timestamp);
        }

        break;
      }

    }

    free(display);
    free(t.path);

    if (rc)
      break;

    // check if we have been SIGINTed and should finish up
    if (sigint_pending()) {
      progress(thread_id, "saw SIGINT; exiting...");
      break;
    }
  }

  // Signals are delivered to one arbitrary thread in a multithreaded process.
  // So if we saw a SIGINT, signal the thread before us so that it cascades and
  // is eventually propagated to all threads.
  if (sigint_pending()) {
    unsigned long previous = (thread_id == 0 ? option.threads : thread_id) - 1;
    if (previous != thread_id)
      (void)pthread_kill(threads[previous], SIGINT);
  }

  return rc;
}

// a vehicle for passing data to process()
typedef struct {
  unsigned long thread_id;
  pthread_t *threads;
  clink_db_t *db;
  work_queue_t *wq;
} process_args_t;

// trampoline for unpacking the calling convention used by pthreads
static void *process_entry(void *args) {

  // unpack our arguments
  const process_args_t *a = args;
  unsigned long thread_id = a->thread_id;
  pthread_t *threads = a->threads;
  clink_db_t *db = a->db;
  work_queue_t *wq = a->wq;

  int rc = process(thread_id, threads, db, wq);

  return (void*)(intptr_t)rc;
}

// call process() multi-threaded
static int mt_process(clink_db_t *db, work_queue_t *wq) {

  // the total threads is ourselves plus all the others
  assert(option.threads >= 1);
  size_t bg_threads = option.threads - 1;

  // create threads
  pthread_t *threads = calloc(option.threads, sizeof(threads[0]));
  if (threads == NULL)
    return ENOMEM;

  // create state for them
  process_args_t *args = calloc(bg_threads, sizeof(args[0]));
  if (args == NULL) {
    free(threads);
    return ENOMEM;
  }

  // set up data for all threads
  for (size_t i = 1; i < option.threads; ++i)
    args[i - 1] = (process_args_t){
      .thread_id = i, .threads = threads, .db = db, .wq = wq };

  // start all threads
  size_t started = 0;
  for (size_t i = 0; i < option.threads; ++i) {
    if (i == 0) {
      threads[i] = pthread_self();
    } else {
      if (pthread_create(&threads[i], NULL, process_entry, &args[i - 1]) != 0)
        break;
    }
    started = i + 1;
  }

  // join in helping with the rest
  int rc = process(0, threads, db, wq);

  // collect other threads
  for (size_t i = 0; i < started; ++i) {

    // skip ourselves
    if (i == 0)
      continue;

    void *ret;
    int r = pthread_join(threads[i], &ret);

    // none of the pthread failure reasons should be possible
    assert(r == 0);
    (void)r;

    if (ret != NULL && rc == 0)
      rc = (int)(intptr_t)ret;
  }

  // clean up memory
  free(args);
  free(threads);

  return rc;
}

int build(clink_db_t *db) {

  assert(db != NULL);

  tty = isatty(STDOUT_FILENO);

  int rc = 0;

  // create a mutex for protecting database accesses
  if ((rc = pthread_mutex_init(&db_lock, NULL))) {
    fprintf(stderr, "failed to create mutex: %s\n", strerror(rc));
    return rc;
  }

  // create a mutex for protecting printf and friends
  if ((rc = pthread_mutex_init(&print_lock, NULL))) {
    fprintf(stderr, "failed to create mutex: %s\n", strerror(rc));
    (void)pthread_mutex_destroy(&db_lock);
    return rc;
  }

  // setup a work queue to manage our tasks
  work_queue_t *wq = NULL;
  if ((rc = work_queue_new(&wq))) {
    fprintf(stderr, "failed to create work queue: %s\n", strerror(rc));
    goto done;
  }

  // add our source paths to the work queue
  for (size_t i = 0; i < option.src_len; ++i) {
    rc = work_queue_push_for_parse(wq, option.src[i]);

    // ignore duplicate paths
    if (rc == EALREADY)
      rc = 0;

    if (rc) {
      fprintf(stderr, "failed to add %s to work queue: %s\n", option.src[i],
        strerror(rc));
      goto done;
    }
  }

  // suppress SIGINT, so that we do not get interrupted midway through a
  // database write and corrupt it
  if ((rc = sigint_block())) {
    fprintf(stderr, "failed to block SIGINT: %s\n", strerror(rc));
    goto done;
  }

  // set up progress output table
  if (smart_progress()) {
    for (unsigned long i = 0; i < option.threads; ++i)
      printf("%lu:\n", i);
  }

  if ((rc = option.threads > 1 ? mt_process(db, wq) : process(0, NULL, db, wq)))
    goto done;

done:
  (void)sigint_unblock();
  work_queue_free(&wq);
  (void)pthread_mutex_destroy(&db_lock);
  (void)pthread_mutex_destroy(&print_lock);

  return rc;
}
