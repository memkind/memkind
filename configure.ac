# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2021 Intel Corporation.

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
AC_SEARCH_LIBS([ceilf], [m])

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

#============================log_to_file=======================================
AC_ARG_ENABLE([logging_to_file],
  [AS_HELP_STRING([--enable-logging-to-file], [Log to file for each process])],
[if test "x$enable_logging_to_file" = "xyes" ; then
  enable_logging_to_file="1"
else
  enable_logging_to_file="0"
fi
],
[enable_logging_to_file="0"]
)
if test "x${enable_logging_to_file}" = "x1" ; then
  AC_DEFINE([MEMKIND_LOG_TO_FILE], [ ], [Log to file for each process])
fi
AC_SUBST([enable_logging_to_file])

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

#=======================jemalloc_debug=========================================
AC_ARG_ENABLE([debug-jemalloc],
  [AS_HELP_STRING([--enable-debug-jemalloc], [Build debugging jemalloc code])],
[if test "x$enable_debug_jemalloc" = "xno" ; then
  enable_debug_jemalloc="0"
else
  enable_debug_jemalloc="1"
fi
],
[enable_debug_jemalloc="0"]
)

if test "x$enable_debug_jemalloc" = "x1" ; then
  jemalloc_build_type_flags=--enable-debug
else
  jemalloc_build_type_flags=--disable-fill
fi
AC_SUBST([jemalloc_build_type_flags])

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

#============================secure_getenv===============================
AC_CHECK_FUNC([secure_getenv],
              [have_secure_getenv="1"],
              [have_secure_getenv="0"]
             )
if test "x$have_secure_getenv" = "x1" ; then
  AC_DEFINE([MEMKIND_HAVE_SECURE_GETENV], [], [Detect secure_getenv])
fi

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

#============================memtier_prefix===========================================
memtier_prefix=je_;
AC_ARG_VAR(MEMTIER_PREFIX,
  [Prefix used for intercept additional malloc API with libmemtier, default value je_]
)
if test "$MEMTIER_PREFIX" != "" ; then
  memtier_prefix=$MEMTIER_PREFIX;
fi

AC_SUBST(memtier_prefix)
if test "$memtier_prefix" == "$memkind_prefix" ; then
  AC_MSG_ERROR([MEMTIER_PREFIX and MEMKIND_PREFIX cannot be the same])
fi
#============================min_log_alignment=====================================
min_lg_align_opt="";
AC_ARG_VAR(MIN_LG_ALIGN,
  [minimum allocation alignment (base 2 log) used in jemalloc, default value is architecture specific]
)
if test "$MIN_LG_ALIGN" != "" ; then
  min_lg_align_opt=--with-lg-quantum=$MIN_LG_ALIGN;
fi

AC_SUBST(min_lg_align_opt)

#============================memkind-initial-exec-tls=====================================
AC_ARG_ENABLE([memkind-initial-exec-tls],
  [AS_HELP_STRING([--enable-memkind-initial-exec-tls], [Build library with initial-exec-tls support])],
[if test "x$enable_memkind_initial_exec_tls" = "xno" ; then
  enable_memkind_initial_exec_tls="0"
else
  enable_memkind_initial_exec_tls="1"
fi
],
[enable_memkind_initial_exec_tls="0"]
)
if test "x$enable_memkind_initial_exec_tls" = "x0" ; then
  memkind_initial_exec_tls=--disable-initial-exec-tls;
fi

AC_SUBST(memkind_initial_exec_tls)

#===============================daxctl=========================================
AC_ARG_ENABLE([daxctl],
  [AS_HELP_STRING([--enable-daxctl], [Build library with daxctl support])],
[if test "x$enable_daxctl" = "xno" ; then
  enable_daxctl="0"
else
  enable_daxctl="2"
fi
],
[enable_daxctl="1"]
)
if test "x$enable_daxctl" = "x1" ; then
    AC_CHECK_LIB(daxctl, daxctl_dev_get_memory, [daxctl_kmem=yes], [daxctl_kmem=no])
fi
if test "x$enable_daxctl" = "x2" ; then
    AC_CHECK_LIB(daxctl, daxctl_dev_get_memory, [daxctl_kmem=yes], [AC_MSG_ERROR([libdaxctl-devel v66 or later is required dependency by --enable-daxctl option])])
fi

if test $daxctl_kmem = "yes" ; then
    AC_DEFINE([MEMKIND_DAXCTL_KMEM], [1], [Automatic recognition of PMEM as system memory in MEMKIND_DAX_KMEM])
    AC_SUBST(DAXCTL_LIBS, [-ldaxctl])
    LDFLAGS="$LDFLAGS -ldaxctl"
