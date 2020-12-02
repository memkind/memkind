# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2020 Intel Corporation.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/jemalloc/include
MAN2HTML = groff -mandoc -Thtml

lib_LTLIBRARIES = libmemkind.la

libmemkind_la_SOURCES = src/hbwmalloc.c \
                        src/heap_manager.c \
                        src/memkind.c \
                        src/memkind_arena.c \
                        src/memkind_bandwidth.c \
                        src/memkind_dax_kmem.c \
                        src/memkind_default.c \
                        src/memkind_gbtlb.c \
                        src/memkind_hbw.c \
                        src/memkind_hugetlb.c \
                        src/memkind_interleave.c \
                        src/memkind_log.c \
                        src/memkind_pmem.c \
                        src/memkind_regular.c \
                        src/tbb_wrapper.c \
                        # end


libmemkind_la_LIBADD = jemalloc/lib/libjemalloc_pic.a
libmemkind_la_LDFLAGS = -version-info 0:1:0 -ldl -lrt
include_HEADERS = include/hbw_allocator.h \
                  include/hbwmalloc.h \
                  include/memkind.h \
                  include/memkind_allocator.h \
                  include/memkind_deprecated.h \
                  include/pmem_allocator.h \
                  # end

noinst_HEADERS =  include/memkind/internal/heap_manager.h \
                  include/memkind/internal/memkind_arena.h \
                  include/memkind/internal/memkind_bandwidth.h \
                  include/memkind/internal/memkind_dax_kmem.h \
                  include/memkind/internal/memkind_default.h \
                  include/memkind/internal/memkind_gbtlb.h \
                  include/memkind/internal/memkind_hbw.h \
                  include/memkind/internal/memkind_hugetlb.h \
                  include/memkind/internal/memkind_interleave.h \
                  include/memkind/internal/memkind_log.h \
                  include/memkind/internal/memkind_pmem.h \
                  include/memkind/internal/memkind_private.h \
                  include/memkind/internal/memkind_regular.h \
                  include/memkind/internal/tbb_mem_pool_policy.h \
                  include/memkind/internal/tbb_wrapper.h \
                  include/memkind/internal/vec.h \
                  # end

