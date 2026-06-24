/*
 * AcharyaOS - editor.c
 * --------------------
 * Small line-oriented text editor for the kernel shell.
 */

#include "editor.h"
#include "fs.h"
#include "keyboard.h"
#include "kio.h"
#include "kstring.h"
#include "vga.h"

#define EDITOR_MAX_LINES 32
#define EDITOR_LINE_MAX   64
#define EDITOR_FILE_MAX   (EDITOR_MAX_LINES * EDITOR_LINE_MAX)

typedef struct {
    char lines[EDITOR_MAX_LINES][EDITOR_LINE_MAX];
    size_t line_count;
    size_t cursor;
    char path[FS_NAME_MAX];
    int dirty;
} editor_state_t;

static void editor_print_help(void) {
    kprintf("Editor commands:\n");
    kprintf("  show          Show current buffer\n");
    kprintf("  i <text>      Insert a line at the cursor\n");
    kprintf("  a <text>      Append a line after the cursor\n");
    kprintf("  d             Delete the current line\n");
    kprintf("  n             Move cursor to next line\n");
    kprintf("  p             Move cursor to previous line\n");
    kprintf("  s             Save to disk\n");
    kprintf("  wq            Save and quit\n");
    kprintf("  q             Quit without saving\n");
    kprintf("  help          Show this help text\n");
}

static int editor_starts_with_word(const char *line, const char *word) {
    size_t n = strlen(word);
    if (memcmp(line, word, n) != 0) {
        return 0;
    }
    return line[n] == '\0' || line[n] == ' ';
}

