/* libdeps.c - Retrieve and calculate dependencies for deborphan.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004, 2005, 2006 Peter Palfrader
   Copyright (C) 2005 Daniel Déchelotte
   Copyright (C) 2009 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deborphan.h"

extern int options[];

static int print_arch_suffix(pkg_info* pkg) {
    if (pkg->self.arch)
        return printf(":%s", pkg->self.arch);
    return 0;
}

/* For each package found, this scans the `package' structure, to
 * see if anything depends on it.
 */
void check_lib_deps(pkg_info* package,
                    pkg_info* current_pkg,
                    int print_suffix) {
    int deps, prov, no_dep_found = 1, search_found = 1;
    int i;
    static int j;

    extern dep* search_for;

   if (options[FIND_CONFIG] && !current_pkg->config)
        return;
    if (current_pkg->hold)
        return;
    if (current_pkg->priority < options[PRIORITY])
        return;
    if (keep && mustkeep(current_pkg->self))
        return;
    if (!is_library(current_pkg, options[SEARCH_LIBDEVEL]))
        return;

    if (options[SEARCH]) {
        search_found = 0;
        /* Count packages. Just once. */
        if (!j)
            for (; search_for[j].name; j++)
                ;

        /* Search for the package, and clear it from the list if it is
           found. */
        for (i = 0; search_for[i].name; i++) {
            if (strcmp(search_for[i].name, current_pkg->self.name) == 0) {
                if (search_for[i].arch == NULL ||
                    (current_pkg->self.arch != NULL &&
                     (strcmp(search_for[i].arch, current_pkg->self.arch) ==
                      0))) {
                    --j;
                    search_for[i].name = search_for[j].name;
                    search_for[i].arch = search_for[j].arch;
                    search_for[i].namehash = search_for[j].namehash;
                    search_for[j].name = NULL;
                    search_found = 1;
                    break;
                }
            }
        }
    }
    if (!search_found)
        return;

    if (options[SHOW_DEPS]) {
        printf("%s", current_pkg->self.name);

        if (print_suffix)
            print_arch_suffix(current_pkg);

        if (options[SHOW_SECTION] > 0)
            printf(" (%s", current_pkg->section);
        if (options[SHOW_PRIORITY])
            printf(" - %s", priority_to_string(current_pkg->priority));
        if (options[SHOW_SIZE])
            printf(", %ld", current_pkg->installed_size);
        if (options[SHOW_SECTION] > 0)
            printf(")");
        printf("\n");
    }

    /* Search all (installed) packages for dependencies.
     */
    for (; package && no_dep_found; package = package->next) {
        /* We assume that a multiarch-dependency pkg:arch is only satisfied by
         * a package that has pkg as package name or provides pkg to make an
         * older deborphan compatible with future versions of the multiarch
         * spec.  To be able to ignore multiarch self-dependencies safely, we
         * would further need to assume these are always an error (which is the
         * case for non-multiarch dependencies).  The latter assumtion might
         * not be safe and being able to check if a dependency is arch
         * qualified would require further hacking in this old code only to
         * display buggy orphaned packages ... so we do not ignore
         * self-dependencies at all for now. */
#if 0
	/* Let's ignore cases where a package depends on itself.  See #366028 */
	if (pkgcmp(current_pkg->self, package->self))
		continue;
#endif

        for (deps = 0; deps < package->deps_cnt && no_dep_found; deps++) {
            for (prov = 0; prov < current_pkg->provides_cnt && no_dep_found;
                 prov++) {
                if (pkgcmp(current_pkg->provides[prov], package->deps[deps])) {
                    if (options[SHOW_DEPS]) {
                        printf("      %s", package->self.name);
                        if (print_suffix)
                            print_arch_suffix(package);
                        putchar('\n');
                    } else
                        no_dep_found = 0;
                }
            }

            if (pkgcmp(current_pkg->self, package->deps[deps])) {
                if (options[SHOW_DEPS]) {
                    printf("      %s", package->self.name);
                    if (print_suffix)
                        print_arch_suffix(package);
                    putchar('\n');
                } else
                    no_dep_found = 0;
            }
        }
    }

    if (no_dep_found && !options[SHOW_DEPS] &&
        (!options[IGNORE_LIBS] || !is_pkg_dev(current_pkg))) {
        size_t prntd;

        if (options[SHOW_SIZE])
            printf("%10ld ", current_pkg->installed_size);

        if (options[SHOW_SECTION] > 0)
            printf("%-25s ", current_pkg->section);

        prntd = printf("%s", current_pkg->self.name);
        if (print_suffix)
            prntd += print_arch_suffix(current_pkg);

        if (options[SHOW_PRIORITY]) {
            size_t sz = 24;
            if (print_suffix)
                sz += 6;
            while (sz > prntd++)
                putchar(' ');
            printf(" %s", priority_to_string(current_pkg->priority));
        }

        printf("\n");
    }
}
