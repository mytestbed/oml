ACLOCAL_AMFLAGS = -I ../../m4

ocommincdir = $(includedir)/ocomm

ocomminc_HEADERS = \
	ocomm/o_eventloop.h \
	ocomm/o_log.h \
	ocomm/o_socket_group.h \
	ocomm/o_socket.h

lib_LTLIBRARIES = libocomm.la

libocomm_la_SOURCES = \
	eventloop.c \
	log.c \
	socket.c \
	socket_group.c

libocomm_la_LIBADD = $(PTHREAD_LIBS)

# To quote the APR build scripts:
#    "Yes, ${minor}:${patch}:${minor} is correct due to libtool idiosyncrasy"
libocomm_la_LDFLAGS = -version-info $(OML_VER_MINOR_LT):$(OML_VER_PATCH_LT):$(OML_VER_MINOR_LT)
