if HAVE_MMS
mmsdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mms.la
mms_la_SOURCES = mmsplug.c\
	libmms/mms.c\
	libmms/mmsh.c\
	libmms/mmsx.c\
	libmms/uri.c\
	libmms/asfheader.h\
	libmms/bswap.h\
	libmms/mms-common.h\
	libmms/mms.h\
	libmms/mmsh.h\
	libmms/mmsio.h\
	libmms/mmsx.h\
	libmms/uri.h

mms_la_LDFLAGS = -module

mms_la_LIBADD = $(LDADD) $(LIBMMS_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
