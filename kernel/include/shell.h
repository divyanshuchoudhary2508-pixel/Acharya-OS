/*
 * AcharyaOS - shell.h
 * -------------------
 * Phase 1, Feature 5: a tiny kernel-resident command shell. This is not
 * user-space yet; it is the interactive command loop that proves keyboard
 * input can drive kernel behavior before memory, processes, and syscalls
 * exist.
 */

#ifndef ACHARYAOS_SHELL_H
#define ACHARYAOS_SHELL_H

#include "kernel.h"

void shell_run(void) NORETURN;

#endif /* ACHARYAOS_SHELL_H */
