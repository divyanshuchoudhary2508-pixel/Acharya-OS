# AcharyaOS Phase 1, Feature 23 - Password Login

## What this subsystem does

Feature 23 adds password-based login to the user account layer.

It provides:

- password storage as a simple kernel hash
- boot-time login before the shell opens
- login, logout, and password-setting commands
- basic authentication state tracking

## Dependencies

This subsystem depends on:

- the user account system
- the keyboard input path
- the shell
- the logger, for later audit-style messages

## Architecture

Passwords are stored as a hash in each user record. The implementation is
intentionally simple and educational, not production-grade cryptography.

At boot, the kernel:

1. initializes user accounts
2. seeds default passwords for built-in accounts
3. prompts for username and password after interrupts are enabled
4. continues to the shell only after a successful login

The shell can also:

- authenticate a user with `login`
- clear the session with `logout`
- change a password with `passwd`
- inspect authentication state with `auth`

## Folder structure

```text
kernel/include/user.h
kernel/user/user.c
docs/23-password-login.md
```

## Shell commands

- `login <username> <password>`
- `logout`
- `passwd <username> <password>`
- `auth`

## Debugging guide

If login fails:

1. Check the username spelling.
2. Make sure the account is enabled.
3. Verify the password was set with `passwd` or the default boot password.

If the shell does not appear after boot, confirm the keyboard is working,
because the login prompt runs before the shell starts.
