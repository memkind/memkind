name = numakind

arch = $(shell uname -p)
version = $(shell git describe | sed 's|[^0-9]*\([0-9]*\.[0-9]*\.[0-9]*\).*|\1|')
release= 1

topdir = $(HOME)/rpmbuild
rpm = $(topdir)/RPMS/$(arch)/$(name)-$(version)-$(release).$(arch).rpm
srpm = $(topdir)/SRPMS/$(name)-$(version)-$(release).src.rpm
specfile = $(topdir)/SPECS/$(name)-$(version).spec
source_tar = $(topdir)/SOURCES/$(name)-$(version).tar.gz

JEPREFIX = /usr
rpmbuild_flags = -E '%define _topdir $(topdir)' -E '%define jeprefix $(JEPREFIX)'

src  = $(shell find . -iname "*\.[c|cpp|h]")
src += $(shell find . -iname "README*")
src += $(shell find . -iname "Makefile")

include make.spec

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	rpmbuild $(rpmbuild_flags) $(specfile) -ba

$(source_tar): $(topdir) $(specfile) $(src)
	if [ -n "$(revision)" ]; then \
	  git archive $(revision) -o $@; \
	else \
	  tar czvf $@ *; \
	fi ; \
	rpmbuild $(rpmbuild_flags) $(specfile) -bp

$(specfile): $(topdir) make.spec
	@echo "$$make_spec" > $@

$(topdir):
	mkdir -p $@/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

clean:
	-rpmbuild $(rpmbuild_flags) --clean --rmsource --rmspec $(specfile)

.PHONY: all clean
