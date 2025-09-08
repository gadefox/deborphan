/* set.h
   Copyright (C) 2000, 2001, 2002, 2003 Cris van Pelt
   Copyright (C) 2003, 2004 Peter Palfrader

   Distributed under the terms of the MIT License, see the
   file COPYING provided in this package for details.
*/
#pragma once

#include "deborphan.h"

#define set_hold(p) ((p)->hold = 1)
#define set_config(p) ((p)->config = 1)
#define set_install(p) ((p)->install = 1)

dep* set_dep(dep*, const char*);
dep* set_provides(pkg_info*, const char*, const int);
void set_section(pkg_info*, const char*, const char*);
int set_priority(pkg_info*, const char*);
void init_pkg(pkg_info*);
void reinit_pkg(pkg_info*);
