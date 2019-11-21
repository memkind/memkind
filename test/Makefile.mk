#
#  Copyright (C) 2014 - 2019 Intel Corporation.
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

AM_CPPFLAGS += -Itest/gtest_fused -DMEMKIND_DEPRECATED\(x\)=x

check_PROGRAMS += test/all_tests \
                  test/allocator_perf_tool_tests \
                  test/autohbw_test_helper \
                  test/dax_kmem_test \
                  test/decorator_test \
                  test/environ_err_dax_kmem_malloc_test \
                  test/environ_err_dax_kmem_malloc_positive_test \
                  test/environ_err_hbw_malloc_test \
                  test/freeing_memory_segfault_test \
                  test/gb_page_tests_bind_policy \
                  test/locality_test \
                  test/memkind_stat_test \
                  test/performance_test \
                  test/trace_mechanism_test_helper \
                  # end
if HAVE_CXX11
check_PROGRAMS += test/pmem_test
endif

TESTS += test/test.sh

EXTRA_DIST += test/autohbw_test.py \
              test/draw_plots.py \
              test/gtest_fused/gtest/gtest-all.cc \
              test/gtest_fused/gtest/gtest.h \
              test/hbw_detection_test.py \
              test/dax_kmem_env_var_test.py \
              test/memkind-afts-ext.ts \
              test/memkind-afts.ts \
              test/memkind-perf-ext.ts \
              test/memkind-perf.ts \
              test/memkind-pytests.ts \
              test/memkind-slts.ts \
              test/python_framework/__init__.py \
              test/python_framework/cmd_helper.py \
              test/python_framework/huge_page_organizer.py \
              test/run_alloc_benchmark.sh \
              test/trace_mechanism_test.py \
              # end



test_all_tests_LDADD = libmemkind.la
test_allocator_perf_tool_tests_LDADD = libmemkind.la
test_autohbw_test_helper_LDADD = libmemkind.la
test_dax_kmem_test_LDADD = libmemkind.la
test_decorator_test_LDADD = libmemkind.la
test_environ_err_hbw_malloc_test_LDADD = libmemkind.la
test_environ_err_dax_kmem_malloc_test_LDADD = libmemkind.la
test_environ_err_dax_kmem_malloc_positive_test_LDADD = libmemkind.la
test_freeing_memory_segfault_test_LDADD = libmemkind.la
test_gb_page_tests_bind_policy_LDADD = libmemkind.la
test_memkind_stat_test_LDADD = libmemkind.la
test_trace_mechanism_test_helper_LDADD = libmemkind.la

if HAVE_CXX11
test_pmem_test_SOURCES = $(fused_gtest) test/memkind_pmem_config_tests.cpp test/memkind_pmem_long_time_tests.cpp test/memkind_pmem_tests.cpp
test_pmem_test_LDADD = libmemkind.la
endif

fused_gtest = test/gtest_fused/gtest/gtest-all.cc \
              test/main.cpp \
              # end

test_all_tests_SOURCES = $(fused_gtest) \
                         test/Allocator.hpp \
                         test/TestPolicy.hpp \
                         test/bat_tests.cpp \
                         test/check.cpp \
                         test/check.h \
                         test/common.h \
                         test/dlopen_test.cpp \
                         test/error_message_tests.cpp \
                         test/get_arena_test.cpp \
                         test/hbw_allocator_tests.cpp \
                         test/hbw_verify_function_test.cpp \
                         test/memkind_allocator_tests.cpp \
                         test/memkind_detect_kind_tests.cpp \
                         test/memkind_null_kind_test.cpp \
                         test/memkind_versioning_tests.cpp \
                         test/multithreaded_tests.cpp \
                         test/negative_tests.cpp \
                         test/pmem_allocator_tests.cpp \
                         test/static_kinds_list.h \
                         test/static_kinds_tests.cpp \
                         test/trial_generator.cpp \
                         test/trial_generator.h \
                         #end

test_performance_test_SOURCES = $(fused_gtest) test/performance/perf_tests.cpp \
                                test/performance/perf_tests.hpp \
                                test/performance/framework.cpp \
                                test/performance/framework.hpp \
                                test/performance/operations.hpp \
                                test/performance/perf_tests.hpp

test_performance_test_LDADD = libmemkind.la

