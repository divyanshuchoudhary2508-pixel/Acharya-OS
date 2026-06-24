/*
 * AcharyaOS - user.h
 * ------------------
 * Phase 1, Feature 22: User Accounts.
 */

#ifndef ACHARYAOS_USER_H
#define ACHARYAOS_USER_H

#include <stddef.h>
#include <stdint.h>

#define USER_NAME_MAX    32
#define USER_FULL_MAX    64
#define USER_PASS_MAX    64
#define USER_ENTRY_MAX   16

typedef enum {
    USER_ROLE_GUEST = 0,
    USER_ROLE_USER  = 1,
    USER_ROLE_ADMIN = 2
} user_role_t;

typedef struct {
    char username[USER_NAME_MAX];
    char full_name[USER_FULL_MAX];
    uint64_t password_hash;
    user_role_t role;
    uint8_t enabled;
    uint8_t locked;
    uint32_t uid;
} user_entry_t;

typedef struct {
    size_t total_users;
    size_t enabled_users;
    size_t locked_users;
    uint32_t active_uid;
    char active_username[USER_NAME_MAX];
    uint8_t authenticated;
} user_stats_t;

void user_init(void);
size_t user_list(user_entry_t *out_entries, size_t max_entries);
int user_info(const char *username, user_entry_t *out_entry);
int user_add(const char *username, const char *full_name, user_role_t role);
int user_enable(const char *username);
int user_disable(const char *username);
int user_set_active(const char *username);
int user_get_active(user_entry_t *out_entry);
void user_get_stats(user_stats_t *stats);
const char *user_role_name(user_role_t role);
int user_get_active_name(char *out_name, size_t out_size);
int user_set_password(const char *username, const char *password);
int user_check_password(const char *username, const char *password);
int user_login(const char *username, const char *password);
void user_logout(void);
uint8_t user_is_authenticated(void);
int user_create_default_passwords(void);

#endif /* ACHARYAOS_USER_H */
