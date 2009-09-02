ACLOCAL_AMFLAGS = -I ../../../../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/external/ocommLib \
	-I $(srcdir)/.. \
	-D WITH_OML

noinst_LTLIBRARIES = libotg2generator.la

libotg2generator_la_SOURCES = \
	factory.cpp \
	cbr_generator.cpp \
	expo_generator.cpp \
	randomvariable.cpp

libotg2generator_la_LIBADD = \
	$(top_builddir)/external/ocommLib/libocomm.la