static void editor_set_status(void) {
    kio_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("editor> ");
    kio_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void editor_readline(char *buffer, int max_len) {
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

static void editor_reset(editor_state_t *ed, const char *path) {
    memset(ed, 0, sizeof(*ed));
    size_t i = 0;
    while (i + 1 < sizeof(ed->path) && path[i] != '\0') {
        ed->path[i] = path[i];
        i++;
    }
    ed->path[i] = '\0';
}

static void editor_load(editor_state_t *ed) {
    uint8_t buffer[EDITOR_FILE_MAX];
    size_t size = 0;
    size_t line = 0;
    size_t pos = 0;

    if (fs_read_file(ed->path, buffer, sizeof(buffer), &size) != 0) {
        kprintf("[editor] new file: %s\n", ed->path);
        return;
    }

    for (size_t i = 0; i < size && line < EDITOR_MAX_LINES; i++) {
        char c = (char) buffer[i];
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            ed->lines[line][pos] = '\0';
            line++;
            pos = 0;
            continue;
        }
        if (pos + 1 < EDITOR_LINE_MAX) {
            ed->lines[line][pos++] = c;
        }
    }

    if (line < EDITOR_MAX_LINES) {
        ed->lines[line][pos] = '\0';
        if (pos > 0 || line == 0) {
            line++;
        }
    }

    ed->line_count = line;
    ed->cursor = 0;
    kprintf("[editor] loaded %d lines from %s\n", (int32_t) ed->line_count, ed->path);
}

static void editor_show(const editor_state_t *ed) {
    kprintf("----- %s -----\n", ed->path);
    if (ed->line_count == 0) {
        kprintf("(empty)\n");
        return;
    }
    for (size_t i = 0; i < ed->line_count; i++) {
        if (i == ed->cursor) {
            kprintf(">%d: %s\n", (int32_t)(i + 1), ed->lines[i]);
        } else {
            kprintf(" %d: %s\n", (int32_t)(i + 1), ed->lines[i]);
        }
    }
}

static void editor_insert_line(editor_state_t *ed, const char *text) {
    size_t insert_at = ed->cursor;
    if (ed->line_count >= EDITOR_MAX_LINES) {
        kprintf("[editor] buffer full\n");
        return;
    }
    for (size_t i = ed->line_count; i > insert_at; i--) {
        memcpy(ed->lines[i], ed->lines[i - 1], EDITOR_LINE_MAX);
    }
    size_t n = 0;
    while (n + 1 < EDITOR_LINE_MAX && text[n] != '\0') {
        ed->lines[insert_at][n] = text[n];
        n++;
    }
    ed->lines[insert_at][n] = '\0';
    ed->line_count++;
    ed->cursor = insert_at;
    ed->dirty = 1;
}

static void editor_append_line(editor_state_t *ed, const char *text) {
    if (ed->line_count >= EDITOR_MAX_LINES) {
        kprintf("[editor] buffer full\n");
        return;
    }
    ed->cursor = ed->line_count;
    editor_insert_line(ed, text);
}

static void editor_delete_line(editor_state_t *ed) {
    if (ed->line_count == 0) {
        kprintf("[editor] nothing to delete\n");
        return;
    }
    for (size_t i = ed->cursor; i + 1 < ed->line_count; i++) {
        memcpy(ed->lines[i], ed->lines[i + 1], EDITOR_LINE_MAX);
    }
    memset(ed->lines[ed->line_count - 1], 0, EDITOR_LINE_MAX);
    ed->line_count--;
    if (ed->cursor >= ed->line_count && ed->cursor > 0) {
        ed->cursor--;
    }
    ed->dirty = 1;
}

static int editor_save(const editor_state_t *ed) {
    char buffer[EDITOR_FILE_MAX];
    size_t pos = 0;

    for (size_t i = 0; i < ed->line_count; i++) {
        size_t j = 0;
        while (ed->lines[i][j] != '\0' && pos + 1 < sizeof(buffer)) {
            buffer[pos++] = ed->lines[i][j++];
        }
        if (pos + 1 < sizeof(buffer)) {
            buffer[pos++] = '\n';
        }
    }
    if (pos == 0) {
        buffer[pos++] = '\n';
    }

    if (fs_write_file(ed->path, buffer, pos) != 0) {
        kprintf("[editor] save failed\n");
        return -1;
    }

    kprintf("[editor] saved %s\n", ed->path);
    return 0;
}

void editor_run(const char *path) {
    editor_state_t ed;
    char line[EDITOR_LINE_MAX];

    if (!path || *path == '\0') {
        kprintf("usage: edit <file>\n");
        return;
    }

    editor_reset(&ed, path);
    editor_load(&ed);
    editor_print_help();

    for (;;) {
        editor_set_status();
        editor_readline(line, sizeof(line));

        if (strcmp(line, "help") == 0) {
            editor_print_help();
        } else if (strcmp(line, "show") == 0) {
            editor_show(&ed);
        } else if (strcmp(line, "n") == 0) {
            if (ed.cursor + 1 < ed.line_count) {
                ed.cursor++;
            }
        } else if (strcmp(line, "p") == 0) {
            if (ed.cursor > 0) {
                ed.cursor--;
            }
        } else if (strcmp(line, "d") == 0) {
            editor_delete_line(&ed);
        } else if (strcmp(line, "s") == 0) {
            (void) editor_save(&ed);
            ed.dirty = 0;
        } else if (strcmp(line, "wq") == 0) {
            if (editor_save(&ed) == 0) {
                return;
            }
        } else if (strcmp(line, "q") == 0) {
            if (ed.dirty) {
                kprintf("[editor] unsaved changes; type wq to save and quit\n");
            } else {
                return;
            }
        } else if (editor_starts_with_word(line, "i")) {
            const char *text = line + 1;
            while (*text == ' ') {
                text++;
            }
            editor_insert_line(&ed, text);
        } else if (editor_starts_with_word(line, "a")) {
            const char *text = line + 1;
            while (*text == ' ') {
                text++;
            }
            editor_append_line(&ed, text);
        } else if (line[0] == '\0') {
            continue;
        } else {
            kprintf("[editor] unknown command: %s\n", line);
        }
    }
}
