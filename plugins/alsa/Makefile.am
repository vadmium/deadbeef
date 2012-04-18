if HAVE_ALSA
alsadir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = alsa.la
alsa_la_SOURCES = alsa.c
alsa_la_LDFLAGS = -module
alsa_la_LIBADD = $(LDADD) $(ALSA_DEPS_LIBS)

AM_CFLAGS = -std=c99 $(ALSA_DEPS_CFLAGS) -Wall -Wextra
AM_CFLAGS += -Wno-missing-field-initializers  # Avoid GCC bug 36446
AM_CFLAGS += -Wno-unused-parameter  # Callback may have unused parameters
endif
