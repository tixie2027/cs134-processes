#include "util/string.h"

#include <stddef.h> // for NULL

/*
 * Copy the string
 */
void strncpy(char *dest, const char *src, long len) {
    if (len == 0) {
        return;
    }

    long i;
    for (i = 0; i < len - 1 && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }

    dest[i] = '\0';
}

/*
 * Return the length of a string
 */
long strlen(const char *str) {
    long length = 0;

    while (str[length] != '\0') {
        length++;
    }

    return length;
}

/*
 * Find the first occurrence of the substring needle in the string haystack
 *
 * Return a pointer to the beginning of the located substring, or NULL if the
 * substring is not found
 */
char *strstr(const char *haystack, const char *needle) {
    long haystack_len = strlen(haystack);
    long needle_len = strlen(needle);

    // Needle cannot be longer than the haystack
    if (needle_len > haystack_len) {
        return NULL;
    }

    // Find the first substring
    for (long i = 0; i < (haystack_len - needle_len + 1); ++i) {
        long j = 0;

        while (j < needle_len && haystack[i + j] == needle[j]) {
            j++;
        }

        if (j == needle_len) {
            return (char *)(haystack + i);
        }
    }

    // No substring found
    return NULL;
}

/*
 * Append the src string to the dest string (use at most n bytes from src)
 */
char *strncat(char *dest, const char *src, long n) {
    // n has to be a positive number
    if (n < 1) {
        return dest;
    }

    long dest_len = strlen(dest);
    long i;

    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

/*
 * Compare the two strings s1 and s2 up to the first n bytes/characters
 *
 * Return an integer less than, equal to, or greater than zero if s1 (or the
 * first  n  bytes  thereof) is found, respectively, to be less than, to match,
 * or be greater than s2.
 */
int strncmp(const char *s1, const char *s2, long n) {
    for (long i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }

        if (s1[i] == '\0') {
            return 0;
        }
    }

    return 0;
}
