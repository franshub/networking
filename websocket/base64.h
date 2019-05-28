/* base64.h */

/*
 * Returns the message encoded in base64.  The message a character
 * array, not a '\0' terminated string.  The returned value is a '\0'
 * terminated string.  The returned value is malloc()-ed and must be
 * freed by the user.  Returns NULL if encoding failed.
 */
char *base64_encode(unsigned char *message, unsigned long message_size);

/*
 * Returns the message decoded from base64.  The encoded message is a
 * '\0' terminated string.  The decoded message a character array, not
 * a '\0' terminated string.  The size of the decoded message is
 * stored in *decoded_size.  The returned value is malloc()-ed and
 * must be freed by the user.  Returns NULL if decoding failed.
 */
unsigned char *base64_decode(char *encoded_message, unsigned long *decoded_size);
