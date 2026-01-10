/* deborphan - Find orphaned libraries on a Debian system.
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader
   Copyright (C) 2008, 2009 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/

/* Header files we should all have. */
#include <errno.h>
#include <getopt.h>
#include <set.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deborphan.h"

/* These files should already be installed on the host machine -
   the GPL does not allow redistribution in this package. */
#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#endif

/* Name this program was called with. */
char* program_name;

/* Options given on the commandline */
int options[NUM_OPTIONS];

/* An optional list of packages to search for. */
dep* search_for;

/* A bunch of packages to keep. */
dep* keep;

static int depcmp(const dep* d1, const dep* d2) {
    return strcmp(d1->name, d2->name);
}

int main(int argc, char* argv[]) {
    char *line, *sfile = NULL, *kfile = NULL;
    char *sfile_content, *sfile_content2;
    pkg_info *package, *this;
    int i, argind;
    size_t j;
    int multiarch = 0;
    int print_arch_suffixes;
    size_t exclude_list_cnt = 0;
    size_t exclude_list_max = 0;
    dep* exclude_list = NULL;

    program_name = argv[0];
    memset(options, 0, NUM_OPTIONS * sizeof(int));

    options[PRIORITY] = DEFAULT_PRIORITY;
#ifdef IGNORE_DEBFOSTER
    options[NO_DEBFOSTER] = 1;
#endif

    /*@unused@*/ /* Actually it is used but splint does not recognise this. */
    struct option longopts[] = {{"version", 0, 0, 'v'},
                                {"help", 0, 0, 'h'},
                                {"status-file", 1, 0, 'f'},
                                {"show-deps", 0, 0, 'd'},
                                {"show-deps-pristine", 0, 0, 'd'},
                                {"nice-mode", 0, 0, 'n'},
                                {"ignore-recommends", 0, 0, 61},
                                {"ignore-suggests", 0, 0, 62},
                                {"print-guess-list", 0, 0, 200},
                                {"check-options", 0, 0, 201},
                                {"all-packages-pristine", 0, 0, 202},
                                {"all-packages", 0, 0, 'a'},
                                {"priority", 1, 0, 'p'},
                                {"show-section", 0, 0, 's'},
                                {"no-show-section", 0, 0, 0},
                                {"show-arch", 0, 0, 203},
                                {"no-show-arch", 0, 0, 204},
                                {"show-priority", 0, 0, 'P'},
                                {"show-size", 0, 0, 'z'},
                                {"force-hold", 0, 0, 'H'},
                                {"keep-file", 1, 0, 'k'},
                                {"add-keep", 0, 0, 'A'},
                                {"del-keep", 0, 0, 'R'},
                                {"list-keep", 0, 0, 'L'},
                                {"zero-keep", 0, 0, 'Z'},
                                {"guess-dev", 0, 0, 1},
                                {"no-guess-dev", 0, 0, 31},
                                {"guess-perl", 0, 0, 2},
                                {"no-guess-perl", 0, 0, 32},
                                {"guess-section", 0, 0, 3},
                                {"no-guess-section", 0, 0, 33},
                                {"guess-all", 0, 0, 4},
                                {"no-guess-all", 0, 0, 34},
                                {"guess-debug", 0, 0, 5},
                                {"no-guess-debug", 0, 0, 35},
#ifdef DEBFOSTER_KEEP
                                {"df-keep", 0, 0, 6},
#endif
                                {"guess-only", 0, 0, 7},
#ifdef DEBFOSTER_KEEP
                                {"no-df-keep", 0, 0, 8},
#endif
                                {"guess-pike", 0, 0, 9},
                                {"no-guess-pike", 0, 0, 39},
                                {"guess-python", 0, 0, 10},
                                {"no-guess-python", 0, 0, 40},
                                {"guess-ruby", 0, 0, 11},
                                {"no-guess-ruby", 0, 0, 41},
                                {"guess-interpreters", 0, 0, 12},
                                {"no-guess-interpreters", 0, 0, 42},
                                {"guess-dummy", 0, 0, 13},
                                {"no-guess-dummy", 0, 0, 43},
                                {"guess-common", 0, 0, 14},
                                {"no-guess-common", 0, 0, 44},
                                {"guess-data", 0, 0, 15},
                                {"no-guess-data", 0, 0, 45},
                                {"guess-doc", 0, 0, 16},
                                {"no-guess-doc", 0, 0, 46},
                                {"find-config", 0, 0, 17},
                                {"libdevel", 0, 0, 18},
                                {"guess-mono", 0, 0, 19},
                                {"no-guess-mono", 0, 0, 49},
                                {"guess-kernel", 0, 0, 20},
                                {"no-guess-kernel", 0, 0, 50},
                                {"guess-java", 0, 0, 21},
                                {"no-guess-java", 0, 0, 51},
                                {"exclude", 1, 0, 'e'},
                                {"exclude-dev", 0, 0, 'D'},
                                {0, 0, 0, 0}};

#ifdef ENABLE_NLS
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#endif

    while ((i = getopt_long(argc, argv, "p:advhe:nf:sPzHk:ARLZD", longopts,
                            NULL)) != EOF) {
        switch (i) {
            case 'd':
                options[SHOW_DEPS] = 1;
                break;
            case 'D':
                options[IGNORE_LIBS] |= IGNORE_LIB_DEV;
                break;
            case 'h':
                exit_help();
                break;
            case 'v':
                exit_version();
                break;
            case 'f':
                sfile = (char*)malloc((strlen(optarg) + 1) * sizeof(char));
                strcpy(sfile, optarg);
                break;
            case 203:
                options[SHOW_ARCH] = ALWAYS;
                break;
            case 204:
                options[SHOW_ARCH] = NEVER;
                break;
            case 'n':
                options[IGNORE_RECOMMENDS] = 1;
                options[IGNORE_SUGGESTS] = 1;
                break;
            case 61:
                options[IGNORE_RECOMMENDS] = 1;
                break;
            case 62:
                options[IGNORE_SUGGESTS] = 1;
                break;
            case 200:
                printf(
                    "common\ndata\ndebug\ndev\ndoc\ndummy\ninterpreters\nkernel"
                    "\nmono"
                    "\nperl\npike\npython\nruby\nsection\n");
                exit(EXIT_SUCCESS);
            case 201:
                options[CHECK_OPTIONS] = 1;
                break;
            case 202:
                /* ALL_PACKAGES_IMPLY_SECTION is defined anyway, so this
                 * fall through is sufficient for now. */
            case 'a':
                options[ALL_PACKAGES] = 1;
#ifdef ALL_PACKAGES_IMPLY_SECTION
                options[SHOW_SECTION]++;
#endif
                break;
            case 'p':
                options[PRIORITY] = string_to_priority(optarg);
                if (!options[PRIORITY]) {
                    if (!sscanf(optarg, "%d", &options[PRIORITY])) {
                        print_usage(stderr);
                        error(EXIT_FAILURE, 0, "invalid priority: %s", optarg);
                    }
                }
                break;
            case 's':
                options[SHOW_SECTION]++;
                break;
            case 0:
                options[SHOW_SECTION]--;
                break;
            case 'P':
                options[SHOW_PRIORITY] = 1;
                break;
            case 'z':
                options[SHOW_SIZE] = 1;
                break;
            case 'H':
                options[FORCE_HOLD] = 1;
                break;
            case 'k':
                kfile = optarg;
                break;
            case 'A':
                options[ADD_KEEP] = 1;
                break;
            case 'R':
                options[DEL_KEEP] = 1;
                break;
            case 'L':
                options[LIST_KEEP] = 1;
                break;
            case 'Z':
                options[ZERO_KEEP] = 1;
                break;
            case 1:
                guess_set(GUESS_DEV);
                break;
            case 31:
                guess_clr(GUESS_DEV);
                break;
            case 2:
                guess_set(GUESS_PERL);
                break;
            case 32:
                guess_clr(GUESS_PERL);
                break;
            case 3:
                guess_set(GUESS_SECTION);
                break;
            case 33:
                guess_clr(GUESS_SECTION);
                break;
            case 4:
                guess_set(GUESS_ALL);
                break;
            case 34:
                guess_clr(GUESS_ALL);
                break;
            case 5:
                guess_set(GUESS_DEBUG);
                break;
            case 35:
                guess_clr(GUESS_DEBUG);
                break;
#ifdef DEBFOSTER_KEEP
            case 6:
                options[NO_DEBFOSTER] = 0;
                break;
#endif
            case 7:
                options[GUESS_ONLY] = 1;
                break;
#ifdef DEBFOSTER_KEEP
            case 8:
                options[NO_DEBFOSTER] = 1;
                break;
#endif
            case 9:
                guess_set(GUESS_PIKE);
                break;
            case 39:
                guess_clr(GUESS_PIKE);
                break;
            case 10:
                guess_set(GUESS_PYTHON);
                break;
            case 40:
                guess_clr(GUESS_PYTHON);
                break;
            case 11:
                guess_set(GUESS_RUBY);
                break;
            case 41:
                guess_clr(GUESS_RUBY);
                break;
            case 12:
                guess_set(GUESS_IP);
                break;
            case 42:
                guess_clr(GUESS_IP);
                break;
            case 13:
                guess_set(GUESS_DUMMY);
                break;
            case 43:
                guess_clr(GUESS_DUMMY);
                break;
            case 14:
                guess_set(GUESS_COMMON);
                break;
            case 44:
                guess_clr(GUESS_COMMON);
                break;
            case 15:
                guess_set(GUESS_DATA);
                break;
            case 45:
                guess_clr(GUESS_DATA);
                break;
            case 16:
                guess_set(GUESS_DOC);
                break;
            case 46:
                guess_clr(GUESS_DOC);
                break;
            case 17:
                options[FIND_CONFIG] = 1;
                options[ALL_PACKAGES] = 1;
                break;
            case 18:
                options[SEARCH_LIBDEVEL] = 1;
                break;
            case 19:
                guess_set(GUESS_MONO);
                break;
            case 49:
                guess_clr(GUESS_MONO);
                break;
            case 20:
                guess_set(GUESS_KERNEL);
                break;
            case 50:
                guess_clr(GUESS_KERNEL);
                break;
            case 21:
                guess_set(GUESS_JAVA);
                break;
            case 51:
                guess_clr(GUESS_JAVA);
                break;
            case 'e':
                while (optarg) {
                    char* c_ptr;
                    if (exclude_list_cnt >= exclude_list_max) {
                        /* grow exclude_list[] array */
                        exclude_list_max = exclude_list_max
                                               ? exclude_list_max * 2
                                               : INIT_EXCLUDES_COUNT;
#ifdef DEBUG
                        fprintf(stderr, "Growing excludes field to %d.\n",
                                exclude_list_max);
                        fflush(stderr);
#endif /* DEBUG */
                        exclude_list =
                            realloc(exclude_list,
                                    exclude_list_max * sizeof(exclude_list[0]));
                    }
                    if (exclude_list_cnt >= exclude_list_max)
                        error(EXIT_FAILURE, 0, "Too many exclude packages");
                    c_ptr = strsep(&optarg, ",");
                    c_ptr[strcspn(c_ptr, ":")] =
                        '\0'; /* remove architecture suffix */
                    exclude_list[exclude_list_cnt].name = c_ptr;
                    exclude_list[exclude_list_cnt].namehash = strhash(c_ptr);
                    ++exclude_list_cnt;
                }
                qsort(exclude_list, exclude_list_cnt, sizeof(exclude_list[0]),
                      (int (*)(const void*, const void*))depcmp);
                break;
            case '?':
                if (options[CHECK_OPTIONS])
                    exit(EXIT_FAILURE);
                print_usage(stderr);
                exit(EXIT_FAILURE);
        }
    }

    if (options[CHECK_OPTIONS])
        exit(EXIT_SUCCESS);

    if (options[ZERO_KEEP]) {
        if (!kfile)
            kfile = KEEPER_FILE;
        if (zerofile(kfile) < 0)
            error(EXIT_FAILURE, errno, "%s", kfile);

        /* Don't always exit. "deborphan -Z -A <foo>" should be valid. */
        if (!options[ADD_KEEP])
            exit(EXIT_SUCCESS);
    }

    if (options[LIST_KEEP]) {
        if (!kfile)
            kfile = KEEPER_FILE;

        if (!listkeep(kfile)) {
#ifndef DEBFOSTER_KEEP
            if (errno != ENOENT) /* If the file doesn't exit, we just have an
                                    empty list */
                error(EXIT_FAILURE, errno, "%s", kfile); /* fatal */
#else
            fprintf(stderr, "%s: %s: %s\n", program_name, kfile,
                    strerror(errno)); /* non-fatal */
#endif
        }
#ifdef DEBFOSTER_KEEP
        if (!options[NO_DEBFOSTER]) {
            fprintf(stderr, "-- %s -- \n", DEBFOSTER_KEEP);
            listkeep(DEBFOSTER_KEEP);
        }
#endif
        exit(EXIT_SUCCESS);
    }

    if (options[GUESS_ONLY] && !options[GUESS]) {
        print_usage(stderr);
        error(EXIT_FAILURE, 0, "need at least one other --guess option.");
    }

    argind = optind;
    i = 0;
    if (kfile == NULL)
        kfile = KEEPER_FILE;
    if (sfile == NULL)
        sfile = STATUS_FILE;

    if ((argc - argind) > 50)
        error(EXIT_FAILURE, E2BIG, "");

    if (argind < argc) {
        options[SEARCH] = 1;
        options[ALL_PACKAGES] = 1;
        options[SHOW_DEPS] = 1;
        options[PRIORITY] = 0;
    }

    if (options[SHOW_DEPS])
        options[FORCE_HOLD] = 1;

    keep = readkeep(kfile);

    if (options[ADD_KEEP] || options[DEL_KEEP]) {
        char** args;
        if (argind >= argc)
            error(EXIT_FAILURE, 0, "not enough arguments for %s.",
                  options[ADD_KEEP] ? "--add-keep" : "--del-keep");

        args = parseargs(argind, argc, argv);

        for (j = 0; args[j] != NULL; j++)
            args[j][strcspn(args[j], ":")] = '\0'; /* remove arch suffix */

        if (options[DEL_KEEP]) {
            switch (delkeep(kfile, args)) {
                case -1:
                    error(EXIT_FAILURE, errno, "%s", kfile);
                    break; /* make splint happy */
                case 1:
                    error(EXIT_FAILURE, 0, "no packages removed.");
            }
        } else {
            i = pkggrep(sfile, args);
            if (i < 0)
                error(EXIT_FAILURE, errno, sfile);
            if (i)
                error(EXIT_FAILURE, 0, "%s: no such package.", args[i - 1]);

            if ((i = hasduplicate(args)))
                error(EXIT_FAILURE, 0, "%s: duplicate entry.", args[i - 1]);

            if (addkeep(kfile, args) < 0)
                error(EXIT_FAILURE, errno, "%s", kfile);
        }

        exit(EXIT_SUCCESS);
    }

    /* We don't want to merge the files if we're adding, because it's perfectly
       alright to have the same entry in debfoster and deborphan.
    */
#ifdef DEBFOSTER_KEEP
    if (!options[NO_DEBFOSTER]) {
        dep* dfkeep = readkeep(DEBFOSTER_KEEP);

        if (dfkeep) {
            dep* d = keep;
            keep = mergekeep(keep, dfkeep);
            free(d);
            free(dfkeep);
        }
    }
#endif

    search_for = parseargs_as_dep(argind, argc, argv);

    if (!(sfile_content = debopen(sfile)))
        error(EXIT_FAILURE, errno, "%s", sfile);

    sfile_content2 = sfile_content;

    this = package = (pkg_info*)malloc(sizeof(pkg_info));
    init_pkg(this);
    init_pkg_regex();

    while ((line = strsep(&sfile_content, "\n")) != NULL) {
        if ((!strchr("AIPpSsEeDdRr", *line)) || (*line == ' '))
            continue;

        if (*line != '\0') {
            strstripchr(line, ' ');
            get_pkg_info(line, this, &multiarch);
            continue;
        }
        if (!this->install && !options[FIND_CONFIG]) {
            reinit_pkg(this);
            continue;
        }
        if (this->self.name && exclude_list &&
            bsearch(&this->self, exclude_list, exclude_list_cnt,
                    sizeof(exclude_list[0]),
                    (int (*)(const void*, const void*))depcmp)) {
            reinit_pkg(this);
            continue;
        }
        this->next = malloc(sizeof(pkg_info));
        this = this->next;
        init_pkg(this);
    }

    free(sfile_content2);

    this->next = NULL;
    this = package;

    print_arch_suffixes = (options[SHOW_ARCH] == ALWAYS ||
                           (options[SHOW_ARCH] == DEFAULT && multiarch));

    /* Check the dependencies. Last package is handled specially, because it
       is not terminated with a \n in STATUS_FILE. */
    while (this->next) {
        check_lib_deps(package, this, print_arch_suffixes);
        this = this->next;
    }
    if (this->install)
        check_lib_deps(package, this, print_arch_suffixes);

    free_pkg_regex();

    fflush(stdout);

    for (i = 0; options[SEARCH] && search_for[i].name; i++) {
        fprintf(stderr, "%s: package %s", argv[0], search_for[i].name);
        if (search_for[i].arch)
            fprintf(stderr, ":%s", search_for[i].arch);
        fprintf(stderr, " not found or not installed\n");
    }

    return i ? EXIT_FAILURE : EXIT_SUCCESS;
}
