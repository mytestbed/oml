ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gnulib lib server proxy_server ruby doc test

EXTRA_DIST = m4/gnulib-cache.m4 \
	     $(top_srcdir)/.version \
	     example/README \
	     example/generator.rb \
	     example/generator.c \
	     example/version.h \
	     example/Makefile \
	     example/config.xml \
	     example/config_text_stream.xml \
	     example/config_two_streams.xml \
	     m4/ax_compare_version.m4 \
	     m4/ax_prog_ruby_version.m4 \
	     m4/ax_with_prog.m4

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

pkgdata_DATA = example/README \
	       example/generator.rb \
	       example/generator.c \
	       example/version.h \
	       example/Makefile \
	       example/config.xml \
	       example/config_text_stream.xml \
	       example/config_two_streams.xml

if HAVE_GEM_BUNDLER
oml4r-gem oml4r-push:
	$(MAKE) -C ruby/ $@
endif
