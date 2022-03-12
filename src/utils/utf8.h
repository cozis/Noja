#ifndef XUTF8_H
#define XUTF8_H
#include <stdint.h> // uint32_t
int utf8_sequence_from_utf32_codepoint(char *utf8_data, int nbytes, uint32_t utf32_code);
int utf8_sequence_to_utf32_codepoint(const char *utf8_data, int nbytes, uint32_t *utf32_code);
int utf8_strlen(const char *utf8_data, int nbytes);
int utf8_prev(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code);
int utf8_next(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code);
int utf8_curr(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code);
#endif // #ifndef UTF8_H