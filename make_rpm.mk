topdir = $(HOME)/rpmbuild
name = numakind
version = $(shell git describe | sed 's|[^0-9]*\([0-9]*\.[0-9]*\.[0-9]*\).*|\1|')
release= 1
arch = $(shell uname -p)
#revision ?= $(shell git rev-parse --abbrev-ref HEAD)

rpm = $(topdir)/RPMS/$(arch)/$(name)-$(version)-$(release).$(arch).rpm
specfile = $(topdir)/SPECS/$(name)-$(version).spec
source_tar = $(topdir)/SOURCES/$(name)-$(version).tar.gz
rpmbuild = rpmbuild -E '%define _topdir $(topdir)' $(specfile)


src = $(shell find)

include make.spec

all: $(rpm)

$(rpm): $(specfile) $(source_tar)
	$(rpmbuild) -bc --short-circuit

$(source_tar): $(topdir) $(specfile) $(src)
	if [ -n "$(revision)" ]; then \
	  git archive $(revision) -o $@; \
	else \
	  tar cvf $@ *; \
	fi ; \
	$(rpmbuild) -bp

$(specfile): $(topdir) make.spec
	@echo "$$numakind_specfile" > $@

$(topdir):
	mkdir -p $@/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

clean:
	$(rpmbuild) --clean --rmsource --rmspec

.PHONY: all prep clean
