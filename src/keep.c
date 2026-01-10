/* keep.c - Maintain a list of packages to keep for deborphan.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader
   Copyright (C) 2008 Andrej Tatarenkow
   Copyright (C) 2008, 2009 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.

   The general idea was borrowed from Wessel Danker's debfoster program.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "deborphan.h"

/* If package is a valid package, this function returns 0. On error, it
 * returns -1, if it is not a valid package, it returns the position in
 * the 'args' list +1.
 */
int pkggrep(const char* sfile, char** pkgnames) {
    FILE* fp;
    char *s, *t = NULL;
    int i = 0;

    if (!(fp = fopen(sfile, "r")))
        return -1;

    s = (char*)malloc(200);

    while (*pkgnames) {
        t = (char*)malloc(9 + strlen(*pkgnames));
        rewind(fp);

        strcpy(t, "ackage:");
        strcat(t, *pkgnames);
        strcat(t, "\n");

        while (fgets(s, 200, fp)) {
            if (upcase(*s) != 'P')
                continue;

            strstripchr(s, ' ');
            if (strcmp(s + 1, t) == 0) {
                free(s);
                free(t);
                fclose(fp);
                return 0;
            }
        }

        i++;
        pkgnames++;
        free(t);
    }

    free(s);
    fclose(fp);
    return i;
}

/* Read the entire keep file into memory as an array of `dep's.
 */
dep* readkeep(const char* kfile) {
    char* filecontent = debopen(kfile);
    if (filecontent == NULL)
        return NULL;

    dep *d, *ret;
    d = ret = malloc(10 * sizeof(dep));

    char* line = NULL;
    char* nextline = filecontent;

    int i = 1;
    while ((line = strsep(&nextline, "\n"))) {
        /* skip over leading blanks */
        line += strspn(line, " \t");

        /* strip arch suffix et al.*/
        char* tmp = line;
        strsep(&tmp, " \t\r\n:#");

        /* skip empty lines */
        if (line[0] == '\0') {
            continue;
        }

        d->name = strdup(line);
        d->namehash = strhash(d->name);

        d++;
        i++;
        if (i % 10 == 0) {
            ret = realloc(ret, (i + 10) * sizeof(dep));
            d = ret + i - 1;
        }
    }
    d->name = NULL;

    free(filecontent);
    return ret;
}

int mustkeep(const dep d) {
    dep* c;

    for (c = keep; c && c->name; c++) {
        if (pkgcmp(*c, d))
            return 1;
    }

    return 0;
}

int addkeep(const char* kfile, char** add) {
    int c;

    FILE* fp = fopen(kfile, "a+");
    if (fp == NULL) {
        error(EXIT_FAILURE, errno, "Opening %s", kfile);
    }

    if (fseek(fp, 0L, SEEK_END) < 0) {
        error(EXIT_FAILURE, errno, "fseek on %s", kfile);
    }

    if (ftell(fp) != 0L) {
        // ... check if the final newline is missing ...
        if (fseek(fp, -1L, SEEK_END) < 0) {
            error(EXIT_FAILURE, errno, "fseek on %s", kfile);
        }
        c = fgetc(fp);
        if (fseek(fp, 0L, SEEK_END) < 0) {
            error(EXIT_FAILURE, errno, "fseek on %s", kfile);
        }
        if (c != EOF && c != '\n') {
            // ... and add '\n' if appropriate.
            fputc('\n', fp);
        }
    }

    int i = 0;
    while (*add) {
        if (**add && **add != '\n') {
            fputs(*add, fp);
            fputc('\n', fp);
        }
        add++;
        i++;
    }

    fclose(fp);
    return i;
}

/* Sure, this function will never win the Function of the Year award, but
 * at least we got rid of the temporary files. Now all I/O is done in
 * memory.
 */
