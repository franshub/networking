#include <stdio.h>

static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char PAD = '=';
static int CHUNKS_PER_LINE = 19;

int main(int argc, char *argv[]) {
    /* a   => aaaaaa aa0000 ====== ======
     * ab  => aaaaaa aabbbb bbbb00 ======
     * abc => aaaaaa aabbbb bbbbcc cccccc
     */

    unsigned char chunk[3];
    int byte_count = 0;
    int chunk_count = 0;

    int ch;
    while ((ch = getchar()) != EOF) {
        chunk[byte_count] = ch;
        byte_count++;

        if (byte_count == sizeof(chunk)) {
            putchar(table[(chunk[0] >> 2) & 0x3fu]);
            putchar(table[((chunk[0] << 4) | (chunk[1] >> 4)) & 0x3fu]);
            putchar(table[((chunk[1] << 2) | (chunk[2] >> 6)) & 0x3fu]);
            putchar(table[chunk[2] & 0x3fu]);
            byte_count = 0;

            /* We violate section 3.1 of RFC4648 here. */
            chunk_count++;
            if (chunk_count == CHUNKS_PER_LINE) {
                putchar('\n');
                chunk_count = 0;
            }
        }
    }

    if (byte_count > 0) {
        putchar(table[(chunk[0] >> 2) & 0x3fu]);
        if (byte_count == 1) {
            putchar(table[(chunk[0] << 4) & 0x30u]);
            putchar(PAD);
        } else {
            putchar(table[((chunk[0] << 4) | (chunk[1] >> 4)) & 0x3fu]);
            putchar(table[(chunk[1] << 2) & 0x3cu]);
        }
        putchar(PAD);
        chunk_count++;
    }

    if (chunk_count > 0) {
        putchar('\n');
    }

    return 0;
}
