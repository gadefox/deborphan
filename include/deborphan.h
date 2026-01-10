/* deborphan.h
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader
   Copyright (C) 2008, 2009 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/
#pragma once

#include <config.h>
#include <stdio.h>

/* Faster than toupper. Less reliable too. */
#define upcase(c) ((c)&32 ? (c) ^ 32 : (c))

typedef struct dep {
    char* name;
    char* arch;
    unsigned int namehash;
} dep;

/* Options for option[IGNORE_LIBS]
 */
#define IGNORE_LIB_DEV  (1 << 0)

/* The initial size of the provides[] and deps[] array, when
 * it is allocated for the first time.
 *
 * Should a grow be required, the count size is doubled.
 */
#define INIT_DEPENDS_COUNT 32
#define INIT_PROVIDES_COUNT 4
#define INIT_EXCLUDES_COUNT 4

/* These arrays aren't exactly neat, but it seems they suffice. */
typedef struct pkg_info {
    dep self;
    int priority;
    dep* provides;
    int provides_cnt;
    int provides_max;
    char* section;
    dep* deps;
    int deps_cnt;
    int deps_max;
    int install;
    int hold;
    int essential;
    int dummy;
    int config;
    long installed_size;
    struct pkg_info* next;
} pkg_info;

/* Make the options[] array easier to read. */
enum {
    SHOW_DEPS = 0,
    NICE_MODE,
    IGNORE_RECOMMENDS,
    IGNORE_SUGGESTS,
    IGNORE_LIBS,
    ALL_PACKAGES,
    PRIORITY,
    SHOW_SECTION,
    SHOW_PRIORITY,
    SHOW_SIZE,
    SHOW_ARCH,
    SEARCH,
    FORCE_HOLD,
    ADD_KEEP,
    DEL_KEEP,
    GUESS,
    NO_DEBFOSTER,
    GUESS_ONLY,
    LIST_KEEP,
    ZERO_KEEP,
    FIND_CONFIG,
    SEARCH_LIBDEVEL,
    CHECK_OPTIONS,
    NUM_OPTIONS /* THIS HAS TO BE THE LAST OF THIS ENUM! */
};

/* options[SHOW_ARCH] is set to one of these values. */
enum { DEFAULT = 0, ALWAYS, NEVER };

#define GUESS_DEV (1 << 1)
#define GUESS_PERL (1 << 2)
#define GUESS_SECTION (1 << 3)
#define GUESS_DEBUG (1 << 4)
#define GUESS_PIKE (1 << 5)
#define GUESS_PYTHON (1 << 6)
#define GUESS_RUBY (1 << 7)
#define GUESS_DUMMY (1 << 8)
#define GUESS_COMMON (1 << 9)
#define GUESS_DATA (1 << 10)
#define GUESS_DOC (1 << 11)
#define GUESS_MONO (1 << 12)
#define GUESS_KERNEL (1 << 13)
#define GUESS_JAVA (1 << 14)

// Interpreters
#define GUESS_IP                                                        \
    (GUESS_PERL | GUESS_PIKE | GUESS_PYTHON | GUESS_RUBY | GUESS_MONO | \
     GUESS_JAVA)
#define GUESS_ALL 0xFFFFFFFF

#define guess_chk(n) ((options[GUESS] & (n)) == (n))
#define guess_set(n) (options[GUESS] |= (n))
#define guess_clr(n) (options[GUESS] &= ~(n))
#define guess_unique(n) (!(options[GUESS] ^ (n)))
#define pkgcmp(a, b) \
    (((a).namehash == (b).namehash ? (strcmp((a).name, (b).name) ? 0 : 1) : 0))

extern dep* keep;
extern int options[NUM_OPTIONS];
extern char* program_name;

/* pkginfo.c */
void init_pkg_regex(void);
void free_pkg_regex(void);
void get_pkg_info(const char* line, pkg_info* package, int* multiarch);
void get_pkg_priority(const char* line, pkg_info* package);
void get_pkg_provides(const char* line, pkg_info* package);
void get_pkg_name(const char* line, pkg_info* package);
void get_pkg_status(const char* line, pkg_info* package);
void get_pkg_section(const char* line, pkg_info* package);
void get_pkg_deps(const char* line, pkg_info* package);
void get_pkg_essential(const char* line, pkg_info* package);
void get_pkg_installed_size(const char* line, pkg_info* package);
void get_pkg_dummy(const char* line, pkg_info* package);
int is_pkg_dev(pkg_info* package);
unsigned int is_library(pkg_info* package, int search_libdevel);

/* libdeps.c */
void check_lib_deps(pkg_info* package, pkg_info* current_pkg, int print_suffix);

/* exit.c */
__attribute__((noreturn)) void error(int exit_status,
                                     int error_no,
                                     const char* format,
                                     ...);
void exit_help(void);
void exit_improperstate(void);
void exit_invalid_statusfile(void);
void exit_version(void);
void print_usage(FILE* output);

/* string.c */
int string_to_priority(const char* priority);
const char* priority_to_string(int priority);
void strstripchr(char* s, int c);
unsigned int strhash(const char* line);

/* keep.c */
dep* readkeep(const char* kfile);
int mustkeep(dep d);
int delkeep(const char* kfile, char** del);
int addkeep(const char* kfile, char** add);
dep* mergekeep(const dep* a, const dep* b);
void listkeepall(const char* kfile);
int listkeep(const char* kfile);
char** parseargs(int argind, int argc, char** argv);
dep* parseargs_as_dep(int argind, int argc, char** argv);
int hasduplicate(char** list);
int pkggrep(const char* sfile, char** pkgnames);

/* file.c */
char* debopen(const char* filename);
int zerofile(const char* filename);

#ifdef ENABLE_NLS
#undef _
#define _(string) gettext(string)
#else
#undef _
#define _(string) (string)
#undef setlocale
#define setlocale(a, b)
#undef bindtextdomain
#define bindtextdomain(a, b)
#undef textdomain
#define textdomain(a)
#endif /* ENABLE_NLS */

#undef D_ATTRIBUTE
