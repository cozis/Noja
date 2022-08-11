
/* +--------------------------------------------------------------------------+
** |                          _   _       _                                   |
** |                         | \ | |     (_)                                  |
** |                         |  \| | ___  _  __ _                             |
** |                         | . ` |/ _ \| |/ _` |                            |
** |                         | |\  | (_) | | (_| |                            |
** |                         |_| \_|\___/| |\__,_|                            |
** |                                    _/ |                                  |
** |                                   |__/                                   |
** +--------------------------------------------------------------------------+
** | Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>       |
** +--------------------------------------------------------------------------+
** | This file is part of The Noja Interpreter.                               |
** |                                                                          |
** | The Noja Interpreter is free software: you can redistribute it and/or    |
** | modify it under the terms of the GNU General Public License as published |
** | by the Free Software Foundation, either version 3 of the License, or (at |
** | your option) any later version.                                          |
** |                                                                          |
** | The Noja Interpreter is distributed in the hope that it will be useful,  |
** | but WITHOUT ANY WARRANTY; without even the implied warranty of           |
** | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General |
** | Public License for more details.                                         |
** |                                                                          |
** | You should have received a copy of the GNU General Public License along  |
** | with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.   |
** +--------------------------------------------------------------------------+ 
*/

#include <assert.h>
#include <stddef.h> // NULL
#include "utf8.h"

// If this is turned on, these functions will assume
// the UTF-8 strings will mainly contain ASCII characters.
#define ASSUME_ASCII 1

/* SYMBOL
**   utf8_sequence_from_utf32_codepoint
**
** DESCRIPTION
**   Transform a UTF-32 encoded codepoint to a UTF-8 encoded byte sequence.
**
** ARGUMENTS
**   The [utf8_data] pointer refers to the location where the UTF-8 sequence
**   will be stored.
**
**   The [nbytes] argument specifies the maximum number of bytes that can
**   be written to [utf8_data]. It can't be negative.
**
**   The [utf32_code] argument is the UTF-32 code that will be converted.
**
** RETURN
**   If [utf32_code] is valid UTF-32 and the provided buffer is big enough, 
**   the UTF-8 equivalent sequence is stored in [utf8_data]. No more than
**   [nbytes] are ever written. If one of those conitions isn't true, -1 is
**   returned.
*/
int utf8_sequence_from_utf32_codepoint(char *utf8_data, int nbytes, uint32_t utf32_code)
{
    if(utf32_code < 128)
    {
        if(nbytes < 1)
            return -1;

        utf8_data[0] = utf32_code;
        return 1;
    }

    if(utf32_code < 2048)
    {
        if(nbytes < 2)
            return -1;

        utf8_data[0] = 0xc0 | (utf32_code >> 6);
        utf8_data[1] = 0x80 | (utf32_code & 0x3f);
        return 2;
    }

    if(utf32_code < 65536)
    {
        if(nbytes < 3)
            return -1;

        utf8_data[0] = 0xe0 | (utf32_code >> 12);
        utf8_data[1] = 0x80 | ((utf32_code >> 6) & 0x3f);
        utf8_data[2] = 0x80 | (utf32_code & 0x3f);
        return 3;
    }

    if(utf32_code <= 0x10ffff)
    {
        if(nbytes < 4)
            return -1;

        utf8_data[0] = 0xf0 | (utf32_code >> 18);
        utf8_data[1] = 0x80 | ((utf32_code >> 12) & 0x3f);
        utf8_data[2] = 0x80 | ((utf32_code >>  6) & 0x3f);
        utf8_data[3] = 0x80 | (utf32_code & 0x3f);
        return 4;
    }

    // Code is out of range for UTF-8.
    return -1;
}

