/*
 * AcharyaOS - user.c
 * ------------------
 * Small in-memory user registry.
 */

#include "user.h"
#include "kstring.h"

static user_entry_t users[USER_ENTRY_MAX];
static uint32_t active_uid;
static uint8_t authenticated;

static uint64_t user_hash_password(const char *text) {
    uint64_t hash = 1469598103934665603ull;
    while (*text != '\0') {
        hash ^= (uint8_t) *text++;
        hash *= 1099511628211ull;
    }
    return hash;
}

static void user_copy_text(char *dest, size_t size, const char *src) {
    size_t i = 0;
    if (size == 0) {
        return;
    }
    while (i + 1 < size && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int user_find(const char *username) {
    for (size_t i = 0; i < USER_ENTRY_MAX; i++) {
        if (users[i].username[0] != '\0' && strcmp(users[i].username, username) == 0) {
            return (int) i;
        }
    }
    return -1;
}

static int user_find_free(void) {
    for (size_t i = 0; i < USER_ENTRY_MAX; i++) {
        if (users[i].username[0] == '\0') {
            return (int) i;
        }
    }
    return -1;
}

const char *user_role_name(user_role_t role) {
    switch (role) {
        case USER_ROLE_GUEST: return "guest";
        case USER_ROLE_USER:  return "user";
        case USER_ROLE_ADMIN: return "admin";
        default: return "unknown";
    }
}

static void user_register(const char *username, const char *full_name, user_role_t role, uint8_t enabled) {
    int idx = user_find_free();
    if (idx < 0) {
        return;
    }
    user_copy_text(users[idx].username, sizeof(users[idx].username), username);
    user_copy_text(users[idx].full_name, sizeof(users[idx].full_name), full_name);
    users[idx].role = role;
    users[idx].enabled = enabled;
    users[idx].locked = 0;
    users[idx].password_hash = 0;
    users[idx].uid = (uint32_t) (idx + 1);
}

void user_init(void) {
    memset(users, 0, sizeof(users));
    user_register("root", "AcharyaOS Root", USER_ROLE_ADMIN, 1);
    user_register("admin", "System Administrator", USER_ROLE_ADMIN, 1);
    user_register("guest", "Guest Session", USER_ROLE_GUEST, 1);
    active_uid = 1;
    authenticated = 0;
}

size_t user_list(user_entry_t *out_entries, size_t max_entries) {
    size_t count = 0;
    if (!out_entries || max_entries == 0) {
        return 0;
    }
    for (size_t i = 0; i < USER_ENTRY_MAX && count < max_entries; i++) {
        if (users[i].username[0] == '\0') {
            continue;
        }
        out_entries[count++] = users[i];
    }
    return count;
}

int user_info(const char *username, user_entry_t *out_entry) {
    int idx = user_find(username);
    if (idx < 0 || !out_entry) {
        return -1;
    }
    *out_entry = users[idx];
    return 0;
}

int user_add(const char *username, const char *full_name, user_role_t role) {
    int idx;
    if (!username || !full_name || *username == '\0') {
        return -1;
    }
    if (user_find(username) >= 0) {
        return -1;
    }
    idx = user_find_free();
    if (idx < 0) {
        return -1;
    }
    user_copy_text(users[idx].username, sizeof(users[idx].username), username);
    user_copy_text(users[idx].full_name, sizeof(users[idx].full_name), full_name);
    users[idx].role = role;
    users[idx].enabled = 1;
    users[idx].locked = 0;
    users[idx].password_hash = 0;
    users[idx].uid = (uint32_t) (idx + 1);
    return 0;
}

int user_enable(const char *username) {
    int idx = user_find(username);
    if (idx < 0) {
        return -1;
    }
    users[idx].enabled = 1;
    return 0;
}

int user_disable(const char *username) {
    int idx = user_find(username);
    if (idx < 0) {
        return -1;
    }
    if (users[idx].uid == active_uid) {
        return -1;
    }
    users[idx].enabled = 0;
    return 0;
}

int user_set_active(const char *username) {
    int idx = user_find(username);
    if (idx < 0 || !users[idx].enabled) {
        return -1;
    }
    active_uid = users[idx].uid;
    return 0;
}

int user_get_active(user_entry_t *out_entry) {
    for (size_t i = 0; i < USER_ENTRY_MAX; i++) {
        if (users[i].uid == active_uid && users[i].username[0] != '\0') {
            if (out_entry) {
                *out_entry = users[i];
            }
            return 0;
        }
    }
    return -1;
}

void user_get_stats(user_stats_t *stats) {
    char active_name[USER_NAME_MAX];
    size_t enabled = 0;
    size_t total = 0;
    if (!stats) {
        return;
    }
    for (size_t i = 0; i < USER_ENTRY_MAX; i++) {
        if (users[i].username[0] == '\0') {
            continue;
        }
        total++;
        if (users[i].enabled) {
            enabled++;
        }
    }
    stats->total_users = total;
    stats->enabled_users = enabled;
    stats->locked_users = 0;
    stats->active_uid = active_uid;
    if (user_get_active_name(active_name, sizeof(active_name)) == 0) {
        user_copy_text(stats->active_username, sizeof(stats->active_username), active_name);
    } else {
        stats->active_username[0] = '\0';
    }
    stats->authenticated = authenticated;
    for (size_t i = 0; i < USER_ENTRY_MAX; i++) {
        if (users[i].username[0] != '\0' && users[i].locked) {
            stats->locked_users++;
        }
    }
}

int user_get_active_name(char *out_name, size_t out_size) {
    user_entry_t active;
    if (!out_name || out_size == 0) {
        return -1;
    }
    if (user_get_active(&active) != 0) {
        out_name[0] = '\0';
        return -1;
    }
    user_copy_text(out_name, out_size, active.username);
    return 0;
}

int user_set_password(const char *username, const char *password) {
    int idx = user_find(username);
    if (idx < 0 || !password) {
        return -1;
    }
    users[idx].password_hash = user_hash_password(password);
    users[idx].locked = 0;
    return 0;
}

int user_check_password(const char *username, const char *password) {
    int idx = user_find(username);
    if (idx < 0 || !password || users[idx].password_hash == 0) {
        return -1;
    }
    return users[idx].password_hash == user_hash_password(password) ? 0 : -1;
}

int user_login(const char *username, const char *password) {
    int idx = user_find(username);
    if (idx < 0 || !users[idx].enabled) {
        return -1;
    }
    if (users[idx].password_hash == 0) {
        return -1;
    }
    if (user_check_password(username, password) != 0) {
        users[idx].locked = 1;
        return -1;
    }
    active_uid = users[idx].uid;
    authenticated = 1;
    users[idx].locked = 0;
    return 0;
}

void user_logout(void) {
    authenticated = 0;
}

uint8_t user_is_authenticated(void) {
    return authenticated;
}

int user_create_default_passwords(void) {
    int idx;
    idx = user_find("root");
    if (idx >= 0) {
        users[idx].password_hash = user_hash_password("root");
    }
    idx = user_find("admin");
    if (idx >= 0) {
        users[idx].password_hash = user_hash_password("admin");
    }
    idx = user_find("guest");
    if (idx >= 0) {
        users[idx].password_hash = user_hash_password("guest");
    }
    return 0;
}
