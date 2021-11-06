//=========================================================================
// lib/string.c (C) Zhao Yanbai
//     wed, 30 Jul 2008 15:22 +0800
//=========================================================================

#include "string.h"

char *strcpy(char *dest, const char *src) {
    char *p = dest;
    while ((*dest++ = *src++))
        ;
    return p;
}

char *strncpy(char *dst, const char *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dst[i] = src[i];
        if (src[i] == 0) {
            break;
        }
    }

    return dst;
}

size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t i = 0;

    if (size == 0) {
        return 0;
    }

    for (i = 0; i < size - 1; i++) {
        dst[i] = src[i];
        if (src[i] == 0) {
            break;
        }
    }

    dst[i] = 0;

    return i;
}

size_t strlen(const char *str) {
    int i = 0;
    while (*str++) i++;
    return i;
}

int strcmp(const char *a, const char *b) {
    int delta;
    while (*a || *b) {
        delta = *a++ - *b++;
        if (delta != 0) return delta;
    }
    return 0;
}

int strncmp(const char *a, const char *b, size_t count) {
    unsigned char c1, c2;
    int delta;
    while (count) {
        c1 = *a++;
        c2 = *b++;

        delta = c1 - c2;
        if (delta != 0) return delta;

        if (c1 == 0) break;

        count--;
    }

    return 0;
}

char *strcat(char *dest, const char *src) {
    char *tmp = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++) != '\0')
        ;
    return tmp;
}
void *memcpy(void *dest, const void *src, size_t size) {
    char *d = (char *)dest;
    char *s = (char *)src;
    while (size-- > 0) {
        *d = *s;
        d++;
        s++;
    }
    return dest;
}

void memset(void *dest, char ch, size_t size) {
    char *p = (char *)dest;
    for (size_t i = 0; i < size; i++) {
        *p = ch;
        p++;
    }
}

int memcmp(const void *a, const void *b, size_t count) {
    const unsigned char *sa, *sb;
    int delta = 0;
    for (sa = a, sb = b; count > 0; ++sa, ++sb, --count)
        if ((delta = *sa - *sb) != 0) break;
    return delta;
}

char *strstr(const char *a, const char *b) {
    size_t la, lb;
    lb = strlen(b);
    if (lb == 0) return (char *)a;
    la = strlen(a);
    while (la >= lb) {
        la--;
        if (memcmp(a, b, lb) == 0) return (char *)a;
        a++;
    }

    return 0;
}