EXTRA_DIST = CONTRIBUTING \
             VERSION \
             astyle.sh \
             autogen.sh \
             build.sh \
             examples/README \
             jemalloc/.appveyor.yml \
             jemalloc/.autom4te.cfg \
             jemalloc/.gitattributes\
             jemalloc/.gitignore \
             jemalloc/.travis.yml \
             jemalloc/autogen.sh \
             jemalloc/bin/jemalloc-config.in \
             jemalloc/bin/jemalloc.sh.in \
             jemalloc/bin/jeprof.in \
             jemalloc/build-aux/config.guess \
             jemalloc/build-aux/config.sub \
             jemalloc/build-aux/install-sh \
             jemalloc/ChangeLog \
             jemalloc/config.stamp.in \
             jemalloc/configure.ac \
             jemalloc/COPYING \
             jemalloc/doc/html.xsl.in \
             jemalloc/doc/jemalloc.xml.in \
             jemalloc/doc/manpages.xsl.in \
             jemalloc/doc/stylesheet.xsl \
             jemalloc/include/jemalloc/internal/arena_externs.h \
             jemalloc/include/jemalloc/internal/arena_inlines_a.h \
             jemalloc/include/jemalloc/internal/arena_inlines_b.h \
             jemalloc/include/jemalloc/internal/arena_stats.h \
             jemalloc/include/jemalloc/internal/arena_structs_a.h \
             jemalloc/include/jemalloc/internal/arena_structs_b.h \
             jemalloc/include/jemalloc/internal/arena_types.h \
             jemalloc/include/jemalloc/internal/assert.h \
             jemalloc/include/jemalloc/internal/atomic.h \
             jemalloc/include/jemalloc/internal/atomic_c11.h \
             jemalloc/include/jemalloc/internal/atomic_gcc_atomic.h \
             jemalloc/include/jemalloc/internal/atomic_gcc_sync.h \
             jemalloc/include/jemalloc/internal/atomic_msvc.h \
             jemalloc/include/jemalloc/internal/background_thread_externs.h \
             jemalloc/include/jemalloc/internal/background_thread_inlines.h \
             jemalloc/include/jemalloc/internal/background_thread_structs.h \
             jemalloc/include/jemalloc/internal/base_externs.h \
             jemalloc/include/jemalloc/internal/base_inlines.h \
             jemalloc/include/jemalloc/internal/base_structs.h \
             jemalloc/include/jemalloc/internal/base_types.h \
             jemalloc/include/jemalloc/internal/bin.h \
             jemalloc/include/jemalloc/internal/bin_stats.h \
             jemalloc/include/jemalloc/internal/bin_types.h \
             jemalloc/include/jemalloc/internal/bitmap.h \
             jemalloc/include/jemalloc/internal/bit_util.h \
             jemalloc/include/jemalloc/internal/cache_bin.h \
             jemalloc/include/jemalloc/internal/ckh.h \
             jemalloc/include/jemalloc/internal/ctl.h \
             jemalloc/include/jemalloc/internal/div.h \
             jemalloc/include/jemalloc/internal/emitter.h \
             jemalloc/include/jemalloc/internal/extent_dss.h \
             jemalloc/include/jemalloc/internal/extent_externs.h \
             jemalloc/include/jemalloc/internal/extent_inlines.h \
             jemalloc/include/jemalloc/internal/extent_mmap.h \
             jemalloc/include/jemalloc/internal/extent_structs.h \
             jemalloc/include/jemalloc/internal/extent_types.h \
             jemalloc/include/jemalloc/internal/hash.h \
             jemalloc/include/jemalloc/internal/hook.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_decls.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_defs.h.in \
             jemalloc/include/jemalloc/internal/jemalloc_internal_externs.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_includes.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_inlines_a.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_inlines_b.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_inlines_c.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_macros.h \
             jemalloc/include/jemalloc/internal/jemalloc_internal_types.h \
             jemalloc/include/jemalloc/internal/jemalloc_preamble.h.in \
             jemalloc/include/jemalloc/internal/large_externs.h \
             jemalloc/include/jemalloc/internal/log.h \
             jemalloc/include/jemalloc/internal/malloc_io.h \
             jemalloc/include/jemalloc/internal/mutex.h \
             jemalloc/include/jemalloc/internal/mutex_pool.h \
             jemalloc/include/jemalloc/internal/mutex_prof.h \
             jemalloc/include/jemalloc/internal/nstime.h \
             jemalloc/include/jemalloc/internal/pages.h \
             jemalloc/include/jemalloc/internal/ph.h \
             jemalloc/include/jemalloc/internal/private_namespace.sh \
             jemalloc/include/jemalloc/internal/private_symbols.sh \
             jemalloc/include/jemalloc/internal/prng.h \
             jemalloc/include/jemalloc/internal/prof_externs.h \
             jemalloc/include/jemalloc/internal/prof_inlines_a.h \
             jemalloc/include/jemalloc/internal/prof_inlines_b.h \
             jemalloc/include/jemalloc/internal/prof_structs.h \
             jemalloc/include/jemalloc/internal/prof_types.h \
             jemalloc/include/jemalloc/internal/public_namespace.sh \
             jemalloc/include/jemalloc/internal/public_unnamespace.sh \
             jemalloc/include/jemalloc/internal/ql.h \
             jemalloc/include/jemalloc/internal/qr.h \
             jemalloc/include/jemalloc/internal/quantum.h \
             jemalloc/include/jemalloc/internal/rb.h \
             jemalloc/include/jemalloc/internal/rtree.h \
             jemalloc/include/jemalloc/internal/rtree_tsd.h \
             jemalloc/include/jemalloc/internal/safety_check.h \
             jemalloc/include/jemalloc/internal/sc.h \
             jemalloc/include/jemalloc/internal/seq.h \
             jemalloc/include/jemalloc/internal/smoothstep.h \
             jemalloc/include/jemalloc/internal/smoothstep.sh \
             jemalloc/include/jemalloc/internal/spin.h \
             jemalloc/include/jemalloc/internal/stats.h \
             jemalloc/include/jemalloc/internal/sz.h \
             jemalloc/include/jemalloc/internal/tcache_externs.h \
             jemalloc/include/jemalloc/internal/tcache_inlines.h \
             jemalloc/include/jemalloc/internal/tcache_structs.h \
             jemalloc/include/jemalloc/internal/tcache_types.h \
             jemalloc/include/jemalloc/internal/test_hooks.h \
             jemalloc/include/jemalloc/internal/ticker.h \
             jemalloc/include/jemalloc/internal/tsd.h \
             jemalloc/include/jemalloc/internal/tsd_generic.h \
             jemalloc/include/jemalloc/internal/tsd_malloc_thread_cleanup.h \
             jemalloc/include/jemalloc/internal/tsd_tls.h \
             jemalloc/include/jemalloc/internal/tsd_types.h \
             jemalloc/include/jemalloc/internal/tsd_win.h \
             jemalloc/include/jemalloc/internal/util.h \
             jemalloc/include/jemalloc/internal/witness.h \
             jemalloc/include/jemalloc/jemalloc.sh \
             jemalloc/include/jemalloc/jemalloc_defs.h.in \
             jemalloc/include/jemalloc/jemalloc_macros.h.in \
             jemalloc/include/jemalloc/jemalloc_mangle.sh \
             jemalloc/include/jemalloc/jemalloc_protos.h.in \
             jemalloc/include/jemalloc/jemalloc_rename.sh \
             jemalloc/include/jemalloc/jemalloc_typedefs.h.in \
             jemalloc/include/msvc_compat/C99/stdbool.h \
             jemalloc/include/msvc_compat/C99/stdint.h \
             jemalloc/include/msvc_compat/strings.h \
             jemalloc/include/msvc_compat/windows_extra.h \
             jemalloc/INSTALL.md \
             jemalloc/jemalloc.pc.in \
             jemalloc/m4/ax_cxx_compile_stdcxx.m4 \
             jemalloc/Makefile.in \
             jemalloc/msvc/jemalloc_vc2015.sln \
             jemalloc/msvc/jemalloc_vc2017.sln \
             jemalloc/msvc/projects/vc2015/jemalloc/jemalloc.vcxproj \
             jemalloc/msvc/projects/vc2015/jemalloc/jemalloc.vcxproj.filters \
             jemalloc/msvc/projects/vc2015/test_threads/test_threads.vcxproj \
             jemalloc/msvc/projects/vc2015/test_threads/test_threads.vcxproj.filters \
             jemalloc/msvc/projects/vc2017/jemalloc/jemalloc.vcxproj \
             jemalloc/msvc/projects/vc2017/jemalloc/jemalloc.vcxproj.filters \
             jemalloc/msvc/projects/vc2017/test_threads/test_threads.vcxproj \
             jemalloc/msvc/projects/vc2017/test_threads/test_threads.vcxproj.filters \
             jemalloc/msvc/ReadMe.txt \
             jemalloc/msvc/test_threads/test_threads.cpp \
             jemalloc/msvc/test_threads/test_threads.h \
             jemalloc/msvc/test_threads/test_threads_main.cpp \
             jemalloc/README \
             jemalloc/run_tests.sh \
             jemalloc/scripts/gen_run_tests.py \
             jemalloc/scripts/gen_travis.py \
             jemalloc/src/arena.c \
             jemalloc/src/background_thread.c \
             jemalloc/src/base.c \
             jemalloc/src/bin.c \
             jemalloc/src/bitmap.c \
             jemalloc/src/ckh.c \
             jemalloc/src/ctl.c \
             jemalloc/src/div.c \
             jemalloc/src/extent.c \
             jemalloc/src/extent_dss.c \
             jemalloc/src/extent_mmap.c \
             jemalloc/src/hash.c \
             jemalloc/src/hook.c \
             jemalloc/src/jemalloc.c \
             jemalloc/src/jemalloc_cpp.cpp \
             jemalloc/src/large.c \
             jemalloc/src/log.c \
             jemalloc/src/malloc_io.c \
             jemalloc/src/mutex.c \
             jemalloc/src/mutex_pool.c \
             jemalloc/src/nstime.c \
             jemalloc/src/pages.c \
             jemalloc/src/prng.c \
             jemalloc/src/prof.c \
             jemalloc/src/rtree.c \
             jemalloc/src/safety_check.c \
             jemalloc/src/sc.c \
             jemalloc/src/stats.c \
             jemalloc/src/sz.c \
             jemalloc/src/tcache.c \
             jemalloc/src/test_hooks.c \
             jemalloc/src/ticker.c \
             jemalloc/src/tsd.c \
             jemalloc/src/witness.c \
             jemalloc/src/zone.c \
             jemalloc/test/include/test/btalloc.h \
             jemalloc/test/include/test/extent_hooks.h \
             jemalloc/test/include/test/jemalloc_test.h.in \
             jemalloc/test/include/test/jemalloc_test_defs.h.in \
             jemalloc/test/include/test/math.h \
             jemalloc/test/include/test/mq.h \
             jemalloc/test/include/test/mtx.h \
             jemalloc/test/include/test/SFMT-alti.h \
             jemalloc/test/include/test/SFMT-params.h \
             jemalloc/test/include/test/SFMT-params607.h \
             jemalloc/test/include/test/SFMT-params1279.h \
             jemalloc/test/include/test/SFMT-params2281.h \
             jemalloc/test/include/test/SFMT-params4253.h \
             jemalloc/test/include/test/SFMT-params11213.h \
             jemalloc/test/include/test/SFMT-params19937.h \
             jemalloc/test/include/test/SFMT-params44497.h \
             jemalloc/test/include/test/SFMT-params86243.h \
             jemalloc/test/include/test/SFMT-params132049.h \
             jemalloc/test/include/test/SFMT-params216091.h \
             jemalloc/test/include/test/SFMT-sse2.h \
             jemalloc/test/include/test/SFMT.h \
             jemalloc/test/include/test/test.h \
             jemalloc/test/include/test/thd.h \
             jemalloc/test/include/test/timer.h \
             jemalloc/test/integration/aligned_alloc.c \
             jemalloc/test/integration/allocated.c \
             jemalloc/test/integration/extent.c \
             jemalloc/test/integration/extent.sh \
             jemalloc/test/integration/malloc.c \
             jemalloc/test/integration/mallocx.c \
             jemalloc/test/integration/mallocx.sh \
             jemalloc/test/integration/MALLOCX_ARENA.c \
             jemalloc/test/integration/overflow.c \
             jemalloc/test/integration/posix_memalign.c \
             jemalloc/test/integration/rallocx.c \
             jemalloc/test/integration/sdallocx.c \
             jemalloc/test/integration/slab_sizes.c \
             jemalloc/test/integration/slab_sizes.sh \
             jemalloc/test/integration/smallocx.c \
             jemalloc/test/integration/smallocx.sh \
             jemalloc/test/integration/thread_arena.c \
             jemalloc/test/integration/thread_tcache_enabled.c \
             jemalloc/test/integration/xallocx.c \
             jemalloc/test/integration/xallocx.sh \
             jemalloc/test/src/btalloc.c \
             jemalloc/test/src/btalloc_0.c \
             jemalloc/test/src/btalloc_1.c \
             jemalloc/test/src/math.c \
             jemalloc/test/src/mq.c \
             jemalloc/test/src/mtx.c \
             jemalloc/test/src/SFMT.c \
             jemalloc/test/src/test.c \
             jemalloc/test/src/thd.c  \
             jemalloc/test/src/timer.c \
             jemalloc/test/stress/hookbench.c \
             jemalloc/test/stress/microbench.c \
             jemalloc/test/test.sh.in \
             jemalloc/test/unit/a0.c \
             jemalloc/test/unit/arena_reset.c \
             jemalloc/test/unit/arena_reset_prof.c \
             jemalloc/test/unit/arena_reset_prof.sh \
             jemalloc/test/unit/atomic.c \
             jemalloc/test/unit/background_thread.c \
             jemalloc/test/unit/background_thread_enable.c \
             jemalloc/test/unit/base.c \
             jemalloc/test/unit/binshard.c \
             jemalloc/test/unit/binshard.sh \
             jemalloc/test/unit/bitmap.c \
             jemalloc/test/unit/bit_util.c \
             jemalloc/test/unit/ckh.c \
             jemalloc/test/unit/decay.c \
             jemalloc/test/unit/decay.sh \
             jemalloc/test/unit/div.c \
             jemalloc/test/unit/emitter.c \
             jemalloc/test/unit/extent_quantize.c\
             jemalloc/test/unit/extent_util.c \
             jemalloc/test/unit/fork.c \
             jemalloc/test/unit/hash.c \
             jemalloc/test/unit/hook.c \
             jemalloc/test/unit/huge.c \
             jemalloc/test/unit/junk.c \
             jemalloc/test/unit/junk.sh \
             jemalloc/test/unit/junk_alloc.c \
             jemalloc/test/unit/junk_alloc.sh \
             jemalloc/test/unit/junk_free.c \
             jemalloc/test/unit/junk_free.sh \
             jemalloc/test/unit/log.c \
             jemalloc/test/unit/mallctl.c \
             jemalloc/test/unit/malloc_io.c \
             jemalloc/test/unit/math.c \
             jemalloc/test/unit/mq.c \
             jemalloc/test/unit/mtx.c \
             jemalloc/test/unit/nstime.c \
             jemalloc/test/unit/pack.c \
             jemalloc/test/unit/pack.sh \
             jemalloc/test/unit/pages.c \
             jemalloc/test/unit/ph.c \
             jemalloc/test/unit/prng.c \
             jemalloc/test/unit/prof_accum.c \
             jemalloc/test/unit/prof_accum.sh \
             jemalloc/test/unit/prof_active.c \
             jemalloc/test/unit/prof_active.sh \
             jemalloc/test/unit/prof_gdump.c \
             jemalloc/test/unit/prof_gdump.sh \
             jemalloc/test/unit/prof_idump.c \
             jemalloc/test/unit/prof_idump.sh \
             jemalloc/test/unit/prof_log.c \
             jemalloc/test/unit/prof_log.sh \
             jemalloc/test/unit/prof_reset.c \
             jemalloc/test/unit/prof_reset.sh \
             jemalloc/test/unit/prof_tctx.c \
             jemalloc/test/unit/prof_tctx.sh \
             jemalloc/test/unit/prof_thread_name.c \
             jemalloc/test/unit/prof_thread_name.sh\
             jemalloc/test/unit/ql.c \
             jemalloc/test/unit/qr.c \
             jemalloc/test/unit/rb.c \
             jemalloc/test/unit/retained.c \
             jemalloc/test/unit/rtree.c \
             jemalloc/test/unit/safety_check.c \
             jemalloc/test/unit/safety_check.sh \
             jemalloc/test/unit/sc.c \
             jemalloc/test/unit/seq.c \
             jemalloc/test/unit/SFMT.c \
             jemalloc/test/unit/size_classes.c \
             jemalloc/test/unit/slab.c \
             jemalloc/test/unit/smoothstep.c \
             jemalloc/test/unit/spin.c \
             jemalloc/test/unit/stats.c \
             jemalloc/test/unit/stats_print.c \
             jemalloc/test/unit/test_hooks.c \
             jemalloc/test/unit/ticker.c \
             jemalloc/test/unit/tsd.c \
             jemalloc/test/unit/witness.c \
             jemalloc/test/unit/zero.c \
             jemalloc/test/unit/zero.sh \
             jemalloc/TUNING.md \
             memkind-$(VERSION).spec \
             memkind.pc.in \
             memkind.spec.mk \
             test/test.sh \
             test/test_dax_kmem.sh \
             utils/docker/Dockerfile.fedora-32 \
             utils/docker/Dockerfile.ubuntu-20.04 \
             utils/docker/README.md \
             utils/docker/docker_install_hwloc.sh \
             utils/docker/docker_install_ndctl.sh \
             utils/docker/docker_install_tbb.sh \
             utils/docker/docker_run_build.sh \
             utils/docker/docker_run_coverage.sh \
             utils/docker/docker_run_test.sh \
             utils/docker/run_local.sh \
             utils/docker/set_host_configuration.sh \
             utils/qemu/README.md \
             utils/qemu/http/preseed.cfg \
             utils/qemu/ubuntu-packer.json \
             # end

