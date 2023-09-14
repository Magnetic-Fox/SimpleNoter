/* Taken from: */
/* utf8_decode.h */

#ifndef UTF8_DECODE_H
#define UTF8_DECODE_H

#define UTF8_END   -1
#define UTF8_ERROR -2

extern long int  utf8_decode_at_byte();
extern long int  utf8_decode_at_character();
extern void utf8_decode_init(char p[], long int length);
extern long int  utf8_decode_next();

#endif

