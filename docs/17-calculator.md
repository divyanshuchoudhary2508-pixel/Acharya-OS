# AcharyaOS Phase 1, Feature 17 - Calculator

## What this subsystem does

Feature 17 adds a tiny integer calculator that evaluates expressions from
the kernel shell. It supports:

- addition
- subtraction
- multiplication
- division
- remainder
- parentheses
- unary `+` and `-`

## Dependencies

This subsystem depends on:

- the shell, to accept the `calc` command
- `kio`, for output
- `kstring`, for small string helpers

It does not depend on the filesystem, scheduler, or user space.

## Architecture

The calculator uses a simple recursive-descent parser:

- `expr` handles `+` and `-`
- `term` handles `*`, `/`, and `%`
- `factor` handles numbers, parentheses, and unary signs

This keeps operator precedence correct without introducing a big parser
framework.

## Folder structure

```text
kernel/include/calc.h
kernel/calc/calc.c
docs/17-calculator.md
```

## Debugging guide

If the calculator returns an error:

1. Check the expression syntax.
2. Make sure parentheses are balanced.
3. Avoid division by zero.

If the result looks wrong, remember the calculator works with signed
integers only, so fractional results are truncated.
