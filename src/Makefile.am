AM_CFLAGS = \
  -I$(top_srcdir) \
  -DDATADIR=\""$(datadir)"\" \
  -D_GNU_SOURCE

AM_CFLAGS += \
  -Wall \
  -pedantic \
  -std=c99 \
  @GLIB_CFLAGS@ \
  @GLIBJSON_CFLAGS@

# add the name of the application
bin_PROGRAMS = webarg

# add the sources to compile for the application
webarg_SOURCES = \
  main.c \
  types.h \
  json.c \
  json.h \
  logging.c \
  logging.h \
  session.c \
  session.h \
  services/exec.c \
  services/exec.h

webarg_LDADD  = \
  -L../libwebsockets -lwebsockets \
  @GLIB_LIBS@ \
  @GLIBJSON_LIBS@

MAINTAINERCLEANFILES  = \
  config.h.in \
  Makefile.in

