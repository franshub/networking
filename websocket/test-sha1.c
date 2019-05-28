#include "sha1.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char *sha1_to_string(unsigned char *s) {
    static char buf[2 * 20 + 1];
    for (int i = 0; i < 20; i++) {
        sprintf(buf + 2 * i, "%02.2x", s[i]);
    }
    return buf;
}

struct test  {
    unsigned char *message;
    unsigned char *sha1;
};

int main(int argc, char *argv[]) {
    printf("Testing SHA-1.\n");

    struct test messages[] = {
        { "The quick brown fox jumps over the lazy dog", "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12" },
        { "The quick brown fox jumps over the lazy cog", "de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3" },
        { "", "da39a3ee5e6b4b0d3255bfef95601890afd80709" }
    };

    for (int i = 0; i < sizeof(messages) / sizeof(*messages); i++) {
        unsigned char *message = messages[i].message;
        printf("Input: \"%s\"\n", message);
        unsigned char *result = sha1(message, strlen(message));
        printf("Actual result:   %s\n", sha1_to_string(result));
        printf("Expected result: %s\n", messages[i].sha1);
        if (strcmp(sha1_to_string(result), messages[i].sha1) == 0) {
            printf("GOOD\n");
        } else {
            printf("BAD\n");
        }
        free(result);
    }
}
