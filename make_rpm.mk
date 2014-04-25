#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
#

name = numakind
arch = $(shell uname -p)
version = $(shell git describe --long | sed 's|[^0-9]*\([0-9]*\.[0-9]*\.[0-9]*\).*|\1|')
release = $(shell git describe --long | sed 's|[^0-9]*[0-9]*\.[0-9]*\.[0-9]*-\([0-9]*\).*|\1|')

src = $(shell cat MANIFEST)

topdir = $(HOME)/rpmbuild
rpm = $(topdir)/RPMS/$(arch)/$(name)-$(version)-$(release).$(arch).rpm
srpm = $(topdir)/SRPMS/$(name)-$(version)-$(release).src.rpm
specfile = $(topdir)/SPECS/$(name)-$(version).spec
source_tar = $(topdir)/SOURCES/$(name)-$(version).tar.gz

rpmbuild_flags = -E '%define _topdir $(topdir)'
rpmclean_flags = -E '%define _topdir $(topdir)' --clean --rmsource --rmspec
ifeq ($(jemalloc_installed),true)
	rpmbuild_flags += -E '%define jemalloc_installed true'
endif

include make.spec

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba

$(source_tar): $(topdir) $(specfile) $(src) MANIFEST
	if [ -n "$(revision)" ]; then \
	  git archive $(revision) -o $@; \
	else \
	  tar czvf $@ $(src); \
	fi ; \
	rpmbuild $(rpmbuild_flags) $(specfile) -bp

$(specfile): $(topdir) make.spec
	@echo "$$make_spec" > $@

$(topdir):
	mkdir -p $@/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

clean:
	-rpmbuild $(rpmclean_flags) $(specfile)

test:
	$(MAKE) test

.PHONY: all clean test
