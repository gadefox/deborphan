/* file.c - Functions to read the status file for deborphan.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader
   Copyright (C) 2008 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "deborphan.h"

/* This uses up as much memory as your status file (near a
 * megabyte). It is considerably faster, though.
 */
char* debopen(const char* filename) {
    int fd;
    char* buf;
    struct stat statbuf;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return NULL;

    if (fstat(fd, &statbuf) < 0) {
        close(fd);
        return NULL;
    }

    buf = (char*)malloc((size_t)(statbuf.st_size + 1));

    if (read(fd, buf, (size_t)statbuf.st_size) < statbuf.st_size) {
        close(fd);
        free(buf);
        return NULL;
    }

    buf[statbuf.st_size] = '\0';
    close(fd);

    return buf;
}

int zerofile(const char* filename) {
    int fd = open(filename, O_WRONLY | O_TRUNC);
    if (fd < 0) {
        if (errno == ENOENT) /* It not existing at all is fine too */
            return 0;
        else
            return -1;
    }
    close(fd);
    return 0;
}
