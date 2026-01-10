# Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
# Copyright (C) 2003, 2004, 2006 Peter Palfrader

AUTOMAKE_OPTIONS = foreign
SUBDIRS = src po doc include
EXTRA_DIST = \
	debian/README.frontends \
	debian/bash-complete \
	debian/changelog \
	debian/control \
	debian/copyright \
	debian/dirs \
	debian/docs \
	debian/menu \
	debian/postinst \
	debian/postrm \
	debian/rules \
	debian/compat \
	autogen.sh \
	mkinstalldirs

ACLOCAL_AMFLAGS = -I m4


.PHONY: reformat

reformat:
	clang-format -i $$(find . -name '*.c' -o -name '*.h')
