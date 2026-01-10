/* pkginfo.c - Retrieve information about packages for deborphan.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004, 2006 Peter Palfrader
   Copyright (C) 2008, 2009 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/

/* This code is not nearly as advanced as dpkg's. It assumes a
   "perfect" statusfile. If the status-file is corrupted it may
   result in deborphan giving the wrong packages, or even crashing. */

#include <regex.h>
#include <set.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deborphan.h"

static regex_t re_statusinst, re_statusnotinst, re_statushold, re_namedev,
    re_gnugrepv, re_descdummy, re_desctransit, re_statusconfig, re_status;

void init_pkg_regex(void) {
    regcomp(&re_statusinst, "^Status:.*[^-]installed$",
            REG_EXTENDED | REG_FLAGS);
    regcomp(&re_statusnotinst, "^Status:.*not\\-installed$",
            REG_EXTENDED | REG_FLAGS);
    regcomp(&re_statushold, "^Status:hold.*[^-]installed$",
            REG_EXTENDED | REG_FLAGS);
    regcomp(&re_statusconfig, "^Status:.*config\\-files$",
            REG_EXTENDED | REG_FLAGS);
    regcomp(&re_status, "^Status:", REG_EXTENDED | REG_FLAGS);

    if (options[GUESS]) {
        char guess[256];
        guess[0] = '\0';

        if (guess_chk(GUESS_PERL))
            strcat(guess, "^lib.*-perl$|");
        if (guess_chk(GUESS_PYTHON))
            strcat(guess, "^python[[:digit:].]*-|");
        if (guess_chk(GUESS_PIKE))
            strcat(guess, "^pike[[:digit:].]*-|");
        if (guess_chk(GUESS_RUBY))
            strcat(guess, "^lib.*-ruby[[:digit:].]*$|");
        if (guess_chk(GUESS_MONO))
            strcat(guess, "^libmono|");
        if (guess_chk(GUESS_DEV))
            strcat(guess, "-dev$|");
        if (guess_chk(GUESS_DEBUG))
            strcat(guess, "-dbg(|sym)$|");
        if (guess_chk(GUESS_COMMON))
            strcat(guess, "-common$|");
        if (guess_chk(GUESS_DATA))
            strcat(guess, "-(data|music)$|");
        if (guess_chk(GUESS_DOC))
            strcat(guess, "-doc$|");
        if (guess_chk(GUESS_KERNEL))
            strcat(guess,
                   "(-modules|^nvidia-kernel)-.*[[:digit:]]+\\.[[:digit:]]+\\."
                   "[[:digit:]]+");
        if (guess_chk(GUESS_JAVA))
            strcat(guess, "^lib.*-java$|");
        if (guess_chk(GUESS_SECTION)) {
            regcomp(&re_gnugrepv,
                    "(-perl|-dev|-doc|-dbg)$|^lib(mono|pam|recad|reoffice)|-("
                    "ruby[[:"
                    "digit:]"
                    ".]*|bin|tools|utils|dbg|dbgsym)$",
                    REG_EXTENDED | REG_FLAGS);
        }

        /* GUESS_DUMMY is a fake. It is not handled like other guess options
         * because the package's name is not really checked, but its
         * description line. The reason we sort of pretend it's a real
         * guess option is that we can combine it with --guess-only and
         * guess-all.
         */
        if (guess_chk(GUESS_DUMMY)) {
            regcomp(&re_descdummy, "^Description:.*dummy",
                    REG_EXTENDED | REG_FLAGS);
            regcomp(&re_desctransit,
                    "^Description:.*transition(|n)($|ing|al|ary| package| "
                    "purposes)",
                    REG_EXTENDED | REG_FLAGS);
        }
        if (guess[strlen(guess) - 1] == '|')
            guess[strlen(guess) - 1] = '\0';

        if (!guess_unique(GUESS_SECTION))
            regcomp(&re_namedev, guess, REG_EXTENDED | REG_FLAGS);
    }
}

void free_pkg_regex(void) {
    regfree(&re_statusinst);
    regfree(&re_statusnotinst);
    regfree(&re_statushold);
    regfree(&re_statusconfig);
    regfree(&re_status);
    regfree(&re_namedev);
    regfree(&re_gnugrepv);
    regfree(&re_descdummy);
    regfree(&re_desctransit);
}

/* A similar "hack" was created by Paul Martin a while ago. It was not
 * implemented then for various reasons. This selects the function to
 * call to get the info, based on the first few characters.
 * Not as versatile as regular expressions, but it makes up for that in
 * speed.
 */