dist_doc_DATA = COPYING README VERSION
dist_man_MANS =  man/autohbw.7 \
                 man/hbwallocator.3 \
                 man/hbwmalloc.3 \
                 man/memkind-auto-dax-kmem-nodes.1 \
                 man/memkind-hbw-nodes.1 \
                 man/memkind.3 \
                 man/memkind_arena.3 \
                 man/memkind_dax_kmem.3 \
                 man/memkind_default.3 \
                 man/memkind_hbw.3 \
                 man/memkind_hugetlb.3 \
                 man/memkind_pmem.3 \
                 man/memkindallocator.3 \
                 man/pmemallocator.3 \
                 # end

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = memkind.pc

CLEANFILES = memkind-$(VERSION).spec $(libmemkind_la_LIBADD)
DISTCLEANFILES = VERSION configure pkgconfig_DATA

bin_PROGRAMS = memkind-auto-dax-kmem-nodes memkind-hbw-nodes

memkind_auto_dax_kmem_nodes_SOURCES = src/memkind-auto-dax-kmem-nodes.c
memkind_auto_dax_kmem_nodes_LDADD = libmemkind.la
memkind_auto_dax_kmem_nodes_LDFLAGS = $(DAXCTL_LIBS)

memkind_hbw_nodes_SOURCES = src/memkind-hbw-nodes.c
memkind_hbw_nodes_LDADD = libmemkind.la

