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
rpmclean_flags = $(rpmbuild_flags) --clean --rmsource --rmspec

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba

$(source_tar): $(topdir)/.setup $(src) MANIFEST
	tar czvf $@ -T MANIFEST --transform="s|^|$(name)-$(version)/|"

$(specfile): $(topdir)/.setup memkind.spec.mk
	@echo "$$memkind_spec" > $@

$(topdir)/.setup:
	mkdir -p $(topdir)/{SOURCES,SPECS}
	touch $@

clean:
	-rpmbuild $(rpmclean_flags) $(specfile)

.PHONY: all clean

include memkind.spec.mk

