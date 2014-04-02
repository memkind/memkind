topdir = $(HOME)/rpmbuild
version = $(shell git describe | sed 's|[^0-9]*\([0-9]*\.[0-9]*\.[0-9]*\).*|\1|')
specfile = $(topdir)/SPECS/numakind-$(version).spec
source_tar = $(topdir)/SOURCES/numakind-$(version).tar.gz
#revision ?= $(shell git rev-parse --abbrev-ref HEAD)
arch = $(shell uname -p)
rpm = $(topdir)/RPMS/$(arch)/numakind-$(version)-1.$(arch).rpm
src = $(shell find)

include make.spec

all: $(rpm)

$(rpm): $(source_tar) $(specfile)
	rpmbuild -bc --short-circuit -E '%define _topdir $(topdir)' $(specfile)

$(source_tar): $(topdir) $(src)
	if [ -n "$(revision)" ]; then \
	  git archive $(revision) -o $@; \
	else \
	  tar cvf $@ *; \
	fi ; \
	rpmbuild -bp -E '%define _topdir $(topdir)' $(specfile)	

$(specfile): $(topdir) make.spec
	@echo "$$numakind_specfile" > $@

$(topdir):
	mkdir -p $@/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

clean:
	rm -rf $(topdir)

.PHONY: all prep clean