bin_SCRIPTS =
check_PROGRAMS =
noinst_PROGRAMS =
noinst_LTLIBRARIES =
TESTS =

req_flags = -fvisibility=hidden -Wall -Werror -D_GNU_SOURCE -DJE_PREFIX=@memkind_prefix@

AM_CFLAGS = $(req_flags)
AM_CXXFLAGS = $(req_flags)

clean-local: clean-jemalloc_deps

html-local:
	$(MAN2HTML) man/hbwmalloc.3 > man/hbwmalloc.html
	$(MAN2HTML) man/memkind.3 > man/memkind.html
	$(MAN2HTML) man/memkindallocator.3 > man/memkindallocator.html
	$(MAN2HTML) man/pmemallocator.3 > man/pmemallocator.html

clean-jemalloc_deps:
	[ -e jemalloc/configure ] && (cd jemalloc && $(MAKE) clean) || exit 0
	[ -e jemalloc/configure ] && (cd jemalloc && rm configure) || exit 0

.PHONY: jemalloc_deps checkprogs clean-jemalloc_deps html-local
.NOTPARALLEL: static_lib

# build check programs without running tests
checkprogs: libmemkind.la $(check_PROGRAMS)

memkind-$(VERSION).spec:
	$(MAKE) version="$(VERSION)" -f memkind.spec.mk $@

