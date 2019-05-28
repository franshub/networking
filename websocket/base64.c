/* base64.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char PAD = '=';

char *base64_encode(unsigned char *message, unsigned long message_size) {
    unsigned long encoded_size = (message_size / 3) * 4;
    unsigned long remainder = message_size % 3;
    if (remainder != 0) {
        encoded_size += 4;
    }
    char *result = (char *) malloc(encoded_size + 1);
    if (result == NULL) {
        return NULL;
    }

    char *p = result;
    unsigned int i;
    for (i = 0; i < message_size - remainder; i += 3) {
        *p++ = table[(message[i] >> 2) & 0x3f];
        *p++ = table[((message[i] << 4) | (message[i + 1] >> 4)) & 0x3f];
        *p++ = table[((message[i + 1] << 2) | (message[i + 2] >> 6)) & 0x3f];
        *p++ = table[message[i + 2] & 0x3f];
    }
    if (remainder > 0) {
        *p++ = table[(message[i] >> 2) & 0x3f];
        if (remainder == 1) {
            *p++ = table[(message[i] << 4) & 0x30];
            *p++ = PAD;
            *p++ = PAD;
        } else {
            *p++ = table[((message[i] << 4) | (message[i + 1] >> 4)) & 0x3f];
            *p++ = table[(message[i + 1] << 2) & 0x3c];
            *p++ = PAD;
        }
    }
    *p = '\0';
    return result;
}

static unsigned char decode_char(char ch) {
    /* Let's not assume ASCII encoding. */
    switch (ch) {
    case 'A': return 0;
    case 'B': return 1;
    case 'C': return 2;
    case 'D': return 3;
    case 'E': return 4;
    case 'F': return 5;
    case 'G': return 6;
    case 'H': return 7;
    case 'I': return 8;
    case 'J': return 9;
    case 'K': return 10;
    case 'L': return 11;
    case 'M': return 12;
    case 'N': return 13;
    case 'O': return 14;
    case 'P': return 15;
    case 'Q': return 16;
    case 'R': return 17;
    case 'S': return 18;
    case 'T': return 19;
    case 'U': return 20;
    case 'V': return 21;
    case 'W': return 22;
    case 'X': return 23;
    case 'Y': return 24;
    case 'Z': return 25;
    case 'a': return 26;
    case 'b': return 27;
    case 'c': return 28;
    case 'd': return 29;
    case 'e': return 30;
    case 'f': return 31;
    case 'g': return 32;
    case 'h': return 33;
    case 'i': return 34;
    case 'j': return 35;
    case 'k': return 36;
    case 'l': return 37;
    case 'm': return 38;
    case 'n': return 39;
    case 'o': return 40;
    case 'p': return 41;
    case 'q': return 42;
    case 'r': return 43;
    case 's': return 44;
    case 't': return 45;
    case 'u': return 46;
    case 'v': return 47;
    case 'w': return 48;
    case 'x': return 49;
    case 'y': return 50;
    case 'z': return 51;
    case '0': return 52;
    case '1': return 53;
    case '2': return 54;
    case '3': return 55;
    case '4': return 56;
    case '5': return 57;
    case '6': return 58;
    case '7': return 59;
    case '8': return 60;
    case '9': return 61;
    case '+': return 62;
    case '/': return 63;
    }
    return 64;
}

static int decode_chunk(char *chunk, unsigned char *decoded) {
    unsigned long buffer = 0;

    for (int i = 0; i < 4; i++) {
        unsigned char value = decode_char(chunk[i]);
        if (value >= 64) {
            return 0;
        }
        buffer <<= 6;
        buffer |= value;
    }

    decoded[0] = (buffer >> 16) & 0xff;
    decoded[1] = (buffer >> 8) & 0xff;
    decoded[2] = buffer & 0xff;

    return 1;
}

static int decode_padded_chunk(char *chunk, unsigned char *decoded) {
    unsigned long buffer = 0;

    int i;
    for (i = 0; i < 4; i++) {
        if (chunk[i] == PAD) {
            break;
        }
        unsigned char value = decode_char(chunk[i]);
        if (value >= 64) {
            return 0;
        }
        buffer <<= 6;
        buffer |= value;
    }
    if (i < 4) {
        if (i < 2
            || (i == 2 && (buffer & 0x0f) != 0)
            || (i == 3 && (buffer & 0x03) != 0)) {
            return 0;
        }
        for (int j = i; j < 4; j++) {
            if (chunk[j] != PAD) {
                return 0;
            }
            buffer <<= 6;
        }
    }

    decoded[0] = (buffer >> 16) & 0xff;
    if (i >= 3) {
        decoded[1] = (buffer >> 8) & 0xff;
    }
    if (i >= 2) {
        decoded[2] = buffer & 0xff;
    }

    return 1;
}

unsigned char *base64_decode(char *encoded_message, unsigned long *decoded_size) {
    unsigned long encoded_size = strlen(encoded_message);
    if (encoded_size % 4 != 0) {
        return NULL;
    }
    unsigned long chunk_count = encoded_size / 4;
    unsigned long size = chunk_count * 3;
    if (encoded_message[encoded_size - 1] == PAD) {
        size--;
        if (encoded_message[encoded_size - 2] == PAD) {
            size--;
        }
    }
    unsigned char *result = (unsigned char *) malloc(size);
    if (result == NULL) {
        return NULL;
    }

    char *p = encoded_message;
    unsigned char *q = result;
    for (int i = 0; i < chunk_count; i++) {
        int is_padded_chunk = i == chunk_count - 1 && p[3] == PAD;
        int ok = is_padded_chunk ? decode_padded_chunk(p, q) : decode_chunk(p, q);
        if (!ok) {
            free(result);
            return NULL;
        }
        p += 4;
        q += 3;
    }

    *decoded_size = size;
    return result;
}
