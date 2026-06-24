/*
 * AcharyaOS - editor.h
 * --------------------
 * Phase 1, Feature 16: Text Editor.
 *
 * This editor is intentionally simple and line-oriented. The goal at this
 * stage is to prove that the shell can launch an interactive editing mode,
 * read and modify text in memory, and save it back through the filesystem.
 */

#ifndef ACHARYAOS_EDITOR_H
#define ACHARYAOS_EDITOR_H

#include "kernel.h"

void editor_run(const char *path);

#endif /* ACHARYAOS_EDITOR_H */