test_locality_test_SOURCES = $(fused_gtest) test/allocator_perf_tool/Allocation_info.cpp test/locality_test.cpp
test_locality_test_LDADD = libmemkind.la

test_locality_test_CPPFLAGS = $(OPENMP_CFLAGS) -O0 -Wno-error $(AM_CPPFLAGS)
test_locality_test_CXXFLAGS = $(OPENMP_CFLAGS) -O0 -Wno-error $(AM_CPPFLAGS)

test_autohbw_test_helper_SOURCES = test/autohbw_test_helper.c
test_decorator_test_SOURCES = $(fused_gtest) test/decorator_test.cpp test/decorator_test.h
test_dax_kmem_test_SOURCES = $(fused_gtest) test/memkind_dax_kmem_test.cpp
test_environ_err_hbw_malloc_test_SOURCES = test/environ_err_hbw_malloc_test.cpp
test_environ_err_dax_kmem_malloc_test_SOURCES = test/environ_err_dax_kmem_malloc_test.cpp
test_environ_err_dax_kmem_malloc_positive_test_SOURCES = test/environ_err_dax_kmem_malloc_positive_test.cpp
test_freeing_memory_segfault_test_SOURCES = $(fused_gtest) test/freeing_memory_segfault_test.cpp
test_gb_page_tests_bind_policy_SOURCES = $(fused_gtest) test/gb_page_tests_bind_policy.cpp test/trial_generator.cpp test/check.cpp
test_memkind_stat_test_SOURCES = $(fused_gtest) test/memkind_stat_test.cpp
test_trace_mechanism_test_helper_SOURCES = test/trace_mechanism_test_helper.c

#Tests based on Allocator Perf Tool
allocator_perf_tool_library_sources = test/allocator_perf_tool/AllocationSizes.hpp \
                                      test/allocator_perf_tool/Allocation_info.hpp \
                                      test/allocator_perf_tool/Allocation_info.cpp \
                                      test/allocator_perf_tool/Allocator.hpp \
                                      test/allocator_perf_tool/AllocatorFactory.hpp \
                                      test/allocator_perf_tool/CSVLogger.hpp \
                                      test/allocator_perf_tool/CommandLine.hpp \
                                      test/allocator_perf_tool/Configuration.hpp \
                                      test/allocator_perf_tool/ConsoleLog.hpp \
                                      test/allocator_perf_tool/FunctionCalls.hpp \
                                      test/allocator_perf_tool/FunctionCallsPerformanceTask.cpp \
                                      test/allocator_perf_tool/FunctionCallsPerformanceTask.h \
                                      test/allocator_perf_tool/GTestAdapter.hpp \
                                      test/allocator_perf_tool/HBWmallocAllocatorWithTimer.hpp \
                                      test/allocator_perf_tool/HugePageOrganizer.hpp \
                                      test/allocator_perf_tool/HugePageUnmap.hpp \
                                      test/allocator_perf_tool/Iterator.hpp \
                                      test/allocator_perf_tool/JemallocAllocatorWithTimer.hpp \
                                      test/allocator_perf_tool/MemkindAllocatorWithTimer.hpp \
                                      test/allocator_perf_tool/Numastat.hpp \
                                      test/allocator_perf_tool/PmemMockup.cpp \
                                      test/allocator_perf_tool/PmemMockup.hpp \
                                      test/allocator_perf_tool/Runnable.hpp \
                                      test/allocator_perf_tool/ScenarioWorkload.cpp \
                                      test/allocator_perf_tool/ScenarioWorkload.h \
                                      test/allocator_perf_tool/StandardAllocatorWithTimer.hpp \
                                      test/allocator_perf_tool/Stats.hpp \
                                      test/allocator_perf_tool/StressIncreaseToMax.cpp \
                                      test/allocator_perf_tool/StressIncreaseToMax.h \
                                      test/allocator_perf_tool/Task.hpp \
                                      test/allocator_perf_tool/TaskFactory.hpp \
                                      test/allocator_perf_tool/Tests.hpp \
                                      test/allocator_perf_tool/Thread.hpp \
                                      test/allocator_perf_tool/TimerSysTime.hpp \
                                      test/allocator_perf_tool/VectorIterator.hpp \
                                      test/allocator_perf_tool/Workload.hpp \
                                      test/allocator_perf_tool/WrappersMacros.hpp \
                                      test/memory_manager.h \
                                      test/proc_stat.h \
                                      test/random_sizes_allocator.h \
                                      # end


