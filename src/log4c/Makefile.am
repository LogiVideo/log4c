INCLUDES = \
	-I$(top_srcdir)/src \
	-DLOG4C_RCPATH="\"$(sysconfdir)\"" \
	@EXPAT_CFLAGS@

lib_LTLIBRARIES  = liblog4c.la

liblog4c_la_SOURCES = \
	rc.c \
	rc.h \
	\
	init.c \
	appender_type_stream.c \
	appender_type_syslog.c \
	appender_type_mmap.c \
	layout_type_basic.c \
	layout_type_dated.c \
	\
	version.c \
	logging_event.c \
	priority.c \
	appender.c \
	layout.c \
	category.c

liblog4c_la_LDFLAGS = -version-info @LT_VERSION@
liblog4c_la_LIBADD  = @EXPAT_LIBS@ ../sd/liblog4c_sd.la

pkginclude_HEADERS = \
	init.h \
	defs.h \
	version.h \
	location_info.h \
	logging_event.h \
	priority.h \
	layout_type_basic.h \
	layout_type_dated.h \
	layout.h \
	appender_type_stream.h \
	appender_type_syslog.h \
	appender_type_mmap.h \
	appender.h \
	category.h