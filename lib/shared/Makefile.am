ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/shared \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm # Needed for logging functions, see #972

noinst_LTLIBRARIES = libshared.la

libshared_la_SOURCES = \
	marshal.c \
	marshal.h \
	mbuf.c \
	mbuf.h \
	cbuf.c \
	cbuf.h \
	mstring.c \
	mstring.h \
	mem.c \
	mem.h \
	oml_value.c \
	oml_value.h \
	validate.c \
	validate.h \
	schema.c \
	schema.h \
	headers.c \
	headers.h \
	message.h \
	binary.c \
	binary.h \
	text.c \
	text.h \
	oml_util.c \
	oml_util.h \
	htonll.h \
	base64.c \
	base64.h \
	string_utils.c \
	string_utils.h \
	guid.c \
	guid.h \
	json.c \
	json.h