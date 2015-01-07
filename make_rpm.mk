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
source_tmp_dir := $(topdir)/SOURCES/memkind-tmp-$(shell date +%s)

rpmbuild_flags = -E '%define _topdir $(topdir)'
rpmclean_flags = $(rpmbuild_flags) --clean --rmsource --rmspec
gtest_archive = /opt/mpss_toolchains/googletest/1.7.0/gtest-1.7.0.zip

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba

$(source_tar): $(topdir)/.setup $(src) MANIFEST
	mkdir -p $(source_tmp_dir)
	tar cf $(source_tmp_dir)/tmp.tar -T MANIFEST --transform="s|^|$(name)-$(version)/|"
	cd $(source_tmp_dir) && tar xf tmp.tar
	if [ -f "$(gtest_archive)" ]; then cp $(gtest_archive) $(source_tmp_dir)/$(name)-$(version); fi
	cd $(source_tmp_dir)/$(name)-$(version) && ./autogen.sh && ./configure && make dist
	mv $(source_tmp_dir)/$(name)-$(version)/$(name)-$(version).tar.gz $@
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