/* SYMBOL
**   utf8_sequence_to_utf32_codepoint
**
** DESCRIPTION
**   Transform a UTF-8 encoded byte sequence pointed by `utf8_data`
**   into a UTF-32 encoded codepoint.
**
** ARGUMENTS
**   The [utf8_data] pointer refers to the location of the UTF-8 sequence.
**
**   The [nbytes] argument specifies the maximum number of bytes that can
**   be read after [utf8_data]. It can't be negative. 
**
**   NOTE: The [nbytes] argument has no relation to the UTF-8 byte count sequence. 
**         You may think about this argument as the "raw" string length (the one 
**         [strlen] whould return if [utf8_data] were zero-terminated). 
**
**   The [utf32_code] argument is the location where the encoded UTF-32 code
**   will be stored. It may be NULL, in which case the value is evaluated and then
**   thrown away.
**
** RETURN
**   The codepoint is returned through the output parameter `utf32_code`.
**   The returned value is the number of bytes of the UTF-8 sequence that
**   were scanned to encode the UTF-32 code, or -1 if the UTF-8 sequence
**   is invalid.
**
** NOTE: By calling this function with a NULL [utf32_code], you can check the
**       validity of a UTF-8 sequence.
*/
int utf8_sequence_to_utf32_codepoint(const char *utf8_data, int nbytes, uint32_t *utf32_code)
{
    assert(utf8_data != NULL);
    assert(nbytes >= 0);

    uint32_t dummy;
    if(utf32_code == NULL)
        utf32_code = &dummy;

    if(nbytes == 0)
        return -1;

    if(utf8_data[0] & 0x80)
    {
        // May be UTF-8.
            
        if((unsigned char) utf8_data[0] >= 0xF0)
            {
                // 4 bytes.
                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

                if(nbytes < 4)
                    return -1;
                    
                uint32_t temp 
                    = (((uint32_t) utf8_data[0] & 0x07) << 18) 
                    | (((uint32_t) utf8_data[1] & 0x3f) << 12)
                    | (((uint32_t) utf8_data[2] & 0x3f) <<  6)
                    | (((uint32_t) utf8_data[3] & 0x3f));

                if(temp > 0x10ffff)
                    return -1;

                *utf32_code = temp;
                return 4;
            }
            
            if((unsigned char) utf8_data[0] >= 0xE0)
            {
                // 3 bytes.
                // 1110xxxx 10xxxxxx 10xxxxxx

                if(nbytes < 3)
                    return -1;

                uint32_t temp
                    = (((uint32_t) utf8_data[0] & 0x0f) << 12)
                    | (((uint32_t) utf8_data[1] & 0x3f) <<  6)
                    | (((uint32_t) utf8_data[2] & 0x3f));
                
                if(temp > 0x10ffff)
                    return -1;

                *utf32_code = temp;
                return 3;
            }
            
            if((unsigned char) utf8_data[0] >= 0xC0)
            {
                // 2 bytes.
                // 110xxxxx 10xxxxxx

                if(nbytes < 2)
                    return -1;

                *utf32_code 
                    = (((uint32_t) utf8_data[0] & 0x1f) << 6)
                    | (((uint32_t) utf8_data[1] & 0x3f));
                    
                assert(*utf32_code <= 0x10ffff);
                return 2;
            }
            
        // 1 byte
        // 10xxxxxx
        *utf32_code = (uint32_t) utf8_data[0] & 0x3f;
        return 1;
    }

    // It's ASCII
    // 0xxxxxxx

    *utf32_code = (uint32_t) utf8_data[0];
    return 1;
}

/* SYMBOL
**   utf8_strlen
**
** DESCRIPTION
**   Count the number of characters of a UTF-8 string. 
**
**   NOTE: By "character" we mean a valid UTF-8 sequence. 
**
** ARGUMENTS
**   The [utf8_data] pointer refers to the location of the UTF-8 string.
**
**   The [nbytes] argument specifies the byte count of the string referred
**   by [utf8_data]. It can't be negative. 
**
** RETURN
**   Returns the number of characters encoded by [utf8_data], or -1 if
**   the string is not valid UTF-8.
**
** NOTE: By calling this function on an ASCII-only string, the return
**       value is equal to [nbytes].
**
** NOTE: You can check the validity of a UTF-8 string
**       by calling this function and checking that it's
**       return value is not negative.
*/
int utf8_strlen(const char *utf8_data, int nbytes)
{
    assert(utf8_data != NULL);
    assert(nbytes >= 0);

    int len = 0;

    int i = 0;
    while(i < nbytes)
    {

#if ASSUME_ASCII
        {
            int ASCII_start = i;

            // Skip through ASCII
            while(i < nbytes && (utf8_data[i] & 0x80) == 0)
                i += 1;

            int ASCII_end = i;

            len += (ASCII_end - ASCII_start);

            // Either we scanned through all of the
            // string, or we encountered some unicode.

            if(i == nbytes)
                // String ended.
                break;
        }
#endif

        // Found unicode.
        {
            int n = utf8_sequence_to_utf32_codepoint(utf8_data + i, nbytes - i, NULL);

            if(n < 1)
                return -1;

            i += n;
            len += 1;
        }
    }
    return len;
}

