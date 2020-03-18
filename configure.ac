#
#  Copyright (C) 2014 - 2020 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([memkind],m4_esyscmd([tr -d '\n' < VERSION]))

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([memkind.spec.mk])

AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.11 silent-rules subdir-objects parallel-tests tar-pax])
AM_SILENT_RULES([yes])

# Checks for programs and libraries.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CXX
AC_PROG_CC
AC_OPENMP
AC_CHECK_LIB(numa, numa_available, [], [AC_MSG_ERROR([libnuma is required dependency])])
AX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS" CFLAGS="$CFLAGS $PTHREAD_CFLAGS" CC="$PTHREAD_CC"],
           [AC_MSG_ERROR([pthreads are required dependency])])

AM_PROG_CC_C_O

#============================tls===============================================
# Check for thread local storage support
AC_ARG_ENABLE([tls],
  [AS_HELP_STRING([--enable-tls], [Enable thread-local storage (__thread keyword)])],
[if test "x$enable_tls" = "xyes" ; then
  enable_tls="1"
else
  enable_tls="0"
fi
],
[enable_tls="0"]
)
if test "x${enable_tls}" = "x1" ; then
AC_MSG_CHECKING([for TLS support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[
    __thread int x;
]], [[
    x = 1234;
]])],
    AC_MSG_RESULT([yes]),
    AC_MSG_RESULT([no])
    enable_tls="0")
fi
if test "x${enable_tls}" = "x1" ; then
  AC_DEFINE([MEMKIND_TLS], [ ], [Enables TLS usage for mapping arenas to threads])
fi
AC_SUBST([enable_tls])

#============================decorators========================================
AC_ARG_ENABLE([decorators],
  [AS_HELP_STRING([--enable-decorators], [Enable decorators])],
[if test "x$enable_decorators" = "xyes" ; then
  enable_decorators="1"
else
  enable_decorators="0"
fi
],
[enable_decorators="0"]
)
if test "x${enable_decorators}" = "x1" ; then
  AC_DEFINE([MEMKIND_DECORATION_ENABLED], [ ], [Enables decorators])
fi
AC_SUBST([enable_decorators])

#============================debug=============================================
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Build debugging code and compile with -O0 -g])],
[if test "x$enable_debug" = "xno" ; then
  enable_debug="0"
else
  enable_debug="1"
fi
],
[enable_debug="0"]
)
if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([MEMKIND_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -O0 -g"
  CXXFLAGS="$CXXFLAGS -O0 -g"
fi
AC_SUBST([enable_debug])

#============================gcov==============================================
AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov], [Build code with gcov instructions])],
[if test "x$enable_gcov" = "xno" ; then
  enable_gcov="0"
else
  enable_gcov="1"
fi
],
[enable_gcov="0"]
)
if test "x$enable_gcov" = "x1" ; then
  CFLAGS="$CFLAGS -O0 -fprofile-arcs -ftest-coverage"
  CXXFLAGS="$CXXFLAGS -O0 -fprofile-arcs -ftest-coverage"
fi
AC_SUBST([enable_gcov])

#============================secure_flags======================================
AC_ARG_ENABLE([secure],
  [AS_HELP_STRING([--enable-secure], [Build library with security enchantments])],
[if test "x$enable_secure" = "xno" ; then
  enable_secure="0"
else
  enable_secure="1"
fi
],
[enable_secure="1"]
)
if test "x$enable_secure" = "x1" ; then
  CFLAGS="$CFLAGS -fstack-protector"
  LDFLAGS="$LDFLAGS -Wl,-z,relro,-z,now"

  if test "$CFLAGS" != "${CFLAGS%-O0*}" ; then # if CFLAGS contains -O0
      echo "WARNING: Could not apply FORTIFY_SOURCE=2 due to lack of optimization (-O0)"
  else
      CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2" #FORTITFY_SOURCE does not work with -O0 (ex. if enable_debug=1 or enable_gcov=1)
  fi
