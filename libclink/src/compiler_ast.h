#pragma once

#include <stdbool.h>
#include <stddef.h>

/** generate ast from C/C++ file
 *
 * \param ast_out [out] Created ast on success
 * \param filename Path to source file to parse
 * \param argc Number of elements in argv
 * \param argv Arguments to pass to Clang
 * \returns 0 on success or an errno on failure
 */

int clink_compiler_ast(const char *compiler, const char *filename, size_t argc,
    const char **argv, char *ast_out);

int write_log(unsigned char *data, size_t length, const char* log, bool log_kind);