rpm: dist
	$(MAKE) version="$(VERSION)" -f memkind.spec.mk $@

all: jemalloc_deps static_lib

# the script merge memkind and jemalloc libraries into one static library.
define ar_prog
create libmemkind.a\n\
addlib .libs/libmemkind.a\n\
addlib $(libmemkind_la_LIBADD)\n\
save\n\
end
endef

static_lib: libmemkind.la
	bash -c "ar -M < <(echo -e '$(ar_prog)')"
	cp libmemkind.a .libs/
	rm libmemkind.a

JEMALLOC_CONFIG = --enable-autogen @min_lg_align_opt@ --without-export --with-version=5.2.1-0-gea6b3e973b477b8061e0076bb257dbd7f3faa756 \
			--disable-fill --disable-initial-exec-tls --with-jemalloc-prefix=@memkind_prefix@ \
			--with-malloc-conf="narenas:@auto_arenas@,lg_tcache_max:@tcache_max_size_class@" \
			#end

jemalloc_deps:
	[ ! -e jemalloc/configure ] && (cd jemalloc && autoconf) || exit 0
	[ ! -e $(libmemkind_la_LIBADD) ] && (cd jemalloc && ./configure $(JEMALLOC_CONFIG) && $(MAKE)) || exit 0

unit_tests: checkprogs
	test/all_tests --gtest_filter=*free_with_NULL_kind_4096_bytes:NegativeTest.*:ErrorMessage.*:GetArenaTest.*:MemkindVersioningTests.*
	test/decorator_test

PMEM_PATH=/tmp/
unit_tests_pmem: checkprogs
	test/pmem_test -d $(PMEM_PATH)

unit_tests_dax_kmem: checkprogs
	test/test_dax_kmem.sh

include autohbw/Makefile.mk
include test/Makefile.mk
include examples/Makefile.mk
include src/Makefile.mk
include utils/memkind_malloc_wrapper/Makefile.mk