fi

AC_SUBST([enable_secure])

#============================tcache_max_size_class======================

tcache_max_size_class=12;
AC_DEFINE_UNQUOTED([JEMALLOC_TCACHE_CLASS], $tcache_max_size_class,
  [Maximum size class (log base 2) to cache in jemalloc tcache])

AC_SUBST(tcache_max_size_class)

#============================auto_arenas================================
auto_arenas=256;
AC_DEFINE_UNQUOTED([ARENA_LIMIT_DEFAULT_KIND], $auto_arenas,
  [Maximum number of automatic managed arenas by jemalloc (used by MEMKIND_DEFAULT)])

AC_SUBST(auto_arenas)

#============================arena_limit=======================================
arena_limit=256;
AC_ARG_VAR(ARENA_LIMIT,
  [Upper bound for number of arenas per kind, if set to 0 then no limit]
)
if test "$ARENA_LIMIT" != "" ; then
  arena_limit=$ARENA_LIMIT;
fi

AC_DEFINE_UNQUOTED([ARENA_LIMIT_PER_KIND], $arena_limit, [Upper bound for number of arenas per kind])

#============================memkind_prefix=======================================
memkind_prefix=jemk_;
AC_ARG_VAR(MEMKIND_PREFIX,
  [Prefix used for all jemalloc public API under memkind, default value jemk_]
)
if test "$MEMKIND_PREFIX" != "" ; then
  memkind_prefix=$MEMKIND_PREFIX;
fi

AC_SUBST(memkind_prefix)

#============================min_log_alignment=====================================
min_lg_align_opt="";
AC_ARG_VAR(MIN_LG_ALIGN,
  [minimum allocation alignment (base 2 log) used in jemalloc, default value is architecture specific]
)
if test "$MIN_LG_ALIGN" != "" ; then
  min_lg_align_opt=--with-lg-quantum=$MIN_LG_ALIGN;
fi

AC_SUBST(min_lg_align_opt)

#===============================daxctl=========================================
AC_ARG_ENABLE([daxctl],
  [AS_HELP_STRING([--enable-daxctl], [Build library with daxctl support])],
[if test "x$enable_daxctl" = "xno" ; then
  enable_daxctl="0"
else
  enable_daxctl="1"
fi
],
[enable_daxctl="1"]
)
if test "x$enable_daxctl" = "x1" ; then
    AC_CHECK_LIB(daxctl, daxctl_dev_get_memory, [daxctl_kmem=yes], [daxctl_kmem=no])
fi
if test $daxctl_kmem = "yes" ; then
    AC_DEFINE([MEMKIND_DAXCTL_KMEM], [1], [Automatic recognition of PMEM as system memory in MEMKIND_DAX_KMEM])
    AC_SUBST(DAXCTL_LIBS, [-ldaxctl])
    LDFLAGS="$LDFLAGS -ldaxctl"
fi
AC_SUBST([enable_daxctl])

#============================cxx11=============================================

AX_CXX_COMPILE_STDCXX_11([noext], [optional])
AM_CONDITIONAL([HAVE_CXX11], [test "x$HAVE_CXX11" = x1])

LT_PREREQ([2.2])
LT_INIT

AC_CONFIG_FILES([Makefile memkind.pc])

AC_OUTPUT
AC_MSG_RESULT([=================================================================================])
AC_MSG_RESULT([
               Memkind version $VERSION])
if test $daxctl_kmem = "yes" ; then
AC_MSG_RESULT([
               Automatic recognition of PMEM NUMA in MEMKIND_DAX_KMEM is enabled])
else
AC_MSG_RESULT([
               Automatic recognition of PMEM NUMA in MEMKIND_DAX_KMEM is disabled
               To enable this feature please install libdaxctl-devel v66 or later
               and run configure again])
fi
AC_MSG_RESULT([=================================================================================])
