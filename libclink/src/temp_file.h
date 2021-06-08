#pragma once

/** create a temporary file
 *
 * \param file [out] Path to created file on success
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int temp_file(char **tfile, const char *filename);

__attribute__((visibility("internal")))
bool is_fileRWNZ(const char *path);
