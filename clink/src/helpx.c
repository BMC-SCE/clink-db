#include <unistd.h>
#include <errno.h>
#include "helpx.h"

// these symbols are generated by an xxd translation of CX_CursorKind.1
extern unsigned char CX_CursorKind_1[] ;
extern unsigned int CX_CursorKind_1_len;

int helpx(void) {

  int rc = 0;  

    ssize_t r = write(1, CX_CursorKind_1, (size_t)CX_CursorKind_1_len);
    if (r < 0 || (size_t)r != CX_CursorKind_1_len)
      rc = errno;

  return rc;
}

