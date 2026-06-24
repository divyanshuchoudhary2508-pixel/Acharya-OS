# AcharyaOS Phase 1, Feature 22 - User Accounts

## What this subsystem does

Feature 22 adds a small local user account system. It tracks usernames,
roles, enabled state, and the currently active user.

## Dependencies

This subsystem depends on:

- the shell, to show and switch users
- the logger, for any future account-related diagnostics
- the kernel heap and string helpers, for storage and text handling

It does not yet depend on disk persistence or password hashing. Those are
good later hardening steps.

## Architecture

The account system uses a fixed in-memory registry of users. Each account
stores:

- username
- full name
- role
- enabled flag
- uid

The kernel keeps one active user at a time. The shell can query and change
that active identity.

## Folder structure

```text
kernel/include/user.h
kernel/user/user.c
docs/22-user-accounts.md
```

## Shell commands

- `users` shows the account table
- `whoami` shows the active user
- `su <username>` switches the active user
- `adduser <username> <full name> [guest|user|admin]`
- `deluser <username>` disables an account

## Debugging guide

If switching users fails:

1. Check that the target user exists.
2. Verify the account is enabled.
3. Use `users` to inspect the table.

If the prompt or shell behavior needs to reflect the active user later,
the account layer already exposes the active identity for that refinement.
