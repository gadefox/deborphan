/* string.c - some string handling functions.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004, 2006 Peter Palfrader

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/

#include <string.h>

#include "config.h"
#include "deborphan.h"

/* Simple function to hash a string to an unsigned int.
 */
unsigned int strhash(const char* line) {
    unsigned int r = 0;

    do {
        r ^= *line;
        r <<= 1;
    } while (*(line++));

    return r;
}

/* This function removes all occurences of the character 'c' from the
   string 's'.
*/
void strstripchr(char* s, int c) {
    register char* t;

    s = strchr(s, c);
    if (!(s && *s))
        return;

    t = s;

    do {
        if (*s != c)
            *(t++) = *s;
    } while (*(s++));
}

int string_to_priority(const char* priority) {
    switch (upcase(*priority)) {
        case 'R': /*equired */
            return 1;
        case 'I': /*mportant */
            return 2;
        case 'S': /*tandard */
            return 3;
        case 'O': /*ptional */
            return 4;
        case 'E': /*xtra */
            return 5;
        default: /*unknown/error */
            return 0;
    }
}

const char* priority_to_string(int priority) {
    static const char* priorities[] = {"unknown",  "required", "important",
                                       "standard", "optional", "extra"};

    return priorities[(priority > 5 || priority < 0) ? 0 : priority];
}
