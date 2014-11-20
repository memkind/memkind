#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
#

name = memkind
arch = $(shell uname -p)
version = 0.0.0
release = 1

src = $(shell cat MANIFEST)

topdir = $(HOME)/rpmbuild
rpm = $(topdir)/RPMS/$(arch)/$(name)-devel-$(version)-$(release).$(arch).rpm
srpm = $(topdir)/SRPMS/$(name)-$(version)-$(release).src.rpm
specfile = $(topdir)/SPECS/$(name)-$(version).spec
source_tar = $(topdir)/SOURCES/$(name)-$(version).tar.gz

rpmbuild_flags = -E '%define _topdir $(topdir)'
rpmclean_flags = -E '%define _topdir $(topdir)' --clean --rmsource --rmspec

all: $(rpm)

$(topdir)/.setup:
	mkdir -p $(topdir)/SOURCES
	mkdir -p $(topdir)/SPECS
	touch $(topdir)/.setup

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba

$(source_tar): $(topdir)/.setup $(src) MANIFEST
	tar czvf $@ -T MANIFEST --transform="s|^|$(name)-$(version)/|"
	rpmbuild $(rpmbuild_flags) $(specfile) -bp

$(specfile): $(topdir)/.setup memkind.spec.mk
	echo "$$memkind_spec" > $@

clean:
	-rpmbuild $(rpmclean_flags) $(specfile)

.PHONY: all clean

include memkind.spec.mk

