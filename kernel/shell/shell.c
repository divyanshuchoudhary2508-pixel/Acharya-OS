/*
 * AcharyaOS - shell.c
 * -------------------
 * Minimal kernel shell for Phase 1, Feature 5. It owns line editing and
 * command dispatch, while the keyboard driver remains a lower-level producer
 * of characters. This separation matters: the driver should not know what a
 * command line is, and the shell should not know what port 0x60 is.
 */

#include "shell.h"
#include "keyboard.h"
#include "kio.h"
#include "kstring.h"
#include "vga.h"
#include "kheap.h"
#include "vmm.h"
#include "timer.h"
#include "scheduler.h"
#include "process.h"
#include "syscall.h"
#include "ata.h"
#include "fs.h"
#include "userspace.h"
#include "log.h"
#include "editor.h"
#include "calc.h"
#include "browser.h"
#include "config.h"
#include "package.h"
#include "service.h"
#include "user.h"
#include "framebuffer.h"
#include "window.h"
#include "mouse.h"
#include "button.h"

#define SHELL_LINE_MAX 128
#define SHELL_HISTORY_MAX 16

static char shell_history[SHELL_HISTORY_MAX][SHELL_LINE_MAX];
static size_t shell_history_count = 0;
static size_t shell_history_next = 0;

static void shell_history_add(const char *line) {
    size_t i = 0;
    if (!line || *line == '\0') {
        return;
    }
    while (i + 1 < SHELL_LINE_MAX && line[i] != '\0') {
        shell_history[shell_history_next][i] = line[i];
        i++;
    }
    shell_history[shell_history_next][i] = '\0';
    shell_history_next = (shell_history_next + 1) % SHELL_HISTORY_MAX;
    if (shell_history_count < SHELL_HISTORY_MAX) {
        shell_history_count++;
    }
}

static const char *shell_history_get(size_t index) {
    size_t start;
    if (index >= shell_history_count) {
        return NULL;
    }
    start = (shell_history_next + SHELL_HISTORY_MAX - shell_history_count) % SHELL_HISTORY_MAX;
    return shell_history[(start + index) % SHELL_HISTORY_MAX];
}

static void shell_history_clear(void) {
    shell_history_count = 0;
    shell_history_next = 0;
    memset(shell_history, 0, sizeof(shell_history));
}

static void shell_print_prompt(void) {
    char prompt[CONFIG_VALUE_MAX];
    char active_user[USER_NAME_MAX];

    kio_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    if (user_get_active_name(active_user, sizeof(active_user)) == 0 && active_user[0] != '\0') {
        kprintf("%s@", active_user);
    }
    if (config_get("prompt", prompt, sizeof(prompt)) == 0 && prompt[0] != '\0') {
        kprintf("%s", prompt);
    } else {
        kprintf("acharyaos> ");
    }
    kio_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void shell_cmd_taskmgr(void) {
    process_info_t processes[PROCESS_MAX];
    sched_task_info_t tasks[SCHED_MAX_TASKS];
    process_stats_t pstats;
    sched_stats_t sstats;
    size_t pcount;
    size_t tcount;

    process_get_stats(&pstats);
    scheduler_get_stats(&sstats);
    pcount = process_copy_table(processes, PROCESS_MAX);
    tcount = scheduler_copy_tasks(tasks, SCHED_MAX_TASKS);

    kprintf("Task Manager\n");
    kprintf("processes: %d ready/running: %d next_pid: %d\n",
            (int32_t) pstats.process_count,
            (int32_t) pstats.ready_count,
            (int32_t) pstats.next_pid);
    kprintf("scheduler ticks: %d current task id: %d runnable: %d\n",
            (int32_t) sstats.total_ticks,
            (int32_t) sstats.current_task_id,
            (int32_t) sstats.runnable_count);
    kprintf("PID  TASK  STATE     NAME                 BORN\n");
    for (size_t i = 0; i < pcount; i++) {
        const char *name = processes[i].name ? processes[i].name : "";
        const char *state = process_state_name(processes[i].state);
        uint32_t task_id = processes[i].scheduler_task_id;
        uint64_t ticks_seen = 0;
        uint32_t slice = 0;

        for (size_t j = 0; j < tcount; j++) {
            if (tasks[j].id == task_id) {
                ticks_seen = tasks[j].ticks_seen;
                slice = tasks[j].time_slice_ticks;
                break;
            }
        }

        kprintf("%d %d %s %s %d (slice %d tick %d)\n",
                (int32_t) processes[i].pid,
                (int32_t) task_id,
                state,
                name,
                (int32_t) processes[i].created_at_tick,
                (int32_t) slice,
                (int32_t) ticks_seen);
    }
}

static void shell_readline(char *buffer, int max_len) {
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

        if (ch == '\t') {
            ch = ' ';
        }

        if (ch >= 32 && ch <= 126 && len < max_len - 1) {
            buffer[len++] = (char) ch;
            kprintf("%c", ch);
        }
    }
}

