//=========================================================================
// vsprintf.c (C) Zhao Yanbai
//     wed, 30 Jul 2008 14:47 +0800
//     Add %012d %012x %12d %12x Support  Mon, 20 Jul 2009 19:30:34
//     Add %u Support                     Sun, 06 Jul 2014 12:07:54
//     Add %o Support                     Tue, 08 Oct 2024 22:54:36
// ========================================================================
#include <assert.h>
#include <types.h>

#include "string.h"

char *itoa(char *s, int n);
char *itou(char *s, unsigned int n);
char *itoo(char *s, unsigned int n);
char *itox(char *s, unsigned int n, int upper);
char *i64tou(char *s, uint64_t n);
char *i64too(char *s, uint64_t n);
char *i64tox(char *s, uint64_t n, int upper);

enum { ALIGN_RIGHT, ALIGN_LEFT };

int write_buf(char *buf, const char *str, char fillch, int charcnt, int align) {
    if (str == 0) return 0;

    int len = strlen(str);
    int delta_char_cnt = charcnt - len;

    int s_pos = 0;
    int c_pos = len;

    if (ALIGN_RIGHT == align) {
        s_pos = delta_char_cnt > 0 ? delta_char_cnt : 0;
        c_pos = 0;
    }

    strcpy(buf + s_pos, str);

    int i = 0;
    for (i = 0; i < delta_char_cnt; ++i) {
        buf[c_pos + i] = fillch;
    }

    return charcnt > len ? charcnt : len;
}

int vsprintf(char *buf, const char *fmt, char *args) {
    char *p = buf;
    int char_cnt;
    char tmp[64];

    while (*fmt) {
        if (*fmt != '%') {
            *p++ = *fmt++;
            continue;
        }

        fmt++;

        int align = ALIGN_RIGHT;
        if (*(fmt) == '-') {
            align = ALIGN_LEFT;
            ++fmt;
        }

        char char_fill = ' ';
        if (*(fmt) == '0' || *(fmt) == ' ') {
            char_fill = *(fmt);
            ++fmt;
        }

        char_cnt = 0;
        while (*(fmt) >= '0' && *(fmt) <= '9') {
            char_cnt += *(fmt) - '0';
            char_cnt *= 10;
            ++fmt;
        }
        char_cnt /= 10;

        switch (*fmt) {
        case 'c':
            *p++ = *args;
            break;
        case 'd':
            itoa(tmp, *((int *)args));
            p += write_buf(p, tmp, char_fill, char_cnt, align);
            break;
        case 'l':
            fmt++;
            if (*fmt == 'u' || *fmt == 'd') {  // d u都当成u来处理
                i64tou(tmp, *((int64_t *)args));
                p += write_buf(p, tmp, char_fill, char_cnt, align);
                args += 4;
            } else if (*fmt == 'o') {
                i64too(tmp, *((uint64_t *)args));
                p += write_buf(p, tmp, char_fill, char_cnt, align);
                args += 4;
            } else if (*fmt == 'x' || *fmt == 'X') {
                // i64tox(tmp, *((uint64_t *)args), *fmt == 'X' ? 1 : 0);
                i64tox(tmp, *((uint64_t *)args), 1);  // x X都强制为大写
                p += write_buf(p, tmp, char_fill, char_cnt, align);
                args += 4;
            }
            break;
        case 's':
            p += write_buf(p, (const char *)*((unsigned int *)args), char_fill, char_cnt, align);
            break;
        case 'u':
            itou(tmp, *((unsigned int *)args));
            p += write_buf(p, tmp, char_fill, char_cnt, align);
            break;
        case 'x':
        case 'X':
            // itox(tmp, *((unsigned int *)args), *fmt == 'X' ? 1 : 0);
            itox(tmp, *((unsigned int *)args), 1);  // x X都强制为大写
            p += write_buf(p, tmp, char_fill, char_cnt, align);
            break;
        case 'o':
            itoo(tmp, *((unsigned *)args));
            p += write_buf(p, tmp, char_fill, char_cnt, align);
            break;
        default:
            break;
        }
        args += 4;
        fmt++;
    }
    *p = 0;

    return p - buf;
}

void swap_char(char *a, char *b) {
    char c;
    c = *a;
    *a = *b;
    *b = c;
}

char *itoa(char *s, int n) {
    int i = 0;
    char *p = 0;

    if (n & 0x80000000) {
        n = ~n + 1;
        *s++ = '-';
    }

    p = s;

    do {
        *p++ = (n % 10) + '0';
        n /= 10;
    } while (n);

    *p-- = 0;

    while (s < p) {
        swap_char(s, p);
        s++;
        p--;
    }
}

char *i64tou(char *s, uint64_t n) {
    itou(s, n >> 32);
    int i = 0;
    if ((n >> 32) > 0) {
        i = strlen(s);
    }
    itou(s + i, n & 0xFFFFFFFF);
}

char *itou(char *s, unsigned int n) {
    char c;
    char *p = s;

    do {
        *p++ = (n % 10) + '0';
        n /= 10;
    } while (n);

    *p-- = 0;

    while (s < p) {
        swap_char(s, p);
        s++;
        p--;
    }
}

char *_itoo(char *s, uint64_t n, int bgn) {
    char *p = s;
    char ch;
    int i;
    bool flag = false;

    for (i = bgn; i >= 0; i -= 3) {
        ch = (n >> i) & 0x07;

        assert(ch >= 0);
        assert(ch <= 7);

        ch += '0';

        if (ch != '0') {
            flag = true;
        }

        if (flag || ch != '0') {
            *p++ = ch;
        }
    }

    if (s == p) {
        *p++ = '0';
    }

    *p = 0;

    return s;
}

char *itoo(char *s, unsigned int n) {
    //
    return _itoo(s, n, 30);
}

char *i64too(char *s, uint64_t n) {
    //
    return _itoo(s, n, 63);
}

char *_itox(char *s, uint64_t n, int upper, int bgn) {
    char *p = s;
    char ch;
    int i;
    bool flag = false;

    for (i = bgn; i >= 0; i -= 4) {
        ch = (n >> i) & 0x0F;

        if (ch >= 0 && ch <= 9) {
            ch += '0';
        } else {
            ch -= 10;
            ch += upper == 1 ? 'A' : 'a';
        }

        if (ch != '0') {
            flag = true;
        }

        if (flag || ch != '0') {
            *p++ = ch;
        }
    }

    if (s == p) {
        *p++ = '0';
    }

    *p = 0;

    return s;
}

char *itox(char *s, unsigned int n, int upper) {
    //
    return _itox(s, n, upper, 28);
}

char *i64tox(char *s, uint64_t n, int upper) {
    //
    return _itox(s, n, upper, 60);
}
