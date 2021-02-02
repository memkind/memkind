# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2021 Intel Corporation.

package_prefix ?=
name = $(package_prefix)memkind
arch = $(shell uname -p)
version = 0.0.0
release = 1

src = $(shell cat MANIFEST)

topdir = $(HOME)/rpmbuild
rpm = $(topdir)/RPMS/$(arch)/$(name)-devel-$(version)-$(release).$(arch).rpm
trpm = $(topdir)/TRPMS/$(arch)/$(name)-tests-$(version)-$(release).$(arch).rpm
srpm = $(topdir)/SRPMS/$(name)-$(version)-$(release).src.rpm
specfile = $(topdir)/SPECS/$(name)-$(version).spec
source_tar = $(topdir)/SOURCES/memkind-$(version).tar.gz
source_tmp_dir := $(topdir)/SOURCES/$(name)-tmp-$(shell date +%s)
rpmbuild_flags = -E '%define _topdir $(topdir)'
rpmclean_flags = $(rpmbuild_flags) --clean --rmsource --rmspec
memkind_test_dir = $(MPSS_TEST_BASEDIR)/memkind-dt
exclude_source_files = test/memkind-afts.ts \
test/memkind-afts-ext.ts \
test/memkind-slts.ts \
test/memkind-perf.ts \
test/memkind-perf-ext.ts \
test/memkind-pytests.ts \
test/python_framework/cmd_helper.py \
test/python_framework/huge_page_organizer.py \
test/python_framework/__init__.py \
test/hbw_detection_test.py \
test/hbw_env_var_test.py \
test/dax_kmem_env_var_test.py \
test/autohbw_test.py \
test/trace_mechanism_test.py \
test/draw_plots.py \
VERSION

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba
	mkdir -p $(topdir)/TRPMS/$(arch)
	mv $(topdir)/RPMS/$(arch)/$(name)-tests-$(version)-$(release).$(arch).rpm $(trpm)

$(source_tar): $(topdir)/.setup $(src) MANIFEST
	mkdir -p $(source_tmp_dir)
	set -e ; \
	for f in $(exclude_source_files); do \
		echo $$f >> $(source_tmp_dir)/EXCLUDE ; \
	done
	tar cf $(source_tmp_dir)/tmp.tar -T MANIFEST --transform="s|^|$(name)-$(version)/|" -X $(source_tmp_dir)/EXCLUDE
	cd $(source_tmp_dir) && tar xf tmp.tar
	set -e ; \
	for f in $(exclude_source_files); do \
		mkdir -p `dirname $(source_tmp_dir)/$(name)-$(version)/$$f` ; \
		cp $$f $(source_tmp_dir)/$(name)-$(version)/$$f ; \
	done
	cd $(source_tmp_dir)/$(name)-$(version) && ./autogen.sh && ./configure && make dist; \
	# tar.gz produced by "make dist" from above produces memkind-$(version).tar.gz
	# If $(package_prefix) is not empty, then need to repackage that tar.gz to $(name)-$(version)
	# thus below command. Otherwise, rpmbuild will fail.
	if [ -n "$(package_prefix)" ]; then \
	    cd $(source_tmp_dir)/$(name)-$(version) && \
	    tar xf memkind-$(version).tar.gz && \
	    rm -rf memkind-$(version).tar.gz && \
	    mv memkind-$(version) $(name)-$(version) && \
	    tar cfz memkind-$(version).tar.gz $(name)-$(version); \
	fi
	mv $(source_tmp_dir)/$(name)-$(version)/memkind-$(version).tar.gz $@
	rm -rf $(source_tmp_dir)

$(specfile): $(topdir)/.setup memkind.spec.mk
	@echo "$$memkind_spec" > $@

$(topdir)/.setup:
	mkdir -p $(topdir)/{SOURCES,SPECS}
	touch $@

clean:
	-rpmbuild $(rpmclean_flags) $(specfile)

.PHONY: all clean

include memkind.spec.mk

