# Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
# Copyright (C) 2003, 2004 Peter Palfrader

bin_PROGRAMS = deborphan
deborphan_SOURCES =  deborphan.c exit.c libdeps.c pkginfo.c string.c keep.c file.c set.c

localedir = $(datadir)/locale

AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -DLOCALSTATEDIR=\"$(localstatedir)\" \
    -DLOCALEDIR=\"$(localedir)\"
