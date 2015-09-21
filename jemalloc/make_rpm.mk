name = jemalloc
arch = $(shell uname -p)
version = 0.0.0
release = 1

src = $(shell cat MANIFEST)

topdir = $(HOME)/rpmbuild
rpm = $(topdir)/RPMS/$(arch)/$(name)-devel-$(version)-$(release).$(arch).rpm
srpm = $(topdir)/SRPMS/$(name)-$(version)-$(release).src.rpm
specfile = $(topdir)/SPECS/$(name)-$(version).spec
source_tar = $(topdir)/SOURCES/$(name)-$(version).tar.gz
source_tmp_dir := $(topdir)/SOURCES/$(name)-tmp-$(shell date +%s)

rpmbuild_flags = -E '%define _topdir $(topdir)'
rpmclean_flags = $(rpmbuild_flags) --clean --rmsource --rmspec

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba

$(source_tar): $(topdir)/.setup $(src) MANIFEST
	mkdir -p $(source_tmp_dir)
	tar cf $(source_tmp_dir)/tmp.tar -T MANIFEST --transform="s|^|$(name)-$(version)/|"
	cd $(source_tmp_dir) && tar xf tmp.tar
	cd $(source_tmp_dir)/$(name)-$(version) && autoconf
	cd $(source_tmp_dir) && tar czf $@ $(name)-$(version)
	rm -rf $(source_tmp_dir)

$(specfile): $(topdir)/.setup make.spec
	@echo "$$make_spec" > $@

$(topdir)/.setup:
	mkdir -p $(topdir)/{SOURCES,SPECS}
	touch $@

clean:
	-rpmbuild $(rpmclean_flags) $(specfile)

.PHONY: all clean

include make.spec