fi
AC_SUBST([enable_daxctl])

#===============================hwloc=========================================
AC_ARG_ENABLE([hwloc],
  [AS_HELP_STRING([--enable-hwloc], [Build library with hwloc support])],
[if test "x$enable_hwloc" = "xno" ; then
  enable_hwloc="0"
else
  enable_hwloc="2"
fi
],
[enable_hwloc="1"]
)
if test "x$enable_hwloc" = "x1" ; then
    AC_CHECK_LIB(hwloc, hwloc_memattr_get_best_target, [hwloc_hmat=yes], [hwloc_hmat=no])
fi
if test "x$enable_hwloc" = "x2" ; then
    AC_CHECK_LIB(hwloc, hwloc_memattr_get_best_target, [hwloc_hmat=yes], [AC_MSG_ERROR([libhwloc-devel v2.3.0 or later is required dependency by --enable-hwloc option])])
fi

if test $hwloc_hmat = "yes" ; then
    AC_DEFINE([MEMKIND_HWLOC], [1], [Support for hwloc library])
    AC_SUBST(HWLOC_LIBS, [-lhwloc])
    LDFLAGS="$LDFLAGS -lhwloc"
fi
AC_SUBST([enable_hwloc])

#===============================heap manager=========================================
AC_ARG_ENABLE([heap_manager],
  [AS_HELP_STRING([--disable-heap-manager], [Use only default allocator: jemalloc])],
[if test "x$enable_heap_manager" = "xno" ; then
  enable_heap_manager="0"
else
  enable_heap_manager="1"
fi
],
[enable_heap_manager="1"]
)
if test "x$enable_heap_manager" = "x1" ; then
  AC_DEFINE([MEMKIND_ENABLE_HEAP_MANAGER], [], [Enable heap manager])
fi
AC_SUBST([enable_heap_manager])

#===============================clang-format====================================
clang_support=no;
AC_CHECK_PROGS(CLANG_FORMAT, [clang-format-11 clang-format], "")
if test "$CLANG_FORMAT" != ""; then
  CLANG_FORMAT_VERSION=$($CLANG_FORMAT --version)
  AX_COMPARE_VERSION([$CLANG_FORMAT_VERSION], [ge], [11.0.0], [clang_support=yes], [])
fi
AC_SUBST([clang_format], $CLANG_FORMAT)
AM_CONDITIONAL([HAVE_CLANG_FORMAT], [test "x$clang_support" = xyes])
AC_CHECK_PROGS(pytest, [pytest pytest-3], false)

#===============================atomic=========================================
AC_CHECK_HEADERS(stdatomic.h)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdint.h>
#include <stdatomic.h>
]],[[
atomic_int v = 8;
atomic_fetch_add_explicit(&v, 1, memory_order_relaxed);
]])], [memkind_c11_atomics="1"], [memkind_c11_atomics="0"])
if test "x$memkind_c11_atomics" = "x1" ; then
  AC_DEFINE([MEMKIND_ATOMIC_C11_SUPPORT], [1], [C11 Atomics are supported])
fi
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[
int v = 8;
__atomic_fetch_add(&v, 1, __ATOMIC_RELAXED);
]])], [memkind_atomic_builtins="1"], [memkind_atomic_builtins="0"])
if test "x$memkind_atomic_builtins" = "x1" ; then
  AC_DEFINE([MEMKIND_ATOMIC_BUILTINS_SUPPORT], [1], [__atomic_c* compiler builtins are supported])
fi
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[
int v = 8;
__sync_fetch_and_add(&v, 1);
]])], [memkind_sync_fetch="1"], [memkind_sync_fetch="0"])
if test "x$memkind_sync_fetch" = "x1" ; then
  AC_DEFINE([MEMKIND_ATOMIC_SYNC_SUPPORT], [1], [__sync_* compiler builtins are supported])
fi

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
if test $hwloc_hmat = "yes" ; then
AC_MSG_RESULT([
               Utilization of information about memory performance characteristics is enabled])
else
AC_MSG_RESULT([
               Utilization of information about memory performance characteristics is disabled
               To enable this feature please install libhwloc-devel v2.3.0 or later
               and run configure again])
fi
if test "$clang_support" = "yes" ; then
AC_MSG_RESULT([
               Automatic code style formatting is enabled
               Calling 'make code-style-check' will print suggested style changes
               Calling 'make code-style-apply' will apply suggested style changes to code])
else
AC_MSG_RESULT([
               Automatic code style formatting is disabled. To enable this feature please
               install clang-format v11.0.0 or later and run configure again.])
fi
AC_MSG_RESULT([=================================================================================])
