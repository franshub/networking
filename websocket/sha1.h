/* sha1.h */

/*
 * Returns the 20 byte SHA-1 hash of the message.  The message_size is
 * in bytes.  The returned value is malloc()-ed and must be freed by
 * the user.
 */
unsigned char *sha1(unsigned char *message, unsigned long message_size);
