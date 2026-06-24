/*
 * AcharyaOS - calc.c
 * ------------------
 * Tiny integer expression evaluator for the shell.
 */

#include "calc.h"
#include "kio.h"
#include "kstring.h"

static const char *calc_skip_spaces(const char *s) {
    while (*s == ' ') {
        s++;
    }
    return s;
}

static int calc_parse_number(const char **s, int64_t *out) {
    int64_t value = 0;
    int digit_count = 0;
    const char *p = calc_skip_spaces(*s);

    if (*p < '0' || *p > '9') {
        return -1;
    }

    while (*p >= '0' && *p <= '9') {
        value = (value * 10) + (int64_t)(*p - '0');
        p++;
        digit_count++;
    }

    if (digit_count == 0) {
        return -1;
    }

    *s = p;
    *out = value;
    return 0;
}

static int calc_apply_op(int64_t left, int64_t right, char op, int64_t *out) {
    switch (op) {
        case '+':
            *out = left + right;
            return 0;
        case '-':
            *out = left - right;
            return 0;
        case '*':
            *out = left * right;
            return 0;
        case '/':
            if (right == 0) {
                return -1;
            }
            *out = left / right;
            return 0;
        case '%':
            if (right == 0) {
                return -1;
            }
            *out = left % right;
            return 0;
        default:
            return -1;
    }
}

static int calc_parse_factor(const char **s, int64_t *out);
static int calc_parse_term(const char **s, int64_t *out);
static int calc_parse_expr(const char **s, int64_t *out);

static int calc_parse_factor(const char **s, int64_t *out) {
    const char *p = calc_skip_spaces(*s);

    if (*p == '(') {
        int64_t inner = 0;
        p++;
        if (calc_parse_expr(&p, &inner) != 0) {
            return -1;
        }
        p = calc_skip_spaces(p);
        if (*p != ')') {
            return -1;
        }
        p++;
        *s = p;
        *out = inner;
        return 0;
    }

    if (*p == '+' || *p == '-') {
        char sign = *p++;
        int64_t value = 0;
        if (calc_parse_factor(&p, &value) != 0) {
            return -1;
        }
        *s = p;
        *out = (sign == '-') ? -value : value;
        return 0;
    }

    if (calc_parse_number(&p, out) != 0) {
        return -1;
    }

    *s = p;
    return 0;
}

static int calc_parse_term(const char **s, int64_t *out) {
    int64_t left = 0;
    int64_t right = 0;
    char op;
    const char *p = *s;

    if (calc_parse_factor(&p, &left) != 0) {
        return -1;
    }

    for (;;) {
        p = calc_skip_spaces(p);
        op = *p;
        if (op != '*' && op != '/' && op != '%') {
            break;
        }
        p++;
        if (calc_parse_factor(&p, &right) != 0) {
            return -1;
        }
        if (calc_apply_op(left, right, op, &left) != 0) {
            return -1;
        }
    }

    *s = p;
    *out = left;
    return 0;
}

static int calc_parse_expr(const char **s, int64_t *out) {
    int64_t left = 0;
    int64_t right = 0;
    char op;
    const char *p = *s;

    if (calc_parse_term(&p, &left) != 0) {
        return -1;
    }

    for (;;) {
        p = calc_skip_spaces(p);
        op = *p;
        if (op != '+' && op != '-') {
            break;
        }
        p++;
        if (calc_parse_term(&p, &right) != 0) {
            return -1;
        }
        if (calc_apply_op(left, right, op, &left) != 0) {
            return -1;
        }
    }

    *s = p;
    *out = left;
    return 0;
}

int calc_evaluate(const char *expression, int64_t *out_value) {
    const char *p = expression;
    int64_t value = 0;

    if (!expression || !out_value) {
        return -1;
    }

    if (calc_parse_expr(&p, &value) != 0) {
        return -1;
    }

    p = calc_skip_spaces(p);
    if (*p != '\0') {
        return -1;
    }

    *out_value = value;
    return 0;
}

void calc_get_help(void) {
    kprintf("Calculator usage:\n");
    kprintf("  calc <expression>\n");
    kprintf("Supported operators: + - * / %% and parentheses\n");
    kprintf("Examples:\n");
    kprintf("  calc 2+2\n");
    kprintf("  calc (10+5)*3\n");
}
