ACLOCAL_AMFLAGS = -I ../../m4

ocommincdir = $(includedir)/ocomm

ocomminc_HEADERS = \
	ocomm/mt_queue.h \
	ocomm/o_eventloop.h \
	ocomm/o_log.h \
	ocomm/o_socket_group.h \
	ocomm/o_socket.h \
	ocomm/queue.h

lib_LTLIBRARIES = libocomm.la

libocomm_la_SOURCES = \
	eventloop.c \
	log.c \
	mt_queue.c \
	queue.c \
	socket.c \
	socket_group.c

libocomm_la_LIBADD = -lpthread