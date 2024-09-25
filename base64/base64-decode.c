#include <stdio.h>

static char PAD = '=';

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

static int decode_chunk(char *chunk) {
    /* a=== -> illegal
     * ab== -> aaaaaabb
     * abc= -> aaaaaabb bbbbcccc
     * abcd -> aaaaaabb bbbbcccc ccdddddd
     */

    unsigned long buffer = 0;
    for (int i = 0; i < 4; i++) {
        buffer <<= 6;
        if (chunk[i] == PAD) {
            if (i < 2 || chunk[3] != PAD) {
                return 0;
            }
        } else {
            unsigned char sixbits = decode_char(chunk[i]);
            if (sixbits >= 64) {
                return 0;
            }
            buffer |= sixbits;
        }
    }

    putchar((buffer >> 16) & 0xffu);
    if (chunk[2] == PAD) {
        if ((buffer & 0x00ffffu) != 0) {
            return 0;
        }
    } else {
        putchar((buffer >> 8) & 0xffu);
        if (chunk[3] == PAD) {
            if ((buffer & 0x0000ffu) != 0) {
                return 0;
            }
        } else {
            putchar(buffer & 0xffu);
        }
    }

    return 1;
}

int main(int argc, char *argv[]) {
    char chunk[4];
    int count = 0;

    int ch;
    while ((ch = getchar()) != EOF) {
        /* We violate section 3.3 of RFC4648 here. */
        if (ch == '\n' || ch == '\r') {
            continue;
        }

        chunk[count] = ch;
        count++;

        if (count == sizeof(chunk)) {
            int ok = decode_chunk(chunk);
            if (!ok) {
                return -1;
            }
            count = 0;
        }
    }

    if (count != 0) {
        return -1;
    }

    return 0;
}
