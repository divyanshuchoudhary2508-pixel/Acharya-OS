/*
 * AcharyaOS - browser.h
 * ---------------------
 * Phase 1, Feature 18: File Browser, Terminal Mode.
 *
 * The browser is an interactive terminal utility for exploring files in
 * the current filesystem. Because AcharyaOS's filesystem is still flat,
 * the browser focuses on listing, filtering, and previewing files rather
 * than directory traversal.
 */

#ifndef ACHARYAOS_BROWSER_H
#define ACHARYAOS_BROWSER_H

void browser_run(const char *filter_prefix);

#endif /* ACHARYAOS_BROWSER_H */
