# AcharyaOS Phase 1, Feature 21 - Startup Services

## What this subsystem does

Feature 21 adds a lightweight startup service manager. It keeps a small
registry of named services, tracks whether they are enabled, and records
whether they are running.

This gives AcharyaOS a small init-style layer without introducing a full
process supervisor yet.

## Dependencies

This subsystem depends on:

- the timer, for service timestamps
- the logger, for service-related diagnostics
- the shell, for manual inspection and control

## Architecture

The service manager is a fixed registry of entries in kernel memory.
Every service has:

- a name
- a description
- enabled state
- runtime state
- start and stop counters
- last start and stop timestamps

During boot, `service_startup_all()` starts every enabled service.

## Folder structure

```text
kernel/include/service.h
kernel/service/service.c
docs/21-startup-services.md
```

## Shell commands

- `svc` shows all services
- `svcinfo <service-name>` shows one service
- `start <service-name>` starts a service
- `stop <service-name>` stops a service
- `restart <service-name>` restarts a service
- `enable <service-name>` enables a service
- `disable <service-name>` disables a service

## Debugging guide

If a service does not start:

1. Check `svcinfo <name>`.
2. Confirm the service is enabled.
3. Check the log buffer with `log`.

If boot behavior looks wrong, compare the service list before and after
startup by using `svc` after boot.
