topdir = $(HOME)/rpmbuild
version = $(shell git describe | sed 's|[^0-9]*\([0-9]*\.[0-9]*\.[0-9]*\).*|\1|')
specfile = $(topdir)/SPECS/numakind-$(version).spec
source_tar = $(topdir)/SOURCES/numakind-$(version).tar.gz
gitrev ?= master

include make.spec

all: $(source_tar) $(specfile)
	rpmbuild -ba -E '%define _topdir $(topdir)' $(specfile)

$(source_tar): $(topdir)
	git archive $(gitrev) | bzip2 > $@

$(specfile): $(topdir) make.spec
	@echo "$$numakind_specfile" > $@

$(topdir):
	mkdir -p $@/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

clean:
	rm -rf $(topdir)