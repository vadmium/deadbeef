if HAVE_MPGMAD
mpgmaddir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mpgmad.la
mpgmad_la_SOURCES = mpgmad.c
mpgmad_la_LDFLAGS = -module

mpgmad_la_LIBADD = $(LDADD) $(MAD_LIBS)

AM_CFLAGS = -std=c99 -fPIC -Wall -Wextra
AM_CFLAGS += -Wno-missing-field-initializers  # Avoid GCC bug 36446
AM_CFLAGS += -Wno-unused-parameter  # Callback may have unused parameters
endif
