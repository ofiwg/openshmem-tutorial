noinst_PROGRAMS += sping
sping_SOURCES = examples/sping.c
sping_LDADD = libshmem.la
sping_CFLAGS = -I${srcdir}/src
