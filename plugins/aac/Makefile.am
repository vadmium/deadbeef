if HAVE_AAC
aacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES = aac.c aac_parser.c aac_parser.h\
mp4ff/mp4atom.c\
mp4ff/mp4ff.c\
mp4ff/mp4meta.c\
mp4ff/mp4sample.c\
mp4ff/mp4tagupdate.c\
mp4ff/mp4util.c\
mp4ff/mp4ff.h\
mp4ff/mp4ffint.h\
mp4ff/mp4ff_int_types.h

aac_la_LDFLAGS = -module

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS)
AM_CFLAGS = -std=c99 -DUSE_MP4FF -DUSE_TAGGING

# Disabled so that Automake can build the FAAD files without warnings
#AM_CFLAGS += -Wall -Wextra
#AM_CFLAGS += -Wno-missing-field-initializers  # Avoid GCC bug 36446
#AM_CFLAGS += -Wno-unused-parameter  # Callbacks may have unused parameters
#AM_CFLAGS += -Wno-missing-braces  # Avoid GCC bug 53119
endif

