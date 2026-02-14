#ifndef SOLOADER_PTCH_H
#define SOLOADER_PTCH_H

#include <stdbool.h>

/**
 * Apply all patches from DATA_PATH "patches/" directory.
 *
 * Scans for .ptch files and applies any that haven't been applied yet.
 * Patches that have already been applied (tracked via SHA1 hash) are skipped.
 *
 * @return Number of patches applied, or -1 on fatal error.
 */
int ptch_apply_all(void);

/**
 * Apply a single patch file.
 *
 * The target path is embedded in the patch file itself.
 *
 * @param patch_path Full path to the .ptch file.
 * @return true on success, false on failure.
 */
bool ptch_apply(const char *patch_path);

#endif // SOLOADER_PTCH_H
