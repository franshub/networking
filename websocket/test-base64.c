/* test-base64.c */

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "base64.h"

struct test  {
    unsigned char *message;
    unsigned char *encoded;
};

int main(int argc, char *argv[]) {
    printf("Testing base-64 encoding and decoding.\n");

    struct test messages[] = {
        { "",       "" },
        { "f",      "Zg==" },
        { "fo",     "Zm8=" },
        { "foo",    "Zm9v" },
        { "foob",   "Zm9vYg==" },
        { "fooba",  "Zm9vYmE=" },
        { "foobar", "Zm9vYmFy" }
    };

    for (int i = 0; i < sizeof(messages) / sizeof(*messages); i++) {
        unsigned char *message = messages[i].message;
        printf("vector \"%s\" encodes to ", message);

        unsigned char *encoded = base64_encode(message, strlen(message));
        if (encoded == NULL) {
            printf("NULL (BAD)\n");
            continue;
        }

        printf("\"%s\"", encoded);
        if (strcmp(encoded, messages[i].encoded) != 0) {
            printf(" (BAD), expected \"%s\"\n", messages[i].encoded);
            free(encoded);
            continue;
        }

        printf(" (GOOD) decodes back to ");
        unsigned long size = 0;
        unsigned char *decoded = base64_decode(encoded, &size);
        free(encoded);
        if (decoded == NULL) {
            printf("NULL (BAD)\n");
            continue;
        }
        printf("%lu bytes", size);
        if (size != strlen(message)) {
            printf(" (BAD), expected %d\n", strlen(message));
            free(decoded);
            continue;
        }
        printf(": \"");
        for (int j = 0; j < size; j++) {
            if (isprint(decoded[j])) {
                printf("%c", decoded[j]);
            } else {
                printf("\\x%02.2x", decoded[j]);
            }
        }
        printf("\"");
        if (memcmp(decoded, message, size) != 0) {
            printf(" (BAD), expected '%s''\n", message);
            free(decoded);
            continue;
        }

        free(decoded);
        printf(" (GOOD)\n");
    }

    return 0;
}