void get_pkg_info(const char* line, pkg_info* package, int* multiarch) {
    if (strchr(line, ':') == 0) {
        exit_invalid_statusfile();
    }

    switch (upcase(line[0])) {
        case 'P':
            switch (upcase(line[2])) {
                case 'I': /* PrIority */
                    get_pkg_priority(line, package);
                    break;
                case 'C': /* PaCkage */
                    get_pkg_name(line, package);
                    break;
                case 'O': /* PrOvides */
                    get_pkg_provides(line, package);
                    break;
                case 'E': /* PrE-depends */
                    get_pkg_deps(line, package);
                    break;
            }
            break;
        case 'D':
            switch (upcase(line[2])) {
                case 'P': /* DePends */
                    get_pkg_deps(line, package);
                    break;
                case 'S': /* DeScription */
                    get_pkg_dummy(line, package);
                    break;
            }
            break;
        case 'E': /* Essential */
            get_pkg_essential(line, package);
            break;
        case 'I':
            get_pkg_installed_size(line, package);
            break;
        case 'R':
            switch (upcase(line[2])) {
                case 'C': /* ReCommends */
                    if (!options[IGNORE_RECOMMENDS])
                        get_pkg_deps(line, package);
                    break;
            }
            break;
        case 'S':
            switch (upcase(line[1])) {
                case 'E': /* SEction */
                    get_pkg_section(line, package);
                    break;
                case 'T': /* STatus */
                    get_pkg_status(line, package);
                    break;
                case 'U': /* SUggests */
                    if (!options[IGNORE_SUGGESTS])
                        get_pkg_deps(line, package);
                    break;
            }
            break;
        case 'A':
            if (strncmp("Architecture:", line, sizeof("Architecture:") - 1) ==
                0) {
                static char* firstarchfound = NULL;
                /* Spaces are removed from the line, so the field's value starts
                 * directly after the colon. */
                const char* arch = line + sizeof("Architecture:") - 1;
                package->self.arch = strdup(arch);
                if (*multiarch == 1 || strcmp(arch, "all") == 0)
                    break;
                if (firstarchfound == NULL)
                    firstarchfound = strdup(arch);
                else if (strcmp(arch, firstarchfound) != 0) {
                    *multiarch = 1;
                    /* The variable firstarchfound is only needed to detect if
                     * packages from multiple architectures are installed (we
                     * can't ask dpkg because the read status file might belong
                     * to a different system).  Now that we know that the status
                     * file contains multiple architectures we can free
                     * firstarchfound.*/
                    free(firstarchfound);
                    firstarchfound = NULL;
                }
            }
            break;
    }
}

void get_pkg_essential(const char* line, pkg_info* package) {
    if (strcasecmp(line, "Essential:yes") == 0)
        package->essential = 1;
}

void get_pkg_installed_size(const char* line, pkg_info* package) {
    if (strncasecmp(line, "Installed-Size:", 15) == 0)
        package->installed_size = strtol(line + 15, NULL, 10);
}

void get_pkg_dummy(const char* line, pkg_info* package) {
    if (!guess_chk(GUESS_DUMMY))
        return;

    if (regexec(&re_descdummy, line, 0, NULL, 0) == 0 ||
        regexec(&re_desctransit, line, 0, NULL, 0) == 0) {
        package->dummy = 1;
    }
}

void get_pkg_deps(const char* line, pkg_info* package) {
    char *tok, *line2, *version = NULL;
    signed int num_deps, i;
    unsigned int dup = 0;
    dep d;

    line2 = strchr(line, ':') + 1;

    num_deps = package->deps_cnt;

    for (; (tok = strsep(&line2, ",|")); num_deps++) {
        /* Versions are up to dpkg. */
        if ((version = strchr(tok, '(')))
            *version = '\0';

        (void)set_dep(&d, tok);
        dup = 0;
        for (i = 0; i < num_deps; i++) {
            if (pkgcmp(package->deps[i], d)) {
                dup = 1;
                /*@innerbreak@*/
                break;
            }
        }

        if (dup)
            num_deps--;
        else {
            if (num_deps >= package->deps_max) {
                /* grow deps[] array */
                package->deps_max = package->deps_max ? package->deps_max * 2
                                                      : INIT_DEPENDS_COUNT;
#ifdef DEBUG
                fprintf(stderr, "Growing deps field to %d.\n",
                        package->deps_max);
                fflush(stderr);
#endif /* DEBUG */
                package->deps =
                    realloc(package->deps,
                            package->deps_max * sizeof(package->deps[0]));
            }
            package->deps[num_deps] = d;
        }
    }

    package->deps_cnt = num_deps;
}

