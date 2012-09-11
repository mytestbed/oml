ACLOCAL_AMFLAGS = -I ../m4 -Wnone

OML4PY_EGG_SRC = \
		oml4py/CHANGES.txt \
		oml4py/LICENSE.txt \
		oml4py/oml4py.py \
		oml4py/README.txt \
		oml4py/PKG-INFO \
		oml4py/setup.py
EXTRA_DIST = \
	     oml4py/PKG-INFO.in \
	     oml4py/setup.py.in \
	     $(OML4PY_EGG_SRC)

if HAVE_PYTHON
DISTCLEANFILES = oml4py_installed_files

oml4pydir=$(pkgdatadir)/oml4py

all: oml4py-build

oml4py-build:
	cd oml4py; $(PYTHON) setup.py build

install-exec-hook:
	cd oml4py; $(PYTHON) setup.py install --prefix=$(DESTDIR)${prefix} --record ../oml4py_installed_files
oml4py_installed_files:
	cd oml4py; $(PYTHON) setup.py install -n --prefix=$(DESTDIR)${prefix} --record ../oml4py_installed_files
uninstall-local: oml4py_installed_files
	for file in `cat oml4py_installed_files`; do rm -f $${file}; done
clean-local:
	rm -rf oml4py/build oml4py/dist

if CLEANUP_OML4PY_SRC
DISTCLEANFILES += $(OML4PY_EGG_SRC)
endif

if BUILD_OML4PY_EGG
OML4PY_EGG = oml4py/dist/oml4py-$(VERSION).$(PYTHON_ARCH).tar.gz
CLEANFILES = $(OML4PY_EGG)

oml4py_DATA = $(OML4PY_EGG)

oml4py-egg: $(OML4PY_EGG)
$(OML4PY_EGG):
if CLEANUP_OML4PY_SRC
	cd $(top_builddir); ./config.status oml4py-vpath-copy
endif
	cd oml4py; $(PYTHON) setup.py bdist

oml4py-push:
	@echo "*** If the following complains that you need to be identified,"
	@echo "*** you'll need to follow the instructions at [0] and create a ~/.pypirc file [1]."
	@echo "*** [0] http://docs.python.org/distutils/packageindex.html"
	@echo "*** [1] http://docs.python.org/distutils/packageindex.html#the-pypirc-file"
	cd oml4py; $(PYTHON) setup.py sdist upload

endif #BUILD_OML4PY_EGG

.PHONY: oml4py-build oml4py-egg oml4py-push
endif #HAVE_PYTHON
