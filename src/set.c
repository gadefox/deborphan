/* set.c - Functions to handle sets for deborphan.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/
#include <set.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "deborphan.h"

dep* set_dep(dep* p, const char* name) {
    char *str, *t;

    str = strdup(name);

    /* Multiarch package relationship fields contain a colon. */
    t = strchr(str, ':');

    if (t != NULL)
        *t = '\0'; /* Strip architecture suffix. */

    p->name = str;
    p->namehash = strhash(str);
    p->arch = NULL;

    return p;
}

dep* set_provides(pkg_info* p, const char* name, const int i) {
    if (i >= p->provides_max) {
        /* grow provides[] array */
        p->provides_max =
            p->provides_max ? p->provides_max * 2 : INIT_PROVIDES_COUNT;
#ifdef DEBUG
        fprintf(stderr, "Growing provides field to %d\n.",
                p->provides_max); /* FIXME */
        fflush(stderr);
#endif /* DEBUG */
        p->provides =
            realloc(p->provides, p->provides_max * sizeof(p->provides[0]));
    }

    set_dep(&(p->provides[i]), name);

    return &(p->provides[i]);
}

void set_section(pkg_info* p, const char* section, const char* prefix) {
    if (prefix) {
        p->section =
            malloc((strlen(section) + strlen(prefix) + 2) * sizeof(char));
        strcpy(p->section, prefix);
        strcat(p->section, "/");
        strcat(p->section, section);
    } else
        p->section = strdup(section);
}

int set_priority(pkg_info* p, const char* priority) {
    return (p->priority = string_to_priority(priority));
}

void init_pkg(pkg_info* p) {
    memset(p, 0, sizeof(pkg_info));
}

void reinit_pkg(pkg_info* p) {
    int i;
    free(p->self.name);
    free(p->self.arch);
    free(p->section);
    for (i = p->deps_cnt - 1; i >= 0; i--) {
        free(p->deps[i].name);
        free(p->deps[i].arch);
    }
    if (p->deps_max > 0)
        free(p->deps);

    for (i = p->provides_cnt - 1; i >= 0; i--) {
        free(p->provides[i].name);
        free(p->provides[i].arch);
    }
    if (p->provides_max > 0)
        free(p->provides);
    init_pkg(p);
}