void get_pkg_priority(const char* line, pkg_info* package) {
    set_priority(package, strchr(line, ':') + 1);
}

void get_pkg_provides(const char* line, pkg_info* package) {
    char *prov, *name;
    int i = 0;

    prov = strchr(line, ':') + 1;

    for (i = 0; (name = strsep(&prov, ",")) || !i; i++) {
        if (!name)
            name = prov;
        (void)set_provides(package, name, i);
    }
    package->provides_cnt = i;
}

void get_pkg_name(const char* line, pkg_info* package) {
    char* name = strchr(line, ':') + 1;

    package->self.name = strdup(name);
    package->self.namehash = strhash(name);
}

void get_pkg_status(const char* line, pkg_info* package) {
    if (!regexec(&re_statusinst, line, 0, NULL, 0)) {
        set_install(package);
        if (!options[FORCE_HOLD]) {
            if (!regexec(&re_statushold, line, 0, NULL, 0))
                set_hold(package);
        }
    } else if (!regexec(&re_statusconfig, line, 0, NULL, 0)) {
        if (options[FIND_CONFIG])
            set_config(package);
    } else if (regexec(&re_statusnotinst, line, 0, NULL, 0)) {
        /* The package state is neither installed, config-files nor
         * not-installed.  It is also possible that get_pkg_info()
         * wrongly detected the current line as a status line.
         *
         * Abort with error message "improper state" if we
         * really parsed a status line.
         */
        if (!regexec(&re_status, line, 0, NULL, 0))
            exit_improperstate();
    }
}

/* Okay, this function does not really check the libraryness of a package,
 * but it checks whether the package should be checked (1) or not (0).
 */
unsigned int is_library(pkg_info* package, int search_libdevel) {
    if (options[ALL_PACKAGES])
        return 1;

    if (!package->section)
        return 0;

#ifndef IGNORE_ESSENTIAL
    if (package->essential)
        return 0;
#endif

    /* Mono libraries must be handeled especially since Mono puts its
     * development libraries in section libs.
     */
    if (!guess_chk(GUESS_MONO) && !strncmp(package->self.name, "libmono", 7))
        return 0;

    if (!options[GUESS_ONLY]) {
        if (strstr(package->section, "/libs") ||
            strstr(package->section, "/oldlibs") ||
            strstr(package->section, "/introspection") ||
            (search_libdevel && strstr(package->section, "/libdevel")))
            return 1;
    }

    if (!options[GUESS])
        return 0;

    /* See comments in init_pkg_regex(). */
    if (guess_chk(GUESS_DUMMY) && package->dummy)
        return 1;

    /* Avoid checking package name if we're only checking dummy,
     * which we get from the description line.
     */
    if (guess_unique(GUESS_DUMMY))
        return 0;

    /* ^lib has been removed from re_namedev in deborphan 1.7.28, thus mark
     * this package as to be checked when re_namedev matches. When
     * GUESS_SECTION is set unique the regex always matches wrongly.
     */
    if (!guess_unique(GUESS_SECTION))
        if (!regexec(&re_namedev, package->self.name, 0, NULL, 0))
            return 1;

    if (!guess_chk(GUESS_SECTION))
        return 0;

    /* Check whether the package begins with lib, but not if it ends in one of:
     * -dbg, -dbgsym, -doc, -perl or -dev. This is what --guess-section does.
     */
    if (!strncmp(package->self.name, "lib", 3))
        if (regexec(&re_gnugrepv, package->self.name, 0, NULL, 0))
            return 1;

    return 0;
}

void get_pkg_section(const char* line, pkg_info* package) {
    char* section;

    section = strchr(line, ':') + 1;

    if (strchr(section, '/'))
        set_section(package, section, NULL);
    else
        set_section(package, section, "main");
}

int is_pkg_dev(pkg_info* package) {
    const char *suffixes[] = { "-dev", "-dbg", NULL };
    size_t len_pkg = strlen(package->self.name);
    size_t len_suf;

    for (int i = 0; suffixes[i] != NULL; i++) {
        len_suf = strlen(suffixes[i]);

        if (len_pkg <= len_suf)
            continue;
        if (strcmp(package->self.name + len_pkg - len_suf, suffixes[i]) == 0)
            return 1;
    }
    return 0;
}
