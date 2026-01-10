/* exit.c - Exit routines and help messages for deborphan
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader
   Copyright (C) 2008, 2009 Carsten Hey

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deborphan.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

/* Print an error message stating the program name, an optional,
   user defined string and an error message as produced by strerror(error_no).
   If exit_status is non-nil, the program will terminate. */
__attribute__((noreturn)) void error(int exit_status,
                                     int error_no,
                                     const char* format,
                                     ...) {
    va_list args;
    va_start(args, format);

    fprintf(stderr, "%s", program_name);
    if (format != NULL) {
        fprintf(stderr, ": ");
        vfprintf(stderr, format, args);
    }
    va_end(args);

    if (error_no)
        fprintf(stderr, ": %s\n", strerror(error_no));
    else
        fprintf(stderr, "\n");

    fflush(stderr);

    exit(exit_status);
}

void exit_help(void) {
    print_usage(stdout);

    printf(_("\nThe following options are available:\n"));
    /* General option */
    printf("--help,           ");
    printf(_("-h        This help.\n"));

    printf("--status-file,    ");
    printf(_("-f FILE   Use FILE as statusfile.\n"));

    printf("--version,        ");
    printf(_("-v        Version information.\n"));

    /* output modifiers */
    printf("--show-deps,      ");
    printf(_("-d        Show dependencies for packages that have them.\n"));

    printf("--show-priority,  ");
    printf(_("-P        Show priority of packages found.\n"));

    printf("--show-section,   ");
    printf(_("-s        Show the sections the packages are in.\n"));

    printf(_("--no-show-section           Do not show sections.\n"));

    printf("--show-size,      ");
    printf(_("-z        Show installed size of packages found.\n"));

    /* search modifiers */
    printf("--all-packages,   ");
    printf(_("-a        Compare all packages, not just libs.\n"));

    printf("--exclude,        ");
    printf(_("-e LIST   Work as if packages in LIST were not installed.\n"));

    printf("--exclude-dev,    ");
    printf(_("-D        exclude -dev packages from output\n"));

    printf("--force-hold,     ");
    printf(_("-H        Ignore hold flags.\n"));

    printf("--ignore-recommends         ");
    printf(_("Disable checks for `recommends'.\n"));
    printf("--ignore-suggests           ");
    printf(_("Disable checks for `suggests'.\n"));

    printf("--nice-mode,      ");
    printf(_("-n        Disable checks for `recommends' and `suggests'.\n"));

    printf("--priority,       ");
    printf(_("-p PRIOR  Select only packages with priority >= PRIOR.\n"));

    printf(
        _("--find-config               Find \"orphaned\" configuration "
          "files.\n"));

    printf(_(
        "--libdevel                  Also search in section \"libdevel\".\n"));

    /* keep file management */
    printf("--add-keep,       ");
    printf(_("-A PKGS.. Never report PKGS.\n"));

    printf("--keep-file,      ");
    printf(_("-k FILE   Use FILE to get/store info about kept packages.\n"));

    printf("--list-keep,      ");
    printf(_("-L        List the packages that are never reported.\n"));

    printf("--del-keep,       ");
    printf(_("-R PKGS.. Remove PKGS from the \"keep\" file.\n"));

    printf("--zero-keep,      ");
    printf(_("-Z        Remove all packages from the \"keep\" file.\n"));

#ifdef DEBFOSTER_KEEP
    printf(
        _("--df-keep                   Read debfoster's \"keepers\" file.\n"));
    printf(
        _("--no-df-keep                Do not read debfoster's \"keepers\" "
          "file.\n"));
#endif

    /* guessing */

    printf(_("--guess-common              Try to report common packages.\n"));
    printf(_("--guess-data                Try to report data packages.\n"));
    printf(
        _("--guess-debug               Try to report debugging libraries.\n"));
    printf(
        _("--guess-dev                 Try to report development packages.\n"));
    printf(_(
        "--guess-doc                 Try to report documentation packages.\n"));
    printf(_("--guess-dummy               Try to report dummy packages.\n"));
    printf(_("--guess-java                Try to report java libraries.\n"));
    printf(_("--guess-kernel              Try to report kernel modules.\n"));
    printf(_(
        "--guess-interpreters        Try to report interpreter libraries.\n"));
    printf(_("--guess-mono                Try to report mono libraries.\n"));
    printf(_("--guess-perl                Try to report perl libraries.\n"));
    printf(_("--guess-pike                Try to report pike libraries.\n"));
    printf(_("--guess-python              Try to report python libraries.\n"));
    printf(_("--guess-ruby                Try to report ruby libraries.\n"));
    printf(
        _("--guess-section             Try to report libraries in wrong "
          "sections.\n"));

    printf(_("--guess-all                 Try all of the above.\n"));
    printf(_("--guess-only                Use --guess options only.\n"));

    printf(_("\nSee also: deborphan(1)"));
    printf("\n");
    exit(EXIT_SUCCESS);
}

void exit_version(void) {
    printf(_("\
%s %s - Find packages without other packages depending on them\n\
"),
           PACKAGE, VERSION);
    printf("\n");
    printf(
        _("\
Distributed under the terms of the MIT License, see the file COPYING\n\
provided in this package, or /usr/share/doc/deborphan/copyright on a\n\
Debian system for details.\n\
"));
    printf("\n");
    printf(
        _("\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n\
SOFTWARE.\n\
"));
    printf("\n");
    printf(
        "\
Copyright (C) 2000 - 2003 Cris van Pelt <\"Cris van Pelt\"@tribe.eu.org>.\n\
Copyright (C) 2003 - 2006 Peter Palfrader <weasel@debian.org>.\n\
Copyright (C) 2005 Daniel Déchelotte <boite_a_spam@club-internet.fr>.\n\
Copyright (C) 2008 Andrej Tatarenkow <tatarenkow@comdasys.com>.\n\
Copyright (C) 2008 - 2009 Carsten Hey <carsten@debian.org>.\n");

    exit(EXIT_SUCCESS);
}

void print_usage(FILE* output) {
    fprintf(output, _("Usage: %s [OPTIONS] [PACKAGE]...\n"), program_name);
}

void exit_improperstate(void) {
    error(EXIT_FAILURE, 0, _("The status file is in an improper state.\n\
One or more packages are marked as half-installed, half-configured,\n\
unpacked, triggers-awaited or triggers-pending. Exiting.\n\
\n\
Note: dpkg --audit may be used to find such packages.\n"));
}

void exit_invalid_statusfile(void) {
    error(EXIT_FAILURE, 0, _("Status file is probably invalid. Exiting.\n"));
}