test_allocator_perf_tool_tests_SOURCES = $(allocator_perf_tool_library_sources) \
                                         $(fused_gtest) \
                                         test/alloc_performance_tests.cpp \
                                         test/allocate_to_max_stress_test.cpp \
                                         test/hbw_allocator_performance_tests.cpp \
                                         test/heap_manager_init_perf_test.cpp \
                                         test/huge_page_test.cpp \
                                         test/memory_footprint_test.cpp \
                                         test/pmem_alloc_performance_tests.cpp \
                                         # end


test_allocator_perf_tool_tests_CPPFLAGS = -Itest/allocator_perf_tool/ -O0 -Wno-error $(AM_CPPFLAGS)
test_allocator_perf_tool_tests_CXXFLAGS = -Itest/allocator_perf_tool/ -O0 -Wno-error $(AM_CPPFLAGS)
test_allocator_perf_tool_tests_LDFLAGS = -lpthread -lnuma

NUMAKIND_MAX = 2048
test_all_tests_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS) $(OPENMP_CFLAGS) -DNUMAKIND_MAX=$(NUMAKIND_MAX)
test_all_tests_LDFLAGS = -ldl

#Allocator Perf Tool stand alone app
check_PROGRAMS += test/perf_tool
test_perf_tool_LDADD = libmemkind.la
test_perf_tool_SOURCES = $(allocator_perf_tool_library_sources) \
                         test/allocator_perf_tool/main.cpp \
                         # end


test_perf_tool_CPPFLAGS = -Itest/allocator_perf_tool/ -O0 -Wno-error $(AM_CPPFLAGS)
test_perf_tool_CXXFLAGS = -Itest/allocator_perf_tool/ -O0 -Wno-error $(AM_CPPFLAGS)
test_perf_tool_LDFLAGS = -lpthread -lnuma
if HAVE_CXX11
test_perf_tool_CPPFLAGS += -std=c++11
test_perf_tool_CXXFLAGS += -std=c++11
endif

#Alloc benchmark
check_PROGRAMS += test/alloc_benchmark_glibc \
                  test/alloc_benchmark_hbw \
                  test/alloc_benchmark_pmem \
                  test/alloc_benchmark_tbb \
                  # end

test_alloc_benchmark_glibc_CFLAGS = -O0 -g $(OPENMP_CFLAGS) -Wall
test_alloc_benchmark_glibc_LDADD = libmemkind.la
test_alloc_benchmark_glibc_SOURCES = test/alloc_benchmark.c

test_alloc_benchmark_hbw_CFLAGS = -O0 -g $(OPENMP_CFLAGS) -Wall -DHBWMALLOC
test_alloc_benchmark_hbw_LDADD = libmemkind.la
test_alloc_benchmark_hbw_LDFLAGS = -lmemkind
test_alloc_benchmark_hbw_SOURCES = test/alloc_benchmark.c

test_alloc_benchmark_pmem_CFLAGS = -O0 -g $(OPENMP_CFLAGS) -Wall -DPMEMMALLOC
test_alloc_benchmark_pmem_LDADD = libmemkind.la
test_alloc_benchmark_pmem_LDFLAGS = -ldl
test_alloc_benchmark_pmem_SOURCES = test/alloc_benchmark.c

test_alloc_benchmark_tbb_CFLAGS = -O0 -g $(OPENMP_CFLAGS) -Wall -DTBBMALLOC
test_alloc_benchmark_tbb_LDADD = libmemkind.la
test_alloc_benchmark_tbb_LDFLAGS = -ldl
test_alloc_benchmark_tbb_SOURCES = test/alloc_benchmark.c \
                                   test/load_tbbmalloc_symbols.c \
                                   test/tbbmalloc.h \
                                   # end

# Pmem fragmentation benchmark
check_PROGRAMS += test/fragmentation_benchmark_pmem
test_fragmentation_benchmark_pmem_LDADD = libmemkind.la
test_fragmentation_benchmark_pmem_SOURCES = test/fragmentation_benchmark_pmem.cpp
test_fragmentation_benchmark_pmem_CXXFLAGS = -O0 -Wall
if HAVE_CXX11
test_fragmentation_benchmark_pmem_CXXFLAGS += -std=c++11
endif

