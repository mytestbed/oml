ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/app/iperf/lib \
	-I $(top_srcdir)/app/iperf/cfg \
	-I $(top_srcdir)/external/ocommLib

progname = iperf_oml2

bin_PROGRAMS = $(progname)

iperf_oml2_SOURCES = lib/config.h \
    lib/Condition.hpp         \
	lib/Mutex.hpp             \
	lib/Socket.cpp            \
	lib/Socket.hpp            \
	lib/SocketAddr.cpp        \
	lib/SocketAddr.hpp        \
	lib/Thread.cpp            \
	lib/Thread.hpp            \
	lib/Timestamp.hpp         \
	lib/delay.cpp             \
	lib/delay.hpp             \
	lib/error.c               \
	lib/gettimeofday.c        \
	lib/gettimeofday.h        \
	lib/gnu_getopt.c          \
	lib/gnu_getopt.h          \
	lib/gnu_getopt_long.c     \
	lib/headers.h             \
	lib/inet_pton.c           \
	lib/inet_ntop.c           \
	lib/inet_aton.h           \
	lib/signal.c              \
	lib/snprintf.c            \
	lib/snprintf.h            \
	lib/sockets.c             \
	lib/stdio.c               \
	lib/string.c              \
	lib/tcp_window_size.c     \
	lib/util.h                \
	src/Client.cpp            \
	src/Client.hpp            \
	src/Extractor.cpp         \
	src/Extractor.hpp         \
	src/Gps.cpp               \
	src/Gps.hpp               \
	src/Listener.cpp          \
	src/Listener.hpp          \
	src/Locale.hpp            \
	src/PerfSocket.cpp        \
	src/PerfSocket.hpp        \
	src/PerfSocket_TCP.cpp    \
	src/PerfSocket_UDP.cpp    \
	src/Server.cpp            \
	src/Server.hpp            \
	src/Settings.cpp          \
	src/Settings.hpp          \
	src/Speaker.cpp           \
	src/Speaker.hpp           \
	src/Notify.cpp            \
	src/Notify.hpp            \
	src/Audience.cpp          \
	src/Audience.hpp          \
	src/List.cpp              \
	src/List.h                \
	src/main.cpp              \
	src/version.h

iperf_oml2_LDADD = \
	$(top_builddir)/client/liboml2.la \
	-ltrace -lpopt 