/* SYMBOL
**   utf8_prev
**
** DESCRIPTION
**   Get the UTF-8 sequence that comes before a given byte index
**   inside a given string.
**   
**   NOTE: This is what you use when you want to iterate over a
**         UTF-8 string backwards.
**
** ARGUMENTS
**   The [utf8_data] pointer refers to the location of the UTF-8 string.
**
**   The [nbytes] argument is the raw size of the [utf8_data] string.
**   It can't be negative. 
**
**   The [idx] argument is the index of the byte that follows the UTF-8 
**   sequence to be decoded.
**
**   The [utf32_code] argument, if not NULL, is used to return the UTF-32
**   version of the decoded UTF-8 sequence.
**
** RETURN
**   Returns the index of the first byte of the decoded UTF-8 sequence,
**   or -1 is the sequence wasn't valid UTF-8.
**
**   NOTE: If the function didn't fail, by subtracting the returned value
**         from [idx], you'll get the number of bytes of the decoded 
**         sequence.
*/
int utf8_prev(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code)
{
    assert(idx >= 0);
    assert(idx <= nbytes);

    // [idx] currently refers to the head byte
    // of a UTF-8 sequence. We need to first
    // get to the last byte of the previous
    // sequence.
    idx -= 1;

    if(idx == -1)
        // There was no previous sequence!
        return 0; // Return the same index that was provided.

    int tail = idx;

#if ASSUME_ASCII
    {
        // This block isn't necessary for
        // this function to work but it
        // makes strings that are mainly ascii
        // to go faster.

        if((utf8_data[tail] & 0x80) == 0)
        {
            if(utf32_code)
                *utf32_code = utf8_data[tail];
            return tail;
        }
    }
#endif

    // Skip all of the auxiliary bytes in the
    // form '10xxxxxx'.

    while(idx > -1 && (utf8_data[idx] & 0xc0) == 0x80)
        idx -= 1;

    if(idx == -1)
    {
        // No head sequence byte was found,
        // so this isn't valid UTF-8.
        return -1;
    }

    // The index of the head byte.
    int head = idx;

    // The number of auxiliary bytes is given
    // by the difference
    int aux = tail - head;

    // The total number of bytes of the
    // sequence is [aux + 1].

    int n = utf8_sequence_to_utf32_codepoint(utf8_data + head, aux + 1, utf32_code);

    if(n < 1)
        // The sequence wasn't valid UTF-8.
        return -1;

    assert(n > 0);

    if(n < aux + 1)
        // Not all of the auxiliary bytes were considered while parsing.
        return -1;

    assert(n == aux + 1);

    return head;
}

/* SYMBOL
**   utf8_next
**
** DESCRIPTION
**   Get the UTF-8 sequence from a UTF-8 string that starts AFTER the
**   sequence that starts at a given byte index.
**   
**   NOTE: This is what you use when you want to iterate over a
**         UTF-8 string.
**
** ARGUMENTS
**   The [utf8_data] pointer refers to the location of the UTF-8 string.
**
**   The [nbytes] argument is the raw size of the [utf8_data] string.
**   It can't be negative. 
**
**   The [idx] argument is the index of the first byte of the sequence 
**   that comes before the sequence to be decoded.
**
**   The [utf32_code] argument, if not NULL, is used to return the UTF-32
**   version of the decoded UTF-8 sequence.
**
** RETURN
**   Returns the index of the first byte of the decoded UTF-8 sequence,
**   or -1 is the sequence wasn't valid UTF-8.
**
**   NOTE: If the function didn't fail, by subtracting [idx] from the 
**         returned value, you'll get the number of bytes of the decoded 
**         sequence.
*/
int utf8_next(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code)
{
    // Get the byte count of the current sequence.
    int n = utf8_sequence_to_utf32_codepoint(utf8_data + idx, nbytes, NULL);
    
    if(n < 1)
        return -1;

    // Now get the codepoint of the next sequence.
    int k = utf8_sequence_to_utf32_codepoint(utf8_data + idx + n, nbytes, utf32_code);

    if(k < 1)
        return -1;

    return idx + n;
}

int utf8_curr(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code)
{
    assert(idx >= 0);
    assert(idx < nbytes);

    int n = utf8_sequence_to_utf32_codepoint(utf8_data + idx, nbytes - idx, utf32_code);

    return n > 0;
}