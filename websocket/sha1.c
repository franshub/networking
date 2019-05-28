/* sha1.c */

#include <stdlib.h>
//#include <stdio.h>
#include <string.h>

/* Size of one chunk, in bytes. */
#define CHUNK_SIZE 64

struct h {
    unsigned long h0;
    unsigned long h1;
    unsigned long h2;
    unsigned long h3;
    unsigned long h4;
};

/**
 * Writes value to buffer, in size bytes, most significant byte first.
 */
static void write_int(unsigned char *buffer, unsigned long value, int size) {
    for (int i = size - 1; i >= 0; i--) {
        buffer[i] = value & (unsigned char) 0xff;
        value >>= 8;
    }
}

/**
 * Reads value from buffer, in size bytes, most significant byte
 * first.
 */
static unsigned long read_int(unsigned char *buffer, int size) {
    unsigned long word = 0;
    for (int i = 0; i < size; i++) {
        word <<= 8;
        word |= buffer[i];
    }
    return word;
}

/**
 * Produces the final hash value.
 */
static unsigned char *format_hash(struct h *h) {
    unsigned char *result = (unsigned char *) malloc(20);

    write_int(result +  0, h->h0, 4);
    write_int(result +  4, h->h1, 4);
    write_int(result +  8, h->h2, 4);
    write_int(result + 12, h->h3, 4);
    write_int(result + 16, h->h4, 4);

    return result;
}

/**
 * Rotates value (a 32 bit unsigned integer, most significant bit
 * first) count bits to the left.
 */
static unsigned long rotate_left(unsigned long value, int count) {
    value &= (unsigned long) 0xffffffff;
    unsigned long result = (value << count) | (value >> (32 - count));
}

/**
 * Processes one chunk.  Reads and updates the values in *h.
 */
static void process_chunk(struct h *h, unsigned char *chunk) {
    const int WORD_SIZE = 4;
    const int WORD_COUNT = 80;
    unsigned long words[WORD_COUNT];
    for (int i = 0; i < CHUNK_SIZE / WORD_SIZE; i++) {
        words[i]= read_int(chunk + i * WORD_SIZE, WORD_SIZE);
    }
    for (int i = CHUNK_SIZE / WORD_SIZE; i < WORD_COUNT; i++) {
        words[i] = rotate_left(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
    }

    unsigned long a = h->h0;
    unsigned long b = h->h1;
    unsigned long c = h->h2;
    unsigned long d = h->h3;
    unsigned long e = h->h4;

    for (int i = 0; i < WORD_COUNT; i++) {
        unsigned long f;
        unsigned long k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }
        unsigned long tmp = rotate_left(a, 5) + f + e + k + words[i];
        e = d;
        d = c;
        c = rotate_left(b, 30);
        b = a;
        a = tmp;
    }

    h->h0 += a;
    h->h1 += b;
    h->h2 += c;
    h->h3 += d;
    h->h4 += e;
}

/**
 * Writes the addendum to buffer.  The message_size and addendum_size
 * are in bytes.
 */
static void fill_addendum(unsigned char *buffer, int message_size, int addendum_size) {
    /* See calculate_addendum_size(). */
    memset(buffer, 0, addendum_size);
    buffer[0] = (unsigned char) 0x80;
    write_int(buffer + addendum_size - 8, 8 * message_size, 8);
}

/**
 * Returns the size of the addendum.  The message_size and the
 * returned value are in bytes.
 */
static int calculate_addendum_size(unsigned long message_size) {
    /* The addendum starts with a '1' bit and ends with the message
     * size (in bits) in 64 bits.  There are '0' bits between the '1'
     * bit and the message size, so that the total message size is a
     * multiple of the chunk size. */

    unsigned long original_size_in_bits = 8 * message_size;
    unsigned long total_size_in_bits = original_size_in_bits + 1 + 64;
    total_size_in_bits += 8 * CHUNK_SIZE - total_size_in_bits % (8 * CHUNK_SIZE);
    return total_size_in_bits / 8 - message_size;
}

static void initialise_h(struct h *h) {
    h->h0 = 0x67452301;
    h->h1 = 0xefcdab89;
    h->h2 = 0x98badcfe;
    h->h3 = 0x10325476;
    h->h4 = 0xc3d2e1f0;
}

unsigned char *sha1(unsigned char *message, unsigned long message_size) {
    struct h h;
    initialise_h(&h);

    unsigned long chunk_count = message_size / CHUNK_SIZE;
    for (unsigned long i = 0; i < chunk_count; i++) {
        process_chunk(&h, message + i * CHUNK_SIZE);
    }

    unsigned char last_chunks[2 * CHUNK_SIZE];
    int remaining_size = message_size % CHUNK_SIZE;
    int addendum_size = calculate_addendum_size(message_size);
    if (remaining_size > 0) {
        memcpy(last_chunks, message + chunk_count * CHUNK_SIZE, remaining_size);
    }
    fill_addendum(last_chunks + remaining_size, message_size, addendum_size);
    process_chunk(&h, last_chunks);
    if (remaining_size + addendum_size > CHUNK_SIZE) {
        process_chunk(&h, last_chunks + CHUNK_SIZE);
    }

    return format_hash(&h);
}
