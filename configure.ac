# Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
# Copyright (C) 2003, 2004, 2005, 2006 Peter Palfrader

dnl Process this file with autoconf to produce a configure script.
AC_INIT([deborphan], [1.7.36])
AC_CONFIG_SRCDIR([src/deborphan.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([include/config.h])

AC_DEFINE(STATUS_FILE, LOCALSTATEDIR"/lib/dpkg/status", [Location of your statusfile])
AC_DEFINE(KEEPER_FILE, LOCALSTATEDIR"/lib/deborphan/keep", [Location of your keepfile])
AC_DEFINE(REG_FLAGS, REG_ICASE, [Bitwise or'd list of flags for regcomp()])

CFLAGS="-Wall -W $CFLAGS"
AC_PROG_CC
ALL_LINGUAS="ca cs da de es eu fr ja nl pl ru pt it vi"

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

AC_PROG_INSTALL
AC_CHECK_FUNCS(getopt_long, ,AC_MSG_ERROR([You need getopt_long()]))

AC_MSG_CHECKING(debfoster's keepers file)
if [[ -r /var/state/debfoster/keepers ]]; then
  dffile="/var/state/debfoster/keepers"
  AC_MSG_RESULT($dffile)
else 
  if [[ -r /etc/apt/keepers ]]; then
    dffile="/etc/apt/keepers"
    AC_MSG_RESULT($dffile)
  else
    with_debfoster="no"
    AC_MSG_RESULT("not found.")
  fi
fi 

AC_MSG_CHECKING(whether to ignore debfoster)
AC_ARG_WITH(debfoster,
[  --without-debfoster     Ignore debfoster's keepers file. ],
[if [[ "$withval" = "no" ]]; then
  ignore_df=1
  AC_MSG_RESULT(yes)
else
  ignore_df=0
  AC_MSG_RESULT(no)
fi], [ ignore_df=0; AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether to imply -s)
AC_ARG_WITH(isfa,
[  --without-isfa          Disable implication of -s when using -a. ],
[if [[ "$withval" = "no" ]]; then
  isfa=0
  AC_MSG_RESULT(no)
else
  isfa=1
  AC_MSG_RESULT(yes)
fi], [ isfa=1; AC_MSG_RESULT(yes) ]
)

AC_MSG_CHECKING(default priority)
AC_ARG_WITH(priority,
[  --with-priority=PRIOR   Default minimum priority. ],
[if echo $withval | grep -E '^[[[:digit:]]]+$' >/dev/null; then
  priority=$withval
  AC_MSG_RESULT($priority)
else
  AC_MSG_ERROR(invalid priority: $withval)
fi], [ priority=2; AC_MSG_RESULT(2) ]
)

AC_MSG_CHECKING(ignore essential)
AC_ARG_WITH(ignore-essential,
[  --with-ignore-essential Ignore 'Essential: yes' lines. ],
[if [[ "$withval" = "yes" ]]; then
  essential=0
  AC_MSG_RESULT(yes)
else
  essential=1
  AC_MSG_RESULT(no)
fi], [ essential=1; AC_MSG_RESULT(no) ]
)

AC_DEFINE_UNQUOTED(DEFAULT_PRIORITY, $priority, [Default minimum priority])

if [[ $isfa -eq 1 ]]; then
  AC_DEFINE(ALL_PACKAGES_IMPLY_SECTION, 1, [Imply --show-section when using --all-packages.])
fi
if [[ $ignore_df -eq 1 ]]; then
  AC_DEFINE(IGNORE_DEBFOSTER, 1, [Ignore debfoster's keepers file.])
fi
if [[ ! $essential -eq 1 ]]; then
  AC_DEFINE(IGNORE_ESSENTIAL, 1, [Ignore 'Essential: yes' lines])
fi
if [[ ! -z $dffile ]]; then
  AC_DEFINE_UNQUOTED(DEBFOSTER_KEEP, "$dffile", [Location of debfoster's keepers file.])
fi

AC_OUTPUT(Makefile src/Makefile doc/Makefile doc/po/Makefile doc/fr/Makefile doc/pl/Makefile doc/es/Makefile doc/de/Makefile doc/pt/Makefile doc/nl/Makefile include/Makefile po/Makefile.in)
