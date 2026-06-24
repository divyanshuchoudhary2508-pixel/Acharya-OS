/*
 * AcharyaOS - browser.c
 * ---------------------
 * Terminal-mode file browser for the simple filesystem.
 */

#include "browser.h"
#include "fs.h"
#include "keyboard.h"
#include "kio.h"
#include "kstring.h"
#include "vga.h"

#define BROWSER_INPUT_MAX 64
#define BROWSER_PREVIEW_MAX 256

static void browser_print_prompt(void) {
    kio_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    kprintf("browser> ");
    kio_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void browser_readline(char *buffer, int max_len) {
    int len = 0;

    for (;;) {
        int ch = keyboard_getchar();
        if (ch < 0) {
            __asm__ volatile ("hlt");
            continue;
        }

        if (ch == '\n') {
            kprintf("\n");
            buffer[len] = '\0';
            return;
        }

        if (ch == '\b') {
            if (len > 0) {
                len--;
                kprintf("\b");
            }
            continue;
        }

        if (ch >= 32 && ch <= 126 && len < max_len - 1) {
            buffer[len++] = (char) ch;
            kprintf("%c", ch);
        }
    }
}

static const char *browser_skip_spaces(const char *s) {
    while (*s == ' ') {
        s++;
    }
    return s;
}

static int browser_starts_with(const char *text, const char *prefix) {
    size_t n = strlen(prefix);
    return memcmp(text, prefix, n) == 0;
}

static int browser_prefix_matches(const char *name, const char *prefix) {
    if (!prefix || *prefix == '\0') {
        return 1;
    }
    return browser_starts_with(name, prefix);
}

static void browser_show_files(const char *prefix) {
    fs_file_entry_t files[FS_FILES_MAX];
    size_t count = fs_list_files(files, FS_FILES_MAX);

    kprintf("Files%s%s:\n",
            (prefix && *prefix) ? " matching " : "",
            (prefix && *prefix) ? prefix : "");

    if (count == 0) {
        kprintf("(empty)\n");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (!browser_prefix_matches(files[i].name, prefix)) {
            continue;
        }
        kprintf("  %s  %d bytes  lba=%d\n",
                files[i].name,
                (int32_t) files[i].size_bytes,
                (int32_t) files[i].start_lba);
    }
}

static void browser_preview_file(const char *name) {
    uint8_t buffer[BROWSER_PREVIEW_MAX];
    size_t size = 0;
    size_t end;

    if (*name == '\0') {
        kprintf("usage: open <name>\n");
        return;
    }

    if (fs_read_file(name, buffer, sizeof(buffer), &size) != 0) {
        kprintf("open failed: %s\n", name);
        return;
    }

    end = size < sizeof(buffer) ? size : sizeof(buffer) - 1;
    buffer[end] = '\0';
    kprintf("----- %s -----\n", name);
    kprintf("%s\n", buffer);
}

static void browser_info_file(const char *name) {
    fs_file_entry_t files[FS_FILES_MAX];
    size_t count = fs_list_files(files, FS_FILES_MAX);

    if (*name == '\0') {
        kprintf("usage: info <name>\n");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            kprintf("name: %s\n", files[i].name);
            kprintf("size: %d bytes\n", (int32_t) files[i].size_bytes);
            kprintf("start lba: %d\n", (int32_t) files[i].start_lba);
            return;
        }
    }

    kprintf("info failed: %s\n", name);
}

static void browser_help(void) {
    kprintf("Browser commands:\n");
    kprintf("  ls           List visible files\n");
    kprintf("  filter <p>   Show files matching prefix\n");
    kprintf("  open <name>  Preview a file\n");
    kprintf("  info <name>  Show file metadata\n");
    kprintf("  refresh      Reload the file list\n");
    kprintf("  q            Quit browser\n");
    kprintf("  help         Show this help\n");
}

void browser_run(const char *filter_prefix) {
    char line[BROWSER_INPUT_MAX];
    char prefix[BROWSER_INPUT_MAX];

    if (!fs_mounted()) {
        kprintf("filesystem not mounted\n");
        return;
    }

    if (!filter_prefix) {
        filter_prefix = "";
    }

    size_t i = 0;
    while (i + 1 < sizeof(prefix) && filter_prefix[i] != '\0') {
        prefix[i] = filter_prefix[i];
        i++;
    }
    prefix[i] = '\0';

    browser_help();
    browser_show_files(prefix);

    for (;;) {
        browser_print_prompt();
        browser_readline(line, sizeof(line));

        if (strcmp(line, "help") == 0) {
            browser_help();
        } else if (strcmp(line, "ls") == 0 || strcmp(line, "refresh") == 0) {
            browser_show_files(prefix);
        } else if (browser_starts_with(line, "filter")) {
            const char *arg = browser_skip_spaces(line + 6);
            size_t j = 0;
            while (j + 1 < sizeof(prefix) && arg[j] != '\0') {
                prefix[j] = arg[j];
                j++;
            }
            prefix[j] = '\0';
            browser_show_files(prefix);
        } else if (browser_starts_with(line, "open")) {
            const char *name = browser_skip_spaces(line + 4);
            browser_preview_file(name);
        } else if (browser_starts_with(line, "info")) {
            const char *name = browser_skip_spaces(line + 4);
            browser_info_file(name);
        } else if (strcmp(line, "q") == 0) {
            return;
        } else if (line[0] == '\0') {
            continue;
        } else {
            kprintf("unknown browser command: %s\n", line);
        }
    }
}
