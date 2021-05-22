#pragma once

/// check if file exist and is executable
__attribute__((visibility("internal")))
int exist_X ( const char* file );

/// find executable name in PATH
__attribute__((visibility("internal")))
int findX_in_PATH ( const char* name, char* fullpath, size_t sz );

/** create a temporary file
 *
 * \param file [out] Path to created file on success
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int temp_file(char **tfile);