# Examples as tests
check_PROGRAMS += test/autohbw_candidates \
                  test/filter_memkind \
                  test/hello_hbw \
                  test/hello_memkind \
                  test/hello_memkind_debug \
                  test/memkind_get_stat \
                  test/pmem_alignment \
                  test/pmem_and_dax_kmem_kind \
                  test/pmem_and_default_kind \
                  test/pmem_config \
                  test/pmem_detect_kind \
                  test/pmem_free_with_unknown_kind \
                  test/pmem_kinds \
                  test/pmem_malloc \
                  test/pmem_malloc_unlimited \
                  test/pmem_multithreads \
                  test/pmem_multithreads_onekind \
                  test/pmem_usable_size \
                  # end
if HAVE_CXX11
check_PROGRAMS += test/memkind_allocated \
                  test/memkind_cpp_allocator \
                  test/pmem_cpp_allocator
endif

test_autohbw_candidates_LDADD = libmemkind.la
test_filter_memkind_LDADD = libmemkind.la
test_hello_hbw_LDADD = libmemkind.la
test_hello_memkind_LDADD = libmemkind.la
test_hello_memkind_debug_LDADD = libmemkind.la
test_memkind_get_stat_LDADD = libmemkind.la
test_pmem_alignment_LDADD = libmemkind.la
test_pmem_and_dax_kmem_kind_LDADD = libmemkind.la
test_pmem_and_default_kind_LDADD = libmemkind.la
test_pmem_config_LDADD = libmemkind.la
test_pmem_detect_kind_LDADD = libmemkind.la
test_pmem_free_with_unknown_kind_LDADD = libmemkind.la
test_pmem_kinds_LDADD = libmemkind.la
test_pmem_malloc_LDADD = libmemkind.la
test_pmem_malloc_unlimited_LDADD = libmemkind.la
test_pmem_multithreads_LDADD = libmemkind.la
test_pmem_multithreads_onekind_LDADD = libmemkind.la
test_pmem_usable_size_LDADD = libmemkind.la
if HAVE_CXX11
test_memkind_allocated_LDADD = libmemkind.la
test_memkind_cpp_allocator_LDADD = libmemkind.la
test_pmem_cpp_allocator_LDADD = libmemkind.la
endif

test_autohbw_candidates_SOURCES = examples/autohbw_candidates.c
test_filter_memkind_SOURCES = examples/filter_example.c
test_hello_hbw_SOURCES = examples/hello_hbw_example.c
test_hello_memkind_SOURCES = examples/hello_memkind_example.c
test_hello_memkind_debug_SOURCES = examples/hello_memkind_example.c examples/memkind_decorator_debug.c
test_memkind_get_stat_SOURCES = examples/memkind_get_stat.c
test_pmem_alignment_SOURCES = examples/pmem_alignment.c
test_pmem_and_dax_kmem_kind_SOURCES = examples/pmem_and_dax_kmem_kind.c
test_pmem_and_default_kind_SOURCES = examples/pmem_and_default_kind.c
test_pmem_config_SOURCES = examples/pmem_config.c
test_pmem_detect_kind_SOURCES = examples/pmem_detect_kind.c
test_pmem_free_with_unknown_kind_SOURCES = examples/pmem_free_with_unknown_kind.c
test_pmem_kinds_SOURCES = examples/pmem_kinds.c
test_pmem_malloc_SOURCES = examples/pmem_malloc.c
test_pmem_malloc_unlimited_SOURCES = examples/pmem_malloc_unlimited.c
test_pmem_multithreads_SOURCES = examples/pmem_multithreads.c
test_pmem_multithreads_onekind_SOURCES = examples/pmem_multithreads_onekind.c
test_pmem_usable_size_SOURCES = examples/pmem_usable_size.c

test_libautohbw_la_SOURCES = autohbw/autohbw.c
noinst_LTLIBRARIES += test/libautohbw.la
if HAVE_CXX11
test_memkind_allocated_SOURCES = examples/memkind_allocated_example.cpp examples/memkind_allocated.hpp
test_memkind_cpp_allocator_SOURCES = examples/memkind_cpp_allocator.cpp
test_pmem_cpp_allocator_SOURCES = examples/pmem_cpp_allocator.cpp
endif

clean-local: test-clean

test-clean:
	find test \( -name "*.gcda" -o -name "*.gcno" \) -type f -delete