int delkeep(const char* kfile, char** del) {
    int fd, cnt;
    struct stat sbuf;
    char *fcont, *orig, *end;

    if ((fd = open(kfile, O_RDONLY | O_CREAT, 0666 /* let umask handle -w */)) <
        0)
        return -1;

    if (fstat(fd, &sbuf) < 0) {
        close(fd);
        return -1;
    }

    for (cnt = 0; del[cnt]; cnt++)
        ;

    orig = fcont = (char*)malloc(sbuf.st_size + 1);

    ssize_t actually_read = read(fd, fcont, sbuf.st_size);
    if (actually_read < 0 || (size_t)actually_read != sbuf.st_size) {
        free(fcont);
        close(fd);
        return -1;
    }
    end = fcont + sbuf.st_size;
    *end = '\0';

    char* tok;
    while ((tok = strsep(&fcont, "\n"))) {
        char* substr = tok;
        size_t substrlen;
        substr += strspn(substr, " \t");
        substrlen = strcspn(substr, " \t\r\n:#");
        if (substrlen > 0) {
            for (int i = 0; i < cnt; i++) {
                if (strlen(del[i]) != substrlen)
                    continue;
                if (strncmp(del[i], substr, substrlen) == 0) {
                    // found package to remove from keeplist.
                    memmove(tok, fcont, end - fcont + 1);
                    end -= fcont - tok;
                    fcont = tok;
                    break;
                }
            }
        }
    }
    close(fd);

    // undo what strsep() did.
    fcont = orig;
    while (1) {
        size_t len = strlen(fcont);
        if (fcont + len < end) {
            fcont[len] = '\n';
            fcont += len;
        } else {
            break;
        }
    }

    if ((fd = open(kfile, O_WRONLY | O_TRUNC)) < 0) {
        free(orig);
        return -1;
    }

    ssize_t actually_wrote = write(fd, orig, end - orig);
    if (actually_wrote < 0 || (size_t)actually_wrote != (end - orig)) {
        error(EXIT_FAILURE, errno,
              "Writing keep file failed and may be corrupt now");
        free(orig);
        close(fd);
        return -1;
    }

    free(orig);
    close(fd);
    return 0;
}

/* If something in list is found in keep, this function returns its
 * position in the list +1, else it returns 0.
 */
int hasduplicate(char** list) {
    int i;
    dep d;

    for (i = 0; list[i]; i++) {
        d.name = list[i];
        d.namehash = strhash(list[i]);
        if (mustkeep(d))
            return i + 1;
    }

    return 0;
}

dep* mergekeep(const dep* a, const dep* b) {
    unsigned int i = 0, j = 0, x = 0, y = 0;
    dep* ret;

    if (!(a || b))
        return NULL;

    if (a)
        while (a[i++].name) {
        };
    if (b)
        while (b[j++].name) {
        };

    ret = (dep*)malloc(sizeof(dep) * (j + i));

    for (i = 0; a && a[i].name; i++)
        ret[i] = a[i];

    for (j = 0; b && b[j].name; j++) {
        for (x = 0, y = 0; ret[x].name; x++) {
            if (pkgcmp(ret[x], b[j]))
                y = 1;
        }
        if (!y)
            ret[i++] = b[j];
    }

    return ret;
}

int listkeep(const char* kfile) {
    FILE* fp = fopen(kfile, "r");

    if (!fp)
        return 0;

    size_t n = 2048;
    char* s = (char*)malloc(n);
    while (getline(&s, &n, fp) > 0) { /* getline() never returns 0 */
        char* t = s;
        t += strspn(t, " \t");             /* skip over leading blanks */
        t[strcspn(t, " \t\r\n:#")] = '\0'; /* cutting at ':' strips ":arch" */
        if (t[0] != '\0')
            printf("%s\n", t);
    }

    /* Errors whilst reading from fp are ignored since a decade,
     * don't change this for a point release ... */

    fclose(fp);
    free(s);

    return 1;
}

/* This function handles the arguments to --add-keep and --del-keep.
 */
char** parseargs(int argind, int argc, char** argv) {
    int i = 0;
    int s = 50;
    char** ret = (char**)calloc(s, sizeof(char*));
    char t[100];

    for (; argind < argc; argind++) {
        if (i == s - 1) {
            s *= 2;
            ret = (char**)realloc(ret, s * sizeof(char*));
        }
        if (argv[argind][0] == '-' && argv[argind][1] == '\0') {
            while (fgets(t, 100, stdin)) {
                if (i == s - 1) {
                    s *= 2;
                    ret = (char**)realloc(ret, s * sizeof(char*));
                }
                ret[i] = (char*)malloc((strlen(t) + 1));
                strstripchr(t, ' ');
                strstripchr(t, '\r');
                strstripchr(t, '\n');
                strcpy(ret[i++], t);
            }
        } else {
            ret[i++] = strdup(argv[argind]);
        }
    }

    ret[i] = NULL;

    return ret;
}

dep* parseargs_as_dep(int argind, int argc, char** argv) {
    dep* rv;
    char* t;
    char** a;
    size_t i, n;

    a = parseargs(argind, argc, argv);

    for (n = 0; a[n]; n++)
        ;

    rv = (dep*)malloc((n + 1) * sizeof(dep));

    for (i = 0; i < n; i++) {
        rv[i].name = a[i];

        t = strchr(rv[i].name, ':');
        if (t) {
            rv[i].arch = t + 1;
            *t = '\0';
        } else {
            rv[i].arch = NULL;
        }

        rv[i].namehash = strhash(rv[i].name);
    }

    rv[n].name = rv[n].arch = NULL;

    free(a);

    return rv;
}