static void shell_cmd_help(void) {
    kprintf("Commands:\n");
    kprintf("  help     Show this command list\n");
    kprintf("  clear    Clear the VGA text screen\n");
    kprintf("  echo     Print text after the command\n");
    kprintf("  mem      Show early heap usage\n");
    kprintf("  vmem     Show virtual memory mapping status\n");
    kprintf("  ticks    Show timer interrupt count\n");
    kprintf("  sched    Show scheduler state\n");
    kprintf("  ps       Show process table\n");
    kprintf("  taskmgr  Show combined task manager view\n");
    kprintf("  spawn    Create demo kernel process metadata\n");
    kprintf("  syscall  Exercise the syscall path\n");
    kprintf("  disk     Show ATA disk status\n");
    kprintf("  disktest Run ATA read/write self-test\n");
    kprintf("  fs       Show filesystem status\n");
    kprintf("  ls       List filesystem files\n");
    kprintf("  cat      Print a filesystem file\n");
    kprintf("  writefs  Write text to a filesystem file\n");
    kprintf("  rmfs     Delete a filesystem file\n");
    kprintf("  edit     Open the text editor\n");
    kprintf("  calc     Evaluate an integer expression\n");
    kprintf("  gfxinfo  Show framebuffer status\n");
    kprintf("  gfxdemo  Draw the framebuffer demo pattern\n");
    kprintf("  wminfo   Show window manager status\n");
    kprintf("  wmdemo   Draw the window manager demo layout\n");
    kprintf("  mouse    Show mouse driver status\n");
    kprintf("  btninfo  Show button control status\n");
    kprintf("  btndemo  Draw the button demo layout\n");
    kprintf("  browse   Open the terminal file browser\n");
    kprintf("  config   Show or change system configuration\n");
    kprintf("  pkg      Show package installer status\n");
    kprintf("  pkginfo  Show one installed package\n");
    kprintf("  install  Install a package manifest\n");
    kprintf("  remove   Remove an installed package record\n");
    kprintf("  svc      Show startup services\n");
    kprintf("  svcinfo  Show one service\n");
    kprintf("  start    Start a service\n");
    kprintf("  stop     Stop a service\n");
    kprintf("  restart  Restart a service\n");
    kprintf("  enable   Enable a service\n");
    kprintf("  disable  Disable a service\n");
    kprintf("  users    Show user accounts\n");
    kprintf("  whoami   Show active user\n");
    kprintf("  su       Switch active user\n");
    kprintf("  adduser  Add a user account\n");
    kprintf("  deluser  Disable a user account\n");
    kprintf("  login    Authenticate as a user\n");
    kprintf("  logout   Clear login state\n");
    kprintf("  passwd   Set a user password\n");
    kprintf("  auth     Show authentication status\n");
    kprintf("  history  Show command history\n");
    kprintf("  !!       Repeat the last command\n");
    kprintf("  !n       Repeat history entry n\n");
    kprintf("  clearhist Clear command history\n");
    kprintf("  userdemo Launch the ring-3 demo program\n");
    kprintf("  log      Show recent kernel log entries\n");
    kprintf("  loglevel Show or change the log level\n");
    kprintf("  version  Show the current AcharyaOS milestone\n");
    kprintf("  halt     Stop the CPU\n");
}

static const char *skip_spaces(const char *s) {
    while (*s == ' ') {
        s++;
    }
    return s;
}

static int starts_with_word(const char *line, const char *word) {
    size_t n = strlen(word);
    if (memcmp(line, word, n) != 0) {
        return 0;
    }
    return line[n] == '\0' || line[n] == ' ';
}

