ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

if HAVE_CHECK
if HAVE_GEM_BUNDLER
TESTS = check_gem.sh
endif
TESTS_ENVIRONMENT=TOPSRCDIR=$(top_srcdir) TOPBUILDDIR=$(top_builddir) PACKAGE_VERSION=$(PACKAGE_VERSION)
endif
EXTRA_DIST = check_gem.sh