static void shell_execute(const char *line) {
    char expanded[SHELL_LINE_MAX];
    const char *exec_line = line;
    const char *history_entry = NULL;
    int store_history = 1;

    line = skip_spaces(line);

    if (*line == '\0') {
        return;
    }

    if (strcmp(line, "!!") == 0) {
        if (shell_history_count == 0) {
            kprintf("history empty\n");
            return;
        }
        exec_line = shell_history_get(shell_history_count - 1);
        history_entry = exec_line;
        kprintf("%s\n", exec_line);
    } else if (line[0] == '!' && line[1] >= '0' && line[1] <= '9') {
        size_t index = 0;
        size_t i = 1;
        while (line[i] >= '0' && line[i] <= '9') {
            index = (index * 10) + (size_t) (line[i] - '0');
            i++;
        }
        if (index == 0 || index > shell_history_count) {
            kprintf("history entry not found\n");
            return;
        }
        exec_line = shell_history_get(index - 1);
        history_entry = exec_line;
        kprintf("%s\n", exec_line);
    } else {
        history_entry = exec_line;
    }

    if (exec_line != line) {
        size_t i = 0;
        while (i + 1 < sizeof(expanded) && exec_line[i] != '\0') {
            expanded[i] = exec_line[i];
            i++;
        }
        expanded[i] = '\0';
        line = expanded;
    }

    if (strcmp(line, "help") == 0) {
        shell_cmd_help();
    } else if (strcmp(line, "clear") == 0) {
        vga_clear();
    } else if (strcmp(line, "version") == 0) {
        kprintf("AcharyaOS Phase 3, Feature 28 - Mouse Driver\n");
    } else if (strcmp(line, "mem") == 0) {
        kheap_stats_t stats;
        kheap_get_stats(&stats);
        kprintf("heap start: 0x%x\n", (uint32_t) stats.heap_start);
        kprintf("heap current: 0x%x\n", (uint32_t) stats.heap_current);
        kprintf("heap end: 0x%x\n", (uint32_t) stats.heap_end);
        kprintf("bytes used: %d\n", (int32_t) stats.bytes_used);
        kprintf("allocations: %d\n", (int32_t) stats.allocation_count);
    } else if (strcmp(line, "vmem") == 0) {
        vmm_stats_t stats;
        vmm_get_stats(&stats);
        kprintf("active pml4: 0x%x\n", (uint32_t) stats.pml4_address);
        kprintf("mapped start: 0x%x\n", (uint32_t) stats.mapped_start);
        kprintf("mapped end: 0x%x\n", (uint32_t) stats.mapped_end);
        kprintf("2MiB pages: %d\n", (int32_t) stats.mapped_2m_pages);
        kprintf("vga virt->phys: 0x%x\n",
                (uint32_t) vmm_translate_identity(0xB8000u));
    } else if (strcmp(line, "ticks") == 0) {
        kprintf("timer frequency: %d Hz\n", (int32_t) timer_get_frequency());
        kprintf("timer ticks: %d\n", (int32_t) timer_get_ticks());
    } else if (strcmp(line, "sched") == 0) {
        sched_stats_t stats;
        sched_task_info_t tasks[SCHED_MAX_TASKS];
        scheduler_get_stats(&stats);
        size_t count = scheduler_copy_tasks(tasks, SCHED_MAX_TASKS);

        kprintf("scheduler ticks: %d\n", (int32_t) stats.total_ticks);
        kprintf("current task id: %d\n", (int32_t) stats.current_task_id);
        kprintf("tasks: %d runnable: %d\n",
                (int32_t) stats.task_count, (int32_t) stats.runnable_count);
        for (size_t i = 0; i < count; i++) {
            kprintf("  #%d %s slice=%d ticks=%d\n",
                    (int32_t) tasks[i].id,
                    tasks[i].name,
                    (int32_t) tasks[i].time_slice_ticks,
                    (int32_t) tasks[i].ticks_seen);
        }
    } else if (strcmp(line, "ps") == 0) {
        process_stats_t stats;
        process_info_t processes[PROCESS_MAX];
        process_get_stats(&stats);
        size_t count = process_copy_table(processes, PROCESS_MAX);

        kprintf("processes: %d ready/running: %d next_pid: %d\n",
                (int32_t) stats.process_count,
                (int32_t) stats.ready_count,
                (int32_t) stats.next_pid);
        for (size_t i = 0; i < count; i++) {
            kprintf("  pid=%d task=%d %s %s born_tick=%d\n",
                    (int32_t) processes[i].pid,
                    (int32_t) processes[i].scheduler_task_id,
                    process_state_name(processes[i].state),
                    processes[i].name,
                    (int32_t) processes[i].created_at_tick);
        }
    } else if (strcmp(line, "taskmgr") == 0) {
        shell_cmd_taskmgr();
    } else if (strcmp(line, "gfxinfo") == 0) {
        fb_info_t info;
        fb_get_info(&info);
        if (!fb_ready()) {
            kprintf("framebuffer not initialized\n");
        } else {
            kprintf("framebuffer ready\n");
            kprintf("size: %d x %d\n", (int32_t) info.width, (int32_t) info.height);
            kprintf("pitch: %d bytes\n", (int32_t) info.pitch);
            kprintf("bytes/pixel: %d\n", (int32_t) info.bytes_per_pixel);
        }
    } else if (strcmp(line, "gfxdemo") == 0) {
        fb_demo_pattern();
        kprintf("framebuffer demo pattern drawn\n");
    } else if (strcmp(line, "wminfo") == 0) {
        wm_stats_t stats;
        wm_get_stats(&stats);
        kprintf("window manager ready: %d\n", (int32_t) wm_ready());
        kprintf("windows: %d active: %d next_id: %d\n",
                (int32_t) stats.window_count,
                (int32_t) stats.active_window_id,
                (int32_t) stats.next_window_id);
    } else if (strcmp(line, "wmdemo") == 0) {
        wm_demo_layout();
        kprintf("window manager demo layout drawn\n");
    } else if (strcmp(line, "mouse") == 0) {
        mouse_state_t state;
        mouse_get_state(&state);
        kprintf("mouse ready: %d\n", (int32_t) mouse_ready());
        kprintf("packets: %d ready: %d buttons: 0x%x\n",
                (int32_t) state.packet_count,
                (int32_t) state.packet_ready,
                (uint32_t) state.buttons);
        kprintf("position: %d,%d delta: %d,%d\n",
                (int32_t) state.x,
                (int32_t) state.y,
                (int32_t) state.dx,
                (int32_t) state.dy);
    } else if (strcmp(line, "btninfo") == 0) {
        btn_stats_t stats;
        btn_get_stats(&stats);
        kprintf("buttons: %d pressed: %d next_id: %d\n",
                (int32_t) stats.button_count,
                (int32_t) stats.pressed_button_id,
                (int32_t) stats.next_button_id);
    } else if (strcmp(line, "btndemo") == 0) {
        btn_demo_layout();
        kprintf("button demo layout drawn\n");
    } else if (strcmp(line, "spawn") == 0) {
        int pid = process_create_kernel("demo-kernel-task", 5);
        if (pid < 0) {
            kprintf("spawn failed: process or scheduler table full\n");
        } else {
            kprintf("created demo process pid=%d\n", (int32_t) pid);
        }
    } else if (strcmp(line, "syscall") == 0) {
        syscall_write("syscall path: write() via int 0x80\n");
        kprintf("syscall getpid: %d\n", (int32_t) syscall_get_pid());
        kprintf("syscall get_ticks: %d\n", (int32_t) syscall_get_ticks());
        kprintf("syscall process_count: %d\n", (int32_t) syscall_get_process_count());
        syscall_yield();
    } else if (strcmp(line, "disk") == 0) {
        ata_device_info_t info;
        ata_get_info(&info);
        if (!info.present) {
            kprintf("no ATA disk detected on primary master\n");
        } else {
            kprintf("ATA disk detected\n");
            kprintf("model: %s\n", info.model);
            kprintf("sectors: %d\n", (int32_t) info.total_sectors);
            kprintf("lba48: %d\n", (int32_t) info.lba48_supported);
        }
    } else if (strcmp(line, "disktest") == 0) {
        int result = ata_disk_self_test();
        if (result == 0) {
            kprintf("ATA read/write self-test passed\n");
        } else {
            kprintf("ATA read/write self-test failed\n");
        }
    } else if (strcmp(line, "fs") == 0) {
        fs_info_t info;
        fs_get_info(&info);
        if (!info.mounted) {
            kprintf("filesystem not mounted\n");
        } else {
            kprintf("filesystem mounted\n");
            kprintf("label: %s\n", info.volume_label);
            kprintf("files: %d used: %d\n",
                    (int32_t) info.total_files, (int32_t) info.used_files);
            kprintf("data start lba: %d\n", (int32_t) info.data_start_lba);
        }
    } else if (strcmp(line, "ls") == 0) {
        fs_file_entry_t files[FS_FILES_MAX];
        size_t count = fs_list_files(files, FS_FILES_MAX);
        if (count == 0) {
            kprintf("(empty)\n");
        }
        for (size_t i = 0; i < count; i++) {
            kprintf("%s %d bytes @ lba %d\n",
                    files[i].name,
                    (int32_t) files[i].size_bytes,
                    (int32_t) files[i].start_lba);
        }
    } else if (starts_with_word(line, "cat")) {
        const char *name = skip_spaces(line + 3);
        uint8_t buffer[FS_BLOCK_SIZE * 4];
        size_t size = 0;
        if (*name == '\0') {
            kprintf("usage: cat <name>\n");
        } else if (fs_read_file(name, buffer, sizeof(buffer), &size) != 0) {
            kprintf("cat failed\n");
        } else {
            size_t end = size < sizeof(buffer) ? size : sizeof(buffer) - 1;
            buffer[end] = '\0';
            kprintf("%s\n", buffer);
        }
    } else if (starts_with_word(line, "writefs")) {
        const char *rest = skip_spaces(line + 7);
        const char *name = rest;
        while (*rest && *rest != ' ') {
            rest++;
        }
        if (*name == '\0' || *rest == '\0') {
            kprintf("usage: writefs <name> <text>\n");
        } else {
            char file_name[FS_NAME_MAX];
            size_t n = (size_t)(rest - name);
            if (n >= FS_NAME_MAX) {
                n = FS_NAME_MAX - 1;
            }
            memcpy(file_name, name, n);
            file_name[n] = '\0';
            rest = skip_spaces(rest);
            if (fs_write_file(file_name, rest, strlen(rest)) != 0) {
                kprintf("writefs failed\n");
            } else {
                kprintf("wrote %s\n", file_name);
            }
        }
    } else if (starts_with_word(line, "rmfs")) {
        const char *name = skip_spaces(line + 4);
        if (*name == '\0') {
            kprintf("usage: rmfs <name>\n");
        } else if (fs_delete_file(name) != 0) {
            kprintf("rmfs failed\n");
        } else {
            kprintf("deleted %s\n", name);
        }
    } else if (starts_with_word(line, "edit")) {
        const char *name = skip_spaces(line + 4);
        if (*name == '\0') {
            kprintf("usage: edit <name>\n");
        } else {
            editor_run(name);
        }
    } else if (starts_with_word(line, "calc")) {
        const char *expr = skip_spaces(line + 4);
        int64_t value = 0;
        if (*expr == '\0') {
            calc_get_help();
        } else if (calc_evaluate(expr, &value) != 0) {
            kprintf("calc error: invalid expression\n");
        } else {
            kprintf("%d\n", (int32_t) value);
        }
    } else if (starts_with_word(line, "browse")) {
        const char *prefix = skip_spaces(line + 6);
        browser_run(prefix);
    } else if (strcmp(line, "config") == 0) {
        config_entry_t entries[CONFIG_ENTRY_MAX];
        config_stats_t stats;
        size_t count;

        config_get_stats(&stats);
        count = config_list(entries, CONFIG_ENTRY_MAX);
        kprintf("configuration file: %s\n", stats.backing_file);
        kprintf("entries: %d/%d\n", (int32_t) stats.used_entries, (int32_t) stats.total_entries);
        for (size_t i = 0; i < count; i++) {
            kprintf("  %s = %s\n", entries[i].key, entries[i].value);
        }
    } else if (starts_with_word(line, "get")) {
        const char *key = skip_spaces(line + 3);
        char value[CONFIG_VALUE_MAX];
        if (*key == '\0') {
            kprintf("usage: get <key>\n");
        } else if (config_get(key, value, sizeof(value)) != 0) {
            kprintf("config key not found: %s\n", key);
        } else {
            kprintf("%s\n", value);
        }
    } else if (starts_with_word(line, "set")) {
        const char *rest = skip_spaces(line + 3);
        const char *value;
        char key[CONFIG_KEY_MAX];
        size_t n = 0;

        while (rest[n] != '\0' && rest[n] != ' ') {
            n++;
        }

        if (n == 0 || rest[n] == '\0') {
            kprintf("usage: set <key> <value>\n");
        } else {
            if (n >= sizeof(key)) {
                n = sizeof(key) - 1;
            }
            memcpy(key, rest, n);
            key[n] = '\0';
            value = skip_spaces(rest + n);
            if (config_set(key, value) != 0) {
                kprintf("config set failed\n");
            } else {
                kprintf("set %s = %s\n", key, value);
            }
        }
    } else if (strcmp(line, "savecfg") == 0) {
        if (config_save() != 0) {
            kprintf("config save failed\n");
        } else {
            kprintf("configuration saved\n");
        }
    } else if (strcmp(line, "loadcfg") == 0) {
        if (config_load() != 0) {
            kprintf("config load failed\n");
        } else {
            char level[16];
            if (config_get("loglevel", level, sizeof(level)) == 0) {
                if (strcmp(level, "trace") == 0) {
                    log_set_minimum_level(LOG_TRACE);
                } else if (strcmp(level, "debug") == 0) {
                    log_set_minimum_level(LOG_DEBUG);
                } else if (strcmp(level, "info") == 0) {
                    log_set_minimum_level(LOG_INFO);
                } else if (strcmp(level, "warn") == 0) {
                    log_set_minimum_level(LOG_WARN);
                } else if (strcmp(level, "error") == 0) {
                    log_set_minimum_level(LOG_ERROR);
                } else if (strcmp(level, "panic") == 0) {
                    log_set_minimum_level(LOG_PANIC);
                }
            }
            kprintf("configuration loaded\n");
        }
    } else if (strcmp(line, "pkg") == 0) {
        package_entry_t entries[PACKAGE_ENTRY_MAX];
        package_stats_t stats;
        size_t count;

        package_get_stats(&stats);
        count = package_list(entries, PACKAGE_ENTRY_MAX);
        kprintf("installed packages: %d/%d\n",
                (int32_t) stats.used_entries, (int32_t) stats.total_entries);
        for (size_t i = 0; i < count; i++) {
            kprintf("  %s %s files=%d source=%s\n",
                    entries[i].name,
                    entries[i].version,
                    (int32_t) entries[i].installed_files,
                    entries[i].source);
        }
    } else if (starts_with_word(line, "pkginfo")) {
        const char *name = skip_spaces(line + 7);
        package_entry_t entry;
        if (*name == '\0') {
            kprintf("usage: pkginfo <package-name>\n");
        } else if (package_info(name, &entry) != 0) {
            kprintf("package not found: %s\n", name);
        } else {
            kprintf("name: %s\n", entry.name);
            kprintf("version: %s\n", entry.version);
            kprintf("source: %s\n", entry.source);
            kprintf("files: %d\n", (int32_t) entry.installed_files);
        }
    } else if (starts_with_word(line, "install")) {
        const char *manifest = skip_spaces(line + 7);
        if (*manifest == '\0') {
            kprintf("usage: install <manifest-file>\n");
        } else if (package_install(manifest) != 0) {
            kprintf("package install failed\n");
        } else {
            kprintf("package installed from %s\n", manifest);
        }
    } else if (starts_with_word(line, "remove")) {
        const char *name = skip_spaces(line + 6);
        if (*name == '\0') {
            kprintf("usage: remove <package-name>\n");
        } else if (package_remove(name) != 0) {
            kprintf("package remove failed\n");
        } else {
            kprintf("package record removed: %s\n", name);
        }
    } else if (strcmp(line, "svc") == 0) {
        service_entry_t entries[SERVICE_ENTRY_MAX];
        service_stats_t stats;
        size_t count;
        service_get_stats(&stats);
        count = service_list(entries, SERVICE_ENTRY_MAX);
        kprintf("services: %d total, %d enabled, %d running\n",
                (int32_t) stats.total_services,
                (int32_t) stats.enabled_services,
                (int32_t) stats.running_services);
        for (size_t i = 0; i < count; i++) {
            kprintf("  %s [%s] enabled=%d started=%d stopped=%d\n",
                    entries[i].name,
                    service_state_name(entries[i].state),
                    (int32_t) entries[i].enabled,
                    (int32_t) entries[i].start_count,
                    (int32_t) entries[i].stop_count);
        }
    } else if (starts_with_word(line, "svcinfo")) {
        const char *name = skip_spaces(line + 7);
        service_entry_t entry;
        if (*name == '\0') {
            kprintf("usage: svcinfo <service-name>\n");
        } else if (service_info(name, &entry) != 0) {
            kprintf("service not found: %s\n", name);
        } else {
            kprintf("name: %s\n", entry.name);
            kprintf("description: %s\n", entry.description);
            kprintf("state: %s\n", service_state_name(entry.state));
            kprintf("enabled: %d\n", (int32_t) entry.enabled);
            kprintf("last started: %d\n", (int32_t) entry.last_started_tick);
            kprintf("last stopped: %d\n", (int32_t) entry.last_stopped_tick);
            kprintf("start count: %d\n", (int32_t) entry.start_count);
            kprintf("stop count: %d\n", (int32_t) entry.stop_count);
        }
    } else if (starts_with_word(line, "start")) {
        const char *name = skip_spaces(line + 5);
        if (*name == '\0') {
            kprintf("usage: start <service-name>\n");
        } else if (service_start(name) != 0) {
            kprintf("service start failed: %s\n", name);
        } else {
            kprintf("service started: %s\n", name);
        }
    } else if (starts_with_word(line, "stop")) {
        const char *name = skip_spaces(line + 4);
        if (*name == '\0') {
            kprintf("usage: stop <service-name>\n");
        } else if (service_stop(name) != 0) {
            kprintf("service stop failed: %s\n", name);
        } else {
            kprintf("service stopped: %s\n", name);
        }
    } else if (starts_with_word(line, "restart")) {
        const char *name = skip_spaces(line + 7);
        if (*name == '\0') {
            kprintf("usage: restart <service-name>\n");
        } else if (service_restart(name) != 0) {
            kprintf("service restart failed: %s\n", name);
        } else {
            kprintf("service restarted: %s\n", name);
        }
    } else if (starts_with_word(line, "enable")) {
        const char *name = skip_spaces(line + 6);
        if (*name == '\0') {
            kprintf("usage: enable <service-name>\n");
        } else if (service_enable(name) != 0) {
            kprintf("service enable failed: %s\n", name);
        } else {
            kprintf("service enabled: %s\n", name);
        }
    } else if (starts_with_word(line, "disable")) {
        const char *name = skip_spaces(line + 7);
        if (*name == '\0') {
            kprintf("usage: disable <service-name>\n");
        } else if (service_disable(name) != 0) {
            kprintf("service disable failed: %s\n", name);
        } else {
            kprintf("service disabled: %s\n", name);
        }
    } else if (strcmp(line, "users") == 0) {
        user_entry_t entries[USER_ENTRY_MAX];
        user_stats_t stats;
        size_t count;
        user_get_stats(&stats);
        count = user_list(entries, USER_ENTRY_MAX);
        kprintf("users: %d total, %d enabled, active=%s\n",
                (int32_t) stats.total_users,
                (int32_t) stats.enabled_users,
                stats.active_username);
        for (size_t i = 0; i < count; i++) {
            kprintf("  %s [%s] uid=%d enabled=%d name=%s\n",
                    entries[i].username,
                    user_role_name(entries[i].role),
                    (int32_t) entries[i].uid,
                    (int32_t) entries[i].enabled,
                    entries[i].full_name);
        }
    } else if (strcmp(line, "whoami") == 0) {
        user_entry_t active;
        if (user_get_active(&active) != 0) {
            kprintf("no active user\n");
        } else {
            kprintf("%s (%s)\n", active.username, user_role_name(active.role));
        }
    } else if (strcmp(line, "auth") == 0) {
        user_stats_t stats;
        user_get_stats(&stats);
        kprintf("authenticated: %d active=%s uid=%d\n",
                (int32_t) stats.authenticated,
                stats.active_username,
                (int32_t) stats.active_uid);
    } else if (strcmp(line, "history") == 0) {
        size_t i;
        if (shell_history_count == 0) {
            kprintf("(empty)\n");
        }
        for (i = 0; i < shell_history_count; i++) {
            const char *entry = shell_history_get(i);
            kprintf("%d %s\n", (int32_t) (i + 1), entry);
        }
    } else if (strcmp(line, "clearhist") == 0) {
        shell_history_clear();
        kprintf("history cleared\n");
        store_history = 0;
    } else if (starts_with_word(line, "login")) {
        const char *rest = skip_spaces(line + 5);
        const char *password;
        char username[USER_NAME_MAX];
        size_t n = 0;
        if (*rest == '\0') {
            kprintf("usage: login <username> <password>\n");
        } else {
            while (rest[n] != '\0' && rest[n] != ' ') {
                n++;
            }
            if (n == 0 || rest[n] == '\0') {
                kprintf("usage: login <username> <password>\n");
            } else {
                if (n >= sizeof(username)) {
                    n = sizeof(username) - 1;
                }
                memcpy(username, rest, n);
                username[n] = '\0';
                password = skip_spaces(rest + n);
                if (*password == '\0') {
                    kprintf("usage: login <username> <password>\n");
                } else {
                    if (user_login(username, password) != 0) {
                        kprintf("login failed\n");
                    } else {
                        kprintf("logged in as %s\n", username);
                    }
                }
            }
        }
    } else if (strcmp(line, "logout") == 0) {
        user_logout();
        kprintf("logged out\n");
    } else if (starts_with_word(line, "passwd")) {
        const char *rest = skip_spaces(line + 6);
        const char *password;
        char username[USER_NAME_MAX];
        size_t n = 0;
        if (*rest == '\0') {
            kprintf("usage: passwd <username> <password>\n");
        } else {
            while (rest[n] != '\0' && rest[n] != ' ') {
                n++;
            }
            if (n == 0 || rest[n] == '\0') {
                kprintf("usage: passwd <username> <password>\n");
            } else {
                if (n >= sizeof(username)) {
                    n = sizeof(username) - 1;
                }
                memcpy(username, rest, n);
                username[n] = '\0';
                password = skip_spaces(rest + n);
                if (*password == '\0') {
                    kprintf("usage: passwd <username> <password>\n");
                } else if (user_set_password(username, password) != 0) {
                    kprintf("passwd failed\n");
                } else {
                    kprintf("password updated for %s\n", username);
                }
            }
        }
    } else if (starts_with_word(line, "su")) {
        const char *name = skip_spaces(line + 2);
        if (*name == '\0') {
            kprintf("usage: su <username>\n");
        } else if (user_set_active(name) != 0) {
            kprintf("switch user failed: %s\n", name);
        } else {
            kprintf("active user is now %s\n", name);
        }
    } else if (starts_with_word(line, "adduser")) {
        const char *rest = skip_spaces(line + 7);
        const char *role_text;
        char username[USER_NAME_MAX];
        char full_name[USER_FULL_MAX];
        size_t n = 0;
        if (*rest == '\0') {
            kprintf("usage: adduser <username> <full name> [guest|user|admin]\n");
        } else {
            while (rest[n] != '\0' && rest[n] != ' ') {
                n++;
            }
            if (n == 0 || rest[n] == '\0') {
                kprintf("usage: adduser <username> <full name> [guest|user|admin]\n");
            } else {
                if (n >= sizeof(username)) {
                    n = sizeof(username) - 1;
                }
                memcpy(username, rest, n);
                username[n] = '\0';
                rest = skip_spaces(rest + n);
                role_text = rest;
                while (*role_text != '\0' && *role_text != ' ') {
                    role_text++;
                }
                if (role_text == rest) {
                    kprintf("usage: adduser <username> <full name> [guest|user|admin]\n");
                } else {
                    size_t full_len = (size_t) (role_text - rest);
                    if (full_len >= sizeof(full_name)) {
                        full_len = sizeof(full_name) - 1;
                    }
                    memcpy(full_name, rest, full_len);
                    full_name[full_len] = '\0';
                    role_text = skip_spaces(role_text);
                    if (*role_text == '\0' || strcmp(role_text, "user") == 0) {
                        if (user_add(username, full_name, USER_ROLE_USER) != 0) {
                            kprintf("adduser failed\n");
                        } else {
                            kprintf("user added: %s\n", username);
                        }
                    } else if (strcmp(role_text, "guest") == 0) {
                        if (user_add(username, full_name, USER_ROLE_GUEST) != 0) {
                            kprintf("adduser failed\n");
                        } else {
                            kprintf("user added: %s\n", username);
                        }
                    } else if (strcmp(role_text, "admin") == 0) {
                        if (user_add(username, full_name, USER_ROLE_ADMIN) != 0) {
                            kprintf("adduser failed\n");
                        } else {
                            kprintf("user added: %s\n", username);
                        }
                    } else {
                        kprintf("usage: adduser <username> <full name> [guest|user|admin]\n");
                    }
                }
            }
        }
    } else if (starts_with_word(line, "deluser")) {
        const char *name = skip_spaces(line + 7);
        if (*name == '\0') {
            kprintf("usage: deluser <username>\n");
        } else if (user_disable(name) != 0) {
            kprintf("deluser failed: %s\n", name);
        } else {
            kprintf("user disabled: %s\n", name);
        }
    } else if (strcmp(line, "userdemo") == 0) {
        userspace_run_demo();
    } else if (strcmp(line, "log") == 0) {
        log_entry_t entries[LOG_BUFFER_MAX];
        size_t count = log_copy_entries(entries, LOG_BUFFER_MAX);
        if (count == 0) {
            kprintf("(log buffer empty)\n");
        }
        for (size_t i = 0; i < count; i++) {
            kprintf("[%s][%d] %s\n",
                    log_level_name(entries[i].level),
                    (int32_t) entries[i].tick,
                    entries[i].message);
        }
    } else if (starts_with_word(line, "loglevel")) {
        const char *arg = skip_spaces(line + 8);
        if (*arg == '\0') {
            kprintf("minimum log level: %s\n", log_level_name(log_get_minimum_level()));
            kprintf("levels: trace debug info warn error panic\n");
        } else if (strcmp(arg, "trace") == 0) {
            log_set_minimum_level(LOG_TRACE);
            config_set("loglevel", "trace");
        } else if (strcmp(arg, "debug") == 0) {
            log_set_minimum_level(LOG_DEBUG);
            config_set("loglevel", "debug");
        } else if (strcmp(arg, "info") == 0) {
            log_set_minimum_level(LOG_INFO);
            config_set("loglevel", "info");
        } else if (strcmp(arg, "warn") == 0) {
            log_set_minimum_level(LOG_WARN);
            config_set("loglevel", "warn");
        } else if (strcmp(arg, "error") == 0) {
            log_set_minimum_level(LOG_ERROR);
            config_set("loglevel", "error");
        } else if (strcmp(arg, "panic") == 0) {
            log_set_minimum_level(LOG_PANIC);
            config_set("loglevel", "panic");
        } else {
            kprintf("usage: loglevel [trace|debug|info|warn|error|panic]\n");
        }
        if (*arg != '\0' &&
            (strcmp(arg, "trace") == 0 || strcmp(arg, "debug") == 0 ||
             strcmp(arg, "info") == 0 || strcmp(arg, "warn") == 0 ||
             strcmp(arg, "error") == 0 || strcmp(arg, "panic") == 0)) {
            kprintf("minimum log level set to %s\n", log_level_name(log_get_minimum_level()));
        }
    } else if (starts_with_word(line, "echo")) {
        const char *text = skip_spaces(line + 4);
        kprintf("%s\n", text);
    } else if (strcmp(line, "halt") == 0) {
        kprintf("System halted.\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    } else {
        kprintf("Unknown command: %s\n", line);
        kprintf("Type 'help' for available commands.\n");
    }

    if (strcmp(line, "history") == 0 || strcmp(line, "clearhist") == 0) {
        store_history = 0;
    }
    if (store_history && history_entry != NULL && history_entry[0] != '\0' &&
        strcmp(history_entry, "history") != 0 && strcmp(history_entry, "clearhist") != 0) {
        shell_history_add(history_entry);
    }
}

void shell_run(void) {
    char line[SHELL_LINE_MAX];

    kprintf("AcharyaOS command shell ready.\n");
    kprintf("Type 'help' for commands.\n\n");

    for (;;) {
        shell_print_prompt();
        shell_readline(line, SHELL_LINE_MAX);
        shell_execute(line);
    }
}
